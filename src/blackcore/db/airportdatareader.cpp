/* Copyright (C) 2016
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "airportdatareader.h"
#include "blackcore/db/databaseutils.h"
#include "blackcore/application.h"
#include "blackmisc/network/networkutils.h"
#include "blackmisc/logmessage.h"
#include <QNetworkReply>

using namespace BlackMisc;
using namespace BlackMisc::Aviation;
using namespace BlackMisc::Network;
using namespace BlackMisc::Db;

namespace BlackCore
{
    namespace Db
    {
        CAirportDataReader::CAirportDataReader(QObject *parent, const CDatabaseReaderConfigList &config) :
            CDatabaseReader(parent, config, QStringLiteral("CAirportDataReader"))
        {
            // void
        }

        BlackMisc::Aviation::CAirportList CAirportDataReader::getAirports() const
        {
            return m_airportCache.get();
        }

        int CAirportDataReader::getAirportsCount() const
        {
            return this->getAirports().size();
        }

        bool CAirportDataReader::readFromJsonFilesInBackground(const QString &dir, CEntityFlags::Entity whatToRead)
        {
            if (dir.isEmpty() || whatToRead == CEntityFlags::NoEntity) { return false; }
            QTimer::singleShot(0, this, [this, dir, whatToRead]()
            {
                const CStatusMessageList msgs = this->readFromJsonFiles(dir, whatToRead);
                if (msgs.isFailure())
                {
                    CLogMessage::preformatted(msgs);
                }
            });
            return true;
        }

        CStatusMessageList CAirportDataReader::readFromJsonFiles(const QString &dir, CEntityFlags::Entity whatToRead)
        {
            const QString fileName = CFileUtils::appendFilePaths(dir, "airports.json");
            if (!QFile::exists(fileName))
            {
                return CStatusMessage(this).error("File '%1' does not exist") << fileName;
            }
            whatToRead &= CEntityFlags::AirportEntity; // can handle these entities
            if (whatToRead == CEntityFlags::NoEntity)
            {
                return CStatusMessage(this).info("'%1' No entity for this reader") << CEntityFlags::flagToString(whatToRead);
            }

            int c = 0;
            CEntityFlags::Entity reallyRead = CEntityFlags::NoEntity;
            const QJsonObject airportsJson(CDatabaseUtils::readQJsonObjectFromDatabaseFile(fileName));
            if (!airportsJson.isEmpty())
            {
                try
                {
                    CAirportList airports;
                    airports.convertFromJson(airportsJson);
                    c = airports.size();
                    this->m_airportCache.set(airports);

                    emit dataRead(CEntityFlags::AirportEntity, CEntityFlags::ReadFinished, c);
                    reallyRead |= CEntityFlags::AirportEntity;
                }
                catch (const CJsonException &ex)
                {
                    emit dataRead(CEntityFlags::AirportEntity, CEntityFlags::ReadFailed, 0);
                    return ex.toStatusMessage(this, QString("Reading airports from '%1'").arg(fileName));
                }
            }
            if ((reallyRead & CEntityFlags::AirportEntity) == CEntityFlags::AirportEntity)
            {
                return CStatusMessage(this).info("Written %1 airports from '%2' to cache") << c << fileName;
            }
            else
            {
                return CStatusMessage(this).error("Not able to read airports from '%1'") << fileName;
            }
        }

        CEntityFlags::Entity CAirportDataReader::getSupportedEntities() const
        {
            return CEntityFlags::AirportEntity;
        }

        QDateTime CAirportDataReader::getCacheTimestamp(CEntityFlags::Entity entities) const
        {
            return entities == CEntityFlags::AirportEntity ? m_airportCache.getAvailableTimestamp() : QDateTime();
        }

        int CAirportDataReader::getCacheCount(CEntityFlags::Entity entity) const
        {
            return entity == CEntityFlags::AirportEntity ? m_airportCache.get().size() : 0;
        }

        void CAirportDataReader::synchronizeCaches(CEntityFlags::Entity entities)
        {
            if (entities.testFlag(CEntityFlags::AirportEntity)) { this->m_airportCache.synchronize(); }
        }

        void CAirportDataReader::admitCaches(CEntityFlags::Entity entities)
        {
            if (entities.testFlag(CEntityFlags::AirportEntity)) { this->m_airportCache.admit(); }
        }

        void CAirportDataReader::invalidateCaches(CEntityFlags::Entity entities)
        {
            if (entities.testFlag(CEntityFlags::AirportEntity)) { CDataCache::instance()->clearAllValues(this->m_airportCache.getKey()); }
        }

        bool CAirportDataReader::hasChangedUrl(CEntityFlags::Entity entity) const
        {
            Q_UNUSED(entity);
            return CDatabaseReader::isChangedUrl(this->m_readerUrlCache.get(), getBaseUrl(CDbFlags::DbReading));
        }

        CUrl CAirportDataReader::getDbServiceBaseUrl() const
        {
            return sApp->getGlobalSetup().getDbAirportReaderUrl();
        }

        CUrl CAirportDataReader::getAirportsUrl(CDbFlags::DataRetrievalModeFlag mode) const
        {
            return getBaseUrl(mode).withAppendedPath(fileNameForMode(CEntityFlags::AirportEntity, mode));
        }

        void CAirportDataReader::ps_parseAirportData(QNetworkReply *nwReplyPtr)
        {
            // wrap pointer, make sure any exit cleans up reply
            // required to use delete later as object is created in a different thread
            QScopedPointer<QNetworkReply, QScopedPointerDeleteLater> nwReply(nwReplyPtr);
            if (!this->doWorkCheck()) { return; }

            const CDatabaseReader::JsonDatastoreResponse res = this->setStatusAndTransformReplyIntoDatastoreResponse(nwReplyPtr);
            if (res.hasErrorMessage())
            {
                CLogMessage::preformatted(res.lastWarningOrAbove());
                emit dataRead(CEntityFlags::AirportEntity, CEntityFlags::ReadFailed, 0);
                return;
            }

            CAirportList airports;
            CAirportList inconsistent;
            if (res.isRestricted())
            {
                const CAirportList incrementalAirports(CAirportList::fromDatabaseJson(res, &inconsistent));
                if (incrementalAirports.isEmpty()) { return; } // currently ignored
                airports = this->getAirports();
                airports.replaceOrAddObjectsByKey(incrementalAirports);
            }
            else
            {
                airports = CAirportList::fromDatabaseJson(res, &inconsistent);
            }

            if (!inconsistent.isEmpty())
            {
                logInconsistentData(
                    CStatusMessage(this, CStatusMessage::SeverityInfo, "Inconsistent airports: " + inconsistent.dbKeysAsStrings(", ")),
                    Q_FUNC_INFO);
            }

            const int size = airports.size();
            qint64 latestTimestamp = airports.latestTimestampMsecsSinceEpoch();
            if (size > 0 && latestTimestamp < 0)
            {
                CLogMessage(this).error("No timestamp in airport list, setting to last modified value");
                latestTimestamp = lastModifiedMsSinceEpoch(nwReply.data());
            }

            this->m_airportCache.set(airports, latestTimestamp);
            this->updateReaderUrl(getBaseUrl(CDbFlags::DbReading));

            this->emitAndLogDataRead(CEntityFlags::AirportEntity, size, res);
        }

        void CAirportDataReader::ps_read(CEntityFlags::Entity entity, CDbFlags::DataRetrievalModeFlag mode, const QDateTime &newerThan)
        {
            this->threadAssertCheck();
            if (!this->doWorkCheck()) { return; }
            entity &= CEntityFlags::AirportEntity;
            if (!this->isNetworkAccessible())
            {
                emit this->dataRead(entity, CEntityFlags::ReadSkipped, 0);
                return;
            }

            if (entity.testFlag(CEntityFlags::AirportEntity))
            {
                CUrl url = getAirportsUrl(mode);
                if (!url.isEmpty())
                {
                    url.appendQuery(queryLatestTimestamp(newerThan));
                    sApp->getFromNetwork(url, { this, &CAirportDataReader::ps_parseAirportData });
                    emit dataRead(CEntityFlags::AirportEntity, CEntityFlags::StartRead, 0);
                }
                else
                {
                    CLogMessage(this).error("No URL for %1") << CEntityFlags::flagToString(CEntityFlags::AirportEntity);
                }
            }
        }

        void CAirportDataReader::ps_airportCacheChanged()
        {
            this->cacheHasChanged(CEntityFlags::AirportEntity);
        }

        void CAirportDataReader::ps_baseUrlCacheChanged()
        {
            // void
        }

        void CAirportDataReader::updateReaderUrl(const CUrl &url)
        {
            const CUrl current = this->m_readerUrlCache.get();
            if (current == url) { return; }
            const CStatusMessage m = this->m_readerUrlCache.set(url);
            if (m.isFailure())
            {
                CLogMessage::preformatted(m);
            }
        }
    } // ns
} // ns

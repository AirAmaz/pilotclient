/* Copyright (C) 2015
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "blackcore/vatsim/vatsimmetarreader.h"
#include "blackcore/application.h"
#include "blackmisc/compare.h"
#include "blackmisc/logmessage.h"
#include "blackmisc/network/entityflags.h"
#include "blackmisc/network/url.h"
#include "blackmisc/network/urllist.h"
#include "blackmisc/statusmessage.h"

#include <QByteArray>
#include <QMetaObject>
#include <QNetworkReply>
#include <QReadLocker>
#include <QScopedPointer>
#include <QScopedPointerDeleteLater>
#include <QString>
#include <QTextStream>
#include <QTimer>
#include <QUrl>
#include <QWriteLocker>
#include <QtGlobal>

using namespace BlackMisc;
using namespace BlackMisc::Network;
using namespace BlackMisc::Weather;
using namespace BlackCore::Data;

namespace BlackCore
{
    namespace Vatsim
    {
        CVatsimMetarReader::CVatsimMetarReader(QObject *owner) :
            CThreadedReader(owner, "CVatsimMetarReader")
        {
            reloadSettings();
        }

        void CVatsimMetarReader::readInBackgroundThread()
        {
            QTimer::singleShot(0, this, &CVatsimMetarReader::readMetars);
        }

        CMetarList CVatsimMetarReader::getMetars() const
        {
            QReadLocker l(&m_lock);
            return m_metars;
        }

        CMetar CVatsimMetarReader::getMetarForAirport(const Aviation::CAirportIcaoCode &icao) const
        {
            QReadLocker l(&m_lock);
            return m_metars.getMetarForAirport(icao);
        }

        int CVatsimMetarReader::getMetarsCount() const
        {
            QReadLocker l(&m_lock);
            return m_metars.size();
        }

        void CVatsimMetarReader::cleanup()
        {
            // void
        }

        void CVatsimMetarReader::doWorkImpl()
        {
            readMetars();
        }

        void CVatsimMetarReader::readMetars()
        {
            if (this->isAbandoned()) { return; }
            if (!this->isNetworkConnectedAndAccessible())
            {
                CLogMessage(this).warning("No network, cannot read METARs");
                return;
            }
            this->threadAssertCheck();

            CFailoverUrlList urls(sApp->getVatsimMetarUrls());
            const CUrl url(urls.obtainNextWorkingUrl(true));
            if (url.isEmpty()) { return; }
            Q_ASSERT_X(sApp, Q_FUNC_INFO, "No Application");
            sApp->getFromNetwork(url.withAppendedQuery("id=all"), { this, &CVatsimMetarReader::decodeMetars});
        }

        void CVatsimMetarReader::decodeMetars(QNetworkReply *nwReplyPtr)
        {
            // wrap pointer, make sure any exit cleans up reply
            // required to use delete later as object is created in a different thread
            QScopedPointer<QNetworkReply, QScopedPointerDeleteLater> nwReply(nwReplyPtr);

            this->threadAssertCheck();

            // Worker thread, make sure to write thread safe!
            if (this->isAbandoned())
            {
                CLogMessage(this).debug() << Q_FUNC_INFO;
                CLogMessage(this).info("terminated METAR decoding process"); // for users
                return; // stop, terminate straight away, ending thread
            }

            if (nwReply->error() == QNetworkReply::NoError)
            {
                QString metarData = nwReply->readAll();
                nwReply->close(); // close asap

                if (!this->didContentChange(metarData)) // Quick check by hash
                {
                    CLogMessage(this).info("METAR file has same content, skipped");
                    return;
                }

                CMetarList metars;
                int invalidLines = 0;
                QTextStream lineReader(&metarData);
                while (!lineReader.atEnd())
                {
                    if (this->isAbandoned()) { return; }
                    QString line = lineReader.readLine();
                    CMetar metar = m_metarDecoder.decode(line);
                    if (metar != CMetar()) { metars.push_back(metar); }
                    else { invalidLines++; }
                }

                CLogMessage(this).debug() << "METAR statistic: " << metars.size() << "Metars ( invalid:" << invalidLines << ")";

                {
                    QWriteLocker l(&m_lock);
                    m_metars = metars;
                }

                emit metarsRead(metars);
                emit dataRead(CEntityFlags::MetarEntity, CEntityFlags::ReadFinished, metars.size());
            }
            else
            {
                // network error
                CLogMessage(this).warning("Reading METARs failed %1 %2") << nwReply->errorString() << nwReply->url().toString();
                nwReply->abort();
                emit dataRead(CEntityFlags::MetarEntity, CEntityFlags::ReadFailed, 0);
            }
        } // method

        void CVatsimMetarReader::reloadSettings()
        {
            CReaderSettings s = m_settings.get();
            setInitialAndPeriodicTime(s.getInitialTime().toMs(), s.getPeriodicTime().toMs());
        }

    } // ns
} // ns

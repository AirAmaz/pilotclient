/* Copyright (C) 2015
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "blackcore/application.h"
#include "blackcore/data/globalsetup.h"
#include "blackcore/db/databasewriter.h"
#include "blackmisc/db/datastoreutility.h"
#include "blackmisc/logcategory.h"
#include "blackmisc/logcategorylist.h"
#include "blackmisc/network/networkutils.h"
#include "blackmisc/statusmessage.h"

#include <QHttpMultiPart>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QScopedPointer>
#include <QScopedPointerDeleteLater>
#include <QString>
#include <QUrl>
#include <QtGlobal>

using namespace BlackMisc;
using namespace BlackMisc::Network;
using namespace BlackMisc::Simulation;

namespace BlackCore
{
    namespace Db
    {
        CDatabaseWriter::CDatabaseWriter(const Network::CUrl &baseUrl, QObject *parent) :
            QObject(parent),
            m_modelPublishUrl(getModelPublishUrl(baseUrl))
        {
            // void
        }

        CStatusMessageList CDatabaseWriter::asyncPublishModel(const CAircraftModel &model)
        {
            return this->asyncPublishModels(CAircraftModelList({ model }));
        }

        CStatusMessageList CDatabaseWriter::asyncPublishModels(const CAircraftModelList &models)
        {
            CStatusMessageList msgs;
            if (m_shutdown)
            {
                msgs.push_back(CStatusMessage(CStatusMessage::SeverityWarning, "Database writer shuts down"));
                return msgs;
            }

            if (this->isReplyOverdue())
            {
                bool killed = this->killPendingReply();
                if (killed)
                {
                    const CStatusMessage msg(CStatusMessage::SeverityWarning, "Aborted outdated pending reply");
                    msgs.push_back(CStatusMessage(msg));
                    // need to let a potential receiver know it has failed
                    emit this->publishedModels(CAircraftModelList(), CAircraftModelList(), msg, false, false);
                }
            }

            if (m_pendingReply)
            {
                msgs.push_back(CStatusMessage(CStatusMessage::SeverityWarning, "Another write operation in progress"));
                return msgs;
            }

            QUrl url(m_modelPublishUrl.toQUrl());
            QNetworkRequest request(url);
            CNetworkUtils::ignoreSslVerification(request);
            QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType, this);
            multiPart->append(CNetworkUtils::getJsonTextMultipart(models.toDatabaseJson()));
            if (sApp->getGlobalSetup().dbDebugFlag())
            {
                multiPart->append(CNetworkUtils::getMultipartWithDebugFlag());
            }

            m_pendingReply = sApp->postToNetwork(request, multiPart, { this, &CDatabaseWriter::ps_postModelsResponse});
            m_replyPendingSince = QDateTime::currentMSecsSinceEpoch();
            return msgs;
        }

        void CDatabaseWriter::gracefulShutdown()
        {
            m_shutdown = true;
            this->killPendingReply();
        }

        void CDatabaseWriter::ps_postModelsResponse(QNetworkReply *nwReplyPtr)
        {
            static const CLogCategoryList cats(CLogCategoryList(this).join({ CLogCategory::swiftDbWebservice()}));
            QScopedPointer<QNetworkReply, QScopedPointerDeleteLater> nwReply(nwReplyPtr);
            m_pendingReply = nullptr;
            QUrl url(nwReply->url());
            QString urlString(url.toString());

            if (m_shutdown)
            {
                nwReply->abort();
                return;
            }

            if (nwReply->error() == QNetworkReply::NoError)
            {
                const QString dataFileData(nwReply->readAll().trimmed());
                nwReply->close(); // close asap
                if (dataFileData.isEmpty())
                {
                    const CStatusMessageList msgs({CStatusMessage(cats, CStatusMessage::SeverityError, "No response data from " + urlString)});
                    emit publishedModels(CAircraftModelList(), CAircraftModelList(), msgs, false, false);
                    return;
                }

                CAircraftModelList modelsPublished;
                CAircraftModelList modelsSkipped;
                CStatusMessageList msgs;
                bool directWrite;
                const bool sendingSuccessful = CDatastoreUtility::parseSwiftPublishResponse(dataFileData, modelsPublished, modelsSkipped, msgs, directWrite);
                emit publishedModels(modelsPublished, modelsSkipped, msgs, sendingSuccessful, directWrite);
            }
            else
            {
                QString error = nwReply->errorString();
                nwReply->close(); // close asap
                const CStatusMessageList msgs( {CStatusMessage(cats, CStatusMessage::SeverityError, "HTTP error: " + error)});
                emit publishedModels(CAircraftModelList(), CAircraftModelList(), msgs, false, false);
            }
        }

        bool CDatabaseWriter::killPendingReply()
        {
            if (!m_pendingReply) { return false; }
            m_pendingReply->abort();
            m_pendingReply = nullptr;
            m_replyPendingSince = -1;
            return true;
        }

        bool CDatabaseWriter::isReplyOverdue() const
        {
            if (m_replyPendingSince < 0 || !m_pendingReply) { return false; }
            qint64 ms = QDateTime::currentMSecsSinceEpoch() - m_replyPendingSince;
            return ms > 7500;
        }

        CUrl CDatabaseWriter::getModelPublishUrl(const Network::CUrl &baseUrl)
        {
            return baseUrl.withAppendedPath("service/publishmodels.php");
        }

        QList<QByteArray> CDatabaseWriter::splitData(const QByteArray &data, int size)
        {
            if (data.size() <= size) { return QList<QByteArray>({data}); }
            int pos = 0, arrsize = data.size();
            QList<QByteArray> arrays;
            while (pos < arrsize)
            {
                QByteArray arr = data.mid(pos, size);
                arrays << arr;
                pos += arr.size();
            }
            return arrays;
        }
    } // ns
} // ns

/* Copyright (C) 2017
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "networkwatchdog.h"
#include "application.h"
#include "blackcore/data/globalsetup.h"
#include "blackmisc/network/networkutils.h"
#include <QNetworkReply>

using namespace BlackMisc;
using namespace BlackMisc::Network;
using namespace BlackCore::Data;

namespace BlackCore
{
    namespace Db
    {
        CNetworkWatchdog::CNetworkWatchdog(QObject *parent) : CContinuousWorker(parent, "swift DB watchdog")
        {
            Q_ASSERT_X(sApp, Q_FUNC_INFO, "Need sApp");
            const bool network = sApp->isNetworkAccessible(); // default
            m_networkAccessible = network;
            m_dbAccessible = network && m_checkDbAccessibility;
            m_internetAccessible = network;

            if (network)
            {
                this->initWorkingSharedUrlFromSetup();
            }

            m_updateTimer.setInterval(10 * 1000);
            connect(&m_updateTimer, &QTimer::timeout, this, &CNetworkWatchdog::doWork);
        }

        void CNetworkWatchdog::setDbAccessibility(bool accessible)
        {
            m_dbAccessible = accessible;
            m_internetAccessible = m_internetAccessible && m_networkAccessible;
            QTimer::singleShot(0, &m_updateTimer, [this] { this->m_updateTimer.start(); }); // restart
        }

        CUrl CNetworkWatchdog::getWorkingSharedUrl() const
        {
            if (!m_networkAccessible) return CUrl();
            QReadLocker l(&m_lockSharedUrl);
            return m_workingSharedUrl;
        }

        int CNetworkWatchdog::triggerCheck()
        {
            if (!this->doWorkCheck()) return false; // senseless
            if (m_checkInProgress) { return -1; }

            const int n = this->getCheckCount();
            QTimer::singleShot(0, this, &CNetworkWatchdog::doWork);
            return n; // triggered
        }

        void CNetworkWatchdog::setWorkingSharedUrl(const CUrl &workingUrl)
        {
            QWriteLocker l(&m_lockSharedUrl);
            m_workingSharedUrl = workingUrl;
        }

        bool CNetworkWatchdog::isDbUrl(const CUrl &url)
        {
            const QString host(url.getHost());
            return host == dbHost();
        }

        void CNetworkWatchdog::doWork()
        {
            if (!this->doWorkCheck()) { return; }
            if (m_checkInProgress) { return; }
            m_checkInProgress = true;

            do
            {
                const bool wasDbAvailable = m_dbAccessible;
                const bool wasInternetAvailable = m_internetAccessible;
                const bool networkAccess = m_networkAccessible;
                const bool canConnectDb = m_checkDbAccessibility && networkAccess &&
                                          CNetworkUtils::canConnect(dbTestUrl()); // running in background worker
                bool canConnectInternet = canConnectDb;
                bool checkInternetAccess = !canConnectDb;

                m_dbAccessible = canConnectDb;
                if (canConnectDb)
                {
                    // DB available means internet available
                    m_internetAccessible = canConnectDb;
                }

                // check shared URL
                if (!this->doWorkCheck()) { break; }
                if (m_checkSharedUrl && networkAccess)
                {
                    if (CNetworkUtils::canConnect(this->getWorkingSharedUrl()))
                    {
                        canConnectInternet = true;
                        checkInternetAccess = false;
                    }
                    else
                    {
                        const CUrl sharedUrl = this->getWorkingSharedUrl();
                        if (!sharedUrl.isEmpty())
                        {
                            canConnectInternet = true;
                            checkInternetAccess = false;
                            this->setWorkingSharedUrl(sharedUrl);
                        }
                    }
                }

                // check internet access
                if (!this->doWorkCheck()) { break; }
                if (checkInternetAccess)
                {
                    QString message;
                    static const QString testHost1("www.google.com"); // what else?
                    canConnectInternet = CNetworkUtils::canConnect(testHost1, 443, message); // running in background worker
                    if (!canConnectInternet)
                    {
                        static const QString testHost2("www.microsoft.com"); // secondary test
                        canConnectInternet = CNetworkUtils::canConnect(testHost2, 80, message); // running in background worker
                    }
                }
                m_internetAccessible = networkAccess && canConnectInternet;

                // signals
                m_checkCount++;
                this->triggerChangedSignals(wasDbAvailable, wasInternetAvailable);
            }
            while (false);

            m_updateTimer.start(); // restart
            m_checkInProgress = false;
        }

        bool CNetworkWatchdog::doWorkCheck() const
        {
            if (!sApp) { return false; }
            if (sApp->isShuttingDown()) { return false; }
            if (!this->isEnabled()) { return false; }
            return true;
        }

        void CNetworkWatchdog::onChangedNetworkAccessibility(QNetworkAccessManager::NetworkAccessibility accessible)
        {
            const bool db = m_dbAccessible;
            const bool internet = m_internetAccessible;

            // Intentionally rating unknown as "accessible"
            if (accessible == QNetworkAccessManager::NotAccessible)
            {
                m_networkAccessible = false;
                m_dbAccessible = false;
                m_internetAccessible = false;
                this->triggerChangedSignals(db, internet);
            }
            else
            {
                m_networkAccessible = true;
                QTimer::singleShot(0, this, &CNetworkWatchdog::doWork);
            }
        }

        void CNetworkWatchdog::gracefulShutdown()
        {
            this->pingDbClientService(PingCompleteShutdown);
        }

        void CNetworkWatchdog::pingDbClientService(CNetworkWatchdog::PingType type)
        {
            if (!this->isSwiftDbAccessible()) { return; }
            if (!sApp) { return; }
            const CGlobalSetup gs = sApp->getGlobalSetup();
            if (!gs.wasLoaded()) { return; }
            CUrl pingUrl = gs.getDbClientPingServiceUrl();
            if (pingUrl.isEmpty()) { return; }

            pingUrl.appendQuery("uuid", this->identifier().toUuidString());
            pingUrl.appendQuery("application", sApp->getApplicationNameAndVersion());
            if (type.testFlag(PingLogoff)) { pingUrl.appendQuery("logoff", "true"); }
            if (type.testFlag(PingShutdown)) { pingUrl.appendQuery("shutdown", "true"); }
            if (type.testFlag(PingStarted)) { pingUrl.appendQuery("started", "true"); }

            sApp->getFromNetwork(pingUrl, { this, &CNetworkWatchdog::replyPingClientService });
        }

        void CNetworkWatchdog::replyPingClientService(QNetworkReply *nwReply)
        {
            QScopedPointer<QNetworkReply> nw(nwReply); // delete reply
            if (!sApp || sApp->isShuttingDown()) { return; }
            const bool ok = nw->error() == QNetworkReply::NoError;
            this->setDbAccessibility(ok);
        }

        void CNetworkWatchdog::triggerChangedSignals(bool oldDbAccessible, bool oldInternetAccessible)
        {
            if (!this->doWorkCheck()) { return; }

            // trigger really queued
            if (oldDbAccessible != m_dbAccessible)
            {
                QTimer::singleShot(0, this, [this] { emit this->changedSwiftDbAccessibility(m_dbAccessible);});
            }
            if (oldInternetAccessible != m_internetAccessible)
            {
                QTimer::singleShot(0, this, [this] { emit this->changedInternetAccessibility(m_internetAccessible);});
            }
        }

        void CNetworkWatchdog::initWorkingSharedUrlFromSetup()
        {
            const CUrl workingUrl(CNetworkWatchdog::workingSharedUrlFromSetup()); // takes long
            this->setWorkingSharedUrl(workingUrl);
        }

        BlackMisc::Network::CUrl CNetworkWatchdog::dbTestUrl()
        {
            // requires global setup to be read
            const CUrl testUrl(sApp->getGlobalSetup().getDbHomePageUrl());
            return testUrl;
        }

        QString CNetworkWatchdog::dbHost()
        {
            const QString host = dbTestUrl().getHost();
            return host;
        }

        CUrl CNetworkWatchdog::workingSharedUrlFromSetup()
        {
            const CUrlList urls(sApp->getGlobalSetup().getSwiftSharedUrls());
            CFailoverUrlList failoverUrls(urls);
            return failoverUrls.getRandomWorkingUrl(); // use CNetworkUtils::canConnect
        }
    } // ns
} // ns

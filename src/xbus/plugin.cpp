/* Copyright (C) 2013
 * swift Project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "plugin.h"
#include "service.h"
#include "traffic.h"
#include "weather.h"
#include "utils.h"
#include "blackmisc/librarypath.h"
#include "blackmisc/logmessage.h"
#include <QTimer>
#include <functional>

namespace {
    inline QString xbusServiceName() {
        return QStringLiteral("org.swift-project.xbus");
    }
}

namespace XBus
{

    CPlugin::CPlugin()
        : m_menu(CMenu::mainMenu().subMenu("XBus"))
    {
        m_startServerMenuItems.push_back(m_menu.item("Start server on session bus", [this]{ startServer(BlackMisc::CDBusServer::sessionBusAddress()); }));
        m_startServerMenuItems.push_back(m_menu.item("Start server on system bus", [this]{ startServer(BlackMisc::CDBusServer::systemBusAddress()); }));
        m_startServerMenuItems.push_back(m_menu.item("Start server on localhost P2P", [this]{ startServer(BlackMisc::CDBusServer::p2pAddress("localhost")); }));
    }

    void CPlugin::startServer(const QString &address)
    {
        for (auto &item : m_startServerMenuItems) { item.setEnabled(false); }

        m_service = new CService(this);
        m_traffic = new CTraffic(this);
        m_weather = new CWeather(this);

        // XPLM API does not like to be called from a QTimer slot, so move the recurring part into a separate method.
        tryStartServer(address);
    }

    void CPlugin::tryStartServer(const QString &address)
    {
        // Make sure that there are no calls to XPLM in this method
        Q_ASSERT(! m_server);
        auto previousLibraryPath = BlackMisc::getCustomLibraryPath();
        auto libraryPath = g_xplanePath + "Resources" + g_sep + "plugins" + g_sep + "xbus";
        #if !defined (Q_OS_MAC) && defined(WORD_SIZE_64)
        libraryPath = libraryPath + g_sep + "64";
        #endif
        BlackMisc::setCustomLibraryPath(libraryPath);

        if (!BlackMisc::CDBusServer::isP2PAddress(address) && !BlackMisc::CDBusServer::isDBusAvailable(address))
        {
            constexpr int msec = 30000;
            BlackMisc::CLogMessage(this).warning("DBus daemon not available. Trying again in %1 sec.") << msec / 1000;
            QTimer::singleShot(msec, this, [&] { tryStartServer(address); });
            BlackMisc::setCustomLibraryPath(previousLibraryPath);
            return;
        }

        m_server = new BlackMisc::CDBusServer(xbusServiceName(), address, this);
        BlackMisc::setCustomLibraryPath(previousLibraryPath);

        m_server->addObject(CService::ObjectPath(), m_service);
        m_server->addObject(CTraffic::ObjectPath(), m_traffic);
        m_server->addObject(CWeather::ObjectPath(), m_weather);
    }

    void CPlugin::onAircraftModelChanged()
    {
        if (m_service)
        {
            m_service->onAircraftModelChanged();
        }
    }

    void CPlugin::onAircraftRepositioned()
    {
        if (m_service)
        {
            m_service->updateAirportsInRange();
        }
    }

}

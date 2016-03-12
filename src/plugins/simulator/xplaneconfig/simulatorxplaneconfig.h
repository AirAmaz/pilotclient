/* Copyright (C) 2015
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file

#ifndef BLACKSIMPLUGIN_SIMULATOR_XPLANE_CONFIG_H
#define BLACKSIMPLUGIN_SIMULATOR_XPLANE_CONFIG_H

#include "blackgui/pluginconfig.h"
#include "blackmisc/settingscache.h"

namespace BlackSimPlugin
{
    namespace XPlane
    {
        /*!
         * Setting for XBus.
         */
        struct XBusServer : public BlackMisc::CSettingTrait<QString>
        {
            //! \copydoc BlackMisc::CSettingTrait::key
            static const char *key() { return "xbus/server"; }

            //! \copydoc BlackMisc::CSettingTrait::defaultValue
            static QString defaultValue() { return QStringLiteral("session"); }
        };

        /*!
         * Config plugin for the X-Plane plugin.
         */
        class CSimulatorXPlaneConfig : public QObject, public BlackGui::IPluginConfig
        {
            Q_OBJECT
            Q_PLUGIN_METADATA(IID "org.swift-project.blackgui.pluginconfiginterface" FILE "simulatorxplaneconfig.json")
            Q_INTERFACES(BlackGui::IPluginConfig)

        public:
            //! Ctor
            CSimulatorXPlaneConfig(QObject *parent = nullptr);

            //! Dtor
            virtual ~CSimulatorXPlaneConfig() {}

            //! \copydoc BlackGui::IPluginConfig::createConfigWindow()
            BlackGui::CPluginConfigWindow *createConfigWindow(QWidget *parent) override;
        };
    }
}

#endif // guard

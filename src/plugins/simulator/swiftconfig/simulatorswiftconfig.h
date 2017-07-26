/* Copyright (C) 2017
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file

#ifndef BLACKSIMPLUGIN_SIMULATOR_SWIFT_CONFIG_H
#define BLACKSIMPLUGIN_SIMULATOR_SWIFT_CONFIG_H

#include "blackgui/pluginconfig.h"

namespace BlackSimPlugin
{
    namespace Swift
    {
        /*!
         * Window for setting up the FSX plugin.
         */
        class CSimulatorSwiftConfig : public QObject, public BlackGui::IPluginConfig
        {
            Q_OBJECT
            Q_PLUGIN_METADATA(IID "org.swift-project.blackgui.pluginconfiginterface" FILE "simulatorswiftconfig.json")
            Q_INTERFACES(BlackGui::IPluginConfig)

        public:
            //! Ctor
            CSimulatorSwiftConfig(QObject *parent = nullptr);

            //! Dtor
            virtual ~CSimulatorSwiftConfig() {}

            //! \copydoc BlackGui::IPluginConfig::createConfigWindow()
            BlackGui::CPluginConfigWindow *createConfigWindow(QWidget *parent) override;
        };
    } // ns
} // ns

#endif // guard

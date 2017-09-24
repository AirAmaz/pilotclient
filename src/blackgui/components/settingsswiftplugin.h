/* Copyright (C) 2017
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file

#ifndef BLACKGUI_COMPONENTS_SETTINGSSWIFTPLUGIN_H
#define BLACKGUI_COMPONENTS_SETTINGSSWIFTPLUGIN_H

#include <QFrame>
#include <blackmisc/simulation/settings/swiftpluginsettings.h>
#include "blackgui/blackguiexport.h"

namespace Ui { class CSettingsSwiftPlugin; }
namespace BlackGui
{
    namespace Components
    {
        /**
         * Settings for the swift pseudo driver
         */
        class BLACKGUI_EXPORT CSettingsSwiftPlugin : public QFrame
        {
            Q_OBJECT

        public:
            //! Ctor
            explicit CSettingsSwiftPlugin(QWidget *parent = nullptr);

            //! Dtor
            virtual ~CSettingsSwiftPlugin();

            //! Get the plugin settings
            BlackMisc::Simulation::Settings::CSwiftPluginSettings getPluginSettings() const;

        private:
            //! Settings changed
            void onSettingsChanged();

            //! Save
            void save();

            //! Get settings
            BlackMisc::Simulation::Settings::CSwiftPluginSettings getSettings() const;

            QScopedPointer<Ui::CSettingsSwiftPlugin> ui;
            BlackMisc::CSetting<BlackMisc::Simulation::Settings::TSwiftPlugin> m_settings { this, &CSettingsSwiftPlugin::onSettingsChanged };
        };
    } // ns
} // ns

#endif // guard

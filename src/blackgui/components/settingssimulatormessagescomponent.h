/* Copyright (C) 2016
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file

#ifndef BLACKGUI_COMPONENTS_SETTINGSSIMULATORMESSAGESCOMPONENT_H
#define BLACKGUI_COMPONENTS_SETTINGSSIMULATORMESSAGESCOMPONENT_H

#include "blackmisc/simulation/simulationsettings.h"
#include <QFrame>
#include <QScopedPointer>

namespace Ui { class CSettingsSimulatorMessagesComponent; }
namespace BlackGui
{
    namespace Components
    {
        /*!
         * Configure what messages are sent to simulator
         */
        class CSettingsSimulatorMessagesComponent : public QFrame
        {
            Q_OBJECT

        public:
            //! Constructor
            explicit CSettingsSimulatorMessagesComponent(QWidget *parent = nullptr);

            //! Destructor
            ~CSettingsSimulatorMessagesComponent();

        private slots:
            //! Save the data
            void ps_save();

            //! Load data
            void ps_load();

        private:
            QScopedPointer<Ui::CSettingsSimulatorMessagesComponent> ui;
            BlackMisc::CSetting<BlackMisc::Simulation::TSimulatorMessages> m_settings { this }; //!< settings for messages
        };
    } // ns
} // ns
#endif // guard

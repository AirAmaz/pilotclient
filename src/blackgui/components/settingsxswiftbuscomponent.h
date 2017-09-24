/* Copyright (C) 2017
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file

#ifndef BLACKGUI_COMPONENTS_SETTINGSXSWIFTBUSCOMPONENT_H
#define BLACKGUI_COMPONENTS_SETTINGSXSWIFTBUSCOMPONENT_H

#include "blackmisc/simulation/settings/xswiftbussettings.h"
#include "blackmisc/settingscache.h"
#include "blackgui/blackguiexport.h"
#include <QFrame>
#include <QScopedPointer>

namespace Ui { class CSettingsXSwiftBusComponent; }
namespace BlackGui
{
    namespace Components
    {
        /*!
         * XSwiftBus setup
         */
        class BLACKGUI_EXPORT CSettingsXSwiftBusComponent : public QFrame
        {
            Q_OBJECT

        public:
            //! Constructor
            explicit CSettingsXSwiftBusComponent(QWidget *parent = nullptr);

            //! Dtor
            virtual ~CSettingsXSwiftBusComponent();

        private:
            QScopedPointer<Ui::CSettingsXSwiftBusComponent> ui;
            BlackMisc::CSetting<BlackMisc::Simulation::Settings::TXSwiftBusServer> m_xSwiftBusServerSetting { this };

            void saveServer(const QString &dBusAddress);
        };
    } // ns
} // ns
#endif // guard

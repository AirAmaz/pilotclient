/* Copyright (C) 2013
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file

#ifndef BLACKGUI_USERCOMPONENT_H
#define BLACKGUI_USERCOMPONENT_H

#include "blackcore/network.h"
#include "blackgui/blackguiexport.h"
#include "blackgui/settings/viewupdatesettings.h"
#include "blackgui/components/enablefordockwidgetinfoarea.h"

#include <QObject>
#include <QScopedPointer>
#include <QTabWidget>
#include <QtGlobal>
#include <QTimer>

class QWidget;

namespace Ui { class CUserComponent; }
namespace BlackGui
{
    namespace Components
    {
        //! User componenet (users, clients)
        class BLACKGUI_EXPORT CUserComponent :
            public QTabWidget,
            public CEnableForDockWidgetInfoArea
        {
            Q_OBJECT

        public:
            //! Constructor
            explicit CUserComponent(QWidget *parent = nullptr);

            //! Destructor
            ~CUserComponent();

            //! Number of clients
            int countClients() const;

            //! Number of users
            int countUsers() const;

        public slots:
            //! Update users
            void update();

        private slots:
            //! Number of elements changed
            void ps_onCountChanged(int count, bool withFilter);

            //! Connection status
            void ps_connectionStatusChanged(BlackCore::INetwork::ConnectionStatus from, BlackCore::INetwork::ConnectionStatus to);

            //! Settings have been changed
            void ps_settingsChanged();

        private:
            QScopedPointer<Ui::CUserComponent> ui;
            QTimer m_updateTimer { this };
            BlackMisc::CSettingReadOnly<BlackGui::Settings::TViewUpdateSettings> m_settings { this, &CUserComponent::ps_settingsChanged };
        };
    }
}
#endif // guard

/* Copyright (C) 2015
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file

#ifndef BLACKGUI_COMPONENTS_SETTINGSGENERALCOMPONENT_H
#define BLACKGUI_COMPONENTS_SETTINGSGENERALCOMPONENT_H

#include "blackgui/blackguiexport.h"
#include "blackcore/application/applicationsettings.h"
#include "blackmisc/settingscache.h"

#include <QFrame>
#include <QObject>
#include <QScopedPointer>

class QWidget;

namespace Ui { class CSettingsAdvancedComponent; }

namespace BlackGui
{
    namespace Components
    {
        //! Configure general settings
        class BLACKGUI_EXPORT CSettingsAdvancedComponent :
            public QFrame
        {
            Q_OBJECT

        public:
            //! Constructor
            CSettingsAdvancedComponent(QWidget *parent = nullptr);

            //! Destructor
            ~CSettingsAdvancedComponent();

        private:
            void crashDumpUploadEnabledChanged(int state);

            QScopedPointer<Ui::CSettingsAdvancedComponent> ui;
            BlackMisc::CSetting<BlackCore::Application::TCrashDumpUploadEnabled> m_crashDumpUploadEnabled { this };
        };

    } // ns
} // ns

#endif // guard

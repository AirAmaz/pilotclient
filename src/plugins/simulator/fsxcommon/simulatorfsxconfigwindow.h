/* Copyright (C) 2015
 * swift project Community / Contributors
 *
 * This file is part of swift Project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file

#ifndef BLACKSIMPLUGIN_FSXCOMMON_SIMULATORFSXCONFIGWINDOW_H
#define BLACKSIMPLUGIN_FSXCOMMON_SIMULATORFSXCONFIGWINDOW_H

#include "blackgui/pluginconfigwindow.h"
#include <QScopedPointer>

namespace Ui { class CSimulatorFsxConfigWindow; }
namespace BlackSimPlugin
{
    namespace FsxCommon
    {
        /**
         * A window that lets user set up the FSX plugin.
         */
        class CSimulatorFsxConfigWindow : public BlackGui::CPluginConfigWindow
        {
            Q_OBJECT

        public:
            //! Ctor.
            CSimulatorFsxConfigWindow(const QString &simulator, QWidget *parent);

            //! Dtor.
            virtual ~CSimulatorFsxConfigWindow();

        private:
            QString m_simulator { "FSX" };
            QScopedPointer<Ui::CSimulatorFsxConfigWindow> ui;
        };
    }
}

#endif // guard

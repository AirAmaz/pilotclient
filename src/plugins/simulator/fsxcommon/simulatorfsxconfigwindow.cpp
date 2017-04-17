/* Copyright (C) 2015
 * swift project Community / Contributors
 *
 * This file is part of swift Project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "simulatorfsxconfigwindow.h"
#include "ui_simulatorfsxconfigwindow.h"

using namespace BlackGui;

namespace BlackSimPlugin
{
    namespace FsxCommon
    {
        CSimulatorFsxConfigWindow::CSimulatorFsxConfigWindow(const QString &simulator, QWidget *parent) :
            CPluginConfigWindow(parent),
            m_simulator(simulator),
            ui(new Ui::CSimulatorFsxConfigWindow)
        {
            ui->setupUi(this);
            connect(ui->bb_OkCancel, &QDialogButtonBox::rejected, this, &QWidget::close);
            this->setWindowTitle(m_simulator + " plugin configuration");
        }

        CSimulatorFsxConfigWindow::~CSimulatorFsxConfigWindow()
        {
            // void
        }
    }
}

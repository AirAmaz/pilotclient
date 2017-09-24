/* Copyright (C) 2017
 * swift project Community / Contributors
 *
 * This file is part of swift Project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "installxswiftbusdialog.h"
#include "ui_installxswiftbusdialog.h"

namespace BlackGui
{
    namespace Components
    {
        CInstallXSwiftBusDialog::CInstallXSwiftBusDialog(QWidget *parent) :
            QDialog(parent),
            ui(new Ui::CInstallXSwiftBusDialog)
        {
            ui->setupUi(this);
        }

        CInstallXSwiftBusDialog::~CInstallXSwiftBusDialog()
        { }
    } // ns
} // ns

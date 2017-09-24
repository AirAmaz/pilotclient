/* Copyright (C) 2017
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "settingsfontdialog.h"
#include "ui_settingsfontdialog.h"

namespace BlackGui
{
    namespace Components
    {
        CSettingsFontDialog::CSettingsFontDialog(QWidget *parent) :
            QDialog(parent),
            ui(new Ui::CSettingsFontDialog)
        {
            ui->setupUi(this);
            ui->comp_FontSettings->setMode(CSettingsFontComponent::GenerateQssOnly);

            connect(ui->comp_FontSettings, &CSettingsFontComponent::accept, this, &CSettingsFontDialog::accept);
            connect(ui->comp_FontSettings, &CSettingsFontComponent::reject, this, &CSettingsFontDialog::reject);
        }

        CSettingsFontDialog::~CSettingsFontDialog()
        { }

        const QString &CSettingsFontDialog::getQss() const
        {
            return ui->comp_FontSettings->getQss();
        }

        void CSettingsFontDialog::setCurrentFont(const QFont &font)
        {
            ui->comp_FontSettings->setCurrentFont(font);
        }
    } // ns
} // ns

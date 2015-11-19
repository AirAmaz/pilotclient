/* Copyright (C) 2015
 * swift project Community / Contributors
 *
 * This file is part of swift Project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "aircraftcombinedtypeselector.h"
#include "ui_aircraftcombinedtypeselector.h"
#include "blackgui/guiutility.h"

using namespace BlackMisc::Aviation;

namespace BlackGui
{

    CAircraftCombinedTypeSelector::CAircraftCombinedTypeSelector(QWidget *parent) :
        QFrame(parent),
        ui(new Ui::CAircraftCombinedTypeSelector)
    {
        ui->setupUi(this);
        this->connect(ui->le_CombinedType, &QLineEdit::editingFinished, this, &CAircraftCombinedTypeSelector::ps_CombinedTypeEntered);
        this->connect(ui->le_CombinedType, &QLineEdit::returnPressed, this, &CAircraftCombinedTypeSelector::ps_CombinedTypeEntered);

        this->connect(ui->cb_EngineCount, &QComboBox::currentTextChanged, this, &CAircraftCombinedTypeSelector::ps_ChangedComboBox);
        this->connect(ui->cb_EngineType, &QComboBox::currentTextChanged, this, &CAircraftCombinedTypeSelector::ps_ChangedComboBox);
        this->connect(ui->cb_Type, &QComboBox::currentTextChanged, this, &CAircraftCombinedTypeSelector::ps_ChangedComboBox);
    }

    CAircraftCombinedTypeSelector::~CAircraftCombinedTypeSelector()
    { }

    void CAircraftCombinedTypeSelector::setCombinedType(const QString &combinedCode)
    {
        QString engineCount, engineType, aircraftType;
        QString cc(combinedCode.trimmed().toUpper().left(3));
        if (cc.length() > 0) { aircraftType = cc.left(1); }
        if (cc.length() > 1) { engineCount = cc.mid(1, 1); }
        if (cc.length() > 2) { engineType = cc.mid(2, 1); }

        CGuiUtility::setComboBoxValueByStartingString(this->ui->cb_EngineCount, engineCount, "unspecified");
        CGuiUtility::setComboBoxValueByStartingString(this->ui->cb_EngineType, engineType, "unspecified");
        CGuiUtility::setComboBoxValueByStartingString(this->ui->cb_Type, aircraftType, "unspecified");

        this->ui->le_CombinedType->setText(cc);
    }

    void CAircraftCombinedTypeSelector::setCombinedType(const BlackMisc::Aviation::CAircraftIcaoCode &icao)
    {
        this->setCombinedType(icao.getCombinedType());
    }

    void CAircraftCombinedTypeSelector::clear()
    {
        this->setCombinedType("");
        this->ui->le_CombinedType->clear();
    }

    void CAircraftCombinedTypeSelector::setReadOnly(bool readOnly)
    {
        ui->le_CombinedType->setReadOnly(readOnly);
        ui->cb_EngineCount->setEnabled(!readOnly);
        ui->cb_EngineType->setEnabled(!readOnly);
        ui->cb_Type->setEnabled(!readOnly);
    }

    QString CAircraftCombinedTypeSelector::getCombinedType() const
    {
        QString ct(ui->le_CombinedType->text().trimmed().toUpper());
        if (CAircraftIcaoCode::isValidCombinedType(ct)) { return ct; }

        QString ct2(getCombinedTypeFromComboBoxes());
        return ct2;
    }

    void CAircraftCombinedTypeSelector::ps_CombinedTypeEntered()
    {
        QString cc(ui->le_CombinedType->text().trimmed().toUpper());
        this->setCombinedType(cc);
    }

    void CAircraftCombinedTypeSelector::ps_ChangedComboBox(const QString &text)
    {
        Q_UNUSED(text);
        QString ct(getCombinedTypeFromComboBoxes());
        ui->le_CombinedType->setText(ct);
    }

    QString CAircraftCombinedTypeSelector::getCombinedTypeFromComboBoxes() const
    {
        // try to get from combox boxes instead
        QString ec = ui->cb_EngineCount->currentText().left(1);
        QString t = ui->cb_Type->currentText().left(1);
        QString et = ui->cb_EngineType->currentText().left(1);

        QString ct2(QString(t + ec + et).toUpper());
        return ct2.replace('U', '-');
    }

} // ns

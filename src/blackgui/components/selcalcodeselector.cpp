/* Copyright (C) 2013
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "blackgui/components/selcalcodeselector.h"
#include "blackgui/ticklabel.h"
#include "blackmisc/aviation/selcal.h"
#include "ui_selcalcodeselector.h"

#include <QComboBox>
#include <QStringList>
#include <QtGlobal>

using namespace BlackMisc;
using namespace BlackMisc::Aviation;

namespace BlackGui
{
    namespace Components
    {
        CSelcalCodeSelector::CSelcalCodeSelector(QWidget *parent) :
            QFrame(parent), ui(new Ui::CSelcalCodeSelector)
        {
            ui->setupUi(this);
            this->resetSelcalCodes(true);
            this->setValidityHint();
            ui->lblp_ValidCodeIcon->setToolTips("valid SELCAL", "invalid SELCAL");

            // limit number of elements: https://forum.qt.io/topic/11315/limit-the-number-of-visible-items-on-qcombobox/6
            ui->cb_SelcalPairs1->setStyleSheet("combobox-popup: 0;");
            ui->cb_SelcalPairs2->setStyleSheet("combobox-popup: 0;");

            connect(ui->cb_SelcalPairs1, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &CSelcalCodeSelector::selcalIndexChanged);
            connect(ui->cb_SelcalPairs2, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &CSelcalCodeSelector::selcalIndexChanged);
        }

        CSelcalCodeSelector::~CSelcalCodeSelector()
        { }

        QString CSelcalCodeSelector::getSelcalCode() const
        {
            QString selcal = ui->cb_SelcalPairs1->currentText();
            selcal.append(ui->cb_SelcalPairs2->currentText());
            return selcal;
        }

        CSelcal CSelcalCodeSelector::getSelcal() const
        {
            CSelcal selcal(getSelcalCode());
            return selcal;
        }

        void CSelcalCodeSelector::resetSelcalCodes(bool allowEmptyValue)
        {
            ui->cb_SelcalPairs1->clear();
            if (allowEmptyValue) ui->cb_SelcalPairs1->addItem("  ");
            ui->cb_SelcalPairs1->addItems(BlackMisc::Aviation::CSelcal::codePairs());
            ui->cb_SelcalPairs2->clear();
            if (allowEmptyValue) ui->cb_SelcalPairs2->addItem("  ");
            ui->cb_SelcalPairs2->addItems(BlackMisc::Aviation::CSelcal::codePairs());
        }

        void CSelcalCodeSelector::setSelcalCode(const QString &selcal)
        {
            if (selcal.length() == 4 && this->getSelcalCode() == selcal) return; // avoid unintended signals
            const QString s = selcal.isEmpty() ? "    " : selcal.toUpper().trimmed();
            Q_ASSERT(s.length() == 4);
            if (s.length() != 4) return;
            const QString s1 = s.left(2);
            const QString s2 = s.right(2);
            if (BlackMisc::Aviation::CSelcal::codePairs().contains(s1))
            {
                ui->cb_SelcalPairs1->setCurrentText(s1);
            }
            if (BlackMisc::Aviation::CSelcal::codePairs().contains(s2))
            {
                ui->cb_SelcalPairs2->setCurrentText(s2);
            }
        }

        void CSelcalCodeSelector::setSelcal(const BlackMisc::Aviation::CSelcal &selcal)
        {
            this->setSelcalCode(selcal.getCode());
        }

        bool CSelcalCodeSelector::hasValidCode() const
        {
            const QString s = this->getSelcalCode();
            if (s.length() != 4) return false;
            return BlackMisc::Aviation::CSelcal::isValidCode(s);
        }

        void CSelcalCodeSelector::clear()
        {
            if (ui->cb_SelcalPairs1->count() < 1) { this->resetSelcalCodes(true); }
            ui->cb_SelcalPairs1->setCurrentIndex(0);
            ui->cb_SelcalPairs2->setCurrentIndex(0);
        }

        void CSelcalCodeSelector::selcalIndexChanged(int index)
        {
            Q_UNUSED(index);
            this->setValidityHint();
            emit valueChanged();
        }

        void CSelcalCodeSelector::setValidityHint()
        {
            ui->lblp_ValidCodeIcon->setTicked(this->hasValidCode());
        }
    }  // ns
} // ns

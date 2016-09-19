/* Copyright (C) 2016
 * swift project Community / Contributors
 *
 * This file is part of swift Project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "blackgui/components/dbmodelmappingmodifycomponent.h"
#include "blackgui/editors/modelmappingmodifyform.h"
#include "ui_dbmodelmappingmodifycomponent.h"

#include <QWidget>

using namespace BlackMisc;
using namespace BlackMisc::Simulation;

namespace BlackGui
{
    namespace Components
    {
        CDbModelMappingModifyComponent::CDbModelMappingModifyComponent(QWidget *parent) :
            QDialog(parent),
            CDbMappingComponentAware(parent),
            ui(new Ui::CDbModelMappingModifyComponent)
        {
            ui->setupUi(this);
        }

        CDbModelMappingModifyComponent::~CDbModelMappingModifyComponent()
        {
            // void
        }

        CPropertyIndexVariantMap CDbModelMappingModifyComponent::getValues() const
        {
            return (ui->editor_ModelMappingModify->getValues());
        }

        void CDbModelMappingModifyComponent::setValue(const CAircraftModel &model)
        {
            ui->editor_ModelMappingModify->setValue(model);
        }
    } // ns
} // ns

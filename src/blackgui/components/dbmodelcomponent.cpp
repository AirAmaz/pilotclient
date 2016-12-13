/* Copyright (C) 2015
 * swift project Community / Contributors
 *
 * This file is part of swift Project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "blackcore/application.h"
#include "blackcore/webdataservices.h"
#include "blackgui/components/dbmodelcomponent.h"
#include "blackgui/filters/aircraftmodelfilterbar.h"
#include "blackgui/guiapplication.h"
#include "blackgui/models/aircraftmodellistmodel.h"
#include "blackgui/views/aircraftmodelview.h"
#include "blackgui/views/viewbase.h"
#include "blackmisc/simulation/aircraftmodel.h"
#include "ui_dbmodelcomponent.h"

#include <QDateTime>
#include <QWidget>
#include <QtGlobal>

using namespace BlackMisc;
using namespace BlackMisc::Network;
using namespace BlackMisc::Simulation;
using namespace BlackCore;
using namespace BlackGui::Views;
using namespace BlackGui::Models;

namespace BlackGui
{
    namespace Components
    {
        CDbModelComponent::CDbModelComponent(QWidget *parent) :
            QFrame(parent),
            CDbMappingComponentAware(parent),
            ui(new Ui::CDbModelComponent)
        {
            ui->setupUi(this);
            this->setViewWithIndicator(ui->tvp_AircraftModel);
            ui->tvp_AircraftModel->setAircraftModelMode(CAircraftModelListModel::Database);
            ui->tvp_AircraftModel->menuAddItems(CAircraftModelView::MenuStashing);
            ui->tvp_AircraftModel->menuRemoveItems(CAircraftModelView::MenuHighlightStashed); // not supported here
            connect(ui->tvp_AircraftModel, &CAircraftModelView::requestNewBackendData, this, &CDbModelComponent::ps_reload);
            connect(ui->tvp_AircraftModel, &CAircraftModelView::requestStash, this, &CDbModelComponent::requestStash);
            connect(sGui, &CGuiApplication::styleSheetsChanged, this, &CDbModelComponent::ps_onStyleSheetChanged);

            // configure view
            ui->tvp_AircraftModel->setFilterWidget(ui->filter_AircraftModelFilter);
            ui->tvp_AircraftModel->allowDragDrop(true, false);

            connect(sApp->getWebDataServices(), &CWebDataServices::dataRead, this, &CDbModelComponent::ps_modelsRead);
            this->ps_modelsRead(CEntityFlags::ModelEntity, CEntityFlags::ReadFinished, sApp->getWebDataServices()->getModelsCount());
        }

        CDbModelComponent::~CDbModelComponent()
        {
            // void
        }

        bool CDbModelComponent::hasModels() const
        {
            return !ui->tvp_AircraftModel->isEmpty();
        }

        void CDbModelComponent::requestUpdatedData()
        {
            QDateTime ts;
            if (!ui->tvp_AircraftModel->isEmpty())
            {
                CAircraftModel model(ui->tvp_AircraftModel->container().latestObject());
                ts = model.getUtcTimestamp();
            }
            sGui->getWebDataServices()->triggerLoadingDirectlyFromDb(CEntityFlags::ModelEntity, ts);
        }

        void CDbModelComponent::ps_modelsRead(CEntityFlags::Entity entity, CEntityFlags::ReadState readState, int count)
        {
            Q_UNUSED(count);
            if (entity.testFlag(CEntityFlags::ModelEntity))
            {
                if (readState == CEntityFlags::ReadFinished || readState == CEntityFlags::ReadFinishedRestricted)
                {
                    ui->tvp_AircraftModel->updateContainerMaybeAsync(sGui->getWebDataServices()->getModels());
                }
            }
        }

        void CDbModelComponent::ps_reload()
        {
            if (!sGui) { return; }
            sGui->getWebDataServices()->triggerLoadingDirectlyFromDb(CEntityFlags::ModelEntity);
        }

        void CDbModelComponent::ps_onStyleSheetChanged()
        {
            // code goes here
        }

        void CDbModelComponent::ps_stashSelectedModels()
        {
            if (!ui->tvp_AircraftModel->hasSelection()) { return; }
            const CAircraftModelList models(ui->tvp_AircraftModel->selectedObjects());
            if (!models.isEmpty())
            {
                emit requestStash(models);
            }
        }
    } // ns
} // ns

/* Copyright (C) 2016
 * swift project Community / Contributors
 *
 * This file is part of swift Project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "blackcore/modelsetbuilder.h"
#include "blackgui/components/dbmappingcomponent.h"
#include "blackgui/components/dbownmodelsetdialog.h"
#include "blackgui/editors/ownmodelsetform.h"
#include "blackmisc/logcategory.h"
#include "blackmisc/logcategorylist.h"
#include "blackmisc/simulation/distributorlist.h"
#include "ui_dbownmodelsetdialog.h"

#include <QPushButton>
#include <QString>
#include <QWidget>
#include <QtGlobal>

using namespace BlackMisc;
using namespace BlackMisc::Simulation;
using namespace BlackCore;
using namespace BlackGui::Editors;

namespace BlackGui
{
    namespace Components
    {
        const CLogCategoryList &CDbOwnModelSetDialog::getLogCategories()
        {
            static const CLogCategoryList cats({ CLogCategory("swift.ownmodelset"), CLogCategory::guiComponent()});
            return cats;
        }

        CDbOwnModelSetDialog::CDbOwnModelSetDialog(QWidget *parent) :
            QDialog(parent),
            CDbMappingComponentAware(parent),
            ui(new Ui::CDbOwnModelSetDialog)
        {
            ui->setupUi(this);
            connect(ui->pb_Cancel, &QPushButton::clicked, this, &CDbOwnModelSetDialog::ps_buttonClicked);
            connect(ui->pb_Ok, &QPushButton::clicked, this, &CDbOwnModelSetDialog::ps_buttonClicked);
            connect(ui->form_OwnModelSet, &COwnModelSetForm::simulatorChanged, this, &CDbOwnModelSetDialog::ps_simulatorChanged);
        }

        CDbOwnModelSetDialog::~CDbOwnModelSetDialog()
        {
            // void
        }

        void CDbOwnModelSetDialog::reloadData()
        {
            this->m_simulatorInfo = this->getMappingComponent()->getOwnModelsSimulator();
            Q_ASSERT_X(this->m_simulatorInfo.isSingleSimulator(), Q_FUNC_INFO, "Need single simulator");
            ui->form_OwnModelSet->setSimulator(this->m_simulatorInfo);
            ui->form_OwnModelSet->reloadData();
            this->m_modelSet = this->getMappingComponent()->getOwnModelSet();
        }

        int CDbOwnModelSetDialog::exec()
        {
            Q_ASSERT_X(this->getMappingComponent(), Q_FUNC_INFO, "missing mapping component");
            const CSimulatorInfo sim(this->getMappingComponent()->getOwnModelsSimulator());
            Q_ASSERT_X(sim.isSingleSimulator(), Q_FUNC_INFO, "need single simulator");
            this->setSimulator(sim);
            this->checkData();
            return QDialog::exec();
        }

        void CDbOwnModelSetDialog::ps_buttonClicked()
        {
            const QObject *sender = QObject::sender();
            if (sender == ui->pb_Cancel)
            {
                this->reject();
            }
            else if (sender == ui->pb_Ok)
            {
                this->m_modelSet = this->buildSet(this->m_simulatorInfo, this->m_modelSet);
                this->accept();
            }
        }

        void CDbOwnModelSetDialog::ps_simulatorChanged(const CSimulatorInfo &simulator)
        {
            Q_ASSERT_X(this->getMappingComponent(), Q_FUNC_INFO, "missing mapping component");
            this->setSimulator(simulator);
            this->getMappingComponent()->setOwnModelsSimulator(simulator);
            this->getMappingComponent()->setOwnModelSetSimulator(simulator);
            this->checkData();
        }

        bool CDbOwnModelSetDialog::checkData()
        {
            // models
            Q_ASSERT_X(this->m_simulatorInfo.isSingleSimulator(), Q_FUNC_INFO, "Need single simulator");
            const int c = this->getMappingComponent()->getOwnModelsCount();
            if (c < 1)
            {
                const CStatusMessage m = CStatusMessage(this).error("No models for %1") << this->m_simulatorInfo.toQString(true);
                ui->form_OwnModelSet->showOverlayMessage(m);
                return false;
            }
            return true;
        }

        void CDbOwnModelSetDialog::setSimulator(const CSimulatorInfo &simulator)
        {
            Q_ASSERT_X(this->m_simulatorInfo.isSingleSimulator(), Q_FUNC_INFO, "Need single simulator");
            this->m_simulatorInfo = simulator;
            ui->form_OwnModelSet->setSimulator(this->m_simulatorInfo);
            this->setWindowTitle("Create model set for " + this->m_simulatorInfo.toQString(true));
        }

        CAircraftModelList CDbOwnModelSetDialog::buildSet(const CSimulatorInfo &simulator, const CAircraftModelList &currentSet)
        {
            Q_ASSERT_X(this->getMappingComponent(), Q_FUNC_INFO, "missing mapping component");
            const bool givenDistributorsOnly  = !ui->form_OwnModelSet->optionUseAllDistributors();
            const bool dbDataOnly = ui->form_OwnModelSet->optionDbDataOnly();
            const bool dbIcaoOnly = ui->form_OwnModelSet->optionDbIcaoCodesOnly();
            const bool incremnental = ui->form_OwnModelSet->optionIncrementalBuild();
            const bool sortByDistributor = ui->form_OwnModelSet->optionSortByDistributorPreferences();
            const bool consolidateWithDb = ui->form_OwnModelSet->optionConsolidateModelSetWithDbData();

            this->m_simulatorInfo = this->getMappingComponent()->getOwnModelsSimulator();
            const CAircraftModelList models = this->getMappingComponent()->getOwnModels();
            const CDistributorList distributors = ui->form_OwnModelSet->getDistributorsBasedOnOptions();

            if (givenDistributorsOnly && distributors.isEmpty())
            {
                // nothing to do, keep current set
                return currentSet;
            }

            const CModelSetBuilder builder(this);
            CModelSetBuilder::Builder options = givenDistributorsOnly ? CModelSetBuilder::GivenDistributorsOnly : CModelSetBuilder::NoOptions;
            if (dbDataOnly)   { options |= CModelSetBuilder::OnlyDbData; }
            if (dbIcaoOnly)   { options |= CModelSetBuilder::OnlyDbIcaoCodes; }
            if (incremnental) { options |= CModelSetBuilder::Incremental; }
            if (sortByDistributor) { options |= CModelSetBuilder::SortByDistributors; }
            if (consolidateWithDb) { options |= CModelSetBuilder::ConsolidateWithDb; }
            return builder.buildModelSet(simulator, models, currentSet, options, distributors);
        }
    } // ns
} // ns

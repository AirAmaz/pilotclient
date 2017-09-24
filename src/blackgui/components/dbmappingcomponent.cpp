/* Copyright (C) 2015
 * swift project Community / Contributors
 *
 * This file is part of swift Project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "blackcore/webdataservices.h"
#include "blackgui/components/dbautostashingcomponent.h"
#include "blackgui/components/dbautosimulatorstashingcomponent.h"
#include "blackgui/components/dbmappingcomponent.h"
#include "blackgui/components/dbmodelmappingmodifycomponent.h"
#include "blackgui/components/dbownmodelscomponent.h"
#include "blackgui/components/dbownmodelsetcomponent.h"
#include "blackgui/components/dbstashcomponent.h"
#include "blackgui/components/modelmatchercomponent.h"
#include "blackgui/editors/aircrafticaoform.h"
#include "blackgui/editors/distributorform.h"
#include "blackgui/editors/liveryform.h"
#include "blackgui/editors/modelmappingform.h"
#include "blackgui/guiapplication.h"
#include "blackgui/guiutility.h"
#include "blackgui/menus/aircraftmodelmenus.h"
#include "blackgui/menus/menuaction.h"
#include "blackgui/models/aircraftmodellistmodel.h"
#include "blackgui/views/aircraftmodelview.h"
#include "blackgui/views/viewbase.h"
#include "blackmisc/aviation/aircrafticaocode.h"
#include "blackmisc/aviation/livery.h"
#include "blackmisc/icons.h"
#include "blackmisc/logmessage.h"
#include "blackmisc/network/authenticateduser.h"
#include "blackmisc/propertyindexvariantmap.h"
#include "blackmisc/simulation/aircraftmodelutils.h"
#include "ui_dbmappingcomponent.h"

#include <QAction>
#include <QDialog>
#include <QFrame>
#include <QKeySequence>
#include <QMenu>
#include <QModelIndex>
#include <QPoint>
#include <QSplitter>
#include <QTabWidget>
#include <QVariant>
#include <QWidget>
#include <Qt>
#include <QtGlobal>

using namespace BlackCore;
using namespace BlackMisc;
using namespace BlackMisc::Aviation;
using namespace BlackMisc::Network;
using namespace BlackMisc::Simulation;
using namespace BlackMisc::Simulation::FsCommon;
using namespace BlackGui;
using namespace BlackGui::Editors;
using namespace BlackGui::Views;
using namespace BlackGui::Models;
using namespace BlackGui::Menus;

namespace BlackGui
{
    namespace Components
    {
        CDbMappingComponent::CDbMappingComponent(QWidget *parent) :
            COverlayMessagesFrame(parent),
            ui(new Ui::CDbMappingComponent),
            m_autoStashDialog(new CDbAutoStashingComponent(this)),
            m_autoSimulatorDialog(new CDbAutoSimulatorStashingComponent(this)),
            m_modelModifyDialog(new CDbModelMappingModifyComponent(this))
        {
            ui->setupUi(this);
            ui->comp_StashAircraft->setMappingComponent(this);
            ui->comp_OwnModelSet->setMappingComponent(this);

            //! \fixme vPilot merging will be most likely removed in the future
            ui->tvp_AircraftModelsForVPilot->setAircraftModelMode(CAircraftModelListModel::VPilotRuleModel);
            ui->tvp_AircraftModelsForVPilot->addFilterDialog();

            // model menus
            ui->comp_OwnAircraftModels->view()->setCustomMenu(new CShowSimulatorFileMenu(ui->comp_OwnAircraftModels->view(), this, true));
            ui->comp_OwnAircraftModels->view()->setCustomMenu(new CMergeWithVPilotMenu(this));
            ui->comp_OwnAircraftModels->view()->setCustomMenu(new COwnModelSetMenu(this, true));
            ui->comp_OwnAircraftModels->view()->setCustomMenu(new CStashToolsMenu(this, false));

            ui->comp_OwnModelSet->view()->setCustomMenu(new CShowSimulatorFileMenu(ui->comp_OwnModelSet->view(), this, true));
            ui->comp_OwnModelSet->view()->setCustomMenu(new CStashToolsMenu(this, true));

            ui->comp_StashAircraft->view()->setCustomMenu(new CShowSimulatorFileMenu(ui->comp_StashAircraft->view(), this, true));
            ui->comp_StashAircraft->view()->setCustomMenu(new CApplyDbDataMenu(this, true));
            ui->comp_StashAircraft->view()->setCustomMenu(new COwnModelSetMenu(this, true));
            ui->comp_StashAircraft->view()->setCustomMenu(new CStashToolsMenu(this, false));

            // connects
            connect(ui->editor_ModelMapping, &CModelMappingForm::requestStash, this, &CDbMappingComponent::ps_stashCurrentModel);

            connect(ui->comp_OwnAircraftModels->view(), &CAircraftModelView::doubleClicked, this, &CDbMappingComponent::ps_onModelRowSelected);
            connect(ui->comp_OwnAircraftModels->view(), &CAircraftModelView::modelDataChanged, this, &CDbMappingComponent::ps_onOwnModelsChanged);
            connect(ui->comp_OwnAircraftModels->view(), &CAircraftModelView::requestStash, this, &CDbMappingComponent::stashSelectedModels);
            connect(ui->comp_OwnAircraftModels->view(), &CAircraftModelView::toggledHighlightStashedModels, this, &CDbMappingComponent::ps_onStashedModelsChanged);

            connect(ui->comp_StashAircraft->view(), &CAircraftModelView::modelDataChanged, this, &CDbMappingComponent::ps_onStashedModelsDataChanged);
            connect(ui->comp_StashAircraft->view(), &CAircraftModelView::doubleClicked, this, &CDbMappingComponent::ps_onModelRowSelected);
            connect(ui->comp_StashAircraft->view(), &CAircraftModelView::requestHandlingOfStashDrop, this, &CDbMappingComponent::ps_handleStashDropRequest);
            connect(ui->comp_StashAircraft, &CDbStashComponent::stashedModelsChanged, this, &CDbMappingComponent::ps_onStashedModelsChanged);
            connect(ui->comp_StashAircraft, &CDbStashComponent::modelsSuccessfullyPublished, this, &CDbMappingComponent::ps_onModelsSuccessfullyPublished);

            connect(ui->comp_OwnModelSet->view(), &CAircraftModelView::modelDataChanged, this, &CDbMappingComponent::ps_onModelSetChanged);
            connect(ui->comp_OwnModelSet->view(), &CAircraftModelView::requestStash, this, &CDbMappingComponent::stashSelectedModels);

            connect(ui->tw_ModelsToBeMapped, &QTabWidget::currentChanged, this, &CDbMappingComponent::ps_tabIndexChanged);
            connect(ui->tw_ModelsToBeMapped, &QTabWidget::currentChanged, ui->comp_ModelMatcher , &CModelMatcherComponent::tabIndexChanged);

            connect(ui->comp_OwnModelSet->view(), &CAircraftModelView::doubleClicked, this, &CDbMappingComponent::ps_onModelRowSelected);

            // initial values
            this->ps_onModelSetChanged(ui->comp_OwnModelSet->view()->rowCount(), ui->comp_OwnModelSet->view()->hasFilter());
            this->ps_onStashedModelsDataChanged(ui->comp_StashAircraft->view()->rowCount(), ui->comp_StashAircraft->view()->hasFilter());
            this->ps_onOwnModelsChanged(ui->comp_OwnAircraftModels->view()->rowCount(), ui->comp_OwnAircraftModels->view()->hasFilter());

            // how to display forms
            ui->editor_AircraftModel->setSelectOnly();

            ui->tw_ModelsToBeMapped->setTabIcon(TabStash, CIcons::appDbStash16());
            ui->tw_ModelsToBeMapped->setTabIcon(TabOwnModels, CIcons::appModels16());
            ui->tw_ModelsToBeMapped->setTabIcon(TabOwnModelSet, CIcons::appModels16());

            // custom menu
            this->setContextMenuPolicy(Qt::CustomContextMenu);
            connect(this, &CDbMappingComponent::customContextMenuRequested, this, &CDbMappingComponent::ps_onCustomContextMenu);

            // vPilot
            this->initVPilotLoading();

            // user changes
            m_swiftDbUser.setNotifySlot(&CDbMappingComponent::ps_userChanged);
        }

        CDbMappingComponent::~CDbMappingComponent()
        {
            gracefulShutdown();
        }

        void CDbMappingComponent::initVPilotLoading()
        {
            m_vPilotEnabled = this->vPilotSupport && m_swiftDbUser.get().hasAdminRole();
            static const QString tabName(ui->tw_ModelsToBeMapped->tabText(TabVPilot));

            if (m_vPilot1stInit && vPilotSupport)
            {
                m_vPilot1stInit = false;
                connect(ui->tvp_AircraftModelsForVPilot, &CAircraftModelView::doubleClicked, this, &CDbMappingComponent::ps_onModelRowSelected);
                connect(ui->tvp_AircraftModelsForVPilot, &CAircraftModelView::modelDataChanged, this, &CDbMappingComponent::ps_onVPilotDataChanged);
                connect(&m_vPilotReader, &CVPilotRulesReader::readFinished, this, &CDbMappingComponent::ps_onLoadVPilotDataFinished);
                connect(ui->tvp_AircraftModelsForVPilot, &CAircraftModelView::requestStash, this, &CDbMappingComponent::stashSelectedModels);
                connect(ui->tvp_AircraftModelsForVPilot, &CAircraftModelView::toggledHighlightStashedModels, this, &CDbMappingComponent::ps_onStashedModelsChanged);
                connect(ui->tvp_AircraftModelsForVPilot, &CAircraftModelView::requestUpdate, this, &CDbMappingComponent::ps_requestVPilotDataUpdate);

                ui->tvp_AircraftModelsForVPilot->setCustomMenu(new CMappingVPilotMenu(this, true));
                ui->tvp_AircraftModelsForVPilot->setCustomMenu(new CStashToolsMenu(this, false));
                ui->tvp_AircraftModelsForVPilot->setDisplayAutomatically(true);

                ui->tvp_AircraftModelsForVPilot->addFilterDialog();
                const CAircraftModelList vPilotModels(m_vPilotReader.getAsModelsFromCache());
                ui->tvp_AircraftModelsForVPilot->updateContainerMaybeAsync(vPilotModels);
                const int noModels = vPilotModels.size();
                CLogMessage(this).info("%1 cached vPilot models loaded") << noModels;
            }
            ui->tab_VPilot->setEnabled(m_vPilotEnabled);
            ui->tab_VPilot->setVisible(m_vPilotEnabled);
            if (m_vPilotEnabled)
            {
                // create / restore tab
                ui->tw_ModelsToBeMapped->addTab(ui->tab_VPilot, tabName);
                this->ps_onVPilotDataChanged(ui->tvp_AircraftModelsForVPilot->rowCount(),
                                             ui->tvp_AircraftModelsForVPilot->hasFilter());
            }
            else
            {
                m_vPilotFormatted = false;
                ui->tw_ModelsToBeMapped->removeTab(TabVPilot);
            }
        }

        void CDbMappingComponent::formatVPilotView()
        {
            if (!m_vPilotEnabled || m_vPilotFormatted) { return; }
            m_vPilotFormatted = true;
            ui->tvp_AircraftModelsForVPilot->presizeOrFullResizeToContents();
        }

        CAircraftModel CDbMappingComponent::getModelFromView(const QModelIndex &index) const
        {
            if (!index.isValid()) { return CAircraftModel(); }
            const QObject *sender = QObject::sender();

            // check if we have an explicit sender
            if (sender == ui->tvp_AircraftModelsForVPilot)
            {
                return ui->tvp_AircraftModelsForVPilot->at(index);
            }
            else if (sender == ui->comp_OwnAircraftModels->view())
            {
                return ui->comp_OwnAircraftModels->view()->at(index);
            }
            else if (sender == ui->comp_StashAircraft || sender == ui->comp_StashAircraft->view())
            {
                return ui->comp_StashAircraft->view()->at(index);
            }
            else if (sender == ui->comp_OwnModelSet->view())
            {
                return ui->comp_OwnModelSet->view()->at(index);
            }

            // no sender, use current tab
            const CAircraftModelView *v = this->currentModelView();
            if (!v) { return CAircraftModel(); }
            return v->at(index);
        }

        void CDbMappingComponent::gracefulShutdown()
        {
            this->disconnect();
            m_vPilotReader.gracefulShutdown();
            ui->comp_OwnAircraftModels->gracefulShutdown();
        }

        bool CDbMappingComponent::hasSelectedModelsToStash() const
        {
            const TabIndex tab = currentTabIndex();
            switch (tab)
            {
            case TabOwnModels:
                return ui->comp_OwnAircraftModels->view()->hasSelectedModelsToStash();
            case TabOwnModelSet:
                return ui->comp_OwnModelSet->view()->hasSelectedModelsToStash();
            case TabVPilot:
                return ui->tvp_AircraftModelsForVPilot->hasSelectedModelsToStash();
            default:
                break;
            }
            return false;
        }

        CAircraftModelView *CDbMappingComponent::currentModelView() const
        {
            const TabIndex tab = currentTabIndex();
            switch (tab)
            {
            case TabOwnModels:
                return ui->comp_OwnAircraftModels->view();
            case TabVPilot:
                return ui->tvp_AircraftModelsForVPilot;
            case TabStash:
                return ui->comp_StashAircraft->view();
            case TabOwnModelSet:
                return ui->comp_OwnModelSet->view();
            default:
                return nullptr;
            }
        }

        QString CDbMappingComponent::currentTabText() const
        {
            const int i = ui->tw_ModelsToBeMapped->currentIndex();
            return ui->tw_ModelsToBeMapped->tabText(i);
        }

        void CDbMappingComponent::updateEditorsWhenApplicable()
        {
            const CAircraftModel currentEditorModel(ui->editor_ModelMapping->getValue());
            if (!currentEditorModel.hasModelString()) { return; } // no related model
            const QString modelString(currentEditorModel.getModelString());
            const CAircraftModel currentStashedModel(ui->comp_StashAircraft->getStashedModel(modelString));
            if (!currentStashedModel.hasModelString()) { return; }

            // we have found a model in the stashed models and this is the one currently displayed
            // in the editors
            bool updated = false;
            const CLivery stashedLivery(currentStashedModel.getLivery());
            if (stashedLivery.hasValidDbKey())
            {
                if (ui->editor_AircraftModel->setLivery(stashedLivery)) { updated = true; }
            }

            const CDistributor stashedDistributor(currentStashedModel.getDistributor());
            if (stashedDistributor.hasValidDbKey())
            {
                if (ui->editor_AircraftModel->setDistributor(stashedDistributor)) { updated = true; }
            }

            const CAircraftIcaoCode stashedIcaoCode(currentStashedModel.getAircraftIcaoCode());
            if (stashedIcaoCode.hasValidDbKey())
            {
                if (ui->editor_AircraftModel->setAircraftIcao(stashedIcaoCode)) { updated = true; }
            }

            if (updated)
            {
                CLogMessage(this).info("Updated editor data for '%1'") << modelString;
            }
        }

        CAircraftModelList CDbMappingComponent::getSelectedModelsToStash() const
        {
            if (!hasSelectedModelsToStash()) { return CAircraftModelList(); }
            const TabIndex tab = currentTabIndex();
            switch (tab)
            {
            case TabOwnModels:
                return ui->comp_OwnAircraftModels->view()->selectedObjects();
            case TabOwnModelSet:
                return ui->comp_OwnModelSet->view()->selectedObjects();
            case TabVPilot:
                return ui->tvp_AircraftModelsForVPilot->selectedObjects();
            default:
                break;
            }
            return CAircraftModelList();
        }

        const CAircraftModelList &CDbMappingComponent::getStashedModels() const
        {
            return ui->comp_StashAircraft->getStashedModels();
        }

        bool CDbMappingComponent::hasStashedModels() const
        {
            return !this->getStashedModels().isEmpty();
        }

        QStringList CDbMappingComponent::getStashedModelStrings() const
        {
            return ui->comp_StashAircraft->getStashedModelStrings();
        }

        CDbMappingComponent::TabIndex CDbMappingComponent::currentTabIndex() const
        {
            if (!ui->tw_ModelsToBeMapped) { return CDbMappingComponent::NoValidTab; }
            const int t = ui->tw_ModelsToBeMapped->currentIndex();
            return static_cast<TabIndex>(t);
        }

        bool CDbMappingComponent::isStashTab() const
        {
            return currentTabIndex() == TabStash;
        }

        bool CDbMappingComponent::canAddToModelSetTab() const
        {
            const bool allowed = this->currentTabIndex() == CDbMappingComponent::TabOwnModels || this->currentTabIndex() == CDbMappingComponent::TabStash;
            return allowed && this->currentModelView()->hasSelection();
        }

        CStatusMessageList CDbMappingComponent::validateCurrentModel(bool withNestedForms) const
        {
            CStatusMessageList msgs;
            if (withNestedForms)
            {
                // tests the 3 subforms and the model itself lenient
                msgs.push_back(ui->editor_ModelMapping->validate(false));
                msgs.push_back(ui->editor_AircraftModel->validate(true));
            }
            else
            {
                // model lenient
                msgs.push_back(ui->editor_ModelMapping->validate(false));
            }
            return msgs;
        }

        void CDbMappingComponent::ps_handleStashDropRequest(const CAirlineIcaoCode &code) const
        {
            const CLivery stdLivery(sGui->getWebDataServices()->getStdLiveryForAirlineCode(code));
            if (!stdLivery.hasValidDbKey()) { return; }
            ui->comp_StashAircraft->applyToSelected(stdLivery);
        }

        void CDbMappingComponent::ps_stashCurrentModel()
        {
            const bool nested = this->isStashTab(); // on stash tab, full validation, otherwise not
            CStatusMessageList msgs(this->validateCurrentModel(nested));
            if (!msgs.hasErrorMessages())
            {
                const CAircraftModel editorModel(getEditorAircraftModel());

                // from stash, do not consolidate, because we want to keep data as they are from the editor
                const bool consolidate = !this->isStashTab();
                msgs.push_back(
                    ui->comp_StashAircraft->stashModel(editorModel, true, consolidate)
                );
            }
            if (msgs.hasErrorMessages())
            {
                this->showOverlayMessages(msgs);
            }
        }

        void CDbMappingComponent::ps_displayAutoStashingDialog()
        {
            m_autoStashDialog->exec();
        }

        void CDbMappingComponent::ps_displayAutoSimulatorStashingDialog()
        {
            m_autoSimulatorDialog->exec();
        }

        void CDbMappingComponent::ps_removeDbModelsFromView()
        {
            const QStringList modelStrings(sGui->getWebDataServices()->getModelStrings());
            if (modelStrings.isEmpty()) { return; }
            switch (currentTabIndex())
            {
            case TabVPilot:
            case TabOwnModels:
            case TabOwnModelSet:
            case TabStash:
                this->currentModelView()->removeModelsWithModelString(modelStrings);
                break;
            default:
                break;
            }
        }

        void CDbMappingComponent::ps_showChangedAttributes()
        {
            if (!this->hasStashedModels()) { return; }
            if (this->currentTabIndex() != TabStash) { return; }
            ui->comp_StashAircraft->showChangedAttributes();
        }

        void CDbMappingComponent::ps_toggleAutoFiltering()
        {
            m_autoFilterInDbViews = !m_autoFilterInDbViews;
        }

        void CDbMappingComponent::ps_applyFormLiveryData()
        {
            if (ui->comp_StashAircraft->view()->selectedRowCount() < 1) { return; }
            const CStatusMessageList msgs = ui->editor_AircraftModel->validateLivery(true);
            if (msgs.hasErrorMessages())
            {
                this->showOverlayMessages(msgs);
            }
            else
            {
                ui->comp_StashAircraft->applyToSelected(ui->editor_AircraftModel->getLivery());
            }
        }

        void CDbMappingComponent::ps_applyFormAircraftIcaoData()
        {
            if (ui->comp_StashAircraft->view()->selectedRowCount() < 1) { return; }
            const CStatusMessageList msgs = ui->editor_AircraftModel->validateAircraftIcao(true);
            if (msgs.hasErrorMessages())
            {
                this->showOverlayMessages(msgs);
            }
            else
            {
                ui->comp_StashAircraft->applyToSelected(ui->editor_AircraftModel->getAircraftIcao());
            }
        }

        void CDbMappingComponent::ps_applyFormDistributorData()
        {
            if (ui->comp_StashAircraft->view()->selectedRowCount() < 1) { return; }
            const CStatusMessageList msgs = ui->editor_AircraftModel->validateDistributor(true);
            if (msgs.hasErrorMessages())
            {
                this->showOverlayMessages(msgs);
            }
            else
            {
                ui->comp_StashAircraft->applyToSelected(ui->editor_AircraftModel->getDistributor());
            }
        }

        void CDbMappingComponent::modifyModelDialog()
        {
            // only one model selected, use as default
            if (ui->comp_StashAircraft->view()->hasSingleSelectedRow())
            {
                m_modelModifyDialog->setValue(ui->comp_StashAircraft->view()->selectedObject());
            }

            const QDialog::DialogCode s = static_cast<QDialog::DialogCode>(m_modelModifyDialog->exec());
            if (s == QDialog::Rejected) { return; }
            const CPropertyIndexVariantMap vm = m_modelModifyDialog->getValues();
            ui->comp_StashAircraft->applyToSelected(vm);
        }

        void CDbMappingComponent::resizeForSelect()
        {
            this->maxTableView();
        }

        void CDbMappingComponent::resizeForMapping()
        {
            const int h = this->height(); // total height
            int h2 = ui->qw_EditorsScrollArea->minimumHeight();
            h2 *= 1.10; // desired height of inner widget + some space for scrollarea
            int currentSize = ui->sp_MappingComponent->sizes().last(); // current size
            if (h2 <= currentSize) { return; }

            int h1;
            if (h * 0.90 > h2)
            {
                // enough space to display as whole
                h1 = h - h2;
            }
            else
            {
                h1 = h / 3;
                h2 = h / 3 * 2;
            }
            const QList<int> sizes({h1, h2});
            ui->sp_MappingComponent->setSizes(sizes);
        }

        void CDbMappingComponent::maxTableView()
        {
            const int h = this->height();
            int h1 = h;
            int h2 = 0;
            const QList<int> sizes({h1, h2});
            ui->sp_MappingComponent->setSizes(sizes);
        }

        void CDbMappingComponent::ps_loadVPilotData()
        {
            if (m_vPilotReader.readInBackground(true))
            {
                CLogMessage(this).info("Start loading vPilot rulesets");
                ui->tvp_AircraftModelsForVPilot->showLoadIndicator();
            }
            else
            {
                CLogMessage(this).warning("Loading vPilot rulesets already in progress");
            }
        }

        void CDbMappingComponent::ps_onLoadVPilotDataFinished(bool success)
        {
            if (!m_vPilotEnabled) { return; }
            if (success)
            {
                CLogMessage(this).info("Loading vPilot ruleset completed");
                const CAircraftModelList models(m_vPilotReader.getAsModels());
                if (ui->tvp_AircraftModelsForVPilot->displayAutomatically())
                {
                    ui->tvp_AircraftModelsForVPilot->updateContainerMaybeAsync(models);
                }
            }
            else
            {
                CLogMessage(this).error("Loading vPilot ruleset failed");
            }
            ui->tvp_AircraftModelsForVPilot->hideLoadIndicator();
        }

        void CDbMappingComponent::ps_onVPilotCacheChanged()
        {
            if (ui->tvp_AircraftModelsForVPilot->displayAutomatically())
            {
                ui->tvp_AircraftModelsForVPilot->updateContainerMaybeAsync(m_vPilotReader.getAsModelsFromCache());
            }
            else
            {
                ui->tvp_AircraftModelsForVPilot->hideLoadIndicator();
            }
        }

        void CDbMappingComponent::ps_requestVPilotDataUpdate()
        {
            this->ps_onVPilotCacheChanged();
        }

        void CDbMappingComponent::ps_onStashedModelsChanged()
        {
            emit this->ps_digestStashedModelsChanged();
        }

        void CDbMappingComponent::ps_onStashedModelsChangedDigest()
        {
            const bool highlightVPilot = ui->tvp_AircraftModelsForVPilot->derivedModel()->highlightModelStrings();
            const bool highlightOwnModels = ui->comp_OwnAircraftModels->view()->derivedModel()->highlightModelStrings();
            const bool highlightModelSet = ui->comp_OwnModelSet->view()->derivedModel()->highlightModelStrings();
            const bool highlight =  highlightOwnModels || highlightModelSet || highlightVPilot;
            if (!highlight) { return; }
            const QStringList stashedModels(ui->comp_StashAircraft->getStashedModelStrings());
            if (highlightVPilot)
            {
                ui->tvp_AircraftModelsForVPilot->derivedModel()->setHighlightModelStrings(stashedModels);
            }
            if (highlightOwnModels)
            {
                ui->comp_OwnAircraftModels->view()->derivedModel()->setHighlightModelStrings(stashedModels);
            }
            if (highlightModelSet)
            {
                ui->comp_OwnModelSet->view()->derivedModel()->setHighlightModelStrings(stashedModels);
            }
        }

        void CDbMappingComponent::ps_tabIndexChanged(int index)
        {
            const CDbMappingComponent::TabIndex ti = static_cast<CDbMappingComponent::TabIndex>(index);
            switch (ti)
            {
            case CDbMappingComponent::TabOwnModelSet:
                {
                    ui->frp_Editors->setVisible(true);
                    ui->editor_ModelMapping->setVisible(true);
                    this->resizeForSelect();
                }
                break;
            case CDbMappingComponent::TabModelMatcher:
                {
                    ui->editor_ModelMapping->setVisible(false);
                    ui->frp_Editors->setVisible(false);
                    this->resizeForSelect();
                }
                break;
            case CDbMappingComponent::TabVPilot:
                {
                    // fall thru intended
                    this->formatVPilotView();
                }
            default:
                {
                    ui->frp_Editors->setVisible(true);
                    ui->editor_ModelMapping->setVisible(true);
                }
                break;
            }
            emit this->tabIndexChanged(index);
        }

        void CDbMappingComponent::ps_onModelsSuccessfullyPublished(const CAircraftModelList &models, bool directWrite)
        {
            if (models.isEmpty()) { return; }
            if (!directWrite) { return; } // no models wwritten, but CRs
            emit this->requestUpdatedData(CEntityFlags::ModelEntity);
        }

        void CDbMappingComponent::ps_onVPilotDataChanged(int count, bool withFilter)
        {
            Q_UNUSED(count);
            Q_UNUSED(withFilter);
            const int i = ui->tw_ModelsToBeMapped->indexOf(ui->tab_VPilot);
            QString o = ui->tw_ModelsToBeMapped->tabText(i);
            const QString f = ui->tvp_AircraftModelsForVPilot->hasFilter() ? "F" : "";
            o = CGuiUtility::replaceTabCountValue(o, ui->tvp_AircraftModelsForVPilot->rowCount()) + f;
            ui->tw_ModelsToBeMapped->setTabText(i, o);
        }

        void CDbMappingComponent::ps_onOwnModelsChanged(int count, bool withFilter)
        {
            Q_UNUSED(count);
            Q_UNUSED(withFilter);
            const int i = ui->tw_ModelsToBeMapped->indexOf(ui->tab_OwnModels);
            static const QString ot(ui->tw_ModelsToBeMapped->tabText(i));
            QString o(ot);
            const QString sim(ui->comp_OwnAircraftModels->getOwnModelsSimulator().toQString(true));
            if (!sim.isEmpty()) { o = o.append(" ").append(sim); }
            QString f = ui->comp_OwnAircraftModels->view()->hasFilter() ? "F" : "";
            o = CGuiUtility::replaceTabCountValue(o, ui->comp_OwnAircraftModels->view()->rowCount()) + f;
            ui->tw_ModelsToBeMapped->setTabText(i, o);
        }

        void CDbMappingComponent::ps_addToOwnModelSet()
        {
            if (!this->canAddToModelSetTab()) { return; }
            const CAircraftModelList models(this->currentModelView()->selectedObjects());
            const CStatusMessage m = this->addToOwnModelSet(models, this->getOwnModelsSimulator());
            CLogMessage::preformatted(m);
        }

        void CDbMappingComponent::ps_mergeWithVPilotModels()
        {
            if (!ui->comp_OwnAircraftModels->modelLoader()) { return; }
            if (m_vPilotReader.getModelsCount() < 1) { return; }
            const CSimulatorInfo sim(ui->comp_OwnAircraftModels->getOwnModelsSimulator());
            if (!sim.isSingleSimulator() || !sim.isMicrosoftOrPrepare3DSimulator()) { return; }
            CAircraftModelList ownModels(getOwnModels());
            if (ownModels.isEmpty()) { return; }
            ui->comp_OwnAircraftModels->view()->showLoadIndicator();
            CAircraftModelUtilities::mergeWithVPilotData(ownModels, m_vPilotReader.getAsModelsFromCache(), true);
            ui->comp_OwnAircraftModels->updateViewAndCache(ownModels);
        }

        void CDbMappingComponent::ps_mergeSelectedWithVPilotModels()
        {
            if (!ui->comp_OwnAircraftModels->modelLoader()) { return; }
            if (m_vPilotReader.getModelsCount() < 1) { return; }
            if (!ui->comp_OwnAircraftModels->view()->hasSelection()) { return; }
            const CSimulatorInfo sim(ui->comp_OwnAircraftModels->getOwnModelsSimulator());
            if (!sim.isSingleSimulator() || !sim.isMicrosoftOrPrepare3DSimulator()) { return; }
            CAircraftModelList ownModels(this->getOwnSelectedModels()); // subset
            if (ownModels.isEmpty()) { return; }
            ui->comp_OwnAircraftModels->view()->showLoadIndicator();
            CAircraftModelUtilities::mergeWithVPilotData(ownModels, m_vPilotReader.getAsModelsFromCache(), true);

            // full models
            CAircraftModelList allModels = m_vPilotReader.getAsModelsFromCache();
            allModels.replaceOrAddModelsWithString(ownModels, Qt::CaseInsensitive);
            ui->comp_OwnAircraftModels->updateViewAndCache(allModels);
        }

        void CDbMappingComponent::ps_onCustomContextMenu(const QPoint &point)
        {
            QPoint globalPos = this->mapToGlobal(point);
            QScopedPointer<QMenu> contextMenu(new QMenu(this));

            contextMenu->addAction("Max.data area", this, &CDbMappingComponent::resizeForSelect, QKeySequence(Qt::CTRL + Qt::Key_M, Qt::Key_D));
            contextMenu->addAction("Max.mapping area", this, &CDbMappingComponent::resizeForMapping, QKeySequence(Qt::CTRL + Qt::Key_M, Qt::Key_M));
            QAction *selectedItem = contextMenu.data()->exec(globalPos);
            Q_UNUSED(selectedItem);
        }

        void CDbMappingComponent::ps_onStashedModelsDataChanged(int count, bool withFilter)
        {
            Q_UNUSED(count);
            Q_UNUSED(withFilter);
            int i = ui->tw_ModelsToBeMapped->indexOf(ui->tab_StashAircraftModels);
            QString o = ui->tw_ModelsToBeMapped->tabText(i);
            const QString f = ui->comp_StashAircraft->view()->hasFilter() ? "F" : "";
            o = CGuiUtility::replaceTabCountValue(o, ui->comp_StashAircraft->view()->rowCount()) + f;
            ui->tw_ModelsToBeMapped->setTabText(i, o);

            // update editors
            this->updateEditorsWhenApplicable();
        }

        void CDbMappingComponent::ps_onModelSetChanged(int count, bool withFilter)
        {
            Q_UNUSED(count);
            Q_UNUSED(withFilter);
            int i = ui->tw_ModelsToBeMapped->indexOf(ui->tab_OwnModelSet);
            QString o = "Model set " + ui->comp_OwnModelSet->getModelSetSimulator().toQString(true);
            const QString f = ui->comp_OwnModelSet->view()->hasFilter() ? "F" : "";
            o = CGuiUtility::replaceTabCountValue(o, ui->comp_OwnModelSet->view()->rowCount()) + f;
            ui->tw_ModelsToBeMapped->setTabText(i, o);
        }

        void CDbMappingComponent::ps_userChanged()
        {
            this->initVPilotLoading();
        }

        void CDbMappingComponent::stashSelectedModels()
        {
            if (!this->hasSelectedModelsToStash()) { return; }
            CStatusMessageList msgs =
                ui->comp_StashAircraft->stashModels(
                    this->getSelectedModelsToStash()
                );
            if (msgs.hasWarningOrErrorMessages())
            {
                this->showOverlayMessages(msgs);
            }
        }

        void CDbMappingComponent::ps_onModelRowSelected(const QModelIndex &index)
        {
            CAircraftModel model(this->getModelFromView(index)); // data from view
            if (!model.hasModelString()) { return; }

            // we either use the model, or try to resolve the data to DB data
            bool dbModel = model.hasValidDbKey();
            const CLivery livery(dbModel ? model.getLivery() : sGui->getWebDataServices()->smartLiverySelector(model.getLivery()));
            const CAircraftIcaoCode aircraftIcao(dbModel ? model.getAircraftIcaoCode() : sGui->getWebDataServices()->smartAircraftIcaoSelector(model.getAircraftIcaoCode()));
            const CDistributor distributor(dbModel ? model.getDistributor() : sGui->getWebDataServices()->smartDistributorSelector(model.getDistributor()));

            // set model part
            ui->editor_ModelMapping->setValue(model);

            // if found, then set in editor
            if (livery.hasValidDbKey())
            {
                ui->editor_AircraftModel->setLivery(livery);
            }
            else
            {
                ui->editor_AircraftModel->clearLivery();
            }
            if (aircraftIcao.hasValidDbKey())
            {
                ui->editor_AircraftModel->setAircraftIcao(aircraftIcao);
            }
            else
            {
                ui->editor_AircraftModel->clearAircraftIcao();
            }
            if (distributor.hasValidDbKey())
            {
                ui->editor_AircraftModel->setDistributor(distributor);
            }
            else
            {
                ui->editor_AircraftModel->clearDistributor();
            }

            // request filtering
            if (m_autoFilterInDbViews)
            {
                emit filterByLivery(model.getLivery());
                emit filterByAircraftIcao(model.getAircraftIcaoCode());
                emit filterByDistributor(model.getDistributor());
            }
        }

        CAircraftModel CDbMappingComponent::getEditorAircraftModel() const
        {
            CAircraftModel model(ui->editor_ModelMapping->getValue());
            model.setDistributor(ui->editor_AircraftModel->getDistributor());
            model.setAircraftIcaoCode(ui->editor_AircraftModel->getAircraftIcao());
            model.setLivery(ui->editor_AircraftModel->getLivery());
            return model;
        }

        CAircraftModelList CDbMappingComponent::getOwnModels() const
        {
            return ui->comp_OwnAircraftModels->getOwnModels();
        }

        CAircraftModelList CDbMappingComponent::getOwnCachedModels(const CSimulatorInfo &simulator) const
        {
            return ui->comp_OwnAircraftModels->getOwnCachedModels(simulator);
        }

        CAircraftModelList CDbMappingComponent::getOwnSelectedModels() const
        {
            return ui->comp_OwnAircraftModels->getOwnSelectedModels();
        }

        CAircraftModel CDbMappingComponent::getOwnModelForModelString(const QString &modelString) const
        {
            return ui->comp_OwnAircraftModels->getOwnModelForModelString(modelString);
        }

        const CSimulatorInfo CDbMappingComponent::getOwnModelsSimulator() const
        {
            return ui->comp_OwnAircraftModels->getOwnModelsSimulator();
        }

        void CDbMappingComponent::setOwnModelsSimulator(const CSimulatorInfo &simulator)
        {
            ui->comp_OwnAircraftModels->setSimulator(simulator);
        }

        int CDbMappingComponent::getOwnModelsCount() const
        {
            return ui->comp_OwnAircraftModels->getOwnModelsCount();
        }

        QString CDbMappingComponent::getOwnModelsInfoString() const
        {
            return ui->comp_OwnAircraftModels->getInfoString();
        }

        QString CDbMappingComponent::getOwnModelsInfoStringFsFamily() const
        {
            return ui->comp_OwnAircraftModels->getInfoStringFsFamily();
        }

        void CDbMappingComponent::setOwnModelSetSimulator(const CSimulatorInfo &simulator)
        {
            Q_ASSERT_X(simulator.isSingleSimulator(), Q_FUNC_INFO, "Need single simulator");
            ui->comp_OwnModelSet->setModelSetSimulator(simulator);
        }

        CAircraftModelList CDbMappingComponent::getOwnModelSet() const
        {
            return ui->comp_OwnModelSet->getModelSet();
        }

        CStatusMessage CDbMappingComponent::stashModel(const CAircraftModel &model, bool replace)
        {
            return ui->comp_StashAircraft->stashModel(model, replace);
        }

        CStatusMessageList CDbMappingComponent::stashModels(const CAircraftModelList &models)
        {
            return ui->comp_StashAircraft->stashModels(models);
        }

        CStatusMessage CDbMappingComponent::addToOwnModelSet(const CAircraftModelList &models, const CSimulatorInfo &simulator)
        {
            return ui->comp_OwnModelSet->addToModelSet(models, simulator);
        }

        CAircraftModel CDbMappingComponent::consolidateModel(const CAircraftModel &model) const
        {
            return ui->comp_StashAircraft->consolidateModel(model);
        }

        void CDbMappingComponent::replaceStashedModelsUnvalidated(const CAircraftModelList &models) const
        {
            ui->comp_StashAircraft->replaceModelsUnvalidated(models);
        }

        void CDbMappingComponent::CMappingVPilotMenu::customMenu(CMenuActions &menuActions)
        {
            CDbMappingComponent *mapComp = qobject_cast<CDbMappingComponent *>(this->parent());
            Q_ASSERT_X(mapComp, Q_FUNC_INFO, "Cannot access mapping component");

            const bool canUseVPilot = mappingComponent()->withVPilot();
            if (canUseVPilot)
            {
                m_menuAction = menuActions.addAction(m_menuAction, CIcons::appMappings16(), "Load vPilot Rules", CMenuAction::pathVPilot(), this, { mapComp, &CDbMappingComponent::ps_loadVPilotData });
            }
            this->nestedCustomMenu(menuActions);
        }

        CDbMappingComponent *CDbMappingComponent::CMappingVPilotMenu::mappingComponent() const
        {
            return qobject_cast<CDbMappingComponent *>(this->parent());
        }

        CDbMappingComponent::CStashToolsMenu::CStashToolsMenu(CDbMappingComponent *mappingComponent, bool separator) :
            BlackGui::Menus::IMenuDelegate(mappingComponent, separator)
        {}

        void CDbMappingComponent::CStashToolsMenu::customMenu(CMenuActions &menuActions)
        {
            CDbMappingComponent *mapComp = mappingComponent();
            Q_ASSERT_X(mapComp, Q_FUNC_INFO, "no mapping component");
            if (!mapComp->currentModelView()->isEmpty() && mapComp->currentModelView()->getMenu().testFlag(CViewBaseNonTemplate::MenuCanStashModels))
            {
                menuActions.addMenuStash();

                // auto filter in DB views
                m_stashFiltering = menuActions.addAction(m_stashFiltering, CIcons::filter16(), "Auto filtering in DB views (on/off)", CMenuAction::pathStash(), this, { mapComp, &CDbMappingComponent::ps_toggleAutoFiltering });
                m_stashFiltering->setCheckable(true);
                m_stashFiltering->setChecked(mapComp->m_autoFilterInDbViews);

                m_autoStashing = menuActions.addAction(m_autoStashing, CIcons::appDbStash16(), "Auto stashing", CMenuAction::pathStash(), this, { mapComp, &CDbMappingComponent::ps_displayAutoStashingDialog });
                m_autoSimulatorStashing = menuActions.addAction(m_autoSimulatorStashing, CIcons::appDbStash16(), "Cross simulator updating (FSX-P3D-FS9)", CMenuAction::pathStash(), this, { mapComp, &CDbMappingComponent::ps_displayAutoSimulatorStashingDialog });
                if (mapComp->m_autoStashDialog && mapComp->m_autoStashDialog->isCompleted())
                {
                    menuActions.addAction(CIcons::appDbStash16(), "Last auto stash run", CMenuAction::pathStash(), nullptr, { mapComp->m_autoStashDialog.data(), &CDbAutoStashingComponent::showLastResults });
                }
            }
            else if (mapComp->currentTabIndex() == CDbMappingComponent::TabStash)
            {
                this->addStashViewSpecificMenus(menuActions);
            }
            this->nestedCustomMenu(menuActions);
        }

        void CDbMappingComponent::CStashToolsMenu::addStashViewSpecificMenus(CMenuActions &menuActions)
        {
            CDbMappingComponent *mapComp = mappingComponent();
            Q_ASSERT_X(mapComp, Q_FUNC_INFO, "no mapping component");

            const int dbModels = sGui->getWebDataServices()->getModelsCount();
            if (dbModels > 0 && mapComp->hasStashedModels())
            {
                menuActions.addMenu(CIcons::appDbStash16(), "Stash", CMenuAction::pathStash());

                // we have keys and data by which we could delete them from view
                const QString msgDelete("Delete " + QString::number(dbModels) + " DB model(s) from '" + mapComp->currentTabText() + "'");
                menuActions.addAction(CIcons::delete16(), msgDelete, CMenuAction::pathStash(), nullptr, { mapComp, &CDbMappingComponent::ps_removeDbModelsFromView});

                // attribute info
                menuActions.addAction(CIcons::info16(), "Show changed attributes", CMenuAction::pathStash(), nullptr, { mapComp, &CDbMappingComponent::ps_showChangedAttributes});
            }
        }

        CDbMappingComponent *CDbMappingComponent::CStashToolsMenu::mappingComponent() const
        {
            return qobject_cast<CDbMappingComponent *>(this->parent());
        }

        void CDbMappingComponent::COwnModelSetMenu::customMenu(CMenuActions &menuActions)
        {
            CDbMappingComponent *mapComp = mappingComponent();
            Q_ASSERT_X(mapComp, Q_FUNC_INFO, "no mapping component");
            if (mapComp->canAddToModelSetTab())
            {
                menuActions.addMenuModelSet();
                m_menuAction = menuActions.addAction(m_menuAction, CIcons::appModels16(), "Add to own model set", CMenuAction::pathModelSet(), this, { mapComp, &CDbMappingComponent::ps_addToOwnModelSet });
            }
            this->nestedCustomMenu(menuActions);
        }

        CDbMappingComponent *CDbMappingComponent::COwnModelSetMenu::mappingComponent() const
        {
            return qobject_cast<CDbMappingComponent *>(this->parent());
        }

        void CDbMappingComponent::CApplyDbDataMenu::customMenu(CMenuActions &menuActions)
        {
            CDbMappingComponent *mapComp = mappingComponent();
            Q_ASSERT_X(mapComp, Q_FUNC_INFO, "no mapping component");

            if (mapComp->currentTabIndex() == CDbMappingComponent::TabStash && mapComp->currentModelView()->hasSelection())
            {
                if (m_menuActions.isEmpty()) { m_menuActions = QList<QAction *>({ nullptr, nullptr, nullptr, nullptr }); }

                // stash view and selection
                menuActions.addMenuStashEditor();

                m_menuActions[0] = menuActions.addAction(m_menuActions[0], CIcons::appAircraftIcao16(), "Current aircraft ICAO", CMenuAction::pathStashEditor(), this, { mapComp, &CDbMappingComponent::ps_applyFormAircraftIcaoData });
                m_menuActions[1] = menuActions.addAction(m_menuActions[1], CIcons::appDistributors16(), "Current distributor", CMenuAction::pathStashEditor(), this, { mapComp, &CDbMappingComponent::ps_applyFormDistributorData });
                m_menuActions[2] = menuActions.addAction(m_menuActions[2], CIcons::appLiveries16(), "Current livery", CMenuAction::pathStashEditor(), this, { mapComp, &CDbMappingComponent::ps_applyFormLiveryData });
                m_menuActions[3] = menuActions.addAction(m_menuActions[3], CIcons::databaseTable16(), "Modify DB model data", CMenuAction::pathStashEditor(), this, { mapComp, &CDbMappingComponent::modifyModelDialog });
            }
            this->nestedCustomMenu(menuActions);
        }

        CDbMappingComponent *CDbMappingComponent::CApplyDbDataMenu::mappingComponent() const
        {
            return qobject_cast<CDbMappingComponent *>(this->parent());
        }

        CDbMappingComponent::CMergeWithVPilotMenu::CMergeWithVPilotMenu(CDbMappingComponent *mappingComponent, bool separator) :
            IMenuDelegate(mappingComponent, separator)
        {
            Q_ASSERT_X(mappingComponent, Q_FUNC_INFO, "Missing vPilot reader");
        }

        void CDbMappingComponent::CMergeWithVPilotMenu::customMenu(CMenuActions &menuActions)
        {
            const CAircraftModelView *mv = mappingComponent()->ui->comp_OwnAircraftModels->view();
            const CSimulatorInfo sim = mappingComponent()->ui->comp_OwnAircraftModels->getOwnModelsSimulator();
            if (!mappingComponent()->withVPilot() || mv->isEmpty() || !sim.isSingleSimulator() || !sim.isMicrosoftOrPrepare3DSimulator())
            {
                this->nestedCustomMenu(menuActions);
                return;
            }

            if (m_menuActions.isEmpty()) { m_menuActions = QList<QAction *>({ nullptr, nullptr }); }
            menuActions.addMenu("Merge with vPilot data", CMenuAction::pathVPilot());
            m_menuActions[0] = menuActions.addAction(m_menuActions[0], "All", CMenuAction::pathVPilot(), this, { mappingComponent(), &CDbMappingComponent::ps_mergeWithVPilotModels });
            if (mv->hasSelection())
            {
                m_menuActions[1] = menuActions.addAction(m_menuActions[1], "Selected only", CMenuAction::pathVPilot(), this, { mappingComponent(), &CDbMappingComponent::ps_mergeSelectedWithVPilotModels });
            }
            this->nestedCustomMenu(menuActions);
        }

        CDbMappingComponent *CDbMappingComponent::CMergeWithVPilotMenu::mappingComponent() const
        {
            return qobject_cast<CDbMappingComponent *>(this->parent());
        }
    } // ns
} // ns

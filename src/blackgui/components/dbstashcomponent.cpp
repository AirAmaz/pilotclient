/* Copyright (C) 2013
 * swift project Community / Contributors
 *
 * This file is part of swift Project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "blackcore/db/databasewriter.h"
#include "blackcore/db/databaseutils.h"
#include "blackcore/application.h"
#include "blackcore/webdataservices.h"
#include "blackgui/components/dbmappingcomponent.h"
#include "blackgui/components/dbstashcomponent.h"
#include "blackgui/guiapplication.h"
#include "blackgui/models/aircraftmodellistmodel.h"
#include "blackgui/views/aircraftmodelview.h"
#include "blackgui/views/viewbase.h"
#include "blackmisc/aviation/aircrafticaocode.h"
#include "blackmisc/aviation/livery.h"
#include "blackmisc/logcategory.h"
#include "blackmisc/logcategorylist.h"
#include "blackmisc/sequence.h"
#include "blackmisc/simulation/aircraftmodel.h"
#include "blackmisc/simulation/distributorlist.h"
#include "blackmisc/verify.h"
#include "ui_dbstashcomponent.h"

#include <QCheckBox>
#include <QMessageBox>
#include <QPushButton>
#include <QWidget>
#include <Qt>
#include <QtGlobal>
#include <iterator>

using namespace BlackCore;
using namespace BlackCore::Db;
using namespace BlackMisc;
using namespace BlackMisc::Simulation;
using namespace BlackMisc::Aviation;
using namespace BlackMisc::Network;
using namespace BlackGui;
using namespace BlackGui::Models;
using namespace BlackGui::Views;

namespace BlackGui
{
    namespace Components
    {
        CDbStashComponent::CDbStashComponent(QWidget *parent) :
            QFrame(parent),
            CDbMappingComponentAware(parent),
            ui(new Ui::CDbStashComponent)
        {
            ui->setupUi(this);
            ui->tvp_StashAircraftModels->setAircraftModelMode(CAircraftModelListModel::StashModel);
            ui->tvp_StashAircraftModels->allowDragDrop(false, true);
            ui->tvp_StashAircraftModels->setAcceptedMetaTypeIds();
            ui->tvp_StashAircraftModels->menuAddItems(CAircraftModelView::MenuLoadAndSave);

            connect(ui->pb_Unstash, &QPushButton::pressed, this, &CDbStashComponent::ps_onUnstashPressed);
            connect(ui->pb_Validate, &QPushButton::pressed, this, &CDbStashComponent::ps_onValidatePressed);
            connect(ui->pb_RemoveInvald, &QPushButton::pressed, this, &CDbStashComponent::ps_onRemoveInvalidPressed);
            connect(ui->pb_Publish, &QPushButton::pressed, this, &CDbStashComponent::ps_onPublishPressed);
            connect(ui->tvp_StashAircraftModels, &CAircraftModelView::modelChanged, this, &CDbStashComponent::stashedModelsChanged);
            connect(ui->tvp_StashAircraftModels, &CAircraftModelView::modelDataChanged, this, &CDbStashComponent::ps_onRowCountChanged);

            // copy over buttons
            connect(ui->pb_AircraftIcao, &QPushButton::pressed, this, &CDbStashComponent::ps_copyOverPartsToSelected);
            connect(ui->pb_AirlineIcao, &QPushButton::pressed, this, &CDbStashComponent::ps_copyOverPartsToSelected);
            connect(ui->pb_Livery, &QPushButton::pressed, this, &CDbStashComponent::ps_copyOverPartsToSelected);
            connect(ui->pb_Distributor, &QPushButton::pressed, this, &CDbStashComponent::ps_copyOverPartsToSelected);
            connect(ui->pb_Model, &QPushButton::pressed, this, &CDbStashComponent::ps_modifyModelDialog);

            ui->tvp_StashAircraftModels->menuAddItems(CAircraftModelView::MenuRemoveSelectedRows);
            ui->tvp_StashAircraftModels->setHighlightModelStrings(true);
            ui->tvp_StashAircraftModels->setHighlightModelStringsColor(Qt::red);
            this->enableButtonRow();

            connect(sApp->getWebDataServices()->getDatabaseWriter(), &CDatabaseWriter::publishedModels, this, &CDbStashComponent::ps_publishedModelsResponse);
            this->ps_userChanged();
        }

        CDbStashComponent::~CDbStashComponent()
        { }

        CStatusMessage CDbStashComponent::validateStashModel(const CAircraftModel &model, bool allowReplace) const
        {
            if (!allowReplace && ui->tvp_StashAircraftModels->container().containsModelStringOrDbKey(model))
            {
                const QString msg("Model '%1' already stashed");
                return CStatusMessage(validationCategories(), CStatusMessage::SeverityError, msg.arg(model.getModelString()));
            }
            return CStatusMessage();
        }

        CStatusMessage CDbStashComponent::stashModel(const CAircraftModel &model, bool replace, bool consolidateWithDbData)
        {
            const CAircraftModel stashModel(consolidateWithDbData ? this->consolidateModel(model) : model);
            const CStatusMessage m(validateStashModel(stashModel, replace));
            if (!m.isWarningOrAbove())
            {
                if (replace)
                {
                    ui->tvp_StashAircraftModels->replaceOrAdd(&CAircraftModel::getModelString, stashModel.getModelString(), stashModel);
                }
                else
                {
                    ui->tvp_StashAircraftModels->insert(stashModel);
                }
            }
            return m;
        }

        CStatusMessageList CDbStashComponent::stashModels(const CAircraftModelList &models)
        {
            if (models.isEmpty()) { return CStatusMessageList(); }
            CStatusMessageList msgs;
            for (const CAircraftModel &model : models)
            {
                const CStatusMessage m(stashModel(model));
                if (m.isWarningOrAbove()) { msgs.push_back(m); }
            }
            return msgs;
        }

        void CDbStashComponent::replaceModelsUnvalidated(const CAircraftModelList &models)
        {
            ui->tvp_StashAircraftModels->updateContainerMaybeAsync(models);
        }

        int CDbStashComponent::unstashModels(QSet<int> keys)
        {
            if (keys.isEmpty()) { return 0; }
            return ui->tvp_StashAircraftModels->removeDbKeys(keys);
        }

        int CDbStashComponent::unstashModels(QStringList modelStrings)
        {
            if (modelStrings.isEmpty()) { return 0; }
            return ui->tvp_StashAircraftModels->removeModelsWithModelString(modelStrings);
        }

        int CDbStashComponent::unstashModels(const CAircraftModelList &models)
        {
            if (models.isEmpty()) { return 0; }
            return ui->tvp_StashAircraftModels->removeModelsWithModelString(models);
        }

        CAircraftModelView *CDbStashComponent::view() const
        {
            return ui->tvp_StashAircraftModels;
        }

        bool CDbStashComponent::hasStashedModels() const
        {
            return !ui->tvp_StashAircraftModels->isEmpty();
        }

        int CDbStashComponent::getStashedModelsCount() const
        {
            return ui->tvp_StashAircraftModels->rowCount();
        }

        QStringList CDbStashComponent::getStashedModelStrings() const
        {
            return ui->tvp_StashAircraftModels->derivedModel()->getModelStrings(false);
        }

        const CAircraftModelList &CDbStashComponent::getStashedModels() const
        {
            return ui->tvp_StashAircraftModels->derivedModel()->container();
        }

        CAircraftModel CDbStashComponent::getStashedModel(const QString &modelString) const
        {
            if (modelString.isEmpty() || ui->tvp_StashAircraftModels->isEmpty()) { return CAircraftModel(); }
            return ui->tvp_StashAircraftModels->container().findFirstByModelStringOrDefault(modelString);
        }

        void CDbStashComponent::applyToSelected(const CLivery &livery, bool acceptWarnings)
        {
            if (!ui->tvp_StashAircraftModels->hasSelection()) { return; }
            CStatusMessageList msgs(livery.validate());
            if (this->showMessages(msgs, acceptWarnings)) { return; }
            ui->tvp_StashAircraftModels->applyToSelected(livery);
        }

        void CDbStashComponent::applyToSelected(const CAircraftIcaoCode &icao, bool acceptWarnings)
        {
            if (!ui->tvp_StashAircraftModels->hasSelection()) { return; }
            CStatusMessageList msgs(icao.validate());
            if (this->showMessages(msgs, acceptWarnings)) { return; }
            ui->tvp_StashAircraftModels->applyToSelected(icao);
        }

        void CDbStashComponent::applyToSelected(const CAirlineIcaoCode &icao, bool acceptWarnings)
        {
            if (!icao.hasValidDesignator())
            {
                static const CStatusMessage msg(CStatusMessage::SeverityError, "No valid designator");
                this->showMessage(msg);
                return;
            }

            // retrieve the std livery
            const CLivery stdLivery(sApp->getWebDataServices()->getStdLiveryForAirlineCode(icao));
            if (!stdLivery.hasValidDbKey())
            {
                static const CStatusMessage msg(CStatusMessage::SeverityError, "No valid standard livery for " + icao.getDesignator());
                this->showMessage(msg);
                return;
            }

            applyToSelected(stdLivery, acceptWarnings);
        }

        void CDbStashComponent::applyToSelected(const CDistributor &distributor, bool acceptWarnings)
        {
            if (!ui->tvp_StashAircraftModels->hasSelection()) { return; }
            CStatusMessageList msgs(distributor.validate());
            if (this->showMessages(msgs, acceptWarnings)) { return; }
            ui->tvp_StashAircraftModels->applyToSelected(distributor);
        }

        void CDbStashComponent::applyToSelected(const CPropertyIndexVariantMap &vm)
        {
            if (vm.isEmpty()) { return; }
            if (!ui->tvp_StashAircraftModels->hasSelection()) { return; }
            ui->tvp_StashAircraftModels->applyToSelected(vm);
        }

        void CDbStashComponent::ps_onUnstashPressed()
        {
            ui->tvp_StashAircraftModels->removeSelectedRows();
        }

        void CDbStashComponent::ps_onValidatePressed()
        {
            if (ui->tvp_StashAircraftModels->isEmpty()) {return; }
            CAircraftModelList validModels;
            CAircraftModelList invalidModels;
            this->validateAndDisplay(validModels, invalidModels, true);
        }

        void CDbStashComponent::ps_onRemoveInvalidPressed()
        {
            if (ui->tvp_StashAircraftModels->isEmpty()) {return; }
            CAircraftModelList validModels;
            CAircraftModelList invalidModels;
            this->validate(validModels, invalidModels);
            this->unstashModels(invalidModels);
        }

        void CDbStashComponent::ps_onPublishPressed()
        {
            if (ui->tvp_StashAircraftModels->isEmpty()) {return; }

            // get models right here, because later steps might affect selection
            const CAircraftModelList models(getSelectedOrAllModels());
            if (models.isEmpty()) { return; }

            // validate
            CAircraftModelList validModels;
            CAircraftModelList invalidModels;
            if (!this->validateAndDisplay(validModels, invalidModels)) { return; }
            CStatusMessageList msgs;
            if (validModels.size() > MaxModelPublished)
            {
                validModels.truncate(MaxModelPublished);
                msgs.push_back(CStatusMessage(validationCategories(), CStatusMessage::SeverityWarning, QString("More than %1 values, values skipped").arg(MaxModelPublished)));
            }

            msgs.push_back(sApp->getWebDataServices()->asyncPublishModels(validModels));
            if (msgs.hasWarningOrErrorMessages())
            {
                this->showMessages(msgs);
            }
            else
            {
                ui->tvp_StashAircraftModels->showLoadIndicator();
            }
        }

        void CDbStashComponent::ps_publishedModelsResponse(const CAircraftModelList &publishedModels, const CAircraftModelList &skippedModels, const CStatusMessageList &msgs, bool sendingSuccesful, bool directWrite)
        {
            ui->tvp_StashAircraftModels->hideLoadIndicator();
            if (!publishedModels.isEmpty() && sendingSuccesful)
            {
                emit this->modelsSuccessfullyPublished(publishedModels, directWrite);
            }

            if (!msgs.isEmpty())
            {
                if (publishedModels.isEmpty())
                {
                    this->showMessages(msgs);
                }
                else
                {
                    const QString confirm("Remove %1 published models from stash?");
                    auto lambda = [this, publishedModels]()
                    {
                        this->unstashModels(publishedModels.getModelStringList(false));
                    };
                    this->showMessagesWithConfirmation(msgs, confirm.arg(publishedModels.size()), lambda, QMessageBox::Ok);
                }
            }

            Q_UNUSED(publishedModels);
            Q_UNUSED(skippedModels);
        }

        CStatusMessageList CDbStashComponent::validate(CAircraftModelList &validModels, CAircraftModelList &invalidModels) const
        {
            if (ui->tvp_StashAircraftModels->isEmpty()) {return CStatusMessageList(); }
            Q_ASSERT_X(sGui->getWebDataServices(), Q_FUNC_INFO, "No web services");
            const CAircraftModelList models(getSelectedOrAllModels());
            if (models.isEmpty()) { return CStatusMessageList(); }
            const CStatusMessageList msgs(sGui->getWebDataServices()->validateForPublishing(models, validModels, invalidModels));

            // OK?
            if (msgs.isEmpty())
            {
                return CStatusMessageList(
                {
                    CStatusMessage(validationCategories(), CStatusMessage::SeverityInfo, QString("No errors in %1 model(s)").arg(models.size()))
                });
            }
            else
            {
                return msgs;
            }
        }

        bool CDbStashComponent::validateAndDisplay(CAircraftModelList &validModels, CAircraftModelList &invalidModels, bool displayInfo)
        {
            const CStatusMessageList msgs(this->validate(validModels, invalidModels));
            if (msgs.hasWarningOrErrorMessages())
            {
                this->showMessages(msgs);
                ui->tvp_StashAircraftModels->setHighlightModelStrings(invalidModels.getModelStringList(false));
            }
            else
            {
                // delete highlighting because no errors
                ui->tvp_StashAircraftModels->clearHighlighting();
                if (displayInfo)
                {
                    const QString no = QString::number(this->getStashedModelsCount());
                    CStatusMessage msg(validationCategories(), CStatusMessage::SeverityInfo, "Validation passed for " + no + " models");
                    this->showMessage(msg);
                }
            }
            return !validModels.isEmpty(); // at least some valid objects
        }

        void CDbStashComponent::enableButtonRow()
        {
            bool e = !ui->tvp_StashAircraftModels->isEmpty();
            ui->pb_AircraftIcao->setEnabled(e);
            ui->pb_AirlineIcao->setEnabled(e);
            ui->pb_Distributor->setEnabled(e);
            ui->pb_Livery->setEnabled(e);
            ui->pb_Unstash->setEnabled(e);
            ui->pb_Validate->setEnabled(e);
            this->ps_userChanged();
        }

        const CLogCategoryList &CDbStashComponent::validationCategories() const
        {
            static const CLogCategoryList cats(CLogCategoryList(this).join({ CLogCategory::validation()}));
            return cats;
        }

        CAircraftModelList CDbStashComponent::getSelectedOrAllModels() const
        {
            bool selectedOnly = ui->cb_SelectedOnly->isChecked();
            const CAircraftModelList models(selectedOnly ? ui->tvp_StashAircraftModels->selectedObjects() : ui->tvp_StashAircraftModels->containerOrFilteredContainer());
            return models;
        }

        CAircraftModel CDbStashComponent::consolidateWithDbData(const CAircraftModel &model) const
        {
            const CAircraftModel consolidatedModel = CDatabaseUtils::consolidateModelWithDbData(model, true);
            return consolidatedModel;
        }

        CAircraftModel CDbStashComponent::consolidateWithOwnModels(const CAircraftModel &model) const
        {
            if (!model.hasModelString()) { return model; }
            if (model.getModelType() == CAircraftModel::TypeOwnSimulatorModel) { return model; }
            CAircraftModel ownModel(this->getMappingComponent()->getOwnModelForModelString(model.getModelString()));
            if (!ownModel.hasModelString()) { return model; }
            ownModel.updateMissingParts(model);
            return ownModel;
        }

        CAuthenticatedUser CDbStashComponent::getSwiftDbUser() const
        {
            return this->m_swiftDbUser.get();
        }

        CAircraftModel CDbStashComponent::consolidateModel(const CAircraftModel &model) const
        {
            CAircraftModel stashModel(model);
            bool ownModel = stashModel.getModelType() == CAircraftModel::TypeOwnSimulatorModel;

            // merge with DB data if any
            if (!stashModel.hasValidDbKey())
            {
                stashModel = this->consolidateWithDbData(stashModel);
            }

            // merge with own models if any
            if (!ownModel)
            {
                stashModel = this->consolidateWithOwnModels(stashModel);
            }

            return stashModel;
        }

        void CDbStashComponent::ps_copyOverPartsToSelected()
        {
            QObject *sender = QObject::sender();
            BLACK_VERIFY_X(this->getMappingComponent(), Q_FUNC_INFO, "missing mapping component");
            if (!this->getMappingComponent()) { return; }
            if (!ui->tvp_StashAircraftModels->hasSelection()) { return; }

            CAircraftModel model(this->getMappingComponent()->getEditorAircraftModel());
            if (sender == ui->pb_AircraftIcao)
            {
                this->applyToSelected(model.getAircraftIcaoCode());
            }
            else if (sender == ui->pb_AirlineIcao)
            {
                this->applyToSelected(model.getAirlineIcaoCode());
            }
            else if (sender == ui->pb_Distributor)
            {
                this->applyToSelected(model.getDistributor());
            }
            else if (sender == ui->pb_Livery)
            {
                this->applyToSelected(model.getLivery());
            }
        }

        void CDbStashComponent::ps_modifyModelDialog()
        {
            if (this->getMappingComponent())
            {
                this->getMappingComponent()->modifyModelDialog();
            }
        }

        void CDbStashComponent::ps_onRowCountChanged(int number, bool filter)
        {
            Q_UNUSED(number);
            Q_UNUSED(filter);
            this->enableButtonRow();
        }

        void CDbStashComponent::ps_userChanged()
        {
            const CAuthenticatedUser user(this->getSwiftDbUser());
            if (!user.isAuthenticated())
            {
                ui->pb_Publish->setText("Publish (login)");
                ui->pb_Publish->setToolTip("Login first");
                ui->pb_Publish->setEnabled(false);
            }
            else if (user.canDirectlyWriteModels())
            {
                ui->pb_Publish->setText("Publish (direct)");
                ui->pb_Publish->setToolTip("Models directly released");
                ui->pb_Publish->setEnabled(true);
            }
            else
            {
                ui->pb_Publish->setText("Publish (CR)");
                ui->pb_Publish->setToolTip("Models published as change request");
                ui->pb_Publish->setEnabled(true);
            }
        }

        bool CDbStashComponent::showMessages(const CStatusMessageList &msgs, bool onlyErrors, int timeoutMs)
        {
            if (msgs.isEmpty()) { return false; }
            if (!msgs.hasErrorMessages() && onlyErrors) { return false; }
            BLACK_VERIFY_X(this->getMappingComponent(), Q_FUNC_INFO, "missing mapping component");
            if (!this->getMappingComponent()) { return false; }
            this->getMappingComponent()->showOverlayMessages(msgs, timeoutMs);
            return true;
        }

        bool CDbStashComponent::showMessagesWithConfirmation(const CStatusMessageList &msgs, const QString &confirmation, std::function<void ()> okLambda, int defaultButton, bool onlyErrors, int timeoutMs)
        {
            if (msgs.isEmpty()) { return false; }
            if (!msgs.hasErrorMessages() && onlyErrors) { return false; }
            BLACK_VERIFY_X(this->getMappingComponent(), Q_FUNC_INFO, "missing mapping component");
            if (!this->getMappingComponent()) { return false; }
            this->getMappingComponent()->showOverlayMessagesWithConfirmation(msgs, confirmation, okLambda, defaultButton, timeoutMs);
            return true;
        }

        bool CDbStashComponent::showMessage(const CStatusMessage &msg, int timeoutMs)
        {
            if (msg.isEmpty()) { return false; }
            BLACK_VERIFY_X(this->getMappingComponent(), Q_FUNC_INFO, "missing mapping component");
            if (!this->getMappingComponent()) { return false; }
            this->getMappingComponent()->showOverlayMessage(msg, timeoutMs);
            return true;
        }
    } // ns
} // ns

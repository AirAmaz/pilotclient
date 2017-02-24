/* Copyright (C) 2013
 * swift project Community / Contributors
 *
 * This file is part of swift Project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "blackconfig/buildconfig.h"
#include "blackcore/context/contextaudio.h"
#include "blackcore/context/contextnetwork.h"
#include "blackcore/context/contextownaircraft.h"
#include "blackcore/context/contextsimulator.h"
#include "blackcore/webdataservices.h"
#include "blackcore/data/globalsetup.h"
#include "blackcore/network.h"
#include "blackcore/simulator.h"
#include "logincomponent.h"
#include "serverlistselector.h"
#include "dbquickmappingwizard.h"
#include "blackgui/editors/serverform.h"
#include "blackgui/guiapplication.h"
#include "blackgui/loginmodebuttons.h"
#include "blackgui/ticklabel.h"
#include "blackgui/uppercasevalidator.h"
#include "blackmisc/aviation/aircrafticaocode.h"
#include "blackmisc/aviation/airlineicaocode.h"
#include "blackmisc/aviation/airporticaocode.h"
#include "blackmisc/icons.h"
#include "blackmisc/logmessage.h"
#include "blackmisc/network/entityflags.h"
#include "blackmisc/network/serverlist.h"
#include "blackmisc/simulation/aircraftmodel.h"
#include "blackmisc/simulation/simulatedaircraft.h"
#include "blackmisc/statusmessage.h"
#include "ui_logincomponent.h"

#include <QDialogButtonBox>
#include <QGroupBox>
#include <QIntValidator>
#include <QLineEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QTabWidget>
#include <QTimer>
#include <QToolButton>
#include <QCompleter>
#include <QStyledItemDelegate>
#include <QtGlobal>

using namespace BlackConfig;
using namespace BlackMisc;
using namespace BlackMisc::Aviation;
using namespace BlackMisc::Network;
using namespace BlackMisc::Simulation;
using namespace BlackCore;
using namespace BlackCore::Data;
using namespace BlackCore::Context;
using namespace BlackGui;

namespace BlackGui
{
    namespace Components
    {
        const CLogCategoryList &CLoginComponent::getLogCategories()
        {
            static const BlackMisc::CLogCategoryList cats { BlackMisc::CLogCategory::guiComponent() };
            return cats;
        }

        CLoginComponent::CLoginComponent(QWidget *parent) :
            QFrame(parent),
            ui(new Ui::CLoginComponent)
        {
            ui->setupUi(this);
            this->m_logoffCountdownTimer = new QTimer(this);
            this->m_logoffCountdownTimer->setObjectName("CLoginComponent:m_logoffCountdownTimer");
            ui->pb_LogoffTimeout->setMaximum(LogoffIntervalSeconds);
            ui->pb_LogoffTimeout->setValue(LogoffIntervalSeconds);
            connect(this->m_logoffCountdownTimer, &QTimer::timeout, this, &CLoginComponent::ps_logoffCountdown);

            ui->selector_AircraftIcao->displayWithIcaoDescription(false);
            ui->selector_AirlineIcao->displayWithIcaoDescription(false);
            ui->selector_AircraftIcao->displayMode(CDbAircraftIcaoSelectorComponent::DisplayIcaoAndId);
            ui->selector_AirlineIcao->displayMode(CDbAirlineIcaoSelectorComponent::DisplayVDesignatorAndId);

            this->setOkButtonString(false);
            connect(ui->bb_OkCancel, &QDialogButtonBox::rejected, this, &CLoginComponent::ps_loginCancelled);
            connect(ui->bb_OkCancel, &QDialogButtonBox::accepted, this, &CLoginComponent::ps_toggleNetworkConnection);
            connect(ui->pb_OtherServersGotoSettings, &QPushButton::pressed, this, &CLoginComponent::requestNetworkSettings);
            connect(ui->tb_MappingWizard, &QToolButton::clicked, this, &CLoginComponent::ps_mappingWizard);

            ui->comp_FsdDetails->showEnableInfo(true);
            ui->comp_FsdDetails->setFsdSetupEnabled(false);

            ui->lblp_AircraftCombinedType->setToolTips("ok", "wrong");
            ui->lblp_AirlineIcao->setToolTips("ok", "wrong");
            ui->lblp_AircraftIcao->setToolTips("ok", "wrong");
            ui->lblp_Callsign->setToolTips("ok", "wrong");
            ui->lblp_VatsimHomeAirport->setToolTips("ok", "wrong");
            ui->lblp_VatsimId->setToolTips("ok", "wrong");
            ui->lblp_VatsimPassword->setToolTips("ok", "wrong");
            ui->lblp_VatsimRealName->setToolTips("ok", "wrong");

            // Stored data
            this->loadRememberedVatsimData();

            // Remark: The validators affect the signals such as returnPressed, editingFinished
            // So I use no ranges in the CUpperCaseValidators, as this disables the signals for invalid values

            // VATSIM
            ui->le_VatsimId->setValidator(new QIntValidator(100000, 9999999, this));
            connect(ui->le_VatsimId, &QLineEdit::editingFinished, this, &CLoginComponent::ps_validateVatsimValues);

            ui->le_VatsimHomeAirport->setValidator(new CUpperCaseValidator(this));
            connect(ui->le_VatsimHomeAirport, &QLineEdit::editingFinished, this, &CLoginComponent::ps_validateVatsimValues);

            connect(ui->le_VatsimPassword, &QLineEdit::editingFinished, this, &CLoginComponent::ps_validateVatsimValues);
            connect(ui->le_VatsimRealName, &QLineEdit::editingFinished, this, &CLoginComponent::ps_validateVatsimValues);

            // own aircraft
            ui->le_Callsign->setMaxLength(LogoffIntervalSeconds);
            ui->le_Callsign->setValidator(new CUpperCaseValidator(this));
            connect(ui->le_Callsign, &QLineEdit::editingFinished, this, &CLoginComponent::ps_validateAircraftValues);

            ui->le_AircraftCombinedType->setMaxLength(3);
            ui->le_AircraftCombinedType->setValidator(new CUpperCaseValidator(this));
            connect(ui->le_AircraftCombinedType, &QLineEdit::editingFinished, this, &CLoginComponent::ps_validateAircraftValues);
            connect(ui->selector_AircraftIcao, &CDbAircraftIcaoSelectorComponent::changedAircraftIcao, this, &CLoginComponent::ps_changedAircraftIcao);
            connect(ui->selector_AirlineIcao, &CDbAirlineIcaoSelectorComponent::changedAirlineIcao, this, &CLoginComponent::ps_changedAirlineIcao);
            connect(ui->tb_SimulatorIcaoReverseLookup, &QToolButton::clicked, this, &CLoginComponent::ps_reverseLookupAircraftModel);

            if (sGui && sGui->getIContextSimulator())
            {
                connect(sGui->getIContextSimulator(), &IContextSimulator::ownAircraftModelChanged, this, &CLoginComponent::ps_simulatorModelChanged);
            }

            // server GUI element
            ui->frp_CurrentServer->setReadOnly(true);
            ui->frp_CurrentServer->showPasswordField(false);

            connect(sGui->getIContextNetwork(), &IContextNetwork::webServiceDataRead, this, &CLoginComponent::ps_onWebServiceDataRead);

            // inital setup, if data already available
            this->ps_validateAircraftValues();
            this->ps_validateVatsimValues();
            this->ps_onWebServiceDataRead(CEntityFlags::VatsimDataFile, CEntityFlags::ReadFinished, -1);
            CServerList otherServers(this->m_otherTrafficNetworkServers.getThreadLocal());

            // add a testserver when no servers can be loaded
            if (otherServers.isEmpty() && (sGui->isRunningInDeveloperEnvironment() || CBuildConfig::isBetaTest()))
            {
                otherServers.push_back(sGui->getGlobalSetup().getFsdTestServersPlusHardcodedServers());
                CLogMessage(this).info("Added servers for testing");
            }
            ui->comp_OtherServers->setServers(otherServers);

            // init completers if data are already available
            if (sGui && sGui->hasWebDataServices())
            {
                this->initCompleters(CEntityFlags::AircraftIcaoEntity | CEntityFlags::AirlineIcaoEntity | CEntityFlags::AirportEntity);
            }
        }

        CLoginComponent::~CLoginComponent()
        { }

        void CLoginComponent::mainInfoAreaChanged(const QWidget *currentWidget)
        {
            this->m_logoffCountdownTimer->stop(); // in any case stop the timer
            if (currentWidget != this && currentWidget != this->parentWidget())
            {
                const bool wasVisible = this->m_visible;
                this->m_visible = false;
                this->m_logoffCountdownTimer->stop();

                if (wasVisible)
                {
                    // set own values, and send signals
                    this->setOwnModelAndIcaoValues();
                }
            }
            else
            {
                this->setOwnModelAndIcaoValues();
                if (this->m_visible)
                {
                    // already visible:
                    // re-trigger! treat as same as OK
                    this->ps_toggleNetworkConnection();
                }
                else
                {
                    this->m_visible = true;
                    const bool isConnected = sGui->getIContextNetwork()->isConnected();
                    this->setGuiVisibility(isConnected);
                    this->setOkButtonString(isConnected);
                    if (isConnected) { this->startLogoffTimerCountdown(); }
                }
            }
        }

        void CLoginComponent::ps_loginCancelled()
        {
            this->m_logoffCountdownTimer->stop();
            ui->pb_LogoffTimeout->setValue(LogoffIntervalSeconds);
            emit this->loginOrLogoffCancelled();
        }

        void CLoginComponent::ps_toggleNetworkConnection()
        {
            if (ui->tw_Network->currentWidget() == ui->pg_FsdDetails)
            {
                CLogMessage(this).validationError("No login possible from this very tab, use VATSIM or other servers");
                return;
            }
            const bool isConnected = sGui && sGui->getIContextNetwork()->isConnected();
            const bool vatsimLogin = (ui->tw_Network->currentWidget() == ui->pg_NetworkVatsim);
            CServer currentServer; // used for login
            CSimulatedAircraft ownAircraft; // used own aircraft
            CStatusMessage msg;
            if (!isConnected)
            {
                if (!this->ps_validateAircraftValues())
                {
                    CLogMessage(this).validationWarning("Invalid aircraft data, login not possible");
                    return;
                }

                if (vatsimLogin && !this->ps_validateVatsimValues())
                {
                    CLogMessage(this).validationWarning("Invalid VATSIM data, login not possible");
                    return;
                }

                // sync values with GUI values
                this->updateOwnAircraftCallsignAndPilotFromGuiValues();
                this->updateOwnAircaftIcaoValuesFromGuiValues();

                // Login mode
                INetwork::LoginMode mode = ui->gbp_LoginMode->getLoginMode();
                switch (mode)
                {
                case INetwork::LoginStealth:
                    CLogMessage(this).info("login in stealth mode");
                    break;
                case INetwork::LoginAsObserver:
                    CLogMessage(this).info("login in observer mode");
                    break;
                case INetwork::LoginNormal:
                default:
                    break;
                }

                // Server
                if (vatsimLogin)
                {
                    currentServer = this->getCurrentVatsimServer();
                    const CUser vatsimUser = this->getUserFromVatsimGuiValues();
                    currentServer.setUser(vatsimUser);
                }
                else
                {
                    currentServer = this->getCurrentOtherServer();
                }

                // FSD setup, then override
                if (ui->comp_FsdDetails->isFsdSetuoEnabled())
                {
                    const CFsdSetup fsd = ui->comp_FsdDetails->getValue();
                    currentServer.setFsdSetup(fsd);
                }

                ui->frp_CurrentServer->setServer(currentServer);
                sGui->getIContextOwnAircraft()->updateOwnAircraftPilot(currentServer.getUser());

                // Login
                msg = sGui->getIContextNetwork()->connectToNetwork(currentServer, mode);
                if (msg.isSuccess() && vatsimLogin)
                {
                    Q_ASSERT_X(currentServer.isValidForLogin(), Q_FUNC_INFO, "invalid server");
                    this->m_currentVatsimServer.set(currentServer);
                    this->m_currentAircraftModel.setAndSave(ownAircraft.getModel());
                }
            }
            else
            {
                // disconnect from network
                sGui->getIContextAudio()->leaveAllVoiceRooms();
                msg = sGui->getIContextNetwork()->disconnectFromNetwork();
            }

            // log message and trigger events
            msg.addCategories(this);
            CLogMessage::preformatted(msg);
            if (msg.isSuccess())
            {
                QString ac(ownAircraft.getCallsignAsString() + " " + ownAircraft.getAircraftIcaoCodeDesignator());
                if (ownAircraft.hasAirlineDesignator()) { ac += " "; ac += ownAircraft.getAirlineIcaoCodeDesignator(); }
                if (!ownAircraft.getAircraftIcaoCombinedType().isEmpty()) { ac += " "; ac += ownAircraft.getAircraftIcaoCode().getCombinedType(); }
                ui->le_LoginSince->setText(QDateTime::currentDateTimeUtc().toString());
                ui->le_LoginAsAircaft->setText(ac);
                emit this->loginOrLogoffSuccessful();
            }
            else
            {
                emit this->loginOrLogoffCancelled();
            }
        }

        void CLoginComponent::ps_onWebServiceDataRead(int entityInt, int stateInt, int number)
        {
            const CEntityFlags::EntityFlag entity = static_cast<CEntityFlags::EntityFlag>(entityInt);
            const CEntityFlags::ReadState state = static_cast<CEntityFlags::ReadState>(stateInt);
            if (state != CEntityFlags::ReadFinished) { return; }
            Q_UNUSED(number);

            if (entity == CEntityFlags::VatsimDataFile)
            {
                CServerList vatsimFsdServers = sGui->getIContextNetwork()->getVatsimFsdServers();
                if (vatsimFsdServers.isEmpty()) { return; }
                vatsimFsdServers.sortBy(&CServer::getName);
                const CServer currentServer = this->m_currentVatsimServer.get();
                ui->comp_VatsimServer->setServers(vatsimFsdServers, true);
                ui->comp_VatsimServer->preSelect(currentServer.getName());
            }
            else
            {
                this->initCompleters(entity);
            }
        }

        void CLoginComponent::loadRememberedVatsimData()
        {
            const CServer lastServer = this->m_currentVatsimServer.get();
            const CUser lastUser = lastServer.getUser();
            if (lastUser.hasValidCallsign())
            {
                ui->le_Callsign->setText(lastUser.getCallsign().asString());
                ui->le_VatsimId->setText(lastUser.getId());
                ui->le_VatsimPassword->setText(lastUser.getPassword());
                ui->le_VatsimHomeAirport->setText(lastUser.getHomeBase().asString());
                ui->le_VatsimRealName->setText(lastUser.getRealName());
            }
            else
            {
                ui->le_Callsign->setText("SWIFT");
                ui->le_VatsimId->setText("1288459");
                ui->le_VatsimPassword->setText("4769");
                ui->le_VatsimHomeAirport->setText("LOWI");
                ui->le_VatsimRealName->setText("Black Swift");
            }
            ui->comp_OtherServers->preSelect(lastServer.getName());
        }

        CLoginComponent::CGuiAircraftValues CLoginComponent::getAircraftValuesFromGui() const
        {
            CGuiAircraftValues values;
            values.ownCallsign = CCallsign(ui->le_Callsign->text().trimmed().toUpper());
            values.ownAircraftIcao = ui->selector_AircraftIcao->getAircraftIcao();
            values.ownAirlineIcao = ui->selector_AirlineIcao->getAirlineIcao();
            values.ownAircraftCombinedType = ui->le_AircraftCombinedType->text().trimmed().toUpper();
            values.ownAircraftSimulatorModel = ui->le_SimulatorModel->text().trimmed().toUpper();
            return values;
        }

        CLoginComponent::CVatsimValues CLoginComponent::getVatsimValuesFromGui() const
        {
            CVatsimValues values;
            values.vatsimHomeAirport = ui->le_VatsimHomeAirport->text().trimmed().toUpper();
            values.vatsimId = ui->le_VatsimId->text().trimmed();
            values.vatsimPassword = ui->le_VatsimPassword->text().trimmed();
            values.vatsimRealName = ui->le_VatsimRealName->text().simplified().trimmed();
            return values;
        }

        CUser CLoginComponent::getUserFromVatsimGuiValues() const
        {
            const CVatsimValues values = this->getVatsimValuesFromGui();
            CUser user(values.vatsimId, values.vatsimRealName, "", values.vatsimPassword, getCallsignFromGui());
            user.setHomeBase(values.vatsimHomeAirport);
            return user;
        }

        CCallsign CLoginComponent::getCallsignFromGui() const
        {
            const CCallsign cs(ui->le_Callsign->text().trimmed().toUpper());
            return cs;
        }

        CServer CLoginComponent::getCurrentVatsimServer() const
        {
            return ui->comp_VatsimServer->currentServer();
        }

        CServer CLoginComponent::getCurrentOtherServer() const
        {
            return ui->comp_OtherServers->currentServer();
        }

        void CLoginComponent::setOkButtonString(bool connected)
        {
            const QString s = connected ? "Disconnect" : "Connect";
            ui->bb_OkCancel->button(QDialogButtonBox::Ok)->setText(s);
        }

        void CLoginComponent::setGuiVisibility(bool connected)
        {
            ui->gbp_LoginMode->setVisible(!connected);
            ui->gb_OwnAircraft->setVisible(!connected);
            ui->gb_Network->setVisible(!connected);
            ui->fr_LogoffConfirmation->setVisible(connected);
        }

        void CLoginComponent::startLogoffTimerCountdown()
        {
            ui->pb_LogoffTimeout->setValue(LogoffIntervalSeconds);
            this->m_logoffCountdownTimer->setInterval(1000);
            this->m_logoffCountdownTimer->start();
        }

        void CLoginComponent::setOwnModelAndIcaoValues()
        {
            Q_ASSERT(sGui->getIContextOwnAircraft());
            Q_ASSERT(sGui->getIContextSimulator());

            CAircraftModel model;
            const bool simulating = sGui->getIContextSimulator() &&
                                    (sGui->getIContextSimulator()->getSimulatorStatus() & ISimulator::Simulating);
            if (simulating)
            {
                model = sGui->getIContextOwnAircraft()->getOwnAircraft().getModel();
                ui->le_SimulatorModel->setText(model.getModelStringAndDbKey());
                this->highlightModelField(model);
            }
            else
            {
                model = this->getPrefillModel();
                ui->le_SimulatorModel->setText("");
                this->highlightModelField();
            }
            ui->le_SimulatorModel->setToolTip(model.asHtmlSummary());

            // reset the model
            if (model.isLoadedFromDb())
            {
                // full model from DB, take all values
                this->setGuiIcaoValues(model, false);
            }
            else
            {
                if (model.hasAircraftDesignator())
                {
                    // typed in model, override unempty values only
                    this->setGuiIcaoValues(model, true);
                }
            }

            const bool changedOwnAircraftCallsignPilot = this->updateOwnAircraftCallsignAndPilotFromGuiValues();
            const bool changedOwnAircraftIcaoValues = this->updateOwnAircaftIcaoValuesFromGuiValues();
            if (changedOwnAircraftIcaoValues || changedOwnAircraftCallsignPilot)
            {
                this->m_changedLoginDataDigestSignal.inputSignal();
            }
        }

        bool CLoginComponent::setGuiIcaoValues(const CAircraftModel &model, bool onlyIfEmpty)
        {
            bool changed = false;
            if (!onlyIfEmpty || !ui->selector_AircraftIcao->isSet())
            {
                changed = ui->selector_AircraftIcao->setAircraftIcao(model.getAircraftIcaoCode());
            }
            if (!onlyIfEmpty || !ui->selector_AirlineIcao->isSet())
            {
                const bool c = ui->selector_AirlineIcao->setAirlineIcao(model.getAirlineIcaoCode());
                changed |= c;
            }
            if (!onlyIfEmpty || ui->le_AircraftCombinedType->text().trimmed().isEmpty())
            {
                const QString combined(model.getAircraftIcaoCode().getCombinedType());
                if (ui->le_AircraftCombinedType->text() != combined)
                {
                    ui->le_AircraftCombinedType->setText(combined);
                    changed = true;
                }
            }
            const bool valid = this->ps_validateAircraftValues();
            return valid ? changed : false;
        }

        bool CLoginComponent::ps_validateAircraftValues()
        {
            const CGuiAircraftValues values = getAircraftValuesFromGui();

            const bool validCombinedType = CAircraftIcaoCode::isValidCombinedType(values.ownAircraftCombinedType);
            ui->lblp_AircraftCombinedType->setTicked(validCombinedType);

            // airline is optional, e.g. C172 has no airline
            const bool validAirlineDesignator = values.ownAirlineIcao.hasValidDesignator() || values.ownAirlineIcao.getDesignator().isEmpty();
            ui->lblp_AirlineIcao->setTicked(validAirlineDesignator);

            const bool validAircraftDesignator = values.ownAircraftIcao.hasValidDesignator();
            ui->lblp_AircraftIcao->setTicked(validAircraftDesignator);

            const bool validCallsign = CCallsign::isValidAircraftCallsign(values.ownCallsign);
            ui->lblp_Callsign->setTicked(validCallsign);

            // model intentionally ignored
            return validCombinedType && validAirlineDesignator && validAircraftDesignator && validCallsign;
        }

        bool CLoginComponent::ps_validateVatsimValues()
        {
            CVatsimValues values = getVatsimValuesFromGui();

            const bool validVatsimId = CUser::isValidVatsimId(values.vatsimId);
            ui->lblp_VatsimId->setTicked(validVatsimId);

            const bool validHomeAirport = values.vatsimHomeAirport.isEmpty() || CAirportIcaoCode::isValidIcaoDesignator(values.vatsimHomeAirport);
            ui->lblp_VatsimHomeAirport->setTicked(validHomeAirport);

            const bool validVatsimPassword = !values.vatsimPassword.isEmpty();
            ui->lblp_VatsimPassword->setTicked(validVatsimPassword);

            const bool validRealUserName = !values.vatsimRealName.isEmpty();
            ui->lblp_VatsimRealName->setTicked(validRealUserName);

            return validVatsimId && validHomeAirport && validVatsimPassword && validRealUserName;
        }

        void CLoginComponent::ps_changedAircraftIcao(const CAircraftIcaoCode &icao)
        {
            if (icao.isLoadedFromDb())
            {
                ui->le_AircraftCombinedType->setText(icao.getCombinedType());
            }
            this->ps_validateAircraftValues();
        }

        void CLoginComponent::ps_changedAirlineIcao(const CAirlineIcaoCode &icao)
        {
            Q_UNUSED(icao);
            this->ps_validateAircraftValues();
        }

        void CLoginComponent::ps_reloadSettings()
        {
            CServerList otherServers(this->m_otherTrafficNetworkServers.getThreadLocal());
            ui->comp_OtherServers->setServers(otherServers);
        }

        void CLoginComponent::ps_logoffCountdown()
        {
            int v = ui->pb_LogoffTimeout->value();
            v -= 1;
            if (v < 0) { v = 0; }
            ui->pb_LogoffTimeout->setValue(v);
            if (v <= 0)
            {
                this->m_logoffCountdownTimer->stop();
                this->ps_toggleNetworkConnection();
            }
        }

        void CLoginComponent::ps_reverseLookupAircraftModel()
        {
            if (!sGui->getIContextSimulator()->isSimulatorAvailable()) { return; }
            const CAircraftModel model(sGui->getIContextOwnAircraft()->getOwnAircraft().getModel());
            this->ps_simulatorModelChanged(model);
        }

        void CLoginComponent::ps_simulatorModelChanged(const CAircraftModel &model)
        {
            Q_ASSERT_X(sGui && sGui->getIContextNetwork(), Q_FUNC_INFO, "Missing context");
            const bool isNetworkConnected = sGui && sGui->getIContextNetwork()->isConnected();
            if (isNetworkConnected) { return; }
            const QString modelStr(model.hasModelString() ? model.getModelString() : "<unknown>");
            if (!model.hasModelString())
            {
                CLogMessage(this).validationInfo("Invalid lookup for '%1' successful: %2") << modelStr << model.toQString();
                return;
            }
            this->setOwnModelAndIcaoValues();

            // open dialog for model mapping
            if (this->m_autoPopupWizard && !model.isLoadedFromDb())
            {
                this->ps_mappingWizard();
            }

            // check state of own aircraft
            this->updateOwnAircraftCallsignAndPilotFromGuiValues();

            // let others know data changed
            this->m_changedLoginDataDigestSignal.inputSignal();
        }

        void CLoginComponent::ps_mappingWizard()
        {
            if (!this->m_mappingWizard)
            {
                this->m_mappingWizard.reset(new CDbQuickMappingWizard(this));
            }

            if (sGui->getIContextSimulator()->isSimulatorAvailable())
            {
                // preset on model
                const CAircraftModel model(sGui->getIContextOwnAircraft()->getOwnAircraft().getModel());
                this->m_mappingWizard->presetModel(model);
            }
            else
            {
                // preset on GUI values only
                const CAircraftIcaoCode icao(ui->selector_AircraftIcao->getAircraftIcao());
                this->m_mappingWizard->presetAircraftIcao(icao);
            }
            this->m_mappingWizard->show();
        }

        void CLoginComponent::initCompleters(CEntityFlags::Entity entity)
        {
            // completers where possible
            if (sGui && sGui->hasWebDataServices())
            {
                if (entity.testFlag(CEntityFlags::AirportEntity) && !ui->le_VatsimHomeAirport->completer())
                {
                    // one time init
                    const QStringList airports = sGui->getWebDataServices()->getAirports().allIcaoCodes(true);
                    if (!airports.isEmpty())
                    {
                        QCompleter *completer = new QCompleter(airports, this);
                        QStyledItemDelegate *itemDelegate = new QStyledItemDelegate(completer);
                        completer->popup()->setItemDelegate(itemDelegate);
                        ui->le_VatsimHomeAirport->setCompleter(completer);
                        completer->popup()->setObjectName("AirportCompleter");
                        completer->popup()->setMinimumWidth(175);
                    }
                }
            }
        }

        void CLoginComponent::highlightModelField(const CAircraftModel &model)
        {
            static const QString error("rgba(255, 0, 0, 40%)");
            static const QString warning("rgba(255, 255, 0, 40%)");
            static const QString ok("rgba(0, 255, 0, 40%)");
            QString color(ok);
            if (!model.hasModelString())
            {
                color = error;
            }
            else
            {
                if (!model.isLoadedFromDb())
                {
                    color = warning;
                }
            }
            static const QString sheet("background-color: %1;");
            ui->le_SimulatorModel->setStyleSheet(sheet.arg(color));
        }

        CAircraftModel CLoginComponent::getPrefillModel() const
        {
            CAircraftModel model = this->m_currentAircraftModel.get();
            if (model.hasAircraftDesignator()) { return model; }
            return IContextOwnAircraft::getDefaultOwnAircraftModel();
        }

        bool CLoginComponent::updateOwnAircraftCallsignAndPilotFromGuiValues()
        {
            if (!sGui || !sGui->getIContextOwnAircraft()) { return false; }
            CSimulatedAircraft ownAircraft(sGui->getIContextOwnAircraft()->getOwnAircraft());
            const QString cs(ui->le_Callsign->text().trimmed().toUpper());
            bool changedCallsign = false;
            if (!cs.isEmpty() && ownAircraft.getCallsignAsString() != cs)
            {
                const CCallsign callsign(cs);
                sGui->getIContextOwnAircraft()->updateOwnCallsign(callsign);
                ownAircraft.setCallsign(callsign); // also update
                changedCallsign = true;
            }
            CUser pilot = ownAircraft.getPilot();
            pilot.setRealName(CUser::beautifyRealName(ui->le_VatsimRealName->text()));
            pilot.setHomeBase(CAirportIcaoCode(ui->le_VatsimHomeAirport->text()));
            pilot.setId(ui->le_VatsimId->text());
            pilot.setCallsign(CCallsign(cs));
            bool changedPilot = false;
            if (ownAircraft.getPilot() != pilot)
            {
                // it can be that the callsign was changed and this results in unchanged here
                changedPilot = sGui->getIContextOwnAircraft()->updateOwnAircraftPilot(pilot);
            }
            return changedCallsign || changedPilot;
        }

        bool CLoginComponent::updateOwnAircaftIcaoValuesFromGuiValues()
        {
            if (!sGui || !sGui->getIContextOwnAircraft()) { return false; }
            const CSimulatedAircraft ownAircraft(sGui->getIContextOwnAircraft()->getOwnAircraft());
            const CGuiAircraftValues aircraftValues = this->getAircraftValuesFromGui();

            CAircraftIcaoCode aircraftCode(ownAircraft.getAircraftIcaoCode());
            CAirlineIcaoCode airlineCode(ownAircraft.getAirlineIcaoCode());

            bool changedIcaoCodes = false;
            if (aircraftValues.ownAircraftIcao.hasValidDesignator() && aircraftValues.ownAircraftIcao != aircraftCode)
            {
                aircraftCode = aircraftValues.ownAircraftIcao;
                changedIcaoCodes = true;
            }
            if (aircraftValues.ownAirlineIcao.hasValidDesignator() && aircraftValues.ownAirlineIcao != airlineCode)
            {
                airlineCode = aircraftValues.ownAirlineIcao;
                changedIcaoCodes = true;
            }

            if (changedIcaoCodes)
            {
                sGui->getIContextOwnAircraft()->updateOwnIcaoCodes(aircraftCode, airlineCode);
            }

            return changedIcaoCodes;
        }
    } // namespace
} // namespace

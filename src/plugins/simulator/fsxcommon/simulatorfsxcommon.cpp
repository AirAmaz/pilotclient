/* Copyright (C) 2013
 * swift Project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "simulatorfsxcommon.h"
#include "blackcore/application.h"
#include "blackmisc/network/textmessage.h"
#include "blackmisc/simulation/fsx/simconnectutilities.h"
#include "blackmisc/simulation/fscommon/bcdconversions.h"
#include "blackmisc/simulation/fscommon/fscommonutil.h"
#include "blackmisc/simulation/aircraftmodel.h"
#include "blackmisc/simulation/interpolatormulti.h"
#include "blackmisc/simulation/interpolationhints.h"
#include "blackmisc/simulation/simulatorplugininfo.h"
#include "blackmisc/aviation/airportlist.h"
#include "blackmisc/geo/elevationplane.h"
#include "blackmisc/logmessage.h"
#include "blackmisc/threadutils.h"
#include "blackmisc/verify.h"

#include <QTimer>
#include <type_traits>

using namespace BlackMisc;
using namespace BlackMisc::Aviation;
using namespace BlackMisc::PhysicalQuantities;
using namespace BlackMisc::Geo;
using namespace BlackMisc::Network;
using namespace BlackMisc::Simulation;
using namespace BlackMisc::Simulation::FsCommon;
using namespace BlackMisc::Simulation::Fsx;
using namespace BlackMisc::Weather;
using namespace BlackCore;

namespace BlackSimPlugin
{
    namespace FsxCommon
    {
        CSimulatorFsxCommon::CSimulatorFsxCommon(const CSimulatorPluginInfo &info,
                IOwnAircraftProvider *ownAircraftProvider,
                IRemoteAircraftProvider *remoteAircraftProvider,
                IWeatherGridProvider *weatherGridProvider,
                QObject *parent) :
            CSimulatorFsCommon(info, ownAircraftProvider, remoteAircraftProvider, weatherGridProvider, parent)
        {
            Q_ASSERT_X(ownAircraftProvider, Q_FUNC_INFO, "Missing provider");
            Q_ASSERT_X(remoteAircraftProvider, Q_FUNC_INFO, "Missing provider");
            Q_ASSERT_X(sApp, Q_FUNC_INFO, "Missing global object");
            m_realityBubbleTimer.setInterval(20 * 1000);
            connect(&m_realityBubbleTimer, &QTimer::timeout, this, &CSimulatorFsxCommon::ps_addAircraftCurrentlyOutOfBubble);

            m_useFsuipc = false;
            m_defaultModel =
            {
                "Boeing 737-800 Paint1",
                CAircraftModel::TypeModelMatchingDefaultModel,
                "B737-800 default model",
                CAircraftIcaoCode("B738", "L2J")
            };
        }

        CSimulatorFsxCommon::~CSimulatorFsxCommon()
        {
            disconnectFrom();
            // fsuipc is disconnected in CSimulatorFsCommon
        }

        bool CSimulatorFsxCommon::isConnected() const
        {
            return m_simConnected;
        }

        bool CSimulatorFsxCommon::isSimulating() const
        {
            return m_simSimulating;
        }

        bool CSimulatorFsxCommon::connectTo()
        {
            if (this->isConnected()) { return true; }
            this->reset();
            if (FAILED(SimConnect_Open(&m_hSimConnect, sApp->swiftVersionChar(), nullptr, 0, 0, 0)))
            {
                // reset state as expected for unconnected
                return false;
            }
            if (m_useFsuipc) { this->m_fsuipc->connect(); } // FSUIPC too

            // set structures and move on
            initEvents();
            initDataDefinitionsWhenConnected();
            m_simConnectTimerId = startTimer(10);
            m_realityBubbleTimer.start();
            return true;
        }

        bool CSimulatorFsxCommon::disconnectFrom()
        {
            if (!m_simConnected) { return true; }
            if (m_simConnectTimerId >= 0) { killTimer(m_simConnectTimerId); }
            m_simConnectTimerId = -1;
            if (m_hSimConnect)
            {
                SimConnect_Close(m_hSimConnect);
                m_hSimConnect = nullptr;
            }

            reset();

            // emit status and disconnect FSUIPC
            CSimulatorFsCommon::disconnectFrom();
            return true;
        }

        bool CSimulatorFsxCommon::physicallyAddRemoteAircraft(const CSimulatedAircraft &newRemoteAircraft)
        {
            const CCallsign callsign(newRemoteAircraft.getCallsign());

            Q_ASSERT_X(CThreadUtils::isCurrentThreadObjectThread(this),  Q_FUNC_INFO, "thread");
            Q_ASSERT_X(!callsign.isEmpty(), Q_FUNC_INFO, "empty callsign");
            Q_ASSERT_X(newRemoteAircraft.hasModelString(), Q_FUNC_INFO, "missing model string");
            if (callsign.isEmpty()) { return false; }

            // check if we have to do something
            m_outOfRealityBubble.removeByCallsign(callsign);
            if (m_simConnectObjects.contains(callsign))
            {
                const CSimConnectObject simObj = m_simConnectObjects[callsign];
                if (simObj.isPendingAdded())
                {
                    return true; // already pending
                }
                else
                {
                    // same model, nothing will change, otherwise add again when removed
                    if (simObj.getAircraft().getModel() != newRemoteAircraft.getModel())
                    {
                        m_aircraftToAddAgainWhenRemoved.push_back(newRemoteAircraft);
                    }
                    return false;
                }
            }

            // create AI
            bool adding = false;
            const CAircraftModel aircraftModel = newRemoteAircraft.getModel();
            CSimulatedAircraft addedAircraft(newRemoteAircraft);
            if (isConnected())
            {
                const DWORD requestId = obtainRequestIdSimData();
                SIMCONNECT_DATA_INITPOSITION initialPosition = aircraftSituationToFsxPosition(addedAircraft.getSituation());
                const QString modelString(addedAircraft.getModelString());

                if (m_interpolationRenderingSetup.showSimulatorDebugMessages())
                {
                    CLogMessage(this).debug() << "physicallyAddRemoteAircraft" << callsign.toQString() << "request" << requestId << "model" << modelString;
                    CLogMessage(this).debug() << "initial position" << fsxPositionToString(initialPosition);
                }

                HRESULT hr = SimConnect_AICreateNonATCAircraft(m_hSimConnect, qPrintable(modelString), qPrintable(callsign.toQString().left(12)), initialPosition, static_cast<SIMCONNECT_DATA_REQUEST_ID>(requestId));
                if (hr != S_OK)
                {
                    const CStatusMessage msg = CStatusMessage(this).error("SimConnect, can not create AI traffic: '%1' '%2'") << callsign.toQString() << aircraftModel.getModelString();
                    CLogMessage::preformatted(msg);
                    emit physicallyAddingRemoteModelFailed(addedAircraft, msg);
                }
                else
                {
                    // we will request a new aircraft by request ID, later we will receive its object id
                    // so far this object id is -1
                    addedAircraft.setRendered(false);
                    CSimConnectObject simObject(addedAircraft, requestId, &m_interpolationLogger);
                    if (addedAircraft.isPartsSynchronized()) { simObject.addAircraftParts(addedAircraft.getParts()); }
                    m_simConnectObjects.insert(callsign, simObject);
                    adding = true;
                }
            }
            else
            {
                CLogMessage(this).warning("FSX: Not connected, not added aircraft '%1' '%2'") << callsign.toQString() << aircraftModel.getModelString();
            }
            return adding;
        }

        bool CSimulatorFsxCommon::updateOwnSimulatorCockpit(const CSimulatedAircraft &ownAircraft, const CIdentifier &originator)
        {
            if (originator == this->identifier()) { return false; }
            if (!this->isSimulating()) { return false; }

            // actually those data should be the same as ownAircraft
            const CComSystem newCom1 = ownAircraft.getCom1System();
            const CComSystem newCom2 = ownAircraft.getCom2System();
            const CTransponder newTransponder = ownAircraft.getTransponder();

            bool changed = false;
            if (newCom1.getFrequencyActive() != this->m_simCom1.getFrequencyActive())
            {
                const CFrequency newFreq = newCom1.getFrequencyActive();
                SimConnect_TransmitClientEvent(m_hSimConnect, 0, EventSetCom1Active,
                                               CBcdConversions::comFrequencyToBcdHz(newFreq), SIMCONNECT_GROUP_PRIORITY_HIGHEST, SIMCONNECT_EVENT_FLAG_GROUPID_IS_PRIORITY);
                changed = true;

            }
            if (newCom1.getFrequencyStandby() != this->m_simCom1.getFrequencyStandby())
            {
                const CFrequency newFreq = newCom1.getFrequencyStandby();
                SimConnect_TransmitClientEvent(m_hSimConnect, 0, EventSetCom1Standby,
                                               CBcdConversions::comFrequencyToBcdHz(newFreq), SIMCONNECT_GROUP_PRIORITY_HIGHEST, SIMCONNECT_EVENT_FLAG_GROUPID_IS_PRIORITY);
                changed = true;
            }

            if (newCom2.getFrequencyActive() != this->m_simCom2.getFrequencyActive())
            {
                const CFrequency newFreq = newCom2.getFrequencyActive();
                SimConnect_TransmitClientEvent(m_hSimConnect, 0, EventSetCom2Active,
                                               CBcdConversions::comFrequencyToBcdHz(newFreq), SIMCONNECT_GROUP_PRIORITY_HIGHEST, SIMCONNECT_EVENT_FLAG_GROUPID_IS_PRIORITY);
                changed = true;
            }
            if (newCom2.getFrequencyStandby() != this->m_simCom2.getFrequencyStandby())
            {
                const CFrequency newFreq = newCom2.getFrequencyStandby();
                SimConnect_TransmitClientEvent(m_hSimConnect, 0, EventSetCom2Standby,
                                               CBcdConversions::comFrequencyToBcdHz(newFreq), SIMCONNECT_GROUP_PRIORITY_HIGHEST, SIMCONNECT_EVENT_FLAG_GROUPID_IS_PRIORITY);
                changed = true;
            }

            if (newTransponder.getTransponderCode() != this->m_simTransponder.getTransponderCode())
            {
                SimConnect_TransmitClientEvent(m_hSimConnect, 0, EventSetTransponderCode,
                                               CBcdConversions::transponderCodeToBcd(newTransponder), SIMCONNECT_GROUP_PRIORITY_HIGHEST, SIMCONNECT_EVENT_FLAG_GROUPID_IS_PRIORITY);
                changed = true;
            }

            if (newTransponder.getTransponderMode() != this->m_simTransponder.getTransponderMode())
            {
                if (m_useSbOffsets)
                {
                    byte ident = newTransponder.isIdentifying() ? 1U : 0U; // 1 is ident
                    byte standby = newTransponder.isInStandby() ? 1U : 0U; // 1 is standby
                    HRESULT hr = S_OK;

                    hr += SimConnect_SetClientData(m_hSimConnect, ClientAreaSquawkBox, CSimConnectDefinitions::DataClientAreaSbIdent, SIMCONNECT_CLIENT_DATA_REQUEST_FLAG_DEFAULT, 0, 1, &ident);
                    hr += SimConnect_SetClientData(m_hSimConnect, ClientAreaSquawkBox, CSimConnectDefinitions::DataClientAreaSbStandby, SIMCONNECT_CLIENT_DATA_REQUEST_FLAG_DEFAULT, 0, 1, &standby);
                    if (hr != S_OK)
                    {
                        CLogMessage(this).warning("Setting transponder mode failed (SB offsets)");
                    }
                }
                changed = true;
            }

            // avoid changes of cockpit back to old values due to an outdated read back value
            if (changed) { m_skipCockpitUpdateCycles = SkipUpdateCyclesForCockpit; }

            // bye
            return changed;
        }

        void CSimulatorFsxCommon::displayStatusMessage(const BlackMisc::CStatusMessage &message) const
        {
            QByteArray m = message.getMessage().toLocal8Bit().constData();
            m.append('\0');

            SIMCONNECT_TEXT_TYPE type = SIMCONNECT_TEXT_TYPE_PRINT_BLACK;
            switch (message.getSeverity())
            {
            case CStatusMessage::SeverityDebug: return;
            case CStatusMessage::SeverityInfo: type = SIMCONNECT_TEXT_TYPE_PRINT_GREEN; break;
            case CStatusMessage::SeverityWarning: type = SIMCONNECT_TEXT_TYPE_PRINT_YELLOW; break;
            case CStatusMessage::SeverityError: type = SIMCONNECT_TEXT_TYPE_PRINT_RED; break;
            }
            HRESULT hr = SimConnect_Text(m_hSimConnect, type, 7.5, EventTextMessage,
                                         static_cast<DWORD>(m.size()), m.data());
            Q_UNUSED(hr);
        }

        void CSimulatorFsxCommon::displayTextMessage(const BlackMisc::Network::CTextMessage &message) const
        {
            this->displayStatusMessage(message.asStatusMessage(true, true));
        }

        bool CSimulatorFsxCommon::isPhysicallyRenderedAircraft(const CCallsign &callsign) const
        {
            return this->m_simConnectObjects.contains(callsign);
        }

        CCallsignSet CSimulatorFsxCommon::physicallyRenderedAircraft() const
        {
            CCallsignSet callsigns(this->m_simConnectObjects.keys());
            callsigns.push_back(m_aircraftToAddAgainWhenRemoved.getCallsigns()); // not really rendered right now, but very soon
            callsigns.push_back(m_outOfRealityBubble.getCallsigns()); // not really rendered, but for the logic it should look like it is
            return CCallsignSet(this->m_simConnectObjects.keys());
        }

        bool CSimulatorFsxCommon::setInterpolatorMode(CInterpolatorMulti::Mode mode, const CCallsign &callsign)
        {
            if (mode == CInterpolatorMulti::ModeUnkown) { return false; }
            if (callsign.isEmpty())
            {
                const int c = m_simConnectObjects.setInterpolatorModes(mode);
                return c > 0;
            }
            else
            {
                if (!m_simConnectObjects.contains(callsign)) { return false; }
                return m_simConnectObjects[callsign].setInterpolatorMode(mode);
            }
        }

        bool CSimulatorFsxCommon::stillDisplayReceiveExceptions()
        {
            m_receiveExceptionCount++;
            return m_receiveExceptionCount < IgnoreReceiveExceptions;
        }

        void CSimulatorFsxCommon::setSimConnected()
        {
            m_simConnected = true;
            this->initSimulatorInternals();
            emitSimulatorCombinedStatus();

            // Internals depends on sim data which take a while to be read
            // this is a trich and I re-init again after a while (which is not really expensive)
            QTimer::singleShot(1000, this, [this] { this->initSimulatorInternals(); });
        }

        void CSimulatorFsxCommon::onSimRunning()
        {
            if (m_simSimulating) { return; }
            m_simSimulating = true; // only place where this should be set to true
            m_simConnected = true;
            HRESULT hr = SimConnect_RequestDataOnSimObject(m_hSimConnect, CSimConnectDefinitions::RequestOwnAircraft,
                         CSimConnectDefinitions::DataOwnAircraft,
                         SIMCONNECT_OBJECT_ID_USER, SIMCONNECT_PERIOD_VISUAL_FRAME);

            hr += SimConnect_RequestDataOnSimObject(m_hSimConnect, CSimConnectDefinitions::RequestOwnAircraftTitle,
                                                    CSimConnectDefinitions::DataOwnAircraftTitle,
                                                    SIMCONNECT_OBJECT_ID_USER, SIMCONNECT_PERIOD_SECOND,
                                                    SIMCONNECT_DATA_REQUEST_FLAG_CHANGED);

            hr += SimConnect_RequestDataOnSimObject(m_hSimConnect, CSimConnectDefinitions::RequestSimEnvironment,
                                                    CSimConnectDefinitions::DataSimEnvironment,
                                                    SIMCONNECT_OBJECT_ID_USER, SIMCONNECT_PERIOD_SECOND,
                                                    SIMCONNECT_DATA_REQUEST_FLAG_CHANGED);

            if (hr != S_OK)
            {
                CLogMessage(this).error("FSX plugin: SimConnect_RequestDataOnSimObject failed");
                return;
            }

            // Request the data from SB only when its changed and only ONCE so we don't have to run a 1sec event to get/set this info ;)
            hr += SimConnect_RequestClientData(m_hSimConnect, ClientAreaSquawkBox, CSimConnectDefinitions::RequestSbData,
                                               CSimConnectDefinitions::DataClientAreaSb, SIMCONNECT_CLIENT_DATA_PERIOD_SECOND, SIMCONNECT_CLIENT_DATA_REQUEST_FLAG_CHANGED);

            if (hr != S_OK)
            {
                CLogMessage(this).error("FSX plugin: SimConnect_RequestClientData failed");
                return;
            }

            emitSimulatorCombinedStatus();
        }

        void CSimulatorFsxCommon::onSimStopped()
        {
            const int oldStatus = getSimulatorStatus();
            m_simSimulating = false;
            emitSimulatorCombinedStatus(oldStatus);
        }

        void CSimulatorFsxCommon::onSimFrame()
        {
            updateRemoteAircraft();
        }

        void CSimulatorFsxCommon::onSimExit()
        {
            // reset complete state, we are going down
            disconnectFrom();
        }

        DWORD CSimulatorFsxCommon::obtainRequestIdSimData()
        {
            const DWORD id = m_requestIdSimData++;
            if (id > RequestSimDataEnd) { m_requestIdSimData = RequestSimDataStart; }
            return id;
        }

        void CSimulatorFsxCommon::updateOwnAircraftFromSimulator(const DataDefinitionOwnAircraft &simulatorOwnAircraft)
        {
            CSimulatedAircraft myAircraft(getOwnAircraft());
            BlackMisc::Geo::CCoordinateGeodetic position;
            position.setLatitude(CLatitude(simulatorOwnAircraft.latitude, CAngleUnit::deg()));
            position.setLongitude(CLongitude(simulatorOwnAircraft.longitude, CAngleUnit::deg()));

            if (simulatorOwnAircraft.pitch < -90.0 || simulatorOwnAircraft.pitch >= 90.0)
            {
                CLogMessage(this).warning("FSX: Pitch value out of limits: %1") << simulatorOwnAircraft.pitch;
            }
            BlackMisc::Aviation::CAircraftSituation aircraftSituation;
            aircraftSituation.setPosition(position);
            // MSFS has inverted pitch and bank angles
            aircraftSituation.setPitch(CAngle(-simulatorOwnAircraft.pitch, CAngleUnit::deg()));
            aircraftSituation.setBank(CAngle(-simulatorOwnAircraft.bank, CAngleUnit::deg()));
            aircraftSituation.setHeading(CHeading(simulatorOwnAircraft.trueHeading, CHeading::True, CAngleUnit::deg()));
            aircraftSituation.setGroundSpeed(CSpeed(simulatorOwnAircraft.velocity, CSpeedUnit::kts()));
            aircraftSituation.setGroundElevation(CAltitude(simulatorOwnAircraft.elevation, CAltitude::MeanSeaLevel, CLengthUnit::ft()));
            aircraftSituation.setAltitude(CAltitude(simulatorOwnAircraft.altitude, CAltitude::MeanSeaLevel, CLengthUnit::ft()));

            const CAircraftLights lights(simulatorOwnAircraft.lightStrobe,
                                         simulatorOwnAircraft.lightLanding,
                                         simulatorOwnAircraft.lightTaxi,
                                         simulatorOwnAircraft.lightBeacon,
                                         simulatorOwnAircraft.lightNav,
                                         simulatorOwnAircraft.lightLogo);

            CAircraftEngineList engines;
            const QList<bool> helperList
            {
                simulatorOwnAircraft.engine1Combustion != 0, simulatorOwnAircraft.engine2Combustion != 0,
                simulatorOwnAircraft.engine3Combustion != 0, simulatorOwnAircraft.engine4Combustion != 0
            };

            for (int index = 0; index < simulatorOwnAircraft.numberOfEngines; ++index)
            {
                engines.push_back(CAircraftEngine(index + 1, helperList.at(index)));
            }

            const CAircraftParts parts(lights, simulatorOwnAircraft.gearHandlePosition,
                                       simulatorOwnAircraft.flapsHandlePosition * 100,
                                       simulatorOwnAircraft.spoilersHandlePosition,
                                       engines,
                                       simulatorOwnAircraft.simOnGround);

            // set values
            updateOwnSituation(aircraftSituation);
            updateOwnParts(parts);

            // When I change cockpit values in the sim (from GUI to simulator, not originating from simulator)
            // it takes a little while before these values are set in the simulator.
            // To avoid jitters, I wait some update cylces to stabilize the values
            if (m_skipCockpitUpdateCycles < 1)
            {
                // defaults
                CComSystem com1(myAircraft.getCom1System()); // set defaults
                CComSystem com2(myAircraft.getCom2System());
                CTransponder transponder(myAircraft.getTransponder());

                // updates
                com1.setFrequencyActive(CFrequency(simulatorOwnAircraft.com1ActiveMHz, CFrequencyUnit::MHz()));
                com1.setFrequencyStandby(CFrequency(simulatorOwnAircraft.com1StandbyMHz, CFrequencyUnit::MHz()));
                const bool changedCom1 = myAircraft.getCom1System() != com1;
                this->m_simCom1 = com1;

                com2.setFrequencyActive(CFrequency(simulatorOwnAircraft.com2ActiveMHz, CFrequencyUnit::MHz()));
                com2.setFrequencyStandby(CFrequency(simulatorOwnAircraft.com2StandbyMHz, CFrequencyUnit::MHz()));
                const bool changedCom2 = myAircraft.getCom2System() != com2;
                this->m_simCom2 = com2;

                transponder.setTransponderCode(simulatorOwnAircraft.transponderCode);
                const bool changedXpr = (myAircraft.getTransponderCode() != transponder.getTransponderCode());

                if (changedCom1 || changedCom2 || changedXpr)
                {
                    this->updateCockpit(com1, com2, transponder, identifier());
                }
            }
            else
            {
                --m_skipCockpitUpdateCycles;
            }

            if (m_isWeatherActivated)
            {
                const auto currentPosition = CCoordinateGeodetic { aircraftSituation.latitude(), aircraftSituation.longitude(), {0} };
                if (CWeatherScenario::isRealWeatherScenario(m_weatherScenarioSettings.get()) &&
                        calculateGreatCircleDistance(m_lastWeatherPosition, currentPosition).value(CLengthUnit::mi()) > 20)
                {
                    m_lastWeatherPosition = currentPosition;
                    const auto weatherGrid = CWeatherGrid { { "GLOB", currentPosition } };
                    requestWeatherGrid(weatherGrid, { this, &CSimulatorFsxCommon::injectWeatherGrid });
                }
            }
        }

        void CSimulatorFsxCommon::updateRemoteAircraftFromSimulator(const CSimConnectObject &simObject, const DataDefinitionRemoteAircraftSimData &remoteAircraftData)
        {
            // Near ground we use faster updates
            if (remoteAircraftData.aboveGround() <= 100.0)
            {
                // switch to fast updates
                if (simObject.getSimDataPeriod() != SIMCONNECT_PERIOD_VISUAL_FRAME)
                {
                    this->requestDataForSimObject(simObject, SIMCONNECT_PERIOD_VISUAL_FRAME);
                }
            }
            else
            {
                // switch to slow updates
                if (simObject.getSimDataPeriod() != SIMCONNECT_PERIOD_SECOND)
                {
                    this->requestDataForSimObject(simObject, SIMCONNECT_PERIOD_SECOND);
                }
            }

            CElevationPlane elevation(remoteAircraftData.latitude, remoteAircraftData.longitude, remoteAircraftData.elevation);
            elevation.setSinglePointRadius();

            // const QString debug(hints.debugInfo(elevation));
            CInterpolationHints &hints = m_hints[simObject.getCallsign()];
            hints.setElevationPlane(elevation); // update elevation
            hints.setCGAboveGround({ remoteAircraftData.cgToGround, CLengthUnit::ft() }); // normally never changing, but if user changes ModelMatching

            // set it in the remote aircraft provider
            this->updateAircraftGroundElevation(simObject.getCallsign(), elevation);
        }

        void CSimulatorFsxCommon::updateOwnAircraftFromSimulator(const DataDefinitionClientAreaSb &sbDataArea)
        {
            CTransponder::TransponderMode newMode;
            if (sbDataArea.isIdent())
            {
                newMode = CTransponder::StateIdent;
            }
            else
            {
                newMode = sbDataArea.isStandby() ? CTransponder::StateStandby : CTransponder::ModeC;
            }
            const CSimulatedAircraft myAircraft(this->getOwnAircraft());
            const bool changed = (myAircraft.getTransponderMode() != newMode);
            if (!changed) { return; }
            CTransponder xpdr = myAircraft.getTransponder();
            xpdr.setTransponderMode(newMode);
            this->updateCockpit(myAircraft.getCom1System(), myAircraft.getCom2System(), xpdr, this->identifier());
        }

        bool CSimulatorFsxCommon::simulatorReportedObjectAdded(DWORD objectID)
        {
            const CSimConnectObject simObject = this->m_simConnectObjects.getSimObjectForObjectId(objectID);
            const CCallsign callsign(simObject.getCallsign());
            if (!simObject.hasValidRequestAndObjectId() || callsign.isEmpty()) { return false; }

            // we know the object has been created. But it can happen it is directly removed afterwards
            QTimer::singleShot(500, this, [ = ] { this->ps_deferredSimulatorReportedObjectAdded(callsign); });
            return true;
        }

        bool CSimulatorFsxCommon::ps_deferredSimulatorReportedObjectAdded(const CCallsign &callsign)
        {
            if (callsign.isEmpty()) { return false; }
            if (!m_simConnectObjects.contains(callsign)) { return false; } // removed in mean time

            CSimConnectObject &simObject = m_simConnectObjects[callsign];
            if (!simObject.hasValidRequestAndObjectId() || simObject.isPendingRemoved()) { return false; }

            Q_ASSERT_X(simObject.isPendingAdded(), Q_FUNC_INFO, "already confirmed");
            simObject.setConfirmedAdded(true);
            const DWORD objectId = simObject.getObjectId();

            if (m_interpolationRenderingSetup.showSimulatorDebugMessages())
            {
                CLogMessage(this).debug() << "Adding AI" << callsign.toQString() << "confirmed" << "id" << objectId << "model" << simObject.getAircraftModelString();
            }

            // P3D also has SimConnect_AIReleaseControlEx;
            const DWORD requestId = obtainRequestIdSimData();
            HRESULT hr = SimConnect_AIReleaseControl(m_hSimConnect, objectId, static_cast<SIMCONNECT_DATA_REQUEST_ID>(requestId));
            if (hr == S_OK)
            {
                SimConnect_TransmitClientEvent(m_hSimConnect, objectId, EventFreezeLat, 1,
                                               SIMCONNECT_GROUP_PRIORITY_HIGHEST, SIMCONNECT_EVENT_FLAG_GROUPID_IS_PRIORITY);
                SimConnect_TransmitClientEvent(m_hSimConnect, objectId, EventFreezeAlt, 1,
                                               SIMCONNECT_GROUP_PRIORITY_HIGHEST, SIMCONNECT_EVENT_FLAG_GROUPID_IS_PRIORITY);
                SimConnect_TransmitClientEvent(m_hSimConnect, objectId, EventFreezeAtt, 1,
                                               SIMCONNECT_GROUP_PRIORITY_HIGHEST, SIMCONNECT_EVENT_FLAG_GROUPID_IS_PRIORITY);
            }
            else
            {
                CLogMessage(this).error("Adding AI %1 failed") << callsign.toQString();
                return false;
            }

            const bool updated = this->updateAircraftRendered(callsign, true);
            if (updated)
            {
                emit aircraftRenderingChanged(simObject.getAircraft());
            }
            return true;
        }

        void CSimulatorFsxCommon::ps_addAircraftCurrentlyOutOfBubble()
        {
            if (m_outOfRealityBubble.isEmpty()) { return; }
            const CCallsignSet aircraftCallsignsInRange(getAircraftInRangeCallsigns());
            CSimulatedAircraftList toBeAddedAircraft;
            CCallsignSet toBeRemovedCallsigns;
            for (const CSimulatedAircraft &aircraft : as_const(m_outOfRealityBubble))
            {
                Q_ASSERT_X(!aircraft.getCallsign().isEmpty(), Q_FUNC_INFO, "missing callsign");
                if (aircraftCallsignsInRange.contains(aircraft.getCallsign()))
                {
                    toBeAddedAircraft.push_back(aircraft);
                }
                else
                {
                    toBeRemovedCallsigns.push_back(aircraft.getCallsign());
                }
            }
            m_outOfRealityBubble.removeByCallsigns(toBeRemovedCallsigns);

            // add aircraft, but non blocking
            int t = 100;
            for (const CSimulatedAircraft &aircraft : as_const(toBeAddedAircraft))
            {
                QTimer::singleShot(t, this, [ = ]
                {
                    this->physicallyAddRemoteAircraft(aircraft);
                });
                t += 100;
            }
        }

        bool CSimulatorFsxCommon::simulatorReportedObjectRemoved(DWORD objectID)
        {
            const CSimConnectObject simObject = this->m_simConnectObjects.getSimObjectForObjectId(objectID);
            if (!simObject.hasValidRequestAndObjectId()) { return false; } // object id from somewhere else
            const CCallsign callsign(simObject.getCallsign());
            Q_ASSERT_X(!callsign.isEmpty(), Q_FUNC_INFO, "missing callsign");

            bool ok = false;
            if (simObject.isPendingRemoved())
            {
                // good case, object has been removed
                // we can remove the sim object
            }
            else
            {
                // object was removed, but removal was not requested by us
                // this means we are out of the reality bubble (or something else went wrong)
                if (!simObject.getAircraftModelString().isEmpty())
                {
                    this->m_outOfRealityBubble.push_back(simObject.getAircraft());
                    CLogMessage(this).info("Aircraft removed, '%1' '%2' object id '%3' out of reality bubble") << callsign.toQString() << simObject.getAircraftModelString() << objectID;
                }
                else
                {
                    CLogMessage(this).warning("Removed %1 from simulator, but was not initiated by us: %1 '%2' object id %3") << callsign.toQString() << simObject.getAircraftModelString() << objectID;
                }
            }

            // in all cases we remove
            const int c = m_simConnectObjects.remove(callsign);
            ok = c > 0;
            const bool updated = this->updateAircraftRendered(simObject.getCallsign(), false);
            if (updated)
            {
                emit aircraftRenderingChanged(simObject.getAircraft());
            }

            // models we have to add again after removing
            if (m_aircraftToAddAgainWhenRemoved.containsCallsign(callsign))
            {
                const CSimulatedAircraft aircraftAddAgain = m_aircraftToAddAgainWhenRemoved.findFirstByCallsign(callsign);
                QTimer::singleShot(1000, this,  [ = ] { this->physicallyAddRemoteAircraft(aircraftAddAgain); });
            }
            return ok;
        }

        bool CSimulatorFsxCommon::setSimConnectObjectId(DWORD requestID, DWORD objectID)
        {
            return this->m_simConnectObjects.setSimConnectObjectIdForRequestId(requestID, objectID, true);
        }

        bool CSimulatorFsxCommon::setCurrentLights(const CCallsign &callsign, const CAircraftLights &lights)
        {
            if (!m_simConnectObjects.contains(callsign)) { return false; }
            m_simConnectObjects[callsign].setCurrentLightsInSimulator(lights);
            return true;
        }

        bool CSimulatorFsxCommon::setLightsAsSent(const CCallsign &callsign, const CAircraftLights &lights)
        {
            if (!m_simConnectObjects.contains(callsign)) { return false; }
            m_simConnectObjects[callsign].setLightsAsSent(lights);
            return true;
        }

        void CSimulatorFsxCommon::timerEvent(QTimerEvent *event)
        {
            Q_UNUSED(event);
            ps_dispatch();
        }

        void CSimulatorFsxCommon::ps_dispatch()
        {
            HRESULT hr = SimConnect_CallDispatch(m_hSimConnect, SimConnectProc, this);
            if (hr != S_OK)
            {
                m_dispatchErrors++;
                if (m_dispatchErrors == 2)
                {
                    // 2nd time, an error / avoid multiple messages
                    // idea: if it happens once ignore
                    CLogMessage(this).error("FSX: Dispatch error");
                }
                else if (m_dispatchErrors > 5)
                {
                    // this normally happens during a FSX crash or shutdown
                    this->disconnectFrom();
                }
                return;
            }
            m_dispatchErrors = 0;
            if (m_useFsuipc && m_fsuipc)
            {
                CSimulatedAircraft fsuipcAircraft(getOwnAircraft());
                //! \fixme split in high / low frequency reads
                bool ok = m_fsuipc->read(fsuipcAircraft, true, true, true);
                if (ok)
                {
                    // do whatever is required
                    Q_UNUSED(fsuipcAircraft);
                }
            }
        }

        bool CSimulatorFsxCommon::physicallyRemoveRemoteAircraft(const CCallsign &callsign)
        {
            // only remove from sim
            Q_ASSERT_X(CThreadUtils::isCurrentThreadObjectThread(this), Q_FUNC_INFO, "wrong thread");
            if (callsign.isEmpty()) { return false; } // can happen if an object is not an aircraft

            m_outOfRealityBubble.removeByCallsign(callsign);
            if (!m_simConnectObjects.contains(callsign)) { return false; } // already fully removed or not yet added

            CSimConnectObject &simObject = m_simConnectObjects[callsign];
            if (simObject.isPendingRemoved()) { return true; }
            if (simObject.isPendingAdded())
            {
                // problem: we try to delete an aircraft just requested to be added
                return false; //! \fixme improve, since this scenario is not really covered
            }

            simObject.setPendingRemoved(true);
            if (m_interpolationRenderingSetup.showSimulatorDebugMessages())
            {
                CLogMessage(this).debug() << "physicallyRemoveRemoteAircraft" << callsign.toQString();
            }

            // call in SIM
            SimConnect_AIRemoveObject(m_hSimConnect, static_cast<SIMCONNECT_OBJECT_ID>(simObject.getObjectId()), static_cast<SIMCONNECT_DATA_REQUEST_ID>(m_requestIdSimData++));
            m_hints.remove(simObject.getCallsign());

            // mark in provider
            bool updated = updateAircraftRendered(callsign, false);
            if (updated)
            {
                CSimulatedAircraft aircraft(simObject.getAircraft());
                aircraft.setRendered(false);
                emit aircraftRenderingChanged(aircraft);
            }

            // cleanup function, actually this should not be needed
            QTimer::singleShot(100, this, &CSimulatorFsxCommon::ps_physicallyRemoveAircraftNotInProvider);

            // bye
            return true;
        }

        int CSimulatorFsxCommon::physicallyRemoveAllRemoteAircraft()
        {
            if (m_simConnectObjects.isEmpty()) { return 0; }
            const QList<CCallsign> callsigns(m_simConnectObjects.keys());
            int r = 0;
            for (const CCallsign &cs : callsigns)
            {
                if (physicallyRemoveRemoteAircraft(cs)) { r++; }
            }
            clearAllAircraft();
            return r;
        }

        HRESULT CSimulatorFsxCommon::initEvents()
        {
            HRESULT hr = S_OK;
            // System events, see http://msdn.microsoft.com/en-us/library/cc526983.aspx#SimConnect_SubscribeToSystemEvent
            hr += SimConnect_SubscribeToSystemEvent(m_hSimConnect, SystemEventSimStatus, "Sim");
            hr += SimConnect_SubscribeToSystemEvent(m_hSimConnect, SystemEventObjectAdded, "ObjectAdded");
            hr += SimConnect_SubscribeToSystemEvent(m_hSimConnect, SystemEventObjectRemoved, "ObjectRemoved");
            hr += SimConnect_SubscribeToSystemEvent(m_hSimConnect, SystemEventFrame, "Frame");
            hr += SimConnect_SubscribeToSystemEvent(m_hSimConnect, SystemEventPause, "Pause");
            hr += SimConnect_SubscribeToSystemEvent(m_hSimConnect, SystemEventFlightLoaded, "FlightLoaded");
            if (hr != S_OK)
            {
                CLogMessage(this).error("FSX plugin error: %1") << "SimConnect_SubscribeToSystemEvent failed";
                return hr;
            }

            // Mapped events, see event ids here: http://msdn.microsoft.com/en-us/library/cc526980.aspx
            hr += SimConnect_MapClientEventToSimEvent(m_hSimConnect, EventPauseToggle, "PAUSE_TOGGLE");
            hr += SimConnect_MapClientEventToSimEvent(m_hSimConnect, SystemEventSlewToggle, "SLEW_TOGGLE");
            hr += SimConnect_MapClientEventToSimEvent(m_hSimConnect, EventFreezeLat, "FREEZE_LATITUDE_LONGITUDE_SET");
            hr += SimConnect_MapClientEventToSimEvent(m_hSimConnect, EventFreezeAlt, "FREEZE_ALTITUDE_SET");
            hr += SimConnect_MapClientEventToSimEvent(m_hSimConnect, EventFreezeAtt, "FREEZE_ATTITUDE_SET");
            hr += SimConnect_MapClientEventToSimEvent(m_hSimConnect, EventSetCom1Active, "COM_RADIO_SET");
            hr += SimConnect_MapClientEventToSimEvent(m_hSimConnect, EventSetCom1Standby, "COM_STBY_RADIO_SET");
            hr += SimConnect_MapClientEventToSimEvent(m_hSimConnect, EventSetCom2Active, "COM2_RADIO_SET");
            hr += SimConnect_MapClientEventToSimEvent(m_hSimConnect, EventSetCom2Standby, "COM2_STBY_RADIO_SET");
            hr += SimConnect_MapClientEventToSimEvent(m_hSimConnect, EventSetTransponderCode, "XPNDR_SET");

            hr += SimConnect_MapClientEventToSimEvent(m_hSimConnect, EventSetTimeZuluYear, "ZULU_YEAR_SET");
            hr += SimConnect_MapClientEventToSimEvent(m_hSimConnect, EventSetTimeZuluDay, "ZULU_DAY_SET");
            hr += SimConnect_MapClientEventToSimEvent(m_hSimConnect, EventSetTimeZuluHours, "ZULU_HOURS_SET");
            hr += SimConnect_MapClientEventToSimEvent(m_hSimConnect, EventSetTimeZuluMinutes, "ZULU_MINUTES_SET");

            hr += SimConnect_MapClientEventToSimEvent(m_hSimConnect, EventLandingLightsOff, "LANDING_LIGHTS_OFF");
            hr += SimConnect_MapClientEventToSimEvent(m_hSimConnect, EventLandinglightsOn, "LANDING_LIGHTS_ON");
            hr += SimConnect_MapClientEventToSimEvent(m_hSimConnect, EventLandingLightsSet, "LANDING_LIGHTS_SET");
            hr += SimConnect_MapClientEventToSimEvent(m_hSimConnect, EventLandingLightsToggle, "LANDING_LIGHTS_TOGGLE");
            hr += SimConnect_MapClientEventToSimEvent(m_hSimConnect, EventPanelLightsOff, "PANEL_LIGHTS_OFF");
            hr += SimConnect_MapClientEventToSimEvent(m_hSimConnect, EventPanelLightsOn, "PANEL_LIGHTS_ON");
            hr += SimConnect_MapClientEventToSimEvent(m_hSimConnect, EventPanelLightsSet, "PANEL_LIGHTS_SET");
            hr += SimConnect_MapClientEventToSimEvent(m_hSimConnect, EventStrobesOff, "STROBES_OFF");
            hr += SimConnect_MapClientEventToSimEvent(m_hSimConnect, EventStrobesOn, "STROBES_ON");
            hr += SimConnect_MapClientEventToSimEvent(m_hSimConnect, EventStrobesSet, "STROBES_SET");
            hr += SimConnect_MapClientEventToSimEvent(m_hSimConnect, EventStrobesToggle, "STROBES_TOGGLE");
            hr += SimConnect_MapClientEventToSimEvent(m_hSimConnect, EventToggleBeaconLights, "TOGGLE_BEACON_LIGHTS");
            hr += SimConnect_MapClientEventToSimEvent(m_hSimConnect, EventToggleCabinLights, "TOGGLE_CABIN_LIGHTS");
            hr += SimConnect_MapClientEventToSimEvent(m_hSimConnect, EventToggleLogoLights, "TOGGLE_LOGO_LIGHTS");
            hr += SimConnect_MapClientEventToSimEvent(m_hSimConnect, EventToggleNavLights, "TOGGLE_NAV_LIGHTS");
            hr += SimConnect_MapClientEventToSimEvent(m_hSimConnect, EventToggleRecognitionLights, "TOGGLE_RECOGNITION_LIGHTS");
            hr += SimConnect_MapClientEventToSimEvent(m_hSimConnect, EventToggleTaxiLights, "TOGGLE_TAXI_LIGHTS");
            hr += SimConnect_MapClientEventToSimEvent(m_hSimConnect, EventToggleWingLights, "TOGGLE_WING_LIGHTS");

            if (hr != S_OK)
            {
                CLogMessage(this).error("FSX plugin error: %1") << "SimConnect_MapClientEventToSimEvent failed";
                return hr;
            }

            // facility
            hr += SimConnect_SubscribeToFacilities(m_hSimConnect, SIMCONNECT_FACILITY_LIST_TYPE_AIRPORT, static_cast<SIMCONNECT_DATA_REQUEST_ID>(m_requestIdSimData++));
            if (hr != S_OK)
            {
                CLogMessage(this).error("FSX plugin error: %1") << "SimConnect_SubscribeToFacilities failed";
                return hr;
            }
            return hr;
        }

        HRESULT CSimulatorFsxCommon::initDataDefinitionsWhenConnected()
        {
            return CSimConnectDefinitions::initDataDefinitionsWhenConnected(m_hSimConnect);
        }

        HRESULT CSimulatorFsxCommon::initWhenConnected()
        {
            // called when connected

            HRESULT hr = initEvents();
            if (hr != S_OK)
            {
                CLogMessage(this).error("FSX plugin: initEvents failed");
                return hr;
            }

            // inti data definitions and SB data area
            hr += initDataDefinitionsWhenConnected();
            if (hr != S_OK)
            {
                CLogMessage(this).error("FSX plugin: initDataDefinitionsWhenConnected failed");
                return hr;
            }

            return hr;
        }

        void CSimulatorFsxCommon::updateRemoteAircraft()
        {
            static_assert(sizeof(DataDefinitionRemoteAircraftPartsWithoutLights) == sizeof(double) * 10, "DataDefinitionRemoteAircraftPartsWithoutLights has an incorrect size.");
            Q_ASSERT_X(CThreadUtils::isCurrentThreadObjectThread(this), Q_FUNC_INFO, "thread");

            // nothing to do, reset request id and exit
            if (this->isPaused() && this->m_pausedSimFreezesInterpolation) { return; } // no interpolation while paused
            const int remoteAircraftNo = this->getAircraftInRangeCount();
            if (remoteAircraftNo < 1) { m_interpolationRequest = 0;  return; }

            // interpolate and send to simulator
            this->m_interpolationRequest++;
            const bool enableAircraftParts = this->m_interpolationRenderingSetup.isAircraftPartsEnabled();
            const CCallsignSet aircraftWithParts = enableAircraftParts ? this->remoteAircraftSupportingParts() : CCallsignSet(); // optimization, fetch all parts supporting aircraft in one step (one lock)

            // values used for position and parts
            const qint64 currentTimestamp = QDateTime::currentMSecsSinceEpoch();
            const QList<CSimConnectObject> simObjects(m_simConnectObjects.values());
            const CCallsignSet callsignsToLog(this->m_interpolationRenderingSetup.getLogCallsigns());

            // interpolation for all remote aircraft
            for (const CSimConnectObject &simObj : simObjects)
            {
                // happening if aircraft is not yet added to simulator or to be deleted
                if (simObj.isPendingAdded()) { continue; }
                if (simObj.isPendingRemoved()) { continue; }

                const CCallsign callsign(simObj.getCallsign());
                Q_ASSERT_X(!callsign.isEmpty(), Q_FUNC_INFO, "missing callsign");
                Q_ASSERT_X(simObj.hasValidRequestAndObjectId(), Q_FUNC_INFO, "Missing ids");

                // fetch parts, as they are needed for ground interpolation
                const bool useAircraftParts = enableAircraftParts && aircraftWithParts.contains(callsign);
                const bool logInterpolationAndParts = callsignsToLog.contains(callsign);
                const CInterpolationAndRenderingSetup setup(getInterpolationAndRenderingSetup());
                CPartsStatus partsStatus(useAircraftParts);
                const CAircraftParts parts = useAircraftParts ? simObj.getInterpolator()->getInterpolatedParts(-1, setup, partsStatus, logInterpolationAndParts) : CAircraftParts();

                // get interpolated situation
                CInterpolationStatus interpolatorStatus;
                CInterpolationHints hints(m_hints[callsign]);
                hints.setAircraftParts(useAircraftParts ? parts : CAircraftParts(), useAircraftParts);
                hints.setLoggingInterpolation(logInterpolationAndParts);
                const CAircraftSituation interpolatedSituation = simObj.getInterpolator()->getInterpolatedSituation(currentTimestamp, setup, hints, interpolatorStatus);

                if (interpolatorStatus.allTrue())
                {
                    // update situation
                    SIMCONNECT_DATA_INITPOSITION position = this->aircraftSituationToFsxPosition(interpolatedSituation);
                    HRESULT hr = S_OK;
                    hr += SimConnect_SetDataOnSimObject(m_hSimConnect, CSimConnectDefinitions::DataRemoteAircraftPosition,
                                                        static_cast<SIMCONNECT_OBJECT_ID>(simObj.getObjectId()), 0, 0,
                                                        sizeof(SIMCONNECT_DATA_INITPOSITION), &position);
                    if (hr != S_OK)
                    {
                        CLogMessage(this).warning("Failed so set position on SimObject '%1' callsign: '%2'") << simObj.getObjectId() << callsign;
                    }

                } // interpolation data

                if (useAircraftParts)
                {
                    this->updateRemoteAircraftParts(simObj, parts, partsStatus);
                }
                else
                {
                    // guess on position, but not every frame
                    if (m_interpolationRequest % GuessRemoteAircraftPartsCycle == 0)
                    {
                        this->guessAndUpdateRemoteAircraftParts(simObj, interpolatedSituation, interpolatorStatus);
                    }
                }
            } // all callsigns
            const qint64 dt = QDateTime::currentMSecsSinceEpoch() - currentTimestamp;
            m_statsUpdateAircraftTimeTotalMs += dt;
            m_statsUpdateAircraftCountMs++;
            m_statsUpdateAircraftTimeAvgMs = m_statsUpdateAircraftTimeTotalMs / m_statsUpdateAircraftCountMs;
        }

        bool CSimulatorFsxCommon::guessAndUpdateRemoteAircraftParts(const CSimConnectObject &simObj, const CAircraftSituation &interpolatedSituation, const CInterpolationStatus &interpolationStatus)
        {
            if (!simObj.hasValidRequestAndObjectId()) { return false; }
            if (!interpolationStatus.didInterpolationSucceed()) { return false; }

            CAircraftLights lights;
            DataDefinitionRemoteAircraftPartsWithoutLights ddRemoteAircraftPartsWintoutLights = {}; // init members
            const bool isOnGround = interpolatedSituation.isOnGround() == CAircraftSituation::OnGround;
            const double gsKts = interpolatedSituation.getGroundSpeed().value(CSpeedUnit::kts());
            ddRemoteAircraftPartsWintoutLights.setAllEngines(true);
            lights.setCabinOn(true);
            lights.setRecognitionOn(true);

            // when first detected moving, lights on
            if (isOnGround)
            {
                ddRemoteAircraftPartsWintoutLights.gearHandlePosition = 1;
                lights.setTaxiOn(true);
                lights.setBeaconOn(true);
                lights.setNavOn(true);

                if (gsKts > 5)
                {
                    // mode taxi
                    lights.setTaxiOn(true);
                    lights.setLandingOn(false);
                }
                else if (gsKts > 30)
                {
                    // mode accelaration for takeoff
                    lights.setTaxiOn(false);
                    lights.setLandingOn(true);
                }
                else
                {
                    // slow movements or parking
                    lights.setTaxiOn(false);
                    lights.setLandingOn(false);
                    ddRemoteAircraftPartsWintoutLights.setAllEngines(false);
                }
            }
            else
            {
                // not on ground
                ddRemoteAircraftPartsWintoutLights.gearHandlePosition = 0;
                lights.setTaxiOn(false);
                lights.setBeaconOn(true);
                lights.setNavOn(true);
                // landing lights for < 10000ft (normally MSL, here ignored)
                lights.setLandingOn(interpolatedSituation.getAltitude().value(CLengthUnit::ft()) < 10000);

                if (!simObj.isVtol() && interpolatedSituation.hasGroundElevation())
                {
                    if (interpolatedSituation.getHeightAboveGround().value(CLengthUnit::ft()) < 1000)
                    {
                        ddRemoteAircraftPartsWintoutLights.gearHandlePosition = 1;
                        ddRemoteAircraftPartsWintoutLights.flapsTrailingEdgeRightPercent = 25;
                        ddRemoteAircraftPartsWintoutLights.flapsTrailingEdgeLeftPercent = 25;
                    }
                    else if (interpolatedSituation.getHeightAboveGround().value(CLengthUnit::ft()) < 2000)
                    {
                        ddRemoteAircraftPartsWintoutLights.gearHandlePosition = 1;
                        ddRemoteAircraftPartsWintoutLights.flapsTrailingEdgeRightPercent = 10;
                        ddRemoteAircraftPartsWintoutLights.flapsTrailingEdgeLeftPercent = 10;
                    }
                }
            }

            return this->sendRemoteAircraftPartsToSimulator(simObj, ddRemoteAircraftPartsWintoutLights, lights);
        }

        bool CSimulatorFsxCommon::updateRemoteAircraftParts(const CSimConnectObject &simObj, const CAircraftParts &parts, const CPartsStatus &partsStatus)
        {
            if (!simObj.hasValidRequestAndObjectId()) { return false; }
            if (!partsStatus.isSupportingParts()) { return false; }

            DataDefinitionRemoteAircraftPartsWithoutLights ddRemoteAircraftPartsWithoutLights; // no init, all values will be set
            ddRemoteAircraftPartsWithoutLights.flapsLeadingEdgeLeftPercent = parts.getFlapsPercent() / 100.0;
            ddRemoteAircraftPartsWithoutLights.flapsLeadingEdgeRightPercent = parts.getFlapsPercent() / 100.0;
            ddRemoteAircraftPartsWithoutLights.flapsTrailingEdgeLeftPercent = parts.getFlapsPercent() / 100.0;
            ddRemoteAircraftPartsWithoutLights.flapsTrailingEdgeRightPercent = parts.getFlapsPercent() / 100.0;
            ddRemoteAircraftPartsWithoutLights.spoilersHandlePosition = parts.isSpoilersOut() ? 1 : 0;
            ddRemoteAircraftPartsWithoutLights.gearHandlePosition = parts.isGearDown() ? 1 : 0;
            ddRemoteAircraftPartsWithoutLights.engine1Combustion = parts.isEngineOn(1) ? 1 : 0;
            ddRemoteAircraftPartsWithoutLights.engine2Combustion = parts.isEngineOn(2) ? 1 : 0;
            ddRemoteAircraftPartsWithoutLights.engine3Combustion = parts.isEngineOn(3) ? 1 : 0;
            ddRemoteAircraftPartsWithoutLights.engine4Combustion = parts.isEngineOn(4) ? 1 : 0;

            CAircraftLights lights = parts.getLights();
            lights.setRecognitionOn(parts.isAnyEngineOn());
            lights.setCabinOn(parts.isAnyEngineOn());

            return this->sendRemoteAircraftPartsToSimulator(simObj, ddRemoteAircraftPartsWithoutLights, parts.getLights());
        }

        bool CSimulatorFsxCommon::sendRemoteAircraftPartsToSimulator(const CSimConnectObject &simObj, DataDefinitionRemoteAircraftPartsWithoutLights &ddRemoteAircraftPartsWithoutLights, const CAircraftLights &lights)
        {
            Q_ASSERT(m_hSimConnect);
            const DWORD objectId = simObj.getObjectId();

            // same as in simulator or same as already send to simulator?
            const CAircraftLights sentLights(simObj.getLightsAsSent());
            if (simObj.getPartsAsSent() == ddRemoteAircraftPartsWithoutLights && sentLights == lights)
            {
                return true;
            }

            // in case we sent, we sent everything
            const HRESULT hr = SimConnect_SetDataOnSimObject(m_hSimConnect, CSimConnectDefinitions::DataRemoteAircraftParts,
                               objectId, SIMCONNECT_DATA_SET_FLAG_DEFAULT, 0,
                               sizeof(DataDefinitionRemoteAircraftPartsWithoutLights), &ddRemoteAircraftPartsWithoutLights);

            if (hr == S_OK)
            {
                // Update data
                CSimConnectObject &objUdpate = m_simConnectObjects[simObj.getCallsign()];
                objUdpate.setPartsAsSent(ddRemoteAircraftPartsWithoutLights);
            }
            else
            {
                CLogMessage(this).warning("Failed so set parts on SimObject '%1' callsign: '%2'") << simObj.getObjectId() << simObj.getCallsign();
            }

            // lights we can set directly
            SimConnect_TransmitClientEvent(m_hSimConnect, objectId, EventLandingLightsSet, lights.isLandingOn() ? 1.0 : 0.0, SIMCONNECT_GROUP_PRIORITY_HIGHEST, SIMCONNECT_EVENT_FLAG_GROUPID_IS_PRIORITY);
            SimConnect_TransmitClientEvent(m_hSimConnect, objectId, EventStrobesSet, lights.isStrobeOn() ? 1.0 : 0.0, SIMCONNECT_GROUP_PRIORITY_HIGHEST, SIMCONNECT_EVENT_FLAG_GROUPID_IS_PRIORITY);

            // lights we need to toggle (risk with quickly changing values that we accidentally toggle back)
            sendToggledLightsToSimulator(simObj, lights);

            // done
            return hr == S_OK;
        }

        void CSimulatorFsxCommon::sendToggledLightsToSimulator(const CSimConnectObject &simObj, const CAircraftLights &lightsWanted, bool force)
        {
            if (!simObj.hasValidRequestAndObjectId()) { return; } // stale
            const CAircraftLights lightsIsState = simObj.getCurrentLightsInSimulator();
            if (lightsWanted == lightsIsState) { return; }
            if (!force && lightsWanted == simObj.getLightsAsSent()) { return; }
            const CCallsign callsign(simObj.getCallsign());

            // Update data
            CSimConnectObject &simObjToUpdate = m_simConnectObjects[callsign];
            simObjToUpdate.setLightsAsSent(lightsWanted);

            // state available, then I can toggle
            if (!lightsIsState.isNull())
            {
                const DWORD objectId = simObj.getObjectId();
                if (lightsWanted.isTaxiOn() != lightsIsState.isTaxiOn())
                {
                    SimConnect_TransmitClientEvent(m_hSimConnect, objectId, EventToggleTaxiLights, 0.0, SIMCONNECT_GROUP_PRIORITY_HIGHEST, SIMCONNECT_EVENT_FLAG_GROUPID_IS_PRIORITY);
                }
                if (lightsWanted.isNavOn() != lightsIsState.isNavOn())
                {
                    SimConnect_TransmitClientEvent(m_hSimConnect, objectId, EventToggleNavLights, 0.0, SIMCONNECT_GROUP_PRIORITY_HIGHEST, SIMCONNECT_EVENT_FLAG_GROUPID_IS_PRIORITY);
                }
                if (lightsWanted.isBeaconOn() != lightsIsState.isBeaconOn())
                {
                    SimConnect_TransmitClientEvent(m_hSimConnect, objectId, EventToggleBeaconLights, 0.0, SIMCONNECT_GROUP_PRIORITY_HIGHEST, SIMCONNECT_EVENT_FLAG_GROUPID_IS_PRIORITY);
                }
                if (lightsWanted.isLogoOn() != lightsIsState.isLogoOn())
                {
                    SimConnect_TransmitClientEvent(m_hSimConnect, objectId, EventToggleLogoLights, 0.0, SIMCONNECT_GROUP_PRIORITY_HIGHEST, SIMCONNECT_EVENT_FLAG_GROUPID_IS_PRIORITY);
                }
                if (lightsWanted.isRecognitionOn() != lightsIsState.isRecognitionOn())
                {
                    SimConnect_TransmitClientEvent(m_hSimConnect, objectId, EventToggleRecognitionLights, 0.0, SIMCONNECT_GROUP_PRIORITY_HIGHEST, SIMCONNECT_EVENT_FLAG_GROUPID_IS_PRIORITY);
                }
                if (lightsWanted.isCabinOn() != lightsIsState.isCabinOn())
                {
                    SimConnect_TransmitClientEvent(m_hSimConnect, objectId, EventToggleCabinLights, 0.0, SIMCONNECT_GROUP_PRIORITY_HIGHEST, SIMCONNECT_EVENT_FLAG_GROUPID_IS_PRIORITY);
                }
                return;
            }

            // missing lights info from simulator
            CLogMessage(this).info("Missing light state for '%1'") << callsign;
            QTimer::singleShot(2500, this, [ = ]
            {
                const CSimConnectObject currentSimObj = m_simConnectObjects[callsign];
                if (!currentSimObj.hasValidRequestAndObjectId()) { return; } // stale
                if (lightsWanted != currentSimObj.getLightsAsSent())  { return; } // changed in between
                sendToggledLightsToSimulator(currentSimObj, lightsWanted, true);
            });
        }

        SIMCONNECT_DATA_INITPOSITION CSimulatorFsxCommon::aircraftSituationToFsxPosition(const CAircraftSituation &situation)
        {
            SIMCONNECT_DATA_INITPOSITION position;
            position.Latitude = situation.latitude().value(CAngleUnit::deg());
            position.Longitude = situation.longitude().value(CAngleUnit::deg());
            position.Altitude = situation.getAltitude().value(CLengthUnit::ft()); // already corrected in interpolator if there is an underflow
            position.Heading = situation.getHeading().value(CAngleUnit::deg());
            position.Airspeed = situation.getGroundSpeed().value(CSpeedUnit::kts());

            // MSFS has inverted pitch and bank angles
            position.Pitch = -situation.getPitch().value(CAngleUnit::deg());
            position.Bank = -situation.getBank().value(CAngleUnit::deg());
            position.OnGround = 0U;

            if (situation.isOnGroundInfoAvailable())
            {
                const bool onGround = situation.isOnGround() == CAircraftSituation::OnGround;
                position.OnGround = onGround ? 1U : 0U;
            }
            return position;
        }

        void CSimulatorFsxCommon::synchronizeTime(const CTime &zuluTimeSim, const CTime &localTimeSim)
        {
            if (!this->m_simTimeSynced) { return; }
            if (!this->isConnected())   { return; }
            if (m_syncDeferredCounter > 0)
            {
                --m_syncDeferredCounter;
            }
            Q_UNUSED(localTimeSim);

            QDateTime myDateTime = QDateTime::currentDateTimeUtc();
            if (!this->m_syncTimeOffset.isZeroEpsilonConsidered())
            {
                int offsetSeconds = this->m_syncTimeOffset.valueRounded(CTimeUnit::s(), 0);
                myDateTime = myDateTime.addSecs(offsetSeconds);
            }
            const QTime myTime = myDateTime.time();
            const DWORD h = static_cast<DWORD>(myTime.hour());
            const DWORD m = static_cast<DWORD>(myTime.minute());
            const int targetMins = myTime.hour() * 60 + myTime.minute();
            const int simMins = zuluTimeSim.valueRounded(CTimeUnit::min());
            const int diffMins = qAbs(targetMins - simMins);
            if (diffMins < 2) { return; }
            HRESULT hr = S_OK;
            hr += SimConnect_TransmitClientEvent(m_hSimConnect, 0, EventSetTimeZuluHours, h, SIMCONNECT_GROUP_PRIORITY_STANDARD, SIMCONNECT_EVENT_FLAG_GROUPID_IS_PRIORITY);
            hr += SimConnect_TransmitClientEvent(m_hSimConnect, 0, EventSetTimeZuluMinutes, m, SIMCONNECT_GROUP_PRIORITY_STANDARD, SIMCONNECT_EVENT_FLAG_GROUPID_IS_PRIORITY);

            if (hr != S_OK)
            {
                CLogMessage(this).warning("Sending time sync failed!");
            }
            else
            {
                m_syncDeferredCounter = 5; // allow some time to sync
                CLogMessage(this).info("Synchronized time to UTC: '%1'") << myTime.toString();
            }
        }

        void CSimulatorFsxCommon::injectWeatherGrid(const Weather::CWeatherGrid &weatherGrid)
        {
            // So far, there is only global weather
            auto glob = weatherGrid.frontOrDefault();
            glob.setIdentifier("GLOB");
            QString metar = CSimConnectUtilities::convertToSimConnectMetar(glob);
            SimConnect_WeatherSetModeCustom(m_hSimConnect);
            SimConnect_WeatherSetModeGlobal(m_hSimConnect);
            SimConnect_WeatherSetObservation(m_hSimConnect, 0, qPrintable(metar));
        }

        bool CSimulatorFsxCommon::requestDataForSimObject(const CSimConnectObject &simObject, SIMCONNECT_PERIOD period)
        {
            if (!simObject.hasValidRequestAndObjectId()) { return false; }
            if (simObject.getSimDataPeriod() == period) { return true; } // already queried like this

            // always request, not only when something has changed
            const HRESULT result = SimConnect_RequestDataOnSimObject(
                                       m_hSimConnect, simObject.getRequestId() + RequestSimDataOffset,
                                       CSimConnectDefinitions::DataRemoteAircraftSimData,
                                       simObject.getObjectId(), period);
            if (result == S_OK)
            {
                m_simConnectObjects[simObject.getCallsign()].setSimDataPeriod(period);
                return true;
            }
            CLogMessage(this).error("Cannot request sim data on object '%1'") << simObject.getObjectId();
            return false;
        }

        bool CSimulatorFsxCommon::requestLightsForSimObject(const CSimConnectObject &simObject)
        {
            if (!simObject.hasValidRequestAndObjectId()) { return false; }

            // always request, not only when something has changed
            const HRESULT result = SimConnect_RequestDataOnSimObject(
                                       m_hSimConnect, simObject.getRequestId() + RequestLightsOffset,
                                       CSimConnectDefinitions::DataRemoteAircraftLights, simObject.getObjectId(),
                                       SIMCONNECT_PERIOD_SECOND);
            if (result == S_OK) { return true; }
            CLogMessage(this).error("Cannot request lights data on object '%1'") << simObject.getObjectId();
            return false;
        }

        void CSimulatorFsxCommon::initSimulatorInternals()
        {
            CSimulatorFsCommon::initSimulatorInternals();
            CSimulatorInternals s = this->m_simulatorInternals;
            const QString fsxPath = CFsCommonUtil::fsxDirFromRegistry(); // can be empty for remote FSX
            if (!fsxPath.isEmpty()) { s.setSimulatorInstallationDirectory(fsxPath); }

            s.setValue("fsx/simConnectCfgFilename", CSimConnectUtilities::getLocalSimConnectCfgFilename());
            s.setValue("fsx/simConnectVersion", this->m_simConnectVersion);
            this->m_simulatorInternals = s;
        }

        void CSimulatorFsxCommon::reset()
        {
            if (m_simConnectTimerId >= 0) { killTimer(m_simConnectTimerId); }
            m_simConnectTimerId   = -1;
            m_simConnected = false;
            m_simSimulating = false;
            m_syncDeferredCounter =  0;
            m_skipCockpitUpdateCycles = 0;
            m_interpolationRequest  = 0;
            m_requestIdSimData = RequestSimDataStart;
            m_dispatchErrors = 0;
            m_receiveExceptionCount = 0;
            CSimulatorFsCommon::reset();
        }

        void CSimulatorFsxCommon::clearAllAircraft()
        {
            m_simConnectObjects.clear();
            m_outOfRealityBubble.clear();
            CSimulatorFsCommon::clearAllAircraft();
        }

        void CSimulatorFsxCommon::ps_remoteProviderAddAircraftSituation(const CAircraftSituation &situation)
        {
            if (!m_simConnectObjects.contains(situation.getCallsign())) { return; }
            m_simConnectObjects[situation.getCallsign()].addAircraftSituation(situation);
        }

        void CSimulatorFsxCommon::ps_remoteProviderAddAircraftParts(const BlackMisc::Aviation::CCallsign &callsign, const CAircraftParts &parts)
        {
            if (!m_simConnectObjects.contains(callsign)) { return; }
            m_simConnectObjects[callsign].addAircraftParts(parts);
        }

        QString CSimulatorFsxCommon::fsxPositionToString(const SIMCONNECT_DATA_INITPOSITION &position)
        {
            const QString positionStr("Lat: %1 lng: %2 alt: %3ft pitch: %4 bank: %5 hdg: %6 airspeed: %7kts onGround: %8");
            return positionStr.
                   arg(position.Latitude).arg(position.Longitude).arg(position.Altitude).
                   arg(position.Pitch).arg(position.Bank).arg(position.Heading).arg(position.Airspeed).arg(position.OnGround);
        }

        CCallsignSet CSimulatorFsxCommon::getCallsignsMissingInProvider() const
        {
            CCallsignSet simObjectCallsigns(m_simConnectObjects.keys());
            CCallsignSet providerCallsigns(this->getAircraftInRangeCallsigns());
            return simObjectCallsigns.difference(providerCallsigns);
        }

        CCallsignSet CSimulatorFsxCommon::ps_physicallyRemoveAircraftNotInProvider()
        {
            const CCallsignSet toBeRemoved(getCallsignsMissingInProvider());
            if (toBeRemoved.isEmpty()) { return toBeRemoved; }
            for (const CCallsign &callsign : toBeRemoved)
            {
                physicallyRemoveRemoteAircraft(callsign);
            }
            return toBeRemoved;
        }

        CSimulatorFsxCommonListener::CSimulatorFsxCommonListener(const CSimulatorPluginInfo &info) :
            ISimulatorListener(info),
            m_timer(new QTimer(this))
        {
            constexpr int QueryInterval = 5 * 1000; // 5 seconds
            m_timer->setInterval(QueryInterval);
            m_timer->setObjectName(this->objectName().append(":m_timer"));
            connect(m_timer, &QTimer::timeout, this, &CSimulatorFsxCommonListener::checkConnection);
        }

        void CSimulatorFsxCommonListener::startImpl()
        {
            m_simulatorVersion.clear();
            m_simConnectVersion.clear();
            m_simulatorName.clear();
            m_simulatorDetails.clear();
            m_timer->start();
        }

        void CSimulatorFsxCommonListener::stopImpl()
        {
            m_timer->stop();
        }

        QString CSimulatorFsxCommonListener::backendInfo() const
        {
            if (m_simulatorName.isEmpty()) { return ISimulatorListener::backendInfo(); }
            return m_simulatorDetails;
        }

        void CSimulatorFsxCommonListener::checkConnection()
        {
            Q_ASSERT_X(sApp, Q_FUNC_INFO, "Missing sApp");
            Q_ASSERT_X(!CThreadUtils::isCurrentThreadApplicationThread(), Q_FUNC_INFO, "Expect to run in background");
            HANDLE hSimConnect;
            HRESULT result = SimConnect_Open(&hSimConnect, sApp->swiftVersionChar(), nullptr, 0, 0, 0);
            bool check = false;
            if (result == S_OK)
            {
                for (int i = 0; !check && i < 3; i++)
                {
                    // result not always in first dispatch
                    result = SimConnect_CallDispatch(hSimConnect, CSimulatorFsxCommonListener::SimConnectProc, this);
                    if (result != S_OK) { break; } // means serious failure
                    check = this->checkVersionAndSimulator();
                    sApp->processEventsFor(500);
                }
            }
            SimConnect_Close(hSimConnect);

            if (check)
            {
                emit simulatorStarted(this->getPluginInfo());
            }
        }

        bool CSimulatorFsxCommonListener::checkVersionAndSimulator() const
        {
            const CSimulatorInfo sim(getPluginInfo().getIdentifier());
            const QString simName = m_simulatorName.toLower().trimmed();

            if (simName.isEmpty()) { return false; }
            if (sim.p3d())
            {
                return simName.contains("lockheed") || simName.contains("martin") || simName.contains("p3d") || simName.contains("prepar");
            }
            else if (sim.fsx())
            {
                return simName.contains("fsx") || simName.contains("microsoft") || simName.contains("simulator x");
            }
            return false;
        }

        bool CSimulatorFsxCommonListener::checkSimConnectDll() const
        {
            static const CWinDllUtils::DLLInfo simConnectInfo = CSimConnectUtilities::simConnectDllInfo();
            if (!simConnectInfo.errorMsg.isEmpty()) { return false; }
            return true;
        }

        void CSimulatorFsxCommonListener::SimConnectProc(SIMCONNECT_RECV *pData, DWORD cbData, void *pContext)
        {
            Q_UNUSED(cbData);
            CSimulatorFsxCommonListener *simListener = static_cast<CSimulatorFsxCommonListener *>(pContext);
            switch (pData->dwID)
            {
            case SIMCONNECT_RECV_ID_OPEN:
                {
                    SIMCONNECT_RECV_OPEN *event = (SIMCONNECT_RECV_OPEN *)pData;
                    simListener->m_simulatorVersion = QString("%1.%2.%3.%4").arg(event->dwApplicationVersionMajor).arg(event->dwApplicationVersionMinor).arg(event->dwApplicationBuildMajor).arg(event->dwApplicationBuildMinor);
                    simListener->m_simConnectVersion = QString("%1.%2.%3.%4").arg(event->dwSimConnectVersionMajor).arg(event->dwSimConnectVersionMinor).arg(event->dwSimConnectBuildMajor).arg(event->dwSimConnectBuildMinor);
                    simListener->m_simulatorName = QString(event->szApplicationName);
                    simListener->m_simulatorDetails = QString("Name: '%1' Version: %2 SimConnect: %3").arg(simListener->m_simulatorName, simListener->m_simulatorVersion, simListener->m_simConnectVersion);
                    CLogMessage(static_cast<CSimulatorFsxCommonListener *>(nullptr)).info("Connect to FSX/P3D: '%1'") << simListener->backendInfo();
                    break;
                }
            case SIMCONNECT_RECV_ID_EXCEPTION:
                break;
            default:
                break;
            }
        }
    } // namespace
} // namespace

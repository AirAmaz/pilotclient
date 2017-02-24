/* Copyright (C) 2014
 * swift project community / contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "fs9.h"
#include "directplayerror.h"
#include "simulatorfs9.h"
#include "fs9client.h"
#include "multiplayerpackets.h"
#include "multiplayerpacketparser.h"
#include "registermetadata.h"
#include "blackmisc/simulation/interpolatorlinear.h"
#include "blackmisc/network/textmessage.h"
#include "blackmisc/simulation/simulatorplugininfo.h"
#include "blackmisc/logmessage.h"
#include "blackmisc/propertyindexallclasses.h"
#include "blackmisc/simulation/fscommon/fscommonutil.h"
#include <QTimer>
#include <algorithm>

using namespace BlackMisc;
using namespace BlackMisc::Aviation;
using namespace BlackMisc::Network;
using namespace BlackMisc::PhysicalQuantities;
using namespace BlackMisc::Geo;
using namespace BlackMisc::Simulation;
using namespace BlackMisc::Simulation::FsCommon;
using namespace BlackMisc::Weather;
using namespace BlackSimPlugin::Fs9;
using namespace BlackSimPlugin::FsCommon;

namespace BlackSimPlugin
{
    namespace Fs9
    {
        CAircraftSituation aircraftSituationfromFS9(const MPPositionVelocity &positionVelocity)
        {
            CAircraftSituation situation;

            double dHigh = positionVelocity.lat_i;
            double dLow = positionVelocity.lat_f;

            dLow = dLow / 65536.0;
            if (dHigh > 0)
            {
                dHigh = dHigh + dLow;
            }
            else
            {
                dHigh = dHigh - dLow;
            }

            CCoordinateGeodetic position;
            position.setLatitude(CLatitude(dHigh * 90.0 / 10001750.0, CAngleUnit::deg()));

            dHigh = positionVelocity.lon_hi;
            dLow = positionVelocity.lon_lo;

            dLow = dLow / 65536.0;
            if (dHigh > 0)
            {
                dHigh = dHigh + dLow;
            }
            else
            {
                dHigh = dHigh - dLow;
            }

            position.setLongitude(CLongitude(dHigh * 360.0 / (65536.0 * 65536.0), CAngleUnit::deg()));

            dHigh = positionVelocity.alt_i;
            dLow = positionVelocity.alt_f;

            dLow = dLow / 65536.0;

            situation.setPosition(position);
            situation.setAltitude(CAltitude(dHigh + dLow, CAltitude::MeanSeaLevel, CLengthUnit::m()));
            const double groundSpeed = positionVelocity.ground_velocity / 65536.0;
            situation.setGroundSpeed(CSpeed(groundSpeed, CSpeedUnit::m_s()));

            FS_PBH pbhstrct;
            pbhstrct.pbh = positionVelocity.pbh;
            int pitch = std::floor(pbhstrct.pitch / CFs9Sdk::pitchMultiplier());
            if (pitch < -90 || pitch > 89) { CLogMessage().warning("FS9: Pitch value out of limits: %1") << pitch; }
            int bank = std::floor(pbhstrct.bank / CFs9Sdk::bankMultiplier());

            // MSFS has inverted pitch and bank angles
            pitch = ~pitch;
            bank = ~bank;
            situation.setPitch(CAngle(pitch, CAngleUnit::deg()));
            situation.setBank(CAngle(bank, CAngleUnit::deg()));
            situation.setHeading(CHeading(pbhstrct.hdg / CFs9Sdk::headingMultiplier(), CHeading::Magnetic, CAngleUnit::deg()));

            return situation;
        }

        CSimulatorFs9::CSimulatorFs9(
            const CSimulatorPluginInfo &info,
            const QSharedPointer<CFs9Host> &fs9Host,
            const QSharedPointer<CLobbyClient> &lobbyClient,
            IOwnAircraftProvider *ownAircraftProvider,
            IRemoteAircraftProvider *remoteAircraftProvider,
            IWeatherGridProvider *weatherGridProvider,
            QObject *parent) :
            CSimulatorFsCommon(info, ownAircraftProvider, remoteAircraftProvider, weatherGridProvider, parent),
            m_fs9Host(fs9Host),
            m_lobbyClient(lobbyClient)
        {
            connect(lobbyClient.data(), &CLobbyClient::disconnected, this, std::bind(&CSimulatorFs9::simulatorStatusChanged, this, 0));
            m_defaultModel =
            {
                "Boeing 737-400",
                CAircraftModel::TypeModelMatchingDefaultModel,
                "B737-400 default model",
                CAircraftIcaoCode("B734", "L2J")
            };
        }

        bool CSimulatorFs9::isConnected() const
        {
            return m_simConnected;
        }

        bool CSimulatorFs9::connectTo()
        {
            Q_ASSERT_X(m_fs9Host, Q_FUNC_INFO, "No FS9 host");
            if (!m_fs9Host->isConnected()) { return false; } // host not available, we quit

            Q_ASSERT_X(m_fsuipc,  Q_FUNC_INFO, "No FSUIPC");
            m_connectionHostMessages = connect(m_fs9Host.data(), &CFs9Host::customPacketReceived, this, &CSimulatorFs9::ps_processFs9Message);

            if (m_useFsuipc)
            {
                m_fsuipc->connect(); // connect FSUIPC too
            }
            initSimulatorInternals();
            m_dispatchTimerId = startTimer(50);
            return true;
        }

        bool CSimulatorFs9::disconnectFrom()
        {
            if (!m_simConnected) { return true; }

            // Don't forward messages when disconnected
            disconnect(m_connectionHostMessages);
            killTimer(m_dispatchTimerId);
            m_dispatchTimerId = -1;
            disconnectAllClients();

            //  disconnect FSUIPC and status
            CSimulatorFsCommon::disconnectFrom();
            m_simConnected = false;
            emitSimulatorCombinedStatus();
            return true;
        }

        bool CSimulatorFs9::physicallyAddRemoteAircraft(const CSimulatedAircraft &newRemoteAircraft)
        {
            CCallsign callsign = newRemoteAircraft.getCallsign();
            if (m_hashFs9Clients.contains(callsign))
            {
                // already exists, remove first
                this->physicallyRemoveRemoteAircraft(callsign);
            }

            bool rendered = true;
            updateAircraftRendered(callsign, rendered);
            CFs9Client *client = new CFs9Client(callsign, newRemoteAircraft.getModelString(), CTime(25, CTimeUnit::ms()), &m_interpolationLogger, this);
            client->setHostAddress(m_fs9Host->getHostAddress());
            client->setPlayerUserId(m_fs9Host->getPlayerUserId());
            client->start();

            m_hashFs9Clients.insert(callsign, client);
            bool updated = updateAircraftRendered(callsign, rendered);
            CSimulatedAircraft remoteAircraftCopy(newRemoteAircraft);
            remoteAircraftCopy.setRendered(rendered);
            if (updated)
            {
                emit aircraftRenderingChanged(remoteAircraftCopy);
            }
            CLogMessage(this).info("FS9: Added aircraft %1") << callsign.toQString();
            return true;
        }

        bool CSimulatorFs9::physicallyRemoveRemoteAircraft(const CCallsign &callsign)
        {
            if (!m_hashFs9Clients.contains(callsign)) { return false; }

            auto fs9Client = m_hashFs9Clients.value(callsign);
            fs9Client->quit();
            m_hashFs9Clients.remove(callsign);
            updateAircraftRendered(callsign, false);
            CLogMessage(this).info("FS9: Removed aircraft %1") << callsign.toQString();
            return true;
        }

        int CSimulatorFs9::physicallyRemoveAllRemoteAircraft()
        {
            if (m_hashFs9Clients.isEmpty()) { return 0; }
            QList<CCallsign> callsigns(this->m_hashFs9Clients.keys());
            int r = 0;
            for (const CCallsign &cs : callsigns)
            {
                if (physicallyRemoveRemoteAircraft(cs)) { r++; }
            }
            return r;

        }

        CCallsignSet CSimulatorFs9::physicallyRenderedAircraft() const
        {
            return CCollection<CCallsign>(m_hashFs9Clients.keys());
        }

        bool CSimulatorFs9::updateOwnSimulatorCockpit(const CSimulatedAircraft &ownAircraft, const CIdentifier &originator)
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
                // CFrequency newFreq = newCom1.getFrequencyActive();
                changed = true;

            }
            if (newCom1.getFrequencyStandby() != this->m_simCom1.getFrequencyStandby())
            {
                // CFrequency newFreq = newCom1.getFrequencyStandby();
                changed = true;
            }

            if (newCom2.getFrequencyActive() != this->m_simCom2.getFrequencyActive())
            {
                // CFrequency newFreq = newCom2.getFrequencyActive();
                changed = true;
            }
            if (newCom2.getFrequencyStandby() != this->m_simCom2.getFrequencyStandby())
            {
                // CFrequency newFreq = newCom2.getFrequencyStandby();
                changed = true;
            }

            if (newTransponder.getTransponderCode() != this->m_simTransponder.getTransponderCode())
            {
                changed = true;
            }

            if (newTransponder.getTransponderMode() != this->m_simTransponder.getTransponderMode())
            {
            }

            // avoid changes of cockpit back to old values due to an outdated read back value

            // bye
            return changed;
        }

        void CSimulatorFs9::displayStatusMessage(const BlackMisc::CStatusMessage &message) const
        {
            /* Avoid errors from CDirectPlayPeer as it may end in infinite loop */
            if (message.getSeverity() == BlackMisc::CStatusMessage::SeverityError && message.isFromClass<CDirectPlayPeer>())
            {
                return;
            }

            if (message.getSeverity() != BlackMisc::CStatusMessage::SeverityDebug)
            {
                QMetaObject::invokeMethod(m_fs9Host.data(), "sendTextMessage", Q_ARG(QString, message.toQString()));
            }
        }

        void CSimulatorFs9::displayTextMessage(const BlackMisc::Network::CTextMessage &message) const
        {
            this->displayStatusMessage(message.asStatusMessage(true, true));
        }

        bool CSimulatorFs9::isPhysicallyRenderedAircraft(const CCallsign &callsign) const
        {
            return m_hashFs9Clients.contains(callsign);
        }

        void CSimulatorFs9::timerEvent(QTimerEvent *event)
        {
            Q_UNUSED(event);
            ps_dispatch();
        }

        void CSimulatorFs9::ps_dispatch()
        {
            if (m_useFsuipc && m_fsuipc && m_fsuipc->isConnected())
            {
                CSimulatedAircraft fsuipcAircraft(getOwnAircraft());
                const bool ok = m_fsuipc->read(fsuipcAircraft, true, true, true);
                if (ok)
                {
                    updateOwnAircraftFromSimulator(fsuipcAircraft);
                }
            }
        }

        void CSimulatorFs9::ps_processFs9Message(const QByteArray &message)
        {
            if (!m_simConnected)
            {
                m_simConnected = true;
                emitSimulatorCombinedStatus();
            }
            CFs9Sdk::MULTIPLAYER_PACKET_ID messageType = MultiPlayerPacketParser::readType(message);
            switch (messageType)
            {
            case CFs9Sdk::MULTIPLAYER_PACKET_ID_PARAMS:
                {
                    break;
                }
            case CFs9Sdk::MULTIPLAYER_PACKET_ID_CHANGE_PLAYER_PLANE:
                {
                    MPChangePlayerPlane mpChangePlayerPlane;
                    MultiPlayerPacketParser::readMessage(message, mpChangePlayerPlane);
                    reverseLookupAndUpdateOwnAircraftModel(mpChangePlayerPlane.aircraft_name);
                    break;
                }
            case CFs9Sdk::MULTIPLAYER_PACKET_ID_POSITION_VELOCITY:
                {
                    MPPositionVelocity mpPositionVelocity;
                    MultiPlayerPacketParser::readMessage(message, mpPositionVelocity);
                    auto aircraftSituation = aircraftSituationfromFS9(mpPositionVelocity);
                    updateOwnSituation(aircraftSituation);

                    if (m_isWeatherActivated)
                    {
                        const auto currentPosition = CCoordinateGeodetic { aircraftSituation.latitude(), aircraftSituation.longitude(), {0} };
                        if (CWeatherScenario::isRealWeatherScenario(m_weatherScenarioSettings.get()) &&
                                calculateGreatCircleDistance(m_lastWeatherPosition, currentPosition).value(CLengthUnit::mi()) > 20)
                        {
                            m_lastWeatherPosition = currentPosition;
                            const auto weatherGrid = CWeatherGrid { { "GLOB", currentPosition } };
                            requestWeatherGrid(weatherGrid, { this, &CSimulatorFs9::injectWeatherGrid });
                        }
                    }
                    break;
                }
            case CFs9Sdk::MPCHAT_PACKET_ID_CHAT_TEXT_SEND:
                {
                    MPChatText mpChatText;
                    MultiPlayerPacketParser::readMessage(message, mpChatText);
                    break;
                }

            default:
                break;
            }
        }

        void CSimulatorFs9::updateOwnAircraftFromSimulator(const CSimulatedAircraft &simDataOwnAircraft)
        {
            this->updateCockpit(
                simDataOwnAircraft.getCom1System(),
                simDataOwnAircraft.getCom2System(),
                simDataOwnAircraft.getTransponder(),
                this->identifier());
            reverseLookupAndUpdateOwnAircraftModel(simDataOwnAircraft.getModelString());
        }

        void CSimulatorFs9::disconnectAllClients()
        {
            // Stop all FS9 client tasks
            const QList<CCallsign> callsigns(m_hashFs9Clients.keys());
            for (auto fs9Client : callsigns)
            {
                physicallyRemoveRemoteAircraft(fs9Client);
            }
        }

        void CSimulatorFs9::injectWeatherGrid(const Weather::CWeatherGrid &weatherGrid)
        {
            if (!m_useFsuipc || !m_fsuipc) { return; }
            if (!m_fsuipc->isConnected()) { return; }
            m_fsuipc->write(weatherGrid);
        }

        void CSimulatorFs9::ps_remoteProviderAddAircraftSituation(const CAircraftSituation &situation)
        {
            const auto it = m_hashFs9Clients.find(situation.getCallsign());
            if (it == m_hashFs9Clients.end()) { return; }
            QTimer::singleShot(0, it->data(), [client = *it, situation] { client->getInterpolator()->addAircraftSituation(situation); });
        }

        void CSimulatorFs9::ps_remoteProviderAddAircraftParts(const BlackMisc::Aviation::CCallsign &callsign, const CAircraftParts &parts)
        {
            const auto it = m_hashFs9Clients.find(callsign);
            if (it == m_hashFs9Clients.end()) { return; }
            QTimer::singleShot(0, it->data(), [client = *it, parts] { client->getInterpolator()->addAircraftParts(parts); });
        }

        CSimulatorFs9Listener::CSimulatorFs9Listener(const CSimulatorPluginInfo &info,
                const QSharedPointer<CFs9Host> &fs9Host,
                const QSharedPointer<CLobbyClient> &lobbyClient) :
            BlackCore::ISimulatorListener(info),
            m_timer(new QTimer(this)),
            m_fs9Host(fs9Host),
            m_lobbyClient(lobbyClient)
        {
            const int QueryInterval = 5 * 1000; // 5 seconds
            m_timer->setInterval(QueryInterval);
            m_timer->setObjectName(this->objectName() + ":m_timer");

            // Test whether we can lobby connect at all.
            bool canLobbyConnect = m_lobbyClient->canLobbyConnect();

            connect(m_timer, &QTimer::timeout, [this, canLobbyConnect]()
            {
                if (m_fs9Host->getHostAddress().isEmpty()) { return; } // host not yet set up
                if (canLobbyConnect)
                {
                    if (m_isConnecting || m_lobbyClient->connectFs9ToHost(m_fs9Host->getHostAddress()) == S_OK)
                    {
                        m_isConnecting = true;
                        CLogMessage(this).info("Swift is joining FS9 to the multiplayer session...");
                    }
                }

                if (!m_isStarted && m_fs9Host->isConnected())
                {
                    emit simulatorStarted(getPluginInfo());
                    m_isStarted = true;
                    m_isConnecting = false;
                }
            });
        }

        void CSimulatorFs9Listener::start()
        {
            m_isStarted = false;
            m_timer->start();
        }

        void CSimulatorFs9Listener::stop()
        {
            m_timer->stop();
        }

        static void cleanupFs9Host(CFs9Host *host)
        {
            host->quitAndWait();
        }

        CSimulatorFs9Factory::CSimulatorFs9Factory(QObject *parent) :
            QObject(parent),
            m_fs9Host(new CFs9Host(this), cleanupFs9Host),
            m_lobbyClient(new CLobbyClient(this))
        {
            registerMetadata();

            /* After FS9 is disconnected, reset its data stored in the host */
            connect(m_lobbyClient.data(), &CLobbyClient::disconnected, m_fs9Host.data(), &CFs9Host::reset);

            m_fs9Host->start();
        }

        CSimulatorFs9Factory::~CSimulatorFs9Factory()
        {
        }

        BlackCore::ISimulator *CSimulatorFs9Factory::create(
            const CSimulatorPluginInfo &info,
            IOwnAircraftProvider *ownAircraftProvider,
            IRemoteAircraftProvider *remoteAircraftProvider,
            IWeatherGridProvider *weatherGridProvider)
        {
            return new CSimulatorFs9(info, m_fs9Host, m_lobbyClient, ownAircraftProvider, remoteAircraftProvider, weatherGridProvider, this);
        }

        BlackCore::ISimulatorListener *CSimulatorFs9Factory::createListener(const CSimulatorPluginInfo &info)
        {
            return new CSimulatorFs9Listener(info, m_fs9Host, m_lobbyClient);
        }

    } // namespace
} // namespace

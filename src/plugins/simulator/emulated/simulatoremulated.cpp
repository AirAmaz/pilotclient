/* Copyright (C) 2017
 * swift Project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "simulatoremulated.h"
#include "blackgui/guiapplication.h"
#include "blackcore/context/contextsimulator.h"
#include "blackmisc/simulation/simulatorplugininfo.h"

#include <QApplication>

using namespace BlackGui;
using namespace BlackMisc;
using namespace BlackMisc::Aviation;
using namespace BlackMisc::PhysicalQuantities;
using namespace BlackMisc::Geo;
using namespace BlackMisc::Network;
using namespace BlackMisc::Simulation;
using namespace BlackMisc::Simulation::Settings;
using namespace BlackMisc::Weather;
using namespace BlackCore;
using namespace BlackCore::Context;

namespace BlackSimPlugin
{
    namespace Emulated
    {
        CSimulatorEmulated::CSimulatorEmulated(const CSimulatorPluginInfo &info,
                                               IOwnAircraftProvider *ownAircraftProvider,
                                               IRemoteAircraftProvider *remoteAircraftProvider,
                                               IWeatherGridProvider *weatherGridProvider,
                                               IClientProvider *clientProvider,
                                               QObject *parent) :
            CSimulatorPluginCommon(info, ownAircraftProvider, remoteAircraftProvider, weatherGridProvider, clientProvider, parent)
        {
            Q_ASSERT_X(sApp && sApp->getIContextSimulator(), Q_FUNC_INFO, "Need context");

            CSimulatorEmulated::registerHelp();
            this->onSettingsChanged(); // init from settings

            m_myAircraft = this->getOwnAircraft(); // sync with provider
            m_monitorWidget.reset(new CSimulatorEmulatedMonitorDialog(this, sGui->mainApplicationWidget()));

            connect(qApp, &QApplication::aboutToQuit, this, &CSimulatorEmulated::closeMonitor);
            connect(sGui, &CGuiApplication::aboutToShutdown, this, &CSimulatorEmulated::closeMonitor, Qt::QueuedConnection);
            connect(&m_interpolatorFetchTimer, &QTimer::timeout, this, &CSimulatorEmulated::updateRemoteAircraft);

            // connect own signals for monitoring
            this->connectOwnSignals();
        }

        bool CSimulatorEmulated::isTimeSynchronized() const
        {
            return m_timeSyncronized;
        }

        bool CSimulatorEmulated::connectTo()
        {
            const QPointer<CSimulatorEmulated> myself(this);
            QTimer::singleShot(1000, this, [ = ]
            {
                if (myself.isNull()) { return; }
                this->emitSimulatorCombinedStatus();
                m_monitorWidget->show();
            });

            if (canLog()) { m_monitorWidget->appendReceivingCall(Q_FUNC_INFO); }
            return true;
        }

        bool CSimulatorEmulated::disconnectFrom()
        {
            if (canLog()) { m_monitorWidget->appendReceivingCall(Q_FUNC_INFO); }
            m_renderedAircraft.clear();
            return CSimulatorPluginCommon::disconnectFrom();
        }

        void CSimulatorEmulated::unload()
        {
            if (canLog()) { m_monitorWidget->appendReceivingCall(Q_FUNC_INFO); }
            return CSimulatorPluginCommon::unload();
        }

        bool CSimulatorEmulated::logicallyAddRemoteAircraft(const CSimulatedAircraft &remoteAircraft)
        {
            if (canLog()) { m_monitorWidget->appendReceivingCall(Q_FUNC_INFO, remoteAircraft.toQString()); }
            return ISimulator::logicallyAddRemoteAircraft(remoteAircraft);
        }

        bool CSimulatorEmulated::logicallyRemoveRemoteAircraft(const CCallsign &callsign)
        {
            if (canLog()) { m_monitorWidget->appendReceivingCall(Q_FUNC_INFO, callsign.toQString()); }
            return ISimulator::logicallyRemoveRemoteAircraft(callsign);
        }

        int CSimulatorEmulated::physicallyRemoveMultipleRemoteAircraft(const CCallsignSet &callsigns)
        {
            if (canLog()) m_monitorWidget->appendReceivingCall(Q_FUNC_INFO, callsigns.toQString());
            return ISimulator::physicallyRemoveMultipleRemoteAircraft(callsigns);
        }

        bool CSimulatorEmulated::changeRemoteAircraftModel(const CSimulatedAircraft &aircraft)
        {
            if (canLog()) { m_monitorWidget->appendReceivingCall(Q_FUNC_INFO, aircraft.toQString()); }
            return ISimulator::changeRemoteAircraftEnabled(aircraft);
        }

        bool CSimulatorEmulated::changeRemoteAircraftEnabled(const CSimulatedAircraft &aircraft)
        {
            if (canLog()) { m_monitorWidget->appendReceivingCall(Q_FUNC_INFO, aircraft.toQString()); }
            return ISimulator::changeRemoteAircraftEnabled(aircraft);
        }

        bool CSimulatorEmulated::updateOwnSimulatorCockpit(const CSimulatedAircraft &aircraft, const CIdentifier &originator)
        {
            if (canLog()) { m_monitorWidget->appendReceivingCall(Q_FUNC_INFO, aircraft.toQString(), originator.toQString()); }
            if (originator == identifier()) { return false; } // myself
            m_myAircraft.setCockpit(aircraft);
            emit this->internalAircraftChanged();
            return true;
        }

        bool CSimulatorEmulated::updateOwnSimulatorSelcal(const CSelcal &selcal, const CIdentifier &originator)
        {
            if (canLog()) { m_monitorWidget->appendReceivingCall(Q_FUNC_INFO, selcal.toQString(), originator.toQString()); }
            if (originator == identifier()) { return false; } // myself
            if (m_myAircraft.getSelcal() == selcal) { return false; }
            m_myAircraft.setSelcal(selcal);
            emit this->internalAircraftChanged();
            return true;
        }

        void CSimulatorEmulated::displayStatusMessage(const CStatusMessage &message) const
        {
            if (canLog()) { m_monitorWidget->appendReceivingCall(Q_FUNC_INFO, message.toQString()); }
            m_monitorWidget->displayStatusMessage(message);
        }

        void CSimulatorEmulated::displayTextMessage(const CTextMessage &message) const
        {
            if (canLog()) m_monitorWidget->appendReceivingCall(Q_FUNC_INFO, message.toQString());
            m_monitorWidget->displayTextMessage(message);
        }

        bool CSimulatorEmulated::setTimeSynchronization(bool enable, const CTime &offset)
        {
            if (canLog()) { m_monitorWidget->appendReceivingCall(Q_FUNC_INFO, boolToTrueFalse(enable), offset.toQString()); }
            m_timeSyncronized = enable;
            m_offsetTime = offset;
            return enable;
        }

        CTime CSimulatorEmulated::getTimeSynchronizationOffset() const
        {
            if (canLog()) { m_monitorWidget->appendReceivingCall(Q_FUNC_INFO); }
            return m_offsetTime;
        }

        bool CSimulatorEmulated::isPhysicallyRenderedAircraft(const CCallsign &callsign) const
        {
            if (canLog()) m_monitorWidget->appendReceivingCall(Q_FUNC_INFO, callsign.toQString());
            return m_renderedAircraft.containsCallsign(callsign);
        }

        CCallsignSet CSimulatorEmulated::physicallyRenderedAircraft() const
        {
            if (canLog()) { m_monitorWidget->appendReceivingCall(Q_FUNC_INFO); }
            return m_renderedAircraft.getCallsigns();
        }

        CStatusMessageList CSimulatorEmulated::getInterpolationMessages(const CCallsign &callsign) const
        {
            if (canLog()) { m_monitorWidget->appendReceivingCall(Q_FUNC_INFO); }
            if (!m_interpolators.contains(callsign)) { return CStatusMessageList(); }
            const CInterpolationAndRenderingSetupPerCallsign setup = this->getInterpolationSetupPerCallsignOrDefault(callsign); // threadsafe copy
            return m_interpolators[callsign]->getInterpolationMessages(setup.getInterpolatorMode());
        }

        void CSimulatorEmulated::highlightAircraft(const CSimulatedAircraft &aircraftToHighlight, bool enableHighlight, const CTime &displayTime)
        {
            if (canLog()) { m_monitorWidget->appendReceivingCall(Q_FUNC_INFO, aircraftToHighlight.toQString(), boolToTrueFalse(enableHighlight), displayTime.toQString()); }
            ISimulator::highlightAircraft(aircraftToHighlight, enableHighlight, displayTime);
        }

        bool CSimulatorEmulated::parseCommandLine(const QString &commandLine, const CIdentifier &originator)
        {
            if (canLog()) { m_monitorWidget->appendReceivingCall(Q_FUNC_INFO, commandLine, originator.toQString()); }
            return CSimulatorPluginCommon::parseCommandLine(commandLine, originator);
        }

        void CSimulatorEmulated::registerHelp()
        {
            if (CSimpleCommandParser::registered("BlackSimPlugin::Swift::CSimulatorEmulated")) { return; }
            CSimpleCommandParser::registerCommand({".drv", "alias: .driver .plugin"});
            CSimpleCommandParser::registerCommand({".drv show", "show emulated driver window"});
            CSimpleCommandParser::registerCommand({".drv hide", "hide emulated driver window"});
        }

        void CSimulatorEmulated::setCombinedStatus(bool connected, bool simulating, bool paused)
        {
            m_connected = connected;
            m_simulating = simulating;
            m_paused = paused;
            this->emitSimulatorCombinedStatus();
        }

        bool CSimulatorEmulated::changeInternalCom(const CSimulatedAircraft &aircraft)
        {
            bool changed = false;
            if (aircraft.getCom1System() != m_myAircraft.getCom1System()) { changed = true; }
            if (aircraft.getCom2System() != m_myAircraft.getCom2System()) { changed = true; }
            if (aircraft.getTransponder() != m_myAircraft.getTransponder()) { changed = true; }
            if (aircraft.getSelcal() != m_myAircraft.getSelcal()) { changed = true; }

            if (!changed) { return false; }
            m_myAircraft.setCockpit(aircraft);
            return this->updateCockpit(aircraft, this->identifier());
        }

        bool CSimulatorEmulated::changeInternalSelcal(const CSelcal &selcal)
        {
            if (m_myAircraft.getSelcal() == selcal) { return false; }
            m_myAircraft.setSelcal(selcal);
            return this->updateSelcal(selcal, identifier());
        }

        bool CSimulatorEmulated::changeInternalSituation(const CAircraftSituation &situation)
        {
            if (m_myAircraft.getSituation() == situation) { return false; }
            m_myAircraft.setSituation(situation);

            if (m_isWeatherActivated)
            {
                const auto currentPosition = CCoordinateGeodetic { situation.latitude(), situation.longitude() };
                if (CWeatherScenario::isRealWeatherScenario(m_weatherScenarioSettings.get()))
                {
                    if (m_lastWeatherPosition.isNull() ||
                            calculateGreatCircleDistance(m_lastWeatherPosition, currentPosition).value(CLengthUnit::mi()) > 20)
                    {
                        m_lastWeatherPosition = currentPosition;
                        const auto weatherGrid = CWeatherGrid { { "GLOB", currentPosition } };
                        requestWeatherGrid(weatherGrid, { this, &CSimulatorEmulated::injectWeatherGrid });
                    }
                }
            }

            return this->updateOwnSituation(situation);
        }

        bool CSimulatorEmulated::changeInternalParts(const CAircraftParts &parts)
        {
            if (m_myAircraft.getParts() == parts) { return false; }
            m_myAircraft.setParts(parts);
            return this->updateOwnParts(parts);
        }

        bool CSimulatorEmulated::setInterpolatorFetchTime(int timeMs)
        {
            if (timeMs < 1)
            {
                m_interpolatorFetchTimer.stop();
            }
            else
            {
                m_interpolatorFetchTimer.start(timeMs);
            }
            return m_interpolatorFetchTimer.isActive();
        }

        bool CSimulatorEmulated::isInterpolatorFetching() const
        {
            return m_interpolatorFetchTimer.isActive();
        }

        bool CSimulatorEmulated::isConnected() const
        {
            if (canLog()) m_monitorWidget->appendReceivingCall(Q_FUNC_INFO);
            return m_connected;
        }

        bool CSimulatorEmulated::isPaused() const
        {
            if (canLog()) { m_monitorWidget->appendReceivingCall(Q_FUNC_INFO); }
            return m_paused;
        }

        bool CSimulatorEmulated::isSimulating() const
        {
            if (canLog()) { m_monitorWidget->appendReceivingCall(Q_FUNC_INFO); }
            return m_simulating;
        }

        bool CSimulatorEmulated::physicallyAddRemoteAircraft(const CSimulatedAircraft &remoteAircraft)
        {
            if (canLog()) { m_monitorWidget->appendReceivingCall(Q_FUNC_INFO, remoteAircraft.toQString()); }
            CSimulatedAircraft aircraft(remoteAircraft);
            aircraft.setRendered(true);
            const CCallsign cs = aircraft.getCallsign();
            m_interpolators.insert(cs, CInterpolatorMultiWrapper(cs, this, this, this->getRemoteAircraftProvider(), &m_interpolationLogger));
            m_renderedAircraft.push_back(aircraft); // my simulator list
            this->updateAircraftRendered(cs, true); // in provider
            emit this->aircraftRenderingChanged(aircraft);
            return true;
        }

        bool CSimulatorEmulated::physicallyRemoveRemoteAircraft(const CCallsign &callsign)
        {
            if (canLog()) { m_monitorWidget->appendReceivingCall(Q_FUNC_INFO, callsign.toQString()); }
            m_interpolators.remove(callsign);
            const int c = m_renderedAircraft.removeByCallsign(callsign);
            return c > 0;
        }

        int CSimulatorEmulated::physicallyRemoveAllRemoteAircraft()
        {
            if (canLog()) { m_monitorWidget->appendReceivingCall(Q_FUNC_INFO); }
            return ISimulator::physicallyRemoveAllRemoteAircraft();
        }

        bool CSimulatorEmulated::parseDetails(const CSimpleCommandParser &parser)
        {
            if (m_monitorWidget && parser.isKnownCommand())
            {
                if (parser.matchesPart(1, "show")) { m_monitorWidget->show(); return true; }
                if (parser.matchesPart(1, "hide")) { m_monitorWidget->hide(); return true; }
            }
            return CSimulatorPluginCommon::parseDetails(parser);
        }

        void CSimulatorEmulated::setObjectName(const CSimulatorInfo &info)
        {
            QObject::setObjectName("Emulated driver for " + info.toQString());
            m_interpolatorFetchTimer.setObjectName(this->objectName() + ":interpolatorFetchTimer");
        }

        bool CSimulatorEmulated::canLog() const
        {
            return sApp && !sApp->isShuttingDown() && m_log && m_monitorWidget;
        }

        void CSimulatorEmulated::closeMonitor()
        {
            if (m_monitorWidget)
            {
                m_monitorWidget->close();
            }
        }

        void CSimulatorEmulated::onSettingsChanged()
        {
            const CSwiftPluginSettings settings(m_pluginSettings.get());
            m_log = settings.isLoggingFunctionCalls();

            const CSimulatorInfo simulator = settings.getEmulatedSimulator();
            const CSimulatorPluginInfoList plugins = sApp->getIContextSimulator()->getAvailableSimulatorPlugins();
            const CSimulatorPluginInfo plugin = plugins.findBySimulator(simulator);

            Q_ASSERT_X(simulator.isSingleSimulator(), Q_FUNC_INFO, "need single simulator");
            if (plugin.isValid())
            {
                // ? restart driver, disconnect/reconnect
                this->setNewPluginInfo(plugin, settings.getDefaultModel());
            }
            else
            {
                CLogMessage(this).validationError("No valid plugin for '%1'") << simulator.toQString();
            }

            // update provider, own name, title
            this->updateOwnModel(settings.getOwnModel());
            this->setObjectName(simulator);
            if (m_monitorWidget) { m_monitorWidget->updateWindowTitle(simulator); }
        }

        void CSimulatorEmulated::connectOwnSignals()
        {
            //! \fixme signal name not hardcoded would be nice
            m_connectionGuard.append(connect(this, &ISimulator::simulatorStatusChanged, this, [ = ](SimulatorStatus status)
            {
                if (!m_monitorWidget) return;
                m_monitorWidget->appendSendingCall("simulatorStatusChanged", CSimulatorEmulated::statusToString(status));
            },
            Qt::QueuedConnection));

            m_connectionGuard.append(connect(this, &ISimulator::ownAircraftModelChanged, this, [ = ](const CAircraftModel &model)
            {
                if (!m_monitorWidget) return;
                m_monitorWidget->appendSendingCall("ownAircraftModelChanged", model.toQString());
            },
            Qt::QueuedConnection));

            m_connectionGuard.append(connect(this, &ISimulator::renderRestrictionsChanged, this, [ = ](bool restricted, bool enabled, int maxAircraft, const CLength & maxRenderedDistance)
            {
                if (!m_monitorWidget) return;
                static const QString params("restricted: %1 enabled: %2 max aircraft: %3");
                m_monitorWidget->appendSendingCall("renderRestrictionsChanged", params.arg(boolToYesNo(restricted), boolToYesNo(enabled)).arg(maxAircraft), maxRenderedDistance.valueRoundedWithUnit(CLengthUnit::m(), 1));
            },
            Qt::QueuedConnection));

            m_connectionGuard.append(connect(this, &ISimulator::interpolationAndRenderingSetupChanged, this, [ = ]()
            {
                if (!m_monitorWidget) return;
                m_monitorWidget->appendSendingCall("interpolationAndRenderingSetupChanged");
            },
            Qt::QueuedConnection));

            m_connectionGuard.append(connect(this, &ISimulator::aircraftRenderingChanged, this, [ = ](const CSimulatedAircraft & aircraft)
            {
                if (!m_monitorWidget) return;
                m_monitorWidget->appendSendingCall("aircraftRenderingChanged", aircraft.toQString());
            },
            Qt::QueuedConnection));

            m_connectionGuard.append(connect(this, &ISimulator::physicallyAddingRemoteModelFailed, this, [ = ](const CSimulatedAircraft & aircraft)
            {
                if (!m_monitorWidget) return;
                m_monitorWidget->appendSendingCall("physicallyAddingRemoteModelFailed", aircraft.toQString());
            },
            Qt::QueuedConnection));

            m_connectionGuard.append(connect(this, &ISimulator::airspaceSnapshotHandled, this, [ = ]
            {
                if (!m_monitorWidget) return;
                m_monitorWidget->appendSendingCall("airspaceSnapshotHandled");
            },
            Qt::QueuedConnection));
        }

        void CSimulatorEmulated::updateRemoteAircraft()
        {
            const qint64 now = QDateTime::currentMSecsSinceEpoch();
            const bool updateAllAircraft = this->isUpdateAllRemoteAircraft(now);
            int aircraftNumber = 0;

            for (const CSimulatedAircraft &aircraft : m_renderedAircraft)
            {
                const CCallsign callsign = aircraft.getCallsign();
                if (!m_interpolators.contains(callsign)) { continue; }
                const CInterpolationAndRenderingSetupPerCallsign setup = this->getInterpolationSetupConsolidated(callsign, updateAllAircraft);
                CInterpolatorMulti *im = m_interpolators[callsign];
                Q_ASSERT_X(im, Q_FUNC_INFO, "interpolator missing");
                const CInterpolationResult result = im->getInterpolation(now, setup, aircraftNumber++);
                const CAircraftSituation s = result;
                const CAircraftParts p = result;
                m_countInterpolatedParts++;
                m_countInterpolatedSituations++;
                Q_UNUSED(s);
                Q_UNUSED(p);
            }

            this->finishUpdateRemoteAircraftAndSetStatistics(now);
        }

        CSimulatorEmulatedListener::CSimulatorEmulatedListener(const CSimulatorPluginInfo &info)
            : ISimulatorListener(info)
        { }

        void CSimulatorEmulatedListener::startImpl()
        {
            if (this->isShuttingDown()) { return; }
            const QPointer<CSimulatorEmulatedListener> myself(this);
            QTimer::singleShot(2000, this, [ = ]
            {
                if (!myself) { return; }
                Q_ASSERT_X(this->getPluginInfo().isValid(), Q_FUNC_INFO, "Invalid plugin");
                emit this->simulatorStarted(this->getPluginInfo());
            });
        }

        void CSimulatorEmulatedListener::stopImpl()
        { }

        void CSimulatorEmulatedListener::checkImpl()
        {
            this->startImpl();
        }
    } // ns
} // ns

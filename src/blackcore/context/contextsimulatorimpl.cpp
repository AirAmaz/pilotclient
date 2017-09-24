/* Copyright (C) 2013
 * swift Project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "blackcore/context/contextapplication.h"
#include "blackcore/context/contextnetwork.h"
#include "blackcore/context/contextnetworkimpl.h"
#include "blackcore/context/contextownaircraft.h"
#include "blackcore/context/contextownaircraftimpl.h"
#include "blackcore/context/contextsimulatorimpl.h"
#include "blackcore/corefacade.h"
#include "blackcore/application.h"
#include "blackcore/pluginmanagersimulator.h"
#include "blackcore/simulator.h"
#include "blackmisc/simulation/matchingutils.h"
#include "blackmisc/aviation/callsign.h"
#include "blackmisc/compare.h"
#include "blackmisc/dbusserver.h"
#include "blackmisc/simplecommandparser.h"
#include "blackmisc/logcategory.h"
#include "blackmisc/loghandler.h"
#include "blackmisc/logmessage.h"
#include "blackmisc/pq/units.h"
#include "blackmisc/simulation/simulatedaircraft.h"
#include "blackmisc/statusmessage.h"
#include "blackmisc/threadutils.h"
#include "blackmisc/verify.h"

#include <QMetaObject>
#include <QStringList>
#include <QThread>
#include <Qt>
#include <QtGlobal>

using namespace BlackMisc;
using namespace BlackMisc::PhysicalQuantities;
using namespace BlackMisc::Aviation;
using namespace BlackMisc::Network;
using namespace BlackMisc::Simulation;
using namespace BlackMisc::Geo;
using namespace BlackMisc::Simulation;

namespace BlackCore
{
    namespace Context
    {
        CContextSimulator::CContextSimulator(CCoreFacadeConfig::ContextMode mode, CCoreFacade *runtime) :
            IContextSimulator(mode, runtime),
            CIdentifiable(this),
            m_plugins(new CPluginManagerSimulator(this))
        {
            setObjectName("CContextSimulator");
            m_enableMatchingMessages = sApp->isRunningInDeveloperEnvironment();
            connect(&m_weatherManager, &CWeatherManager::weatherGridReceived, this, &CContextSimulator::weatherGridReceived);
            m_plugins->collectPlugins();
            restoreSimulatorPlugins();

            connect(&m_modelSetLoader, &CAircraftModelSetLoader::simulatorChanged, this, &CContextSimulator::ps_modelSetChanged);
            connect(&m_modelSetLoader, &CAircraftModelSetLoader::cacheChanged, this, &CContextSimulator::ps_modelSetChanged);

            // deferred init of last model set, if no other data are set in meantime
            QTimer::singleShot(1250, this, &CContextSimulator::initByLastUsedModelSet);
        }

        CContextSimulator *CContextSimulator::registerWithDBus(CDBusServer *server)
        {
            if (!server || this->m_mode != CCoreFacadeConfig::LocalInDbusServer) return this;
            server->addObject(CContextSimulator::ObjectPath(), this);
            return this;
        }

        CContextSimulator::~CContextSimulator()
        {
            this->gracefulShutdown();
        }

        void CContextSimulator::gracefulShutdown()
        {
            this->disconnect();
            this->unloadSimulatorPlugin();
        }

        CSimulatorPluginInfoList CContextSimulator::getAvailableSimulatorPlugins() const
        {
            return m_plugins->getAvailableSimulatorPlugins();
        }

        bool CContextSimulator::startSimulatorPlugin(const CSimulatorPluginInfo &simulatorInfo)
        {
            return this->listenForSimulator(simulatorInfo);
        }

        void CContextSimulator::stopSimulatorPlugin(const CSimulatorPluginInfo &simulatorInfo)
        {
            if (!m_simulatorPlugin.first.isUnspecified() && m_simulatorPlugin.first == simulatorInfo)
            {
                this->unloadSimulatorPlugin();
            }

            ISimulatorListener *listener = m_plugins->getListener(simulatorInfo.getIdentifier());
            Q_ASSERT(listener);
            QMetaObject::invokeMethod(listener, "stop");
        }

        int CContextSimulator::getSimulatorStatus() const
        {
            if (m_debugEnabled) { CLogMessage(this, CLogCategory::contextSlot()).debug() << Q_FUNC_INFO; }
            if (m_simulatorPlugin.first.isUnspecified()) { return 0; }

            Q_ASSERT_X(m_simulatorPlugin.second, Q_FUNC_INFO, "Missing simulator");
            return m_simulatorPlugin.second->getSimulatorStatus();
        }

        BlackMisc::Simulation::CSimulatorPluginInfo CContextSimulator::getSimulatorPluginInfo() const
        {
            if (m_debugEnabled) { CLogMessage(this, CLogCategory::contextSlot()).debug() << Q_FUNC_INFO; }
            if (m_simulatorPlugin.first.isUnspecified()) { return BlackMisc::Simulation::CSimulatorPluginInfo(); }

            Q_ASSERT(m_simulatorPlugin.second);
            return m_simulatorPlugin.first;
        }

        CSimulatorInternals CContextSimulator::getSimulatorInternals() const
        {
            if (m_debugEnabled) { CLogMessage(this, CLogCategory::contextSlot()).debug() << Q_FUNC_INFO; }
            if (m_simulatorPlugin.first.isUnspecified())
            {
                return BlackMisc::Simulation::CSimulatorInternals();
            }

            Q_ASSERT(m_simulatorPlugin.second);
            return m_simulatorPlugin.second->getSimulatorInternals();
        }

        CAirportList CContextSimulator::getAirportsInRange() const
        {
            if (m_debugEnabled) { CLogMessage(this, CLogCategory::contextSlot()).debug() << Q_FUNC_INFO; }
            // If no ISimulator object is available, return a dummy.
            if (m_simulatorPlugin.first.isUnspecified())
            {
                return CAirportList();
            }

            Q_ASSERT(m_simulatorPlugin.second);
            return m_simulatorPlugin.second->getAirportsInRange();
        }

        CAircraftModelList CContextSimulator::getModelSet() const
        {
            if (m_debugEnabled) { CLogMessage(this, CLogCategory::contextSlot()).debug() << Q_FUNC_INFO; }
            return m_aircraftMatcher.getModelSet();
        }

        CSimulatorInfo CContextSimulator::simulatorsWithInitializedModelSet() const
        {
            if (m_debugEnabled) { CLogMessage(this, CLogCategory::contextSlot()).debug() << Q_FUNC_INFO; }
            return m_modelSetLoader.simulatorsWithInitializedModelSet();
        }

        QStringList CContextSimulator::getModelSetStrings() const
        {
            if (m_debugEnabled) { CLogMessage(this, CLogCategory::contextSlot()).debug() << Q_FUNC_INFO; }
            return this->getModelSet().getModelStringList(false);
        }

        QStringList CContextSimulator::getModelSetCompleterStrings(bool sorted) const
        {
            if (m_debugEnabled) { CLogMessage(this, CLogCategory::contextSlot()).debug() << Q_FUNC_INFO << sorted; }
            return this->getModelSet().toCompleterStrings(sorted);
        }

        int CContextSimulator::getModelSetCount() const
        {
            if (m_debugEnabled) { CLogMessage(this, CLogCategory::contextSlot()).debug() << Q_FUNC_INFO; }
            if (m_simulatorPlugin.first.isUnspecified()) { return 0; }

            Q_ASSERT(m_simulatorPlugin.second);
            return getModelSet().size();
        }

        CAircraftModelList CContextSimulator::getModelSetModelsStartingWith(const QString &modelString) const
        {
            if (m_debugEnabled) { CLogMessage(this, CLogCategory::contextSlot()).debug() << Q_FUNC_INFO << modelString; }
            if (m_simulatorPlugin.first.isUnspecified())
            {
                return CAircraftModelList();
            }

            Q_ASSERT(m_simulatorPlugin.second);
            return getModelSet().findModelsStartingWith(modelString);
        }

        bool CContextSimulator::setTimeSynchronization(bool enable, const CTime &offset)
        {
            if (m_debugEnabled) { CLogMessage(this, CLogCategory::contextSlot()).debug() << Q_FUNC_INFO; }
            if (m_simulatorPlugin.first.isUnspecified()) { return false; }

            Q_ASSERT(m_simulatorPlugin.second);
            bool c = m_simulatorPlugin.second->setTimeSynchronization(enable, offset);
            if (!c) { return false; }

            CLogMessage(this).info(enable ? QStringLiteral("Set time syncronization to %1").arg(offset.toQString()) : QStringLiteral("Disabled time syncrhonization"));
            return true;
        }

        bool CContextSimulator::isTimeSynchronized() const
        {
            if (m_debugEnabled) { CLogMessage(this, CLogCategory::contextSlot()).debug() << Q_FUNC_INFO; }
            if (m_simulatorPlugin.first.isUnspecified()) { return false; }

            Q_ASSERT(m_simulatorPlugin.second);
            return m_simulatorPlugin.second->isTimeSynchronized();
        }

        CInterpolationAndRenderingSetup CContextSimulator::getInterpolationAndRenderingSetup() const
        {
            if (m_debugEnabled) { CLogMessage(this, CLogCategory::contextSlot()).debug() << Q_FUNC_INFO; }
            if (m_simulatorPlugin.first.isUnspecified()) { return CInterpolationAndRenderingSetup(); }
            Q_ASSERT(m_simulatorPlugin.second);
            return m_simulatorPlugin.second->getInterpolationAndRenderingSetup();
        }

        void CContextSimulator::setInterpolationAndRenderingSetup(const CInterpolationAndRenderingSetup &setup)
        {
            if (m_debugEnabled) { CLogMessage(this, CLogCategory::contextSlot()).debug() << Q_FUNC_INFO << setup; }
            if (m_simulatorPlugin.first.isUnspecified()) { return; }
            Q_ASSERT(m_simulatorPlugin.second);
            m_simulatorPlugin.second->setInterpolationAndRenderingSetup(setup);
        }

        CTime CContextSimulator::getTimeSynchronizationOffset() const
        {
            if (m_debugEnabled) { CLogMessage(this, CLogCategory::contextSlot()).debug() << Q_FUNC_INFO; }
            if (m_simulatorPlugin.first.isUnspecified()) { return CTime(0, CTimeUnit::hrmin()); }
            Q_ASSERT(m_simulatorPlugin.second);
            return m_simulatorPlugin.second->getTimeSynchronizationOffset();
        }

        bool CContextSimulator::loadSimulatorPlugin(const CSimulatorPluginInfo &simulatorPluginInfo)
        {
            Q_ASSERT(getIContextApplication());
            Q_ASSERT(getIContextApplication()->isUsingImplementingObject());
            Q_ASSERT(!simulatorPluginInfo.isUnspecified());
            Q_ASSERT(CThreadUtils::isCurrentThreadApplicationThread()); // only run in main thread

            // Is a plugin already loaded?
            if (!m_simulatorPlugin.first.isUnspecified())
            {
                // This can happen, if a listener emitted simulatorStarted twice or two different simulators
                // are running at the same time. In this case, we leave the loaded plugin and just return.
                return false;
            }

            if (!simulatorPluginInfo.isValid())
            {
                CLogMessage(this).error("Illegal plugin");
                return false;
            }

            ISimulatorFactory *factory = m_plugins->getFactory(simulatorPluginInfo.getIdentifier());
            Q_ASSERT_X(factory, Q_FUNC_INFO, "no factory");

            // We assume we run in the same process as the own aircraft context
            // Hence we pass in memory reference to own aircraft object
            Q_ASSERT(this->getIContextOwnAircraft()->isUsingImplementingObject());
            Q_ASSERT(this->getIContextNetwork()->isUsingImplementingObject());
            IOwnAircraftProvider *ownAircraftProvider = this->getRuntime()->getCContextOwnAircraft();
            IRemoteAircraftProvider *renderedAircraftProvider = this->getRuntime()->getCContextNetwork();
            ISimulator *simulator = factory->create(simulatorPluginInfo, ownAircraftProvider, renderedAircraftProvider, &m_weatherManager);
            Q_ASSERT_X(simulator, Q_FUNC_INFO, "no simulator driver can be created");

            setRemoteAircraftProvider(renderedAircraftProvider);
            const CSimulatorInfo simInfo(simulatorPluginInfo.getIdentifier());
            m_modelSetLoader.changeSimulator(simInfo);
            m_aircraftMatcher.setModelSet(m_modelSetLoader.getAircraftModels(), simInfo);
            m_aircraftMatcher.setDefaultModel(simulator->getDefaultModel());

            bool c = connect(simulator, &ISimulator::simulatorStatusChanged, this, &CContextSimulator::ps_onSimulatorStatusChanged);
            Q_ASSERT(c);
            c = connect(simulator, &ISimulator::physicallyAddingRemoteModelFailed, this, &CContextSimulator::ps_addingRemoteAircraftFailed);
            Q_ASSERT(c);
            c = connect(simulator, &ISimulator::ownAircraftModelChanged, this, &IContextSimulator::ownAircraftModelChanged);
            Q_ASSERT(c);
            c = connect(simulator, &ISimulator::aircraftRenderingChanged, this, &IContextSimulator::aircraftRenderingChanged);
            Q_ASSERT(c);
            c = connect(simulator, &ISimulator::renderRestrictionsChanged, this, &IContextSimulator::renderRestrictionsChanged);
            Q_ASSERT(c);
            c = connect(simulator, &ISimulator::airspaceSnapshotHandled, this, &IContextSimulator::airspaceSnapshotHandled);
            Q_ASSERT(c);

            // log from context to simulator
            c = connect(CLogHandler::instance(), &CLogHandler::localMessageLogged, this, &CContextSimulator::ps_relayStatusMessageToSimulator);
            Q_ASSERT(c);
            c = connect(CLogHandler::instance(), &CLogHandler::remoteMessageLogged, this, &CContextSimulator::ps_relayStatusMessageToSimulator);
            Q_ASSERT(c);
            Q_UNUSED(c);

            // Once the simulator signaled it is ready to simulate, add all known aircraft
            m_initallyAddAircrafts = true;
            m_matchingMessages.clear();

            // try to connect to simulator
            const bool connected = simulator->connectTo();
            simulator->setWeatherActivated(m_isWeatherActivated);

            // when everything is set up connected, update the current plugin info
            m_simulatorPlugin.first = simulatorPluginInfo;
            m_simulatorPlugin.second = simulator;

            //! \fixme KB 7/2017 wonder if it was better to force an Qt::QueuedConnection emit by calling CTimer::singleShot
            //! Replace this comment by an info comment after review, or change
            emit simulatorPluginChanged(simulatorPluginInfo);
            CLogMessage(this).info("Simulator plugin loaded: '%1' connected: %2")
                    << simulatorPluginInfo.toQString(true)
                    << boolToYesNo(connected);

            return true;
        }

        bool CContextSimulator::listenForSimulator(const CSimulatorPluginInfo &simulatorInfo)
        {
            Q_ASSERT(this->getIContextApplication());
            Q_ASSERT(this->getIContextApplication()->isUsingImplementingObject());
            Q_ASSERT(!simulatorInfo.isUnspecified());
            Q_ASSERT(m_simulatorPlugin.first.isUnspecified());

            if (!m_listenersThread.isRunning())
            {
                m_listenersThread.setObjectName("CContextSimulator: Thread for listener " + simulatorInfo.getIdentifier());
                m_listenersThread.start(QThread::LowPriority);
            }

            ISimulatorListener *listener = m_plugins->createListener(simulatorInfo.getIdentifier());
            if (!listener) { return false; }

            if (listener->thread() != &m_listenersThread)
            {
                Q_ASSERT_X(!listener->parent(), Q_FUNC_INFO, "Objects with parent cannot be moved to thread");

                const bool c = connect(listener, &ISimulatorListener::simulatorStarted, this, &CContextSimulator::ps_simulatorStarted);
                if (!c)
                {
                    CLogMessage(this).error("Unable to use '%1'") << simulatorInfo.toQString();
                    return false;
                }
                listener->setProperty("isInitialized", true);
                listener->moveToThread(&m_listenersThread);
            }

            const bool s = QMetaObject::invokeMethod(listener, "start", Qt::QueuedConnection);
            Q_ASSERT_X(s, Q_FUNC_INFO, "cannot invoke method");
            Q_UNUSED(s);

            CLogMessage(this).info("Listening for simulator '%1'") << simulatorInfo.getIdentifier();
            return true;
        }

        void CContextSimulator::listenForAllSimulators()
        {
            const auto plugins = getAvailableSimulatorPlugins();
            for (const CSimulatorPluginInfo &p : plugins)
            {
                Q_ASSERT(!p.isUnspecified());

                if (p.isValid())
                {
                    listenForSimulator(p);
                }
            }
        }

        void CContextSimulator::unloadSimulatorPlugin()
        {
            if (!m_simulatorPlugin.first.isUnspecified())
            {
                ISimulator *sim = m_simulatorPlugin.second;
                m_simulatorPlugin.second = nullptr;
                m_simulatorPlugin.first = CSimulatorPluginInfo();

                Q_ASSERT(this->getIContextNetwork());
                Q_ASSERT(this->getIContextNetwork()->isLocalObject());

                // unload and disconnect
                if (sim)
                {
                    // disconnect signals and delete
                    sim->disconnect(this);
                    sim->unload();
                    sim->deleteLater();
                    emit simulatorPluginChanged(CSimulatorPluginInfo());
                }
            }
        }

        void CContextSimulator::ps_addedRemoteAircraft(const CSimulatedAircraft &remoteAircraft)
        {
            if (!isSimulatorSimulating()) { return; }
            const CCallsign callsign = remoteAircraft.getCallsign();
            BLACK_VERIFY_X(!callsign.isEmpty(), Q_FUNC_INFO, "Remote aircraft with empty callsign");
            if (callsign.isEmpty()) { return; }

            CStatusMessageList matchingMessages;
            CStatusMessageList *pMatchingMessages = m_enableMatchingMessages ? &matchingMessages : nullptr;
            const CAircraftModel aircraftModel = m_aircraftMatcher.getClosestMatch(remoteAircraft, pMatchingMessages);
            Q_ASSERT_X(remoteAircraft.getCallsign() == aircraftModel.getCallsign(), Q_FUNC_INFO, "Mismatching callsigns");
            updateAircraftModel(callsign, aircraftModel, identifier());
            const CSimulatedAircraft aircraftAfterModelApplied = getAircraftInRangeForCallsign(remoteAircraft.getCallsign());
            m_simulatorPlugin.second->logicallyAddRemoteAircraft(aircraftAfterModelApplied);
            CMatchingUtils::addLogDetailsToList(pMatchingMessages, callsign, QString("Logically added remote aircraft: %1").arg(aircraftAfterModelApplied.toQString()));
            addMatchingMessages(callsign, matchingMessages);
            emit modelMatchingCompleted(remoteAircraft);
        }

        void CContextSimulator::ps_removedRemoteAircraft(const CCallsign &callsign)
        {
            if (!isSimulatorSimulating()) { return; }
            m_simulatorPlugin.second->logicallyRemoveRemoteAircraft(callsign);
        }

        void CContextSimulator::ps_onSimulatorStatusChanged(int status)
        {
            ISimulator::SimulatorStatus statusEnum = ISimulator::statusToEnum(status);
            if (m_initallyAddAircrafts && statusEnum.testFlag(ISimulator::Simulating))
            {
                // use network to initally add aircraft
                IContextNetwork *networkContext = this->getIContextNetwork();
                Q_ASSERT(networkContext);
                Q_ASSERT(networkContext->isLocalObject());

                // initially add aircraft
                const CSimulatedAircraftList aircrafts = networkContext->getAircraftInRange();
                for (const CSimulatedAircraft &simulatedAircraft : aircrafts)
                {
                    Q_ASSERT(!simulatedAircraft.getCallsign().isEmpty());
                    ps_addedRemoteAircraft(simulatedAircraft);
                }
                m_initallyAddAircrafts = false;
            }
            if (!statusEnum.testFlag(ISimulator::Connected))
            {
                // we got disconnected, plugin no longer needed
                unloadSimulatorPlugin();
                restoreSimulatorPlugins();
            }
            emit simulatorStatusChanged(status);
        }

        void CContextSimulator::ps_modelSetChanged(const CSimulatorInfo &simulator)
        {
            Q_UNUSED(simulator);
            emit this->modelSetChanged();
        }

        void CContextSimulator::ps_textMessagesReceived(const Network::CTextMessageList &textMessages)
        {
            if (!isSimulatorSimulating()) { return; }
            if (!this->getIContextOwnAircraft()) { return; }
            const CSimulatorMessagesSettings settings = m_messageSettings.getThreadLocal();
            const CSimulatedAircraft ownAircraft = this->getIContextOwnAircraft()->getOwnAircraft();
            for (const auto &tm : textMessages)
            {
                if (!settings.relayThisTextMessage(tm, ownAircraft)) { continue; }
                m_simulatorPlugin.second->displayTextMessage(tm);
            }
        }

        void CContextSimulator::ps_cockpitChangedFromSimulator(const CSimulatedAircraft &ownAircraft)
        {
            Q_ASSERT(getIContextOwnAircraft());
            emit getIContextOwnAircraft()->changedAircraftCockpit(ownAircraft, IContextSimulator::InterfaceName());
        }

        void CContextSimulator::ps_changedRemoteAircraftModel(const CSimulatedAircraft &aircraft, const BlackMisc::CIdentifier &originator)
        {
            if (CIdentifiable::isMyIdentifier(originator)) { return; }
            if (!isSimulatorSimulating()) { return; }
            m_simulatorPlugin.second->changeRemoteAircraftModel(aircraft);
        }

        void CContextSimulator::ps_changedRemoteAircraftEnabled(const CSimulatedAircraft &aircraft)
        {
            if (!isSimulatorSimulating()) { return; }
            m_simulatorPlugin.second->changeRemoteAircraftEnabled(aircraft);
        }

        void CContextSimulator::ps_networkConnectionStatusChanged(int from, int to)
        {
            Q_UNUSED(from);
            BLACK_VERIFY_X(getIContextNetwork(), Q_FUNC_INFO, "Missing network context");
            const INetwork::ConnectionStatus statusTo = static_cast<INetwork::ConnectionStatus>(to);
            if (statusTo == INetwork::Connected && this->getIContextNetwork())
            {
                this->m_networkSessionId = this->getIContextNetwork()->getConnectedServer().getServerSessionId();
            }
            else if (!m_networkSessionId.isEmpty())
            {
                this->m_networkSessionId.clear();
                this->m_aircraftMatcher.clearMatchingStatistics();
            }
        }

        void CContextSimulator::ps_addingRemoteAircraftFailed(const CSimulatedAircraft &remoteAircraft, const CStatusMessage &message)
        {
            if (!isSimulatorSimulating()) { return; }
            emit addingRemoteModelFailed(remoteAircraft, message);
        }

        void CContextSimulator::ps_updateSimulatorCockpitFromContext(const CSimulatedAircraft &ownAircraft, const CIdentifier &originator)
        {
            if (!isSimulatorSimulating()) { return; }
            if (originator.getName().isEmpty() || originator == IContextSimulator::InterfaceName()) { return; }

            // update
            m_simulatorPlugin.second->updateOwnSimulatorCockpit(ownAircraft, originator);
        }

        void CContextSimulator::ps_networkRequestedNewAircraft(const CCallsign &callsign, const QString &aircraftIcao, const QString &airlineIcao, const QString &livery)
        {
            if (m_networkSessionId.isEmpty()) { return; }
            m_aircraftMatcher.evaluateStatisticsEntry(m_networkSessionId, callsign, aircraftIcao, airlineIcao, livery);
        }

        void CContextSimulator::ps_relayStatusMessageToSimulator(const BlackMisc::CStatusMessage &message)
        {
            if (!isSimulatorSimulating()) { return; }
            const CSimulatorMessagesSettings simMsg = m_messageSettings.getThreadLocal();
            if (simMsg.relayThisStatusMessage(message))
            {
                m_simulatorPlugin.second->displayStatusMessage(message);
            }
        }

        void CContextSimulator::changeEnabledSimulators()
        {
            CSimulatorPluginInfo currentPluginInfo = m_simulatorPlugin.first;
            const QStringList enabledSimulators = m_enabledSimulators.getThreadLocal();

            // Unload the current plugin, if it is no longer enabled
            if (!currentPluginInfo.isUnspecified() && !enabledSimulators.contains(currentPluginInfo.getIdentifier()))
            {
                unloadSimulatorPlugin();
                emit simulatorStatusChanged(ISimulator::Disconnected);
            }
            restoreSimulatorPlugins();
        }

        void CContextSimulator::restoreSimulatorPlugins()
        {
            if (!m_simulatorPlugin.first.isUnspecified()) { return; }

            stopSimulatorListeners();
            const QStringList enabledSimulators = m_enabledSimulators.getThreadLocal();
            const CSimulatorPluginInfoList allSimulators = m_plugins->getAvailableSimulatorPlugins();
            for (const CSimulatorPluginInfo &s : allSimulators)
            {
                if (enabledSimulators.contains(s.getIdentifier()))
                {
                    startSimulatorPlugin(s);
                }
            }
        }

        CPixmap CContextSimulator::iconForModel(const QString &modelString) const
        {
            if (m_simulatorPlugin.first.isUnspecified()) { return CPixmap(); }
            Q_ASSERT_X(m_simulatorPlugin.second, Q_FUNC_INFO, "Missing simulator");
            const CAircraftModel model(this->m_modelSetLoader.getModelForModelString(modelString));

            // load from file
            CStatusMessage msg;
            const CPixmap pm(model.loadIcon(msg));
            if (!msg.isEmpty()) { CLogMessage::preformatted(msg);}
            return pm;
        }

        CStatusMessageList CContextSimulator::getMatchingMessages(const CCallsign &callsign) const
        {
            if (m_debugEnabled) { CLogMessage(this, CLogCategory::contextSlot()).debug() << Q_FUNC_INFO << callsign; }
            return m_matchingMessages[callsign];
        }

        bool CContextSimulator::isMatchingMessagesEnabled() const
        {
            if (m_debugEnabled) { CLogMessage(this, CLogCategory::contextSlot()).debug() << Q_FUNC_INFO; }
            return m_enableMatchingMessages;
        }

        void CContextSimulator::enableMatchingMessages(bool enabled)
        {
            if (m_debugEnabled) { CLogMessage(this, CLogCategory::contextSlot()).debug() << Q_FUNC_INFO << enabled; }
            if (m_enableMatchingMessages == enabled) { return; }
            m_enableMatchingMessages = enabled;
            emit CContext::changedLogOrDebugSettings();
        }

        CMatchingStatistics CContextSimulator::getCurrentMatchingStatistics(bool missingOnly) const
        {
            if (m_debugEnabled) { CLogMessage(this, CLogCategory::contextSlot()).debug() << Q_FUNC_INFO << missingOnly; }
            const CMatchingStatistics statistics = this->m_aircraftMatcher.getCurrentStatistics();
            return missingOnly ?
                   statistics.findMissingOnly() :
                   statistics;
        }

        bool CContextSimulator::parseCommandLine(const QString &commandLine, const CIdentifier &originator)
        {
            Q_UNUSED(originator);
            if (commandLine.isEmpty()) { return false; }
            CSimpleCommandParser parser(
            {
                ".plugin",
                ".drv", ".driver"    // forwarded to driver
            });
            parser.parse(commandLine);
            if (!parser.isKnownCommand()) { return false; }

            if (parser.matchesCommand("plugin") || parser.matchesCommand("drv") || parser.matchesCommand("driver"))
            {
                if (!this->m_simulatorPlugin.second) { return false; }
                return this->m_simulatorPlugin.second->parseCommandLine(commandLine, originator);
            }
            return false;
        }

        void CContextSimulator::highlightAircraft(const CSimulatedAircraft &aircraftToHighlight, bool enableHighlight, const CTime &displayTime)
        {
            if (m_debugEnabled) { CLogMessage(this, CLogCategory::contextSlot()).debug() << Q_FUNC_INFO << aircraftToHighlight << enableHighlight << displayTime; }
            Q_ASSERT(m_simulatorPlugin.second);
            m_simulatorPlugin.second->highlightAircraft(aircraftToHighlight, enableHighlight, displayTime);
        }

        bool CContextSimulator::resetToModelMatchingAircraft(const CCallsign &callsign)
        {
            CSimulatedAircraft aircraft = getAircraftInRangeForCallsign(callsign);
            if (aircraft.getCallsign() != callsign) { return false; } // not found
            aircraft.setModel(aircraft.getNetworkModel());
            ps_addedRemoteAircraft(aircraft);
            return true;
        }

        void CContextSimulator::setWeatherActivated(bool activated)
        {
            m_isWeatherActivated = activated;

            if (m_debugEnabled) { CLogMessage(this, CLogCategory::contextSlot()).debug() << Q_FUNC_INFO; }
            if (m_simulatorPlugin.first.isUnspecified()) { return; }
            m_simulatorPlugin.second->setWeatherActivated(activated);
        }

        void CContextSimulator::requestWeatherGrid(const Weather::CWeatherGrid &weatherGrid, const CIdentifier &identifier)
        {
            if (m_debugEnabled) { CLogMessage(this, CLogCategory::contextSlot()).debug() << Q_FUNC_INFO << identifier; }
            m_weatherManager.requestWeatherGrid(weatherGrid, identifier);
        }

        void CContextSimulator::ps_simulatorStarted(const CSimulatorPluginInfo &info)
        {
            stopSimulatorListeners();
            loadSimulatorPlugin(info);
        }

        void CContextSimulator::stopSimulatorListeners()
        {
            for (const auto &info : getAvailableSimulatorPlugins())
            {
                ISimulatorListener *listener = m_plugins->getListener(info.getIdentifier());
                if (listener)
                {
                    const bool s = QMetaObject::invokeMethod(listener, "stop");
                    Q_ASSERT_X(s, Q_FUNC_INFO, "Cannot invoke stop");
                    Q_UNUSED(s);
                }
            }
        }

        void CContextSimulator::addMatchingMessages(const CCallsign &callsign, const CStatusMessageList &messages)
        {
            if (callsign.isEmpty()) { return; }
            if (messages.isEmpty()) { return; }
            if (!this->m_enableMatchingMessages) { return; }
            if (this->m_matchingMessages.contains(callsign))
            {
                CStatusMessageList &msgs = this->m_matchingMessages[callsign];
                msgs.push_back(messages);
            }
            else
            {
                this->m_matchingMessages.insert(callsign, messages);
            }
        }

        void CContextSimulator::initByLastUsedModelSet()
        {
            // no models in matcher, but in cache, we can set them as default
            const CSimulatorInfo sim(m_modelSetLoader.getSimulator());
            if (!m_aircraftMatcher.hasModels() && m_modelSetLoader.getAircraftModelsCount() > 0)
            {
                const CAircraftModelList models(m_modelSetLoader.getAircraftModels());
                CLogMessage(this).info("Init aircraft matcher with %1 models from set for '%2'") << models.size() << sim.toQString();
                m_aircraftMatcher.setModelSet(models, sim);
            }
            else
            {
                CLogMessage(this).info("Start loading of model set for '%1'") << sim.toQString();
                m_modelSetLoader.admitCache(); // when ready chache change signal
            }
        }
    } // namespace
} // namespace

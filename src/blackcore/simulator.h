/* Copyright (C) 2013
 * swift Project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file

#ifndef BLACKCORE_SIMULATOR_H
#define BLACKCORE_SIMULATOR_H

#include "blackconfig/buildconfig.h"
#include "blackcore/blackcoreexport.h"
#include "blackmisc/aviation/airportlist.h"
#include "blackmisc/aviation/callsignset.h"
#include "blackmisc/simulation/aircraftmodellist.h"
#include "blackmisc/simulation/simulatedaircraft.h"
#include "blackmisc/simulation/simulatorplugininfo.h"
#include "blackmisc/simulation/interpolationrenderingsetup.h"
#include "blackmisc/simulation/interpolatormulti.h"
#include "blackmisc/pq/length.h"
#include "blackmisc/pq/time.h"
#include "blackmisc/statusmessage.h"
#include "blackmisc/identifiable.h"
#include "blackmisc/identifier.h"
#include "blackmisc/pixmap.h"
#include "blackmisc/simplecommandparser.h"

#include <QFlags>
#include <QObject>
#include <QString>

namespace BlackMisc
{
    namespace Aviation { class CCallsign; }
    namespace Network { class CTextMessage; }
    namespace Simulation
    {
        class CSimulatorInternals;
        class IOwnAircraftProvider;
        class IRemoteAircraftProvider;
    }
    namespace Weather { class IWeatherGridProvider; }
}

namespace BlackCore
{
    //! Interface to a simulator.
    class BLACKCORE_EXPORT ISimulator :
        public QObject,
        public BlackMisc::CIdentifiable
    {
        Q_OBJECT

    public:
        //! ISimulator status
        enum SimulatorStatusFlag
        {
            Unspecified  = 0,      //!< unspecied, needed as default value
            Disconnected = 1 << 0, //!< not connected, and hence not simulating/paused
            Connected    = 1 << 1, //!< Is the plugin connected to the simulator?
            Simulating   = 1 << 2, //!< Is the simulator actually simulating?
            Paused       = 1 << 3, //!< Is the simulator paused?
        };
        Q_DECLARE_FLAGS(SimulatorStatus, SimulatorStatusFlag)
        Q_FLAG(SimulatorStatus)

        //! Render all aircraft if number of aircraft >= MaxAircraftInfinite
        const int MaxAircraftInfinite = 100;

        //! Destructor
        virtual ~ISimulator() {}

        //! Combined status
        virtual SimulatorStatus getSimulatorStatus() const;

        //! Is time synchronization on?
        virtual bool isTimeSynchronized() const = 0;

        //! Get the simulator info (metadata of plugin)
        virtual const BlackMisc::Simulation::CSimulatorPluginInfo &getSimulatorPluginInfo() const = 0;

        //! Get simulator info (default implementation)
        virtual BlackMisc::Simulation::CSimulatorInfo getSimulatorInfo() const;

        //! Get the setup (simulator environemnt)
        virtual const BlackMisc::Simulation::CSimulatorInternals &getSimulatorInternals() const = 0;

        //! Connect to simulator
        virtual bool connectTo() = 0;

        //! Disconnect from simulator
        virtual bool disconnectFrom() = 0;

        //! Get default aircraft model
        virtual BlackMisc::Simulation::CAircraftModel getDefaultModel() const = 0;

        //! Logically add a new aircraft. Depending on max. aircraft, enabled status etc.
        //! it will physically added to the simulator.
        //! \sa physicallyAddRemoteAircraft
        virtual bool logicallyAddRemoteAircraft(const BlackMisc::Simulation::CSimulatedAircraft &remoteAircraft) = 0;

        //! Logically remove remote aircraft from simulator. Depending on max. aircraft, enabled status etc.
        //! it will physically added to the simulator.
        virtual bool logicallyRemoveRemoteAircraft(const BlackMisc::Aviation::CCallsign &callsign) = 0;

        //! Change remote aircraft per property
        virtual bool changeRemoteAircraftModel(const BlackMisc::Simulation::CSimulatedAircraft &aircraft) = 0;

        //! Aircraft got enabled / disabled
        virtual bool changeRemoteAircraftEnabled(const BlackMisc::Simulation::CSimulatedAircraft &aircraft) = 0;

        //! Update own aircraft cockpit (usually from context)
        virtual bool updateOwnSimulatorCockpit(const BlackMisc::Simulation::CSimulatedAircraft &aircraft, const BlackMisc::CIdentifier &originator) = 0;

        //! Update own aircraft cockpit (usually from context)
        virtual bool updateOwnSimulatorSelcal(const BlackMisc::Aviation::CSelcal &selcal, const BlackMisc::CIdentifier &originator) = 0;

        //! Display a status message in the simulator
        virtual void displayStatusMessage(const BlackMisc::CStatusMessage &message) const = 0;

        //! Display a text message
        virtual void displayTextMessage(const BlackMisc::Network::CTextMessage &message) const = 0;

        //! Airports in range from simulator, or if not available from web service
        virtual BlackMisc::Aviation::CAirportList getAirportsInRange() const = 0;

        //! Set time synchronization between simulator and user's computer time
        //! \remarks not all drivers implement this, e.g. if it is an intrinsic simulator feature
        virtual bool setTimeSynchronization(bool enable, const BlackMisc::PhysicalQuantities::CTime &offset) = 0;

        //! Time synchronization offset
        virtual BlackMisc::PhysicalQuantities::CTime getTimeSynchronizationOffset() const = 0;

        //! Debugging messages etc.
        //! \threadsafe
        virtual BlackMisc::Simulation::CInterpolationAndRenderingSetup getInterpolationAndRenderingSetup() const = 0;

        //! Enable debugging messages etc.
        //! \threadsafe
        virtual void setInterpolationAndRenderingSetup(const BlackMisc::Simulation::CInterpolationAndRenderingSetup &setup) = 0;

        //! Is the aircraft rendered (displayed in simulator)?
        //! This shall only return true if the aircraft is really visible in the simulator
        virtual bool isPhysicallyRenderedAircraft(const BlackMisc::Aviation::CCallsign &callsign) const = 0;

        //! Physically rendered (displayed in simulator)
        //! This shall only return aircraft handled in the simulator
        virtual BlackMisc::Aviation::CCallsignSet physicallyRenderedAircraft() const = 0;

        //! Highlight the aircraft for given time (or disable highlight)
        virtual void highlightAircraft(const BlackMisc::Simulation::CSimulatedAircraft &aircraftToHighlight, bool enableHighlight, const BlackMisc::PhysicalQuantities::CTime &displayTime) = 0;

        //! Activates or deactivates simulator weather
        virtual void setWeatherActivated(bool activated) = 0;

        //! Driver will be unloaded
        virtual void unload() = 0;

        //! Is overall (swift) application shutting down
        virtual bool isShuttingDown() const = 0;

        //! Set interpolation mode, empty callsign applies to all know callsigns
        virtual bool setInterpolatorMode(BlackMisc::Simulation::CInterpolatorMulti::Mode mode, const BlackMisc::Aviation::CCallsign &callsign) = 0;

        //! \addtogroup swiftdotcommands
        //! @{
        //! <pre>
        //! .drv    unload                  unload driver                           BlackCore::CSimulatorCommon
        //! .drv    fsuipc      on|off      enable/disable FSUIPC (if applicable)   BlackSimPlugin::FsCommon::CSimulatorFsCommon
        //! </pre>
        //! @}
        //! Parse command line
        virtual bool parseCommandLine(const QString &commandLine, const BlackMisc::CIdentifier &originator) = 0;

        //! Register help
        static void registerHelp();

        //! Status to string
        static QString statusToString(SimulatorStatus status);

    signals:
        //! Simulator combined status
        void simulatorStatusChanged(SimulatorStatus status);

        //! Emitted when own aircraft model has changed
        void ownAircraftModelChanged(const BlackMisc::Simulation::CAircraftModel &model);

        //! Render restrictions have been changed
        void renderRestrictionsChanged(bool restricted, bool enabled, int maxAircraft, const BlackMisc::PhysicalQuantities::CLength &maxRenderedDistance);

        //! Aircraft rendering changed
        void aircraftRenderingChanged(const BlackMisc::Simulation::CSimulatedAircraft &aircraft);

        //! Adding the remote model failed
        void physicallyAddingRemoteModelFailed(const BlackMisc::Simulation::CSimulatedAircraft &remoteAircraft, const BlackMisc::CStatusMessage &message);

        //! An airspace snapshot was handled
        void airspaceSnapshotHandled();

    protected:
        //! Default constructor
        ISimulator(QObject *parent = nullptr);

        //! Are we connected to the simulator?
        virtual bool isConnected() const = 0;

        //! Simulator paused?
        virtual bool isPaused() const = 0;

        //! Simulator running?
        virtual bool isSimulating() const = 0;

        //! Add new remote aircraft physically to the simulator
        //! \sa changeRemoteAircraftEnabled to hide a remote aircraft
        virtual bool physicallyAddRemoteAircraft(const BlackMisc::Simulation::CSimulatedAircraft &remoteAircraft) = 0;

        //! Remove remote aircraft from simulator
        virtual bool physicallyRemoveRemoteAircraft(const BlackMisc::Aviation::CCallsign &callsign) = 0;

        //! Remove remote aircraft from simulator
        virtual int physicallyRemoveMultipleRemoteAircraft(const BlackMisc::Aviation::CCallsignSet &callsigns) = 0;

        //! Remove all remote aircraft
        virtual int physicallyRemoveAllRemoteAircraft() = 0;

        //! Emit the combined status
        //! \param oldStatus optionally one can capture and provide the old status for comparison. In case of equal status values no signal will be sent
        //! \sa simulatorStatusChanged;
        void emitSimulatorCombinedStatus(SimulatorStatus oldStatus = Unspecified);
    };

    //! Interface to a simulator listener.
    //! The simulator listener is responsible for letting the core know when
    //! the corresponding simulator is started.
    class BLACKCORE_EXPORT ISimulatorListener : public QObject
    {
        Q_OBJECT

    public:
        //! Constructor
        //! \sa ISimulatorFactory::createListener().
        ISimulatorListener(const BlackMisc::Simulation::CSimulatorPluginInfo &info);

        //! Destructor
        virtual ~ISimulatorListener() = default;

        //! Corresponding info
        const BlackMisc::Simulation::CSimulatorPluginInfo &getPluginInfo() const { return m_info; }

        //! Info about the backend system (if available)
        virtual QString backendInfo() const;

        //! Overall (swift) application shutting down
        virtual bool isShuttingDown() const;

    public slots:
        //! Start listening for the simulator to start.
        void start();

        //! Stops listening.
        void stop();

    signals:
        //! Emitted when the listener discovers the simulator running.
        void simulatorStarted(const BlackMisc::Simulation::CSimulatorPluginInfo &info);

    protected:
        //! Plugin specific implementation to start listener
        virtual void startImpl() = 0;

        //! Plugin specific implementation to stop listener
        virtual void stopImpl() = 0;

    private:
        BlackMisc::Simulation::CSimulatorPluginInfo m_info;
        bool m_isRunning = false;
    };

    //! Factory pattern class to create instances of ISimulator
    class BLACKCORE_EXPORT ISimulatorFactory
    {
    public:
        //! ISimulatorVirtual destructor
        virtual ~ISimulatorFactory() {}

        //! Create a new instance of a driver
        //! \param info                      metadata about simulator
        //! \param ownAircraftProvider       in memory access to own aircraft data
        //! \param remoteAircraftProvider    in memory access to rendered aircraft data such as situation history and aircraft itself
        //! \param weatherGridProvider       in memory access to weather grid data
        //! \return driver instance
        virtual ISimulator *create(
            const BlackMisc::Simulation::CSimulatorPluginInfo &info,
            BlackMisc::Simulation::IOwnAircraftProvider *ownAircraftProvider,
            BlackMisc::Simulation::IRemoteAircraftProvider *remoteAircraftProvider,
            BlackMisc::Weather::IWeatherGridProvider *weatherGridProvider) = 0;

        //! Simulator listener instance
        virtual ISimulatorListener *createListener(const BlackMisc::Simulation::CSimulatorPluginInfo &info) = 0;
    };
} // namespace

Q_DECLARE_INTERFACE(BlackCore::ISimulatorFactory, "org.swift-project.blackcore.simulatorinterface")
Q_DECLARE_OPERATORS_FOR_FLAGS(BlackCore::ISimulator::SimulatorStatus)

#endif // guard

/* Copyright (C) 2013
 * swift Project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file

#ifndef BLACKCORE_SIMULATOR_COMMON_H
#define BLACKCORE_SIMULATOR_COMMON_H

#include <QObject>
#include <QTimer>
#include <QReadWriteLock>
#include <QtGlobal>

#include "blackcore/aircraftmatcher.h"
#include "blackcore/blackcoreexport.h"
#include "blackcore/simulator.h"
#include "blackmisc/aviation/callsignset.h"
#include "blackmisc/simulation/aircraftmodelsetloader.h"
#include "blackmisc/simulation/ownaircraftprovider.h"
#include "blackmisc/simulation/remoteaircraftprovider.h"
#include "blackmisc/simulation/simulatedaircraftlist.h"
#include "blackmisc/simulation/simulatorinfo.h"
#include "blackmisc/simulation/simulatorplugininfo.h"
#include "blackmisc/simulation/simulatorinternals.h"
#include "blackmisc/simulation/settings/simulatorsettings.h"
#include "blackmisc/simulation/interpolationrenderingsetup.h"
#include "blackmisc/simulation/interpolationhints.h"
#include "blackmisc/simulation/interpolationlogger.h"
#include "blackmisc/weather/weathergridprovider.h"
#include "blackmisc/pq/length.h"
#include "blackmisc/pq/time.h"
#include "blackmisc/pq/units.h"
#include "blackmisc/connectionguard.h"

namespace BlackMisc
{
    class CSimpleCommandParser;

    namespace Aviation
    {
        class CAircraftParts;
        class CAircraftSituation;
        class CCallsign;
    }
    namespace Simulation
    {
        class CAirspaceAircraftSnapshot;
        class CSimulatedAircraft;
    }
}

namespace BlackCore
{
    //! Common base class with providers, interface and some base functionality
    class BLACKCORE_EXPORT CSimulatorCommon :
        public BlackCore::ISimulator,
        public BlackMisc::Simulation::COwnAircraftAware,    // gain access to in memory own aircraft data
        public BlackMisc::Simulation::CRemoteAircraftAware, // gain access to in memory remote aircraft data
        public BlackMisc::Weather::CWeatherGridAware        // gain access to in memory weather grid
    {
        Q_OBJECT

    public:
        //! Log categories
        static const BlackMisc::CLogCategoryList &getLogCategories();

        //! Destructor
        virtual ~CSimulatorCommon();

        // --------- ISimulator implementations ------------
        virtual BlackMisc::Simulation::CAircraftModel getDefaultModel() const override;
        virtual void setInterpolationAndRenderingSetup(const BlackMisc::Simulation::CInterpolationAndRenderingSetup &setup) override;
        virtual BlackMisc::Simulation::CInterpolationAndRenderingSetup getInterpolationAndRenderingSetup() const override;
        virtual void highlightAircraft(const BlackMisc::Simulation::CSimulatedAircraft &aircraftToHighlight, bool enableHighlight, const BlackMisc::PhysicalQuantities::CTime &displayTime) override;
        virtual const BlackMisc::Simulation::CSimulatorPluginInfo &getSimulatorPluginInfo() const override;
        virtual const BlackMisc::Simulation::CSimulatorInternals &getSimulatorInternals() const override;
        virtual BlackMisc::Aviation::CAirportList getAirportsInRange() const override;
        virtual void setWeatherActivated(bool activated) override;
        virtual void unload() override;
        virtual bool isShuttingDown() const override;
        virtual int physicallyRemoveMultipleRemoteAircraft(const BlackMisc::Aviation::CCallsignSet &callsigns) override;
        virtual int physicallyRemoveAllRemoteAircraft() override;

        //! \addtogroup swiftdotcommands
        //! @{
        //! <pre>
        //! .drv unload                    unload plugin                           BlackCore::CSimulatorCommon
        //! .drv logint callsign           log interpolator for callsign           BlackCore::CSimulatorCommon
        //! .drv logint off                no log information for interpolator     BlackCore::CSimulatorCommon
        //! .drv logint write              write interpolator log to file          BlackCore::CSimulatorCommon
        //! .drv logint clear              clear current log                       BlackCore::CSimulatorCommon
        //! .drv spline|linear callsign    interpolator spline                     BlackCore::CSimulatorCommon
        //! </pre>
        //! @}
        //! \copydoc ISimulator::parseCommandLine
        virtual bool parseCommandLine(const QString &commandLine, const BlackMisc::CIdentifier &originator) override;

        //! Register help
        static void registerHelp();

        // --------- ISimulator implementations ------------

    protected slots:
        //! \name Connected with remote aircraft provider signals
        //! @{
        //! Slow timer used to highlight aircraft, can be used for other things too
        virtual void ps_oneSecondTimer();

        //! Recalculate the rendered aircraft, this happens when restrictions are applied (max. aircraft, range)
        virtual void ps_recalculateRenderedAircraft(const BlackMisc::Simulation::CAirspaceAircraftSnapshot &snapshot);

        //! Provider added situation
        virtual void ps_remoteProviderAddAircraftSituation(const BlackMisc::Aviation::CAircraftSituation &situation);

        //! Provider added parts
        virtual void ps_remoteProviderAddAircraftParts(const BlackMisc::Aviation::CCallsign &callsign, const BlackMisc::Aviation::CAircraftParts &parts);

        //! Provider removed aircraft
        virtual void ps_remoteProviderRemovedAircraft(const BlackMisc::Aviation::CCallsign &callsign);
        //! @}

        //! All swift data read from DB
        virtual void ps_allSwiftDataRead();

        //! DB models read
        virtual void ps_modelMatchingEntities();

        //! DB Airports read
        virtual void ps_airportsRead();

    protected:
        //! Constructor
        CSimulatorCommon(const BlackMisc::Simulation::CSimulatorPluginInfo &info,
                         BlackMisc::Simulation::IOwnAircraftProvider *ownAircraftProvider,
                         BlackMisc::Simulation::IRemoteAircraftProvider *remoteAircraftProvider,
                         BlackMisc::Weather::IWeatherGridProvider *weatherGridProvider,
                         QObject *parent);

        //! \name Interface implementations
        //! @{
        virtual bool logicallyAddRemoteAircraft(const BlackMisc::Simulation::CSimulatedAircraft &remoteAircraft) override;
        virtual bool logicallyRemoveRemoteAircraft(const BlackMisc::Aviation::CCallsign &callsign) override;
        //! @}

        //! New plugin info
        //! \remark normally only used by
        void setNewPluginInfo(const BlackMisc::Simulation::CSimulatorPluginInfo &info, const BlackMisc::Simulation::CAircraftModel &defaultModel);

        //! Max.airports in range
        int maxAirportsInRange() const;

        //! Reset state
        virtual void reset();

        //! Clear all aircraft related data
        virtual void clearAllAircraft();

        //! Inject weather grid to simulator
        virtual void injectWeatherGrid(const BlackMisc::Weather::CWeatherGrid &weatherGrid) { Q_UNUSED(weatherGrid); }

        //! Airports from web services
        BlackMisc::Aviation::CAirportList getWebServiceAirports() const;

        //! Airport from web services by ICAO code
        BlackMisc::Aviation::CAirport getWebServiceAirport(const BlackMisc::Aviation::CAirportIcaoCode &icao) const;

        //! Blink the highlighted aircraft
        void blinkHighlightedAircraft();

        //! Restore aircraft from the provider data
        void resetAircraftFromProvider(const BlackMisc::Aviation::CCallsign &callsign);

        //! Set own model
        void reverseLookupAndUpdateOwnAircraftModel(const BlackMisc::Simulation::CAircraftModel &model);

        //! Set own model
        void reverseLookupAndUpdateOwnAircraftModel(const QString &modelString);

        //! Reload weather settings
        void reloadWeatherSettings();

        //! Parse driver specific details for ISimulator::parseCommandLine
        virtual bool parseDetails(const BlackMisc::CSimpleCommandParser &parser);

        //! Display a debug log message based on BlackMisc::Simulation::CInterpolationAndRenderingSetup
        void debugLogMessage(const QString &msg) const;

        //! Display a debug log message based on BlackMisc::Simulation::CInterpolationAndRenderingSetup
        void debugLogMessage(const QString &funcInfo, const QString &msg) const;

        //! Show log messages?
        bool showDebugLogMessage() const;

        bool m_pausedSimFreezesInterpolation = false;                      //!< paused simulator will also pause interpolation (so AI aircraft will hold)
        bool m_autoCalcAirportDistance = true;                             //!< automatically calculate airport distance and bearing
        BlackMisc::Simulation::CAircraftModel m_defaultModel;              //!< default model
        int    m_statsUpdateAircraftCountMs = 0;                           //!< statistics update count
        qint64 m_statsUpdateAircraftTimeTotalMs = 0;                       //!< statistics update time
        qint64 m_statsUpdateAircraftTimeAvgMs = 0;                         //!< statistics update time
        BlackMisc::Simulation::CSimulatorInternals m_simulatorInternals;   //!< setup object
        BlackMisc::Simulation::CInterpolationLogger m_interpolationLogger; //!< log interpolation
        mutable QReadWriteLock m_interpolationRenderingSetupMutex;         //!< mutex protecting setup object

        // setup for debugging, logs ..
        BlackMisc::Simulation::CInterpolationAndRenderingSetup m_interpolationRenderingSetup; //!< logging, rendering etc.

        // some optional functionality which can be used by the sims as needed
        BlackMisc::Simulation::CSimulatedAircraftList m_aircraftToAddAgainWhenRemoved;             //!< add this model again when removed, normally used to change model
        QHash<BlackMisc::Aviation::CCallsign, BlackMisc::Simulation::CInterpolationHints> m_hints; //!< last ground elevation fetched

        bool m_isWeatherActivated = false;                         //!< Is simulator weather activated?
        BlackMisc::Geo::CCoordinateGeodetic m_lastWeatherPosition; //!< Own aircraft position at which weather was fetched and injected last
        BlackMisc::CSetting<BlackMisc::Simulation::Settings::TSelectedWeatherScenario> m_weatherScenarioSettings { this, &CSimulatorCommon::reloadWeatherSettings }; //!< Selected weather scenario

        //! Lookup against DB data
        static BlackMisc::Simulation::CAircraftModel reverseLookupModel(const BlackMisc::Simulation::CAircraftModel &model);

    private:
        bool m_blinkCycle = false;            //!< used for highlighting
        qint64 m_highlightEndTimeMsEpoch = 0; //!< end highlighting
        int m_timerCounter = 0;               //!< allows to calculate n seconds
        QTimer                                        m_oneSecondTimer {this};              //!< multi purpose timer
        BlackMisc::Simulation::CSimulatorPluginInfo   m_simulatorPluginInfo;                //!< info object
        BlackMisc::Simulation::CSimulatedAircraftList m_highlightedAircraft;                //!< all other aircraft are to be ignored
        BlackMisc::Aviation::CCallsignSet             m_callsignsToBeRendered;              //!< callsigns which will be rendered
        BlackMisc::CConnectionGuard                   m_remoteAircraftProviderConnections;  //!< connected signal/slots
    };
} // namespace

#endif // guard

/* Copyright (C) 2013
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file

#ifndef BLACKSIMPLUGIN_SIMULATOR_XPLANE_H
#define BLACKSIMPLUGIN_SIMULATOR_XPLANE_H

#include "blackcore/simulatorcommon.h"
#include "blackmisc/simulation/ownaircraftprovider.h"
#include "blackmisc/simulation/aircraftmodellist.h"
#include "blackmisc/pixmap.h"
#include "blackmisc/weather/weathergrid.h"
#include "plugins/simulator/xplaneconfig/simulatorxplaneconfig.h"
#include <QDBusConnection>

class QDBusServiceWatcher;

namespace BlackSimPlugin
{
    namespace XPlane
    {
        class CXBusServiceProxy;
        class CXBusTrafficProxy;
        class CXBusWeatherProxy;

        //! X-Plane ISimulator implementation
        class CSimulatorXPlane : public BlackCore::CSimulatorCommon
        {
            Q_OBJECT

        public:
            //! Constructor
            CSimulatorXPlane(const BlackMisc::Simulation::CSimulatorPluginInfo &info,
                             BlackMisc::Simulation::IOwnAircraftProvider *ownAircraftProvider,
                             BlackMisc::Simulation::IRemoteAircraftProvider *remoteAircraftProvider,
                             BlackMisc::IPluginStorageProvider *pluginStorageProvider,
                             BlackMisc::Weather::IWeatherGridProvider *weatherGridProvider,
                             QObject *parent = nullptr);

            //! ISimulator implementations
            //! @{
            virtual bool isTimeSynchronized() const override { return false; } // TODO: Can we query the XP intrinisc feature?
            virtual bool connectTo() override;
            virtual bool disconnectFrom() override;
            virtual bool physicallyAddRemoteAircraft(const BlackMisc::Simulation::CSimulatedAircraft &newRemoteAircraft) override;
            virtual bool physicallyRemoveRemoteAircraft(const BlackMisc::Aviation::CCallsign &callsign) override;
            virtual int physicallyRemoveAllRemoteAircraft() override;
            virtual BlackMisc::Aviation::CCallsignSet physicallyRenderedAircraft() const override;
            virtual bool isPhysicallyRenderedAircraft(const BlackMisc::Aviation::CCallsign &callsign) const override;
            virtual bool changeRemoteAircraftModel(const BlackMisc::Simulation::CSimulatedAircraft &aircraft, const BlackMisc::CIdentifier &originator) override;
            virtual bool changeRemoteAircraftEnabled(const BlackMisc::Simulation::CSimulatedAircraft &aircraft, const BlackMisc::CIdentifier &originator) override;
            virtual bool updateOwnSimulatorCockpit(const BlackMisc::Simulation::CSimulatedAircraft &aircraft, const BlackMisc::CIdentifier &originator) override;
            virtual void displayStatusMessage(const BlackMisc::CStatusMessage &message) const override;
            virtual void displayTextMessage(const BlackMisc::Network::CTextMessage &message) const override;
            virtual BlackMisc::Simulation::CAircraftModelList getInstalledModels() const override;
            virtual BlackMisc::Aviation::CAirportList getAirportsInRange() const override;
            virtual bool setTimeSynchronization(bool enable, const BlackMisc::PhysicalQuantities::CTime &offset) override;
            virtual BlackMisc::PhysicalQuantities::CTime getTimeSynchronizationOffset() const override { return BlackMisc::PhysicalQuantities::CTime(0, BlackMisc::PhysicalQuantities::CTimeUnit::hrmin()); }
            virtual BlackMisc::CPixmap iconForModel(const QString &modelString) const override;
            //! @}

            //! Creates an appropriate dbus connection from the string describing it
            static QDBusConnection connectionFromString(const QString &str);

        protected slots:
            //! \copydoc BlackCore::CSimulatorCommon::ps_remoteProviderAddAircraftSituation
            virtual void ps_remoteProviderAddAircraftSituation(const BlackMisc::Aviation::CAircraftSituation &situation) override;

            //! \copydoc BlackCore::CSimulatorCommon::ps_remoteProviderAddAircraftParts
            virtual void ps_remoteProviderAddAircraftParts(const BlackMisc::Aviation::CCallsign &callsign, const BlackMisc::Aviation::CAircraftParts &parts) override;

            //! \copydoc BlackCore::CSimulatorCommon::ps_remoteProviderRemovedAircraft
            virtual void ps_remoteProviderRemovedAircraft(const BlackMisc::Aviation::CCallsign &callsign) override;

        protected:
            //! \copydoc BlackCore::ISimulator::isConnected
            virtual bool isConnected() const override;

            //! \copydoc BlackCore::ISimulator::isPaused
            virtual bool isPaused() const override
            {
                //! \todo XP: provide correct pause state
                return false;
            }

            //! \copydoc BlackCore::ISimulator::isSimulating
            virtual bool isSimulating() const override { return isConnected(); }

        private slots:
            void ps_serviceUnregistered();
            void ps_setAirportsInRange(const QStringList &icaoCodes, const QStringList &names, const BlackMisc::CSequence<double> &lats, const BlackMisc::CSequence<double> &lons, const BlackMisc::CSequence<double> &alts);
            void ps_emitOwnAircraftModelChanged(const QString &path, const QString &filename, const QString &livery, const QString &icao);
            void ps_fastTimerTimeout();
            void ps_slowTimerTimeout();
            void ps_installedModelsUpdated(const QStringList &modelStrings, const QStringList &icaos, const QStringList &airlines, const QStringList &liveries);

        private:
            //! Inject weather grid to simulator
            void injectWeatherGrid(const BlackMisc::Weather::CWeatherGrid &weatherGrid);

            QDBusConnection m_conn { "default" };
            QDBusServiceWatcher *m_watcher { nullptr };
            CXBusServiceProxy *m_service { nullptr };
            CXBusTrafficProxy *m_traffic { nullptr };
            CXBusWeatherProxy *m_weather { nullptr };
            QTimer *m_fastTimer { nullptr };
            QTimer *m_slowTimer { nullptr };
            BlackMisc::Aviation::CAirportList m_airportsInRange;         //!< aiports in range of own aircraft
            BlackMisc::Simulation::CAircraftModelList m_installedModels; //!< \todo Do we still need this, as we now focus on model set

            BlackMisc::Geo::CCoordinateGeodetic m_lastWeatherPosition; //!< Own aircraft position at which weather was fetched and injected last

            //! \todo Add units to members? pitchDeg?, altitudeFt?
            struct // data is written by DBus async method callbacks
            {
                QString aircraftModelPath;
                QString aircraftIcaoCode;
                double latitude;
                double longitude;
                double altitude;
                double groundspeed;
                double pitch;
                double roll;
                double trueHeading;
                bool onGroundAll;
                int com1Active;
                int com1Standby;
                int com2Active;
                int com2Standby;
                int xpdrCode;
                int xpdrMode;
                bool xpdrIdent;
                bool beaconLightsOn;
                bool landingLightsOn;
                bool navLightsOn;
                bool strobeLightsOn;
                bool taxiLightsOn;
                double flapsReployRatio;
                double gearReployRatio;
                QList<double> enginesN1Percentage;
                double speedBrakeRatio;
            } m_xplaneData;

            void resetData()
            {
                m_xplaneData = { "", "", 0, 0, 0, 0, 0, 0, 0, false, 122800, 122800, 122800, 122800, 2000, 0, false, false, false, false,
                                 false, false, 0, 0, {}, false
                               };

            }
        };

        //! Listener waits for xbus service to show up
        class CSimulatorXPlaneListener : public BlackCore::ISimulatorListener
        {
            Q_OBJECT

        public:
            //! Constructor
            CSimulatorXPlaneListener(const BlackMisc::Simulation::CSimulatorPluginInfo &info);

        public slots:
            //! \copydoc BlackCore::ISimulatorListener::start
            virtual void start() override;

            //! \copydoc BlackCore::ISimulatorListener::stop
            virtual void stop() override;

        private:
            //! \brief Check if XBus service is already registered
            bool isXBusRunning() const;

        private slots:
            void ps_serviceRegistered(const QString &serviceName);
            void ps_xbusServerSettingChanged();

        private:
            QDBusConnection m_conn { "default" };
            QDBusServiceWatcher *m_watcher { nullptr };
            BlackMisc::CSetting<XBusServer> m_xbusServerSetting { this, &CSimulatorXPlaneListener::ps_xbusServerSettingChanged };

        };

        //! Factory for creating CSimulatorXPlane instance
        class CSimulatorXPlaneFactory : public QObject, public BlackCore::ISimulatorFactory
        {
            Q_OBJECT
            Q_PLUGIN_METADATA(IID "org.swift-project.blackcore.simulatorinterface" FILE "simulatorxplane.json")
            Q_INTERFACES(BlackCore::ISimulatorFactory)

        public:
            //! \copydoc BlackCore::ISimulatorFactory::create
            virtual BlackCore::ISimulator *create(const BlackMisc::Simulation::CSimulatorPluginInfo &info,
                                                  BlackMisc::Simulation::IOwnAircraftProvider    *ownAircraftProvider,
                                                  BlackMisc::Simulation::IRemoteAircraftProvider *remoteAircraftProvider,
                                                  BlackMisc::IPluginStorageProvider *pluginStorageProvider,
                                                  BlackMisc::Weather::IWeatherGridProvider *weatherGridProvider) override;

            //! \copydoc BlackCore::ISimulatorFactory::createListener
            virtual BlackCore::ISimulatorListener *createListener(const BlackMisc::Simulation::CSimulatorPluginInfo &info) override { return new CSimulatorXPlaneListener(info); }
        };

    } // ns
} // ns

#endif // guard

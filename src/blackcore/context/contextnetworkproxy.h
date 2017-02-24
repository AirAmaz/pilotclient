/* Copyright (C) 2013
 * swift Project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file

#ifndef BLACKCORE_CONTEXT_CONTEXTNETWORK_PROXY_H
#define BLACKCORE_CONTEXT_CONTEXTNETWORK_PROXY_H

#include "blackcore/blackcoreexport.h"
#include "blackcore/context/contextnetwork.h"
#include "blackcore/corefacadeconfig.h"
#include "blackcoreexport.h"
#include "blackmisc/audio/voiceroomlist.h"
#include "blackmisc/aviation/airporticaocode.h"
#include "blackmisc/aviation/atcstation.h"
#include "blackmisc/aviation/atcstationlist.h"
#include "blackmisc/aviation/callsignset.h"
#include "blackmisc/aviation/flightplan.h"
#include "blackmisc/identifier.h"
#include "blackmisc/network/clientlist.h"
#include "blackmisc/network/network.h"
#include "blackmisc/network/server.h"
#include "blackmisc/network/serverlist.h"
#include "blackmisc/network/textmessagelist.h"
#include "blackmisc/network/user.h"
#include "blackmisc/network/userlist.h"
#include "blackmisc/simulation/simulatedaircraft.h"
#include "blackmisc/simulation/simulatedaircraftlist.h"
#include "blackmisc/statusmessage.h"
#include "blackmisc/weather/metar.h"

#include <stdbool.h>
#include <QObject>
#include <QString>

class QDBusConnection;

namespace BlackMisc
{
    class CGenericDBusInterface;
    namespace Aviation
    {
        class CAircraftParts;
        class CCallsign;
    }
    namespace Simulation { class CAircraftModel; }
}

namespace BlackCore
{
    class CCoreFacade;
    namespace Context
    {
        //! Network context proxy
        //! \ingroup dbus
        class BLACKCORE_EXPORT CContextNetworkProxy : public IContextNetwork
        {
            Q_OBJECT
            friend class IContextNetwork;

        public:

            //! Destructor
            virtual ~CContextNetworkProxy() {}

        public slots:
            //! \name Interface overrides
            //! @{
            virtual void requestAtcBookingsUpdate() const override;
            virtual BlackMisc::Aviation::CAtcStationList getAtcStationsOnline() const override;
            virtual BlackMisc::Aviation::CAtcStationList getAtcStationsBooked() const override;
            virtual BlackMisc::Simulation::CSimulatedAircraftList getAircraftInRange() const override;
            virtual BlackMisc::Aviation::CCallsignSet getAircraftInRangeCallsigns() const override;
            virtual int getAircraftInRangeCount() const override;
            virtual BlackMisc::Simulation::CSimulatedAircraft getAircraftInRangeForCallsign(const BlackMisc::Aviation::CCallsign &callsign) const override;
            virtual BlackMisc::Aviation::CAtcStation getOnlineStationForCallsign(const BlackMisc::Aviation::CCallsign &callsign) const override;
            virtual BlackMisc::CStatusMessage connectToNetwork(const BlackMisc::Network::CServer &server, BlackCore::INetwork::LoginMode mode) override;
            virtual BlackMisc::CStatusMessage disconnectFromNetwork() override;
            virtual bool isConnected() const override;
            virtual BlackMisc::Network::CServer getConnectedServer() const override;
            virtual bool parseCommandLine(const QString &commandLine, const BlackMisc::CIdentifier &originator) override;
            virtual void sendTextMessages(const BlackMisc::Network::CTextMessageList &textMessages) override;
            virtual void sendFlightPlan(const BlackMisc::Aviation::CFlightPlan &flightPlan) override;
            virtual BlackMisc::Aviation::CFlightPlan loadFlightPlanFromNetwork(const BlackMisc::Aviation::CCallsign &callsign) const override;
            BlackMisc::Weather::CMetar getMetarForAirport(const BlackMisc::Aviation::CAirportIcaoCode &airportIcaoCode) const override;
            virtual BlackMisc::Audio::CVoiceRoomList getSelectedVoiceRooms() const override;
            virtual BlackMisc::Aviation::CAtcStationList getSelectedAtcStations() const override;
            virtual BlackMisc::Network::CUserList getUsers() const override;
            virtual BlackMisc::Network::CUserList getUsersForCallsigns(const BlackMisc::Aviation::CCallsignSet &callsigns) const override;
            virtual BlackMisc::Network::CUser getUserForCallsign(const BlackMisc::Aviation::CCallsign &callsign) const override;
            virtual BlackMisc::Network::CClientList getOtherClients() const override;
            virtual BlackMisc::Network::CServerList getVatsimVoiceServers() const override;
            virtual BlackMisc::Network::CServerList getVatsimFsdServers() const override;
            virtual BlackMisc::Network::CClientList getOtherClientsForCallsigns(const BlackMisc::Aviation::CCallsignSet &callsigns) const override;
            virtual void requestDataUpdates()override;
            virtual void requestAtisUpdates() override;
            virtual bool updateAircraftEnabled(const BlackMisc::Aviation::CCallsign &callsign, bool enabledForRedering) override;
            virtual bool updateAircraftModel(const BlackMisc::Aviation::CCallsign &callsign, const BlackMisc::Simulation::CAircraftModel &model, const BlackMisc::CIdentifier &originator) override;
            virtual bool updateAircraftNetworkModel(const BlackMisc::Aviation::CCallsign &callsign, const BlackMisc::Simulation::CAircraftModel &model, const BlackMisc::CIdentifier &originator) override;
            virtual bool updateFastPositionEnabled(const BlackMisc::Aviation::CCallsign &callsign, bool enableFastPositionSending) override;
            virtual void setFastPositionEnabledCallsigns(BlackMisc::Aviation::CCallsignSet &callsigns) override;
            virtual BlackMisc::Aviation::CCallsignSet getFastPositionEnabledCallsigns() const override;
            virtual BlackMisc::CStatusMessageList getReverseLookupMessages(const BlackMisc::Aviation::CCallsign &callsign) const override;
            virtual bool isReverseLookupMessagesEnabled() const override;
            virtual void enableReverseLookupMessages(bool enabled) override;
            virtual BlackMisc::CStatusMessageList getAircraftPartsHistory(const BlackMisc::Aviation::CCallsign &callsign) const override;
            virtual BlackMisc::Aviation::CAircraftPartsList getRemoteAircraftParts(const BlackMisc::Aviation::CCallsign &callsign, qint64 cutoffTimeValuesBefore) const override;
            virtual bool isAircraftPartsHistoryEnabled() const override;
            virtual void enableAircraftPartsHistory(bool enabled) override;
            virtual void testCreateDummyOnlineAtcStations(int number) override;
            virtual void testAddAircraftParts(const BlackMisc::Aviation::CCallsign &callsign, const BlackMisc::Aviation::CAircraftParts &parts, bool incremental) override;
            virtual void testReceivedTextMessages(const BlackMisc::Network::CTextMessageList &textMessages) override;
            virtual void testRequestAircraftConfig(const BlackMisc::Aviation::CCallsign &callsign) override;
            //! @}

        private:
            BlackMisc::CGenericDBusInterface *m_dBusInterface; /*!< DBus interface */

            //! Relay connection signals to local signals.
            void relaySignals(const QString &serviceName, QDBusConnection &connection);

        protected:
            //! Constructor
            CContextNetworkProxy(CCoreFacadeConfig::ContextMode mode, CCoreFacade *runtime) : IContextNetwork(mode, runtime), m_dBusInterface(nullptr) {}

            //! DBus version constructor
            CContextNetworkProxy(const QString &serviceName, QDBusConnection &connection, CCoreFacadeConfig::ContextMode mode, CCoreFacade *runtime);
        };
    } // ns
} // ns
#endif // guard

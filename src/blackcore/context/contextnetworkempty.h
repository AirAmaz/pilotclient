/* Copyright (C) 2013
 * swift Project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file

#ifndef BLACKCORE_CONTEXT_CONTEXTNETWORK_EMPTY_H
#define BLACKCORE_CONTEXT_CONTEXTNETWORK_EMPTY_H

#include "blackcoreexport.h"
#include "contextnetwork.h"
#include "blackmisc/logmessage.h"

namespace BlackCore
{
    namespace Context
    {
        //! Empty context, used during shutdown/initialization
        class BLACKCORE_EXPORT CContextNetworkEmpty : public IContextNetwork
        {
            Q_OBJECT

        public:
            //! Constructor
            CContextNetworkEmpty(CCoreFacade *runtime) : IContextNetwork(CCoreFacadeConfig::NotUsed, runtime) {}

        public slots: // IContextNetwork overrides

            //! \copydoc IContextNetwork::readAtcBookingsFromSource()
            virtual void readAtcBookingsFromSource() const override
            {
                logEmptyContextWarning(Q_FUNC_INFO);
            }

            //! \copydoc IContextNetwork::getAtcStationsOnline()
            virtual BlackMisc::Aviation::CAtcStationList getAtcStationsOnline() const override
            {
                logEmptyContextWarning(Q_FUNC_INFO);
                return BlackMisc::Aviation::CAtcStationList();
            }

            //! \copydoc IContextNetwork::getAtcStationsBooked()
            virtual BlackMisc::Aviation::CAtcStationList getAtcStationsBooked() const override
            {
                logEmptyContextWarning(Q_FUNC_INFO);
                return BlackMisc::Aviation::CAtcStationList();
            }

            //! \copydoc IContextNetwork::getAircraftInRange()
            virtual BlackMisc::Simulation::CSimulatedAircraftList getAircraftInRange() const override
            {
                logEmptyContextWarning(Q_FUNC_INFO);
                return BlackMisc::Simulation::CSimulatedAircraftList();
            }

            //! \copydoc IContextNetwork::getAircraftInRangeForCallsign
            virtual BlackMisc::Simulation::CSimulatedAircraft getAircraftInRangeForCallsign(const BlackMisc::Aviation::CCallsign &callsign) const override
            {
                Q_UNUSED(callsign);
                logEmptyContextWarning(Q_FUNC_INFO);
                return BlackMisc::Simulation::CSimulatedAircraft();
            }

            //! \copydoc IContextNetwork::getAircraftInRangeCallsigns()
            virtual BlackMisc::Aviation::CCallsignSet getAircraftInRangeCallsigns() const override
            {
                logEmptyContextWarning(Q_FUNC_INFO);
                return BlackMisc::Aviation::CCallsignSet();
            }

            //! \copydoc IContextNetwork::getAircraftInRangeCount
            virtual int getAircraftInRangeCount() const override
            {
                logEmptyContextWarning(Q_FUNC_INFO);
                return 0;
            }

            //! \copydoc IContextNetwork::getOnlineStationForCallsign
            virtual BlackMisc::Aviation::CAtcStation getOnlineStationForCallsign(const BlackMisc::Aviation::CCallsign &callsign) const override
            {
                Q_UNUSED(callsign);
                logEmptyContextWarning(Q_FUNC_INFO);
                return BlackMisc::Aviation::CAtcStation();
            }

            //! \copydoc IContextNetwork::connectToNetwork
            virtual BlackMisc::CStatusMessage connectToNetwork(const BlackMisc::Network::CServer &server, BlackCore::INetwork::LoginMode mode) override
            {
                Q_UNUSED(mode);
                Q_UNUSED(server);
                logEmptyContextWarning(Q_FUNC_INFO);
                return statusMessageEmptyContext();
            }

            //! \copydoc IContextNetwork::disconnectFromNetwork()
            virtual BlackMisc::CStatusMessage disconnectFromNetwork() override
            {
                logEmptyContextWarning(Q_FUNC_INFO);
                return statusMessageEmptyContext();
            }

            //! \copydoc IContextNetwork::isConnected()
            virtual bool isConnected() const override
            {
                logEmptyContextWarning(Q_FUNC_INFO);
                return false;
            }

            //! \copydoc IContextNetwork::getConnectedServer
            virtual BlackMisc::Network::CServer getConnectedServer() const override
            {
                logEmptyContextWarning(Q_FUNC_INFO);
                return BlackMisc::Network::CServer();
            }

            //! \copydoc IContextNetwork::sendTextMessages()
            virtual void sendTextMessages(const BlackMisc::Network::CTextMessageList &textMessages) override
            {
                Q_UNUSED(textMessages);
                logEmptyContextWarning(Q_FUNC_INFO);
            }

            //! \copydoc IContextNetwork::sendFlightPlan()
            virtual void sendFlightPlan(const BlackMisc::Aviation::CFlightPlan &flightPlan) override
            {
                Q_UNUSED(flightPlan);
                logEmptyContextWarning(Q_FUNC_INFO);
            }

            //! \copydoc IContextNetwork::loadFlightPlanFromNetwork()
            virtual BlackMisc::Aviation::CFlightPlan loadFlightPlanFromNetwork(const BlackMisc::Aviation::CCallsign &callsign) const override
            {
                Q_UNUSED(callsign);
                logEmptyContextWarning(Q_FUNC_INFO);
                return BlackMisc::Aviation::CFlightPlan();
            }

            //! \copydoc IContextNetwork::getMetarForAirport
            BlackMisc::Weather::CMetar getMetarForAirport(const BlackMisc::Aviation::CAirportIcaoCode &airportIcaoCode) const override
            {
                Q_UNUSED(airportIcaoCode);
                logEmptyContextWarning(Q_FUNC_INFO);
                return {};
            }

            //! \copydoc IContextNetwork::getSelectedVoiceRooms()
            virtual BlackMisc::Audio::CVoiceRoomList getSelectedVoiceRooms() const override
            {
                logEmptyContextWarning(Q_FUNC_INFO);
                return BlackMisc::Audio::CVoiceRoomList();
            }

            //! \copydoc IContextNetwork::getSelectedAtcStations
            virtual BlackMisc::Aviation::CAtcStationList getSelectedAtcStations() const override
            {
                logEmptyContextWarning(Q_FUNC_INFO);
                // normally 2 entries
                return BlackMisc::Aviation::CAtcStationList({ BlackMisc::Aviation::CAtcStation(), BlackMisc::Aviation::CAtcStation()});
            }

            //! \copydoc IContextNetwork::getUsers()
            virtual BlackMisc::Network::CUserList getUsers() const override
            {
                logEmptyContextWarning(Q_FUNC_INFO);
                return BlackMisc::Network::CUserList();
            }

            //! \copydoc IContextNetwork::getUsersForCallsigns
            virtual BlackMisc::Network::CUserList getUsersForCallsigns(const BlackMisc::Aviation::CCallsignSet &callsigns) const override
            {
                Q_UNUSED(callsigns);
                logEmptyContextWarning(Q_FUNC_INFO);
                return BlackMisc::Network::CUserList();
            }

            //! \copydoc IContextNetwork::getUserForCallsign
            virtual BlackMisc::Network::CUser getUserForCallsign(const BlackMisc::Aviation::CCallsign &callsign) const override
            {
                Q_UNUSED(callsign);
                logEmptyContextWarning(Q_FUNC_INFO);
                return BlackMisc::Network::CUser();
            }

            //! \copydoc IContextNetwork::getOtherClients
            virtual BlackMisc::Network::CClientList getOtherClients() const override
            {
                logEmptyContextWarning(Q_FUNC_INFO);
                return BlackMisc::Network::CClientList();
            }

            //! \copydoc IContextNetwork::getOtherClientsForCallsigns
            virtual BlackMisc::Network::CClientList getOtherClientsForCallsigns(const BlackMisc::Aviation::CCallsignSet &callsigns) const override
            {
                Q_UNUSED(callsigns);
                logEmptyContextWarning(Q_FUNC_INFO);
                return BlackMisc::Network::CClientList();
            }

            //! \copydoc IContextNetwork::requestDataUpdates
            virtual void requestDataUpdates()override
            {
                logEmptyContextWarning(Q_FUNC_INFO);
            }

            //! \copydoc IContextNetwork::requestAtisUpdates
            virtual void requestAtisUpdates() override
            {
                logEmptyContextWarning(Q_FUNC_INFO);
            }

            //! \copydoc IContextNetwork::testCreateDummyOnlineAtcStations
            virtual void testCreateDummyOnlineAtcStations(int number) override
            {
                Q_UNUSED(number);
                logEmptyContextWarning(Q_FUNC_INFO);
            }

            //! \copydoc IContextNetwork::testAddAircraftParts
            virtual void testAddAircraftParts(const BlackMisc::Aviation::CCallsign &callsign, const BlackMisc::Aviation::CAircraftParts &parts, bool incremental) override
            {
                Q_UNUSED(callsign);
                Q_UNUSED(parts);
                Q_UNUSED(incremental);
                logEmptyContextWarning(Q_FUNC_INFO);
            }

            //! \copydoc IContextNetwork::parseCommandLine
            virtual bool parseCommandLine(const QString &commandLine, const BlackMisc::CIdentifier &originator) override
            {
                Q_UNUSED(commandLine);
                Q_UNUSED(originator);
                logEmptyContextWarning(Q_FUNC_INFO);
                return false;
            }

            //! \copydoc IContextNetwork::getVatsimVoiceServers
            virtual BlackMisc::Network::CServerList getVatsimVoiceServers() const override
            {
                logEmptyContextWarning(Q_FUNC_INFO);
                return BlackMisc::Network::CServerList();
            }

            //! \copydoc IContextNetwork::getVatsimFsdServers
            virtual BlackMisc::Network::CServerList getVatsimFsdServers() const override
            {
                logEmptyContextWarning(Q_FUNC_INFO);
                return BlackMisc::Network::CServerList();
            }

            //! \copydoc IContextNetwork::updateAircraftEnabled
            virtual bool updateAircraftEnabled(const BlackMisc::Aviation::CCallsign &callsign, bool enabledForRedering) override
            {
                Q_UNUSED(callsign);
                Q_UNUSED(enabledForRedering);
                logEmptyContextWarning(Q_FUNC_INFO);
                return false;
            }

            //! \copydoc IContextNetwork::updateAircraftModel
            virtual bool updateAircraftModel(const BlackMisc::Aviation::CCallsign &callsign, const BlackMisc::Simulation::CAircraftModel &model, const BlackMisc::CIdentifier &originator) override
            {
                Q_UNUSED(callsign);
                Q_UNUSED(model);
                Q_UNUSED(originator);
                logEmptyContextWarning(Q_FUNC_INFO);
                return false;
            }

            //! \copydoc IContextNetwork::updateAircraftNetworkModel
            virtual bool updateAircraftNetworkModel(const BlackMisc::Aviation::CCallsign &callsign, const BlackMisc::Simulation::CAircraftModel &model, const BlackMisc::CIdentifier &originator) override
            {
                Q_UNUSED(callsign);
                Q_UNUSED(model);
                Q_UNUSED(originator);
                logEmptyContextWarning(Q_FUNC_INFO);
                return false;
            }

            //! \copydoc IContextNetwork::updateFastPositionEnabled
            virtual bool updateFastPositionEnabled(const BlackMisc::Aviation::CCallsign &callsign, bool enableFastPositionSending) override
            {
                logEmptyContextWarning(Q_FUNC_INFO);
                Q_UNUSED(callsign);
                Q_UNUSED(enableFastPositionSending);
                return false;
            }

            //! \copydoc IContextNetwork::setFastPositionEnabledCallsigns
            virtual void setFastPositionEnabledCallsigns(BlackMisc::Aviation::CCallsignSet &callsigns) override
            {
                Q_UNUSED(callsigns);
                logEmptyContextWarning(Q_FUNC_INFO);
            }

            //! \copydoc IContextNetwork::getFastPositionEnabledCallsigns
            virtual BlackMisc::Aviation::CCallsignSet getFastPositionEnabledCallsigns() override
            {
                logEmptyContextWarning(Q_FUNC_INFO);
                return BlackMisc::Aviation::CCallsignSet();
            }

            //! \copydoc IContextNetwork::getReverseLookupMessages
            virtual BlackMisc::CStatusMessageList getReverseLookupMessages(const BlackMisc::Aviation::CCallsign &callsign) const override
            {
                logEmptyContextWarning(Q_FUNC_INFO);
                Q_UNUSED(callsign);
                return BlackMisc::CStatusMessageList();
            }

            //! \copydoc IContextNetwork::isReverseLookupMessagesEnabled
            virtual bool isReverseLookupMessagesEnabled() const override
            {
                logEmptyContextWarning(Q_FUNC_INFO);
                return false;
            }

            //! \copydoc IContextNetwork::enableReverseLookupMessages
            virtual void enableReverseLookupMessages(bool enabled) override
            {
                logEmptyContextWarning(Q_FUNC_INFO);
                Q_UNUSED(enabled);
            }
        };
    } // namespace
} // namespace
#endif // guard

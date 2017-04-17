/* Copyright (C) 2013
 * swift Project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "simconnectdatadefinition.h"
#include "blackmisc/logmessage.h"
#include <tuple>

using namespace BlackMisc;
using namespace BlackMisc::Aviation;

namespace BlackSimPlugin
{
    namespace FsxCommon
    {
        CSimConnectDefinitions::CSimConnectDefinitions() { }

        HRESULT CSimConnectDefinitions::initDataDefinitionsWhenConnected(const HANDLE hSimConnect)
        {
            HRESULT hr = S_OK;
            hr += initOwnAircraft(hSimConnect);
            hr += initRemoteAircraft(hSimConnect);
            hr += initRemoteAircraftSimData(hSimConnect);
            hr += initSimulatorEnvironment(hSimConnect);
            hr += initSbDataArea(hSimConnect);
            return hr;
        }

        HRESULT CSimConnectDefinitions::initOwnAircraft(const HANDLE hSimConnect)
        {
            HRESULT hr = S_OK;
            hr +=  SimConnect_AddToDataDefinition(hSimConnect, CSimConnectDefinitions::DataOwnAircraft, "PLANE LATITUDE", "Degrees");
            hr +=  SimConnect_AddToDataDefinition(hSimConnect, CSimConnectDefinitions::DataOwnAircraft, "PLANE LONGITUDE", "Degrees");
            hr +=  SimConnect_AddToDataDefinition(hSimConnect, CSimConnectDefinitions::DataOwnAircraft, "PLANE ALTITUDE", "Feet");
            hr +=  SimConnect_AddToDataDefinition(hSimConnect, CSimConnectDefinitions::DataOwnAircraft, "PLANE ALT ABOVE GROUND", "Feet");
            hr +=  SimConnect_AddToDataDefinition(hSimConnect, CSimConnectDefinitions::DataOwnAircraft, "STATIC CG TO GROUND", "Feet");
            hr +=  SimConnect_AddToDataDefinition(hSimConnect, CSimConnectDefinitions::DataOwnAircraft, "PLANE HEADING DEGREES TRUE", "Degrees");
            hr +=  SimConnect_AddToDataDefinition(hSimConnect, CSimConnectDefinitions::DataOwnAircraft, "PLANE PITCH DEGREES", "Degrees");
            hr +=  SimConnect_AddToDataDefinition(hSimConnect, CSimConnectDefinitions::DataOwnAircraft, "PLANE BANK DEGREES", "Degrees");
            hr +=  SimConnect_AddToDataDefinition(hSimConnect, CSimConnectDefinitions::DataOwnAircraft, "GROUND VELOCITY", "Knots");
            hr +=  SimConnect_AddToDataDefinition(hSimConnect, CSimConnectDefinitions::DataOwnAircraft, "GROUND ALTITUDE", "Feet");
            hr +=  SimConnect_AddToDataDefinition(hSimConnect, CSimConnectDefinitions::DataOwnAircraft, "SIM ON GROUND", "Bool");
            hr +=  SimConnect_AddToDataDefinition(hSimConnect, CSimConnectDefinitions::DataOwnAircraft, "LIGHT STROBE", "Bool");
            hr +=  SimConnect_AddToDataDefinition(hSimConnect, CSimConnectDefinitions::DataOwnAircraft, "LIGHT LANDING", "Bool");
            hr +=  SimConnect_AddToDataDefinition(hSimConnect, CSimConnectDefinitions::DataOwnAircraft, "LIGHT TAXI", "Bool");
            hr +=  SimConnect_AddToDataDefinition(hSimConnect, CSimConnectDefinitions::DataOwnAircraft, "LIGHT BEACON", "Bool");
            hr +=  SimConnect_AddToDataDefinition(hSimConnect, CSimConnectDefinitions::DataOwnAircraft, "LIGHT NAV", "Bool");
            hr +=  SimConnect_AddToDataDefinition(hSimConnect, CSimConnectDefinitions::DataOwnAircraft, "LIGHT LOGO", "Bool");
            hr +=  SimConnect_AddToDataDefinition(hSimConnect, CSimConnectDefinitions::DataOwnAircraft, "TRANSPONDER CODE:1", NULL);
            hr +=  SimConnect_AddToDataDefinition(hSimConnect, CSimConnectDefinitions::DataOwnAircraft, "COM ACTIVE FREQUENCY:1", "MHz");
            hr +=  SimConnect_AddToDataDefinition(hSimConnect, CSimConnectDefinitions::DataOwnAircraft, "COM ACTIVE FREQUENCY:2", "MHz");
            hr +=  SimConnect_AddToDataDefinition(hSimConnect, CSimConnectDefinitions::DataOwnAircraft, "COM STANDBY FREQUENCY:1", "MHz");
            hr +=  SimConnect_AddToDataDefinition(hSimConnect, CSimConnectDefinitions::DataOwnAircraft, "COM STANDBY FREQUENCY:2", "MHz");
            hr +=  SimConnect_AddToDataDefinition(hSimConnect, CSimConnectDefinitions::DataOwnAircraft, "FLAPS HANDLE PERCENT", "Percent Over 100");
            hr +=  SimConnect_AddToDataDefinition(hSimConnect, CSimConnectDefinitions::DataOwnAircraft, "SPOILERS HANDLE POSITION", "Percent Over 100");
            hr +=  SimConnect_AddToDataDefinition(hSimConnect, CSimConnectDefinitions::DataOwnAircraft, "GEAR HANDLE POSITION", "Bool");
            hr +=  SimConnect_AddToDataDefinition(hSimConnect, CSimConnectDefinitions::DataOwnAircraft, "NUMBER OF ENGINES", "Number");
            // Simconnect supports index 1 - 4
            hr +=  SimConnect_AddToDataDefinition(hSimConnect, CSimConnectDefinitions::DataOwnAircraft, "GENERAL ENG COMBUSTION:1", "Bool");
            hr +=  SimConnect_AddToDataDefinition(hSimConnect, CSimConnectDefinitions::DataOwnAircraft, "GENERAL ENG COMBUSTION:2", "Bool");
            hr +=  SimConnect_AddToDataDefinition(hSimConnect, CSimConnectDefinitions::DataOwnAircraft, "GENERAL ENG COMBUSTION:3", "Bool");
            hr +=  SimConnect_AddToDataDefinition(hSimConnect, CSimConnectDefinitions::DataOwnAircraft, "GENERAL ENG COMBUSTION:4", "Bool");
            hr +=  SimConnect_AddToDataDefinition(hSimConnect, CSimConnectDefinitions::DataOwnAircraftTitle, "TITLE", NULL, SIMCONNECT_DATATYPE_STRING256);
            if (hr != S_OK)
            {
                CLogMessage(static_cast<CSimConnectDefinitions *>(nullptr)).error("SimConnect error: initOwnAircraft %1") << hr;
            }
            return hr;
        }

        HRESULT CSimConnectDefinitions::initRemoteAircraft(const HANDLE hSimConnect)
        {
            HRESULT hr = S_OK;
            // Position
            hr += SimConnect_AddToDataDefinition(hSimConnect, CSimConnectDefinitions::DataRemoteAircraftPosition, "Initial Position", NULL, SIMCONNECT_DATATYPE_INITPOSITION);

            // Hint: "Bool" and "Percent .." are units name
            // default data type is SIMCONNECT_DATATYPE_FLOAT64 -> double

            // Flaps
            hr += SimConnect_AddToDataDefinition(hSimConnect, CSimConnectDefinitions::DataRemoteAircraftParts, "LEADING EDGE FLAPS LEFT PERCENT", "Percent Over 100");
            hr += SimConnect_AddToDataDefinition(hSimConnect, CSimConnectDefinitions::DataRemoteAircraftParts, "LEADING EDGE FLAPS RIGHT PERCENT", "Percent Over 100");
            hr += SimConnect_AddToDataDefinition(hSimConnect, CSimConnectDefinitions::DataRemoteAircraftParts, "TRAILING EDGE FLAPS LEFT PERCENT", "Percent Over 100");
            hr += SimConnect_AddToDataDefinition(hSimConnect, CSimConnectDefinitions::DataRemoteAircraftParts, "TRAILING EDGE FLAPS RIGHT PERCENT", "Percent Over 100");

            // Gear & Spoiler
            hr += SimConnect_AddToDataDefinition(hSimConnect, CSimConnectDefinitions::DataRemoteAircraftParts, "GEAR HANDLE POSITION", "Bool");
            hr += SimConnect_AddToDataDefinition(hSimConnect, CSimConnectDefinitions::DataRemoteAircraftParts, "SPOILERS HANDLE POSITION", "Percent Over 100");

            // Engines
            hr += SimConnect_AddToDataDefinition(hSimConnect, CSimConnectDefinitions::DataRemoteAircraftParts, "GENERAL ENG COMBUSTION:1", "Bool");
            hr += SimConnect_AddToDataDefinition(hSimConnect, CSimConnectDefinitions::DataRemoteAircraftParts, "GENERAL ENG COMBUSTION:2", "Bool");
            hr += SimConnect_AddToDataDefinition(hSimConnect, CSimConnectDefinitions::DataRemoteAircraftParts, "GENERAL ENG COMBUSTION:3", "Bool");
            hr += SimConnect_AddToDataDefinition(hSimConnect, CSimConnectDefinitions::DataRemoteAircraftParts, "GENERAL ENG COMBUSTION:4", "Bool");

            // Lights (other definition)
            hr += SimConnect_AddToDataDefinition(hSimConnect, CSimConnectDefinitions::DataRemoteAircraftLights, "LIGHT STROBE", "Bool");
            hr += SimConnect_AddToDataDefinition(hSimConnect, CSimConnectDefinitions::DataRemoteAircraftLights, "LIGHT LANDING", "Bool");
            hr += SimConnect_AddToDataDefinition(hSimConnect, CSimConnectDefinitions::DataRemoteAircraftLights, "LIGHT TAXI", "Bool");
            hr += SimConnect_AddToDataDefinition(hSimConnect, CSimConnectDefinitions::DataRemoteAircraftLights, "LIGHT BEACON", "Bool");
            hr += SimConnect_AddToDataDefinition(hSimConnect, CSimConnectDefinitions::DataRemoteAircraftLights, "LIGHT NAV", "Bool");
            hr += SimConnect_AddToDataDefinition(hSimConnect, CSimConnectDefinitions::DataRemoteAircraftLights, "LIGHT LOGO", "Bool");
            hr += SimConnect_AddToDataDefinition(hSimConnect, CSimConnectDefinitions::DataRemoteAircraftLights, "LIGHT NAV", "Bool");
            hr += SimConnect_AddToDataDefinition(hSimConnect, CSimConnectDefinitions::DataRemoteAircraftLights, "LIGHT RECOGNITION", "Bool");
            hr += SimConnect_AddToDataDefinition(hSimConnect, CSimConnectDefinitions::DataRemoteAircraftLights, "LIGHT CABIN", "Bool");

            if (hr != S_OK)
            {
                CLogMessage(static_cast<CSimConnectDefinitions *>(nullptr)).error("SimConnect error: initRemoteAircraftSituation %1") << hr;
            }
            return hr;
        }

        HRESULT CSimConnectDefinitions::initRemoteAircraftSimData(const HANDLE hSimConnect)
        {
            HRESULT hr = S_OK;
            hr += SimConnect_AddToDataDefinition(hSimConnect, CSimConnectDefinitions::DataRemoteAircraftSimData, "PLANE LATITUDE", "degrees");
            hr += SimConnect_AddToDataDefinition(hSimConnect, CSimConnectDefinitions::DataRemoteAircraftSimData, "PLANE LONGITUDE", "degrees");
            hr += SimConnect_AddToDataDefinition(hSimConnect, CSimConnectDefinitions::DataRemoteAircraftSimData, "PLANE ALTITUDE", "Feet");
            hr += SimConnect_AddToDataDefinition(hSimConnect, CSimConnectDefinitions::DataRemoteAircraftSimData, "GROUND ALTITUDE", "Feet");
            hr += SimConnect_AddToDataDefinition(hSimConnect, CSimConnectDefinitions::DataRemoteAircraftSimData, "STATIC CG TO GROUND", "Feet");
            if (hr != S_OK)
            {
                CLogMessage(static_cast<CSimConnectDefinitions *>(nullptr)).error("SimConnect error: initRemoteAircraftSimData %1") << hr;
            }
            return hr;
        }

        HRESULT CSimConnectDefinitions::initSimulatorEnvironment(const HANDLE hSimConnect)
        {
            HRESULT hr = S_OK;
            hr += SimConnect_AddToDataDefinition(hSimConnect, CSimConnectDefinitions::DataSimEnvironment, "ZULU TIME", "", SIMCONNECT_DATATYPE_INT32);
            hr += SimConnect_AddToDataDefinition(hSimConnect, CSimConnectDefinitions::DataSimEnvironment, "LOCAL TIME", "", SIMCONNECT_DATATYPE_INT32);
            if (hr != S_OK)
            {
                CLogMessage(static_cast<CSimConnectDefinitions *>(nullptr)).error("SimConnect error: initSimulatorEnvironment %1") << hr;
            }
            return hr;
        }

        HRESULT CSimConnectDefinitions::initSbDataArea(const HANDLE hSimConnect)
        {
            HRESULT hr = S_OK;
            DWORD sbSize = sizeof(DataDefinitionClientAreaSb);

            // We need to know the client area 'name' and map it to a client ID
            hr += SimConnect_MapClientDataNameToID(hSimConnect, "SquawkBox Data", ClientAreaSquawkBox);
            if (hr != S_OK)
            {
                CLogMessage(static_cast<CSimConnectDefinitions *>(nullptr)).error("SimConnect error: SimConnect_MapClientDataNameToID %1") << hr;
                return hr;
            }

            // Mapping needs to be first
            hr += SimConnect_CreateClientData(hSimConnect, ClientAreaSquawkBox, sbSize, SIMCONNECT_CREATE_CLIENT_DATA_FLAG_DEFAULT);
            if (hr != S_OK)
            {
                CLogMessage(static_cast<CSimConnectDefinitions *>(nullptr)).error("SimConnect error: SimConnect_CreateClientData %1") << hr;
                return hr;
            }

            hr += SimConnect_AddToClientDataDefinition(hSimConnect, CSimConnectDefinitions::DataClientAreaSb, 0, sbSize);
            hr += SimConnect_AddToClientDataDefinition(hSimConnect, CSimConnectDefinitions::DataClientAreaSbStandby, 17, 1);
            hr += SimConnect_AddToClientDataDefinition(hSimConnect, CSimConnectDefinitions::DataClientAreaSbIdent, 19, 1);
            if (hr != S_OK)
            {
                CLogMessage(static_cast<CSimConnectDefinitions *>(nullptr)).error("SimConnect error: SB data area data definitions %1") << hr;
                return hr;
            }

            // write a default client area so we are not suddenly squawking ident or so
            DataDefinitionClientAreaSb sbArea;
            sbArea.setDefaultValues();
            hr += SimConnect_SetClientData(hSimConnect, ClientAreaSquawkBox, CSimConnectDefinitions::DataClientAreaSb, SIMCONNECT_CLIENT_DATA_REQUEST_FLAG_DEFAULT, 0, sbSize, &sbArea);
            if (hr != S_OK)
            {
                CLogMessage(static_cast<CSimConnectDefinitions *>(nullptr)).error("SimConnect error: SimConnect_SetClientData %1") << hr;
            }
            return hr;
        }

        bool DataDefinitionRemoteAircraftPartsWithoutLights::operator==(const DataDefinitionRemoteAircraftPartsWithoutLights &rhs) const
        {
            return std::tie(flapsLeadingEdgeLeftPercent, flapsLeadingEdgeRightPercent, flapsTrailingEdgeLeftPercent, flapsTrailingEdgeRightPercent,
                            gearHandlePosition, spoilersHandlePosition,
                            engine1Combustion, engine2Combustion, engine3Combustion, engine4Combustion) ==
                   std::tie(rhs.flapsLeadingEdgeLeftPercent, rhs.flapsLeadingEdgeRightPercent, rhs.flapsTrailingEdgeLeftPercent, rhs.flapsTrailingEdgeRightPercent,
                            rhs.gearHandlePosition, rhs.spoilersHandlePosition,
                            rhs.engine1Combustion, rhs.engine2Combustion, rhs.engine3Combustion, rhs.engine4Combustion);
        }

        void DataDefinitionRemoteAircraftPartsWithoutLights::setAllEngines(bool on)
        {
            engine1Combustion = on ? 1 : 0;
            engine2Combustion = on ? 1 : 0;
            engine3Combustion = on ? 1 : 0;
            engine4Combustion = on ? 1 : 0;
        }

        void DataDefinitionRemoteAircraftPartsWithoutLights::resetAllFlaps()
        {
            flapsLeadingEdgeLeftPercent = 0;
            flapsLeadingEdgeRightPercent = 0;
            flapsTrailingEdgeLeftPercent = 0;
            flapsTrailingEdgeRightPercent = 0;
        }

        void DataDefinitionRemoteAircraftPartsWithoutLights::resetSpoilers()
        {
            spoilersHandlePosition = 0;
        }

        void DataDefinitionRemoteAircraftPartsWithoutLights::resetToInvalid()
        {
            flapsLeadingEdgeLeftPercent = -1;
            flapsLeadingEdgeRightPercent = -1;
            flapsTrailingEdgeLeftPercent = -1;
            flapsTrailingEdgeRightPercent = -1;
            gearHandlePosition = -1;
            spoilersHandlePosition = -1;
            engine1Combustion = -1;
            engine2Combustion = -1;
            engine3Combustion = -1;
            engine4Combustion = -1;
        }

        CAircraftLights DataDefinitionRemoteAircraftLights::toLights() const
        {
            return CAircraftLights(lightStrobe, lightLanding, lightTaxi, lightBeacon, lightNav, lightLogo, lightRecognition, lightCabin);
        }
    } // namespace
} // namespace

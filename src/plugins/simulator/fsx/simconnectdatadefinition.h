/* Copyright (C) 2013
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file

#ifndef BLACKSIMPLUGIN_FSX_SIMCONNECT_DATADEFINITION_H
#define BLACKSIMPLUGIN_FSX_SIMCONNECT_DATADEFINITION_H

#include <QtGlobal>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <simconnect/SimConnect.h>
#include <windows.h>
#include <algorithm>
#include <QString>

namespace BlackSimPlugin
{
    namespace Fsx
    {
        //! Data struct of our own aircraft
        //! \sa SimConnect variables http://msdn.microsoft.com/en-us/library/cc526981.aspx
        //! \sa SimConnect events http://msdn.microsoft.com/en-us/library/cc526980.aspx
        struct DataDefinitionOwnAircraft
        {
            double latitude;        //!< Latitude (deg)
            double longitude;       //!< Longitude (deg)
            double altitude;        //!< Altitude (ft)
            double altitudeAGL;     //!< Altitude above ground (ft)
            double cgToGround;      //!< Static CG to ground (ft)
            double trueHeading;     //!< True heading (deg)
            double pitch;           //!< Pitch (deg)
            double bank;            //!< Bank (deg)
            double velocity;        //!< Ground velocity
            double elevation;       //!< Elevation (ft)
            double simOnGround;     //!< Is aircraft on ground?

            double lightStrobe;     //!< Is strobe light on?
            double lightLanding;    //!< Is landing light on?
            double lightTaxi;       //!< Is taxi light on?
            double lightBeacon;     //!< Is beacon light on?
            double lightNav;        //!< Is nav light on?
            double lightLogo;       //!< Is logo light on?

            double transponderCode; //!< Transponder Code
            double com1ActiveMHz;   //!< COM1 active frequency
            double com2ActiveMHz;   //!< COM2 active frequency
            double com1StandbyMHz;  //!< COM1 standby frequency
            double com2StandbyMHz;  //!< COM1 standby frequency

            double flapsHandlePosition;     //!< Flaps handle position in percent
            double spoilersHandlePosition;  //!< Spoilers out?
            double gearHandlePosition;      //!< Gear handle position

            double numberOfEngines;     //!< Number of engines
            double engine1Combustion;   //!< Engine 1 combustion flag
            double engine2Combustion;   //!< Engine 2 combustion flag
            double engine3Combustion;   //!< Engine 3 combustion flag
            double engine4Combustion;   //!< Engine 4 combustion flag
        };

        //! Data struct of aircraft position
        struct DataDefinitionOwnAircraftModel
        {
            char title[256]; //!< Aircraft model string
        };

        //! Data struct of remote aircraft parts
        struct DataDefinitionRemoteAircraftParts
        {
            double lightStrobe;                   //!< Is strobe light on?
            double lightLanding;                  //!< Is landing light on?
            // double lightTaxi;                  //!< Is taxi light on?
            double lightBeacon;                   //!< Is beacon light on?
            double lightNav;                      //!< Is nav light on?
            double lightLogo;                     //!< Is logo light on?
            double flapsLeadingEdgeLeftPercent;   //!< Leading edge left in percent
            double flapsLeadingEdgeRightPercent;  //!< Leading edge right in percent
            double flapsTrailingEdgeLeftPercent;  //!< Trailing edge left in percent
            double flapsTrailingEdgeRightPercent; //!< Trailing edge right in percent
            double gearHandlePosition;            //!< Gear handle position
            double spoilersHandlePosition;        //!< Spoilers out?
            double engine1Combustion;             //!< Engine 1 combustion flag
            double engine2Combustion;             //!< Engine 2 combustion flag
            double engine3Combustion;             //!< Engine 3 combustion flag
            double engine4Combustion;             //!< Engine 4 combustion flag

            //! Equal to other parts
            bool operator==(const DataDefinitionRemoteAircraftParts &rhs) const;
        };

        //! Data for AI object sent back from simulator
        struct DataDefinitionRemoteAircraftSimData
        {
            double latitude;   //!< Latitude (deg)
            double longitude;  //!< Longitude (deg)
            double altitude;   //!< Altitude (ft)
            double elevation;  //!< Elevation (ft)
            double cgToGround; //!< Static CG to ground (ft)

            //! Above ground ft
            double aboveGround() const { return altitude - elevation; }
        };

        //! Data struct simulator environment
        struct DataDefinitionSimEnvironment
        {
            qint32 zuluTimeSeconds;  //!< Simulator zulu (GMT) ime in secs.
            qint32 localTimeSeconds; //!< Simulator local time in secs.
        };

        //! The whole SB data area
        struct DataDefinitionClientAreaSb
        {
            byte data[128] {}; //!< 128 bytes of data, offsets http://www.squawkbox.ca/doc/sdk/fsuipc.php

            //! Standby = 1, else 0
            byte getTransponderMode() const { return data[17]; }

            //! Ident = 1, else 0
            byte getIdent() const { return data[19]; }

            //! Ident?
            bool isIdent() const { return getIdent() != 0; }

            //! Standby
            bool isStandby() const { return getTransponderMode() != 0; }

            //! Set default values
            void setDefaultValues()
            {
                std::fill(data, data + 128, static_cast<byte>(0));
                data[17] = 1; // standby
                data[19] = 0; // no ident
            }
        };

        //! Client areas
        enum ClientAreaId
        {
            ClientAreaSquawkBox
        };

        //! Handles SimConnect data definitions
        class CSimConnectDefinitions
        {
        public:
            //! SimConnect definiton IDs
            enum DataDefiniton
            {
                DataOwnAircraft,
                DataOwnAircraftTitle,
                DataRemoteAircraftParts,
                DataRemoteAircraftPosition,
                DataRemoteAircraftSimData,
                DataSimEnvironment,
                DataClientAreaSb,       //!< whole SB area
                DataClientAreaSbIdent,  //!< ident single value
                DataClientAreaSbStandby //!< standby
            };

            //! SimConnect request IDs
            enum Request
            {
                RequestOwnAircraft,
                RequestRemoveAircraft,
                RequestOwnAircraftTitle,
                RequestSimEnvironment,
                RequestSbData,   //!< SB client area / XPDR mode
                RequestEndMarker //!< free request ids can start here
            };

            //! Constructor
            CSimConnectDefinitions();

            //! Initialize all data definitions
            static HRESULT initDataDefinitionsWhenConnected(const HANDLE hSimConnect);

            //! Log message category
            static QString getLogCategory() { return "swift.fsx.simconnect"; }

        private:
            //! Initialize data definition for our own aircraft
            static HRESULT initOwnAircraft(const HANDLE hSimConnect);

            //! Initialize data definition for remote aircraft
            static HRESULT initRemoteAircraft(const HANDLE hSimConnect);

            //! Initialize data for remote aircraft queried from simulator
            static HRESULT initRemoteAircraftSimData(const HANDLE hSimConnect);

            //! Initialize data definition for Simulator environment
            static HRESULT initSimulatorEnvironment(const HANDLE hSimConnect);

            //! Initialize the SB data are
            static HRESULT initSbDataArea(const HANDLE hSimConnect);
        };
    } // namespace
} // namespace

#endif // guard

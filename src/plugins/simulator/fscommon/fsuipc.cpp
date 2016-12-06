/* Copyright (C) 2014
 * swift Project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "fsuipc.h"
#include <Windows.h>
// bug in FSUIPC_User.h, windows.h not included, so we have to import it first
#include "FSUIPC/FSUIPC_User.h"
#include "FSUIPC/NewWeather.h"

#include "blackmisc/simulation/fscommon/bcdconversions.h"
#include "blackmisc/logmessage.h"
#include <QDebug>
#include <QLatin1Char>
#include <QDateTime>

using namespace BlackMisc;
using namespace BlackMisc::Simulation::FsCommon;
using namespace BlackMisc::Aviation;
using namespace BlackMisc::Network;
using namespace BlackMisc::Geo;
using namespace BlackMisc::Simulation;
using namespace BlackMisc::PhysicalQuantities;
using namespace BlackMisc::Weather;

namespace BlackSimPlugin
{
    namespace FsCommon
    {

        CFsuipc::CFsuipc()
        {
            startTimer(100);
        }

        CFsuipc::~CFsuipc()
        {
            this->disconnect();
        }

        bool CFsuipc::connect()
        {
            DWORD result;
            this->m_lastErrorMessage = "";
            if (this->m_connected) return this->m_connected; // already connected
            if (FSUIPC_Open(SIM_ANY, &result))
            {
                this->m_connected = true;
                int simIndex = static_cast<int>(FSUIPC_FS_Version);
                QString sim(
                    (simIndex >= 0 && simIndex < CFsuipc::simulators().size()) ?
                    CFsuipc::simulators().at(simIndex) :
                    "Unknown FS");
                QString ver("%1.%2.%3.%4%5");
                ver = ver.arg(QLatin1Char(48 + (0x0f & (FSUIPC_Version >> 28))))
                      .arg(QLatin1Char(48 + (0x0f & (FSUIPC_Version >> 24))))
                      .arg(QLatin1Char(48 + (0x0f & (FSUIPC_Version >> 20))))
                      .arg(QLatin1Char(48 + (0x0f & (FSUIPC_Version >> 16))))
                      .arg((FSUIPC_Version & 0xffff) ? "a" + (FSUIPC_Version & 0xff) - 1 : "");
                this->m_fsuipcVersion = QString("FSUIPC %1 (%2)").arg(ver).arg(sim);
//                CLogMessage(this).info("FSUIPC connected: %1") << this->m_fsuipcVersion;
            }
            else
            {
                this->m_connected = false;
                int index = static_cast<int>(result);
                this->m_lastErrorMessage = CFsuipc::errorMessages().at(index);
                CLogMessage(this).info("FSUIPC not connected: %1") << this->m_lastErrorMessage;
            }
            return this->m_connected;
        }

        void CFsuipc::disconnect()
        {
            FSUIPC_Close(); // Closing when it wasn't open is okay, so this is safe here
            this->m_connected = false;
        }

        bool CFsuipc::write(const CSimulatedAircraft &aircraft)
        {
            if (!this->isConnected()) { return false; }

            Q_UNUSED(aircraft);
            //! \todo FSUIPC write values
            return false;
        }

        bool CFsuipc::write(const BlackMisc::Weather::CWeatherGrid &weatherGrid)
        {
            if (!this->isConnected()) { return false; }

            clearAllWeather();

            CGridPoint gridPoint = weatherGrid.front();

            NewWeather nw;
            // Clear new weather
            nw.uCommand = NW_SET;
            nw.uFlags = 0;
            nw.ulSignature = 0;
            nw.uDynamics = 0;
            for (std::size_t i = 0; i < sizeof(nw.uSpare) / sizeof(nw.uSpare[0]); i++) { nw.uSpare[i] = 0; }

            nw.dLatitude = 0.0;
            nw.dLongitude = 0.0;
            nw.nElevation = 0;
            nw.ulTimeStamp = 0;
            nw.nTempCtr = 0;
            nw.nWindsCtr = 0;
            nw.nCloudsCtr = 0;
            nw.nElevation = 0; // metres * 65536;
            nw.nUpperVisCtr = 0;

            // todo: Take station from weather grid
            memcpy(nw.chICAO, "GLOB", 4);

            CVisibilityLayerList visibilityLayers = gridPoint.getVisibilityLayers();
            visibilityLayers.sortBy(&CVisibilityLayer::getBase);
            auto surfaceVisibility = visibilityLayers.frontOrDefault();
            NewVis vis;
            vis.LowerAlt = surfaceVisibility.getBase().value(CLengthUnit::m());
            vis.UpperAlt = surfaceVisibility.getTop().value(CLengthUnit::m());
            // Range is measured in: 1/100ths sm
            vis.Range = surfaceVisibility.getVisibility().value(CLengthUnit::SM()) * 100;
            nw.Vis = vis;

            for (const auto &visibilityLayer : visibilityLayers)
            {
                vis.LowerAlt = visibilityLayer.getBase().value(CLengthUnit::m());
                vis.UpperAlt = visibilityLayer.getTop().value(CLengthUnit::m());
                vis.Range = visibilityLayer.getVisibility().value(CLengthUnit::SM()) * 100;
                nw.UpperVis[nw.nUpperVisCtr++] = vis;
            }

            CTemperatureLayerList temperatureLayers = gridPoint.getTemperatureLayers();
            temperatureLayers.sortBy(&CTemperatureLayer::getLevel);
            for (const auto &temperatureLayer : temperatureLayers)
            {
                NewTemp temp;
                temp.Alt = temperatureLayer.getLevel().value(CLengthUnit::m());
                temp.Day = temperatureLayer.getTemperature().value(CTemperatureUnit::C());
                temp.DayNightVar = 3;
                temp.DewPoint = temperatureLayer.getDewPoint().value(CTemperatureUnit::C());
                nw.Temp[nw.nTempCtr++] = temp;
            }

            CCloudLayerList cloudLayers = gridPoint.getCloudLayers();
            cloudLayers.sortBy(&CCloudLayer::getBase);
            for (const auto &cloudLayer : cloudLayers)
            {
                NewCloud cloud;

                switch (cloudLayer.getCoverage())
                {
                case CCloudLayer::None: cloud.Coverage = 0; break;
                case CCloudLayer::Few: cloud.Coverage = 2; break;
                case CCloudLayer::Scattered: cloud.Coverage = 4; break;
                case CCloudLayer::Broken: cloud.Coverage = 6; break;
                case CCloudLayer::Overcast: cloud.Coverage = 8; break;
                default: cloud.Coverage = 0;
                }

                cloud.Deviation = 0;
                cloud.Icing = 0;
                cloud.LowerAlt = cloudLayer.getBase().value(CLengthUnit::m());
                cloud.PrecipBase = 0;

                // Light rain - when the precipitation rate is < 2.5 mm (0.098 in) per hour
                // Moderate rain - when the precipitation rate is between 2.5 mm (0.098 in) - 7.6 mm (0.30 in) or 10 mm (0.39 in) per hour
                // Heavy rain - when the precipitation rate is > 7.6 mm (0.30 in) per hour, or between 10 mm (0.39 in) and 50 mm (2.0 in) per hour
                // Violent rain - when the precipitation rate is > 50 mm (2.0 in) per hour

                cloud.PrecipRate = 2 * static_cast<unsigned char>(cloudLayer.getPrecipitationRate());
                cloud.PrecipType = static_cast<unsigned char>(cloudLayer.getPrecipitation());
                cloud.TopShape = 0;
                cloud.Turbulence = 0;

                switch (cloudLayer.getClouds())
                {
                case CCloudLayer::NoClouds: cloud.Type = 0; break;
                case CCloudLayer::Cirrus: cloud.Type = 1; break;
                case CCloudLayer::Stratus: cloud.Type = 8; break;
                case CCloudLayer::Cumulus: cloud.Type = 9; break;
                case CCloudLayer::Thunderstorm: cloud.Type = 10; break;
                default: cloud.Type = 0;
                }

                cloud.UpperAlt = cloudLayer.getTop().value(CLengthUnit::m());
                nw.Cloud[nw.nCloudsCtr++] = cloud;
            }

            CWindLayerList windLayers = gridPoint.getWindLayers();
            windLayers.sortBy(&CWindLayer::getLevel);
            for (const auto &windLayer : as_const(windLayers))
            {
                NewWind wind;
                wind.Direction = windLayer.getDirection().value(CAngleUnit::deg()) * 65536 / 360.0;
                wind.GapAbove = 0;
                wind.Gust = windLayer.getGustSpeed().value(CSpeedUnit::kts());
                wind.Shear = 0;
                wind.Speed = windLayer.getSpeed().value(CSpeedUnit::kts());
                wind.SpeedFract = 0;
                wind.Turbulence = 0;
                wind.UpperAlt = windLayer.getLevel().value(CLengthUnit::m());
                wind.Variance = 0;
                nw.Wind[nw.nWindsCtr++] = wind;
            }

            NewPress press;
            press.Drift = 0;
            // Pressure is measured in: 16 x mb
            press.Pressure = gridPoint.getSurfacePressure().value(CPressureUnit::mbar()) * 16;
            nw.Press = press;

            QByteArray weatherData(reinterpret_cast<const char *>(&nw), sizeof(NewWeather));
            m_weatherMessageQueue.append(FsuipcWeatherMessage(0xC800, weatherData, 5));
            return true;
        }

        bool CFsuipc::read(CSimulatedAircraft &aircraft, bool cockpit, bool situation, bool aircraftParts)
        {
            DWORD dwResult;
            char localFsTimeRaw[3];
            char modelNameRaw[256];
            qint16 com1ActiveRaw = 0, com2ActiveRaw = 0, com1StandbyRaw = 0, com2StandbyRaw = 0;
            qint16 transponderCodeRaw = 0;
            qint8 xpdrModeSb3Raw = 0, xpdrIdentSb3Raw = 0;
            qint32 groundspeedRaw = 0, pitchRaw = 0, bankRaw = 0, headingRaw = 0;
            qint64 altitudeRaw = 0;
            qint32 groundAltitudeRaw = 0;
            qint64 latitudeRaw = 0, longitudeRaw = 0;
            qint16 lightsRaw = 0;
            qint16 onGroundRaw = 0;
            qint32 flapsControlRaw = 0, gearControlRaw = 0, spoilersControlRaw = 0;
            qint16 numberOfEngines = 0;
            qint16 engine1CombustionFlag = 0, engine2CombustionFlag = 0, engine3CombustionFlag = 0, engine4CombustionFlag = 0;


            // http://www.projectmagenta.com/all-fsuipc-offsets/
            // https://www.ivao.aero/softdev/ivap/fsuipc_sdk.asp
            // http://squawkbox.ca/doc/sdk/fsuipc.php

            if (!this->isConnected()) { return false; }
            if (!(aircraftParts || situation || cockpit)) { return false; }

            bool read = false;
            bool cockpitN = !cockpit;
            bool situationN = !situation;
            bool aircraftPartsN = ! aircraftParts;

            if (FSUIPC_Read(0x0238, 3, localFsTimeRaw, &dwResult) &&

                    // COM settings
                    (cockpitN || FSUIPC_Read(0x034e, 2, &com1ActiveRaw, &dwResult)) &&
                    (cockpitN || FSUIPC_Read(0x3118, 2, &com2ActiveRaw, &dwResult)) &&
                    (cockpitN || FSUIPC_Read(0x311a, 2, &com1StandbyRaw, &dwResult)) &&
                    (cockpitN || FSUIPC_Read(0x311c, 2, &com2StandbyRaw, &dwResult)) &&
                    (cockpitN || FSUIPC_Read(0x0354, 2, &transponderCodeRaw, &dwResult)) &&

                    // COM Settings, transponder, SB3
                    (cockpitN || FSUIPC_Read(0x7b91, 1, &xpdrModeSb3Raw, &dwResult)) &&
                    (cockpitN || FSUIPC_Read(0x7b93, 1, &xpdrIdentSb3Raw, &dwResult)) &&

                    // Speeds, situation
                    (situationN || FSUIPC_Read(0x02b4, 4, &groundspeedRaw, &dwResult)) &&
                    (situationN || FSUIPC_Read(0x0578, 4, &pitchRaw, &dwResult)) &&
                    (situationN || FSUIPC_Read(0x057c, 4, &bankRaw, &dwResult)) &&
                    (situationN || FSUIPC_Read(0x0580, 4, &headingRaw, &dwResult)) &&
                    (situationN || FSUIPC_Read(0x0570, 8, &altitudeRaw, &dwResult)) &&

                    // Position
                    (situationN || FSUIPC_Read(0x0560, 8, &latitudeRaw, &dwResult)) &&
                    (situationN || FSUIPC_Read(0x0568, 8, &longitudeRaw, &dwResult)) &&
                    (situationN || FSUIPC_Read(0x0020, 4, &groundAltitudeRaw, &dwResult)) &&

                    // model name
                    FSUIPC_Read(0x3d00, 256, &modelNameRaw, &dwResult) &&

                    // aircraft parts
                    (aircraftPartsN || FSUIPC_Read(0x0D0C, 2, &lightsRaw, &dwResult)) &&
                    (aircraftPartsN || FSUIPC_Read(0x0366, 2, &onGroundRaw, &dwResult)) &&
                    (aircraftPartsN || FSUIPC_Read(0x0BDC, 4, &flapsControlRaw, &dwResult)) &&
                    (aircraftPartsN || FSUIPC_Read(0x0BE8, 4, &gearControlRaw, &dwResult)) &&
                    (aircraftPartsN || FSUIPC_Read(0x0BD0, 4, &spoilersControlRaw, &dwResult)) &&

                    // engines
                    (aircraftPartsN || FSUIPC_Read(0x0AEC, 2, &numberOfEngines, &dwResult)) &&
                    (aircraftPartsN || FSUIPC_Read(0x0894, 2, &engine1CombustionFlag, &dwResult)) &&
                    (aircraftPartsN || FSUIPC_Read(0x092C, 2, &engine2CombustionFlag, &dwResult)) &&
                    (aircraftPartsN || FSUIPC_Read(0x09C4, 2, &engine3CombustionFlag, &dwResult)) &&
                    (aircraftPartsN || FSUIPC_Read(0x0A5C, 2, &engine4CombustionFlag, &dwResult)) &&

                    // If we wanted other reads/writes at the same time, we could put them here
                    FSUIPC_Process(&dwResult))
            {
                read = true;

                // time, basically as a heartbeat
                QString fsTime;
                fsTime.sprintf("%02d:%02d:%02d", localFsTimeRaw[0], localFsTimeRaw[1], localFsTimeRaw[2]);

                if (cockpit)
                {
                    // COMs
                    CComSystem com1 = aircraft.getCom1System();
                    CComSystem com2 = aircraft.getCom2System();
                    CTransponder xpdr = aircraft.getTransponder();

                    // 2710 => 12710 => / 100.0 => 127.1
                    com1ActiveRaw = (10000 + CBcdConversions::bcd2Dec(com1ActiveRaw));
                    com2ActiveRaw = (10000 + CBcdConversions::bcd2Dec(com2ActiveRaw));
                    com1StandbyRaw = (10000 + CBcdConversions::bcd2Dec(com1StandbyRaw));
                    com2StandbyRaw = (10000 + CBcdConversions::bcd2Dec(com2StandbyRaw));
                    com1.setFrequencyActiveMHz(com1ActiveRaw / 100.0);
                    com2.setFrequencyActiveMHz(com2ActiveRaw / 100.0);
                    com1.setFrequencyStandbyMHz(com1StandbyRaw / 100.0);
                    com2.setFrequencyStandbyMHz(com2StandbyRaw / 100.0);

                    transponderCodeRaw = static_cast<qint16>(CBcdConversions::bcd2Dec(transponderCodeRaw));
                    xpdr.setTransponderCode(transponderCodeRaw);
                    // Mode by SB3
                    if (xpdrIdentSb3Raw != 0)
                    {
                        //! \todo Reset value for FSUIPC
                        xpdr.setTransponderMode(CTransponder::StateIdent);
                    }
                    else
                    {
                        xpdr.setTransponderMode(
                            xpdrModeSb3Raw == 0 ? CTransponder::ModeC : CTransponder::StateStandby
                        );
                    }
                    aircraft.setCockpit(com1, com2, xpdr);
                } // cockpit

                if (situation)
                {
                    // position
                    const double latCorrectionFactor = 90.0 / (10001750.0 * 65536.0 * 65536.0);
                    const double lonCorrectionFactor = 360.0 / (65536.0 * 65536.0 * 65536.0 * 65536.0);
                    CAircraftSituation situation = aircraft.getSituation();
                    CCoordinateGeodetic position = situation.getPosition();
                    CLatitude lat(latitudeRaw * latCorrectionFactor, CAngleUnit::deg());
                    CLongitude lon(longitudeRaw * lonCorrectionFactor, CAngleUnit::deg());
                    CLength groundAltitude(groundAltitudeRaw / 256.0, CLengthUnit::m());
                    position.setLatitude(lat);
                    position.setLongitude(lon);
                    position.setGeodeticHeight(groundAltitude);
                    situation.setPosition(position);

                    const double angleCorrectionFactor = 360.0 / 65536.0 / 65536.0; // see FSUIPC docu
                    pitchRaw = std::floor(pitchRaw * angleCorrectionFactor);
                    bankRaw = std::floor(bankRaw * angleCorrectionFactor);

                    // MSFS has inverted pitch and bank angles
                    pitchRaw = ~pitchRaw;
                    bankRaw = ~bankRaw;
                    if (pitchRaw < -90 || pitchRaw > 89) { CLogMessage(this).warning("FSUIPC: Pitch value out of limits: %1") << pitchRaw; }

                    // speeds, situation
                    CAngle pitch = CAngle(pitchRaw, CAngleUnit::deg());
                    CAngle bank = CAngle(bankRaw, CAngleUnit::deg());
                    CHeading heading = CHeading(headingRaw * angleCorrectionFactor, CHeading::True, CAngleUnit::deg());
                    CSpeed groundspeed(groundspeedRaw / 65536.0, CSpeedUnit::m_s());
                    CAltitude altitude(altitudeRaw / (65536.0 * 65536.0), CAltitude::MeanSeaLevel, CLengthUnit::m());
                    situation.setBank(bank);
                    situation.setHeading(heading);
                    situation.setPitch(pitch);
                    situation.setGroundSpeed(groundspeed);
                    situation.setAltitude(altitude);
                    aircraft.setSituation(situation);

                } // situation

                // model
                const QString modelName = QString(modelNameRaw); // to be used to distinguish offsets for different models
                aircraft.setModelString(modelName);

                if (aircraftParts)
                {
                    CAircraftLights lights(lightsRaw & (1 << 4), lightsRaw & (1 << 2), lightsRaw & (1 << 3), lightsRaw & (1 << 1),
                                           lightsRaw & (1 << 0), lightsRaw & (1 << 8));

                    QList<bool> helperList { engine1CombustionFlag != 0, engine2CombustionFlag != 0,
                                             engine3CombustionFlag != 0, engine4CombustionFlag != 0
                                           };

                    CAircraftEngineList engines;
                    for (int index = 0; index < numberOfEngines; ++index)
                    {
                        engines.push_back(CAircraftEngine(index + 1, helperList.at(index)));
                    }

                    CAircraftParts parts(lights, gearControlRaw == 16383, flapsControlRaw * 100 / 16383,
                                         spoilersControlRaw == 16383, engines, onGroundRaw == 1);

                    aircraft.setParts(parts);
                } // parts
            } // read
            return read;
        }

        void CFsuipc::timerEvent(QTimerEvent *event)
        {
            Q_UNUSED(event);
            processWeatherMessages();
        }

        CFsuipc::FsuipcWeatherMessage::FsuipcWeatherMessage(unsigned int offset, const QByteArray &data, int leftTrials) :
            m_offset(offset), m_messageData(data), m_leftTrials(leftTrials)
        { }


        void CFsuipc::clearAllWeather()
        {
            if (!this->isConnected()) { return; }

            // clear all weather
            NewWeather nw;

            // Clear new weather
            nw.uCommand = NW_CLEAR;
            nw.uFlags = 0;
            nw.ulSignature = 0;
            nw.uDynamics = 0;
            for (std::size_t i = 0; i < sizeof(nw.uSpare) / sizeof(nw.uSpare[0]); i++) { nw.uSpare[i] = 0; }

            nw.dLatitude = 0.;
            nw.dLongitude = 0.;
            nw.nElevation = 0;
            nw.ulTimeStamp = 0;
            nw.nTempCtr = 0;
            nw.nWindsCtr = 0;
            nw.nCloudsCtr = 0;
            QByteArray clearWeather(reinterpret_cast<const char *>(&nw), sizeof(NewWeather));
            m_weatherMessageQueue.append(FsuipcWeatherMessage(0xC800, clearWeather, 1));
        }

        void CFsuipc::processWeatherMessages()
        {
            if (m_weatherMessageQueue.empty()) { return; }
            FsuipcWeatherMessage &weatherMessage = m_weatherMessageQueue.first();

            DWORD dwResult;
            weatherMessage.m_leftTrials--;
            FSUIPC_Write(weatherMessage.m_offset, weatherMessage.m_messageData.size(), reinterpret_cast<void *>(weatherMessage.m_messageData.data()), &dwResult);

            unsigned int timeStamp = 0;
            FSUIPC_Read(0xC824, sizeof(timeStamp), &timeStamp, &dwResult);
            FSUIPC_Process(&dwResult);
            if (timeStamp > m_lastTimestamp)
            {
                m_weatherMessageQueue.removeFirst();
                m_lastTimestamp = timeStamp;
                return;
            }

            if (weatherMessage.m_leftTrials == 0)
            {
                CLogMessage(this).debug() << "Number of trials reached for weather message. Dropping it.";
                m_weatherMessageQueue.removeFirst();
            }
        }

        double CFsuipc::intToFractional(double fractional)
        {
            double f = fractional / 10.0;
            if (f < 1.0) return f;
            return intToFractional(f);
        }
    } // namespace
} // namespace

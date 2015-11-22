/* Copyright (C) 2013
 * swift Project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file

#ifndef BLACKCORE_CONTEXTOWNAIRCRAFT_EMPTY_H
#define BLACKCORE_CONTEXTOWNAIRCRAFT_EMPTY_H

#include "blackcoreexport.h"
#include "contextownaircraft.h"
#include "blackmisc/logmessage.h"

namespace BlackCore
{
    //! Empty context, used during shutdown/initialization
    class BLACKCORE_EXPORT CContextOwnAircraftEmpty : public IContextOwnAircraft
    {
        Q_OBJECT

    public:
        //! Constructor
        CContextOwnAircraftEmpty(CRuntime *runtime) : IContextOwnAircraft(CRuntimeConfig::NotUsed, runtime) {}

    public slots:
        //! \copydoc IContextOwnAircraft::getOwnAircraft()
        virtual BlackMisc::Simulation::CSimulatedAircraft getOwnAircraft() const override
        {
            logEmptyContextWarning(Q_FUNC_INFO);
            return BlackMisc::Simulation::CSimulatedAircraft();
        }

        //! \copydoc IContextOwnAircraft::updatePosition
        virtual bool updateOwnPosition(const BlackMisc::Geo::CCoordinateGeodetic &position, const BlackMisc::Aviation::CAltitude &altitude) override
        {
            Q_UNUSED(position);
            Q_UNUSED(altitude);
            logEmptyContextWarning(Q_FUNC_INFO);
            return false;
        }

        //! \copydoc IContextOwnAircraft::updateCockpit
        virtual bool updateCockpit(const BlackMisc::Aviation::CComSystem &com1, const BlackMisc::Aviation::CComSystem &com2, const BlackMisc::Aviation::CTransponder &transponder, const BlackMisc::CIdentifier &originator) override
        {
            Q_UNUSED(com1);
            Q_UNUSED(com2);
            Q_UNUSED(transponder);
            Q_UNUSED(originator);
            logEmptyContextWarning(Q_FUNC_INFO);
            return false;
        }

        //! \copydoc IContextOwnAircraft::updateComFrequency
        virtual bool updateActiveComFrequency(const BlackMisc::PhysicalQuantities::CFrequency &frequency, BlackMisc::Aviation::CComSystem::ComUnit comUnit, const BlackMisc::CIdentifier &originator) override
        {
            Q_UNUSED(frequency);
            Q_UNUSED(comUnit);
            Q_UNUSED(originator);
            logEmptyContextWarning(Q_FUNC_INFO);
            return false;
        }

        //! \copydoc IContextOwnAircraft::updatePilot
        virtual bool updateOwnAircraftPilot(const BlackMisc::Network::CUser &pilot) override
        {
            Q_UNUSED(pilot);
            logEmptyContextWarning(Q_FUNC_INFO);
            return false;
        }

        //! \copydoc IContextOwnAircraft::updateSelcal
        virtual bool updateSelcal(const BlackMisc::Aviation::CSelcal &selcal, const BlackMisc::CIdentifier &originator) override
        {
            Q_UNUSED(selcal);
            Q_UNUSED(originator);
            logEmptyContextWarning(Q_FUNC_INFO);
            return false;
        }

        //! \copydoc IContextOwnAircraft::updateOwnCallsign
        virtual bool updateOwnCallsign(const BlackMisc::Aviation::CCallsign &callsign) override
        {
            Q_UNUSED(callsign);
            logEmptyContextWarning(Q_FUNC_INFO);
            return false;
        }

        //! \copydoc IContextOwnAircraft::updateOwnIcaoCodes
        virtual bool updateOwnIcaoCodes(const BlackMisc::Aviation::CAircraftIcaoCode &aircraftIcaoCode, const BlackMisc::Aviation::CAirlineIcaoCode &airlineIcaoCode) override
        {
            Q_UNUSED(aircraftIcaoCode);
            Q_UNUSED(airlineIcaoCode);
            logEmptyContextWarning(Q_FUNC_INFO);
            return false;
        }

        //! \copydoc IContextOwnAircraft::setAudioOutputVolumes
        virtual void setAudioOutputVolume(int outputVolume) override
        {
            Q_UNUSED(outputVolume);
        }

        //! \copydoc IContextOwnAircraft::setAudioVoiceRoomOverrideUrls
        virtual void setAudioVoiceRoomOverrideUrls(const QString &voiceRoom1Url, const QString &voiceRoom2Url)
        {
            Q_UNUSED(voiceRoom1Url);
            Q_UNUSED(voiceRoom2Url);
            logEmptyContextWarning(Q_FUNC_INFO);
        }

        //! \copydoc IContextOwnAircraft::enableAutomaticVoiceRoomResolution
        virtual void enableAutomaticVoiceRoomResolution(bool enable)
        {
            Q_UNUSED(enable);
            logEmptyContextWarning(Q_FUNC_INFO);
        }

        //! \copydoc IContextOwnAircraft::parseCommandLine
        virtual bool parseCommandLine(const QString &commandLine, const BlackMisc::CIdentifier &originator) override
        {
            Q_UNUSED(commandLine);
            Q_UNUSED(originator);
            logEmptyContextWarning(Q_FUNC_INFO);
            return false;
        }
    };
} // namespace

#endif // guard

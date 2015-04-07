/* Copyright (C) 2013
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file

#ifndef BLACKMISC_PQ_CONSTANTS_H
#define BLACKMISC_PQ_CONSTANTS_H

#include "blackmisc/pq/pq.h"

namespace BlackMisc
{
    namespace PhysicalQuantities
    {

        /*!
         * Physical quantities constants
         */
        class CPhysicalQuantitiesConstants
        {
        public:
            //! Triple point of purified water, 0.01°C
            static const CTemperature &TemperatureTriplePointOfVSMOW()
            {
                static CTemperature t(-273.16, CTemperatureUnit::K());
                return t;
            }

            //! Temperature absolute Zero in °C
            static const CTemperature &TemperatureAbsoluteZeroC()
            {
                static CTemperature t(-273.15, CTemperatureUnit::C());
                return t;
            }

            //! International Standard Atmosphere pressure at mean sea level, 1013.25hPa
            static const CPressure &ISASeaLevelPressure()
            {
                static CPressure p(1013.25, CPressureUnit::hPa());
                return p;
            }

            //! ICAO standard pressure datum for flight levels, 1013.2hPa
            static const CPressure &ICAOFlightLevelPressure()
            {
                static CPressure p(1013.2, CPressureUnit::hPa());
                return p;
            }

            //! Standard pressure datum for flight levels in USA, Canada, parts of Latin America, 29.92inHg
            static const CPressure &USFlightLevelPressure()
            {
                static CPressure p(29.92, CPressureUnit::inHg());
                return p;
            }

            //! Standard pressure datum for flight levels expressed in mmHg, such as in Russia, 760mmHg
            static const CPressure &RuFlightLevelPressure()
            {
                static CPressure p(760, CPressureUnit::mmHg());
                return p;
            }

            //! Unicom frequency
            static const CFrequency &FrequencyUnicom()
            {
                static CFrequency f(122.8, CFrequencyUnit::MHz());
                return f;
            }

            //! Civil aircraft emergency frequency
            static const CFrequency &FrequencyInternationalAirDistress()
            {
                static CFrequency f(121.5, CFrequencyUnit::MHz());
                return f;
            }

            //! Military aircraft emergency frequency
            static const CFrequency &FrequencyMilitaryAirDistress()
            {
                static CFrequency f(243.0, CFrequencyUnit::MHz());
                return f;
            }
        };

    } // namespace
} // namespace

#endif // guard

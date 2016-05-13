/* Copyright (c) 2014
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file

#ifndef BLACKMISC_AVIATION_AIRCRAFTPARTS_H
#define BLACKMISC_AVIATION_AIRCRAFTPARTS_H

#include "blackmisc/blackmiscexport.h"
#include "blackmisc/valueobject.h"
#include "blackmisc/timestampbased.h"
#include "blackmisc/propertyindex.h"
#include "blackmisc/aviation/aircraftlights.h"
#include "blackmisc/aviation/aircraftenginelist.h"

namespace BlackMisc
{
    namespace Aviation
    {
        //! Value object encapsulating information of aircraft's parts
        class BLACKMISC_EXPORT CAircraftParts :
            public CValueObject<CAircraftParts>,
            public BlackMisc::ITimestampBased
        {
        public:
            //! Properties by index
            enum ColumnIndex
            {
                IndexLights = BlackMisc::CPropertyIndex::GlobalIndexCAircraftParts,
                IndexGearDown,
                IndexFlapsPercentage,
                IndexSpoilersOut,
                IndexEngines,
                IndexOnGround
            };

            //! Default constructor
            CAircraftParts() {}

            //! Constructor
            CAircraftParts(const CAircraftLights &lights, bool gearDown, int flapsPercent, bool spoilersOut,
                           const CAircraftEngineList &engines, bool onGround)
                : m_lights(lights), m_engines(engines), m_flapsPercentage(flapsPercent), m_gearDown(gearDown),
                  m_spoilersOut(spoilersOut), m_isOnGround(onGround)
            {}

            //! \copydoc BlackMisc::Mixin::Index::propertyByIndex
            CVariant propertyByIndex(const BlackMisc::CPropertyIndex &index) const;

            //! \copydoc BlackMisc::Mixin::Index::setPropertyByIndex
            void setPropertyByIndex(const BlackMisc::CPropertyIndex &index, const CVariant &variant);

            //! Compare for index
            int comparePropertyByIndex(const CPropertyIndex &index, const CAircraftParts &compareValue) const;

            //! Get aircraft lights
            CAircraftLights getLights() const { return m_lights; }

            //! Set aircraft lights
            void setLights(const CAircraftLights &lights) { m_lights = lights; }

            //! Set all lights on
            void setAllLightsOn();

            //! Set all lights off
            void setAllLightsOff();

            //! Is gear down?
            bool isGearDown() const { return m_gearDown; }

            //! Set gear down
            void setGearDown(bool down) { m_gearDown = down; }

            //! Get flaps position in percent
            int getFlapsPercent() const { return m_flapsPercentage; }

            //! Set flaps position in percent
            void setFlapsPercent(int flapsPercent) { m_flapsPercentage = flapsPercent; }

            //! Are spoilers out?
            bool isSpoilersOut() const { return m_spoilersOut; }

            //! Set spoilers out
            void setSpoilersOut(bool out) { m_spoilersOut = out; }

            //! Get engines
            CAircraftEngineList getEngines() const { return m_engines; }

            //! Engine with number
            CAircraftEngine getEngine(int number) const;

            //! Number of engines
            int getEnginesCount() const { return m_engines.size(); }

            //! Is engine with number 1..n on?
            bool isEngineOn(int number) const;

            //! Set engines
            void setEngines(const CAircraftEngineList &engines) { m_engines = engines; }

            //! Is aircraft on ground?
            bool isOnGround() const { return m_isOnGround; }

            //! Set aircraft on ground
            void setOnGround(bool onGround) { m_isOnGround = onGround; }

            //! \copydoc BlackMisc::Mixin::String::toQString
            QString convertToQString(bool i18n = false) const;

        private:
            CAircraftLights m_lights;
            CAircraftEngineList m_engines;
            int m_flapsPercentage  = 0;
            bool m_gearDown     = false;
            bool m_spoilersOut  = false;
            bool m_isOnGround   = false;

            BLACK_METACLASS(
                CAircraftParts,
                BLACK_METAMEMBER_NAMED(lights, "lights"),
                BLACK_METAMEMBER_NAMED(gearDown, "gear_down"),
                BLACK_METAMEMBER_NAMED(flapsPercentage, "flaps_pct"),
                BLACK_METAMEMBER_NAMED(spoilersOut, "spoilers_out"),
                BLACK_METAMEMBER_NAMED(engines, "engines"),
                BLACK_METAMEMBER_NAMED(isOnGround, "on_ground"),
                BLACK_METAMEMBER(timestampMSecsSinceEpoch, 0, DisabledForJson | DisabledForComparison)
            );
        };
    } // namespace
} // namespace

Q_DECLARE_METATYPE(BlackMisc::Aviation::CAircraftParts)

#endif // guard

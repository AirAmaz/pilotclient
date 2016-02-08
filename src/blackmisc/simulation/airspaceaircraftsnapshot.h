/* Copyright (C) 2015
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file

#ifndef BLACKMISC_SIMULATION_AIRSPACEAIRCRAFTANALYZER_H
#define BLACKMISC_SIMULATION_AIRSPACEAIRCRAFTANALYZER_H

#include "blackmisc/blackmiscexport.h"
#include "blackmisc/simulation/simulatedaircraftlist.h"
#include "blackmisc/aviation/callsignset.h"
#include "blackmisc/propertyindex.h"
#include <QDateTime>

namespace BlackMisc
{
    namespace Simulation
    {
        //! Current situation in the sky analyzed.
        class BLACKMISC_EXPORT CAirspaceAircraftSnapshot : public CValueObject<CAirspaceAircraftSnapshot>
        {
        public:
            //! Default constructor
            CAirspaceAircraftSnapshot();

            //! Constructor
            CAirspaceAircraftSnapshot(
                const BlackMisc::Simulation::CSimulatedAircraftList &allAircraft,
                bool restricted       = false,
                bool renderingEnabled = true,
                int maxAircraft       = 100,
                const BlackMisc::PhysicalQuantities::CLength &maxRenderedDistance = BlackMisc::PhysicalQuantities::CLength(0, BlackMisc::PhysicalQuantities::CLengthUnit::nullUnit()),
                const BlackMisc::PhysicalQuantities::CLength &maxRenderedBoundary = BlackMisc::PhysicalQuantities::CLength(0, BlackMisc::PhysicalQuantities::CLengthUnit::nullUnit())
            );

            //! Time when snapshot was taken
            const QDateTime getTimestamp() const { return QDateTime::fromMSecsSinceEpoch(m_timestampMsSinceEpoch); }

            //! Callsigns by distance
            const BlackMisc::Aviation::CCallsignSet &getAircraftCallsignsByDistance() const { return m_aircraftCallsignsByDistance; }

            //! Callsigns by distance, only enabled aircraft
            const BlackMisc::Aviation::CCallsignSet &getEnabledAircraftCallsignsByDistance() const { return m_enabledAircraftCallsignsByDistance; }

            //! Callsigns by distance, only disabled aircraft
            const BlackMisc::Aviation::CCallsignSet &getDisabledAircraftCallsignsByDistance() const { return m_disabledAircraftCallsignsByDistance; }

            //! VTOL aircraft callsigns by distance, only enabled aircraft
            const BlackMisc::Aviation::CCallsignSet &getVtolAircraftCallsignsByDistance() const { return m_vtolAircraftCallsignsByDistance; }

            //! VTOL aircraft callsigns by distance, only enabled aircraft
            const BlackMisc::Aviation::CCallsignSet &getEnabledVtolAircraftCallsignsByDistance() const { return m_enabledVtolAircraftCallsignsByDistance; }

            //! Valid snapshot?
            bool isValidSnapshot() const;

            //! Restricted snapshot?
            bool isValidRestricted() const { return m_restricted; }

            //! Did restriction change compared to last snapshot
            void setRestrictionChanged(const CAirspaceAircraftSnapshot &snapshot);

            //! Did the restriction flag change?
            bool isRestrictionChanged() const { return m_restrictionChanged; }

            //! Restricted values?
            bool isRestricted() const { return m_restricted; }

            //! Rendering enabled or all aircraft disabled?
            bool isRenderingEnabled() const { return m_renderingEnabled; }

            //! \copydoc BlackMisc::Mixin::Index::propertyByIndex
            CVariant propertyByIndex(const BlackMisc::CPropertyIndex &index) const;

            //! \copydoc BlackMisc::Mixin::Index::setPropertyByIndex
            void setPropertyByIndex(const CVariant &variant, const BlackMisc::CPropertyIndex &index);

            //! \copydoc BlackMisc::Mixin::String::toQString
            QString convertToQString(bool i18n = false) const;

            //! Generating thread name
            const QString &generatingThreadName() const { return m_threadName; }

        private:
            BLACK_ENABLE_TUPLE_CONVERSION(CAirspaceAircraftSnapshot)
            qint64 m_timestampMsSinceEpoch = -1;
            bool m_restricted = false;
            bool m_renderingEnabled = true;
            bool m_restrictionChanged = false;
            QString m_threadName; //!< generating thread name for debugging purposes

            // remark closest aircraft always first
            BlackMisc::Aviation::CCallsignSet m_aircraftCallsignsByDistance;

            BlackMisc::Aviation::CCallsignSet m_enabledAircraftCallsignsByDistance;
            BlackMisc::Aviation::CCallsignSet m_disabledAircraftCallsignsByDistance;

            BlackMisc::Aviation::CCallsignSet m_vtolAircraftCallsignsByDistance;
            BlackMisc::Aviation::CCallsignSet m_enabledVtolAircraftCallsignsByDistance;
        };
    } // namespace
} // namespace

BLACK_DECLARE_TUPLE_CONVERSION(
    BlackMisc::Simulation::CAirspaceAircraftSnapshot, (
        attr(o.m_timestampMsSinceEpoch),
        attr(o.m_aircraftCallsignsByDistance, flags<DisabledForComparison>()),
        attr(o.m_enabledAircraftCallsignsByDistance, flags <DisabledForComparison> ()),
        attr(o.m_disabledAircraftCallsignsByDistance, flags<DisabledForComparison>()),
        attr(o.m_vtolAircraftCallsignsByDistance, flags <DisabledForComparison> ()),
        attr(o.m_enabledVtolAircraftCallsignsByDistance, flags<DisabledForComparison>())
    ))
Q_DECLARE_METATYPE(BlackMisc::Simulation::CAirspaceAircraftSnapshot)

#endif

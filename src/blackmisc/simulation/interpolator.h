/* Copyright (C) 2014
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file

#ifndef BLACKMISC_SIMULATION_INTERPOLATOR_H
#define BLACKMISC_SIMULATION_INTERPOLATOR_H

#include "interpolationrenderingsetup.h"
#include "blackmisc/aviation/aircraftpartslist.h"
#include "blackmisc/aviation/aircraftsituation.h"
#include "blackmisc/aviation/aircraftpartslist.h"
#include "blackmisc/simulation/remoteaircraftprovider.h"

#include <QObject>
#include <QString>
#include <QtGlobal>

namespace BlackMisc
{
    class CWorker;
    namespace Aviation { class CCallsign; }
    namespace Simulation
    {
        class CInterpolationHints;
        class CInterpolationLogger;
        class CInterpolatorLinear;
        class CInterpolatorSpline;
        struct CInterpolationStatus;
        struct CPartsStatus;

        //! Interpolator, calculation inbetween positions
        template <typename Derived>
        class CInterpolator : public QObject
        {
        public:
            //! Log category
            static QString getLogCategory() { return "swift.interpolator"; }

            //! Current interpolated situation
            BlackMisc::Aviation::CAircraftSituation getInterpolatedSituation(
                qint64 currentTimeSinceEpoc, const CInterpolationAndRenderingSetup &setup, const CInterpolationHints &hints, CInterpolationStatus &status);

            //! Parts before given offset time (aka pending parts)
            BlackMisc::Aviation::CAircraftParts getInterpolatedParts(
                qint64 cutoffTime, const CInterpolationAndRenderingSetup &setup, CPartsStatus &partsStatus, bool log = false);

            //! Add a new aircraft situation
            void addAircraftSituation(const BlackMisc::Aviation::CAircraftSituation &situation);

            //! Add a new aircraft parts
            void addAircraftParts(const BlackMisc::Aviation::CAircraftParts &parts);

            //! Takes input between 0 and 1 and returns output between 0 and 1 smoothed with an S-shaped curve.
            //!
            //! Useful for making interpolation seem smoother, efficiently as it just uses simple arithmetic.
            //! \see https://en.wikipedia.org/wiki/Smoothstep
            //! \see http://sol.gfxile.net/interpolation/
            static double smootherStep(double x)
            {
                return x * x * x * (x * (x * 6.0 - 15.0) + 10.0);
            }

            //! Attach an observer to read the interpolator's state for debugging
            void attachLogger(CInterpolationLogger *logger) { m_logger = logger; }

        protected:
            BlackMisc::Aviation::CAircraftSituationList m_aircraftSituations; //!< recent situations
            BlackMisc::Aviation::CAircraftPartsList m_aircraftParts;          //!< recent parts
            BlackMisc::Aviation::CCallsign m_callsign;                        //!< callsign
            bool m_isFirstInterpolation = true;                               //!< set to false after the first successful interpolation

            //! Constructor
            CInterpolator(const QString &objectName, const BlackMisc::Aviation::CCallsign &callsign, QObject *parent);

            //! Set the ground elevation from hints
            static void setGroundElevationFromHint(const CInterpolationHints &hints, BlackMisc::Aviation::CAircraftSituation &situation, bool override = true);

            //! Set on ground flag
            static void setGroundFlagFromInterpolator(const CInterpolationHints &hints, double groundFactor, BlackMisc::Aviation::CAircraftSituation &situation);

        private:
            CInterpolationLogger *m_logger = nullptr;

            Derived *derived() { return static_cast<Derived *>(this); }
            const Derived *derived() const { return static_cast<const Derived *>(this); }
        };

        //! Simple interpolator for pitch, bank, heading, groundspeed
        class BLACKMISC_EXPORT CInterpolatorPbh
        {
        public:
            //! Constructor
            //! @{
            CInterpolatorPbh()
            {}
            CInterpolatorPbh(const Aviation::CAircraftSituation &older, const Aviation::CAircraftSituation &newer) :
                oldSituation(older), newSituation(newer)
            {}
            CInterpolatorPbh(double time, const Aviation::CAircraftSituation &older, const Aviation::CAircraftSituation &newer) :
                simulationTimeFraction(time), oldSituation(older), newSituation(newer)
            {}
            //! @}

            //! Getter
            //! @{
            Aviation::CHeading getHeading() const;
            PhysicalQuantities::CAngle getPitch() const;
            PhysicalQuantities::CAngle getBank() const;
            PhysicalQuantities::CSpeed getGroundSpeed() const;
            //! @}

            //! Change time fraction
            void setTimeFraction(double tf) { simulationTimeFraction = tf; }

        private:
            double simulationTimeFraction = 0.0;
            Aviation::CAircraftSituation oldSituation;
            Aviation::CAircraftSituation newSituation;
        };

        //! Status of interpolation
        struct BLACKMISC_EXPORT CInterpolationStatus
        {
        public:
            //! Did interpolation succeed?
            bool didInterpolationSucceed() const { return m_interpolationSucceeded; }

            //! Set succeeded
            void setInterpolationSucceeded(bool succeeded) { m_interpolationSucceeded = succeeded; }

            //! Changed position?
            bool hasChangedPosition() const { return m_changedPosition; }

            //! Set as changed
            void setChangedPosition(bool changed) { m_changedPosition = changed; }

            //! all OK
            bool allTrue() const;

            //! Reset to default values
            void reset();

        private:
            bool m_changedPosition = false;        //!< position was changed
            bool m_interpolationSucceeded = false; //!< interpolation succeeded (means enough values, etc.)
        };

        //! Status regarding parts
        struct BLACKMISC_EXPORT CPartsStatus
        {
        public:
            //! all OK
            bool allTrue() const;

            //! Supporting parts
            bool isSupportingParts() const { return m_supportsParts; }

            //! Set support flag
            void setSupportsParts(bool supports) { m_supportsParts = supports; }

            //! Reset to default values
            void reset();

        private:
            bool m_supportsParts = false;   //!< supports parts for given callsign
        };

        //! \cond PRIVATE
        extern template class BLACKMISC_EXPORT_DECLARE_TEMPLATE CInterpolator<CInterpolatorLinear>;
        extern template class BLACKMISC_EXPORT_DECLARE_TEMPLATE CInterpolator<CInterpolatorSpline>;
        //! \endcond
    } // namespace
} // namespace
#endif // guard

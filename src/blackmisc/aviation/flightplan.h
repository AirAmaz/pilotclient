/* Copyright (C) 2013
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file

#ifndef BLACKMISC_AVIATION_FLIGHTPLAN_H
#define BLACKMISC_AVIATION_FLIGHTPLAN_H

#include "blackmisc/aviation/airporticaocode.h"
#include "blackmisc/aviation/altitude.h"
#include "blackmisc/blackmiscexport.h"
#include "blackmisc/icon.h"
#include "blackmisc/metaclass.h"
#include "blackmisc/pq/speed.h"
#include "blackmisc/pq/time.h"
#include "blackmisc/pq/units.h"
#include "blackmisc/valueobject.h"

#include <QDateTime>
#include <QMetaType>
#include <QString>
#include <QTime>
#include <QtGlobal>

namespace BlackMisc
{
    namespace Aviation
    {
        //! Value object for a flight plan
        class BLACKMISC_EXPORT CFlightPlan : public CValueObject<CFlightPlan>
        {
        public:
            //! Flight rules (VFR or IFR)
            enum FlightRules
            {
                VFR = 0,    //!< Visual flight rules
                IFR,        //!< Instrument flight rules
                SVFR,       //!< Special VFR (reserved for ATC use),
                DVFR        //!< Defense VFR
            };

            static constexpr int MaxRemarksLength = 150; //!< Max remarks length
            static constexpr int MaxRouteLength = 150; //!< Max route length

            //! Default constructor
            CFlightPlan();

            //! Constructor
            CFlightPlan(const QString &equipmentIcao, const CAirportIcaoCode &originAirportIcao, const CAirportIcaoCode &destinationAirportIcao, const CAirportIcaoCode &alternateAirportIcao,
                        const QDateTime &takeoffTimePlanned, const QDateTime &takeoffTimeActual, const PhysicalQuantities::CTime &enrouteTime, const PhysicalQuantities::CTime &fuelTime,
                        const CAltitude &cruiseAltitude, const PhysicalQuantities::CSpeed &cruiseTrueAirspeed, FlightRules flightRules, const QString &route, const QString &remarks);

            //! Set ICAO aircraft equipment code string (e.g. "T/A320/F")
            void setEquipmentIcao(const QString &equipmentIcao) { m_equipmentIcao = equipmentIcao; }

            //! Set origin airport ICAO code
            void setOriginAirportIcao(const QString &originAirportIcao) { m_originAirportIcao = originAirportIcao; }

            //! Set origin airport ICAO code
            void setOriginAirportIcao(const CAirportIcaoCode &originAirportIcao) { m_originAirportIcao = originAirportIcao; }

            //! Set destination airport ICAO code
            void setDestinationAirportIcao(const QString &destinationAirportIcao) { m_destinationAirportIcao = destinationAirportIcao; }

            //! Set destination airport ICAO code
            void setDestinationAirportIcao(const CAirportIcaoCode &destinationAirportIcao) { m_destinationAirportIcao = destinationAirportIcao; }

            //! Set alternate destination airport ICAO code
            void setAlternateAirportIcao(const QString &alternateAirportIcao) { m_alternateAirportIcao = alternateAirportIcao; }

            //! Set alternate destination airport ICAO code
            void setAlternateAirportIcao(const CAirportIcaoCode &alternateAirportIcao) { m_alternateAirportIcao = alternateAirportIcao; }

            //! Set planned takeoff time
            void setTakeoffTimePlanned(const QDateTime &takeoffTimePlanned) { m_takeoffTimePlanned = takeoffTimePlanned; }

            //! Set planned takeoff time hh:mm
            void setTakeoffTimePlanned(const QString &time) { m_takeoffTimePlanned = QDateTime::currentDateTimeUtc(); m_takeoffTimePlanned.setTime(QTime::fromString(time, "hh:mm"));}

            //! Set actual takeoff time (reserved for ATC use)
            void setTakeoffTimeActual(const QDateTime &takeoffTimeActual) { m_takeoffTimeActual = takeoffTimeActual; }

            //! Set actual takeoff time hh:mm
            void setTakeoffTimeActual(const QString &time) { m_takeoffTimeActual = QDateTime::currentDateTimeUtc(); m_takeoffTimeActual.setTime(QTime::fromString(time, "hh:mm"));}

            //! Set planned enroute flight time
            void setEnrouteTime(const PhysicalQuantities::CTime &enrouteTime) { m_enrouteTime = enrouteTime; m_enrouteTime.switchUnit(BlackMisc::PhysicalQuantities::CTimeUnit::hrmin());}

            //! Set amount of fuel load in time
            void setFuelTime(const PhysicalQuantities::CTime &fuelTime) { m_fuelTime = fuelTime; m_fuelTime.switchUnit(BlackMisc::PhysicalQuantities::CTimeUnit::hrmin());}

            //! Set amount of fuel load in time hh:mm
            void setFuelTime(const QString &fuelTime) { m_fuelTime = PhysicalQuantities::CTime(fuelTime); }

            //! Set planned cruise altitude
            void setCruiseAltitude(const CAltitude &cruiseAltitude) { m_cruiseAltitude = cruiseAltitude; }

            //! Set planned cruise TAS
            void setCruiseTrueAirspeed(const PhysicalQuantities::CSpeed &cruiseTrueAirspeed) { m_cruiseTrueAirspeed = cruiseTrueAirspeed; }

            //! Set flight rules (VFR or IFR)
            void setFlightRule(FlightRules flightRules) { m_flightRules = flightRules; }

            //! Set route string
            void setRoute(const QString &route) { m_route = route.trimmed().left(MaxRouteLength).toUpper(); }

            //! Set remarks string (max 100 characters)
            void setRemarks(const QString &remarks) { m_remarks = remarks.trimmed().left(MaxRemarksLength).toUpper(); }

            //! When last sent
            void setWhenLastSentOrLoaded(const QDateTime &dateTime) { m_lastSentOrLoaded = dateTime; }

            //! Get ICAO aircraft equipment code string
            const QString &getEquipmentIcao() const { return m_equipmentIcao; }

            //! Get origin airport ICAO code
            const CAirportIcaoCode &getOriginAirportIcao() const { return m_originAirportIcao; }

            //! Get destination airport ICAO code
            const CAirportIcaoCode &getDestinationAirportIcao() const { return m_destinationAirportIcao; }

            //! Get alternate destination airport ICAO code
            const CAirportIcaoCode &getAlternateAirportIcao() const { return m_alternateAirportIcao; }

            //! Get planned takeoff time (planned)
            const QDateTime &getTakeoffTimePlanned() const { return m_takeoffTimePlanned; }

            //! Get planned takeoff time (planned)
            QString getTakeoffTimePlannedHourMin() const { return m_takeoffTimePlanned.toString("hh:mm"); }

            //! Get actual takeoff time (actual)
            const QDateTime &getTakeoffTimeActual() const { return m_takeoffTimeActual; }

            //! Get actual takeoff time (actual)
            QString getTakeoffTimeActualHourMin() const { return m_takeoffTimeActual.toString("hh:mm"); }

            //! Get planned enroute flight time
            const PhysicalQuantities::CTime &getEnrouteTime() const { return m_enrouteTime; }

            //! Get planned enroute flight time
            QString getEnrouteTimeHourMin() const { return m_enrouteTime.valueRoundedWithUnit(BlackMisc::PhysicalQuantities::CTimeUnit::hrmin()); }

            //! Get amount of fuel load in time
            const PhysicalQuantities::CTime &getFuelTime() const { return m_fuelTime; }

            //! Get amount of fuel load in time
            QString getFuelTimeHourMin() const { return m_fuelTime.valueRoundedWithUnit(BlackMisc::PhysicalQuantities::CTimeUnit::hrmin()); }

            //! Cruising altitudes
            const BlackMisc::Aviation::CAltitude &getCruiseAltitude() const { return m_cruiseAltitude; }

            //! Get planned cruise TAS
            const PhysicalQuantities::CSpeed &getCruiseTrueAirspeed() const { return m_cruiseTrueAirspeed; }

            //! Get flight rules (VFR or IFR)
            FlightRules getFlightRules() const { return m_flightRules; }

            //! Get route string
            const QString &getRoute() const { return m_route; }

            //! When last sent
            const QDateTime &whenLastSentOrLoaded() const { return m_lastSentOrLoaded; }

            //! Flight plan already sent
            bool wasSentOrLoaded() const { return m_lastSentOrLoaded.isValid() && !m_lastSentOrLoaded.isNull(); }

            //! Received before n ms
            qint64 timeDiffSentOrLoadedMs() const
            {
                return this->m_lastSentOrLoaded.msecsTo(QDateTime::currentDateTimeUtc());
            }

            //! Get remarks string
            const QString &getRemarks() const { return m_remarks; }

            //! \copydoc BlackMisc::Mixin::Icon::toIcon
            CIcon toIcon() const;

            //! \copydoc BlackMisc::Mixin::String::toQString()
            QString convertToQString(bool i18n = false) const;

            //! Rules to string
            static const QString flightRuleToString(FlightRules rule);

        private:
            QString m_equipmentIcao; //!< e.g. "T/A320/F"
            CAirportIcaoCode m_originAirportIcao;
            CAirportIcaoCode m_destinationAirportIcao;
            CAirportIcaoCode m_alternateAirportIcao;
            QDateTime m_takeoffTimePlanned;
            QDateTime m_takeoffTimeActual;
            PhysicalQuantities::CTime m_enrouteTime;
            PhysicalQuantities::CTime m_fuelTime;
            CAltitude m_cruiseAltitude;
            PhysicalQuantities::CSpeed m_cruiseTrueAirspeed;
            FlightRules m_flightRules;
            QString m_route;
            QString m_remarks;
            QDateTime m_lastSentOrLoaded;

            BLACK_METACLASS(
                CFlightPlan,
                BLACK_METAMEMBER(equipmentIcao),
                BLACK_METAMEMBER(originAirportIcao),
                BLACK_METAMEMBER(destinationAirportIcao),
                BLACK_METAMEMBER(alternateAirportIcao),
                BLACK_METAMEMBER(takeoffTimePlanned),
                BLACK_METAMEMBER(takeoffTimeActual),
                BLACK_METAMEMBER(enrouteTime),
                BLACK_METAMEMBER(fuelTime),
                BLACK_METAMEMBER(cruiseAltitude),
                BLACK_METAMEMBER(cruiseTrueAirspeed),
                BLACK_METAMEMBER(flightRules),
                BLACK_METAMEMBER(route),
                BLACK_METAMEMBER(remarks),
                BLACK_METAMEMBER(lastSentOrLoaded)
            );
        };
    } // namespace
} // namespace

Q_DECLARE_METATYPE(BlackMisc::Aviation::CFlightPlan)

#endif // guard

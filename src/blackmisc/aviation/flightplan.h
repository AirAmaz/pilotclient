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

#include "airporticaocode.h"
#include "aircrafticaocode.h"
#include "altitude.h"
#include "airlineicaocode.h"
#include "callsign.h"
#include "selcal.h"
#include "blackmisc/network/voicecapabilities.h"
#include "blackmisc/pq/speed.h"
#include "blackmisc/pq/time.h"
#include "blackmisc/pq/units.h"
#include "blackmisc/timestampbased.h"
#include "blackmisc/valueobject.h"
#include "blackmisc/metaclass.h"
#include "blackmisc/icon.h"
#include "blackmisc/blackmiscexport.h"

#include <QDateTime>
#include <QMetaType>
#include <QString>
#include <QTime>
#include <QtGlobal>

namespace BlackMisc
{
    namespace Aviation
    {
        //! Flight plan remarks, parsed values
        //! \remark Actually the term "remarks" is not accurate, as the FP remarks are normally only the /RMK part of a flight plan.
        //!         But on flight sim. networks "remarks" is used to fill in all parts not fitting in other fields.
        //!         The correct term would be ITEM18 ("OTHER INFORMATION") or ITEM19 ("SUPPLEMENTARY INFORMATION")
        //!         according to https://www.skybrary.aero/index.php/Flight_Plan_Filling
        class BLACKMISC_EXPORT CFlightPlanRemarks : public CValueObject<CFlightPlanRemarks>
        {
        public:
            //! Ctor
            CFlightPlanRemarks();

            //! Ctor
            CFlightPlanRemarks(const QString &remarks, bool parse = true);

            //! Ctor
            CFlightPlanRemarks(const QString &remarks, Network::CVoiceCapabilities voiceCapabilities, bool parse = true);

            //! The unparsed remarks
            const QString &getRemarks() const { return m_remarks; }

            //! Radio telephony designator
            const QString &getRadioTelephony() const { return m_radioTelephony; }

            //! Operator, i.e. normally the airline name
            const QString &getFlightOperator() const { return m_flightOperator; }

            //! Airline ICAO if provided in flight plan
            const CAirlineIcaoCode &getAirlineIcao() const { return m_airlineIcao; }

            //! SELCAL code
            const CSelcal &getSelcalCode() const { return m_selcalCode; }

            //! Get registration (a callsign, but normally not the flight callsign)
            const CCallsign &getRegistration() const { return m_registration; }

            //! Voice capabilities
            const Network::CVoiceCapabilities &getVoiceCapabilities() const { return m_voiceCapabilities; }

            //! Any remarks available?
            bool hasAnyParsedRemarks() const;

            //! Airline remarks
            bool hasParsedAirlineRemarks() const;

            //! Parse remarks from a flight plan
            void parseFlightPlanRemarks(bool force = false);

            //! Valid airline ICAO?
            //! \remark valid here means valid syntax, no guarantee it really exists
            bool hasValidAirlineIcao() const { return m_airlineIcao.hasValidDesignator(); }

            //! Empty remarks?
            bool isEmpty() const { return m_remarks.isEmpty(); }

            //! Already parsed?
            bool isParsed() const { return m_isParsed; }

            //! \copydoc BlackMisc::Mixin::String::toQString()
            QString convertToQString(bool i18n = false) const;

            //! Turn text into voice capabilities for remarks
            static QString textToVoiceCapabilities(const QString &text);

            //! Replace the voice capabilities remarks part
            static QString replaceVoiceCapabilities(const QString &newCaps, const QString &oldRemarks);

        private:
            QString m_remarks;        //!< the unparsed string
            QString m_radioTelephony; //!< radio telephony designator
            QString m_flightOperator; //!< operator, i.e. normally the airline name
            CCallsign m_registration; //!< callsign of pilot
            CSelcal m_selcalCode;     //!< SELCAL code
            CAirlineIcaoCode m_airlineIcao; //!< airline ICAO if provided in flight plan
            Network::CVoiceCapabilities m_voiceCapabilities; //!< voice capabilities
            bool m_isParsed = false;  //!< marked as parsed

            BLACK_METACLASS(
                CFlightPlanRemarks,
                BLACK_METAMEMBER(remarks, 0, DisabledForComparison),
                BLACK_METAMEMBER(radioTelephony),
                BLACK_METAMEMBER(flightOperator),
                BLACK_METAMEMBER(airlineIcao),
                BLACK_METAMEMBER(selcalCode),
                BLACK_METAMEMBER(registration),
                BLACK_METAMEMBER(isParsed),
                BLACK_METAMEMBER(voiceCapabilities)
            );

            //! Cut the remarks part
            static QString cut(const QString &remarks, const QString &marker);
        };

        //! Value object for a flight plan
        class BLACKMISC_EXPORT CFlightPlan :
            public CValueObject<CFlightPlan>,
            public ITimestampBased
        {
        public:
            //! Flight rules (VFR or IFR)
            enum FlightRules
            {
                VFR = 0,    //!< Visual flight rules
                IFR,        //!< Instrument flight rules
                SVFR,       //!< Special VFR (reserved for ATC use),
                DVFR,       //!< Defense VFR
                UNKNOWN     //!< Unknown
            };

            //! Properties by index
            enum ColumnIndex
            {
                IndexCallsign = CPropertyIndex::GlobalIndexCFlightPlan,
                IndexOriginAirportIcao,
                IndexDestinationAirportIcao,
                IndexAlternateAirportIcao,
                IndexRemarks
            };

            //! \fixme max.length of complete flight plan is 768 characters, this here is an assumption and should be part of the underlying network layers
            //  https://forums.vatsim.net/viewtopic.php?f=6&t=63416
            static constexpr int MaxRemarksLength = 256; //!< Max.remarks length
            static constexpr int MaxRouteLength = 256; //!< Max.route length

            //! Default constructor
            CFlightPlan();

            //! Constructor
            CFlightPlan(const CCallsign &callsign,
                        const QString &equipmentIcao, const CAirportIcaoCode &originAirportIcao, const CAirportIcaoCode &destinationAirportIcao, const CAirportIcaoCode &alternateAirportIcao,
                        const QDateTime &takeoffTimePlanned, const QDateTime &takeoffTimeActual, const PhysicalQuantities::CTime &enrouteTime, const PhysicalQuantities::CTime &fuelTime,
                        const CAltitude &cruiseAltitude, const PhysicalQuantities::CSpeed &cruiseTrueAirspeed, FlightRules flightRules, const QString &route, const QString &remarks);

            //! Callsign (of aircraft)
            void setCallsign(const CCallsign &callsign);

            //! Set single char ICAO aircraft equipment code like used in "T/A320/F" (here "F")
            //! \remark function can handle full codes like "T/A320/F" of just the "F"
            void setEquipmentIcao(const QString &equipmentIcao);

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
            void setRemarks(const QString &remarks);

            //! When last sent
            void setWhenLastSentOrLoaded(const QDateTime &dateTime) { this->setUtcTimestamp(dateTime); }

            //! Aircraft callsign
            const CCallsign &getCallsign() const { return m_callsign; }

            //! Has callsign?
            bool hasCallsign() const { return !m_callsign.isEmpty(); }

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
            const CAltitude &getCruiseAltitude() const { return m_cruiseAltitude; }

            //! Get planned cruise TAS
            const PhysicalQuantities::CSpeed &getCruiseTrueAirspeed() const { return m_cruiseTrueAirspeed; }

            //! Get flight rules as in FlightRules
            FlightRules getFlightRules() const { return m_flightRules; }

            //! Rules only as IFR or VFR
            FlightRules getFlightRulesAsVFRorIFR() const;

            //! Get flight rules as in FlightRules as string
            QString getFlightRulesAsString() const { return flightRuleToString(this->getFlightRules()); }

            //! Get route string
            const QString &getRoute() const { return m_route; }

            //! When last sent
            const QDateTime whenLastSentOrLoaded() const { return this->getUtcTimestamp() ; }

            //! Flight plan already sent
            bool wasSentOrLoaded() const { return this->hasValidTimestamp(); }

            //! Received before n ms
            qint64 timeDiffSentOrLoadedMs() const { return this->msecsToNow(); }

            //! Get remarks string
            const QString &getRemarks() const { return m_remarks.getRemarks(); }

            //! Get the parsable remarks
            const CFlightPlanRemarks &getFlightPlanRemarks() const { return m_remarks; }

            //! Get ICAO aircraft equipment prefix H/B737/F "H"
            const QString &getEquipmentPrefix() const { return m_equipmentPrefix; }

            //! Set ICAO aircraft equipment prefix H/B737/F "H"
            void setEquipmentPrefix(const QString &prefix) { m_equipmentPrefix = prefix; }

            //! Get ICAO aircraft equipment suffix H/B737/F "F"
            const QString &getEquipmentSuffix() const { return m_equipmentSuffix; }

            //! Set ICAO aircraft equipment suffix H/B737/F "F"
            void setEquipmentSuffix(const QString &suffix) { m_equipmentSuffix = suffix; }

            //! Get aircraft ICAO H/B737/F "B737"
            const CAircraftIcaoCode &getAircraftIcao() const { return m_aircraftIcao; }

            //! Set aircraft ICAO code H/B737/F "B737"
            void setAircraftIcao(const CAircraftIcaoCode &icao) { m_aircraftIcao = icao; }

            //! Has aircraft ICAO?
            bool hasAircraftIcao() const { return m_aircraftIcao.hasDesignator(); }

            //! Full string like "H/B737/F"
            QString getPrefixIcaoSuffix() const;

            //! \copydoc BlackMisc::Mixin::Index::propertyByIndex
            CVariant propertyByIndex(const CPropertyIndex &index) const;

            //! \copydoc BlackMisc::Mixin::Index::setPropertyByIndex
            void setPropertyByIndex(const CPropertyIndex &index, const CVariant &variant);

            //! \copydoc BlackMisc::Mixin::Icon::toIcon
            CIcon toIcon() const;

            //! \copydoc BlackMisc::Mixin::String::toQString()
            QString convertToQString(bool i18n = false) const;

            //! Rules to string
            static const QString flightRuleToString(FlightRules rule);

            //! String to flight rules
            static FlightRules stringToFlightRules(const QString &flightRules);

            //! Get aircraft ICAO code from equipment code like
            //! \remark we expect something like "H/B772/F" "B773" "B773/F"
            static QString aircraftIcaoCodeFromEquipmentCode(const QString &equipmentCodeAndAircraft);

            //! Get the 3 parts of "H/B772/F", returned as prefix, ICAO, suffix
            static QStringList splitEquipmentCode(const QString &equipmentCodeAndAircraft);

            //! Concat the 3 parts to "H/B772/F"
            static QString concatPrefixIcaoSuffix(const QString &prefix, const QString &icao, const QString &suffix);

            //! Equipment codes 1 character
            static const QStringList &faaEquipmentCodes();

            //! Codes plus info
            static const QStringList &faaEquipmentCodesInfo();

            //! SquawkBox equipment codes
            static const QStringList &squawkBoxEquipmentCodes();

            //! Codes plus info
            static const QStringList &squawkBoxEquipmentCodesInfo();

            //! All equipment codes
            static const QStringList &equipmentCodes();

            //! Equipment codes info
            static const QStringList &equipmentCodesInfo();

            //! Prefix codes "H" .. Heavy, "T" .. TCAS
            static const QStringList &prefixCodes();

        private:
            CCallsign m_callsign; //!< aircraft callsign
            CAircraftIcaoCode m_aircraftIcao; //!< Aircraft ICAO code
            QString m_equipmentPrefix; //!< e.g. "T/A320/F" -> the "T"
            QString m_equipmentSuffix; //!< e.g. "T/A320/F" -> the "F"
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
            CFlightPlanRemarks m_remarks;

            BLACK_METACLASS(
                CFlightPlan,
                // callsign will be current flight
                BLACK_METAMEMBER(aircraftIcao),
                BLACK_METAMEMBER(equipmentPrefix),
                BLACK_METAMEMBER(equipmentSuffix),
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
                BLACK_METAMEMBER(timestampMSecsSinceEpoch)
            );
        };
    } // namespace
} // namespace

Q_DECLARE_METATYPE(BlackMisc::Aviation::CFlightPlanRemarks)
Q_DECLARE_METATYPE(BlackMisc::Aviation::CFlightPlan)

#endif // guard

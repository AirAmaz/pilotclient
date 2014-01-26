/* Copyright (C) 2013 VATSIM Community / authors
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/*!
    \file
*/

#ifndef BLACKMISC_AIRCRAFT_H
#define BLACKMISC_AIRCRAFT_H
#include "nwuser.h"
#include "avaircraftsituation.h"
#include "avaircrafticao.h"
#include "avcallsign.h"
#include "aviotransponder.h"
#include "aviocomsystem.h"
#include "valueobject.h"

namespace BlackMisc
{
    namespace Aviation
    {
        /*!
         * Value object encapsulating information of an aircraft
         */
        class CAircraft : public BlackMisc::CValueObject, public BlackMisc::Geo::ICoordinateGeodetic
        {
        public:
            /*!
             * Default constructor.
             */
            CAircraft() : m_distanceToPlane(-1.0, BlackMisc::PhysicalQuantities::CLengthUnit::NM()) {}

            /*!
             * Constructor.
             */
            CAircraft(const QString &callsign, const BlackMisc::Network::CUser &user, const CAircraftSituation &situation)
                : m_callsign(callsign), m_pilot(user), m_situation(situation), m_distanceToPlane(-1.0, BlackMisc::PhysicalQuantities::CLengthUnit::NM()) {}

            /*!
             * \brief QVariant, required for DBus QVariant lists
             */
            virtual QVariant toQVariant() const
            {
                return QVariant::fromValue(*this);
            }

            /*!
             * Get callsign.
             */
            const CCallsign &getCallsign() const { return m_callsign; }

            /*!
             * Get callsign.
             */
            QString getCallsignAsString() const { return m_callsign.asString(); }

            /*!
             * Set callsign
             */
            void setCallsign(const CCallsign &callsign) { this->m_callsign = callsign; this->m_pilot.setCallsign(callsign); }

            /*!
             * Get situation.
             */
            const CAircraftSituation &getSituation() const { return m_situation; }

            /*!
             * Set situation.
             */
            void setSituation(const CAircraftSituation &situation) { m_situation = situation; }

            /*!
             * Get user
             */
            const BlackMisc::Network::CUser &getPilot() const { return m_pilot; }

            /*!
             * \brief Set user
             */
            void setPilot(const BlackMisc::Network::CUser &user) { m_pilot = user; this->m_pilot.setCallsign(this->m_callsign);}

            /*!
             * Get ICAO info
             */
            const CAircraftIcao &getIcaoInfo() const { return m_icao; }

            /*!
             * \brief Set ICAO info
             */
            void setIcaoInfo(const CAircraftIcao &icao) { m_icao = icao; }

            /*!
             * Get the distance to own plane
             */
            const BlackMisc::PhysicalQuantities::CLength &getDistanceToPlane() const { return m_distanceToPlane; }

            /*!
             * Set distance to own plane
             */
            void setDistanceToPlane(const BlackMisc::PhysicalQuantities::CLength &distance) { this->m_distanceToPlane = distance; }

            /*!
             * \brief Valid distance?
             */
            bool hasValidDistance() const { return !this->m_distanceToPlane.isNegativeWithEpsilonConsidered();}

            /*!
             * \brief Calculcate distance to plane, set it, and also return it
             * \param position calculated from this postion to my own aircraft
             */
            const BlackMisc::PhysicalQuantities::CLength &calculcateDistanceToPlane(const BlackMisc::Geo::CCoordinateGeodetic &position);

            /*!
             * \brief Get position
             * \return
             */
            BlackMisc::Geo::CCoordinateGeodetic getPosition() const { return this->m_situation.getPosition(); }

            /*!
             * \brief Set position
             */
            void setPosition(const BlackMisc::Geo::CCoordinateGeodetic &position) { this->m_situation.setPosition(position); }

            /*!
             * \brief Get altitude
             */
            const BlackMisc::Aviation::CAltitude &getAltitude() const { return this->m_situation.getAltitude(); }

            /*!
             * \brief Set altitude
             */
            void setAltitude(const BlackMisc::Aviation::CAltitude &altitude) { this->m_situation.setAltitude(altitude); }

            /*!
             * \brief Get groundspeed
             */
            const BlackMisc::PhysicalQuantities::CSpeed &getGroundSpeed() const { return this->m_situation.getGroundSpeed(); }

            /*!
             * \brief Get latitude
             */
            virtual const BlackMisc::Geo::CLatitude &latitude() const { return this->m_situation.latitude(); }

            /*!
             * \brief Get longitude
             */
            virtual const BlackMisc::Geo::CLongitude &longitude() const { return this->m_situation.longitude(); }

            /*!
             * \brief Get height (height of current position)
             */
            const BlackMisc::PhysicalQuantities::CLength &getHeight() const { return this->m_situation.getHeight(); }

            /*!
             * \brief Get heading
             */
            const BlackMisc::Aviation::CHeading &getHeading() const { return this->m_situation.getHeading(); }

            /*!
             * \brief Get pitch
             */
            const BlackMisc::PhysicalQuantities::CAngle &getPitch() const { return this->m_situation.getPitch(); }

            /*!
             * \brief Get bank
             */
            const BlackMisc::PhysicalQuantities::CAngle &getBank() const { return this->m_situation.getBank(); }

            /*!
             * \brief Get COM1 system
             */
            const BlackMisc::Aviation::CComSystem &getCom1System() const { return this->m_com1system; }

            /*!
             * \brief Get COM2 system
             */
            const BlackMisc::Aviation::CComSystem &getCom2System() const { return this->m_com2system; }

            /*!
             * \brief Set COM1 system
             */
            void setCom1System(const CComSystem &comSystem) { this->m_com1system = comSystem; }

            /*!
             * \brief Set COM2 system
             */
            void setCom2System(const CComSystem &comSystem) { this->m_com2system = comSystem; }


            /*!
             * \brief Is any (COM1/2) active frequency within 8.3383kHz channel?
             */
            bool isActiveFrequencyWithin8_33kHzChannel(const BlackMisc::PhysicalQuantities::CFrequency &comFrequency)
            {
                return this->m_com1system.isActiveFrequencyWithin8_33kHzChannel(comFrequency) ||
                       this->m_com2system.isActiveFrequencyWithin8_33kHzChannel(comFrequency);
            }

            /*!
             * \brief Get transponder
             */
            const BlackMisc::Aviation::CTransponder &getTransponder() const { return this->m_transponder; }

            /*!
             * \brief Set transponder
             */
            void setTransponder(const CTransponder &transponder) { this->m_transponder = transponder; }

            /*!
             * \brief Get transponder code
             */
            QString getTransponderCodeFormatted() const { return this->m_transponder.getTransponderCodeFormatted(); }

            /*!
             * \brief Get transponder code
             */
            qint32 getTransponderCode() const { return this->m_transponder.getTransponderCode(); }

            /*!
             * \brief Get transponder mode
             */
            BlackMisc::Aviation::CTransponder::TransponderMode getTransponderMode() const { return this->m_transponder.getTransponderMode(); }

            /*!
             * \brief Is valid for login?
             */
            bool isValidForLogin() const;

            /*!
             * \brief Meaningful default settings for COM Systems
             */
            void initComSystems();

            /*!
             * \brief Meaningful default settings for Transponder
             */
            void initTransponder();

            /*!
             * \brief Equal operator ==
             */
            bool operator ==(const CAircraft &other) const;

            /*!
             * \brief Unequal operator ==
             * \param other
             * \return
             */
            bool operator !=(const CAircraft &other) const;

            /*!
             * \brief Value hash
             */
            virtual uint getValueHash() const;

            /*!
             * \brief Register metadata
             */
            static void registerMetadata();

            /*!
             * \brief Properties by index
             */
            enum ColumnIndex
            {
                IndexCallsign = 0,
                IndexCallsignAsString,
                IndexCallsignAsStringAsSet,
                IndexPilotId,
                IndexPilotRealName,
                IndexDistance,
                IndexCom1System,
                IndexFrequencyCom1,
                IndexTransponder,
                IndexTansponderFormatted,
                IndexSituation,
                IndexIcao
            };

            /*!
             * \copydoc CValueObject::propertyByIndex()
             */
            virtual QVariant propertyByIndex(int index) const;

            /*!
             * \copydoc CValueObject::propertyByIndexAsString()
             */
            virtual QString propertyByIndexAsString(int index, bool i18n) const;

            /*!
             * \copydoc CValueObject::setPropertyByIndex()
             */
            virtual void setPropertyByIndex(const QVariant &variant, int index);

        protected:
            /*!
             * \copydoc CValueObject::convertToQString()
             */
            virtual QString convertToQString(bool i18n = false) const;

            /*!
             * \copydoc CValueObject::getMetaTypeId
             */
            virtual int getMetaTypeId() const;

            /*!
             * \copydoc CValueObject::isA
             */
            virtual bool isA(int metaTypeId) const;

            /*!
             * \copydoc CValueObject::compareImpl
             */
            virtual int compareImpl(const CValueObject &other) const;

            /*!
             * \copydoc CValueObject::marshallToDbus()
             */
            virtual void marshallToDbus(QDBusArgument &argument) const;

            /*!
             * \copydoc CValueObject::marshallFromDbus()
             */
            virtual void unmarshallFromDbus(const QDBusArgument &argument);

        private:
            CCallsign m_callsign;
            BlackMisc::Network::CUser m_pilot;
            CAircraftSituation m_situation;
            BlackMisc::Aviation::CComSystem m_com1system;
            BlackMisc::Aviation::CComSystem m_com2system;
            BlackMisc::Aviation::CTransponder m_transponder;
            CAircraftIcao m_icao;
            BlackMisc::PhysicalQuantities::CLength m_distanceToPlane;


        };
    } // namespace
} // namespace

Q_DECLARE_METATYPE(BlackMisc::Aviation::CAircraft)

#endif // guard

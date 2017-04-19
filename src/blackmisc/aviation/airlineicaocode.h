/* Copyright (C) 2013
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file

#ifndef BLACKMISC_AVIATION_AIRLINEICAOCODE_H
#define BLACKMISC_AVIATION_AIRLINEICAOCODE_H

#include "blackmisc/blackmiscexport.h"
#include "blackmisc/country.h"
#include "blackmisc/db/datastore.h"
#include "blackmisc/icon.h"
#include "blackmisc/metaclass.h"
#include "blackmisc/propertyindex.h"
#include "blackmisc/statusmessagelist.h"
#include "blackmisc/valueobject.h"
#include "blackmisc/variant.h"

#include <QJsonObject>
#include <QMetaType>
#include <QString>

namespace BlackMisc
{
    namespace Aviation
    {
        class CCallsign;

        //! Value object for ICAO classification
        class BLACKMISC_EXPORT CAirlineIcaoCode :
            public CValueObject<CAirlineIcaoCode>,
            public BlackMisc::Db::IDatastoreObjectWithIntegerKey
        {
        public:
            //! Properties by index
            enum ColumnIndex
            {
                IndexAirlineDesignator = BlackMisc::CPropertyIndex::GlobalIndexCAirlineIcaoCode,
                IndexIataCode,
                IndexAirlineName,
                IndexAirlineCountryIso,
                IndexAirlineCountry,
                IndexTelephonyDesignator,
                IndexGroupId,
                IndexGroupName,
                IndexGroupDesignator,
                IndexIsVirtualAirline,
                IndexIsOperating,
                IndexIsMilitary,
                IndexDesignatorNameCountry,
            };

            //! Default constructor.
            CAirlineIcaoCode() = default;

            //! Constructor.
            CAirlineIcaoCode(const QString &airlineDesignator);

            //! Constructor.
            CAirlineIcaoCode(const QString &airlineDesignator, const QString &airlineName, const BlackMisc::CCountry &country, const QString &telephony, bool virtualAirline, bool operating);

            //! Get airline, e.g. "DLH"
            const QString &getDesignator() const { return this->m_designator; }

            //! Get airline, e.g. "DLH", but "VMVA" for virtual airlines
            const QString getVDesignator() const;

            //! Get VDesignator plus key
            QString getVDesignatorDbKey() const;

            //! Set airline, e.g. "DLH"
            void setDesignator(const QString &icaoDesignator);

            //! IATA code
            const QString &getIataCode() const { return m_iataCode; }

            //! Set IATA code
            void setIataCode(const QString &iataCode) { this->m_iataCode = iataCode.trimmed().toUpper(); }

            //! Get country, e.g. "FR"
            const QString &getCountryIso() const { return this->m_country.getIsoCode(); }

            //! Get country, e.g. "FRANCE"
            const BlackMisc::CCountry &getCountry() const { return this->m_country; }

            //! Combined string designator, name, country
            QString getDesignatorNameCountry() const;

            //! Set country
            void setCountry(const BlackMisc::CCountry &country) { this->m_country = country; }

            //! Get name, e.g. "Lufthansa"
            const QString &getName() const { return this->m_name; }

            //! \copydoc BlackMisc::simplifyNameForSearch
            QString getSimplifiedName() const;

            //! Name plus key, e.g. "Lufthansa (3421)"
            QString getNameWithKey() const;

            //! Set name
            void setName(const QString &name) { this->m_name = name.trimmed(); }

            //! Telephony designator such as "Speedbird"
            const QString &getTelephonyDesignator() const { return this->m_telephonyDesignator; }

            //! Telephony designator such as "Speedbird"
            void setTelephonyDesignator(const QString &telephony) { this->m_telephonyDesignator = telephony.trimmed().toUpper(); }

            //! Group designator
            const QString &getGroupDesignator() const { return m_groupDesignator; }

            //! Group designator
            void setGroupDesignator(const QString &designator) { m_groupDesignator = designator.trimmed().toUpper(); }

            //! Group name
            const QString &getGroupName() const { return m_groupName; }

            //! Group name
            void setGroupName(const QString &name) { m_groupName = name.trimmed(); }

            //! Group id
            int getGroupId() const { return m_groupId; }

            //! Group id
            void setGroupId(int id) { m_groupId = id; }

            //! Virtual airline
            bool isVirtualAirline() const { return m_isVa; }

            //! Virtual airline
            void setVirtualAirline(bool va) { m_isVa = va; }

            //! Operating?
            bool isOperating() const { return m_isOperating; }

            //! Operating airline?
            void setOperating(bool operating) { m_isOperating = operating; }

            //! Military, air force or such?
            bool isMilitary() const { return m_isMilitary; }

            //! Military, air force or such?
            void setMilitary(bool military) { m_isMilitary = military; }

            //! Country?
            bool hasValidCountry() const;

            //! Airline designator available?
            bool hasValidDesignator() const;

            //! IATA code?
            bool hasIataCode() const;

            //! Matches designator string?
            bool matchesDesignator(const QString &designator) const;

            //! Matches v-designator string?
            bool matchesVDesignator(const QString &designator) const;

            //! Matches IATA code?
            bool matchesIataCode(const QString &iata) const;

            //! Matches IATA code or designator?
            bool matchesDesignatorOrIataCode(const QString &candidate) const;

            //! Matches IATA code or v-designator?
            bool matchesVDesignatorOrIataCode(const QString &candidate) const;

            //! Relaxed check by name or telephony designator (aka callsign, not to be confused with CCallsign)
            bool matchesNamesOrTelephonyDesignator(const QString &candidate) const;

            //! Does simplified name contain the candidate
            bool isContainedInSimplifiedName(const QString &candidate) const;

            //! Telephony designator?
            bool hasTelephonyDesignator() const { return !this->m_telephonyDesignator.isEmpty(); }

            //! Has (airline) name?
            bool hasName() const { return !m_name.isEmpty(); }

            //! Has simplified airline name?
            bool hasSimplifiedName() const;

            //! Complete data
            bool hasCompleteData() const;

            //! Comined string with key
            QString getCombinedStringWithKey() const;

            //! What is better, the callsign airline code or this code. Return the better one.
            CAirlineIcaoCode thisOrCallsignCode(const CCallsign &callsign) const;

            //! \copydoc BlackMisc::Mixin::Icon::toIcon
            CIcon toIcon() const;

            //! \copydoc BlackMisc::Mixin::String::toQString
            QString convertToQString(bool i18n = false) const;

            //! \copydoc BlackMisc::Mixin::Index::propertyByIndex
            CVariant propertyByIndex(const BlackMisc::CPropertyIndex &index) const;

            //! \copydoc BlackMisc::Mixin::Index::setPropertyByIndex
            void setPropertyByIndex(const BlackMisc::CPropertyIndex &index, const CVariant &variant);

            //! Compare for index
            int comparePropertyByIndex(const CPropertyIndex &index, const CAirlineIcaoCode &compareValue) const;

            //! Validate data
            BlackMisc::CStatusMessageList validate() const;

            //! Update missing parts
            void updateMissingParts(const CAirlineIcaoCode &otherIcaoCode);

            //! As a brief HTML summary (e.g. used in tooltips)
            QString asHtmlSummary() const;

            //! Score against other code 0..100
            int calculateScore(const CAirlineIcaoCode &otherCode) const;

            //! Valid designator?
            static bool isValidAirlineDesignator(const QString &airline);

            //! Some special valid designator which do not fit standard rule (e.g. 3-letter code)
            static QSet<QString> specialValidDesignators();

            //! Normalize string as airline designator
            static QString normalizeDesignator(const QString &candidate);

            //! From our DB JSON
            static CAirlineIcaoCode fromDatabaseJson(const QJsonObject &json, const QString &prefix = QString());

        private:
            QString m_designator;           //!< "DLH"
            QString m_iataCode;             //!< "LH"
            QString m_name;                 //!< "Lufthansa"
            QString m_telephonyDesignator;  //!< "Speedbird"
            QString m_groupDesignator;      //!< Group designator
            QString m_groupName;            //!< Group name
            BlackMisc::CCountry m_country;  //!< Country
            int m_groupId = -1;             //!< Group id
            bool m_isVa = false;            //!< virtual airline
            bool m_isOperating = true;      //!< still operating?
            bool m_isMilitary = false;      //!< Air Force or such

            BLACK_METACLASS(
                CAirlineIcaoCode,
                BLACK_METAMEMBER(dbKey),
                BLACK_METAMEMBER(timestampMSecsSinceEpoch),
                BLACK_METAMEMBER(designator),
                BLACK_METAMEMBER(name),
                BLACK_METAMEMBER(country),
                BLACK_METAMEMBER(telephonyDesignator),
                BLACK_METAMEMBER(groupDesignator),
                BLACK_METAMEMBER(groupName),
                BLACK_METAMEMBER(groupId),
                BLACK_METAMEMBER(isVa),
                BLACK_METAMEMBER(isOperating),
                BLACK_METAMEMBER(isMilitary)
            );
        };
    } // namespace
} // namespace

Q_DECLARE_METATYPE(BlackMisc::Aviation::CAirlineIcaoCode)

#endif // guard

/* Copyright (C) 2015
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file

#ifndef BLACKMISC_AVIATION_AIRCRAFTICAOCODELIST_H
#define BLACKMISC_AVIATION_AIRCRAFTICAOCODELIST_H

#include "aircrafticaocode.h"
#include "blackmisc/aviation/aircrafticaocode.h"
#include "blackmisc/blackmiscexport.h"
#include "blackmisc/collection.h"
#include "blackmisc/db/datastoreobjectlist.h"
#include "blackmisc/sequence.h"
#include "blackmisc/variant.h"

#include <QJsonArray>
#include <QMetaType>
#include <QString>
#include <QStringList>
#include <tuple>

namespace BlackMisc
{
    namespace Aviation
    {
        //! Value object encapsulating a list of ICAO codes.
        class BLACKMISC_EXPORT CAircraftIcaoCodeList :
            public CSequence<CAircraftIcaoCode>,
            public BlackMisc::Db::IDatastoreObjectList<CAircraftIcaoCode, CAircraftIcaoCodeList, int>,
            public BlackMisc::Mixin::MetaType<CAircraftIcaoCodeList>
        {
        public:
            BLACKMISC_DECLARE_USING_MIXIN_METATYPE(CAircraftIcaoCodeList)

            //! Default constructor.
            CAircraftIcaoCodeList();

            //! Construct from a base class object.
            CAircraftIcaoCodeList(const CSequence<CAircraftIcaoCode> &other);

            //! Find by designator
            CAircraftIcaoCodeList findByDesignator(const QString &designator) const;

            //! Find by IATA code
            CAircraftIcaoCodeList findByIataCode(const QString &iata) const;

            //! Find by family
            CAircraftIcaoCodeList findByFamily(const QString &family) const;

            //! Ones with a valid designator
            CAircraftIcaoCodeList findByValidDesignator() const;

            //! Ones with an invalid designator
            CAircraftIcaoCodeList findByInvalidDesignator() const;

            //! Find by ICAO/IATA code
            CAircraftIcaoCodeList findByDesignatorOrIataCode(const QString &icaoOrIata) const;

            //! Find by ICAO/IATA code or family
            CAircraftIcaoCodeList findByDesignatorIataOrFamily(const QString &icaoIataOrFamily) const;

            //! Find code ending with string, e.g. "738" finds "B738"
            //! \remark many users use wrong ICAO designators, one typical mistake is "738" for  "B737"
            CAircraftIcaoCodeList findEndingWith(const QString &icaoEnding) const;

            //! Find by manufacturer
            CAircraftIcaoCodeList findByManufacturer(const QString &manufacturer) const;

            //! Find by model description
            CAircraftIcaoCodeList findByDescription(const QString &description) const;

            //! Those with IATA code
            CAircraftIcaoCodeList findWithIataCode(bool removeWhenSameAsDesignator) const;

            //! Those with family
            CAircraftIcaoCodeList findWithFamily(bool removeWhenSameAsDesignator) const;

            //! Find by designator, then best match by rank
            CAircraftIcaoCode findFirstByDesignatorAndRank(const QString &designator) const;

            //! Best selection by given pattern, also searches IATA and family information
            CAircraftIcaoCode smartAircraftIcaoSelector(const CAircraftIcaoCode &icaoPattern) const;

            //! Group by designator and manufacturer
            CAircraftIcaoCodeList groupByDesignatorAndManufacturer() const;

            //! Sort by rank
            void sortByRank();

            //! Sort by designator first, then by rank
            void sortByDesignatorAndRank();

            //! Sort by designator first, then by manufacturer and rank
            void sortByDesignatorManufacturerAndRank();

            //! For selection completion
            QStringList toCompleterStrings(bool withIataCodes = false, bool withFamily = false, bool sort = true) const;

            //! All ICAO codes, no duplicates
            QSet<QString> allIcaoCodes(bool noUnspecified = true) const;

            //! All manufacturers
            QSet<QString> allManufacturers(bool onlyKnownDesignators = true) const;

            //! From our database JSON format
            static CAircraftIcaoCodeList fromDatabaseJson(const QJsonArray &array, bool ignoreIncomplete = true);
        };
    } //namespace
} // namespace

Q_DECLARE_METATYPE(BlackMisc::Aviation::CAircraftIcaoCodeList)
Q_DECLARE_METATYPE(BlackMisc::CCollection<BlackMisc::Aviation::CAircraftIcaoCode>)
Q_DECLARE_METATYPE(BlackMisc::CSequence<BlackMisc::Aviation::CAircraftIcaoCode>)

#endif //guard

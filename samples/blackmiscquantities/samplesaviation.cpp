/* Copyright (C) 2015
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file
//! \ingroup sampleblackmiscquantities

#include "samplesaviation.h"
#include "blackmisc/aviation/airporticaocode.h"
#include "blackmisc/aviation/altitude.h"
#include "blackmisc/aviation/atcstation.h"
#include "blackmisc/aviation/atcstationlist.h"
#include "blackmisc/aviation/callsign.h"
#include "blackmisc/aviation/comsystem.h"
#include "blackmisc/aviation/heading.h"
#include "blackmisc/aviation/navsystem.h"
#include "blackmisc/aviation/transponder.h"
#include "blackmisc/compare.h"
#include "blackmisc/geo/coordinategeodetic.h"
#include "blackmisc/network/user.h"
#include "blackmisc/pq/frequency.h"
#include "blackmisc/pq/length.h"
#include "blackmisc/pq/units.h"
#include "blackmisc/range.h"
#include "blackmisc/stringutils.h"

#include <QDateTime>
#include <QString>
#include <QTextStream>

using namespace BlackMisc;
using namespace BlackMisc::Aviation;
using namespace BlackMisc::PhysicalQuantities;
using namespace BlackMisc::Geo;
using namespace BlackMisc::Network;

namespace BlackSample
{
    int CSamplesAviation::samples(QTextStream &out)
    {
        CHeading h1(180, CHeading::Magnetic, CAngleUnit::deg());
        CHeading h2(180, CHeading::True, CAngleUnit::deg());

        out << h1 << endl;
        out << h1 << " " << h2 << " " << (h1 == h2) << " " << (h1 != h2) << " " << (h1 == h1) << endl;

        // COM system
        CComSystem c1 = CComSystem::getCom1System(125.3);
        out << c1 << endl;
        c1.setActiveUnicom();
        out << c1 << endl;

        // NAV system
        CNavSystem nav1 = CNavSystem::getNav1System(110.0);
        out << nav1 << endl;

        // Transponder tests
        CTransponder tr1(7000, CTransponder::StateStandby);
        CTransponder tr2("4532", CTransponder::ModeMil3);
        out << tr1 << " " << tr2 << endl;

        // Callsign and ATC station
        CCallsign callsign1("d-ambz");
        CCallsign callsign2("DAmbz");
        out << callsign1 << " " << callsign2 << " " << (callsign1 == callsign2) << endl;

        QDateTime dtFrom = QDateTime::currentDateTimeUtc();
        QDateTime dtUntil = dtFrom.addSecs(60 * 60.0); // 1 hour
        QDateTime dtFrom2 = dtUntil;
        QDateTime dtUntil2 = dtUntil.addSecs(60 * 60.0);
        CCoordinateGeodetic geoPos =
            CCoordinateGeodetic::fromWgs84("48° 21′ 13″ N", "11° 47′ 09″ E", CLength(1487, CLengthUnit::ft()));
        CAtcStation station1(CCallsign("eddm_twr"), CUser("123456", "Joe Doe"),
                             CFrequency(118.7, CFrequencyUnit::MHz()),
                             geoPos, CLength(50, CLengthUnit::km()), false, dtFrom, dtUntil);
        CAtcStation station2(station1);
        CAtcStation station3(CCallsign("eddm_twr"), CUser("654321", "Jen Doe"),
                             CFrequency(118.7, CFrequencyUnit::MHz()),
                             geoPos, CLength(100, CLengthUnit::km()), false, dtFrom2, dtUntil2);
        out << station1 << " " << station2 << " " << (station1.getCallsign() == station2.getCallsign()) << endl;

        // User parsing
        CUser user("12345", "Joe KING KGLC");
        out << user.getRealName() << user.getHomebase() << endl;

        // ATC List
        CAtcStationList atcList;
        atcList.push_back(station1);
        atcList.push_back(station2);
        atcList.push_back(station3);
        atcList.push_back(station1);
        atcList.push_back(station2);
        atcList.push_back(station3);
        atcList = atcList.findBy(&CAtcStation::getCallsign, "eddm_twr", &CAtcStation::getFrequency, CFrequency(118.7, CFrequencyUnit::MHz()));
        atcList = atcList.sortedBy(&CAtcStation::getBookedFromUtc, &CAtcStation::getCallsign, &CAtcStation::getControllerRealName);
        out << atcList << endl;
        out << "-----------------------------------------------" << endl;

        // flight plan
        CAltitude alt("FL110");
        CAltitude altMsl(alt);
        altMsl.toMeanSeaLevel();

        out << alt << " " << altMsl << endl;
        CAirportIcaoCode frankfurt("eddf");
        out << frankfurt << endl;
        out << "-----------------------------------------------" << endl;

        return 0;
    }
} // namespace

/* Copyright (C) 2013
 * swift Project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file
//! \ingroup sampleblackmisc

#include "samplescontainer.h"
#include "blackmisc/blackmiscfreefunctions.h"
#include "blackmisc/aviation/atcstationlist.h"
#include "blackmisc/propertyindexallclasses.h"
#include <QDebug>
#include <QMetaType>

using namespace BlackMisc;
using namespace BlackMisc::Aviation;
using namespace BlackMisc::PhysicalQuantities;
using namespace BlackMisc::Geo;
using namespace BlackMisc::Network;

namespace BlackSample
{

    /*
     * Samples
     */
    int CSamplesContainer::samples()
    {
        // ATC stations
        QDateTime dtFrom = QDateTime::currentDateTimeUtc();
        QDateTime dtUntil = dtFrom.addSecs(60 * 60.0); // 1 hour
        QDateTime dtFrom2 = dtUntil;
        QDateTime dtUntil2 = dtUntil.addSecs(60 * 60.0);
        CFrequency freqEddmTwr(118.7, CFrequencyUnit::MHz());
        CCallsign callsignEddmTwr("eddm_twr");
        CCoordinateGeodetic geoPos =
            CCoordinateGeodetic::fromWgs84("48° 21′ 13″ N", "11° 47′ 09″ E", CLength(1487, CLengthUnit::ft()));
        CAtcStation station1(callsignEddmTwr, CUser("123456", "Joe Doe"),
                             freqEddmTwr,
                             geoPos, CLength(50, CLengthUnit::km()), false, dtFrom, dtUntil);
        CAtcStation station2(station1);
        CAtcStation station3(CCallsign("eddm_app"), CUser("654321", "Jen Doe"),
                             CFrequency(120.7, CFrequencyUnit::MHz()),
                             geoPos, CLength(100, CLengthUnit::km()), false, dtFrom2, dtUntil2);

        // ATC List
        CAtcStationList atcList;
        atcList.push_back(station1);
        atcList.push_back(station2);
        atcList.push_back(station3);
        atcList.push_back(station1);
        atcList.push_back(station2);
        atcList.push_back(station3);
        qDebug() << "-- list:";
        qDebug() << atcList.toQString();

        CAtcStationList atcListFind = atcList.findBy(&CAtcStation::getCallsign, "eddm_twr", &CAtcStation::getFrequency, CFrequency(118.7, CFrequencyUnit::MHz()));
        qDebug() << "-- find by:";
        qDebug() << atcListFind.toQString();

        CAtcStationList atcListSort = atcList.sortedBy(&CAtcStation::getBookedFromUtc, &CAtcStation::getCallsign, &CAtcStation::getControllerRealName);
        qDebug() << "-- sort by:";
        qDebug() << atcListSort.toQString();

        qDebug() << "-----------------------------------------------";

        // Apply if tests
        atcList.clear();
        atcList.push_back(station1);
        CAtcStation station1Cpy(station1);
        CFrequency changedFrequency(118.25, CFrequencyUnit::MHz());
        CPropertyIndexVariantMap vm(CAtcStation::IndexFrequency, CVariant::from(changedFrequency));


        // demonstration apply
        CPropertyIndexList changedProperties;
        changedProperties = station1Cpy.apply(vm, true);
        qDebug() << "apply, changed" << changedProperties << vm << "expected 1";
        changedProperties = station1Cpy.apply(vm, true);
        qDebug() << "apply, changed" << changedProperties << vm << "expected 0";

        // applyIf
        int changed;
        changed = atcList.applyIf(&CAtcStation::getCallsign, callsignEddmTwr, vm);
        qDebug() << "applyIf, changed" << changed << vm << "expected 1";
        changed = atcList.applyIf(&CAtcStation::getCallsign, callsignEddmTwr, vm);
        qDebug() << "applyIf, changed" << changed << vm << "expected 1";
        changed = atcList.applyIf(&CAtcStation::getCallsign, callsignEddmTwr, vm, true);
        qDebug() << "applyIf, changed" << changed << vm << "expected 0";

        return 0;
    }

} // namespace

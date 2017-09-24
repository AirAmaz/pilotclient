/* Copyright (C) 2013
 * swift Project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \cond PRIVATE_TESTS

/*!
 * \file
 * \ingroup testblackmisc
 */

#include "testgeo.h"
#include "blackmisc/geo/coordinategeodetic.h"
#include "blackmisc/geo/earthangle.h"
#include "blackmisc/geo/latitude.h"
#include "blackmisc/pq/physicalquantity.h"
#include "blackmisc/pq/units.h"

#include <QTest>

using namespace BlackMisc::Geo;
using namespace BlackMisc::PhysicalQuantities;
using namespace BlackMisc::Math;

namespace BlackMiscTest
{
    void CTestGeo::geoBasics()
    {
        CLatitude lat(10, CAngleUnit::deg());
        QVERIFY2(lat * 2 == lat + lat, "Latitude addition should be equal");
        lat += CLatitude(20, CAngleUnit::deg());
        QVERIFY2(lat.valueRounded() == 30.0, "Latitude should be 30 degrees");

        CAngle a(20, 0);
        lat = CLatitude(a);
        double v = lat.valueRounded(CAngleUnit::deg());
        QVERIFY2(v == 20.0, "Values shall be the same");

        a = CAngle(28, 0);
        lat = CLatitude(a);
        v = lat.valueRounded(CAngleUnit::deg());
        QVERIFY2(v == 28.0, "Values shall be the same");

        a = CAngle(30, 0, 0);
        lat = CLatitude(a);
        v = lat.valueRounded(CAngleUnit::deg());
        QVERIFY2(v == 30.0, "Values shall be the same");
    }

    void CTestGeo::coordinateGeodetic()
    {
        CCoordinateGeodetic northPole = { 90.0, 0.0 };
        CCoordinateGeodetic southPole = { -90.0, 0.0 };
        QCOMPARE(calculateEuclideanDistance(northPole, southPole), 2.0);
        CCoordinateGeodetic equator = { 0.0, 70.354683 };
        QCOMPARE(calculateEuclideanDistance(northPole, equator), std::sqrt(2.0f));

        CCoordinateGeodetic testCoordinate = northPole;
        double latValue = testCoordinate.latitude().value(CAngleUnit::deg());
        double lngValue = testCoordinate.longitude().value(CAngleUnit::deg());
        QVERIFY2(latValue == 90.0, "Latitude value supposed to be 90");
        QVERIFY2(lngValue == 0.0, "Longitude value supposed to be 0");
        CLatitude newLat(90.0, CAngleUnit::deg());
        testCoordinate.setLatitude(newLat);
        latValue = testCoordinate.latitude().value(CAngleUnit::deg());
        QVERIFY2(latValue == newLat.value(CAngleUnit::deg()), "Latitude value supposed to be equal");
    }
} // ns

//! \endcond

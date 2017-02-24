/* Copyright (C) 2015
 * swift project Community / Contributors
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

#include "testinterpolator.h"
#include "blackmisc/aviation/aircraftengine.h"
#include "blackmisc/aviation/aircraftenginelist.h"
#include "blackmisc/aviation/aircraftlights.h"
#include "blackmisc/aviation/aircraftpartslist.h"
#include "blackmisc/aviation/aircraftsituationlist.h"
#include "blackmisc/aviation/altitude.h"
#include "blackmisc/aviation/callsign.h"
#include "blackmisc/aviation/heading.h"
#include "blackmisc/compare.h"
#include "blackmisc/geo/coordinategeodetic.h"
#include "blackmisc/geo/latitude.h"
#include "blackmisc/geo/longitude.h"
#include "blackmisc/simulation/interpolator.h"
#include "blackmisc/simulation/interpolatorlinear.h"
#include "blackmisc/pq/angle.h"
#include "blackmisc/pq/length.h"
#include "blackmisc/pq/physicalquantity.h"
#include "blackmisc/pq/speed.h"
#include "blackmisc/pq/units.h"
#include "blackmisc/simulation/interpolationhints.h"
#include "blackmisc/simulation/remoteaircraftprovider.h"
#include "blackmisc/simulation/remoteaircraftproviderdummy.h"

#include <QCoreApplication>
#include <QDebug>
#include <QEventLoop>
#include <QScopedPointer>
#include <QTest>
#include <QTime>
#include <QtDebug>

using namespace BlackMisc;
using namespace BlackMisc::Aviation;
using namespace BlackMisc::Geo;
using namespace BlackMisc::PhysicalQuantities;
using namespace BlackMisc::Simulation;

namespace BlackMiscTest
{

    void CTestInterpolator::linearInterpolator()
    {
        QScopedPointer<CRemoteAircraftProviderDummy> provider(new CRemoteAircraftProviderDummy());
        CInterpolatorLinear interpolator(provider.data());

        // fixed time so everything can be debugged
        const qint64 ts =  1425000000000; // QDateTime::currentMSecsSinceEpoch();
        const qint64 deltaT = 5000; // ms
        const qint64 offset = 5000;
        CCallsign cs("SWIFT");
        for (int i = 0; i < IRemoteAircraftProvider::MaxSituationsPerCallsign; i++)
        {
            CAircraftSituation s(getTestSituation(cs, i, ts, deltaT, offset));

            // check height above ground
            CLength hag = (s.getAltitude() - s.getGroundElevation());
            QVERIFY2(s.getHeightAboveGround() == hag, "Wrong elevation");
            provider->insertNewSituation(s);
        }

        constexpr int partsCount = 10;
        for (int i = 0; i < partsCount; i++)
        {
            CAircraftParts p(getTestParts(i, ts, deltaT));
            provider->insertNewAircraftParts(cs, p);
        }

        // make sure signals are processed, if the interpolator depends on those signals
        QCoreApplication::processEvents(QEventLoop::AllEvents, 1000);

        // check if all situations / parts have been received
        QVERIFY2(provider->remoteAircraftSituations(cs).size() == IRemoteAircraftProvider::MaxSituationsPerCallsign, "Missing situations");
        QVERIFY2(provider->remoteAircraftParts(cs).size() == partsCount, "Missing parts");

        // interpolation functional check
        IInterpolator::InterpolationStatus status;
        const CInterpolationHints hints;
        const CInterpolationAndRenderingSetup setup;
        double latOld = 360.0;
        double lngOld = 360.0;
        for (qint64 currentTime = ts - 2 * deltaT + offset; currentTime < ts + offset; currentTime += (deltaT / 20))
        {
            // This will use time range
            // from:  ts - 2 * deltaT + offset
            // to:    ts              + offset
            CAircraftSituation currentSituation(interpolator.getInterpolatedSituation
                                                (cs, currentTime, setup, hints, status)
                                               );
            QVERIFY2(status.didInterpolationSucceed(), "Interpolation was not succesful");
            QVERIFY2(status.hasChangedPosition(), "Interpolation did not changed");
            double latDeg = currentSituation.getPosition().latitude().valueRounded(CAngleUnit::deg(), 5);
            double lngDeg = currentSituation.getPosition().longitude().valueRounded(CAngleUnit::deg(), 5);
            QVERIFY2(latDeg < latOld && lngDeg < lngOld, "Values shall decrease");
            QVERIFY2(latDeg >= 0 && latDeg <= IRemoteAircraftProvider::MaxSituationsPerCallsign, "Values shall be in range");
            latOld = latDeg;
            lngOld = lngDeg;
        }

        QTime timer;
        timer.start();
        int interpolationNo = 0;
        qint64 startTimeMsSinceEpoch = ts - 2 * deltaT;

        // Pseudo performance test:
        // Those make not completely sense, as the performance depends on the implementation of
        // the dummy provider, which is different from the real provider
        // With one callsign in the lists (of dummy provider) it is somehow expected to be roughly the same performance

        for (int loops = 0; loops < 20; loops++)
        {
            for (qint64 currentTime = startTimeMsSinceEpoch + offset; currentTime < ts + offset; currentTime += (deltaT / 20))
            {
                // This will use range
                // from:  ts - 2* deltaT + offset
                // to:    ts             + offset
                CAircraftSituation currentSituation(interpolator.getInterpolatedSituation
                                                    (cs, currentTime, setup, hints, status)
                                                   );
                QVERIFY2(status.allTrue(), "Failed interpolation");
                QVERIFY2(currentSituation.getCallsign() == cs, "Wrong callsign");
                double latDeg = currentSituation.getPosition().latitude().valueRounded(CAngleUnit::deg(), 5);
                double lngDeg = currentSituation.getPosition().longitude().valueRounded(CAngleUnit::deg(), 5);
                Q_UNUSED(latDeg);
                Q_UNUSED(lngDeg);
                interpolationNo++;
            }
        }
        int timeMs = timer.elapsed();
        QVERIFY2(timeMs < interpolationNo, "Interpolation > 1ms");
        qDebug() << timeMs << "ms" << "for" << interpolationNo << "interpolations";

        int fetchedParts = 0;
        timer.start();
        for (qint64 currentTime = ts - 2 * deltaT; currentTime < ts; currentTime += 250)
        {
            IInterpolator::PartsStatus partsStatus;
            CAircraftParts pl(interpolator.getInterpolatedParts(cs, ts, setup, partsStatus));
            fetchedParts++;
            QVERIFY2(partsStatus.isSupportingParts(), "Parts not supported");
        }
        timeMs = timer.elapsed();
        qDebug() << timeMs << "ms" << "for" << fetchedParts << "fetched parts";
    }

    CAircraftSituation CTestInterpolator::getTestSituation(const CCallsign &callsign, int number, qint64 ts, qint64 deltaT, qint64 offset)
    {
        CAltitude alt(number, CAltitude::MeanSeaLevel, CLengthUnit::m());
        CLatitude lat(number, CAngleUnit::deg());
        CLongitude lng(180.0 + number, CAngleUnit::deg());
        CHeading heading(number * 10, CHeading::True, CAngleUnit::deg());
        CAngle bank(number, CAngleUnit::deg());
        CAngle pitch(number, CAngleUnit::deg());
        CSpeed gs(number * 10, CSpeedUnit::km_h());
        CAltitude gndElev({ 0, CLengthUnit::m() }, CAltitude::MeanSeaLevel);
        CCoordinateGeodetic c(lat, lng, alt);
        CAircraftSituation s(callsign, c, heading, pitch, bank, gs, gndElev);
        s.setMSecsSinceEpoch(ts - deltaT * number); // values in past
        s.setTimeOffsetMs(offset);
        return s;
    }

    CAircraftParts CTestInterpolator::getTestParts(int number, qint64 ts, qint64 deltaT)
    {
        CAircraftLights l(true, false, true, false, true, false);
        CAircraftEngineList e({ CAircraftEngine(1, true), CAircraftEngine(2, false), CAircraftEngine(3, true) });
        CAircraftParts p(l, true, 20, true, e, false);
        p.setMSecsSinceEpoch(ts - deltaT * number); // values in past
        return p;
    }

} // namespace

//! \endcond

/* Copyright (C) 2018
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

#include "testaircraftparts.h"
#include "blackmisc/aviation/aircraftparts.h"
#include "blackmisc/json.h"
#include <QTest>
#include <QJsonObject>

using namespace BlackMisc::Aviation;
using namespace BlackMisc::Json;

namespace BlackMiscTest
{
    void CTestAircraftParts::groundFlag()
    {
        const CAircraftParts ap1 = this->testParts1();
        CAircraftParts ap2(ap1);
        const QJsonObject ap1Json = ap1.toJson();
        QJsonObject ap2Json = ap2.toJson();
        QJsonObject deltaJson12 = getIncrementalObject(ap1Json, ap2Json);
        QJsonObject deltaJson21 = getIncrementalObject(ap2Json, ap1Json);
        QVERIFY2(deltaJson12.isEmpty(), "Values shall be the same");
        QVERIFY2(deltaJson21.isEmpty(), "Values shall be the same");

        ap2.setOnGround(false);
        ap2Json = ap2.toJson();
        deltaJson12 = getIncrementalObject(ap1Json, ap2Json);
        deltaJson21 = getIncrementalObject(ap2Json, ap1Json);

        QVERIFY2(deltaJson12.keys().size() == 1, "Values shall be 1");
        QVERIFY2(deltaJson21.keys().size() == 1, "Values shall be 1");
        bool deltaGnd = deltaJson12.value("on_ground").toBool(true);
        QVERIFY2(deltaGnd == false, "Values shall be false");
        deltaGnd = deltaJson21.value("on_ground").toBool(false);
        QVERIFY2(deltaGnd == true, "Values shall be false");

        // const QString json1 = stringFromJsonObject(deltaJson12);
        // const QString json2 = stringFromJsonObject(deltaJson21);
    }

    CAircraftParts CTestAircraftParts::testParts1() const
    {
        CAircraftLights lights;
        lights.allLightsOn();
        CAircraftEngineList engines;
        engines.initEngines(4, true);
        const bool onGround = true;
        CAircraftParts ap(lights, true, 0, false, engines, onGround);
        return ap;
    }
} // ns

//! \endcond

/* Copyright (C) 2016
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
 */

#include "testmath.h"
#include "blackmisc/math/mathutils.h"

using namespace BlackMisc::Math;

namespace BlackMiscTest
{
    void CTestMath::testRoundToMultipleOf()
    {
        QVERIFY2(CMathUtils::roundToMultipleOf(0, 3) == 0, "Nearest multiple of 3 from 0 should be 0");
        QVERIFY2(CMathUtils::roundToMultipleOf(1, 3) == 0, "Nearest multiple of 3 from 1 should be 0");
        QVERIFY2(CMathUtils::roundToMultipleOf(2, 3) == 3, "Nearest multiple of 3 from 2 should be 3");
        QVERIFY2(CMathUtils::roundToMultipleOf(3, 3) == 3, "Nearest multiple of 3 from 3 should be 3");

        QVERIFY2(CMathUtils::roundToMultipleOf(0, -3) == 0, "Nearest multiple of -3 from 0 should be 0");
        QVERIFY2(CMathUtils::roundToMultipleOf(1, -3) == 0, "Nearest multiple of -3 from 1 should be 0");
        QVERIFY2(CMathUtils::roundToMultipleOf(2, -3) == 3, "Nearest multiple of -3 from 2 should be 3");
        QVERIFY2(CMathUtils::roundToMultipleOf(3, -3) == 3, "Nearest multiple of -3 from 3 should be 3");

        QVERIFY2(CMathUtils::roundToMultipleOf(-1, 3) == 0, "Nearest multiple of 3 from -1 should be 0");
        QVERIFY2(CMathUtils::roundToMultipleOf(-2, 3) == -3, "Nearest multiple of 3 from -2 should be -3");
        QVERIFY2(CMathUtils::roundToMultipleOf(-3, 3) == -3, "Nearest multiple of 3 from -3 should be -3");

        QVERIFY2(CMathUtils::roundToMultipleOf(-1, -3) == 0, "Nearest multiple of -3 from -1 should be 0");
        QVERIFY2(CMathUtils::roundToMultipleOf(-2, -3) == -3, "Nearest multiple of -3 from -2 should be -3");
        QVERIFY2(CMathUtils::roundToMultipleOf(-3, -3) == -3, "Nearest multiple of -3 from -3 should be -3");
    }

} // namespace

//! \endcond

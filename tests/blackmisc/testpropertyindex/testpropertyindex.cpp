/* Copyright (C) 2017
 * swift Project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \cond PRIVATE_TESTS
//! \file
//! \ingroup testblackmisc

#include "blackmisc/simulation/simulatedaircraft.h"
#include "blackmisc/statusmessagelist.h"
#include "blackmisc/sequence.h"
#include "blackmisc/comparefunctions.h"
#include "test.h"
#include <QTest>

using namespace BlackMisc;
using namespace BlackMisc::Aviation;
using namespace BlackMisc::Simulation;
using namespace BlackMisc::PhysicalQuantities;

namespace BlackMiscTest
{
    //! Testing property index access
    class CTestPropertyIndex : public QObject
    {
        Q_OBJECT

    private slots:
        //! Simulated aircraft index checks
        void propertyIndexCSimulatedAircraft();

        //! Sorting based on property index
        void propertyIndexSort();
    };

    void CTestPropertyIndex::propertyIndexCSimulatedAircraft()
    {
        const CFrequency f(123.50, CFrequencyUnit::MHz());
        const CPropertyIndex i({ CSimulatedAircraft::IndexCom1System, CComSystem::IndexActiveFrequency });
        CSimulatedAircraft aircraft;
        aircraft.setCallsign("DEIHL");
        aircraft.setCom1ActiveFrequency(f);
        CVariant vf = aircraft.propertyByIndex(i);
        const CFrequency pf = vf.value<CFrequency>();
        QVERIFY2(pf == f, "Frequencies should have same value");
    }

    void CTestPropertyIndex::propertyIndexSort()
    {
        CCallsign cs1("DLH1000");
        CCallsign cs2("DLH2000");
        CCallsign cs3("DLH3000");
        CSequence<CCallsign> callsigns;
        callsigns.push_back(cs3);
        callsigns.push_back(cs2);
        callsigns.push_back(cs1);

        const CPropertyIndex indexCs(CCallsign::IndexCallsignString);
        const CPropertyIndexList indexListCs({ indexCs });

        const int vrefCs = cs1.asString().compare(cs2.asString());
        int v1 = cs1.comparePropertyByIndex(indexCs, cs2);
        int v2 = cs2.comparePropertyByIndex(indexCs, cs1);
        int v3 = cs1.comparePropertyByIndex(indexCs, cs1);
        int v11 = indexCs.comparator()(cs1, cs2);
        int v22 = indexCs.comparator()(cs2, cs1);
        int v33 = indexCs.comparator()(cs1, cs1);
        QVERIFY(v1 == v11 && v1 < 0 && vrefCs == v1);
        QVERIFY(v2 == v22 && v2 > 0);
        QVERIFY(v3 == v33 && v3 == 0);

        QVERIFY(callsigns.front().equalsString("DLH3000"));
        callsigns.sortByProperty(indexListCs);
        QVERIFY(callsigns.front().equalsString("DLH1000"));

        CStatusMessage s1(CStatusMessage::SeverityDebug, "debug msg.");
        s1.setMSecsSinceEpoch(4000);
        CStatusMessage s2(CStatusMessage::SeverityInfo, "info msg.");
        s2.setMSecsSinceEpoch(3000);
        CStatusMessage s3(CStatusMessage::SeverityWarning, "warning msg.");
        s3.setMSecsSinceEpoch(2000);
        CStatusMessage s4(CStatusMessage::SeverityError, "error msg.");
        s4.setMSecsSinceEpoch(1000);

        CStatusMessageList msgs;
        msgs.push_back(s1);
        msgs.push_back(s2);
        msgs.push_back(s3);
        msgs.push_back(s4);

        const CPropertyIndex index(CStatusMessage::IndexUtcTimestamp);
        const CPropertyIndexList indexList({ index });

        int vrefTs = Compare::compare(s2.getUtcTimestamp(), s1.getUtcTimestamp());
        int v4 = s2.comparePropertyByIndex(index, s1);
        int v5 = s1.comparePropertyByIndex(index, s2);
        int v6 = s3.comparePropertyByIndex(index, s3);

        int v44 = index.comparator()(s2, s1);
        int v55 = index.comparator()(s1, s2);
        int v66 = index.comparator()(s3, s3);

        QVERIFY(v4 == v44 && v1 < 0 && v4 == vrefTs);
        QVERIFY(v5 == v55 && v2 > 0);
        QVERIFY(v6 == v66 && v3 == 0);

        QVERIFY(msgs.front().getMSecsSinceEpoch() == 4000);
        QVERIFY(msgs.back().getMSecsSinceEpoch() == 1000);
        msgs.sortByProperty(indexList);
        QVERIFY(msgs.front().getMSecsSinceEpoch() == 1000);
        QVERIFY(msgs.back().getMSecsSinceEpoch() == 4000);
    }
} // namespace

//! main
BLACKTEST_APPLESS_MAIN(BlackMiscTest::CTestPropertyIndex);

#include "testpropertyindex.moc"

//! \endcond

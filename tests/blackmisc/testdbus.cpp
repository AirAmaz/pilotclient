/* Copyright (C) 2017
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
 * \ingroup testblackcore
 */

#include "testdbus.h"
#include "blackmisc/simulation/simulatedaircraftlist.h"
#include "blackmisc/test/testservice.h"
#include "blackmisc/test/testserviceinterface.h"
#include "blackmisc/dbusutils.h"
#include <QDBusConnection>
#include <QTest>

using namespace BlackMisc;
using namespace BlackMisc::Simulation;
using namespace BlackMisc::Test;

namespace BlackMiscTest
{
    void CTestDBus::marshallUnmarshall()
    {
        QDBusConnection connection = QDBusConnection::sessionBus();
        if (!CTestService::canRegisterTestService(connection))
        {
            QSKIP("Cannot register DBus service, skip unit test");
            return;
        }
        CTestService *testService = CTestService::registerTestService(connection, false, QCoreApplication::instance());
        Q_UNUSED(testService);
        ITestServiceInterface testServiceInterface(CTestService::InterfaceName(), CTestService::ObjectPath(), connection);
        const int errors = ITestServiceInterface::pingTests(testServiceInterface, false);
        QVERIFY2(errors == 0, "DBus Ping tests fail");
    }

    void CTestDBus::signatureSize()
    {
        constexpr int max = 255;
        QString s;

        // normally CSimulatedAircraftList is expected to be the biggest one
        const CAircraftModel model;
        s = CDBusUtils::dBusSignature(model);
        QVERIFY2(s.length() <= max, "Signature CAircraftModel");

        const CSimulatedAircraft aircraft;
        s = CDBusUtils::dBusSignature(aircraft);
        QVERIFY2(s.length() <= max, "Signature CSimulatedAircraft");

        const CSimulatedAircraftList al;
        s = CDBusUtils::dBusSignature(al);
        QVERIFY2(s.length() <= max, "Signature CSimulatedAircraftList");
    }
}

//! \endcond

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

#include "testvaluecache.h"
#include "blackmisc/aviation/atcstation.h"
#include "blackmisc/aviation/atcstationlist.h"
#include "blackmisc/dictionary.h"
#include "blackmisc/identifier.h"
#include "blackmisc/simulation/simulatedaircraft.h"
#include "blackmisc/simulation/simulatedaircraftlist.h"
#include "blackmisc/statusmessage.h"
#include "blackmisc/variant.h"
#include "blackmisc/variantmap.h"
#include "blackmisc/worker.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QFlags>
#include <QJsonObject>
#include <QJsonValue>
#include <QList>
#include <QMetaObject>
#include <QRegularExpression>
#include <QString>
#include <QTest>
#include <QThread>
#include <QTimer>
#include <QtDebug>
#include <chrono>
#include <future>
#include <ratio>

namespace BlackMiscTest
{
    using namespace BlackMisc;
    using namespace BlackMisc::Aviation;
    using namespace BlackMisc::Simulation;

    CTestValueCache::CTestValueCache(QObject *parent) : QObject(parent)
    {
        CVariant::registerMetadata();
    }

    void CTestValueCache::insertAndGet()
    {
        CVariantMap testData
        {
            { "value1", CVariant::from(1) },
            { "value2", CVariant::from(2) },
            { "value3", CVariant::from(3) }
        };
        CVariantMap testData2
        {
            { "value2", CVariant::from(42) },
            { "value4", CVariant::from(4) }
        };
        CVariantMap testDataCombined
        {
            { "value1", CVariant::from(1) },
            { "value2", CVariant::from(42) },
            { "value3", CVariant::from(3) },
            { "value4", CVariant::from(4) }
        };

        CValueCache cache;
        QVERIFY(cache.getAllValues() == CVariantMap());
        cache.insertValues({ testData, QDateTime::currentMSecsSinceEpoch() });
        QVERIFY(cache.getAllValues() == testData);
        cache.insertValues({ testData2, QDateTime::currentMSecsSinceEpoch() });
        QVERIFY(cache.getAllValues() == testDataCombined);
    }

    //! \cond PRIVATE
    void waitForQueueOf(QObject *object)
    {
        if (object->thread() != QThread::currentThread())
        {
            std::promise<void> promise;
            QTimer::singleShot(0, object, [ & ] { promise.set_value(); });
            promise.get_future().wait();
        }
    }

    template <typename F>
    void singleShotAndWait(QObject *object, F task)
    {
        if (object->thread() == QThread::currentThread())
        {
            task();
        }
        else
        {
            QTimer::singleShot(0, object, task);
            waitForQueueOf(object);
        }
    }

    void testCommon(CValueCacheUser &user1, CValueCacheUser &user2)
    {
        user1.m_value1.set(42);
        QVERIFY(user2.slotFired());
        QVERIFY(! user1.slotFired());
        singleShotAndWait(&user2, [ & ] { QVERIFY(user2.m_value1.get() == 42); });
        QVERIFY(user1.m_value1.get() == 42);

        user1.m_value2.set(42);
        user2.slotFired();
        auto status = user1.m_value2.set(-1337);
        QVERIFY(status.isFailure());
        QVERIFY(! user1.slotFired());
        QVERIFY(! user2.slotFired());
        singleShotAndWait(&user2, [ & ] { QVERIFY(user2.m_value2.get() == 42); });
        QVERIFY(user1.m_value2.get() == 42);
    }
    //! \endcond

    void CTestValueCache::localOnly()
    {
        CValueCache cache;
        for (int i = 0; i < 2; ++i) { QTest::ignoreMessage(QtDebugMsg, QRegularExpression("Empty cache value")); }
        CValueCacheUser user1(&cache);
        CValueCacheUser user2(&cache);
        testCommon(user1, user2);
    }

    void CTestValueCache::localOnlyWithThreads()
    {
        CValueCache cache;
        for (int i = 0; i < 2; ++i) { QTest::ignoreMessage(QtDebugMsg, QRegularExpression("Empty cache value")); }
        CValueCacheUser user1(&cache);
        CValueCacheUser user2(&cache);
        CRegularThread thread;
        user2.moveToThread(&thread);
        thread.start();
        testCommon(user1, user2);
    }

    void CTestValueCache::distributed()
    {
        CIdentifier thisProcess;
        CIdentifier otherProcess;
        auto json = otherProcess.toJson();
        json.insert("processId", otherProcess.getProcessId() + 1);
        otherProcess.convertFromJson(json);

        CValueCache thisCache;
        CValueCache otherCache;
        connect(&thisCache, &CValueCache::valuesChangedByLocal, &thisCache, [ & ](const CValueCachePacket &values)
        {
            QMetaObject::invokeMethod(&thisCache, "changeValuesFromRemote", Q_ARG(BlackMisc::CValueCachePacket, values), Q_ARG(BlackMisc::CIdentifier, thisProcess));
            QMetaObject::invokeMethod(&otherCache, "changeValuesFromRemote", Q_ARG(BlackMisc::CValueCachePacket, values), Q_ARG(BlackMisc::CIdentifier, otherProcess));
        });
        connect(&otherCache, &CValueCache::valuesChangedByLocal, &thisCache, [ & ](const CValueCachePacket &values)
        {
            QMetaObject::invokeMethod(&thisCache, "changeValuesFromRemote", Q_ARG(BlackMisc::CValueCachePacket, values), Q_ARG(BlackMisc::CIdentifier, otherProcess));
            QMetaObject::invokeMethod(&otherCache, "changeValuesFromRemote", Q_ARG(BlackMisc::CValueCachePacket, values), Q_ARG(BlackMisc::CIdentifier, thisProcess));
        });

        for (int i = 0; i < 4; ++i) { QTest::ignoreMessage(QtDebugMsg, QRegularExpression("Empty cache value")); }
        CValueCacheUser thisUser(&thisCache);
        CValueCacheUser otherUser(&otherCache);

        CRegularThread thread;
        otherCache.moveToThread(&thread);
        otherUser.moveToThread(&thread);
        thread.start();

        singleShotAndWait(&otherUser, [ & ] { otherUser.m_value1.set(99); });
        thisUser.m_value1.set(100);
        QCoreApplication::processEvents();
        waitForQueueOf(&otherUser);
        QVERIFY(thisUser.slotFired() != otherUser.slotFired());
        auto thisValue = thisUser.m_value1.get();
        singleShotAndWait(&otherUser, [ & ] { QVERIFY(thisValue == otherUser.m_value1.get()); });
    }

    void CTestValueCache::batched()
    {
#if 0 // MS temp disabled 2016-09-09
        CValueCache cache;
        for (int i = 0; i < 2; ++i) { QTest::ignoreMessage(QtDebugMsg, QRegularExpression("Empty cache value")); }
        CValueCacheUser user1(&cache);
        CValueCacheUser user2(&cache);

        {
            auto batch = cache.batchChanges(&user1);
            user1.m_value1.set(42);
            user1.m_value2.set(42);
        }
        QVERIFY(! user1.slotFired());
        QVERIFY(user2.slotFired());
        singleShotAndWait(&user2, [ & ]
        {
            QVERIFY(user2.m_value1.get() == 42);
            QVERIFY(user2.m_value2.get() == 42);
        });
#endif
    }

    void CTestValueCache::json()
    {
        QJsonObject testJson
        {
            { "value1", CVariant::from(1).toJson() },
            { "value2", CVariant::from(2).toJson() },
            { "value3", CVariant::from(3).toJson() }
        };
        CVariantMap testData
        {
            { "value1", CVariant::from(1) },
            { "value2", CVariant::from(2) },
            { "value3", CVariant::from(3) }
        };

        CValueCache cache;
        cache.loadFromJson(testJson);
        QVERIFY(cache.getAllValues() == testData);
        QVERIFY(cache.saveToJson() == testJson);
    }

    void CTestValueCache::saveAndLoad()
    {
        CSimulatedAircraftList aircraft({ CSimulatedAircraft("BAW001", {}, {}) });
        CAtcStationList atcStations({ CAtcStation("EGLL_TWR") });
        CVariantMap testData
        {
            { "namespace1/value1", CVariant::from(1) },
            { "namespace1/value2", CVariant::from(2) },
            { "namespace1/value3", CVariant::from(3) },
            { "namespace2/aircraft", CVariant::from(aircraft) },
            { "namespace2/atcstations", CVariant::from(atcStations) }
        };
        CValueCache cache;
        cache.insertValues({ testData, QDateTime::currentMSecsSinceEpoch() });

        QDir dir(QDir::currentPath() + "/testcache");
        if (dir.exists()) { dir.removeRecursively(); }

        auto status = cache.saveToFiles(dir.absolutePath());
        QVERIFY(status.isSuccess());

        auto files = dir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot, QDir::Name);
        QCOMPARE(files.size(), 2);
        QCOMPARE(files[0].fileName(), QString("namespace1.json"));
        QCOMPARE(files[1].fileName(), QString("namespace2.json"));

        CValueCache cache2;
        status = cache2.loadFromFiles(dir.absolutePath());
        QVERIFY(status.isSuccess());
        QCOMPARE(cache2.getAllValues(), testData);
    }

    //! Is value between 0 - 100?
    bool validator(int value)
    {
        return value >= 0 && value <= 100;
    }

    CValueCacheUser::CValueCacheUser(CValueCache *cache) :
        m_value1(cache, "value1", "", validator, 0, this),
        m_value2(cache, "value2", "", validator, 0, this)
    {
        m_value1.setNotifySlot(&CValueCacheUser::ps_valueChanged);
        m_value2.setNotifySlot(&CValueCacheUser::ps_valueChanged);
    }

    void CValueCacheUser::ps_valueChanged()
    {
        m_slotFired.set_value();
    }

    bool CValueCacheUser::slotFired()
    {
        auto status = m_slotFired.get_future().wait_for(std::chrono::milliseconds(250));
        m_slotFired = std::promise<void>();
        switch (status)
        {
        case std::future_status::ready: return true;
        case std::future_status::timeout: return false;
        case std::future_status::deferred:
        default: QTEST_ASSERT(false);
        }
        return false;
    }
} // ns

//! \endcond

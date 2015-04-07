/* Copyright (C) 2013
 * swift Project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of Swift Project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file

#include "testcontainers.h"
#include "testvalueobject.h"
#include "blackmisc/blackmiscfreefunctions.h"
#include "blackmisc/collection.h"
#include "blackmisc/sequence.h"
#include "blackmisc/aviation/callsignlist.h"
#include "blackmisc/aviation/aircraftsituationlist.h"
#include "blackmisc/dictionary.h"
#include <QList>
#include <QVector>
#include <QSet>
#include <vector>
#include <set>

using namespace BlackMisc;
using namespace BlackMisc::Aviation;

namespace BlackMiscTest
{

    void CTestContainers::collectionBasics()
    {
        CCollection<int> c1;
        QVERIFY2(c1.empty(), "Uninitialized collection is empty");
        auto c2 = CCollection<int>::fromImpl(QSet<int>());
        QVERIFY2(c1 == c2, "Uninitialized and empty collections are equal");
        c1.changeImpl(std::set<int>());
        QVERIFY2(c1 == c2, "Two empty collections are equal");
        c1.insert(1);
        QVERIFY2(c1 != c2, "Different collections are not equal");
        QVERIFY2(c1.size() == 1, "Collection has expected size");
        c2.insert(1);
        QVERIFY2(c1 == c2, "Collections with equal elements are equal");
        c1.changeImpl(QSet<int>());
        QVERIFY2(c1 == c2, "Collection stays equal after changing implementation");
        c1.clear();
        QVERIFY2(c1.empty(), "Cleared collection is empty");
        c1.insert(2);
        QVERIFY2(c1 != c2, "Collections with different elements are not equal");
        c1 = c2;
        QVERIFY2(c1 == c2, "Copied collection is equal");
    }

    void CTestContainers::sequenceBasics()
    {
        CSequence<int> s1;
        QVERIFY2(s1.empty(), "Uninitialized sequence is empty");
        auto s2 = CSequence<int>::fromImpl(QList<int>());
        QVERIFY2(s1 == s2, "Uninitialized and empty sequence are equal");
        s1.changeImpl(std::vector<int>());
        QVERIFY2(s1 == s2, "Two empty sequences are equal");
        s1.push_back(1);
        QVERIFY2(s1 != s2, "Different sequences are not equal");
        QVERIFY2(s1.size() == 1, "Sequence has expected size");
        s2.push_back(1);
        QVERIFY2(s1 == s2, "Sequences with equal elements are equal");
        s1.changeImpl(QVector<int>());
        QVERIFY2(s1 == s2, "Sequence stays equal after changing implementation");
        s1.clear();
        QVERIFY2(s1.empty(), "Cleared sequence is empty");
        s1.push_back(2);
        QVERIFY2(s1 != s2, "Sequences with different elements are not equal");
        s1 = s2;
        QVERIFY2(s1 == s2, "Copied sequence is equal");

        QVERIFY2((s1[0] = 1), "Subscripted element mutation");
        QVERIFY2(s1[0] == 1, "Subscripted element has expected value");
        QVERIFY2(s1.back() == 1, "Last element has expected value");
    }

    void CTestContainers::joinAndSplit()
    {
        CSequence<int> s1, s2;
        s1.push_back(1);
        s1.push_back(2);
        s1.push_back(3);
        s2.push_back(4);
        s2.push_back(5);
        s2.push_back(6);
        auto joined = s1.join(s2);
        s1.push_back(s2);
        QVERIFY2(s1.size() == 6, "Combine sequences");
        QVERIFY2(s1 == joined, "Combine sequences");

        CCollection<int> c1, c2, c3, c4;
        c1.push_back(1);
        c1.push_back(2);
        c1.push_back(3);
        c1.push_back(4);
        c1.push_back(5);
        c1.push_back(6);
        c2.push_back(1);
        c2.push_back(2);
        c2.push_back(3);
        c3.push_back(4);
        c3.push_back(5);
        c3.push_back(6);
        c4.push_back(10);
        c4.push_back(20);
        c4.push_back(30);
        QVERIFY2(c1.makeUnion(c2) == c1, "Combine collections");
        QVERIFY2(c2.makeUnion(c3) == c1, "Combine collections");
        QVERIFY2(c1.intersection(c2) == c2, "Combine collections");
        QVERIFY2(c1.difference(c2) == c3, "Split collections");
        c1.insert(c4);
        QVERIFY2(c1.size() == 9, "Combine collections");
        c1.remove(c4);
        QVERIFY2(c1.size() == 6, "Split collections");
        c1.remove(c2);
        QVERIFY2(c1 == c3, "Split collections");
    }

    void CTestContainers::findTests()
    {
        BlackMisc::registerMetadata();
        CCallsignList callsigns;
        CSequence<CCallsign> found = callsigns.findBy(&CCallsign::asString, "Foo");
        QVERIFY2(found.isEmpty(), "Empty found");
        callsigns.push_back(CCallsign("EDDM_TWR"));
        callsigns.push_back(CCallsign("KLAX_TWR"));
        found = callsigns.findBy(&CCallsign::asString, "KLAX_TWR");
        QVERIFY2(found.size() == 1, "found");
    }

    void CTestContainers::dictionaryBasics()
    {
        CTestValueObject key1("Key1", "This is key object 1");
        CTestValueObject key2("Key2", "This is key object 2");

        CTestValueObject value1("Value1", "This is value object 1");
        CTestValueObject value2("Value2", "This is value object 2");

        CValueObjectDictionary d1;
        d1.insert(key1, value1);
        d1.insert(key2, value2);

        CValueObjectDictionary d3;
        d3.insert(key1, value1);
        d3.insert(key2, value2);

        CValueObjectDictionary d2;

        // Operators
        QVERIFY2(d1 != d2, "Inequality operator failed");
        QVERIFY2(d1 == d3, "Equality operator failed");

        // size
        QVERIFY2(d1.size() == 2, "size() wrong");
        QVERIFY2(d1.size() == d1.count(), "size() is different to count()");

        // clear/empty
        d1.clear();
        QVERIFY2(d1.isEmpty(), "clear failed");
        d1.insert(key1, value1);
        d1.insert(key2, value2);

        // keys range
        auto keys = d1.keys();
        QVERIFY2(std::distance(keys.begin(), keys.end()) == 2, "keys range size wrong");

        // keys collection
        CCollection<CTestValueObject> keyCollection = d1.keys();
        QVERIFY2(keyCollection.size() == 2, "keys collection size wrong");

        // keys sequence
        CSequence<CTestValueObject> keySequence = d1.keys();
        QVERIFY2(keySequence.size() == 2, "keys sequence size wrong");

        // findKeyBy
        d2 = d1.findKeyBy(&CTestValueObject::getName, QString("Key1"));
        QVERIFY2(d2.size() == 1, "findKeyBy returned wrong container");
        CTestValueObject o1 = d2.value(key1);
        QVERIFY2(o1.getName() == "Value1", "findKeyBy returned wrong container");

        // findValueBy
        d2 = d1.findValueBy(&CTestValueObject::getName, QString("Value1"));
        QVERIFY2(d2.size() == 1, "findValueBy returned wrong container");
        o1 = d2.value(key1);
        QVERIFY2(o1.getName() == "Value1", "findKeyBy returned wrong container");

        // containsByKey
        QVERIFY2(d1.containsByKey(&CTestValueObject::getName, QString("Key1")), "containsByKey failed");
        QVERIFY2(!d1.containsByKey(&CTestValueObject::getName, QString("Key5")), "containsByKey failed");

        // containsByValue
        QVERIFY2(d1.containsByValue(&CTestValueObject::getName, QString("Value1")), "containsByValue failed");
        QVERIFY2(!d1.containsByValue(&CTestValueObject::getName, QString("Value5")), "containsByValue failed");

        // removeByKeyIf
        d2 = d1;
        d2.removeByKeyIf(&CTestValueObject::getName, "Key2");
        QVERIFY2(d2.size() == 1, "size() wrong");

        // removeByValueIf
        d2 = d1;
        d2.removeByValueIf(&CTestValueObject::getName, "Value2");
        QVERIFY2(d2.size() == 1, "size() wrong");

        // JSON
        QJsonObject jsonObject = d1.toJson();
        CValueObjectDictionary d4;
        d4.convertFromJson(jsonObject);
        QVERIFY2(d1 == d4, "JSON serialization/deserialization failed");
    }

    void CTestContainers::timestampList()
    {
        CAircraftSituationList situations;
        const qint64 ts = QDateTime::currentMSecsSinceEpoch();
        const int no = 10;
        for (int i = 0; i < no; ++i)
        {
            CAircraftSituation s;
            s.setCallsign("CS" + QString::number(i));
            s.setMSecsSinceEpoch(ts - 10 * i);
            situations.push_back(s);
        }

        // test sorting
        situations.sortOldestFirst();
        qint64 ms = situations.front().getMSecsSinceEpoch();
        QVERIFY2(ms == ts - 10 * (no - 1), "Oldest value not first");

        situations.sortLatestFirst();
        ms = situations.front().getMSecsSinceEpoch();
        QVERIFY2(ms == ts, "Latest value not first");

        // split in half
        situations.sortOldestFirst(); // check that we really get latest first
        QList<CAircraftSituationList> split = situations.splitByTime(ts - ((no / 2) * 10) + 1);
        CAircraftSituationList before = split[0];
        CAircraftSituationList after = split[1];

        int beforeSize = before.size();
        int afterSize = after.size();

        QVERIFY(beforeSize == no / 2);
        QVERIFY(afterSize == no / 2);

        // check sort order, latest should
        for (int i = 0; i < no; ++i)
        {
            CAircraftSituation s = (i < no / 2) ? before[i] : after[i - no / 2];
            ms = s.getMSecsSinceEpoch();
            QVERIFY2(ms == ts - 10 * i, "time does not match");
        }

        // test shifting
        situations.clear();
        const int maxElements = 8;
        QVERIFY(situations.isEmpty());
        for (int i = 0; i < no; ++i)
        {
            qint64 cTs = ts - 10 * i;
            CAircraftSituation s;
            s.setCallsign("CS" + QString::number(i));
            s.setMSecsSinceEpoch(cTs);
            situations.push_frontMaxElements(s, maxElements);
            if (i > maxElements - 1)
            {
                QVERIFY2(situations.size() == maxElements, "Situations must only contain max.elements");
            }
            else
            {
                QVERIFY2(situations.size() == i + 1, "Element size does not match");
                QVERIFY2(situations.front().getMSecsSinceEpoch() == cTs, "Wrong front element");
            }
        }

    }

} //namespace

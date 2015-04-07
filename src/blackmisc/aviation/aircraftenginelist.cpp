/* Copyright (C) 2014
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "aircraftenginelist.h"

namespace BlackMisc
{
    namespace Aviation
    {

        CAircraftEngineList::CAircraftEngineList(std::initializer_list<bool> enginesOnOff)
        {
            int no = 1; // engines 1 based
            for (auto it = enginesOnOff.begin(); it != enginesOnOff.end(); ++it)
            {
                CAircraftEngine engine(no++, *it);
                this->push_back(engine);
            }
        }

        CAircraftEngineList::CAircraftEngineList(const CSequence<CAircraftEngine> &other) :
            CSequence<CAircraftEngine>(other)
        { }

        CAircraftEngine CAircraftEngineList::getEngine(int engineNumber) const
        {
            Q_ASSERT(engineNumber >= 0);
            return this->findBy(&CAircraftEngine::getNumber, engineNumber).frontOrDefault();
        }

        bool CAircraftEngineList::isEngineOn(int engineNumber) const
        {
            return getEngine(engineNumber).isOn();
        }

        QJsonObject CAircraftEngineList::toJson() const
        {
            QJsonObject map;

            for (const auto &e : *this)
            {
                QJsonObject value = e.toJson();
                map.insert(QString::number(e.getNumber()), value);
            }
            return map;
        }

        void CAircraftEngineList::convertFromJson(const QJsonObject &json)
        {
            clear();
            for (const auto &e : json.keys())
            {

                CAircraftEngine engine;
                int number = e.toInt();
                engine.convertFromJson(json.value(e).toObject());
                engine.setNumber(number);
                push_back(engine);
            }
        }

        void CAircraftEngineList::registerMetadata()
        {
            qRegisterMetaType<BlackMisc::CSequence<CAircraftEngine>>();
            qDBusRegisterMetaType<BlackMisc::CSequence<CAircraftEngine>>();
            qRegisterMetaType<BlackMisc::CCollection<CAircraftEngine>>();
            qDBusRegisterMetaType<BlackMisc::CCollection<CAircraftEngine>>();
            qRegisterMetaType<CAircraftEngineList>();
            qDBusRegisterMetaType<CAircraftEngineList>();
            registerMetaValueType<CAircraftEngineList>();
        }

    } // namespace
} // namespace

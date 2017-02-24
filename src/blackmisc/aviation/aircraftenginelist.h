/* Copyright (C) 2014
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file

#ifndef BLACKMISC_AVIATION_AIRCRAFTENGINELIST_H
#define BLACKMISC_AVIATION_AIRCRAFTENGINELIST_H

#include "blackmisc/aviation/aircraftengine.h"
#include "blackmisc/aviation/aircraftengine.h"
#include "blackmisc/blackmiscexport.h"
#include "blackmisc/collection.h"
#include "blackmisc/json.h"
#include "blackmisc/sequence.h"
#include "blackmisc/variant.h"

#include <QJsonObject>
#include <QMetaType>
#include <initializer_list>
#include <tuple>

namespace BlackMisc
{
    namespace Aviation
    {
        //! Value object encapsulating a list of aircraft engines.
        class BLACKMISC_EXPORT CAircraftEngineList :
            public CSequence<CAircraftEngine>,
            public BlackMisc::Mixin::MetaType<CAircraftEngineList>,
            public BlackMisc::Mixin::JsonOperators<CAircraftEngineList>
        {
        public:
            BLACKMISC_DECLARE_USING_MIXIN_METATYPE(CAircraftEngineList)

            //! Default constructor.
            CAircraftEngineList() = default;

            //! Construct by bool values for engines 1,2 ...
            CAircraftEngineList(std::initializer_list<bool> enginesOnOff);

            //! Construct from a base class object.
            CAircraftEngineList(const CSequence<CAircraftEngine> &other);

            //! Get engine 1..n
            CAircraftEngine getEngine(int engineNumber) const;

            //! Engine number 1..x on?
            bool isEngineOn(int engineNumber) const;

            //! Is any engine on?
            bool isAnyEngineOn() const;

            //! \copydoc BlackMisc::Mixin::JsonByMetaClass::toJson
            QJsonObject toJson() const;

            //! \copydoc BlackMisc::Mixin::JsonByMetaClass::convertFromJson
            void convertFromJson(const QJsonObject &json);
        };
    } //namespace
} // namespace

Q_DECLARE_METATYPE(BlackMisc::Aviation::CAircraftEngineList)
Q_DECLARE_METATYPE(BlackMisc::CCollection<BlackMisc::Aviation::CAircraftEngine>)
Q_DECLARE_METATYPE(BlackMisc::CSequence<BlackMisc::Aviation::CAircraftEngine>)

#endif //guard

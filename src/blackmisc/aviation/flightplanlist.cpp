/* Copyright (C) 2017
 * swift Project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "flightplanlist.h"
#include "blackmisc/compare.h"
#include "blackmisc/iterator.h"
#include "blackmisc/predicates.h"
#include "blackmisc/range.h"

#include <QString>
#include <QtGlobal>

namespace BlackMisc
{
    namespace Aviation
    {
        CFlightPlanList::CFlightPlanList() { }

        CFlightPlanList::CFlightPlanList(const CSequence<CFlightPlan> &other) :
            CSequence<CFlightPlan>(other)
        { }
    } // namespace
} // namespace

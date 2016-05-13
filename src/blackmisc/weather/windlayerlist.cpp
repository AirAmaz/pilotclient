/* Copyright (C) 2016
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "blackmisc/aviation/altitude.h"
#include "blackmisc/compare.h"
#include "blackmisc/weather/windlayerlist.h"

using namespace BlackMisc::Aviation;

namespace BlackMisc
{
    namespace Weather
    {
        CWindLayerList::CWindLayerList(const CSequence<CWindLayer> &other) :
            CSequence<CWindLayer>(other)
        { }

        bool CWindLayerList::containsLevel(const CAltitude &level) const
        {
            return contains(&CWindLayer::getLevel, level);
        }

        CWindLayer CWindLayerList::findByLevel(const CAltitude &level) const
        {
            return findFirstByOrDefault(&CWindLayer::getLevel, level);
        }

    } // namespace
} // namespace

/* Copyright (C) 2016
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "blackmisc/metaclassprivate.h"
#include "blackmisc/range.h"
#include "blackmisc/weather/weatherdataplugininfo.h"
#include "blackmisc/weather/weatherdataplugininfolist.h"

#include <tuple>

namespace BlackMisc
{
    namespace Weather
    {

        CWeatherDataPluginInfoList::CWeatherDataPluginInfoList() { }

        QStringList CWeatherDataPluginInfoList::toStringList(bool i18n) const
        {
            return this->transform([i18n](const CWeatherDataPluginInfo & info) { return info.toQString(i18n); });
        }

    } // namespace
} // namespace

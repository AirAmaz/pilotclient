/* Copyright (C) 2013
 * swift Project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "blackmisc/aviation/transponder.h"
#include "blackmisc/pq/units.h"
#include "blackmisc/simulation/fscommon/bcdconversions.h"

using namespace BlackMisc::PhysicalQuantities;
using namespace BlackMisc::Aviation;

namespace BlackMisc
{
    namespace Simulation
    {
        namespace FsCommon
        {

            quint32 CBcdConversions::comFrequencyToBcdHz(const BlackMisc::PhysicalQuantities::CFrequency &comFrequency)
            {
                // FSX documentation is wrong, we need to use kHz + 2 digits, not Hz
                quint32 f = comFrequency.valueRounded(CFrequencyUnit::kHz(), 0) / 10;
                f = dec2Bcd(f);
                return f;
            }

            quint32 CBcdConversions::transponderCodeToBcd(const BlackMisc::Aviation::CTransponder &transponder)
            {
                // FSX documentation is wrong, we need to use kHz + 2 digits, not Hz
                quint32 t = transponder.getTransponderCode();
                t = dec2Bcd(t);
                return t;
            }

            quint32 CBcdConversions::hornerScheme(quint32 num, quint32 divider, quint32 factor)
            {
                quint32 remainder = 0, quotient = 0, result = 0;
                remainder = num % divider;
                quotient = num / divider;
                if (!(quotient == 0 && remainder == 0))
                {
                    result += hornerScheme(quotient, divider, factor) * factor + remainder;
                }
                return result;
            }

        } // namespace
    } // namespace
} // namespacee

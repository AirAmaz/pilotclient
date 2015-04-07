/* Copyright (C) 2014
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "aircraftengine.h"

namespace BlackMisc
{
    namespace Aviation
    {

        CAircraftEngine::CAircraftEngine(int number, bool on) : m_number(number), m_on(on)
        {
            Q_ASSERT_X(number > 0, "CAircraftEngine", "Engine number have to be > 1");
        }

        void CAircraftEngine::setNumber(int number)
        {
            Q_ASSERT_X(number > 0, "setNumber", "Engine number have to be > 1");
            m_number = number;
        }

        QString CAircraftEngine::convertToQString(bool i18n) const
        {
            Q_UNUSED(i18n);
            QString s(m_number);
            s += BlackMisc::boolToOnOff(m_on);
            return s;
        }

    } // namespace
} // namespace

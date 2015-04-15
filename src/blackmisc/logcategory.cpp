/* Copyright (C) 2014
 * Swift Project Community / Contributors
 *
 * This file is part of Swift Project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "logcategory.h"

namespace BlackMisc
{
    QString CLogCategory::convertToQString(bool i18n) const
    {
        Q_UNUSED(i18n);
        return m_string;
    }
}

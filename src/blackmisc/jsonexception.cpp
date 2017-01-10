/* Copyright (C) 2016
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "blackmisc/jsonexception.h"
#include "blackmisc/statusmessage.h"
#include "blackmisc/logcategorylist.h"
#include "blackmisc/logmessage.h"
#include <QStringBuilder>
#include <QThreadStorage>
#include <vector>

namespace BlackMisc
{
    //! \private
    auto &jsonStack() noexcept
    {
        static QThreadStorage<std::vector<const CJsonScope *>> stack;
        return stack.localData();
    }

    CStatusMessage CJsonException::toStatusMessage(const CLogCategoryList &categories, const QString &prefix) const
    {
        return CStatusMessage(categories).validationError("%1: %2 in '%3'") << prefix << what() << getStackTrace();
    }

    void CJsonException::toLogMessage(const CLogCategoryList &categories, const QString &prefix) const
    {
        CLogMessage(categories).validationError("%1: %2 in '%3'") << prefix << what() << getStackTrace();
    }

    QString CJsonException::stackString()
    {
        QStringList list;
        for (const auto scope : BlackMisc::as_const(jsonStack()))
        {
            list.push_back(scope->m_string ? *scope->m_string : scope->m_latin1);
            if (scope->m_index >= 0) { list.back() += "[" % QString::number(scope->m_index) % "]"; }
        }
        return list.isEmpty() ? QStringLiteral("<document root>") : list.join('.');
    }

    void CJsonScope::push() const noexcept
    {
        jsonStack().push_back(this);
    }

    void CJsonScope::pop() const noexcept
    {
        Q_ASSERT(jsonStack().back() == this);
        jsonStack().pop_back();
    }
}

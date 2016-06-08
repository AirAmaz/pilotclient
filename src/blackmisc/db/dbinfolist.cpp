/* Copyright (C) 2016
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "dbinfolist.h"

using namespace BlackMisc::Network;

namespace BlackMisc
{
    namespace Db
    {
        CDbInfoList::CDbInfoList(const CSequence<CDbInfo> &other) :
            CSequence<CDbInfo>(other)
        { }

        CDbInfo CDbInfoList::findFirstByEntityOrDefault(CEntityFlags::Entity entity) const
        {
            for (const CDbInfo &info : *this)
            {
                if (info.matchesEntity(entity)) { return info; }
            }
            return CDbInfo();
        }

        CDbInfoList CDbInfoList::fromDatabaseJson(const QJsonArray &array)
        {
            CDbInfoList infoObjects;
            for (const QJsonValue &value : array)
            {
                const CDbInfo info(CDbInfo::fromDatabaseJson(value.toObject()));
                infoObjects.push_back(info);
            }
            return infoObjects;
        }
    } // namespace
} // namespace

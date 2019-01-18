/* Copyright (C) 2019
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "blackmisc/aviation/aircraftcategory.h"
#include "blackmisc/db/datastoreutility.h"
#include "blackmisc/comparefunctions.h"
#include "blackmisc/logcategorylist.h"
#include "blackmisc/propertyindex.h"
#include "blackmisc/statusmessage.h"
#include "blackmisc/variant.h"

#include <QChar>
#include <QJsonValue>
#include <QMultiMap>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QStringBuilder>
#include <Qt>
#include <QtGlobal>

using namespace BlackMisc;
using namespace BlackMisc::Db;
using namespace BlackMisc::Simulation;

namespace BlackMisc
{
    namespace Aviation
    {
        CAircraftCategory::CAircraftCategory(const QString &name, const QString &description, const QString &path, bool assignable) :
            m_name(name), m_description(description), m_path(path), m_assignable(assignable)
        {  }

        QString CAircraftCategory::getNameDbKey() const
        {
            return (this->isLoadedFromDb()) ?
                   this->getName() % u' ' % this->getDbKeyAsStringInParentheses() :
                   this->getName();
        }

        QString CAircraftCategory::convertToQString(bool i18n) const
        {
            Q_UNUSED(i18n);
            return QStringLiteral("%1 %2").arg(this->getNameDbKey(), this->getDescription());
        }

        CStatusMessageList CAircraftCategory::validate() const
        {
            static const CLogCategoryList cats({ CLogCategory("swift.blackmisc.aircraftcategory"), CLogCategory::validation()});
            CStatusMessageList msg;
            return msg;
        }

        bool CAircraftCategory::isNull() const
        {
            return m_name.isEmpty() && m_description.isEmpty();
        }

        const CAircraftCategory &CAircraftCategory::null()
        {
            static const CAircraftCategory null;
            return null;
        }

        bool CAircraftCategory::hasName() const
        {
            return !m_name.isEmpty();
        }

        bool CAircraftCategory::matchesName(const QString &name, Qt::CaseSensitivity cs) const
        {
            return stringCompare(name, this->getName(), cs);
        }

        void CAircraftCategory::setLevel(int l1, int l2, int l3)
        {
            m_level[0] = l1;
            m_level[1] = l2;
            m_level[2] = l3;
        }

        QString CAircraftCategory::getLevelString() const
        {
            return QStringLiteral("%1.%2.%3").arg(m_level[0]).arg(m_level[1]).arg(m_level[2]);
        }

        bool CAircraftCategory::matchesPath(const QString &path, Qt::CaseSensitivity cs)
        {
            return stringCompare(path, this->getPath(), cs);
        }

        CVariant CAircraftCategory::propertyByIndex(const BlackMisc::CPropertyIndex &index) const
        {
            if (index.isMyself()) { return CVariant::from(*this); }
            if (IDatastoreObjectWithIntegerKey::canHandleIndex(index)) { return IDatastoreObjectWithIntegerKey::propertyByIndex(index); }
            const ColumnIndex i = index.frontCasted<ColumnIndex>();
            switch (i)
            {
            case IndexName:        return CVariant::fromValue(m_name);
            case IndexDescription: return CVariant::fromValue(m_description);
            case IndexAssignable:  return CVariant::fromValue(m_assignable);
            case IndexPath:        return CVariant::fromValue(m_path);
            case IndexLevelString: return CVariant::fromValue(this->getLevelString());
            default:               return CValueObject::propertyByIndex(index);
            }
        }

        void CAircraftCategory::setPropertyByIndex(const CPropertyIndex &index, const CVariant &variant)
        {
            if (index.isMyself()) { (*this) = variant.to<CAircraftCategory>(); return; }
            if (IDatastoreObjectWithIntegerKey::canHandleIndex(index)) { IDatastoreObjectWithIntegerKey::setPropertyByIndex(index, variant); return; }
            const ColumnIndex i = index.frontCasted<ColumnIndex>();
            switch (i)
            {
            case IndexName:        this->setName(variant.value<QString>()); break;
            case IndexDescription: this->setDescription(variant.value<QString>()); break;
            case IndexAssignable:  this->setAssignable(variant.value<bool>()); break;
            default: CValueObject::setPropertyByIndex(index, variant); break;
            }
        }

        int CAircraftCategory::comparePropertyByIndex(const CPropertyIndex &index, const CAircraftCategory &compareValue) const
        {
            if (index.isMyself()) { return m_path.compare(compareValue.getPath(), Qt::CaseInsensitive); }
            if (IDatastoreObjectWithIntegerKey::canHandleIndex(index)) { return IDatastoreObjectWithIntegerKey::comparePropertyByIndex(index, compareValue);}
            const ColumnIndex i = index.frontCasted<ColumnIndex>();
            switch (i)
            {
            case IndexName: return m_name.compare(compareValue.getName(), Qt::CaseInsensitive);
            case IndexPath: return m_description.compare(compareValue.getPath(), Qt::CaseInsensitive);
            case IndexDescription: return m_path.compare(compareValue.getDescription(), Qt::CaseInsensitive);
            case IndexAssignable: return Compare::compare(this->isAssignable(), compareValue.isAssignable());
            default: return CValueObject::comparePropertyByIndex(index, *this);
            }
            Q_ASSERT_X(false, Q_FUNC_INFO, "No comparison");
            return 0;
        }

        CAircraftCategory CAircraftCategory::fromDatabaseJson(const QJsonObject &json, const QString &prefix)
        {
            const QString name(json.value(prefix % u"name").toString());
            const QString description(json.value(prefix % u"description").toString());
            const QString path(json.value(prefix % u"path").toString());
            const bool assignable = CDatastoreUtility::dbBoolStringToBool(json.value(prefix % u"assignable").toString());
            const int l1 = json.value(prefix % u"l1").toInt();
            const int l2 = json.value(prefix % u"l2").toInt();
            const int l3 = json.value(prefix % u"l3").toInt();

            CAircraftCategory cat(name, description, path, assignable);
            cat.setLevel(l1, l2, l3);
            cat.setKeyVersionTimestampFromDatabaseJson(json, prefix);
            return cat;
        }
    } // namespace
} // namespace
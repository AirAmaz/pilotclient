/* Copyright (C) 2013
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "blackmisc/network/user.h"
#include "blackmisc/aviation/airporticaocode.h"
#include "blackmisc/icon.h"
#include "blackmisc/propertyindex.h"
#include "blackmisc/variant.h"
#include <tuple>
#include <QRegularExpression>

using namespace BlackMisc::Aviation;

namespace BlackMisc
{
    namespace Network
    {
        CUser::CUser(const CCallsign &callsign)
            : m_callsign(callsign)
        {
            this->deriveHomeBaseFromCallsign();
        }

        CUser::CUser(const QString &id, const QString &realname, const CCallsign &callsign)
            : m_id(id.trimmed()), m_realname(realname), m_callsign(callsign)
        {
            this->deriveHomeBaseFromCallsign();
            this->setRealName(realname); // extracts homebase if this is included in real name
        }

        CUser::CUser(const QString &id, const QString &realname, const QString &email, const QString &password, const CCallsign &callsign)
            : m_id(id.trimmed()), m_realname(realname), m_email(email), m_password(password), m_callsign(callsign)
        {
            this->deriveHomeBaseFromCallsign();
            this->setRealName(realname); // extracts homebase
        }

        void CUser::setCallsign(const CCallsign &callsign)
        {
            m_callsign = callsign;
            this->deriveHomeBaseFromCallsign();
        }

        QString CUser::convertToQString(bool i18n) const
        {
            Q_UNUSED(i18n);
            if (this->m_realname.isEmpty()) return "<no realname>";
            QString s = this->m_realname;
            if (this->hasValidId())
            {
                s.append(" (").append(this->m_id).append(')');
            }
            if (this->hasValidCallsign())
            {
                s.append(' ').append(this->getCallsign().getStringAsSet());
            }
            return s;
        }

        void CUser::deriveHomeBaseFromCallsign()
        {
            if (this->m_callsign.isEmpty()) { return; }
            if (this->m_homebase.isEmpty())
            {
                if (this->m_callsign.isAtcCallsign())
                {
                    this->m_homebase = this->m_callsign.getIcaoCode();
                }
            }
        }

        void CUser::setRealName(const QString &realname)
        {
            QString rn(realname.trimmed().simplified());
            if (rn.isEmpty())
            {
                this->m_realname = "";
                return;
            }

            if (!this->hasValidHomebase())
            {
                // only apply stripping if home base is not explicitly given
                // try to strip homebase: I understand the limitations, but we will have more correct hits as failures I assume
                static QThreadStorage<QRegularExpression> tsRegex;
                if (! tsRegex.hasLocalData()) { tsRegex.setLocalData(QRegularExpression("(-\\s*|\\s)([A-Z]{4})$")); }
                const auto &regex = tsRegex.localData();
                QRegularExpressionMatch match = regex.match(rn);
                if (match.hasMatch())
                {
                    int pos = match.capturedStart(0);
                    QString icao = match.captured(0).trimmed().right(4);
                    rn = rn.left(pos).trimmed();
                    this->setHomebase(CAirportIcaoCode(icao));
                }
            }

            // do not beautify before stripping home base
            this->m_realname = beautifyRealName(rn);
        }

        CStatusMessageList CUser::validate() const
        {
            CStatusMessageList msgs;
            // callsign optional
            if (!this->hasValidId()) { msgs.push_back(CStatusMessage(CStatusMessage::SeverityWarning, "Invalid id"));}
            if (!this->hasValidRealName()) { msgs.push_back(CStatusMessage(CStatusMessage::SeverityWarning, "Invalid real name"));}
            if (!this->hasValidCredentials()) { msgs.push_back(CStatusMessage(CStatusMessage::SeverityWarning, "Invalid credentials"));}
            return msgs;
        }

        void CUser::updateMissingParts(const CUser &otherUser)
        {
            if (this == &otherUser) { return; }
            if (!this->hasValidRealName()) { this->setRealName(otherUser.getRealName()); }
            if (!this->hasValidId()) { this->setId(otherUser.getId()); }
            if (!this->hasValidEmail()) { this->setEmail(otherUser.getEmail()); }
            if (!this->hasValidCallsign()) { this->setCallsign(otherUser.getCallsign()); }
        }

        void CUser::syncronizeData(CUser &otherUser)
        {
            if (this == &otherUser) { return; }
            this->updateMissingParts(otherUser);
            otherUser.updateMissingParts(*this);
        }

        bool CUser::isValidVatsimId(const QString &id)
        {
            if (id.isEmpty()) { return false; }
            bool ok;
            int i = id.toInt(&ok);
            if (!ok) { return false; }
            return i >= 100000 && i <= 9999999;
        }

        QString CUser::beautifyRealName(const QString &realName)
        {
            QString newRealName(realName.simplified().trimmed().toLower());
            if (newRealName.isEmpty()) { return ""; }

            // simple title case
            QString::Iterator i = newRealName.begin();
            bool upperNextChar = true;
            while (i != newRealName.end())
            {
                if (i->isSpace())
                {
                    upperNextChar = true;
                }
                else if (upperNextChar)
                {
                    QChar u(i->toUpper());
                    *i = u;
                    upperNextChar = false;
                }
                ++i;
            }
            return newRealName;
        }

        CVariant CUser::propertyByIndex(const BlackMisc::CPropertyIndex &index) const
        {
            if (index.isMyself()) { return CVariant::from(*this); }
            ColumnIndex i = index.frontCasted<ColumnIndex>();
            switch (i)
            {
            case IndexEmail:
                return CVariant(this->m_email);
            case IndexId:
                return CVariant(this->m_id);
            case IndexPassword:
                return CVariant(this->m_password);
            case IndexRealName:
                return CVariant(this->m_realname);
            case IndexHomebase:
                return this->m_homebase.propertyByIndex(index.copyFrontRemoved());
            case IndexCallsign:
                return this->m_callsign.propertyByIndex(index.copyFrontRemoved());
            default:
                return CValueObject::propertyByIndex(index);
            }
        }

        void CUser::setPropertyByIndex(const CPropertyIndex &index, const CVariant &variant)
        {
            if (index.isMyself()) { (*this) = variant.to<CUser>(); return; }
            ColumnIndex i = index.frontCasted<ColumnIndex>();
            switch (i)
            {
            case IndexEmail:
                this->setEmail(variant.value<QString>());
                break;
            case IndexId:
                this->setId(variant.value<QString>());
                break;
            case IndexPassword:
                this->setPassword(variant.value<QString>());
                break;
            case IndexRealName:
                this->setRealName(variant.value<QString>());
                break;
            case IndexHomebase:
                this->m_homebase.setPropertyByIndex(index.copyFrontRemoved(), variant);
                break;
            case IndexCallsign:
                this->m_callsign.setPropertyByIndex(index.copyFrontRemoved(), variant);
                break;
            default:
                CValueObject::setPropertyByIndex(index, variant);
                break;
            }
        }

        int CUser::comparePropertyByIndex(const CPropertyIndex &index, const CUser &compareValue) const
        {
            if (index.isMyself()) { return this->getRealName().compare(compareValue.getRealName(), Qt::CaseInsensitive); }
            ColumnIndex i = index.frontCasted<ColumnIndex>();
            switch (i)
            {
            case IndexEmail:
                return this->m_email.compare(compareValue.getEmail(), Qt::CaseInsensitive);
            case IndexId:
                return this->m_id.compare(compareValue.getId(), Qt::CaseInsensitive);
            case IndexPassword:
                break;
            case IndexRealName:
                return this->m_realname.compare(compareValue.getRealName(), Qt::CaseInsensitive);
            case IndexHomebase:
                return this->m_homebase.comparePropertyByIndex(index.copyFrontRemoved(), compareValue.getHomebase());
            case IndexCallsign:
                return this->m_callsign.comparePropertyByIndex(index.copyFrontRemoved(), compareValue.getCallsign());
            default:
                break;
            }
            Q_ASSERT_X(false, Q_FUNC_INFO, "compare failed");
            return 0;
        }
    } // namespace
} // namespace

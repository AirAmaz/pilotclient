/* Copyright (C) 2016
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "blackmisc/network/fsdsetup.h"
#include "blackmisc/logcategory.h"
#include "blackmisc/logcategorylist.h"
#include "blackmisc/propertyindex.h"
#include "blackmisc/statusmessage.h"
#include "blackmisc/stringutils.h"
#include "blackmisc/variant.h"

#include <Qt>
#include <QtGlobal>

using namespace BlackMisc;

namespace BlackMisc
{
    namespace Network
    {
        CFsdSetup::CFsdSetup(const QString &codec, SendReceiveDetails sendReceive)
            : m_textCodec(codec), m_sendReceive(sendReceive) {}

        CFsdSetup::SendReceiveDetails CFsdSetup::getSendReceiveDetails() const
        {
            return static_cast<SendReceiveDetails>(m_sendReceive);
        }

        QString CFsdSetup::convertToQString(bool i18n) const
        {
            Q_UNUSED(i18n);
            static const QString s("Codec: '%1' details: '%2'");
            return s.arg(this->getTextCodec(), CFsdSetup::sendReceiveDetailsToString(this->getSendReceiveDetails()));
        }

        QString CFsdSetup::sendReceiveDetailsToString(SendReceiveDetails details)
        {
            static const QString ds("Send parts; %1 interim: %2 Receive parts: %3 interim: %4");
            return ds.arg(boolToYesNo(details.testFlag(SendAircraftParts)),
                          boolToYesNo(details.testFlag(SendIterimPositions)),
                          boolToYesNo(details.testFlag(ReceiveAircraftParts)),
                          boolToYesNo(details.testFlag(ReceiveInterimPositions)));
        }

        void CFsdSetup::setSendReceiveDetails(bool partsSend, bool partsReceive, bool interimSend, bool interimReceive)
        {
            SendReceiveDetails s = Nothing;
            if (partsSend) { s |= SendAircraftParts; }
            if (partsReceive) { s |= ReceiveAircraftParts; }
            if (interimSend) { s |= SendIterimPositions; }
            if (interimReceive) { s |= ReceiveInterimPositions; }
            this->setSendReceiveDetails(s);
        }

        const CFsdSetup &CFsdSetup::vatsimStandard()
        {
            static const CFsdSetup s;
            return s;
        }

        CStatusMessageList CFsdSetup::validate() const
        {
            static const CLogCategoryList cats(CLogCategoryList(this).join({ CLogCategory::validation()}));
            CStatusMessageList msgs;
            if (this->getTextCodec().isEmpty()) { msgs.push_back(CStatusMessage(CStatusMessage::SeverityError, "No codec")); }
            if (!textCodecNames(true, true).contains(this->getTextCodec())) { msgs.push_back(CStatusMessage(CStatusMessage::SeverityError, "Unrecognized codec name")); }
            msgs.addCategories(cats);
            return msgs;
        }

        CVariant CFsdSetup::propertyByIndex(const BlackMisc::CPropertyIndex &index) const
        {
            if (index.isMyself()) { return CVariant::from(*this); }
            ColumnIndex i = index.frontCasted<ColumnIndex>();
            switch (i)
            {
            case IndexTextCodec:
                return CVariant::fromValue(this->m_textCodec);
            case IndexSendReceiveDetails:
                return CVariant::fromValue(this->m_sendReceive);
            default:
                return CValueObject::propertyByIndex(index);
            }
        }

        void CFsdSetup::setPropertyByIndex(const CPropertyIndex &index, const CVariant &variant)
        {
            if (index.isMyself()) { (*this) = variant.to<CFsdSetup>(); return; }
            ColumnIndex i = index.frontCasted<ColumnIndex>();
            switch (i)
            {
            case IndexTextCodec:
                this->setTextCodec(variant.value<QString>());
                break;
            case IndexSendReceiveDetails:
                this->setSendReceiveDetails(variant.value<SendReceiveDetails>());
                break;
            default:
                CValueObject::setPropertyByIndex(index, variant);
                break;
            }
        }
    } // namespace
} // namespace

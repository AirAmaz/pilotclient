/* Copyright (C) 2019
 * swift project community / contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution. No part of swift project, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 */

//! \file

#ifndef BLACKCORE_FSD_PLANEINFOREQUESTFSINN_H
#define BLACKCORE_FSD_PLANEINFOREQUESTFSINN_H

#include "messagebase.h"

namespace BlackCore
{
    namespace Fsd
    {
        class BLACKCORE_EXPORT PlaneInfoRequestFsinn : public MessageBase
        {
        public:
            PlaneInfoRequestFsinn(const QString &sender,
                                  const QString &receiver,
                                  const QString &airlineIcao,
                                  const QString &aircraftIcao,
                                  const QString &aircraftIcaoCombinedType,
                                  const QString &sendMModelString);

            virtual ~PlaneInfoRequestFsinn() {}

            QStringList toTokens() const;
            static PlaneInfoRequestFsinn fromTokens(const QStringList &tokens);
            static QString pdu() { return QStringLiteral("#SB"); }

            QString m_airlineIcao;
            QString m_aircraftIcao;
            QString m_aircraftIcaoCombinedType;
            QString m_sendMModelString;

        private:
            PlaneInfoRequestFsinn();
        };

        inline bool operator==(const PlaneInfoRequestFsinn &lhs, const PlaneInfoRequestFsinn &rhs)
        {
            return  lhs.sender() == rhs.sender() &&
                    lhs.receiver() == rhs.receiver() &&
                    lhs.m_airlineIcao == rhs.m_airlineIcao &&
                    lhs.m_aircraftIcao == rhs.m_aircraftIcao &&
                    lhs.m_aircraftIcaoCombinedType == rhs.m_aircraftIcaoCombinedType &&
                    lhs.m_sendMModelString == rhs.m_sendMModelString;
        }

        inline bool operator!=(const PlaneInfoRequestFsinn &lhs, const PlaneInfoRequestFsinn &rhs)
        {
            return !(lhs == rhs);
        }
    }
}

#endif // guard

/* Copyright (C) 2014
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution. No part of swift project, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 */

#include "blackmisc/tokenbucket.h"
#include "blackmisc/pq/units.h"

#include <QtGlobal>

using namespace BlackMisc::PhysicalQuantities;

namespace BlackMisc
{
    CTokenBucket::CTokenBucket(int capacity, const CTime &interval, int numTokensToRefill)
        : m_capacity(capacity), m_numTokensToRefill(numTokensToRefill), m_intervalMs(interval.value(CTimeUnit::ms()))
    {}

    CTokenBucket::CTokenBucket(int capacity, qint64 intervalMs, int numTokensToRefill)
        : m_capacity(capacity), m_numTokensToRefill(numTokensToRefill), m_intervalMs(intervalMs)
    {}

    bool CTokenBucket::tryConsume(int numTokens, qint64 msSinceEpoch)
    {
        Q_ASSERT(numTokens > 0 && numTokens < m_capacity);

        // enough tokens in stock?
        if (numTokens <= m_availableTokens)
        {
            m_availableTokens -= numTokens;
            return true;
        }

        // Replenish maximal up to capacity
        const int tokens = this->getTokens(msSinceEpoch);
        const int replenishedTokens = qMin(m_capacity, tokens);

        // Take care of overflows
        m_availableTokens = qMin(m_availableTokens + replenishedTokens, m_capacity);

        // check again after replenishment
        if (numTokens <= m_availableTokens)
        {
            m_availableTokens -= numTokens;
            return true;
        }
        return false;
    }

    void CTokenBucket::setNumberOfTokensToRefill(int numTokens)
    {
        m_numTokensToRefill = numTokens;
    }

    void CTokenBucket::setCapacity(int capacity)
    {
        m_capacity = capacity;
    }

    void CTokenBucket::setCapacityAndTokensToRefill(int numTokens)
    {
        this->setCapacity(numTokens);
        this->setNumberOfTokensToRefill(numTokens);
    }

    int CTokenBucket::getTokensPerSecond() const
    {
        if (m_intervalMs < 1) { return 0; }
        return m_numTokensToRefill * 1000 / m_intervalMs;
    }

    int CTokenBucket::getTokens(qint64 msSinceEpoch)
    {
        const qint64 now = msSinceEpoch > 0 ? msSinceEpoch : QDateTime::currentMSecsSinceEpoch();
        const qint64 deltaMs = now - m_lastReplenishmentTime;
        const int numberOfTokens = static_cast<int>(m_numTokensToRefill * deltaMs / m_intervalMs);

        // Update the time only when replenishment actually took place. We will end up in a infinite loop otherwise.
        if (numberOfTokens > 0) { m_lastReplenishmentTime = now; }
        return numberOfTokens;
    }
} // namespace

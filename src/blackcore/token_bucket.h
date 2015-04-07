/* Copyright (C) 2014
 * swift Project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#ifndef BLACKCORE_TOKENBUCKET_H
#define BLACKCORE_TOKENBUCKET_H

#include "blackmisc/pq/time.h"
#include <QtGlobal>
#include <QDateTime>

namespace BlackCore
{
    /*!
     * Token bucket algorithm
     *
     * This class implements the token bucket algorithm. Tokens as arbitrary unit are added to the bucket at a defined rate.
     * Token can be consumsed as long as there are enough available. This class can be used to throttle traffic and packet
     * generation. Each time a packet needs to be generated and sent a token is consumed. If no token is available, consumption
     * will fail the the packet cannot be sent.
     */
    class CTokenBucket
    {

    public:

        //! Constructor
        //! \tparam Replenishment policy
        CTokenBucket(int capacity, BlackMisc::PhysicalQuantities::CTime interval, int numTokensToRefill)
            : m_capacity(capacity), m_interval(interval), m_numTokensToRefill(numTokensToRefill) {}

        //! Try to consume a token
        bool tryConsume()
        {
            return tryConsume(1);
        }

        //! Try to consume a number of tokens
        bool tryConsume(int numTokens);

    private:

        //! Get available tokens since last replenishment.
        // Note that replenishment is implemented lazy. This means, tokens will not replenished
        // on regular basis via a running timer, but they will be replenished while trying to consume
        // them.
        int getTokens();

        int m_capacity = 10;                    //!< Maximum capacity of tokens
        int m_availableTokens = 10;             //!< Currently available tokens. The initial value is 10
        BlackMisc::PhysicalQuantities::CTime m_interval;            //!< Refill interval, e.g. every 5 secs
        int m_numTokensToRefill;                                    //!< Number of tokens to be refilled each interval
        QDateTime m_lastReplenishmentTime = QDateTime::currentDateTime();  //!< Last time
    };
}

#endif

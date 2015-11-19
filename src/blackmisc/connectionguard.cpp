/* Copyright (C) 2015
 * swift Project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "connectionguard.h"

namespace BlackMisc
{
    CConnectionGuard::CConnectionGuard(const QMetaObject::Connection &connection)
    {
        this->m_connections.append(connection);
    }

    void CConnectionGuard::append(const QMetaObject::Connection &connection)
    {
        this->m_connections.append(connection);
    }

    void CConnectionGuard::append(const QList<QMetaObject::Connection> &connections)
    {
        this->m_connections.append(connections);
    }

    CConnectionGuard::~CConnectionGuard()
    {
        disconnectAll();
    }

    int CConnectionGuard::disconnectAll()
    {
        if (this->m_connections.isEmpty()) { return 0; }
        int c = 0;
        for (const QMetaObject::Connection &con : this->m_connections)
        {
            if (QObject::disconnect(con)) { c++; }
        }
        this->m_connections.clear();
        return c;
    }
} // ns

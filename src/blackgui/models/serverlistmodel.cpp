/* Copyright (C) 2013
 * swift Project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "blackgui/models/columns.h"
#include "blackgui/models/serverlistmodel.h"
#include "blackmisc/network/server.h"
#include "blackmisc/network/user.h"

#include <QtGlobal>

using namespace BlackMisc::Network;

namespace BlackGui
{
    namespace Models
    {
        CServerListModel::CServerListModel(QObject *parent) :
            CListModelBase("ServerListModel", parent)
        {
            this->m_columns.addColumn(CColumn::standardString("name", CServer::IndexName));
            this->m_columns.addColumn(CColumn::standardString("description", CServer::IndexDescription));
            this->m_columns.addColumn(CColumn::standardString("address", CServer::IndexAddress));
            this->m_columns.addColumn(CColumn::standardString("port", CServer::IndexPort));
            this->m_columns.addColumn(CColumn::standardString("realname", { CServer::IndexUser, CUser::IndexRealName}));
            this->m_columns.addColumn(CColumn::standardString("userid", { CServer::IndexUser, CUser::IndexId}));

            // force strings for translation in resource files
            (void)QT_TRANSLATE_NOOP("ServerListModel", "name");
            (void)QT_TRANSLATE_NOOP("ServerListModel", "description");
            (void)QT_TRANSLATE_NOOP("ServerListModel", "address");
            (void)QT_TRANSLATE_NOOP("ServerListModel", "port");
            (void)QT_TRANSLATE_NOOP("ServerListModel", "realname");
            (void)QT_TRANSLATE_NOOP("ServerListModel", "userid");
        }

    } // class
} // namespace

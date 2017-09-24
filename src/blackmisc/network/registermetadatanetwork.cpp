/* Copyright (C) 2015
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "registermetadatanetwork.h"
#include "network.h"

namespace BlackMisc
{
    namespace Network
    {
        void registerMetadata()
        {
            CAuthenticatedUser::registerMetadata();
            CClient::registerMetadata();
            CClientList::registerMetadata();
            CEntityFlags::registerMetadata();
            CFsdSetup::registerMetadata();
            CRemoteFile::registerMetadata();
            CRemoteFileList::registerMetadata();
            CRole::registerMetadata();
            CRoleList::registerMetadata();
            CServer::registerMetadata();
            CServerList::registerMetadata();
            CTextMessage::registerMetadata();
            CTextMessageList::registerMetadata();
            CUrl::registerMetadata();
            CUrlList::registerMetadata();
            CFailoverUrlList::registerMetadata();
            CUser::registerMetadata();
            CUserList::registerMetadata();
            CVoiceCapabilities::registerMetadata();
        }
    } // ns
} // ns

/* Copyright (C) 2015
 * swift project community / contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file

#ifndef BLACKCORE_DATA_VATSIMDATA_H
#define BLACKCORE_DATA_VATSIMDATA_H

#include "blackcore/blackcoreexport.h"
#include "blackmisc/datacache.h"
#include "blackmisc/metaclass.h"
#include "blackmisc/network/serverlist.h"
#include "blackmisc/network/urllist.h"
#include "blackmisc/network/user.h"
#include "blackmisc/propertyindex.h"
#include "blackmisc/timestampbased.h"
#include "blackmisc/valueobject.h"
#include "blackmisc/variant.h"

#include <QMetaType>
#include <QString>

namespace BlackCore
{
    namespace Data
    {
        //! VATSIM data (servers, URLs) cached as last known good setup.
        class BLACKCORE_EXPORT CVatsimSetup :
            public BlackMisc::CValueObject<CVatsimSetup>,
            public BlackMisc::ITimestampBased
        {
        public:
            //! Properties by index
            enum ColumnIndex
            {
                IndexServerFiles = BlackMisc::CPropertyIndex::GlobalIndexCVatsimSetup,
                IndexDataFiles,
                IndexMetarFiles,
                IndexFsdServers,
                IndexCVoiceServers
            };

            //! Default constructor
            CVatsimSetup();

            //! Destructor.
            ~CVatsimSetup() {}

            //! VATSIM data file URLs
            const BlackMisc::Network::CUrlList &getDataFileUrls() const { return m_dataFileUrls; }

            //! Set VATSIM data file URLs
            void setDataFileUrls(const BlackMisc::Network::CUrlList &urls) { m_dataFileUrls = urls; }

            //! Server file URLs (like data file, only servers)
            const BlackMisc::Network::CUrlList &getServerFileUrls() const { return m_serverFileUrls; }

            //! Set server file URLs (like data file, only servers)
            void setServerFileUrls(const BlackMisc::Network::CUrlList &urls) { m_serverFileUrls = urls; }

            //! METAR file URLs
            const BlackMisc::Network::CUrlList &getMetarFileUrls() const { return m_metarFileUrls; }

            //! METAR file URLs
            void setMetarFileUrls(const BlackMisc::Network::CUrlList &urls) { m_metarFileUrls = urls; }

            //! Set all URLs and indicate if something has changed
            bool setUrls(const BlackMisc::Network::CUrlList &dataFileUrls, const BlackMisc::Network::CUrlList &serverFileUrls, const BlackMisc::Network::CUrlList &metarFileUrls);

            //! FSD test servers
            const BlackMisc::Network::CServerList &getFsdServers() const { return m_fsdServers; }

            //! Set FSD servers
            void setFsdServers(const BlackMisc::Network::CServerList &servers) { m_fsdServers = servers; }

            //! Voice servers
            const BlackMisc::Network::CServerList &getVoiceServers() const { return m_voiceServers; }

            //! Set voice servers
            void setVoiceServers(const BlackMisc::Network::CServerList &servers) { m_voiceServers = servers; }

            //! Set servers
            bool setServers(const BlackMisc::Network::CServerList &fsdServers, const BlackMisc::Network::CServerList &voiceServers);

            //! \copydoc BlackMisc::Mixin::String::toQString
            QString convertToQString(bool i18n = false) const;

            //! To string
            QString convertToQString(const QString &separator, bool i18n = false) const;

            //! \copydoc BlackMisc::Mixin::Index::propertyByIndex
            BlackMisc::CVariant propertyByIndex(const BlackMisc::CPropertyIndex &index) const;

            //! \copydoc BlackMisc::Mixin::Index::setPropertyByIndex
            void setPropertyByIndex(const BlackMisc::CPropertyIndex &index, const BlackMisc::CVariant &variant);

        private:
            BlackMisc::Network::CUrlList    m_serverFileUrls; //!< only the FSD servers
            BlackMisc::Network::CUrlList    m_dataFileUrls;   //!< Full VATSIM files
            BlackMisc::Network::CUrlList    m_metarFileUrls;  //!< METAR files
            BlackMisc::Network::CServerList m_fsdServers;     //!< FSD test servers
            BlackMisc::Network::CServerList m_voiceServers;   //!< voice servers

            BLACK_METACLASS(
                CVatsimSetup,
                BLACK_METAMEMBER(serverFileUrls),
                BLACK_METAMEMBER(dataFileUrls),
                BLACK_METAMEMBER(metarFileUrls),
                BLACK_METAMEMBER(fsdServers),
                BLACK_METAMEMBER(voiceServers),
                BLACK_METAMEMBER(timestampMSecsSinceEpoch)
            );
        };

        //! Trait for global setup data
        struct TVatsimSetup : public BlackMisc::TDataTrait<CVatsimSetup>
        {
            //! Key in data cache
            static const char *key() { return "vatsimsetup"; }

            //! First load is synchronous
            static constexpr bool isPinned() { return true; }
        };

        //! Trait for currently used VATSIM server and user
        struct TVatsimCurrentServer : public BlackMisc::TDataTrait<BlackMisc::Network::CServer>
        {
            //! Key in data cache
            static const char *key() { return "vatsimserver"; }

            //! First load is synchronous
            static constexpr bool isPinned() { return true; }
        };
    } // ns
} // ns

Q_DECLARE_METATYPE(BlackCore::Data::CVatsimSetup)

#endif // guard

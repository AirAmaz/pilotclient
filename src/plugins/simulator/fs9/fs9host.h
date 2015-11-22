/* Copyright (C) 2014
 * swift project community / contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#ifndef DIRECTPLAY_HOST_H
#define DIRECTPLAY_HOST_H

#include "directplaypeer.h"

//! \file

namespace BlackSimPlugin
{
    namespace Fs9
    {
        //! Class encapsulating a FS9 host
        class CFs9Host : public CDirectPlayPeer
        {
            Q_OBJECT

        public:
            //! Connection status
            enum HostStatus
            {
                Hosting,
                Terminated
            };

            //! Constructor
            CFs9Host(QObject *owner);

            //! Destructor
            virtual ~CFs9Host() {}

            //! Returns true if the users simulator is connected
            bool isConnected() const { return m_playerUser != 0; }

            //! Get DirectPlay host url
            QString getHostAddress();

        public slots:
            //! Send new text message
            void sendTextMessage(const QString &textMessage);

        signals:
            //! Hosting status changed
            void statusChanged(BlackSimPlugin::Fs9::CFs9Host::HostStatus);

        protected:
            //! \copydoc CContinuousWorker::initialize
            virtual void initialize() override;

            //! \copydoc CContinuousWorker::cleanup
            virtual void cleanup() override;

        private:
            //! Start host session
            HRESULT startHosting(const QString &session, const QString &callsign);

            //! Terminate a current active hosting session
            HRESULT stopHosting();

            HostStatus m_hostStatus = Terminated;
        };
    }
}

Q_DECLARE_METATYPE(BlackSimPlugin::Fs9::CFs9Host::HostStatus)

#endif // DIRECTPLAY_HOST_H

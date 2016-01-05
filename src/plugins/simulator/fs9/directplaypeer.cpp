/* Copyright (C) 2014
 * swift project community / contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "directplaypeer.h"
#include "directplayutils.h"
#include "multiplayerpacketparser.h"
#include "directplayerror.h"
#include "blackmisc/logmessage.h"
#include <QDebug>
#include <QTimer>
#include <QFile>
#include <QStringList>
#include <QScopedPointer>
#include <QMutexLocker>

using namespace BlackMisc;

namespace BlackSimPlugin
{
    namespace Fs9
    {
        CDirectPlayPeer::CDirectPlayPeer(QObject *owner, const BlackMisc::Aviation::CCallsign &callsign)
            : CContinuousWorker(owner, "peer_" + callsign.toQString()),
              m_callsign(callsign),
              m_mutexHostList(QMutex::Recursive),
              m_callbackWrapper(this, &CDirectPlayPeer::directPlayMessageHandler)
        {
        }

        CDirectPlayPeer::~CDirectPlayPeer()
        {
            if (m_directPlayPeer)
            {
                m_directPlayPeer->Close(DPNCLOSE_IMMEDIATE);
                m_directPlayPeer->Release();
            }

            SafeRelease(m_deviceAddress);

            CoUninitialize();
        }

        HRESULT CDirectPlayPeer::directPlayMessageHandler(DWORD messageId, void *msgBuffer)
        {
            HRESULT hr = S_OK;

            switch (messageId)
            {
            case DPN_MSGID_CREATE_PLAYER:
                {
                    DPNMSG_CREATE_PLAYER *pCreatePlayerMsg = static_cast<DPNMSG_CREATE_PLAYER *>(msgBuffer);

                    HRESULT hr;

                    // Get the peer info and extract its name
                    DWORD dwSize = 0;
                    DPN_PLAYER_INFO *pdpPlayerInfo = nullptr;
                    hr = DPNERR_CONNECTING;

                    // GetPeerInfo might return DPNERR_CONNECTING when connecting,
                    // so just keep calling it if it does
                    while (hr == DPNERR_CONNECTING)
                        hr = m_directPlayPeer->GetPeerInfo(pCreatePlayerMsg->dpnidPlayer, pdpPlayerInfo, &dwSize, 0);

                    if (hr == DPNERR_BUFFERTOOSMALL)
                    {
                        QScopedArrayPointer<unsigned char> memPtr(new unsigned char[dwSize]);
                        pdpPlayerInfo = reinterpret_cast<DPN_PLAYER_INFO *>(memPtr.data());
                        if (pdpPlayerInfo == nullptr)
                        {
                            break;
                        }

                        ZeroMemory(pdpPlayerInfo, dwSize);
                        pdpPlayerInfo->dwSize = sizeof(DPN_PLAYER_INFO);

                        hr = m_directPlayPeer->GetPeerInfo(pCreatePlayerMsg->dpnidPlayer, pdpPlayerInfo, &dwSize, 0);
                        if (SUCCEEDED(hr))
                        {
                            if (pdpPlayerInfo->dwPlayerFlags & DPNPLAYER_LOCAL)
                                m_playerLocal = pCreatePlayerMsg->dpnidPlayer;
                            else
                            {
                                // The first connecting player should be the user
                                if (m_playerUser == 0)
                                {
                                    m_playerUser = pCreatePlayerMsg->dpnidPlayer;

                                }
                            }
                        }
                    }

                    break;
                }

            case DPN_MSGID_RECEIVE:
                {
                    PDPNMSG_RECEIVE pReceiveMsg = static_cast<PDPNMSG_RECEIVE>(msgBuffer);

                    // Proceeed only, if the sender is our local player
                    if (pReceiveMsg->dpnidSender == m_playerUser)
                    {
                        QByteArray messageData = QByteArray((char *)pReceiveMsg->pReceiveData, pReceiveMsg->dwReceiveDataSize);

                        emit customPacketReceived(messageData);
                    }
                    break;
                }

            case DPN_MSGID_ENUM_HOSTS_RESPONSE:
                {
                    PDPNMSG_ENUM_HOSTS_RESPONSE enumHostsResponseMsg = static_cast<PDPNMSG_ENUM_HOSTS_RESPONSE>(msgBuffer);
                    const DPN_APPLICATION_DESC *applicationDescription = enumHostsResponseMsg->pApplicationDescription;

                    QMutexLocker locker(&m_mutexHostList);

                    auto iterator = std::find_if(m_hostNodeList.begin(), m_hostNodeList.end(), [&](const CHostNode & hostNode)
                    {
                        return applicationDescription->guidInstance == hostNode.getApplicationDesc().guidInstance;
                    });

                    if (iterator == m_hostNodeList.end())
                    {

                        // This host session is not in the list then so insert it.
                        CHostNode hostNode;
                        HRESULT hr;

                        // Copy the Host Address
                        if (FAILED(hr = enumHostsResponseMsg->pAddressSender->Duplicate(hostNode.getHostAddressPtr())))
                        {
                            qWarning() << "Failed to duplicate host address!";
                            return hr;
                        }

                        DPN_APPLICATION_DESC appDesc;

                        ZeroMemory(&appDesc, sizeof(DPN_APPLICATION_DESC));
                        memcpy(&appDesc, applicationDescription, sizeof(DPN_APPLICATION_DESC));

                        // Null out all the pointers we aren't copying
                        appDesc.pwszSessionName = nullptr;
                        appDesc.pwszPassword = nullptr;
                        appDesc.pvReservedData = nullptr;
                        appDesc.dwReservedDataSize = 0;
                        appDesc.pvApplicationReservedData = nullptr;
                        appDesc.dwApplicationReservedDataSize = 0;
                        //hostNode.setApplicationDesc(appDesc);
                        hostNode.setApplicationDesc(appDesc);
                        hostNode.setSessionName(QString::fromWCharArray(applicationDescription->pwszSessionName));
                        m_hostNodeList.append(hostNode);
                    }
                    break;
                }

            }

            return hr;
        }

        HRESULT CDirectPlayPeer::initDirectPlay()
        {
            HRESULT hr = S_OK;

            // Init COM so we can use CoCreateInstance
            CoInitializeEx(nullptr, COINIT_MULTITHREADED);

            // Create the IDirectPlay8Peer Object
            if (FAILED(hr = CoCreateInstance(CLSID_DirectPlay8Peer,
                                             nullptr,
                                             CLSCTX_INPROC_SERVER,
                                             IID_IDirectPlay8Peer,
                                             reinterpret_cast<void **>(&m_directPlayPeer))))
            {
                qWarning() << "Failed to create DirectPlay8Peer object!";
                return hr;
            }

            // Init DirectPlay
            if (FAILED(hr = m_directPlayPeer->Initialize(&m_callbackWrapper, m_callbackWrapper.messageHandler, 0)))
            {
                qWarning() << "Failed to initialize directplay peer!";
                return hr;
            }

            // Ensure that TCP/IP is a valid Service Provider
            if (!isServiceProviderValid(&CLSID_DP8SP_TCPIP))
            {
                hr = E_FAIL;
                qWarning() << "Service provider is invalid!";
                return hr;
            }

            return hr;
        }

        bool CDirectPlayPeer::isServiceProviderValid(const GUID * /*pGuidSP*/)
        {
            DWORD dwItems = 0;
            DWORD dwSize = 0;

            // The first call is to retrieve the size of the DPN_SERVICE_PROVIDER_INFO array
            HRESULT hr = m_directPlayPeer->EnumServiceProviders(&CLSID_DP8SP_TCPIP, nullptr, nullptr, &dwSize, &dwItems, 0);

            if (hr != DPNERR_BUFFERTOOSMALL)
            {
                qWarning() << "Failed to enumerate service providers!";
                return false;
            }

            // Allocating an array with new DPN_SERVICE_PROVIDER_INFO[items] does not work, because the struct has
            // several pointers in it. Hence EnumServiceProviders tells us how much memory it exactly needs.
            QScopedArrayPointer<unsigned char> memPtr(new unsigned char[dwSize]);
            DPN_SERVICE_PROVIDER_INFO *dpnSPInfo = reinterpret_cast<DPN_SERVICE_PROVIDER_INFO *>(memPtr.data());

            if (FAILED(hr = m_directPlayPeer->EnumServiceProviders(&CLSID_DP8SP_TCPIP, nullptr, dpnSPInfo, &dwSize, &dwItems, 0)))
            {
                qWarning() << "Failed to enumerate service providers!";
                return false;
            }

            // There are no items returned so the requested SP is not available
            if (dwItems == 0) hr = E_FAIL;

            if (SUCCEEDED(hr)) return true;
            else return false;
        }

        HRESULT CDirectPlayPeer::createDeviceAddress()
        {
            HRESULT hr = S_OK;

            // Create our IDirectPlay8Address Device Address
            if (FAILED(hr = CoCreateInstance(CLSID_DirectPlay8Address, nullptr,
                                             CLSCTX_INPROC_SERVER,
                                             IID_IDirectPlay8Address,
                                             reinterpret_cast<void **>(&m_deviceAddress))))
            {
                qWarning() << "Failed to create DirectPlay8Address instance!";
                return hr;
            }

            // Set the SP for our Device Address
            if (FAILED(hr = m_deviceAddress->SetSP(&CLSID_DP8SP_TCPIP)))
            {
                qWarning() << "Failed to set SP!";
                return hr;
            }

            return hr;
        }

        HRESULT CDirectPlayPeer::createHostAddress()
        {
            HRESULT hr = S_OK;

            // Create our IDirectPlay8Address Device Address
            if (FAILED(hr = CoCreateInstance(CLSID_DirectPlay8Address, nullptr,
                                             CLSCTX_INPROC_SERVER,
                                             IID_IDirectPlay8Address,
                                             reinterpret_cast<void **>(&m_deviceAddress))))
                return logDirectPlayError(hr);

            // Set the SP for our Device Address
            if (FAILED(hr = m_deviceAddress->SetSP(&CLSID_DP8SP_TCPIP)))
                return logDirectPlayError(hr);

            return S_OK;
        }

        HRESULT CDirectPlayPeer::sendMessage(const QByteArray &message)
        {
            HRESULT hr = S_OK;
            DPN_BUFFER_DESC dpBufferDesc;

            if ((dpBufferDesc.dwBufferSize = message.size()) == 0) return S_FALSE;

            dpBufferDesc.pBufferData = (BYTE *)message.data();

            // If m_playerUser is non zero, send it only to him
            if (FAILED(hr = m_directPlayPeer->SendTo(m_playerUser,
                            &dpBufferDesc,
                            1, 0,
                            nullptr, nullptr,
                            DPNSEND_SYNC | DPNSEND_NOLOOPBACK)))
            {
                CLogMessage(this).warning("DirectPlay: Failed to send message!");
                qDebug() << message;
            }
            return hr;
        }

        void CDirectPlayPeer::reset()
        {
            m_playerUser = 0;
        }
    }
}

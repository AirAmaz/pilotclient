/* Copyright (C) 2014
 * swift project community / contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \cond PRIVATE

#include "blackcore/vatsim/voicechannelvatlib.h"
#include "blackmisc/logmessage.h"
#include "blackmisc/statusmessage.h"

#include <QList>
#include <QStringList>
#include <QtGlobal>

using namespace BlackMisc;
using namespace BlackMisc::Audio;
using namespace BlackMisc::Aviation;

namespace BlackCore
{
    namespace Vatsim
    {
        CVoiceChannelVatlib::CVoiceChannelVatlib(VatAudioService audioService, VatUDPAudioPort udpPort, QObject *parent)
            : IVoiceChannel(parent),
              m_audioService(audioService),
              m_udpPort(udpPort)
        {
            m_voiceChannel.reset(Vat_CreateVoiceChannel(m_audioService, "", 3782, "", "", m_udpPort));
            Vat_SetConnectionChangedHandler(m_voiceChannel.data(), roomStatusUpdate, this);
            Vat_SetClientJoinedHandler(m_voiceChannel.data(), processUserJoined, this);
            Vat_SetClientLeftHandler(m_voiceChannel.data(), processUserLeft, this);
            Vat_SetVoiceTransmissionChangedHandler(m_voiceChannel.data(), processTransmissionChange, this);

        }

        CVoiceChannelVatlib::~CVoiceChannelVatlib()
        {
            Q_ASSERT(m_roomStatus == IVoiceChannel::Disconnected);
        }

        void CVoiceChannelVatlib::joinVoiceRoom(const CVoiceRoom &voiceRoom)
        {
            if (m_roomStatus == IVoiceChannel::Connecting || m_roomStatus == IVoiceChannel::Connected) return;

            // No one else is using this voice room, so prepare to join
            m_voiceRoom = voiceRoom;
            Vat_SetRoomInfo(m_voiceChannel.data(), qPrintable(voiceRoom.getHostname()), 3782,
                            qPrintable(voiceRoom.getChannel()),
                            qPrintable(m_callsign.toQString()));

            CLogMessage(this).debug() << "Joining voice room " << m_voiceRoom.getVoiceRoomUrl();
            Vat_JoinRoom(m_voiceChannel.data());
        }

        // Leave room
        void CVoiceChannelVatlib::leaveVoiceRoom()
        {
            // If this room is not connected, there is nothing to do
            if (m_roomStatus == IVoiceChannel::Disconnecting || m_roomStatus == IVoiceChannel::Disconnected) return;

            CLogMessage(this).debug() << "Leaving voice room " << m_voiceRoom.getVoiceRoomUrl();
            Vat_DisconnectFromRoom(m_voiceChannel.data());
            m_voiceRoom = {};
            m_listCallsigns = {};
        }

        CCallsignSet CVoiceChannelVatlib::getVoiceRoomCallsigns() const
        {
            return m_listCallsigns;
        }

        void CVoiceChannelVatlib::setOwnAircraftCallsign(const CCallsign &callsign)
        {
            m_callsign = callsign;
        }

        BlackMisc::Audio::CVoiceRoom CVoiceChannelVatlib::getVoiceRoom() const
        {
            return m_voiceRoom;
        }

        bool CVoiceChannelVatlib::isMuted() const
        {
            // Remove
            return false;
        }

        void CVoiceChannelVatlib::setVolume(int /* volume */)
        {
            // Remove
        }

        int CVoiceChannelVatlib::getVolume() const
        {
            // Remove
            return 100;
        }

        VatVoiceChannel CVoiceChannelVatlib::getVoiceChannel() const
        {
            return m_voiceChannel.data();
        }

        CCallsign CVoiceChannelVatlib::extractCallsign(const QString &name)
        {
            CCallsign callsign;
            if (name.isEmpty()) return callsign;

            // callsign might contain: VATSIM id, user name
            if (name.contains(" "))
            {
                QStringList parts = name.split(" ");
                callsign = CCallsign(parts[0]);
                // I throw away VATSIM id here, maybe we could use it
            }
            else
            {
                callsign = CCallsign(name);
            }

            return callsign;
        }

        void CVoiceChannelVatlib::userJoinedVoiceRoom(VatVoiceChannel, int /** id **/, const char *name)
        {
            CCallsign callsign(extractCallsign(name));
            m_listCallsigns.push_back(callsign);
            emit userJoinedRoom(callsign);
        }

        void CVoiceChannelVatlib::userLeftVoiceRoom(VatVoiceChannel, int /** id **/, const char *name)
        {
            CCallsign callsign(extractCallsign(name));
            m_listCallsigns.remove(callsign);
            emit userLeftRoom(callsign);
        }

        void CVoiceChannelVatlib::transmissionChanged(VatVoiceChannel, VatVoiceTransmissionStatus status)
        {
            if (status == vatVoiceStarted) emit audioStarted();
            else emit audioStopped();
        }

        void CVoiceChannelVatlib::updateRoomStatus(VatVoiceChannel channel, VatConnectionStatus oldVatStatus, VatConnectionStatus newVatStatus)
        {
            Q_UNUSED(channel);
            Q_UNUSED(oldVatStatus);

            IVoiceChannel::ConnectionStatus oldStatus = m_roomStatus;
            switch (newVatStatus)
            {
            case vatStatusConnecting:
                {
                    m_roomStatus = IVoiceChannel::Connecting;
                    break;
                }
            case vatStatusConnected:
                {
                    m_voiceRoom.setConnected(true);
                    m_roomStatus = IVoiceChannel::Connected;
                    break;
                }
            case vatStatusDisconnecting:
                {
                    m_roomStatus = IVoiceChannel::Disconnecting;
                    break;
                }
            case vatStatusDisconnected:
                {
                    // Clear all internals
                    m_listCallsigns.clear();
                    m_voiceRoom = {};
                    m_roomStatus = IVoiceChannel::Disconnected;
                    break;
                }
            default:
                break;
            }
            emit connectionStatusChanged(oldStatus, m_roomStatus);
        }

        //! Cast void* to a pointer of CVoiceChannelVatlib
        CVoiceChannelVatlib *cbvar_cast_voiceChannel(void *cbvar)
        {
            return static_cast<CVoiceChannelVatlib *>(cbvar);
        }

        void CVoiceChannelVatlib::processUserJoined(VatVoiceChannel channel, int id, const char *name, void *cbVar)
        {
            auto obj = cbvar_cast_voiceChannel(cbVar);
            obj->userJoinedVoiceRoom(channel, id, name);
        }

        void CVoiceChannelVatlib::processUserLeft(VatVoiceChannel channel, int id, const char *name, void *cbVar)
        {
            auto obj = cbvar_cast_voiceChannel(cbVar);
            obj->userLeftVoiceRoom(channel, id, name);
        }

        void CVoiceChannelVatlib::processTransmissionChange(VatVoiceChannel channel, VatVoiceTransmissionStatus status, void *cbVar)
        {
            auto obj = cbvar_cast_voiceChannel(cbVar);
            obj->transmissionChanged(channel, status);
        }

        void CVoiceChannelVatlib::roomStatusUpdate(VatVoiceChannel channel, VatConnectionStatus oldStatus, VatConnectionStatus newStatus, void *cbVar)
        {
            auto obj = cbvar_cast_voiceChannel(cbVar);
            obj->updateRoomStatus(channel, oldStatus, newStatus);
        }
    } // ns
} // ns
//! \endcond

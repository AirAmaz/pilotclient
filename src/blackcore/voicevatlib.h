/* Copyright (C) 2013
 * swift Project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file

#ifndef BLACKCORE_VOICE_VATLIB_H
#define BLACKCORE_VOICE_VATLIB_H

#include "blackcore/audiomixer.h"
#include "blackcore/blackcoreexport.h"
#include "blackcore/voice.h"

#include <vatlib/vatlib.h>
#include <QObject>
#include <QScopedPointer>
#include <QSharedPointer>
#include <memory>

class QTimerEvent;

struct VatAudioService_tag;
struct VatUDPAudioPort_tag;
template <class T> class QSharedPointer;

#ifdef Q_OS_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#endif

namespace BlackCore
{
    class IAudioInputDevice;
    class IAudioOutputDevice;
    class IVoiceChannel;

    //! Vatlib implementation of the IVoice interface.
    class BLACKCORE_EXPORT CVoiceVatlib : public IVoice
    {
        Q_OBJECT

    public:

        //! Default constructor
        CVoiceVatlib(QObject *parent = nullptr);

        //! Destructor
        virtual ~CVoiceVatlib();

        //! \copydoc IVoice::createVoiceChannel()
        virtual QSharedPointer<IVoiceChannel> createVoiceChannel() override;

        //! \copydoc IVoice::createInputDevice()
        virtual std::unique_ptr<IAudioInputDevice> createInputDevice() override;

        //! \copydoc IVoice::createOutputDevice()
        virtual std::unique_ptr<IAudioOutputDevice> createOutputDevice() override;

        //! \copydoc IVoice::createAudioMixer()
        virtual std::unique_ptr<IAudioMixer> createAudioMixer() override;

        //! \copydoc IVoice::connectVoice()
        virtual void connectVoice(IAudioInputDevice *device, IAudioMixer *mixer, IAudioMixer::InputPort inputPort) override;

        //! \copydoc IVoice::connectVoice()
        virtual void connectVoice(IVoiceChannel *channel, IAudioMixer *mixer, IAudioMixer::InputPort inputPort) override;

        //! \copydoc IVoice::connectVoice()
        virtual void connectVoice(IAudioMixer *mixer, IAudioMixer::OutputPort outputPort, IAudioOutputDevice *device) override;

        //! \copydoc IVoice::connectVoice()
        virtual void connectVoice(IAudioMixer *mixer, IAudioMixer::OutputPort outputPort, IVoiceChannel *channel) override;

        //! \copydoc IVoice::disconnectVoice()
        void disconnectVoice(IAudioInputDevice *device) override;

        //! \copydoc IVoice::disconnectVoice()
        void disconnectVoice(IVoiceChannel *channel) override;

        //! \copydoc IVoice::disconnectVoice()
        void disconnectVoice(IAudioMixer *mixer, IAudioMixer::OutputPort outputPort) override;

    protected: // QObject overrides

        //! Process voice lib
        virtual void timerEvent(QTimerEvent *) override;

    private:

        // this struct calls "myCustomDeallocator" to delete the pointer
        struct VatAudioServiceDeleter
        {
            static inline void cleanup(VatAudioService_tag *obj)
            {
                Vat_DestroyAudioService(obj);
            }
        };

        struct VatUDPAudioPortDeleter
        {
            static inline void cleanup(VatUDPAudioPort_tag *obj)
            {
                Vat_DestroyUDPAudioPort(obj);
            }
        };

        static void voiceLogHandler(SeverityLevel severity, const char *message);

        QScopedPointer<VatAudioService_tag, VatAudioServiceDeleter> m_audioService;
        QScopedPointer<VatUDPAudioPort_tag, VatUDPAudioPortDeleter> m_udpPort;

    };

} // namespace

#endif // guard

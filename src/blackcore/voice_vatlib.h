/* Copyright (C) 2013 VATSIM Community / authors
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BLACKCORE_VOICE_VATLIB_H
#define BLACKCORE_VOICE_VATLIB_H

#include "voice.h"
#include "../blacksound/soundgenerator.h"
#include "../blackmisc/vaudiodevicelist.h"
#include "../blackmisc/nwuserlist.h"
#include "../blackmisc/avcallsignlist.h"

#include <QScopedPointer>
#include <QMap>
#include <QSet>
#include <QString>

#ifdef Q_OS_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#endif

namespace BlackCore
{
    /*!
     * Vatlib implementation of the IVoice interface.
     */
    class CVoiceVatlib : public IVoice
    {
        Q_OBJECT

    public:
        /*!
         * \brief Constructor
         * \param parent
         */
        CVoiceVatlib(QObject *parent = nullptr);

        //! \brief Destructor
        virtual ~CVoiceVatlib();

        // Hardware devices
        // TODO: Vatlib supports multiple output devices. That basically means, you could connect
        // to different voice rooms and send their audio to different devices, e.g. ATIS to loudspeakers
        // and ATC to headspeakers. Is not important to implement that now, if ever.

        //! \copydoc IVoice::audioDevices()
        virtual const BlackMisc::Voice::CAudioDeviceList &audioDevices() const override;

        //! \copydoc IVoice::defaultAudioInputDevice()
        virtual const BlackMisc::Voice::CAudioDevice defaultAudioInputDevice() const override;

        //! \copydoc IVoice::defaultAudioOutputDevice()
        virtual const BlackMisc::Voice::CAudioDevice defaultAudioOutputDevice() const override;

        /************************************************
         * SETUP TESTS
         ***********************************************/

        //! \copydoc IVoice::runSquelchTest
        virtual void runSquelchTest() override;

        //! \copydoc IVoice::runMicTest
        virtual void runMicrophoneTest() override;

        //! \copydoc IVoice::inputSquelch
        virtual float inputSquelch() const override;

        //! \copydoc IVoice::micTestResult()
        virtual qint32 micTestResult() const override;

        //! \copydoc IVoice::micTestResultAsString
        virtual QString micTestResultAsString() const override;

    public slots:
        //! \copydoc IVoice::setMyAircraftCallsign()
        virtual void setMyAircraftCallsign(const BlackMisc::Aviation::CCallsign &callsign) override;

        //! \copydoc IVoice::joinVoiceRoom()
        virtual void joinVoiceRoom(const ComUnit comUnit, const BlackMisc::Voice::CVoiceRoom &voiceRoom) override;

        //! \copydoc IVoice::leaveVoiceRoom()
        virtual void leaveVoiceRoom(const ComUnit comUnit) override;

        //! \copydoc IVoice::leaveAllVoiceRooms()
        virtual void leaveAllVoiceRooms() override;

        //! \copydoc IVoice::setRoomOutputVolume()
        virtual void setRoomOutputVolume(const ComUnit comUnit, const qint32 volumne) override;

        //! \copydoc IVoice::startTransmitting()
        virtual void startTransmitting(const ComUnit comUnit) override;

        //! \copydoc IVoice::stopTransmitting()
        virtual void stopTransmitting(const ComUnit comUnit) override;

        //! \copydoc IVoice::getComVoiceRoomsWithAudioStatus()
        virtual BlackMisc::Voice::CVoiceRoomList getComVoiceRoomsWithAudioStatus() const override;

        //! \copydoc IVoice::getComVoiceRooms()
        virtual BlackMisc::Voice::CVoiceRoomList getComVoiceRooms() const override
        {
            return this->m_voiceRooms;
        }

        //! \copydoc IVoice::getVoiceRoomCallsigns()
        virtual BlackMisc::Aviation::CCallsignList getVoiceRoomCallsigns(const ComUnit comUnit) const override;

        //! \copydoc IVoice::setInputDevice
        virtual void setInputDevice(const BlackMisc::Voice::CAudioDevice &device) override;

        //! \copydoc IVoice::setOutputDevice
        virtual void setOutputDevice(const BlackMisc::Voice::CAudioDevice &device) override;

        //! \copydoc IVoice::getCurrentInputDevice()
        virtual BlackMisc::Voice::CAudioDevice getCurrentInputDevice() const override;

        //! \copydoc IVoice::getCurrentOutputDevice()
        virtual BlackMisc::Voice::CAudioDevice getCurrentOutputDevice() const override;

        //! \copydoc IVoice::switchAudioOutput
        virtual void switchAudioOutput(const ComUnit comUnit, bool enable) override;

        //! \copydoc IVoice::isMuted
        virtual bool isMuted() const override
        {
            if (this->m_outputEnabled.isEmpty()) return false;
            bool enabled = this->m_outputEnabled[COM1] || this->m_outputEnabled[COM2];
            return !enabled;
        }

        /************************************************
         * NON API METHODS:
         * The following methods are not part of the
         * public API. They are needed for internal
         * workflow.
         * *********************************************/

        /*!
         * \brief Voice room index
         * \return
         */
        qint32 temporaryUserRoomIndex() const
        {
            return m_temporaryUserRoomIndex;
        }

        /*!
         * \brief Room status update, used in callback
         * \param comUnit
         * \param roomStatus
         */
        void changeRoomStatus(ComUnit comUnit, Cvatlib_Voice_Simple::roomStatusUpdate roomStatus);

    signals:

        /*!
         * \brief User joined or left
         * \param comUnit
         */
        void userJoinedLeft(const ComUnit comUnit);

    protected: // QObject overrides

        /*!
         * \brief Process voice lib
         */
        virtual void timerEvent(QTimerEvent *);

    private slots:
        // slots for Mic tests
        void onEndFindSquelch();
        void onEndMicTest();

        /*!
         * \brief User (identified by callsign) joined or left voice room
         */
        void onUserJoinedLeft(const ComUnit comUnit);

    private:

        // shimlib callbacks
        static void onRoomStatusUpdate(Cvatlib_Voice_Simple *obj, Cvatlib_Voice_Simple::roomStatusUpdate upd, qint32 roomIndex, void *cbVar);
        static void onRoomUserReceived(Cvatlib_Voice_Simple *obj, const char *name, void *cbVar);
        static void onInputHardwareDeviceReceived(Cvatlib_Voice_Simple *obj, const char *name, void *cbVar);
        static void onOutputHardwareDeviceReceived(Cvatlib_Voice_Simple *obj, const char *name, void *cbVar);

        BlackMisc::Voice::CVoiceRoom voiceRoomForUnit(const ComUnit comUnit) const;
        void setVoiceRoomForUnit(const IVoice::ComUnit comUnit, const BlackMisc::Voice::CVoiceRoom &voiceRoom);
        void addTemporaryCallsignForRoom(const ComUnit comUnit, const BlackMisc::Aviation::CCallsign &callsign);
        void removeUserFromRoom(const ComUnit comUnit, const QString &callsign);
        void exceptionDispatcher(const char *caller);
        void enableAudio(const ComUnit comUnit);
        void handlePushToTalk();

        /*!
         * \brief Deleter
         */
        struct Cvatlib_Voice_Simple_Deleter
        {
            /*!
             * \brief Cleanup
             * \param pointer
             */
            static inline void cleanup(Cvatlib_Voice_Simple *pointer)
            {
                pointer->Destroy();
            }
        };

#ifdef Q_OS_WIN

        /*!
         * \brief Keyboard PTT handling class
         */
        class CKeyboard
        {
        public:
            // Keyboard hook
            static HHOOK s_keyboardHook;
            static CVoiceVatlib *s_voice;
            static LRESULT CALLBACK keyboardProcedure(int nCode, WPARAM wParam, LPARAM lParam);

            /*!
             * \brief Constructor, keyboard handling
             */
            CKeyboard(CVoiceVatlib *vatlib)
            {
                CVoiceVatlib::CKeyboard::s_voice = vatlib;
                CVoiceVatlib::CKeyboard::s_keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, CVoiceVatlib::CKeyboard::keyboardProcedure, GetModuleHandle(NULL), 0);
            }

            /*!
             * Destructor
             */
            ~CKeyboard()
            {
                if (!CVoiceVatlib::CKeyboard::s_keyboardHook) return;
                UnhookWindowsHookEx(CVoiceVatlib::CKeyboard::s_keyboardHook);
                CVoiceVatlib::CKeyboard::s_keyboardHook = nullptr;
                CVoiceVatlib::CKeyboard::s_voice = nullptr;
            }
        };

#else

        /*!
         * \brief Keyboard PTT handling class
         */
        class CKeyboard
        {
        public:
            static CVoiceVatlib *s_voice;

            /*!
             * \brief Constructor, keyboard handling
             */
            CKeyboard(CVoiceVatlib *vatlib)
            {
                CVoiceVatlib::CKeyboard::s_voice = vatlib;
            }

            /*!
             * Destructor
             */
            ~CKeyboard()
            {
                // void
            }
        };

#endif

        QScopedPointer<Cvatlib_Voice_Simple, Cvatlib_Voice_Simple_Deleter> m_voice;
        QScopedPointer<QAudioOutput> m_audioOutput;
        BlackMisc::Aviation::CCallsign m_aircraftCallsign; /*!< own callsign to join voice rooms */
        BlackMisc::Voice::CVoiceRoomList m_voiceRooms;
        BlackMisc::Voice::CAudioDeviceList m_devices; /*!< in and output devices */
        BlackMisc::Voice::CAudioDevice m_currentOutputDevice;
        BlackMisc::Voice::CAudioDevice m_currentInputDevice;
        QScopedPointer<CKeyboard> m_keyboardPtt; /*!< handler for PTT */
        bool m_pushToTalk; /*!< flag, PTT pressed */
        float m_inputSquelch;
        Cvatlib_Voice_Simple::agc m_micTestResult;
        QMap <ComUnit, BlackMisc::Aviation::CCallsignList> m_voiceRoomCallsigns; /*!< voice room callsigns */
        BlackMisc::Aviation::CCallsignList m_temporaryVoiceRoomCallsigns; /*!< temp. storage of voice rooms during update */
        QMap<ComUnit, bool> m_outputEnabled; /*!< output enabled, basically a mute flag */

        // Need to keep the roomIndex?
        // KB: I would remove this approach, it is potentially unsafe
        //     Maybe just use 2 "wrapper" callbacks, which then set explicitly the voice room (it is only 2 methods)
        qint32 m_temporaryUserRoomIndex; /*!< temp. storage of voice room, in order to retrieve it in static callback */
        const static qint32 InvalidRoomIndex = -1; /*! marks invalid room */

    };

} // namespace

#endif // guard

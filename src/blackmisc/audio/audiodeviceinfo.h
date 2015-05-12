/* Copyright (C) 2013
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file

#ifndef BLACKMISC_AUDIO_AUDIODEVICE_H
#define BLACKMISC_AUDIO_AUDIODEVICE_H

#include "blackmisc/blackmiscexport.h"
#include "blackmisc/blackmiscfreefunctions.h"
#include "blackmisc/valueobject.h"
#include <QString>

namespace BlackMisc
{
    namespace Audio
    {
        /*!
         * Value object encapsulating information of a audio device.
         * If you want to safe this object, use the name instead of the index, since the index can change after
         * a restart.
         */
        class BLACKMISC_EXPORT CAudioDeviceInfo : public CValueObject<CAudioDeviceInfo>
        {
        public:
            //! Type
            enum DeviceType
            {
                InputDevice,
                OutputDevice,
                Unknown
            };

            /*!
             * Default constructor.
             * If m_deviceIndex is -1, default should be used. However on Windows this doesnt work. Needs
             * to be checked in Vatlib.
             */
            CAudioDeviceInfo();

            //! Constructor.
            CAudioDeviceInfo(DeviceType type, const int index, const QString &getName);

            //! Get the device index
            int getIndex() const { return m_deviceIndex; }

            //! Get the device name
            const QString &getName() const { return m_deviceName; }

            //! Host name
            const QString &getHostName() const { return m_hostName; }

            //! Type
            DeviceType getType() const { return m_type; }

            //! Valid audio device object?
            bool isValid() const { return m_deviceIndex >= -1 && !m_deviceName.isEmpty(); }

            //! Device index for default device
            static int defaultDeviceIndex() {return -1;}

            //! Invalid device index
            static int invalidDeviceIndex() {return -2;}

            //! Default output device
            static CAudioDeviceInfo getDefaultOutputDevice()
            {
                return CAudioDeviceInfo(OutputDevice, defaultDeviceIndex(), "default");
            }

            //! Default input device
            static CAudioDeviceInfo getDefaultInputDevice()
            {
                return CAudioDeviceInfo(InputDevice, defaultDeviceIndex(), "default");
            }

            //! \copydoc CValueObject::convertToQString
            QString convertToQString(bool i18n = false) const;

        private:
            BLACK_ENABLE_TUPLE_CONVERSION(CAudioDeviceInfo)
            //! Device type, @see CAudioDeviceInfo::DeviceType
            DeviceType m_type;
            /*!
             * deviceIndex is the number is the reference for the VVL. The device is selected by this index.
             * The managing class needs to take care, that indexes are valid.
             */
            int m_deviceIndex;
            //! Device name
            QString m_deviceName;
            //! We use a DBus based system. Hence an audio device can reside on a differen computers, this here is its name
            QString m_hostName;
        };
    } // namespace
} // namespace

BLACK_DECLARE_TUPLE_CONVERSION(BlackMisc::Audio::CAudioDeviceInfo, (o.m_type, o.m_deviceIndex, o.m_deviceName, o.m_hostName))
Q_DECLARE_METATYPE(BlackMisc::Audio::CAudioDeviceInfo)

#endif // guard

/* Copyright (C) 2018
 * swift Project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution. No part of swift project, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 */

#include "notificationsounds.h"
#include <QStringList>
#include <QString>

namespace BlackMisc
{
    namespace Audio
    {
        constexpr CNotificationSounds::Notification CNotificationSounds::AllNotifications;
        constexpr CNotificationSounds::Notification CNotificationSounds::DefaultNotifications;

        const QString &CNotificationSounds::flagToString(CNotificationSounds::NotificationFlag notification)
        {
            static const QString unknown("unknown");
            static const QString error("error");
            static const QString login("login");
            static const QString logoff("logoff");
            static const QString privateMsg("private msg.");
            static const QString supMsg("supervisor msg.");
            static const QString mentioned("cs mentioned");
            static const QString joined("room joined");
            static const QString left("room left");
            static const QString pttUp("PTT click up");
            static const QString pttDown("PTT click down");
            static const QString pttBlocked("PTT blocked");
            static const QString load("load sounds");
            static const QString noaudiotx("No audio tx");
            static const QString afvclick("AFC click");
            static const QString afvblock("AFV blocked");

            switch (notification)
            {
            case NotificationError:  return error;
            case NotificationLogin:  return login;
            case NotificationLogoff: return logoff;
            case NotificationTextMessagePrivate:    return privateMsg;
            case NotificationTextMessageSupervisor: return supMsg;
            case NotificationTextCallsignMentioned: return mentioned;
            case NotificationNoAudioTransmission:   return noaudiotx;
            case PTTClickKeyDown:   return pttDown;
            case PTTClickKeyUp:     return pttUp;
            case PTTBlocked:        return pttBlocked;
            case AFVClicked:        return afvclick;
            case AFVBlocked:        return afvblock;
            default: break;
            }
            return unknown;
        }

        const QString CNotificationSounds::toString(Notification notification)
        {
            QStringList n;
            if (notification.testFlag(NotificationError))  n << flagToString(NotificationError);
            if (notification.testFlag(NotificationLogin))  n << flagToString(NotificationLogin);
            if (notification.testFlag(NotificationLogoff)) n << flagToString(NotificationLogoff);
            if (notification.testFlag(NotificationTextMessagePrivate))    n << flagToString(NotificationTextMessagePrivate);
            if (notification.testFlag(NotificationTextMessageSupervisor)) n << flagToString(NotificationTextMessageSupervisor);
            if (notification.testFlag(NotificationTextCallsignMentioned)) n << flagToString(NotificationTextCallsignMentioned);
            if (notification.testFlag(NotificationNoAudioTransmission))   n << flagToString(NotificationNoAudioTransmission);
            if (notification.testFlag(PTTClickKeyUp))   n << flagToString(PTTClickKeyUp);
            if (notification.testFlag(PTTClickKeyDown)) n << flagToString(PTTClickKeyDown);
            if (notification.testFlag(PTTBlocked))      n << flagToString(PTTBlocked);

            if (notification.testFlag(AFVClicked)) n << flagToString(AFVClicked);
            if (notification.testFlag(AFVBlocked)) n << flagToString(AFVBlocked);
            return n.join(", ");
        }
    } // ns
} // ns

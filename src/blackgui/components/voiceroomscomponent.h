/* Copyright (C) 2013
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file

#ifndef BLACKGUI_VOICEROOMSCOMPONENT_H
#define BLACKGUI_VOICEROOMSCOMPONENT_H

#include "blackgui/blackguiexport.h"
#include "blackmisc/audio/voiceroomlist.h"

#include <QFrame>
#include <QObject>
#include <QScopedPointer>

class QWidget;

namespace Ui { class CVoiceRoomsComponent; }

namespace BlackGui
{
    namespace Components
    {
        //! Displays the voice rooms
        class BLACKGUI_EXPORT CVoiceRoomsComponent :
            public QFrame
        {
            Q_OBJECT

        public:
            //! Constructor
            explicit CVoiceRoomsComponent(QWidget *parent = nullptr);

            //! Destructor
            ~CVoiceRoomsComponent();

        private slots:
            //! Override for voice was changed
            void ps_onVoiceRoomOverrideChanged(bool checked);

            //! Return pressed
            void ps_onVoiceRoomUrlsReturnPressed();

            //! Set the voice room url fields (checkboxes, line edits)
            void ps_updateAudioVoiceRoomsFromContext(const BlackMisc::Audio::CVoiceRoomList &selectedVoiceRooms, bool connected);

            //! Update voice room views
            void ps_updateVoiceRoomMembers();

        private:
            //! Set the URL fields
            void setVoiceRoomUrlFieldsReadOnlyState();

            QScopedPointer<Ui::CVoiceRoomsComponent> ui;
        };

    } // namespace
} // namespace

#endif // guard

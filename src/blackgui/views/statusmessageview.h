/* Copyright (C) 2013
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file

#ifndef BLACKGUI_STATUSMESSAGEVIEW_H
#define BLACKGUI_STATUSMESSAGEVIEW_H

#include "blackgui/blackguiexport.h"
#include "blackgui/models/statusmessagelistmodel.h"
#include "blackgui/views/viewbase.h"
#include "blackmisc/statusmessage.h"
#include "blackmisc/statusmessagelist.h"

#include <QObject>

class QWidget;

namespace BlackGui
{
    namespace Views
    {
        //! Status message view
        class BLACKGUI_EXPORT CStatusMessageView :
            public CViewBase<Models::CStatusMessageListModel, BlackMisc::CStatusMessageList, BlackMisc::CStatusMessage>
        {
            Q_OBJECT

        public:
            //! Constructor
            explicit CStatusMessageView(QWidget *parent = nullptr);

            //! Set mode
            void setMode(BlackGui::Models::CStatusMessageListModel::Mode mode);
        };
    } // ns
} // ns
#endif // guard

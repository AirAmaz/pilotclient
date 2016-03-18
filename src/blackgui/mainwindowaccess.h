/* Copyright (C) 2016
 * swift project Community / Contributors
 *
 * This file is part of swift Project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#ifndef BLACKGUI_MAINWINDOWACCESS_H
#define BLACKGUI_MAINWINDOWACCESS_H

#include "blackgui/blackguiexport.h"
#include "blackmisc/statusmessage.h"
#include <QStatusBar>

namespace BlackGui
{
    class CManagedStatusBar;
    class COverlayMessagesFrame;
    namespace Components { class CLogComponent; }

    /*!
     * Direct acccess to main window`s status bar, info bar and such
     */
    class BLACKGUI_EXPORT IMainWindowAccess
    {
    public:
        //! Destructor
        virtual ~IMainWindowAccess();

        //! Display in console
        virtual bool displayTextInConsole(const QString &message);

        //! Display in status bar
        virtual bool displayInStatusBar(const BlackMisc::CStatusMessage &message);

        //! Display in overlay window
        virtual bool displayInOverlayWindow(const BlackMisc::CStatusMessage &message);

    protected:
        Components::CLogComponent *m_mwaLogComponent = nullptr; //!< the log component if any
        CManagedStatusBar         *m_mwaStatusBar = nullptr;    //!< status bar if any
        COverlayMessagesFrame     *m_mwaOverlayFrame = nullptr; //!< overlay messages if any
    };
} // ns

#endif // guard

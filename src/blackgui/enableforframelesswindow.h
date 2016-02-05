/* Copyright (C) 2014
 * swift project Community / Contributors
 *
 * This file is part of swift Project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file

#ifndef BLACKGUI_ENABLEFORFRAMLESSWINDOW_H
#define BLACKGUI_ENABLEFORFRAMLESSWINDOW_H

#include "blackgui/blackguiexport.h"
#include <QWidget>
#include <QStatusBar>
#include <QMouseEvent>
#include <QPushButton>
#include <QHBoxLayout>
#include <QMenuBar>
#include <QSizeGrip>

namespace BlackGui
{

    //! Main window which can be frameless
    //! \details QMainWindows cannot be promoted. Hence a derived class does not work properly here.
    //!          Furthermore frameless functionality is also required for CDockWidgets as well.
    class BLACKGUI_EXPORT CEnableForFramelessWindow
    {
    public:
        //! Window modes
        enum WindowMode
        {
            WindowNormal,
            WindowFrameless,
            WindowTool
        };

        //! Constructor
        //! \param mode                    window mode as defined in WindowMode
        //! \param isMainApplicationWindow is this the main (there should be only one) application window
        //! \param framelessPropertyname   qss property indication frameless
        //! \param correspondingWidget     the widget representing the window
        //!
        CEnableForFramelessWindow(WindowMode mode, bool isMainApplicationWindow, const char *framelessPropertyname, QWidget *correspondingWidget);

        //! Window mode
        void setMode(WindowMode mode);

        //! Framless
        virtual void setFrameless(bool frameless);

        //! Frameless?
        bool isFrameless() const { return this->m_windowMode == WindowFrameless; }

        //! Is main application, explicitly set
        bool isMainApplicationWindow() const { return m_mainApplicationWindow; }

        //! Always on top?
        void alwaysOnTop(bool onTop);

        //! Corresponding QMainWindow
        QWidget *getWidget() const { return m_widget; }

        //! String to window mode
        static WindowMode stringToWindowMode(const QString &s);

        //! String to window mode
        static QString windowModeToString(WindowMode m);

    protected:
        QPoint       m_framelessDragPosition;             //!< position, if moving is handled with frameless window */
        QPushButton *m_framelessCloseButton = nullptr;    //!< close button
        WindowMode   m_windowMode = WindowNormal;         //!< Window mode, \sa WindowMode
        WindowMode   m_originalWindowMode = WindowNormal; //!< mode when initialized
        bool         m_mainApplicationWindow = false;     //!< is the main application window (only 1)
        QWidget     *m_widget = nullptr;                  //!< corresponding widget or dock widget
        QSizeGrip   *m_framelessSizeGrip = nullptr;       //!< size grip object
        QByteArray   m_framelessPropertyName;             //!< property name for frameless widgets

        //! Can be used as notification if window mode changes
        virtual void windowFlagsChanged();

        //! Resize grip handle
        void addFramelessSizeGripToStatusBar(QStatusBar *statusBar);

        //! Resize grip handle
        void hideFramelessSizeGripInStatusBar();

        //! Attributes
        void setWindowAttributes(WindowMode mode);

        //! Set dynamic properties such as frameless
        void setDynamicProperties(bool frameless);

        //! Close button for frameless windows
        QHBoxLayout *addFramelessCloseButton(QMenuBar *menuBar);

        //! Remove tool and add desktop window
        void toolToNormalWindow();

        //! Remove desktop and add tool window
        void normalToToolWindow();

        //! Tool window
        bool isToolWindow() const;

        //! Mouse press, required for frameless window
        bool handleMousePressEvent(QMouseEvent *event);

        //! Mouse moving, required for frameless window
        bool handleMouseMoveEvent(QMouseEvent *event);

        //! Mouse window change event
        bool handleChangeEvent(QEvent *event);

        //! Check mode and then show minimized
        void showMinimizedModeChecked();

        //! Check mode and then show normal
        void showNormalModeChecked();

        //! Translate mode
        static Qt::WindowFlags modeToWindowFlags(WindowMode mode);
    };
} // namespace

#endif // guard

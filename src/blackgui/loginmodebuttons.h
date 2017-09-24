/* Copyright (C) 2014
 * swift project Community / Contributors
 *
 * This file is part of swift Project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file

#ifndef BLACKGUI_LOGINMODEBUTTONS_H
#define BLACKGUI_LOGINMODEBUTTONS_H

#include "blackcore/network.h"
#include "blackgui/blackguiexport.h"
#include <QGroupBox>
#include <QObject>
#include <QScopedPointer>

class QWidget;

namespace Ui { class CLoginModeButtons; }
namespace BlackGui
{
    //! Display login modes (normal, stealth, ...)
    class BLACKGUI_EXPORT CLoginModeButtons : public QGroupBox
    {
        Q_OBJECT

    public:
        //! Constructor
        explicit CLoginModeButtons(QWidget *parent = nullptr);

        //! Destructor
        virtual ~CLoginModeButtons();

        //! Get login mode, \sa BlackCore::INetwork::LoginMode
        BlackCore::INetwork::LoginMode getLoginMode() const;

        //! Set login mode
        void setLoginMode(BlackCore::INetwork::LoginMode mode);

        //! Set to read only
        void setReadOnly(bool readonly);

    private:
        void configureLoginModes();

        QScopedPointer<Ui::CLoginModeButtons> ui;
    };
} // ns

#endif // guard

/* Copyright (C) 2015
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file

#ifndef BLACKGUI_PLUGINCONFIGWINDOW_H
#define BLACKGUI_PLUGINCONFIGWINDOW_H

#include "blackgui/blackguiexport.h"
#include <QWidget>

namespace BlackGui
{
    /**
     * Base class for plugin config window.
     */
    class BLACKGUI_EXPORT CPluginConfigWindow : public QWidget
    {
        Q_OBJECT
    public:
        //! No parent
        explicit CPluginConfigWindow(QWidget *parent);

    protected:
        virtual void showEvent(QShowEvent *event) override;

    };
}

#endif // CPLUGINCONFIGWINDOW_H

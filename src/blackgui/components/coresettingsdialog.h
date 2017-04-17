/* Copyright (C) 2017
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file

#ifndef BLACKGUI_COMPONENTS_CORESETTINGSDIALOG_H
#define BLACKGUI_COMPONENTS_CORESETTINGSDIALOG_H

#include "blackgui/blackguiexport.h"
#include <QDialog>
#include <QScopedPointer>

namespace Ui { class CCoreSettingsDialog; }
namespace BlackGui
{
    namespace Components
    {
        /**
         * Core / swift pilot client settings as Dialog
         */
        class BLACKGUI_EXPORT CCoreSettingsDialog : public QDialog
        {
            Q_OBJECT

        public:
            //! Constructor
            explicit CCoreSettingsDialog(QWidget *parent = nullptr);

            //! Destructor
            virtual ~CCoreSettingsDialog();

        private:
            QScopedPointer<Ui::CCoreSettingsDialog> ui;
        };
    } // ns
} // ns

#endif // guard

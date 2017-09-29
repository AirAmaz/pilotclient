/* Copyright (C) 2017
 * swift project Community / Contributors
 *
 * This file is part of swift Project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file

#ifndef BLACKGUI_COMPONENTS_DOWNLOADANDINSTALLDIALOG_H
#define BLACKGUI_COMPONENTS_DOWNLOADANDINSTALLDIALOG_H

#include "blackgui/settings/updatenotification.h"
#include "blackgui/blackguiexport.h"
#include <QDialog>

namespace Ui { class CDownloadAndInstallDialog; }
namespace BlackGui
{
    namespace Components
    {
        /**
         * Download and install swift
         */
        class BLACKGUI_EXPORT CDownloadAndInstallDialog : public QDialog
        {
            Q_OBJECT

        public:
            //! Constructor
            explicit CDownloadAndInstallDialog(QWidget *parent = nullptr);

            //! Destructor
            virtual ~CDownloadAndInstallDialog();

            //! A new version existing?
            bool isNewVersionAvailable() const;

            //! \copydoc QDialog::exec
            virtual int exec() override;

            //! \copydoc QObject::event
            virtual bool event(QEvent *event) override;

        private:
            QScopedPointer<Ui::CDownloadAndInstallDialog> ui;
            BlackMisc::CSetting<BlackGui::Settings::TUpdateNotificationSettings> m_setting { this }; //!< show again?

            //! Toggled checkbox
            void onDontShowAgain(bool dontShowAgain);

            //! Selection in distribution component changed
            void selectionChanged();

            //! Request context help
            void requestHelp();
        };
    } // ns
} // ns

#endif // guard

/* Copyright (C) 2017
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file

#ifndef BLACKGUI_COMPONENTS_SETTINGSFONTDIALOG_H
#define BLACKGUI_COMPONENTS_SETTINGSFONTDIALOG_H

#include <QDialog>

namespace Ui { class CSettingsFontDialog; }
namespace BlackGui
{
    namespace Components
    {
        /*!
         * Dialog to edit fonts
         * \see BlackMisc::Components::CSettingsFontComponent
         */
        class CSettingsFontDialog : public QDialog
        {
            Q_OBJECT

        public:
            //! Constructor
            explicit CSettingsFontDialog(QWidget *parent = nullptr);

            //! Destructor
            virtual ~CSettingsFontDialog();

            //! Get stylesheet
            const QString &getQss() const;

            //! Set the current font
            void setCurrentFont(const QFont &font);

        private:
            QScopedPointer<Ui::CSettingsFontDialog> ui;
        };
    } // ns
} // ns
#endif // guard

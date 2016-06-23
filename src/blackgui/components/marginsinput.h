/* Copyright (C) 2016
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file

#ifndef BLACKGUI_COMPONENTS_MARGINSINPUT_H
#define BLACKGUI_COMPONENTS_MARGINSINPUT_H

#include <QFrame>
#include <QMargins>
#include <QScopedPointer>

namespace Ui { class CMarginsInput; }

namespace BlackGui
{
    namespace Components
    {
        /*!
         * Widget alows to enter margins
         */
        class CMarginsInput : public QFrame
        {
            Q_OBJECT

        public:
            //! Constructor
            explicit CMarginsInput(QWidget *parent = nullptr);

            //! Destructor
            ~CMarginsInput();

            //! Set margins
            void setMargins(const QMargins &margins);

            //! Current values of margins
            QMargins getMargins() const;

        protected:
            //! \copydoc QFrame::paintEvent
            virtual void paintEvent(QPaintEvent *event) override;

        signals:
            //! Margins changed
            void changedMargins(const QMargins &margins);

        private slots:
            //! Ok
            void ps_Confirmed();

        private:
            QScopedPointer<Ui::CMarginsInput> ui;
        };
    } // ns
} // ns

#endif // guard

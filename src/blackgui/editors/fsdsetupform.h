/* Copyright (C) 2013
 * swift project Community / Contributors
 *
 * This file is part of swift Project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file

#ifndef BLACKGUI_EDITORS_FSDSETUPCOMPONENT_H
#define BLACKGUI_EDITORS_FSDSETUPCOMPONENT_H

#include "blackmisc/network/fsdsetup.h"
#include "form.h"
#include <QFrame>
#include <QScopedPointer>

namespace Ui { class CFsdSetupForm; }
namespace BlackGui
{
    namespace Editors
    {
        /*!
         * Setup for FSD
         */
        class CFsdSetupForm : public CForm
        {
            Q_OBJECT

        public:
            //! Constructor
            explicit CFsdSetupForm(QWidget *parent = nullptr);

            //! Constructor
            virtual ~CFsdSetupForm();

            //! FSD setup from GUI
            BlackMisc::Network::CFsdSetup getValue() const;

            //! FSD setup when disabled
            const BlackMisc::Network::CFsdSetup &getDisabledValue() const;

            //! Set to GUI
            void setValue(const BlackMisc::Network::CFsdSetup &setup);

            //! Enabled?
            bool isFsdSetupEnabled() const;

            //! Set enabled / disabled
            void setFsdSetupEnabled(bool enabled);

            //! Show the enable info
            void showEnableInfo(bool visible);

            //! Set default values
            void resetToDefaultValues();

            //! \name Form class implementations
            //! @{
            virtual void setReadOnly(bool readonly) override;
            virtual BlackMisc::CStatusMessageList validate(bool nested = false) const override;
            //! @}

        private:
            //! Enable / disable
            void enabledToggled(bool enabled);

            QScopedPointer<Ui::CFsdSetupForm> ui;
        };
    } // ns
} // ns

#endif // guard

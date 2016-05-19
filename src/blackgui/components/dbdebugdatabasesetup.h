/* Copyright (C) 2015
 * swift project Community / Contributors
 *
 * This file is part of swift Project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file

#ifndef BLACKGUI_COMPONENTS_DBDEBUGDATABASESETUP_H
#define BLACKGUI_COMPONENTS_DBDEBUGDATABASESETUP_H

#include "blackcore/data/globalsetup.h"
#include "blackgui/blackguiexport.h"
#include "blackmisc/datacache.h"

#include <QFrame>
#include <QObject>
#include <QScopedPointer>

class QWidget;

namespace Ui { class CDbDebugDatabaseSetup; }

namespace BlackGui
{
    namespace Components
    {
        /*!
         * Debug settings for DB (only to be used as developer)
         * \remarks Disabled when not runnig in dev.environment
         */
        class BLACKGUI_EXPORT CDbDebugDatabaseSetup : public QFrame
        {
            Q_OBJECT

        public:
            //! Constructor
            explicit CDbDebugDatabaseSetup(QWidget *parent = nullptr);

            //! Dstructor
            ~CDbDebugDatabaseSetup();

        private slots:
            //! Changed the debug checkbox
            void ps_debugChanged(bool set);

        private:
            QScopedPointer<Ui::CDbDebugDatabaseSetup> ui;
            BlackMisc::CData<BlackCore::Data::GlobalSetup> m_setup {this};   //!< data cache
        };
    } // ns
} // ns

#endif // guard

/* Copyright (C) 2013
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file

#ifndef BLACKGUI_NAMEVARIANTPAIRVIEW_H
#define BLACKGUI_NAMEVARIANTPAIRVIEW_H

#include "blackgui/blackguiexport.h"
#include "viewbase.h"
#include "../models/namevariantpairlistmodel.h"

namespace BlackGui
{
    namespace Views
    {
        //! User view
        class BLACKGUI_EXPORT CNameVariantPairView : public CViewBase<Models::CNameVariantPairModel, BlackMisc::CNameVariantPairList, BlackMisc::CNameVariantPair>
        {

        public:
            //! Constructor
            explicit CNameVariantPairView(QWidget *parent = nullptr);

            //! Icon mode
            void setIconMode(bool withIcon);

            //! Update or add value, QVariant version
            bool addOrUpdateByName(const QString &name, const BlackMisc::CVariant &value, const BlackMisc::CIcon &icon = BlackMisc::CIcon(), bool performResizing = true, bool skipEqualValues = true);

            //! Remove by name
            void removeByName(const QString &name, bool performResizing = true);

            //! Contains name
            bool containsName(const QString &name);

        };
    }
}
#endif // guard

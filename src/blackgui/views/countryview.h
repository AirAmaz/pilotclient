/* Copyright (C) 2015
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file

#ifndef BLACKGUI_COUNTRYVIEW_H
#define BLACKGUI_COUNTRYVIEW_H

#include "blackgui/blackguiexport.h"
#include "blackgui/models/countrylistmodel.h"
#include "blackgui/views/viewbase.h"
#include "blackmisc/countrylist.h"

class QWidget;

namespace BlackMisc { class CCountry; }

namespace BlackGui
{
    namespace Views
    {
        //! Distributors
        class BLACKGUI_EXPORT CCountryView : public CViewBase<Models::CCountryListModel, BlackMisc::CCountryList, BlackMisc::CCountry>
        {
        public:
            //! Constructor
            explicit CCountryView(QWidget *parent = nullptr);
        };
    }
}
#endif // guard

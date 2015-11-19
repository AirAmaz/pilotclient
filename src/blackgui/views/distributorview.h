/* Copyright (C) 2015
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file

#ifndef BLACKGUI_DISTRIBUTORVIEW_H
#define BLACKGUI_DISTRIBUTORVIEW_H

#include "blackgui/blackguiexport.h"
#include "viewbase.h"
#include "../models/distributorlistmodel.h"

namespace BlackGui
{
    namespace Views
    {
        //! Distributors
        class BLACKGUI_EXPORT CDistributorView : public CViewBase<Models::CDistributorListModel, BlackMisc::Simulation::CDistributorList, BlackMisc::Simulation::CDistributor>
        {

        public:
            //! Constructor
            explicit CDistributorView(QWidget *parent = nullptr);
        };
    }
}
#endif // guard

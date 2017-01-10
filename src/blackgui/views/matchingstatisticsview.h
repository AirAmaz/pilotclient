/* Copyright (C) 2017
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file

#ifndef BLACKGUI_VIEWS_MATCHINGSTATISTICSVIEW_H
#define BLACKGUI_VIEWS_MATCHINGSTATISTICSVIEW_H

#include "blackgui/blackguiexport.h"
#include "blackgui/models/matchingstatisticsmodel.h"
#include "blackgui/views/viewbase.h"
#include "blackmisc/network/serverlist.h"

class QWidget;

namespace BlackGui
{
    namespace Views
    {
        //! Matching statistics
        class BLACKGUI_EXPORT CMatchingStatisticsView : public CViewBase<Models::CMatchingStatisticsModel, BlackMisc::Simulation::CMatchingStatistics, BlackMisc::Simulation::CMatchingStatisticsEntry>
        {
        public:
            //! Constructor
            explicit CMatchingStatisticsView(QWidget *parent = nullptr);

            //! Set vie mode
            void setStatisticsModelMode(Models::CMatchingStatisticsModel::MatchingStatisticsMode mode);
        };
    }
} // ns
#endif // guard

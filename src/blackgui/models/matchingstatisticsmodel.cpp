/* Copyright (C) 2017
 * swift Project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "matchingstatisticsmodel.h"
#include "blackgui/models/columns.h"
#include <QtGlobal>

using namespace BlackMisc::Simulation;

namespace BlackGui
{
    namespace Models
    {
        CMatchingStatisticsModel::CMatchingStatisticsModel(MatchingStatisticsMode mode, QObject *parent) :
            CListModelBase("MatchingStatisticsModel", parent)
        {
            this->setMode(mode);

            // force strings for translation in resource files
            (void)QT_TRANSLATE_NOOP("MatchingStatisticsModel", "session");
            (void)QT_TRANSLATE_NOOP("MatchingStatisticsModel", "model set");
            (void)QT_TRANSLATE_NOOP("MatchingStatisticsModel", "combination");
            (void)QT_TRANSLATE_NOOP("MatchingStatisticsModel", "type");
            (void)QT_TRANSLATE_NOOP("MatchingStatisticsModel", "aircraft");
            (void)QT_TRANSLATE_NOOP("MatchingStatisticsModel", "airline");
        }

        void CMatchingStatisticsModel::setMode(CMatchingStatisticsModel::MatchingStatisticsMode mode)
        {
            if (this->m_mode == mode) { return; }
            this->m_mode = mode;
            this->m_columns.clear();
            switch (mode)
            {
            case ForMultiSessions:
                this->m_columns.addColumn(CColumn::standardString("session", CMatchingStatisticsEntry::IndexSessionId));
                this->m_columns.addColumn(CColumn::standardString("model set", CMatchingStatisticsEntry::IndexModelSetId));
            // fall thru
            case ForSingleSession:
                this->m_columns.addColumn(CColumn("type", CMatchingStatisticsEntry::IndexEntryTypeAsIcon));
                this->m_columns.addColumn(CColumn::standardString("aircraft", CMatchingStatisticsEntry::IndexAircraftDesignator));
                this->m_columns.addColumn(CColumn::standardString("airline", CMatchingStatisticsEntry::IndexAirlineDesignator));
                this->m_columns.addColumn(CColumn::standardString("description", CMatchingStatisticsEntry::IndexDescription));
                this->m_columns.addColumn(CColumn::standardInteger("#", "count", CMatchingStatisticsEntry::IndexCount));
                break;
            default:
                break;
            }
            this->setSortColumnByPropertyIndex(CMatchingStatisticsEntry::IndexAircraftDesignator);
        }
    } // namespace
} // namespace

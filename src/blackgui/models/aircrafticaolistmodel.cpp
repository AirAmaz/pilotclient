/* Copyright (C) 2015
 * Swift Project Community / Contributors
 *
 * This file is part of swift Project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "aircrafticaolistmodel.h"
#include <QMetaProperty>
#include <QBrush>

using namespace BlackMisc::Aviation;

namespace BlackGui
{
    namespace Models
    {
        CAircraftIcaoCodeListModel::CAircraftIcaoCodeListModel(QObject *parent) :
            CListModelDbObjects("AircraftIcaoListModel", parent)
        {
            this->m_columns.addColumn(CColumn::standardString("id", CAircraftIcaoCode::IndexDbIntegerKey, CDefaultFormatter::alignRightVCenter()));
            this->m_columns.addColumn(CColumn::standardString("ICAO", CAircraftIcaoCode::IndexAircraftDesignator));
            this->m_columns.addColumn(CColumn::standardString("IATA", CAircraftIcaoCode::IndexIataCode));
            this->m_columns.addColumn(CColumn::standardValueObject("rank", CAircraftIcaoCode::IndexRank, CDefaultFormatter::alignRightVCenter()));
            this->m_columns.addColumn(CColumn::standardString("manufacturer", CAircraftIcaoCode::IndexManufacturer));
            this->m_columns.addColumn(CColumn::standardString("family", CAircraftIcaoCode::IndexFamily));
            this->m_columns.addColumn(CColumn::standardString("desc.", "description", CAircraftIcaoCode::IndexModelDescription));
            this->m_columns.addColumn(CColumn::standardString("code", "combined code", CAircraftIcaoCode::IndexCombinedAircraftType));
            this->m_columns.addColumn(CColumn::standardString("WTC", "wake turbulence category", CAircraftIcaoCode::IndexWtc));
            this->m_columns.addColumn(CColumn("mil.", "military", CAircraftIcaoCode::IndexIsMilitary, new CBoolIconFormatter("military", "civil")));
            this->m_columns.addColumn(CColumn("VTOL", "vertical take-off and landing", CAircraftIcaoCode::IndexIsVtol, new CBoolIconFormatter("VTOL", "non VTOL")));
            this->m_columns.addColumn(CColumn("leg.", "legacy", CAircraftIcaoCode::IndexIsLegacy, new CBoolIconFormatter("legacy", "operating")));
            this->m_columns.addColumn(CColumn("real.", "real world aircraft", CAircraftIcaoCode::IndexIsRealworld, new CBoolIconFormatter("real", "non real")));
            this->m_columns.addColumn(CColumn::standardString("changed", CAircraftIcaoCode::IndexUtcTimestampFormattedYmdhms));

            // default sort order
            this->setSortColumnByPropertyIndex(CAircraftIcaoCode::IndexAircraftDesignator);
            this->m_sortOrder = Qt::AscendingOrder;

            // force strings for translation in resource files
            (void)QT_TRANSLATE_NOOP("ModelAircraftIcaoList", "ICAO");
            (void)QT_TRANSLATE_NOOP("ModelAircraftIcaoList", "distance");
            (void)QT_TRANSLATE_NOOP("ModelAircraftIcaoList", "name");
            (void)QT_TRANSLATE_NOOP("ModelAircraftIcaoList", "elevation");
            (void)QT_TRANSLATE_NOOP("ModelAircraftIcaoList", "bearing");
        }
    } // ns
} // ns

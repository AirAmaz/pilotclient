/* Copyright (C) 2013
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file

#ifndef BLACKGUI_MODELS_ATCLISTMODEL_H
#define BLACKGUI_MODELS_ATCLISTMODEL_H

#include "blackgui/blackguiexport.h"
#include "blackgui/models/listmodelcallsignobjects.h"
#include "blackmisc/aviation/atcstation.h"
#include "blackmisc/aviation/atcstationlist.h"

#include <QObject>

class QStandardItemModel;

namespace BlackGui
{
    namespace Models
    {
        //! ATC list model
        class BLACKGUI_EXPORT CAtcStationListModel : public CListModelCallsignObjects<BlackMisc::Aviation::CAtcStation, BlackMisc::Aviation::CAtcStationList, true>
        {
        public:
            //! What kind of stations
            enum AtcStationMode
            {
                NotSet,
                StationsBooked,
                StationsOnline
            };

            //! Constructor
            explicit CAtcStationListModel(AtcStationMode stationMode, QObject *parent = nullptr);

            //! Destructor
            virtual ~CAtcStationListModel() {}

            //! Set station mode
            void setStationMode(AtcStationMode stationMode);

            //! Station mode
            AtcStationMode getStationMode() const { return this->m_stationMode; }

            //! A group by type (TWR, APP, ...) model
            QStandardItemModel *toAtcGroupModel() const;

        public slots:
            //! Used to quickly update single station (better response for the user)
            void changedAtcStationConnectionStatus(const BlackMisc::Aviation::CAtcStation &station, bool added);

        private:
            AtcStationMode m_stationMode = NotSet;
        };
    }
} // ns
#endif // guard

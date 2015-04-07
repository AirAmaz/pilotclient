/* Copyright (C) 2013
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file

#ifndef BLACKGUI_AIRPORTLISTMODEL_H
#define BLACKGUI_AIRPORTLISTMODEL_H

#include <QAbstractItemModel>
#include "blackmisc/aviation/airportlist.h"
#include "blackgui/models/listmodelbase.h"

namespace BlackGui
{
    namespace Models
    {

        /*!
         * Airport list model
         */
        class CAirportListModel : public CListModelBase<BlackMisc::Aviation::CAirport, BlackMisc::Aviation::CAirportList>
        {

        public:

            //! Constructor
            explicit CAirportListModel(QObject *parent = nullptr);

            //! Destructor
            virtual ~CAirportListModel() {}
        };
    }
}
#endif // guard

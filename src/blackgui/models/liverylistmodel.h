/* Copyright (C) 2015
 * swift Project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file

#ifndef BLACKGUI_LIVERYLISTMODEL_H
#define BLACKGUI_LIVERYLISTMODEL_H

#include "blackgui/blackguiexport.h"
#include "blackmisc/aviation/liverylist.h"
#include "blackgui/models/listmodeldbobjects.h"
#include <QAbstractItemModel>

namespace BlackGui
{
    namespace Models
    {
        //! Distributor list model
        class BLACKGUI_EXPORT CLiveryListModel :
            public CListModelDbObjects<BlackMisc::Aviation::CLivery, BlackMisc::Aviation::CLiveryList, int, true>
        {
        public:
            //! Constructor
            explicit CLiveryListModel(QObject *parent = nullptr);

            //! Destructor
            virtual ~CLiveryListModel() {}
        };
    } // ns
} // ns

#endif // guard

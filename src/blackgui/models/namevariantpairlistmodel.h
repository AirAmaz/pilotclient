/* Copyright (C) 2013
 * swift Project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file

#ifndef BLACKGUI_NAMEVARIANTLISTMODEL_H
#define BLACKGUI_NAMEVARIANTLISTMODEL_H

#include "blackgui/blackguiexport.h"
#include "blackmisc/namevariantpairlist.h"
#include "blackgui/models/listmodelbase.h"
#include <QAbstractItemModel>
#include <QMap>

namespace BlackGui
{
    namespace Models
    {

        //! Simple model displaying name / variant values
        class BLACKGUI_EXPORT CNameVariantPairModel : public CListModelBase<BlackMisc::CNameVariantPair, BlackMisc::CNameVariantPairList>
        {

        public:

            //! Constructor
            explicit CNameVariantPairModel(bool withIcon, QObject *parent = nullptr);

            //! Destructor
            virtual ~CNameVariantPairModel() {}

            //! Icon on / off
            void setIconMode(bool withIcon);

            //! Remove by given name
            void removeByName(const QString &name);

            //! Contains name already?
            bool containsName(const QString &name) const;

            //! Contains name / value?
            bool containsNameValue(const QString &name, const BlackMisc::CVariant &value) const;

            //! Add our update a value
            bool addOrUpdateByName(const QString &name, const BlackMisc::CVariant &value, const BlackMisc::CIcon &icon, bool skipEqualValues);

            //! Current row index of given name
            int getRowIndexForName(const QString &name) const;

        };
    }
}
#endif // guard

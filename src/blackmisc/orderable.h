/* Copyright (C) 2015
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file

#ifndef BLACKMISC_ORDERABLE_H
#define BLACKMISC_ORDERABLE_H

#include "blackmiscexport.h"
#include "propertyindex.h"

namespace BlackMisc
{
    //! Entity with timestamp
    class BLACKMISC_EXPORT IOrderable
    {
    public:
        //! Properties by index
        enum ColumnIndex
        {
            IndexOrder = BlackMisc::CPropertyIndex::GlobalIndexIOrderable,
            IndexOrderString
        };

        //! Order
        int getOrder() const { return m_order; }

        //! Order as string
        QString getOrderAsString() const;

        //! Set order
        void setOrder(int order) { m_order = order; }

        //! Valid order set?
        bool hasValidOrder() const;

        //! Can given index be handled
        static bool canHandleIndex(const BlackMisc::CPropertyIndex &index);

    protected:
        //! Constructor
        IOrderable();

        //! Constructor
        IOrderable(int order);

        //! \copydoc BlackMisc::Mixin::Index::propertyByIndex
        CVariant propertyByIndex(const BlackMisc::CPropertyIndex &index) const;

        //! \copydoc BlackMisc::Mixin::Index::setPropertyByIndex
        void setPropertyByIndex(const CVariant &variant, const BlackMisc::CPropertyIndex &index);

        //! Compare for index
        int comparePropertyByIndex(const IOrderable &compareValue, const CPropertyIndex &index) const;

        int m_order = -1; //!< order number
    };
} // namespace

Q_DECLARE_INTERFACE(BlackMisc::IOrderable, "org.swift-project.blackmisc.iorderable")

#endif // guard

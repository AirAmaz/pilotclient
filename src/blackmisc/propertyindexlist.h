/* Copyright (C) 2013
 * swift Project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file

#ifndef BLACKMISC_PROPERTYINDEXLIST_H
#define BLACKMISC_PROPERTYINDEXLIST_H

#include "blackmisc/blackmiscexport.h"
#include "blackmisc/collection.h"
#include "blackmisc/propertyindex.h"
#include "blackmisc/sequence.h"
#include "blackmisc/variant.h"

#include <QMetaType>

namespace BlackMisc
{
    //! Value object encapsulating a list of property indexes.
    class BLACKMISC_EXPORT CPropertyIndexList :
        public CSequence<CPropertyIndex>,
        public BlackMisc::Mixin::MetaType<CPropertyIndexList>
    {
    public:
        BLACKMISC_DECLARE_USING_MIXIN_METATYPE(CPropertyIndexList)

        //! Default constructor.
        CPropertyIndexList();

        //! Construct from a base class object.
        CPropertyIndexList(const CSequence<CPropertyIndex> &other);
    };
} //namespace

Q_DECLARE_METATYPE(BlackMisc::CPropertyIndexList)
Q_DECLARE_METATYPE(BlackMisc::CCollection<BlackMisc::CPropertyIndex>)
Q_DECLARE_METATYPE(BlackMisc::CSequence<BlackMisc::CPropertyIndex>)

#endif //guard

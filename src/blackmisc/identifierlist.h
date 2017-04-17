/* Copyright (C) 2015
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file

#ifndef BLACKMISC_IDENTIFIERLIST_H
#define BLACKMISC_IDENTIFIERLIST_H

#include "blackmisc/blackmiscexport.h"
#include "blackmisc/collection.h"
#include "blackmisc/identifier.h"
#include "blackmisc/sequence.h"
#include "blackmisc/timestampobjectlist.h"
#include "blackmisc/variant.h"

#include <QMetaType>

namespace BlackMisc
{
    /*!
     * Value object encapsulating a list of object identifiers
     */
    class BLACKMISC_EXPORT CIdentifierList :
        public CSequence<BlackMisc::CIdentifier>,
        public BlackMisc::Mixin::MetaType<CIdentifierList>,
        public BlackMisc::ITimestampObjectList<CIdentifier, CIdentifierList>
    {
    public:
        BLACKMISC_DECLARE_USING_MIXIN_METATYPE(CIdentifierList)

        //! Default constructor.
        CIdentifierList();

        //! Construct from a base class object.
        CIdentifierList(const CSequence<BlackMisc::CIdentifier> &other);

        //! This list contains an identifier which is not contained in other.
        bool containsAnyNotIn(const CIdentifierList &other) const;

        //! Get a list of identifiers reduced to maximum one per machine.
        //! If there is more than one per machine, it is undefined which one will be added.
        CIdentifierList getMachinesUnique() const;

        //! Get machine names
        QStringList getMachineNames(bool unique = true, bool sort = true) const;

        //! Remove duplicates
        int removeDuplicates();
    };
} //namespace

Q_DECLARE_METATYPE(BlackMisc::CIdentifierList)
Q_DECLARE_METATYPE(BlackMisc::CCollection<BlackMisc::CIdentifier>)
Q_DECLARE_METATYPE(BlackMisc::CSequence<BlackMisc::CIdentifier>)

#endif //guard

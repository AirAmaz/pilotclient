/* Copyright (C) 2017
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution. No part of swift project, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 */

//! \file

#ifndef BLACKMISC_PLATFORMSET_H
#define BLACKMISC_PLATFORMSET_H

#include "platform.h"
#include "blackmisc/blackmiscexport.h"
#include "blackmisc/collection.h"
#include "blackmisc/sequence.h"

#include <QMetaType>
#include <tuple>

namespace BlackMisc
{
    //! Value object for a set of platforms.
    class BLACKMISC_EXPORT CPlatformSet : public CCollection<CPlatform>
    {
    public:
        //! Default constructor.
        CPlatformSet();

        //! Initializer list constructor.
        CPlatformSet(std::initializer_list<CPlatform> il);

        //! Construct from a base class object.
        CPlatformSet(const CCollection<CPlatform> &other);

        //! All platform names
        QStringList getPlatformNames() const;

        //! Matches any given platform
        CPlatformSet matchesAny(CPlatform::Platform platform) const;

        //! Matches given platform
        CPlatformSet exactMatch(CPlatform::Platform platform) const;

        //! Contains name?
        bool containsName(const QString &platformName) const;

        //! All platforms
        static const CPlatformSet &allPlatforms();

        //! Register metadata
        static void registerMetadata();
    };
} //namespace

Q_DECLARE_METATYPE(BlackMisc::CPlatformSet)
Q_DECLARE_METATYPE(BlackMisc::CCollection<BlackMisc::CPlatform>)
Q_DECLARE_METATYPE(BlackMisc::CSequence<BlackMisc::CPlatform>)

#endif //guard

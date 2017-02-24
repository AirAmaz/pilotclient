/* Copyright (C) 2013
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file

#ifndef BLACKMISC_SAMPLEUTILS_H
#define BLACKMISC_SAMPLEUTILS_H

#include "blackmisc/blackmiscexport.h"
#include <QString>
#include <QStringList>

class QTextStream;

namespace BlackMisc
{
    //! Utils for sample programms
    class BLACKMISC_EXPORT CSampleUtils
    {
    public:
        //! Select directory among given ones
        static QString selectDirectory(const QStringList &directoryOptions, QTextStream &streamOut, QTextStream &streamIn);

    private:
        CSampleUtils() = delete;
    };
}

#endif // guard

/* Copyright (C) 2015
 * swift Project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */


//! \file

#ifndef BLACKMISC_IDENTIFIABLE_H
#define BLACKMISC_IDENTIFIABLE_H

#include "blackmisc/blackmiscexport.h"
#include "blackmisc/compare.h"
#include "blackmisc/identifier.h"

#include <QMetaObject>
#include <QObject>
#include <QString>

namespace BlackMisc
{
    /*!
     * Base class with a member CIdentifier to be inherited by a class which has an identity in the environment of modular distributed swift processes
     */
    class BLACKMISC_EXPORT CIdentifiable
    {
    public:
        //! Get identifier
        const CIdentifier &identifier() const { return m_identifier; }

        //! Identifier with current timestamp
        CIdentifier getCurrentTimestampIdentifier() const;

        //! My identifier?
        bool isMyIdentifier(const CIdentifier &otherIdentifier) const { return m_identifier == otherIdentifier; }

    protected:
        //! Use literal based object name
        CIdentifiable(const QString &objectName) : m_identifier(objectName) {}

        //! Connect with QObject providing then name
        CIdentifiable(QObject *nameProvider);

        //! Destructor
        ~CIdentifiable();

    private:
        CIdentifier m_identifier;
        QMetaObject::Connection m_connection;
    };
} // namespace

#endif // guard

/* Copyright (C) 2015
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file

#ifndef BLACKMISC_DB_DATASTORE_H
#define BLACKMISC_DB_DATASTORE_H

#include "blackmisc/blackmiscexport.h"
#include "blackmisc/propertyindex.h"
#include "blackmisc/timestampbased.h"
#include "blackmisc/variant.h"

#include <QJsonObject>
#include <QJsonValue>
#include <QObject>
#include <QString>

namespace BlackMisc
{
    class CIcon;
    namespace Db
    {
        //! State of key (in DB, ...?)
        enum DbKeyState
        {
            Undefined = 0,
            Valid     = 1 << 0,
            Invalid   = 1 << 1,
            All       = Valid | Invalid
        };

        //! Supposed to be used only in filter operations
        Q_DECLARE_FLAGS(DbKeyStateFilter, DbKeyState)

        /*!
         * Class from which a derived class can inherit datastore-related functions.
         */
        class BLACKMISC_EXPORT IDatastoreObjectWithIntegerKey : public ITimestampBased
        {
        public:
            //! Property index
            enum ColumnIndex
            {
                IndexDbIntegerKey = CPropertyIndex::GlobalIndexIDatastoreInteger,
                IndexDatabaseIcon
            };

            //! Get DB key.
            int getDbKey() const { return m_dbKey; }

            //! DB key as string
            QString getDbKeyAsString() const;

            //! Key as JSON value, or null
            QJsonValue getDbKeyAsJsonValue() const;

            //! Db ley in parentheses, e.g. "(3)"
            QString getDbKeyAsStringInParentheses(const QString &prefix = {}) const;

            //! Set the DB key
            void setDbKey(int key) { m_dbKey = key; }

            //! DB key passed as string
            void setDbKey(const QString &key);

            //! Has valid DB key
            bool hasValidDbKey() const { return m_dbKey >= 0; }

            //! Same DB key and hence equal
            bool isDbEqual(const IDatastoreObjectWithIntegerKey &other) const { return other.isLoadedFromDb() && this->isLoadedFromDb() && other.getDbKey() == this->getDbKey(); }

            //! Loaded from DB
            //! \remarks here not really needed, but added to have similar signature as IDatastoreObjectWithStringKey
            bool isLoadedFromDb() const;

            //! Matches filter?
            bool matchesDbKeyState(Db::DbKeyStateFilter filter) const;

            //! Database icon if this has valid key, otherwise empty
            const CIcon &toDatabaseIcon() const;

            //! Invalid key
            static int invalidDbKey() { return -1; }

            //! Convert string to DB key
            static int stringToDbKey(const QString &candidate);

        protected:
            //! Constructor
            IDatastoreObjectWithIntegerKey() {}

            //! Constructor
            IDatastoreObjectWithIntegerKey(int key) : m_dbKey(key) {}

            //! Set key and timestamp values
            void setKeyAndTimestampFromDatabaseJson(const QJsonObject &json, const QString &prefix = QString());

            //! Is a key available?
            static bool existsKey(const QJsonObject &json, const QString &prefix = QString());

            //! \copydoc BlackMisc::Mixin::Index::propertyByIndex
            CVariant propertyByIndex(const BlackMisc::CPropertyIndex &index) const;

            //! \copydoc BlackMisc::Mixin::Index::setPropertyByIndex
            void setPropertyByIndex(const BlackMisc::CPropertyIndex &index, const CVariant &variant);

            //! Compare by index
            int comparePropertyByIndex(const CPropertyIndex &index, const IDatastoreObjectWithIntegerKey &compareValue) const;

            //! Can given index be handled?
            static bool canHandleIndex(const BlackMisc::CPropertyIndex &index);

            int m_dbKey = -1; //!< key
        };

        /*!
         * Class from which a derived class can inherit datastore-related functions.
         */
        class BLACKMISC_EXPORT IDatastoreObjectWithStringKey : public ITimestampBased
        {
        public:
            //! Property index
            enum ColumnIndex
            {
                IndexDbStringKey = CPropertyIndex::GlobalIndexIDatastoreString,
                IndexIsLoadedFromDb,
                IndexDatabaseIcon
            };

            //! Get DB key.
            const QString &getDbKey() const { return m_dbKey; }

            //! Key as JSON value, or null
            QJsonValue getDbKeyAsJsonValue() const;

            //! Set the DB key
            void setDbKey(const QString &key) { m_dbKey = key.trimmed().toUpper(); }

            //! Has valid DB key
            bool hasValidDbKey() const { return !m_dbKey.isEmpty(); }

            //! Same DB key and hence equal
            bool isDbEqual(const IDatastoreObjectWithStringKey &other) const { return other.isLoadedFromDb() && this->isLoadedFromDb() && other.getDbKey() == this->getDbKey(); }

            //! Loaded from DB
            bool isLoadedFromDb() const { return m_loadedFromDb; }

            //! Mark as loaded from DB
            void setLoadedFromDb(bool loaded) { m_loadedFromDb = loaded; }

            //! Matches filter?
            bool matchesDbKeyState(Db::DbKeyStateFilter filter) const;

            //! Database icon if this has valid key, otherwise empty
            const CIcon &toDatabaseIcon() const;

            //! Invalid key
            static QString invalidDbKey() { return ""; }

        protected:
            //! Constructor
            IDatastoreObjectWithStringKey() {}

            //! Constructor
            IDatastoreObjectWithStringKey(const QString &key) : m_dbKey(key) {}

            //! Set key and timestamp values
            void setKeyAndTimestampFromDatabaseJson(const QJsonObject &json, const QString &prefix = QString());

            //! Is a key available?
            static bool existsKey(const QJsonObject &json, const QString &prefix = QString());

            //! \copydoc BlackMisc::Mixin::Index::propertyByIndex
            CVariant propertyByIndex(const BlackMisc::CPropertyIndex &index) const;

            //! \copydoc BlackMisc::Mixin::Index::setPropertyByIndex
            void setPropertyByIndex(const BlackMisc::CPropertyIndex &index, const CVariant &variant);

            //! Compare by index
            int comparePropertyByIndex(const BlackMisc::CPropertyIndex &index, const IDatastoreObjectWithStringKey &compareValue) const;

            //! Can given index be handled
            static bool canHandleIndex(const BlackMisc::CPropertyIndex &index);

            QString m_dbKey;                //!< key
            bool    m_loadedFromDb = false; //!< as we have no artificial key, it can happen key is set, but not loaded from DB
        };
    } // ns
} // ns

Q_DECLARE_METATYPE(BlackMisc::Db::DbKeyState)
Q_DECLARE_METATYPE(BlackMisc::Db::DbKeyStateFilter)
Q_DECLARE_OPERATORS_FOR_FLAGS(BlackMisc::Db::DbKeyStateFilter)

#endif // guard

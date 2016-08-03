/* Copyright (C) 2014
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file

#ifndef BLACKMISC_JSON_H
#define BLACKMISC_JSON_H

#include "blackmisc/blackmiscexport.h"
#include "blackmisc/fileutils.h"
#include "blackmisc/inheritancetraits.h"
#include "blackmisc/metaclass.h"

#include <QByteArray>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonValueRef>
#include <QString>
#include <QtGlobal>
#include <type_traits>
#include <utility>

class QDateTime;
class QPixmap;
class QStringList;

/*!
 * \defgroup JSON Streaming operators for JSON
 */

//! \name Streaming operators for QJsonValue (to value)
//! \ingroup JSON
//! @{
BLACKMISC_EXPORT const QJsonValue &operator >>(const QJsonValue &json, int &value);
BLACKMISC_EXPORT const QJsonValue &operator >>(const QJsonValue &json, qlonglong &value);
BLACKMISC_EXPORT const QJsonValue &operator >>(const QJsonValue &json, qulonglong &value);
BLACKMISC_EXPORT const QJsonValue &operator >>(const QJsonValue &json, uint &value);
BLACKMISC_EXPORT const QJsonValue &operator >>(const QJsonValue &json, qint16 &value);
BLACKMISC_EXPORT const QJsonValue &operator >>(const QJsonValue &json, QString &value);
BLACKMISC_EXPORT const QJsonValue &operator >>(const QJsonValue &json, QStringList &value);
BLACKMISC_EXPORT const QJsonValue &operator >>(const QJsonValue &json, double &value);
BLACKMISC_EXPORT const QJsonValue &operator >>(const QJsonValue &json, bool &value);
BLACKMISC_EXPORT const QJsonValue &operator >>(const QJsonValue &json, QDateTime &value);
BLACKMISC_EXPORT const QJsonValue &operator >>(const QJsonValue &json, QPixmap &value);
BLACKMISC_EXPORT const QJsonValue &operator >>(const QJsonValue &json, QByteArray &value);
BLACKMISC_EXPORT const QJsonValueRef &operator >>(const QJsonValueRef &json, int &value);
BLACKMISC_EXPORT const QJsonValueRef &operator >>(const QJsonValueRef &json, qlonglong &value);
BLACKMISC_EXPORT const QJsonValueRef &operator >>(const QJsonValueRef &json, qulonglong &value);
BLACKMISC_EXPORT const QJsonValueRef &operator >>(const QJsonValueRef &json, uint &value);
BLACKMISC_EXPORT const QJsonValueRef &operator >>(const QJsonValueRef &json, qint16 &value);
BLACKMISC_EXPORT const QJsonValueRef &operator >>(const QJsonValueRef &json, QString &value);
BLACKMISC_EXPORT const QJsonValueRef &operator >>(const QJsonValueRef &json, QStringList &value);
BLACKMISC_EXPORT const QJsonValueRef &operator >>(const QJsonValueRef &json, double &value);
BLACKMISC_EXPORT const QJsonValueRef &operator >>(const QJsonValueRef &json, bool &value);
BLACKMISC_EXPORT const QJsonValueRef &operator >>(const QJsonValueRef &json, QDateTime &value);
BLACKMISC_EXPORT const QJsonValueRef &operator >>(const QJsonValueRef &json, QPixmap &value);
BLACKMISC_EXPORT const QJsonValueRef &operator >>(const QJsonValueRef &json, QByteArray &value);

//! @}

//! \brief Specialized JSON serialization for enum
//! \remarks needs to be in global namespace
//! \ingroup JSON
template<class ENUM>
std::enable_if_t<std::is_enum<ENUM>::value, QJsonObject>
&operator<<(QJsonObject &json, std::pair<QString, const ENUM &> value)
{
    json.insert(value.first, QJsonValue(static_cast<int>(value.second)));
    return json;
}

//! \brief Specialized JSON serialization for QFlags generated enum
//! \ingroup JSON
template<class ENUM>
QJsonObject &operator<<(QJsonObject &json, std::pair<QString, const QFlags<ENUM> &> value)
{
    json.insert(value.first, QJsonValue(static_cast<int>(value.second)));
    return json;
}

//! \brief Specialized JSON deserialization for enum
//! \ingroup JSON
template<class ENUM>
std::enable_if_t<std::is_enum<ENUM>::value, QJsonValue>
const &operator>>(const QJsonValue &json, ENUM &value)
{
    value = static_cast<ENUM>(json.toInt());
    return json;
}

//! \brief Specialized JSON deserialization for QFlags enum
//! \ingroup JSON
template<class ENUM>
const QJsonValue &operator>>(const QJsonValue &json, QFlags<ENUM> &value)
{
    value = static_cast<QFlags<ENUM>>(json.toInt());
    return json;
}

//! \brief Specialized JSON deserialization for enum
//! \ingroup JSON
template<class ENUM>
std::enable_if_t<std::is_enum<ENUM>::value, QJsonValueRef>
const &operator>>(const QJsonValueRef &json, ENUM &value)
{
    value = static_cast<ENUM>(json.toInt());
    return json;
}

//! \brief Specialized JSON deserialization for QFlags enum
//! \ingroup JSON
template<class ENUM>
const QJsonValueRef &operator>>(const QJsonValueRef &json, QFlags<ENUM> &value)
{
    value = static_cast<QFlags<ENUM>>(json.toInt());
    return json;
}

//! \brief Specialized JSON deserialization for pair
//! \ingroup JSON
template<class FIRST, class SECOND>
const QJsonValueRef &operator>>(const QJsonValueRef &json, std::pair<FIRST, SECOND> &pair)
{
    json.toArray() >> pair.first >> pair.second;
    return json;
}

//! \brief Specialized JSON serialization for pair
//! \ingroup JSON
template<class FIRST, class SECOND>
QJsonArray &operator<<(QJsonArray &json, const std::pair<FIRST, SECOND> &pair)
{
    QJsonArray array;
    return json << QJsonValue(array << pair.first << pair.second);
}

//! \name Streaming operators for QJsonArray (from value)
//! \ingroup JSON
//! @{
BLACKMISC_EXPORT QJsonArray &operator<<(QJsonArray &json, const int value);
BLACKMISC_EXPORT QJsonArray &operator<<(QJsonArray &json, const std::pair<QString, qint16> &value);
BLACKMISC_EXPORT QJsonArray &operator<<(QJsonArray &json, const qlonglong value);
BLACKMISC_EXPORT QJsonArray &operator<<(QJsonArray &json, const uint value);
BLACKMISC_EXPORT QJsonArray &operator<<(QJsonArray &json, const qulonglong value);
BLACKMISC_EXPORT QJsonArray &operator<<(QJsonArray &json, const QString &value);
BLACKMISC_EXPORT QJsonArray &operator<<(QJsonArray &json, const double value);
BLACKMISC_EXPORT QJsonArray &operator<<(QJsonArray &json, const bool value);
BLACKMISC_EXPORT QJsonArray &operator<<(QJsonArray &json, const QDateTime &value);
BLACKMISC_EXPORT QJsonArray &operator<<(QJsonArray &json, const QPixmap &value);
BLACKMISC_EXPORT QJsonArray &operator<<(QJsonArray &json, const QByteArray &value);
//! @}

//! \name Streaming operators for QJsonObject (from value)
//! \ingroup JSON
//! @{
BLACKMISC_EXPORT QJsonObject &operator<<(QJsonObject &json, const std::pair<QString, const int &> &value);
BLACKMISC_EXPORT QJsonObject &operator<<(QJsonObject &json, const std::pair<QString, const qint16 &> &value);
BLACKMISC_EXPORT QJsonObject &operator<<(QJsonObject &json, const std::pair<QString, const qlonglong &> &value);
BLACKMISC_EXPORT QJsonObject &operator<<(QJsonObject &json, const std::pair<QString, const uint &> &value);
BLACKMISC_EXPORT QJsonObject &operator<<(QJsonObject &json, const std::pair<QString, const qulonglong &> &value);
BLACKMISC_EXPORT QJsonObject &operator<<(QJsonObject &json, const std::pair<QString, const QString &> &value);
BLACKMISC_EXPORT QJsonObject &operator<<(QJsonObject &json, const std::pair<QString, const QStringList &> &value);
BLACKMISC_EXPORT QJsonObject &operator<<(QJsonObject &json, const std::pair<QString, const double &> &value);
BLACKMISC_EXPORT QJsonObject &operator<<(QJsonObject &json, const std::pair<QString, const bool &> &value);
BLACKMISC_EXPORT QJsonObject &operator<<(QJsonObject &json, const std::pair<QString, const QDateTime &> &value);
BLACKMISC_EXPORT QJsonObject &operator<<(QJsonObject &json, const std::pair<QString, const QPixmap &> &value);
BLACKMISC_EXPORT QJsonObject &operator<<(QJsonObject &json, const std::pair<QString, const QByteArray &> &value);
//! @}

namespace BlackMisc
{
    class CEmpty;

    namespace Json
    {
        //! Append to first JSON object (concatenate)
        //! \ingroup JSON
        BLACKMISC_EXPORT QJsonObject &appendJsonObject(QJsonObject &target, const QJsonObject &toBeAppended);

        //! JSON Object from string
        //! \ingroup JSON
        BLACKMISC_EXPORT QJsonObject jsonObjectFromString(const QString &json);

        //! JSON Array from string
        //! \ingroup JSON
        BLACKMISC_EXPORT QJsonArray jsonArrayFromString(const QString &json);

        //! Creates an incremental json object from two existing objects
        BLACKMISC_EXPORT QJsonObject getIncrementalObject(const QJsonObject &previousObject, const QJsonObject &currentObject);

        //! Merges an incremental json object into an existing one
        BLACKMISC_EXPORT QJsonObject applyIncrementalObject(const QJsonObject &previousObject, const QJsonObject &incrementalObject);

        /*!
         * Load JSON file and init by that
         */
        template <class T>
        bool loadFromJsonFile(T &object, const QString &fileNameAndPath)
        {
            const QString jsonString(CFileUtils::readFileToString(fileNameAndPath));
            if (jsonString.isEmpty()) { return false; }
            object.convertFromJson(jsonString);
            return true;
        }

        /*!
         * Save to JSON file
         */
        template <class T>
        bool saveToJsonFile(const T &object, const QString &fileNameAndPath)
        {
            const QString jsonString(object.toJsonString());
            if (jsonString.isEmpty()) { return false; }
            return CFileUtils::writeStringToFile(jsonString, fileNameAndPath);
        }
    } // Json

    namespace Mixin
    {
        /*!
         * CRTP class template which will generate marshalling operators for a derived class with its own marshalling implementation.
         *
         * \tparam Must implement public methods QJsonObject toJson() const and void convertFromJson(const QJsonObject &json).
         */
        template <class Derived>
        class JsonOperators
        {
        public:
            //! operator >> for JSON
            friend const QJsonObject &operator>>(const QJsonObject &json, Derived &obj)
            {
                obj.convertFromJson(json);
                return json;
            }

            //! operator >> for JSON
            friend const QJsonValue &operator>>(const QJsonValue &json, Derived &obj)
            {
                obj.convertFromJson(json.toObject());
                return json;
            }

            //! operator >> for JSON
            friend const QJsonValueRef &operator>>(const QJsonValueRef &json, Derived &obj)
            {
                obj.convertFromJson(json.toObject());
                return json;
            }

            //! operator << for JSON
            friend QJsonArray &operator<<(QJsonArray &json, const Derived &obj)
            {
                json.append(obj.toJson());
                return json;
            }

            //! operator << for JSON
            friend QJsonObject &operator<<(QJsonObject &json, const std::pair<QString, const Derived &> &value)
            {
                json.insert(value.first, QJsonValue(value.second.toJson()));
                return json;
            }
        };

        /*!
         * CRTP class template from which a derived class can inherit common methods dealing with JSON by metatuple.
         *
         * \see BLACKMISC_DECLARE_USING_MIXIN_JSON
         */
        template <class Derived>
        class JsonByMetaClass : public JsonOperators<Derived>
        {
        public:
            //! Cast to JSON object
            QJsonObject toJson() const
            {
                QJsonObject json;
                auto meta = introspect<Derived>().without(MetaFlags<DisabledForJson>());
                meta.forEachMemberName(*derived(), [ & ](const auto & member, const QString & name)
                {
                    json << std::pair<QString, const std::decay_t<decltype(member)> &>(name, member); // std::make_pair causes an ambiguous operator<<
                });
                return Json::appendJsonObject(json, baseToJson(static_cast<const TBaseOfT<Derived> *>(derived())));
            }

            //! Convenience function JSON as string
            QString toJsonString(QJsonDocument::JsonFormat format = QJsonDocument::Indented) const
            {
                QJsonDocument jsonDoc(toJson());
                return jsonDoc.toJson(format);
            }

            //! Assign from JSON object
            void convertFromJson(const QJsonObject &json)
            {
                baseConvertFromJson(static_cast<TBaseOfT<Derived> *>(derived()), json);
                auto meta = introspect<Derived>().without(MetaFlags<DisabledForJson>());
                meta.forEachMemberName(*derived(), [ & ](auto & member, const QString & name)
                {
                    if (json.contains(name)) { json.value(name) >> member; }
                });
            }

            //! Assign from JSON object string
            void convertFromJson(const QString &jsonString)
            {
                convertFromJson(BlackMisc::Json::jsonObjectFromString(jsonString));
            }

        private:
            const Derived *derived() const { return static_cast<const Derived *>(this); }
            Derived *derived() { return static_cast<Derived *>(this); }

            template <typename T> static QJsonObject baseToJson(const T *base) { return base->toJson(); }
            template <typename T> static void baseConvertFromJson(T *base, const QJsonObject &json) { base->convertFromJson(json); }
            static QJsonObject baseToJson(const void *) { return {}; }
            static void baseConvertFromJson(void *, const QJsonObject &) {}
            static QJsonObject baseToJson(const CEmpty *) { return {}; }
            static void baseConvertFromJson(CEmpty *, const QJsonObject &) {}
        };

        /*!
         * When a derived class and a base class both inherit from Mixin::JsonByTuple,
         * the derived class uses this macro to disambiguate the inherited members.
         */
#       define BLACKMISC_DECLARE_USING_MIXIN_JSON(DERIVED)                      \
    using ::BlackMisc::Mixin::JsonByMetaClass<DERIVED>::toJson;             \
    using ::BlackMisc::Mixin::JsonByMetaClass<DERIVED>::convertFromJson;

    } // Mixin
} // BlackMisc

#endif // guard

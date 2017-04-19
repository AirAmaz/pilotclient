/* Copyright (C) 2014
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "json.h"
#include "blackmisc/imageutils.h"
#include "blackmisc/stringutils.h"

#include <QDateTime>
#include <QJsonDocument>
#include <QList>
#include <QStringList>

class QPixmap;

const QJsonValue &operator >>(const QJsonValue &json, int &value)
{
    value = json.toInt();
    return json;
}

const QJsonValue &operator >>(const QJsonValue &json, qlonglong &value)
{
    value = static_cast<qlonglong>(json.toDouble());
    return json;
}

const QJsonValue &operator >>(const QJsonValue &json, qulonglong &value)
{
    value = static_cast<qulonglong>(json.toDouble());
    return json;
}

const QJsonValue &operator >>(const QJsonValue &json, uint &value)
{
    value = static_cast<uint>(json.toInt());
    return json;
}

const QJsonValue &operator >>(const QJsonValue &json, qint16 &value)
{
    value = static_cast<qint16>(json.toInt());
    return json;
}

const QJsonValue &operator >>(const QJsonValue &json, QString &value)
{
    value = json.toString();
    return json;
}

const QJsonValue &operator >>(const QJsonValue &json, QStringList &value)
{
    for (auto && element : json.toArray()) { value << element.toString(); }
    return json;
}

const QJsonValue &operator >>(const QJsonValue &json, double &value)
{
    value = json.toDouble();
    return json;
}

const QJsonValue &operator >>(const QJsonValue &json, bool &value)
{
    value = json.toBool();
    return json;
}

const QJsonValue &operator >>(const QJsonValue &json, QDateTime &value)
{
    value = QDateTime::fromString(json.toString());
    return json;
}

const QJsonValue &operator >>(const QJsonValue &json, QPixmap &value)
{
    const QString hex(json.toString());
    BlackMisc::pngHexStringToPixmapRef(hex, value);
    return json;
}

const QJsonValue &operator >>(const QJsonValue &json, QByteArray &value)
{
    const QString hex(json.toString());
    value = BlackMisc::byteArrayFromHexString(hex);
    return json;
}

const QJsonValueRef &operator >>(const QJsonValueRef &json, int &value)
{
    value = json.toInt();
    return json;
}

const QJsonValueRef &operator >>(const QJsonValueRef &json, qlonglong &value)
{
    value = static_cast<qlonglong>(json.toDouble());
    return json;
}

const QJsonValueRef &operator >>(const QJsonValueRef &json, qulonglong &value)
{
    value = static_cast<qulonglong>(json.toDouble());
    return json;
}

const QJsonValueRef &operator >>(const QJsonValueRef &json, uint &value)
{
    value = static_cast<uint>(json.toInt());
    return json;
}

const QJsonValueRef &operator >>(const QJsonValueRef &json, qint16 &value)
{
    value = static_cast<qint16>(json.toInt());
    return json;
}

const QJsonValueRef &operator >>(const QJsonValueRef &json, QString &value)
{
    value = json.toString();
    return json;
}

const QJsonValueRef &operator >>(const QJsonValueRef &json, QStringList &value)
{
    for (auto && element : json.toArray()) { value << element.toString(); }
    return json;
}

const QJsonValueRef &operator >>(const QJsonValueRef &json, double &value)
{
    value = json.toDouble();
    return json;
}

const QJsonValueRef &operator >>(const QJsonValueRef &json, bool &value)
{
    value = json.toBool();
    return json;
}

const QJsonValueRef &operator >>(const QJsonValueRef &json, QDateTime &value)
{
    value = QDateTime::fromString(json.toString());
    return json;
}

const QJsonValueRef &operator >>(const QJsonValueRef &json, QPixmap &value)
{
    const QString hex(json.toString());
    BlackMisc::pngHexStringToPixmapRef(hex, value);
    return json;
}

const QJsonValueRef &operator >>(const QJsonValueRef &json, QByteArray &value)
{
    const QString hex(json.toString());
    value = BlackMisc::byteArrayFromHexString(hex);
    return json;
}

QJsonArray &operator<<(QJsonArray &json, const int value)
{
    json.append(QJsonValue(value));
    return json;
}

QJsonArray &operator<<(QJsonArray &json, const qint16 value)
{
    json.append(QJsonValue(value));
    return json;
}

QJsonArray &operator<<(QJsonArray &json, const qlonglong value)
{
    json.append(QJsonValue(value));
    return json;
}

QJsonArray &operator<<(QJsonArray &json, const uint value)
{
    json.append(QJsonValue(static_cast<int>(value)));
    return json;
}

QJsonArray &operator<<(QJsonArray &json, const qulonglong value)
{
    json.append(QJsonValue(static_cast<int>(value)));
    return json;
}

QJsonArray &operator<<(QJsonArray &json, const QString &value)
{
    json.append(QJsonValue(value));
    return json;
}

QJsonArray &operator<<(QJsonArray &json, const double value)
{
    json.append(QJsonValue(value));
    return json;
}

QJsonArray &operator<<(QJsonArray &json, const bool value)
{
    json.append(QJsonValue(value));
    return json;
}

QJsonArray &operator<<(QJsonArray &json, const QDateTime &value)
{
    json.append(QJsonValue(value.toString()));
    return json;
}

QJsonArray &operator<<(QJsonArray &json, const QPixmap &value)
{
    QString pm(BlackMisc::pixmapToPngHexString(value));
    json.append(QJsonValue(pm));
    return json;
}

QJsonArray &operator<<(QJsonArray &json, const QByteArray &value)
{
    QString pm(BlackMisc::byteArrayFromHexString(value));
    json.append(QJsonValue(pm));
    return json;
}

QJsonObject &operator<<(QJsonObject &json, const std::pair<QString, const int &> &value)
{
    json.insert(value.first, QJsonValue(value.second));
    return json;
}

QJsonObject &operator<<(QJsonObject &json, const std::pair<QString, const qint16 &> &value)
{
    json.insert(value.first, QJsonValue(value.second));
    return json;
}

QJsonObject &operator<<(QJsonObject &json, const std::pair<QString, const qulonglong &> &value)
{
    json.insert(value.first, QJsonValue(static_cast<int>(value.second)));
    return json;
}

QJsonObject &operator<<(QJsonObject &json, const std::pair<QString, const qlonglong &> &value)
{
    json.insert(value.first, QJsonValue(value.second));
    return json;
}

QJsonObject &operator<<(QJsonObject &json, const std::pair<QString, const uint &> &value)
{
    json.insert(value.first, QJsonValue(static_cast<int>(value.second)));
    return json;
}

QJsonObject &operator<<(QJsonObject &json, const std::pair<QString, const QString &> &value)
{
    json.insert(value.first, QJsonValue(value.second));
    return json;
}

QJsonObject &operator<<(QJsonObject &json, const std::pair<QString, const QStringList &> &value)
{
    json.insert(value.first, QJsonValue(QJsonArray::fromStringList(value.second)));
    return json;
}

QJsonObject &operator<<(QJsonObject &json, const std::pair<QString, const double &> &value)
{
    json.insert(value.first, QJsonValue(value.second));
    return json;
}

QJsonObject &operator<<(QJsonObject &json, const std::pair<QString, const bool &> &value)
{
    json.insert(value.first, QJsonValue(value.second));
    return json;
}

QJsonObject &operator<<(QJsonObject &json, const std::pair<QString, const QDateTime &> &value)
{
    json.insert(value.first, QJsonValue(value.second.toString()));
    return json;
}

QJsonObject &operator<<(QJsonObject &json, const std::pair<QString, const QPixmap &> &value)
{
    QString pm(BlackMisc::pixmapToPngHexString(value.second));
    json.insert(value.first, pm);
    return json;
}

QJsonObject &operator<<(QJsonObject &json, const std::pair<QString, const QByteArray &> &value)
{
    QString pm(BlackMisc::bytesToHexString(value.second));
    json.insert(value.first, pm);
    return json;
}

QJsonObject &operator<<(QJsonObject &json, const std::pair<BlackMisc::CExplicitLatin1String, const int &> &value)
{
    json[value.first] = QJsonValue(value.second);
    return json;
}

QJsonObject &operator<<(QJsonObject &json, const std::pair<BlackMisc::CExplicitLatin1String, const qint16 &> &value)
{
    json[value.first] = QJsonValue(value.second);
    return json;
}

QJsonObject &operator<<(QJsonObject &json, const std::pair<BlackMisc::CExplicitLatin1String, const qulonglong &> &value)
{
    json[value.first] = QJsonValue(static_cast<int>(value.second));
    return json;
}

QJsonObject &operator<<(QJsonObject &json, const std::pair<BlackMisc::CExplicitLatin1String, const qlonglong &> &value)
{
    json[value.first] = QJsonValue(value.second);
    return json;
}

QJsonObject &operator<<(QJsonObject &json, const std::pair<BlackMisc::CExplicitLatin1String, const uint &> &value)
{
    json[value.first] = QJsonValue(static_cast<int>(value.second));
    return json;
}

QJsonObject &operator<<(QJsonObject &json, const std::pair<BlackMisc::CExplicitLatin1String, const QString &> &value)
{
    json[value.first] = QJsonValue(value.second);
    return json;
}

QJsonObject &operator<<(QJsonObject &json, const std::pair<BlackMisc::CExplicitLatin1String, const QStringList &> &value)
{
    json[value.first] = QJsonValue(QJsonArray::fromStringList(value.second));
    return json;
}

QJsonObject &operator<<(QJsonObject &json, const std::pair<BlackMisc::CExplicitLatin1String, const double &> &value)
{
    json[value.first] = QJsonValue(value.second);
    return json;
}

QJsonObject &operator<<(QJsonObject &json, const std::pair<BlackMisc::CExplicitLatin1String, const bool &> &value)
{
    json[value.first] = QJsonValue(value.second);
    return json;
}

QJsonObject &operator<<(QJsonObject &json, const std::pair<BlackMisc::CExplicitLatin1String, const QDateTime &> &value)
{
    json[value.first] = QJsonValue(value.second.toString());
    return json;
}

QJsonObject &operator<<(QJsonObject &json, const std::pair<BlackMisc::CExplicitLatin1String, const QPixmap &> &value)
{
    QString pm(BlackMisc::pixmapToPngHexString(value.second));
    json[value.first] = pm;
    return json;
}

QJsonObject &operator<<(QJsonObject &json, const std::pair<BlackMisc::CExplicitLatin1String, const QByteArray &> &value)
{
    QString pm(BlackMisc::bytesToHexString(value.second));
    json[value.first] = pm;
    return json;
}

namespace BlackMisc
{
    namespace Json
    {
        QJsonObject jsonObjectFromString(const QString &json)
        {
            if (json.isEmpty()) { return QJsonObject();}
            QJsonDocument jsonDoc(QJsonDocument::fromJson(json.toUtf8()));
            return jsonDoc.object();
        }

        QJsonArray jsonArrayFromString(const QString &json)
        {
            if (json.isEmpty()) { return QJsonArray();}
            QJsonDocument jsonDoc(QJsonDocument::fromJson(json.toUtf8()));
            return jsonDoc.array();
        }

        QJsonObject &appendJsonObject(QJsonObject &target, const QJsonObject &toBeAppended)
        {
            if (toBeAppended.isEmpty()) return target;
            QStringList keys = toBeAppended.keys();
            foreach (const QString &key, keys)
            {
                target.insert(key, toBeAppended.value(key));
            }
            return target;
        }

        QJsonObject getIncrementalObject(const QJsonObject &previousObject, const QJsonObject &currentObject)
        {
            QJsonObject incrementalObject = currentObject;
            for (const auto &key : previousObject.keys()) // clazy:exclude=range-loop
            {
                if (previousObject.value(key).isObject())
                {
                    auto child = getIncrementalObject(previousObject.value(key).toObject(), currentObject.value(key).toObject());
                    if (child.isEmpty()) incrementalObject.remove(key);
                    else incrementalObject.insert(key, child);
                }
                else
                {
                    if (currentObject.value(key) == previousObject.value(key))
                        incrementalObject.remove(key);
                }
            }
            return incrementalObject;
        }

        QJsonObject applyIncrementalObject(const QJsonObject &previousObject, const QJsonObject &incrementalObject)
        {
            QJsonObject currentObject = previousObject;
            for (const auto &key : incrementalObject.keys()) // clazy:exclude=range-loop
            {
                // If it is not an object, just insert the value
                if (!incrementalObject.value(key).isObject())
                {
                    currentObject.insert(key, incrementalObject.value(key));
                }
                else
                {
                    auto child = applyIncrementalObject(currentObject.value(key).toObject(), incrementalObject.value(key).toObject());
                    currentObject.insert(key, child);
                }
            }
            return currentObject;
        }

        bool looksLikeJson(const QString &json)
        {
            if (json.isEmpty()) { return false; }
            const QString t = json.trimmed();
            return t.startsWith('{') && t.endsWith('}');
        }

        bool looksLikeSwiftJson(const QString &json)
        {
            // further checks would go here
            return looksLikeJson(json);
        }
    }
}

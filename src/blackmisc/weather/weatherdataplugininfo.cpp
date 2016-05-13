/* Copyright (C) 2016
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "blackmisc/weather/weatherdataplugininfo.h"

#include <QJsonValue>
#include <QtGlobal>

using namespace BlackMisc;

namespace BlackMisc
{
    namespace Weather
    {
        CWeatherDataPluginInfo::CWeatherDataPluginInfo(const QString &identifier, const QString &name, const QString &description, bool valid) :
            m_identifier(identifier), m_name(name), m_description(description), m_valid(valid)
        { }

        void CWeatherDataPluginInfo::convertFromJson(const QJsonObject &json)
        {
            if (json.contains("IID"))   // comes from the plugin
            {
                // json data is already validated by CPluginManagerWeatherData
                CValueObject::convertFromJson(json["MetaData"].toObject());
                m_valid = true;
            }
            else
            {
                CValueObject::convertFromJson(json);
            }
        }

        QString CWeatherDataPluginInfo::convertToQString(bool i18n) const
        {
            Q_UNUSED(i18n);
            return QString("%1 (%2)").arg(m_name, m_identifier);
        }

    } // ns
} // ns

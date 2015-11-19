/*  Copyright (C) 2013 VATSIM Community / contributors
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "service.h"
#include <XPLM/XPLMPlanes.h>
#include <XPLM/XPLMUtilities.h>
#include <QDebug>
#include <QTimer>

namespace XBus
{

    CService::CService(QObject *parent) : QObject(parent)
    {
        m_airportUpdater = new QTimer(this);
        m_airportUpdater->start(60000);
        connect(m_airportUpdater, &QTimer::timeout, this, &CService::updateAirportsInRange);
        updateAirportsInRange();
    }

    void CService::onAircraftModelChanged()
    {
        char filename[256];
        char path[512];
        XPLMGetNthAircraftModel(XPLM_USER_AIRCRAFT, filename, path);
        emit aircraftModelChanged(path, filename, getAircraftLivery(), getAircraftIcaoCode());
    }

    void CService::addTextMessage(const QString &text, double red, double green, double blue)
    {
        if (text.isEmpty()) { return; }
        int lineLength = m_messages.maxLineLength() - 1;
        QStringList wrappedLines;
        for (int i = 0; i < text.size(); i += lineLength)
        {
            static const QChar ellipsis = 0x2026;
            wrappedLines.push_back(text.mid(i, lineLength) + ellipsis);
        }
        wrappedLines.back().chop(1);
        if (wrappedLines.back().isEmpty()) { wrappedLines.pop_back(); }
        else if (wrappedLines.back().size() == 1 && wrappedLines.size() > 1)
        {
            (wrappedLines.end() - 2)->chop(1);
            (wrappedLines.end() - 2)->append(wrappedLines.back());
            wrappedLines.pop_back();
        }
        for (const auto &line : wrappedLines)
        {
            m_messages.addMessage({ line.toStdString(), static_cast<float>(red), static_cast<float>(green), static_cast<float>(blue) });
        }
    }

    QString CService::getAircraftModelPath() const
    {
        char filename[256];
        char path[512];
        XPLMGetNthAircraftModel(XPLM_USER_AIRCRAFT, filename, path);
        return path;
    }

    QString CService::getAircraftModelFilename() const
    {
        char filename[256];
        char path[512];
        XPLMGetNthAircraftModel(XPLM_USER_AIRCRAFT, filename, path);
        return filename;
    }

    int CService::getXPlaneVersionMajor() const
    {
        int version;
        XPLMGetVersions(&version, nullptr, nullptr);
        if (version > 5000) { version /= 10; }
        return version / 100;
    }

    int CService::getXPlaneVersionMinor() const
    {
        int version;
        XPLMGetVersions(&version, nullptr, nullptr);
        if (version > 5000) { version /= 10; }
        return version % 100;
    }

    QString CService::getXPlaneInstallationPath() const
    {
        char path[512];
        XPLMGetSystemPath(path);
        return path;
    }

    QString CService::getXPlanePreferencesPath() const
    {
        char path[512];
        XPLMGetPrefsPath(path);
        return path;
    }

    void CService::readAirportsDatabase()
    {
        auto first = XPLMFindFirstNavAidOfType(xplm_Nav_Airport);
        auto last = XPLMFindLastNavAidOfType(xplm_Nav_Airport);
        if (first != XPLM_NAV_NOT_FOUND)
        {
            for (auto i = first; i <= last; ++i)
            {
                float lat, lon;
                char icao[32];
                XPLMGetNavAidInfo(i, nullptr, &lat, &lon, nullptr, nullptr, nullptr, icao, nullptr, nullptr);
                if (icao[0] != 0)
                {
                    using namespace BlackMisc::Math;
                    m_airports.push_back(BlackMisc::Simulation::XPlane::CNavDataReference(i, lat, lon));
                }
            }
        }
    }

    void CService::updateAirportsInRange()
    {
        if (m_airports.isEmpty())
        {
            readAirportsDatabase();
        }
        using namespace BlackMisc::Math;
        using namespace BlackMisc::Geo;
        QStringList icaos, names;
        QDoubleList lats, lons, alts;
        for (const auto &navref : m_airports.findClosest(20, CCoordinateGeodetic(getLatitude(), getLongitude(), 0)))
        {
            float lat, lon, alt;
            char icao[32], name[256];
            XPLMGetNavAidInfo(navref.id(), nullptr, &lat, &lon, &alt, nullptr, nullptr, icao, name, nullptr);
            icaos.push_back(icao);
            names.push_back(name);
            lats.push_back(lat);
            lons.push_back(lon);
            alts.push_back(alt);
        }
        emit airportsInRangeUpdated(icaos, names, lats, lons, alts);
    }

}

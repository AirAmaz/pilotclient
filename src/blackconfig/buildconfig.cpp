/* Copyright (C) 2013
 * swift Project Community/Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \cond PRIVATE

#include "buildconfig.h"
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLocale>
#include <QStandardPaths>
#include <QStringList>
#include <QtGlobal>
#include <QSysInfo>

namespace BlackConfig
{
    bool CBuildConfig::isCompiledWithMsFlightSimulatorSupport()
    {
        return CBuildConfig::isCompiledWithFs9Support() || CBuildConfig::isCompiledWithFsxSupport() || CBuildConfig::isCompiledWithP3DSupport();
    }

    bool CBuildConfig::isCompiledWithFlightSimulatorSupport()
    {
        return CBuildConfig::isCompiledWithFsxSupport() || CBuildConfig::isCompiledWithXPlaneSupport();
    }

    const QString &CBuildConfig::swiftGuiExecutableName()
    {
        static const QString s("swiftguistd");
        return s;
    }

    const QString &CBuildConfig::swiftCoreExecutableName()
    {
        static const QString s("swiftcore");
        return s;
    }

    const QString &CBuildConfig::swiftDataExecutableName()
    {
        static const QString s("swiftdata");
        return s;
    }

    bool CBuildConfig::isKnownExecutableName(const QString &executable)
    {
        return executable == CBuildConfig::swiftCoreExecutableName() ||
               executable == CBuildConfig::swiftDataExecutableName() ||
               executable == CBuildConfig::swiftGuiExecutableName();
    }

    bool CBuildConfig::isRunningOnWindowsNtPlatform()
    {
#ifdef Q_OS_WIN
        // QSysInfo::WindowsVersion only available on Win platforms
        return (QSysInfo::WindowsVersion & QSysInfo::WV_NT_based);
#else
        return false;
#endif
    }

    bool CBuildConfig::isRunningOnWindows10()
    {
#ifdef Q_OS_WIN
        // QSysInfo::WindowsVersion only available on Win platforms
        if (!isRunningOnWindowsNtPlatform()) { return false; }
        return (QSysInfo::WindowsVersion == QSysInfo::WV_10_0);
#else
        return false;
#endif
    }

    bool CBuildConfig::isRunningOnMacOSXPlatform()
    {
#ifdef Q_OS_OSX
        return true;
#else
        return false;
#endif
    }

    bool CBuildConfig::isRunningOnLinuxPlatform()
    {
#ifdef Q_OS_LINUX
        return true;
#else
        return false;
#endif
    }

    bool CBuildConfig::isRunningOnUnixPlatform()
    {
        return isRunningOnMacOSXPlatform() || isRunningOnLinuxPlatform();
    }

    bool CBuildConfig::isDebugBuild()
    {
#ifdef QT_DEBUG
        return true;
#else
        return false;
#endif
    }

    bool CBuildConfig::isReleaseBuild()
    {
#ifdef QT_NO_DEBUG
        return true;
#else
        return false;
#endif
    }

    bool CBuildConfig::isLifetimeExpired()
    {
        if (getEol().isValid())
        {
            return QDateTime::currentDateTime() > getEol();
        }
        else
        {
            return true;
        }
    }

    bool CBuildConfig::canRunInDeveloperEnvironment()
    {
        if (CBuildConfig::isDevBranch()) { return true; }
        return !CBuildConfig::isStableBranch();
    }

    QString boolToYesNo(bool v)
    {
        return v ? "yes" : "no";
    }

    const QString &CBuildConfig::compiledWithInfo(bool shortVersion)
    {
        if (shortVersion)
        {
            static QString infoShort;
            if (infoShort.isEmpty())
            {
                QStringList sl;
                if (CBuildConfig::isCompiledWithBlackCore())     { sl << "BlackCore"; }
                if (CBuildConfig::isCompiledWithBlackSound())    { sl << "BlackSound"; }
                if (CBuildConfig::isCompiledWithBlackInput())    { sl << "BlackInput"; }
                if (CBuildConfig::isCompiledWithGui())           { sl << "BlackGui"; }
                if (CBuildConfig::isCompiledWithFs9Support())    { sl << "FS9"; }
                if (CBuildConfig::isCompiledWithFsxSupport())    { sl << "FSX"; }
                if (CBuildConfig::isCompiledWithXPlaneSupport()) { sl << "XPlane"; }
                if (CBuildConfig::isCompiledWithP3DSupport())    { sl << "P3D"; }
                infoShort = sl.join(", ");
                if (infoShort.isEmpty()) { infoShort = "<none>"; }
            }
            return infoShort;
        }
        else
        {
            static QString infoLong;
            if (infoLong.isEmpty())
            {
                infoLong = infoLong.append("BlackCore: ").append(boolToYesNo(isCompiledWithBlackCore()));
                infoLong = infoLong.append(" BlackInput: ").append(boolToYesNo(isCompiledWithBlackInput()));
                infoLong = infoLong.append(" BlackSound: ").append(boolToYesNo(isCompiledWithBlackSound()));
                infoLong = infoLong.append(" GUI: ").append(boolToYesNo(isCompiledWithGui()));

                infoLong = infoLong.append(" FS9: ").append(boolToYesNo(isCompiledWithFs9Support()));
                infoLong = infoLong.append(" FSX: ").append(boolToYesNo(isCompiledWithFsxSupport()));
                infoLong = infoLong.append(" P3D: ").append(boolToYesNo(isCompiledWithP3DSupport()));
                infoLong = infoLong.append(" XPlane: ").append(boolToYesNo(isCompiledWithXPlaneSupport()));
            }
            return infoLong;
        }
    }

    const QString &CBuildConfig::buildDateAndTime()
    {
        // http://en.cppreference.com/w/cpp/preprocessor/replace#Predefined_macros
        static const QString buildDateAndTime(__DATE__ " "  __TIME__);
        return buildDateAndTime;
    }

    const QVersionNumber &CBuildConfig::getVersion()
    {
        static const QVersionNumber v { versionMajor(), versionMinor(), versionMicro(), buildTimestampAsVersionSegment(buildTimestamp()) };
        return v;
    }

    const QString &CBuildConfig::getVersionString()
    {
        static const QString s(getVersion().toString());
        return s;
    }

    namespace Private
    {
        const QDateTime buildTimestampImpl()
        {
            // Mar 27 2017 20:17:06 (needs to be on english locale, otherwise fails - e.g.
            QDateTime dt = QLocale(QLocale::English).toDateTime(CBuildConfig::buildDateAndTime().simplified(), "MMM d yyyy hh:mm:ss");
            dt.setUtcOffset(0);
            return dt;
        }
    }

    const QDateTime &CBuildConfig::buildTimestamp()
    {
        // Mar 27 2017 20:17:06
        static const QDateTime dt = Private::buildTimestampImpl();
        return dt;
    }

    int CBuildConfig::buildTimestampAsVersionSegment(const QDateTime &buildTimestamp)
    {
        if (buildTimestamp.isValid())
        {
            const QString bts = buildTimestamp.toString("yyyyMMddHHmm");
            bool ok;
            const long long btsll = bts.toLongLong(&ok); // at least 64bit
            if (!ok) { return 0; }
            // now we have to converto int
            // max 2147483647 (2^31 - 1)
            //      1MMddHHmm (years since 2010)
            const long long yearOffset = 201000000000;
            const int btsInt = btsll - yearOffset;
            return btsInt;
        }
        return 0; // intentionally 0 => 0.7.3.0 <-
    }

    const QStringList &CBuildConfig::getBuildAbiParts()
    {
        static const QStringList parts = QSysInfo::buildAbi().split('-');
        return parts;
    }

    int buildWordSizeImpl()
    {
        if (CBuildConfig::getBuildAbiParts().length() < 3) { return -1; }
        const QString abiWs = CBuildConfig::getBuildAbiParts()[2];
        if (abiWs.contains("32")) { return 32; }
        if (abiWs.contains("64")) { return 64; }
        return -1;
    }

    int CBuildConfig::buildWordSize()
    {
        static const int bws = buildWordSizeImpl();
        return bws;
    }

    QString CBuildConfig::guessMyPlatformString()
    {
        // 32/64 only matters on Windows
        // 64 is default
        QString p;
        QString ws = (buildWordSize() == 32) ? "32" : "64";
        if (BlackConfig::CBuildConfig::isRunningOnWindowsNtPlatform())
        {
            p = "win-" + ws;
        }
        else if (BlackConfig::CBuildConfig::isRunningOnMacOSXPlatform()) { p = "macos-64"; }
        else if (BlackConfig::CBuildConfig::isRunningOnLinuxPlatform()) { p = "linux-64"; }

        if (!p.isEmpty() && BlackConfig::CBuildConfig::isVatsimVersion()) { p += "-vatsim"; }
        return p;
    }
} // ns

//! \endcond

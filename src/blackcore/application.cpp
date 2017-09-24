/* Copyright (C) 2016
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "blackconfig/buildconfig.h"
#include "blackcore/application.h"
#include "blackcore/context/contextapplication.h"
#include "blackcore/cookiemanager.h"
#include "blackcore/corefacade.h"
#include "blackcore/vatsim/networkvatlib.h"
#include "blackcore/registermetadata.h"
#include "blackcore/setupreader.h"
#include "blackcore/webdataservices.h"
#include "blackmisc/atomicfile.h"
#include "blackmisc/datacache.h"
#include "blackmisc/dbusserver.h"
#include "blackmisc/directoryutils.h"
#include "blackmisc/eventloop.h"
#include "blackmisc/filelogger.h"
#include "blackmisc/logcategory.h"
#include "blackmisc/logcategorylist.h"
#include "blackmisc/loghandler.h"
#include "blackmisc/logmessage.h"
#include "blackmisc/logpattern.h"
#include "blackmisc/network/networkutils.h"
#include "blackmisc/registermetadata.h"
#include "blackmisc/settingscache.h"
#include "blackmisc/slot.h"
#include "blackmisc/stringutils.h"
#include "blackmisc/threadutils.h"
#include "blackmisc/verify.h"

#include <stdbool.h>
#include <stdio.h>
#include <QCoreApplication>
#include <QDateTime>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QHttpMultiPart>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProcess>
#include <QSslSocket>
#include <QStandardPaths>
#include <QStringBuilder>
#include <QTemporaryDir>
#include <QThread>
#include <QTime>
#include <QTimer>
#include <QTranslator>
#include <QWriteLocker>
#include <Qt>
#include <QtGlobal>
#include <cstdlib>

#ifdef BLACK_USE_CRASHPAD
#include "crashpad/client/crashpad_client.h"
#include "crashpad/client/crash_report_database.h"
#include "crashpad/client/settings.h"
#include "crashpad/client/simulate_crash.h"
#endif

using namespace BlackConfig;
using namespace BlackMisc;
using namespace BlackMisc::Db;
using namespace BlackMisc::Network;
using namespace BlackMisc::Aviation;
using namespace BlackMisc::Simulation;
using namespace BlackMisc::Weather;
using namespace BlackCore;
using namespace BlackCore::Context;
using namespace BlackCore::Vatsim;
using namespace BlackCore::Data;
using namespace BlackCore::Db;
using namespace crashpad;

BlackCore::CApplication *sApp = nullptr; // set by constructor

//! \private
static const QString &swiftDataRoot()
{
    static const QString path = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/org.swift-project/";
    return path;
}

namespace BlackCore
{
    CApplication::CApplication(CApplicationInfo::Application application, bool init) :
        CApplication(executable(), application, init)
    { }

    CApplication::CApplication(const QString &applicationName, CApplicationInfo::Application application, bool init) :
        m_accessManager(new QNetworkAccessManager(this)),
        m_application(application), m_cookieManager( {}, this), m_applicationName(applicationName), m_coreFacadeConfig(CCoreFacadeConfig::allEmpty())
    {
        Q_ASSERT_X(!sApp, Q_FUNC_INFO, "already initialized");
        Q_ASSERT_X(QCoreApplication::instance(), Q_FUNC_INFO, "no application object");

        // init skipped when called from CGuiApplication
        if (init)
        {
            this->init(true);
        }
    }

    void CApplication::init(bool withMetadata)
    {
        if (!sApp)
        {
            if (withMetadata) { CApplication::registerMetadata(); }
            QCoreApplication::setApplicationName(this->m_applicationName);
            QCoreApplication::setApplicationVersion(CBuildConfig::getVersionString());
            this->setObjectName(this->m_applicationName);
            const QString executable = QFileInfo(QCoreApplication::applicationFilePath()).fileName();
            if (executable.startsWith("test"))
            {
                this->m_unitTest = true;
                const QString tempPath(this->getTemporaryDirectory());
                BlackMisc::setMockCacheRootDirectory(tempPath);
            }
            this->m_alreadyRunning = CApplication::getRunningApplications().containsApplication(CApplication::getSwiftApplication());
            this->initParser();
            this->initLogging();

            //
            // cmd line arguments not yet parsed here
            //

            // Translations
            QTranslator translator;
            if (translator.load("blackmisc_i18n_de", ":blackmisc/translations/")) { CLogMessage(this).debug() << "Translator loaded"; }
            QCoreApplication::instance()->installTranslator(&translator);

            // Init network
            Q_ASSERT_X(m_accessManager, Q_FUNC_INFO, "Need QAM");
            this->m_cookieManager.setParent(this->m_accessManager);
            this->m_accessManager->setCookieJar(&this->m_cookieManager);
            connect(this->m_accessManager, &QNetworkAccessManager::networkAccessibleChanged, this, &CApplication::ps_networkAccessibleChanged);
            CLogMessage::preformatted(CNetworkUtils::createNetworkReport(this->m_accessManager));

            // global setup
            sApp = this;
            this->m_setupReader.reset(new CSetupReader(this));
            connect(this->m_setupReader.data(), &CSetupReader::setupHandlingCompleted, this, &CApplication::ps_setupHandlingCompleted);
            connect(this->m_setupReader.data(), &CSetupReader::distributionInfoAvailable, this, &CApplication::distributionInfoAvailable);

            this->m_parser.addOptions(this->m_setupReader->getCmdLineOptions()); // add options from reader

            // startup done
            connect(this, &CApplication::startUpCompleted, this, &CApplication::ps_startupCompleted);

            // notify when app goes down
            connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, this, &CApplication::gracefulShutdown);
        }
    }

    bool CApplication::registerAsRunning()
    {
        CApplicationInfoList apps = CApplication::getRunningApplications();
        const CApplicationInfo myself = CApplication::instance()->getApplicationInfo();
        if (!apps.contains(myself)) { apps.insert(myself); }
        const bool ok = CFileUtils::writeStringToLockedFile(apps.toJsonString(), CFileUtils::appendFilePaths(swiftDataRoot(), "apps.json"));
        if (!ok) { CLogMessage(static_cast<CApplication *>(nullptr)).error("Failed to write to application list file"); }
        return ok;
    }

    int CApplication::exec()
    {
        Q_ASSERT_X(instance(), Q_FUNC_INFO, "missing application");
        CApplication::registerAsRunning();
        return QCoreApplication::exec();
    }

    void CApplication::restartApplication()
    {
        this->gracefulShutdown();
        const QString prg = QCoreApplication::applicationFilePath();
        const QStringList args = CApplication::arguments();
        QProcess::startDetached(prg, args);
        this->exit(0);
    }

    CApplication::~CApplication()
    {
        this->gracefulShutdown();
    }

    CApplicationInfo CApplication::getApplicationInfo() const
    {
        CApplicationInfo::ApplicationMode mode;
        if (isRunningInDeveloperEnvironment()) { mode |= CApplicationInfo::Developer; }
        if (CBuildConfig::isDevBranch()) { mode |= CApplicationInfo::BetaTest; }
        return { CApplication::getSwiftApplication(), mode, QCoreApplication::applicationFilePath(), CBuildConfig::getVersionString(), CProcessInfo::currentProcess() };
    }

    CApplicationInfoList CApplication::getRunningApplications()
    {
        CApplicationInfoList apps;
        apps.convertFromJsonNoThrow(CFileUtils::readLockedFileToString(swiftDataRoot() + "apps.json"), {}, {});
        apps.removeIf([](const CApplicationInfo & info) { return !info.processInfo().exists(); });
        return apps;
    }

    bool CApplication::isApplicationRunning(CApplicationInfo::Application application)
    {
        const CApplicationInfoList running = CApplication::getRunningApplications();
        return running.containsApplication(application);
    }

    bool CApplication::isAlreadyRunning() const
    {
        return getRunningApplications().containsBy([this](const CApplicationInfo & info) { return info.application() == getSwiftApplication(); });
    }

    bool CApplication::isShuttingDown() const
    {
        return m_shutdown;
    }

    const QString &CApplication::getApplicationNameAndVersion() const
    {
        static const QString s(QCoreApplication::instance()->applicationName() + " " + CBuildConfig::getVersionString());
        return s;
    }

    const QString &CApplication::getApplicationNameVersionBetaDev() const
    {
        static const QString s(QCoreApplication::instance()->applicationName() + " " + this->versionStringDetailed());
        return s;
    }

    void CApplication::setSingleApplication(bool singleApplication)
    {
        this->m_singleApplication = singleApplication;
    }

    CApplicationInfo::Application CApplication::getSwiftApplication() const
    {
        if (this->isUnitTest()) { return CApplicationInfo::UnitTest; }
        if (this->m_application != CApplicationInfo::Unknown) { return this->m_application; }

        // if not set, guess
        BLACK_VERIFY_X(false, Q_FUNC_INFO, "Missing application");
        const QString a(QCoreApplication::instance()->applicationName().toLower());
        if (a.contains("core"))     { return CApplicationInfo::PilotClientCore; }
        if (a.contains("launcher")) { return CApplicationInfo::Laucher; }
        if (a.contains("gui"))      { return CApplicationInfo::PilotClientGui; }
        if (a.contains("test"))     { return CApplicationInfo::UnitTest; }
        if (a.contains("sample"))   { return CApplicationInfo::Sample; }
        if (a.contains("data") || a.contains("mapping")) { return CApplicationInfo::MappingTool; }
        return CApplicationInfo::Unknown;
    }

    QString CApplication::getExecutableForApplication(CApplicationInfo::Application application) const
    {
        QString search;
        switch (application)
        {
        case CApplicationInfo::PilotClientCore: search = "core"; break;
        case CApplicationInfo::Laucher: search = "launcher"; break;
        case CApplicationInfo::MappingTool: search = "data"; break;
        case CApplicationInfo::PilotClientGui: search = "gui"; break;
        default:
            break;
        }
        if (search.isEmpty()) return "";
        for (const QString &executable : CFileUtils::getSwiftExecutables())
        {
            if (!executable.contains("swift", Qt::CaseInsensitive)) { continue; }
            if (executable.contains(search, Qt::CaseInsensitive)) { return executable; }
        }
        return "";
    }

    bool CApplication::startLauncher()
    {
        static const QString launcher = CApplication::getExecutableForApplication(CApplicationInfo::Application::Laucher);
        if (launcher.isEmpty() || CApplication::isApplicationRunning(CApplicationInfo::Laucher)) { return false; }
        return QProcess::startDetached(launcher);
    }

    bool CApplication::isUnitTest() const
    {
        return this->m_unitTest;
    }

    CGlobalSetup CApplication::getGlobalSetup() const
    {
        if (this->m_shutdown) { return CGlobalSetup(); }
        const CSetupReader *r = this->m_setupReader.data();
        if (!r) { return CGlobalSetup(); }
        return r->getSetup();
    }

    CDistributionList CApplication::getDistributionInfo() const
    {
        if (this->m_shutdown) { return CDistributionList(); }
        const CSetupReader *r = this->m_setupReader.data();
        if (!r) { return CDistributionList(); }
        return r->getDistributionInfo();
    }

    bool CApplication::start()
    {
        this->m_started = false; // reset

        // parse if needed, parsing contains its own error handling
        if (!this->m_parsed)
        {
            const bool s = this->parseAndStartupCheck();
            if (!s) { return false; }
        }

        // parsing itself is done
        CStatusMessageList msgs;
        do
        {
            // clear cache?
            if (this->isSet(this->m_cmdClearCache))
            {
                const QStringList files(CApplication::clearCaches());
                msgs.push_back(
                    CLogMessage(this).debug() << "Cleared cache, " << files.size() << " files"
                );
            }

            // crashpad dump
            if (this->isSet(this->m_cmdTestCrashpad))
            {
                QTimer::singleShot(10 * 1000, [ = ]
                {
                #ifdef BLACK_USE_CRASHPAD
                    CRASHPAD_SIMULATE_CRASH();
                #else
                    CLogMessage(this).warning("This compiler or platform does not support crashpad. Cannot simulate crash dump!");
                #endif
                });
            }

            // load setup
            if (this->m_startSetupReader && !this->m_setupReader->isSetupAvailable())
            {
                msgs = this->requestReloadOfSetupAndVersion();
                if (msgs.isFailure()) { break; }
                if (msgs.isSuccess()) { msgs.push_back(this->waitForSetup()); }
            }

            // start hookin
            msgs.push_back(this->startHookIn());
            if (msgs.isFailure()) { break; }

            // trigger loading and saving of settings in appropriate scenarios
            if (this->m_coreFacadeConfig.getModeApplication() != CCoreFacadeConfig::Remote)
            {
                // facade running here locally
                msgs.push_back(CSettingsCache::instance()->loadFromStore());
                if (msgs.isFailure()) { break; }

                // Settings are distributed via DBus. So only one application is responsible for saving. `enableLocalSave()` means
                // "this is the application responsible for saving". If swiftgui requests a setting to be saved, it is sent to swiftcore and saved by swiftcore.
                CSettingsCache::instance()->enableLocalSave();

                // From this moment on, we have settings, so enable crash handler.
                msgs.push_back(this->initCrashHandler());
            }
        }
        while (false);

        // terminate with failures, otherwise log messages
        if (msgs.isFailure())
        {
            this->cmdLineErrorMessage(msgs);
            return false;
        }
        else if (!msgs.isEmpty())
        {
            CLogMessage::preformatted(msgs);
        }

        this->m_started = this->m_startSetupReader; // only if requested it will be started
        return this->m_started;
    }

    CStatusMessageList CApplication::waitForSetup()
    {
        if (!this->m_setupReader) { return CStatusMessage(this).error("No setup reader"); }
        CEventLoop::processEventsUntil(this, &CApplication::setupHandlingCompleted, CNetworkUtils::getLongTimeoutMs(), [this]
        {
            return this->m_setupReader->isSetupAvailable();
        });

        // setup handling completed with success or failure, or we run into time out
        if (this->m_setupReader->isSetupAvailable()) { return CStatusMessage(this).info("Setup available"); }
        CStatusMessageList msgs(CStatusMessage(this).error("Setup not available, setup reading failed or timed out."));
        if (this->m_setupReader->getLastSetupReadErrorMessages().hasErrorMessages())
        {
            msgs.push_back(this->m_setupReader->getLastSetupReadErrorMessages());
        }
        if (this->m_setupReader->hasCmdLineBootstrapUrl())
        {
            msgs.push_back(CStatusMessage(this).info("Check cmd line argument '%1'") << this->m_setupReader->getCmdLineBootstrapUrl());
        }
        return msgs;
    }

    bool CApplication::isSetupAvailable() const
    {
        if (this->m_shutdown || !this->m_setupReader) { return false; }
        return this->m_setupReader->isSetupAvailable();
    }

    CStatusMessageList CApplication::requestReloadOfSetupAndVersion()
    {
        if (!this->m_shutdown)
        {
            Q_ASSERT_X(this->m_setupReader, Q_FUNC_INFO, "Missing reader");
            Q_ASSERT_X(this->m_parsed, Q_FUNC_INFO, "Not yet parsed");
            return this->m_setupReader->asyncLoad();
        }
        else
        {
            return CStatusMessage(this).error("No reader for setup/version");
        }
    }

    bool CApplication::hasWebDataServices() const
    {
        if (this->isShuttingDown()) { return false; } // service will not survive for long
        return this->m_webDataServices;
    }

    CWebDataServices *CApplication::getWebDataServices() const
    {
        // use hasWebDataServices() to test if services are available

        Q_ASSERT_X(this->m_webDataServices, Q_FUNC_INFO, "Missing web data services, use hasWebDataServices to test if existing");
        return this->m_webDataServices.data();
    }

    bool CApplication::isApplicationThread() const
    {
        return CThreadUtils::isCurrentThreadApplicationThread();
    }

    const QString &CApplication::versionStringDetailed() const
    {
        if (isRunningInDeveloperEnvironment() && CBuildConfig::isDevBranch())
        {
            static const QString s(CBuildConfig::getVersionString() + " [dev,DEV]");
            return s;
        }
        if (isRunningInDeveloperEnvironment())
        {
            static const QString s(CBuildConfig::getVersionString() + " [dev]");
            return s;
        }
        if (CBuildConfig::isDevBranch())
        {
            static const QString s(CBuildConfig::getVersionString() + " [DEV]");
            return s;
        }
        return CBuildConfig::getVersionString();
    }

    const QString &CApplication::swiftVersionString() const
    {
        static const QString s(QString("swift %1").arg(versionStringDetailed()));
        return s;
    }

    const char *CApplication::swiftVersionChar()
    {
        static const QByteArray a(swiftVersionString().toUtf8());
        return a.constData();
    }

    bool CApplication::initIsRunningInDeveloperEnvironment() const
    {
        if (!CBuildConfig::canRunInDeveloperEnvironment()) { return false; }
        if (this->m_unitTest) { return true; }
        if (this->isSet(this->m_cmdDevelopment)) { return true; }
        if (this->isSetupAvailable())
        {
            // assume value from setup
            return this->getGlobalSetup().isDevelopment();
        }
        return false;
    }

    void CApplication::setSignalStartupAutomatically(bool enabled)
    {
        this->m_signalStartup = enabled;
    }

    QString CApplication::getEnvironmentInfoString(const QString &separator) const
    {
        const QString env =
            QLatin1String("Beta: ") %
            boolToYesNo(CBuildConfig::isDevBranch()) %
            QLatin1String(" dev.env,: ") %
            boolToYesNo(isRunningInDeveloperEnvironment()) %
            separator %
            QLatin1String("Windows: ") %
            boolToYesNo(CBuildConfig::isRunningOnWindowsNtPlatform());
        return env;
    }

    bool CApplication::hasUnsavedSettings() const
    {
        return !this->getAllUnsavedSettings().isEmpty();
    }

    void CApplication::setSettingsAutoSave(bool autoSave)
    {
        this->m_autoSaveSettings = autoSave;
    }

    QStringList CApplication::getAllUnsavedSettings() const
    {
        if (this->supportsContexts())
        {
            return this->getIContextApplication()->getUnsavedSettingsKeys();
        }
        return {};
    }

    CStatusMessage CApplication::saveSettingsByKey(const QStringList &keys)
    {
        if (keys.isEmpty()) { return CStatusMessage(); }
        if (this->supportsContexts())
        {
            return this->getIContextApplication()->saveSettingsByKey(keys);
        }
        return CSettingsCache::instance()->saveToStore(keys);
    }

    QString CApplication::getTemporaryDirectory() const
    {
        static QTemporaryDir tempDir;
        if (tempDir.isValid()) { return tempDir.path(); }
        return QDir::tempPath();
    }

    QString CApplication::getInfoString(const QString &separator) const
    {
        const QString str =
            CBuildConfig::getVersionString() %
            QLatin1Char(' ') % (CBuildConfig::isReleaseBuild() ? QLatin1String("Release build") : QLatin1String("Debug build")) %
            separator %
            getEnvironmentInfoString(separator) %
            separator %
            CBuildConfig::compiledWithInfo(false);
        return str;
    }

    QNetworkReply *CApplication::getFromNetwork(const CUrl &url, const CSlot<void(QNetworkReply *)> &callback, int maxRedirects)
    {
        return httpRequestImpl(url.toNetworkRequest(), callback, maxRedirects, [ ](QNetworkAccessManager & nam, const QNetworkRequest & request) { return nam.get(request); });
    }

    QNetworkReply *CApplication::getFromNetwork(const QNetworkRequest &request, const CSlot<void(QNetworkReply *)> &callback, int maxRedirects)
    {
        return httpRequestImpl(request, callback, maxRedirects, [ ](QNetworkAccessManager & nam, const QNetworkRequest & request) { return nam.get(request); });
    }

    QNetworkReply *CApplication::postToNetwork(const QNetworkRequest &request, const QByteArray &data, const CSlot<void(QNetworkReply *)> &callback)
    {
        return httpRequestImpl(request, callback, -1, [ data ](QNetworkAccessManager & nam, const QNetworkRequest & request) { return nam.post(request, data); });
    }

    QNetworkReply *CApplication::postToNetwork(const QNetworkRequest &request, QHttpMultiPart *multiPart, const CSlot<void(QNetworkReply *)> &callback)
    {
        if (!this->isNetworkAccessible()) { return nullptr; }
        if (QThread::currentThread() != this->m_accessManager->thread())
        {
            multiPart->moveToThread(this->m_accessManager->thread());
        }

        return httpRequestImpl(request, callback, -1, [ this, multiPart ](QNetworkAccessManager & nam, const QNetworkRequest & request)
        {
            QNetworkReply *reply = nam.post(request, multiPart);
            Q_ASSERT(reply);
            multiPart->setParent(reply);
            return reply;
        });
    }

    QNetworkReply *CApplication::headerFromNetwork(const CUrl &url, const CSlot<void (QNetworkReply *)> &callback, int maxRedirects)
    {
        return httpRequestImpl(url.toNetworkRequest(), callback, maxRedirects, [ ](QNetworkAccessManager & nam, const QNetworkRequest & request) { return nam.head(request); });
    }

    QNetworkReply *CApplication::headerFromNetwork(const QNetworkRequest &request, const CSlot<void (QNetworkReply *)> &callback, int maxRedirects)
    {
        return httpRequestImpl(request, callback, maxRedirects, [ ](QNetworkAccessManager & nam, const QNetworkRequest & request) { return nam.head(request); });
    }

    void CApplication::deleteAllCookies()
    {
        this->m_cookieManager.deleteAllCookies();
    }

    bool CApplication::isNetworkAccessible() const
    {
        if (!this->m_accessManager) return false;
        const QNetworkAccessManager::NetworkAccessibility a = this->m_accessManager->networkAccessible();
        if (a == QNetworkAccessManager::Accessible) return true;

        // currently I also accept unknown
        return a == QNetworkAccessManager::UnknownAccessibility;
    }

    bool CApplication::hasSetupReader() const
    {
        // m_startSetupReader set to false, if something wrong with parsing
        return m_setupReader && m_startSetupReader;
    }

    QString CApplication::getLastSuccesfulSetupUrl() const
    {
        if (!this->hasSetupReader()) { return ""; }
        return m_setupReader->getLastSuccessfulSetupUrl();
    }

    QString CApplication::getLastSuccesfulDistributionUrl() const
    {
        if (!this->hasSetupReader()) { return ""; }
        return m_setupReader->getLastSuccessfulDistributionUrl();
    }

    void CApplication::exit(int retcode)
    {
        if (instance())
        {
            instance()->gracefulShutdown();
        }
        // when the event loop is not running, this does nothing
        QCoreApplication::exit(retcode);
    }

    QStringList CApplication::arguments()
    {
        return QCoreApplication::arguments();
    }

    void CApplication::processEventsFor(int milliseconds)
    {
        QEventLoop eventLoop;
        QTimer::singleShot(milliseconds, &eventLoop, &QEventLoop::quit);
        eventLoop.exec();
    }

    CStatusMessageList CApplication::useContexts(const CCoreFacadeConfig &coreConfig)
    {
        Q_ASSERT_X(this->m_parsed, Q_FUNC_INFO, "Call this function after parsing");

        this->m_useContexts = true;
        this->m_coreFacadeConfig = coreConfig;

        // if not yet initialized, init web data services
        if (!this->m_useWebData)
        {
            const CStatusMessageList msgs = this->useWebDataServices(CWebReaderFlags::AllReaders, CDatabaseReaderConfigList::forPilotClient());
            if (msgs.hasErrorMessages()) { return msgs; }
        }
        return this->startCoreFacadeAndWebDataServices(); // will do nothing if setup is not yet loaded
    }

    CStatusMessageList CApplication::useWebDataServices(const CWebReaderFlags::WebReader webReaders, const CDatabaseReaderConfigList &dbReaderConfig)
    {
        Q_ASSERT_X(this->m_webDataServices.isNull(), Q_FUNC_INFO, "Services already started");
        BLACK_VERIFY_X(QSslSocket::supportsSsl(), Q_FUNC_INFO, "No SSL");
        if (!QSslSocket::supportsSsl())
        {
            return CStatusMessage(this).error("No SSL supported, can`t be used");
        }

        this->m_webReadersUsed = webReaders;
        this->m_dbReaderConfig = dbReaderConfig;
        this->m_useWebData = true;
        return this->startWebDataServices();
    }

    CStatusMessageList CApplication::startCoreFacadeAndWebDataServices()
    {
        Q_ASSERT_X(this->m_parsed, Q_FUNC_INFO, "Call this function after parsing");

        if (!this->m_useContexts) { return CStatusMessage(this).error("No need to start core facade"); } // we do not use context, so no need to startup
        if (!this->m_setupReader || !this->m_setupReader->isSetupAvailable()) { return CStatusMessage(this).error("No setup reader or setup available"); }

        Q_ASSERT_X(this->m_coreFacade.isNull(), Q_FUNC_INFO, "Cannot alter facade");
        Q_ASSERT_X(this->m_setupReader, Q_FUNC_INFO, "No facade without setup possible");
        Q_ASSERT_X(this->m_useWebData, Q_FUNC_INFO, "Need web data services");

        this->startWebDataServices();

        const CStatusMessageList msgs(CStatusMessage(this).info("Will start core facade now"));
        this->m_coreFacade.reset(new CCoreFacade(this->m_coreFacadeConfig));
        emit this->coreFacadeStarted();
        return msgs;
    }

    CStatusMessageList CApplication::startWebDataServices()
    {
        Q_ASSERT_X(this->m_parsed, Q_FUNC_INFO, "Call this function after parsing");

        if (!this->m_useWebData) { return CStatusMessage(this).warning("No need to start web data services"); }
        if (!this->m_setupReader || !this->m_setupReader->isSetupAvailable()) { return CStatusMessage(this).error("No setup reader or setup available"); }

        Q_ASSERT_X(this->m_setupReader, Q_FUNC_INFO, "No web data services without setup possible");
        CStatusMessageList msgs;
        if (!this->m_webDataServices)
        {
            msgs.push_back(CStatusMessage(this).info("Will start web data services now"));
            this->m_webDataServices.reset(
                new CWebDataServices(this->m_webReadersUsed, this->m_dbReaderConfig, {}, this)
            );
            emit webDataServicesStarted(true);
        }
        else
        {
            msgs.push_back(CStatusMessage(this).info("Web data services already running"));
        }

        return msgs;
    }

    void CApplication::initLogging()
    {
        CLogHandler::instance()->install(); // make sure we have a log handler!

        // File logger
        this->m_fileLogger.reset(new CFileLogger(executable(), CDirectoryUtils::logDirectory()));
        this->m_fileLogger->changeLogPattern(CLogPattern().withSeverityAtOrAbove(CStatusMessage::SeverityDebug));
    }

    void CApplication::initParser()
    {
        this->m_parser.setSingleDashWordOptionMode(QCommandLineParser::ParseAsLongOptions);
        this->m_parser.setApplicationDescription(m_applicationName);
        this->m_cmdHelp = this->m_parser.addHelpOption();
        this->m_cmdVersion = this->m_parser.addVersionOption();

        // dev. system
        this->m_cmdDevelopment = QCommandLineOption({ "dev", "development" },
                                 QCoreApplication::translate("application", "Dev. system features?"));
        this->addParserOption(this->m_cmdDevelopment);

        // can read a local bootstrap file
        this->m_cmdSharedDir = QCommandLineOption({ "shared", "shareddir" },
                               QCoreApplication::translate("application", "Local shared directory."),
                               "shared");
        this->addParserOption(this->m_cmdSharedDir);

        // reset caches upfront
        this->m_cmdClearCache = QCommandLineOption({ "ccache", "clearcache" },
                                QCoreApplication::translate("application", "Clear (reset) the caches."));
        this->addParserOption(this->m_cmdClearCache);

        // test crashpad upload
        this->m_cmdTestCrashpad = QCommandLineOption({ "testcp", "testcrashpad" },
                                  QCoreApplication::translate("application", "Simulate crashpad situation."));
        this->addParserOption(this->m_cmdTestCrashpad);
    }

    bool CApplication::isSet(const QCommandLineOption &option) const
    {
        return (this->m_parser.isSet(option));
    }

    void CApplication::registerMetadata()
    {
        BlackMisc::registerMetadata();
        BlackCore::registerMetadata();
    }

    QStringList CApplication::clearCaches()
    {
        const QStringList files(CDataCache::instance()->enumerateStore());
        CDataCache::instance()->clearAllValues();
        return files;
    }

    void CApplication::gracefulShutdown()
    {
        if (this->m_shutdown) { return; }
        this->m_shutdown = true;

        // save settings (but only when application was really alive)
        CStatusMessage m;
        if (this->m_parsed)
        {
            if (this->supportsContexts() && this->m_autoSaveSettings)
            {
                // this will eventually also call saveToStore
                m = this->getIContextApplication()->saveSettings();
            }
            else
            {
                m = CSettingsCache::instance()->saveToStore();
            }
            CLogMessage(getLogCategories()).preformatted(m);
        }

        // from here on we really rip appart the application object
        // and it should no longer be used
        sApp = nullptr;
        disconnect(this);

        if (this->supportsContexts())
        {
            // clean up facade
            this->m_coreFacade->gracefulShutdown();
            this->m_coreFacade.reset();
        }

        if (this->m_webDataServices)
        {
            this->m_webDataServices->gracefulShutdown();
            this->m_webDataServices.reset();
        }

        if (this->m_setupReader)
        {
            this->m_setupReader->gracefulShutdown();
            this->m_setupReader.reset();
        }

        this->m_fileLogger->close();
    }

    void CApplication::ps_setupHandlingCompleted(bool available)
    {
        if (available)
        {
            // start follow ups when setup is avaialable
            const CStatusMessageList msgs = this->asyncWebAndContextStart();
            this->m_started = msgs.isSuccess();
        }

        emit this->setupHandlingCompleted(available);

        if (this->m_signalStartup)
        {
            emit this->startUpCompleted(this->m_started);
        }
    }

    void CApplication::ps_startupCompleted()
    {
        // void
    }

    void CApplication::ps_networkAccessibleChanged(QNetworkAccessManager::NetworkAccessibility accessible)
    {
        switch (accessible)
        {
        case QNetworkAccessManager::Accessible:
            this->m_accessManager->setNetworkAccessible(accessible); // for some reasons the queried value still is unknown
            CLogMessage(this).info("Network is accessible");
            break;
        case QNetworkAccessManager::NotAccessible:
            CLogMessage(this).error("Network not accessible");
            break;
        default:
            CLogMessage(this).warning("Network accessibility unknown");
            break;
        }
    }

    CStatusMessageList CApplication::asyncWebAndContextStart()
    {
        if (this->m_started) { return CStatusMessage(this).info("Already started "); }

        // follow up startups
        CStatusMessageList msgs = this->startWebDataServices();
        if (msgs.isFailure()) return msgs;
        msgs.push_back(this->startCoreFacadeAndWebDataServices());
        return msgs;
    }

    void CApplication::severeStartupProblem(const CStatusMessage &message)
    {
        CLogMessage::preformatted(message);
        this->cmdLineErrorMessage(message.getMessage());
        this->exit(EXIT_FAILURE);

        // if I get here the event loop was not yet running
        std::exit(EXIT_FAILURE);
    }

    CApplication *BlackCore::CApplication::instance()
    {
        return sApp;
    }

    const QString &CApplication::executable()
    {
        static const QString e(QFileInfo(QCoreApplication::applicationFilePath()).completeBaseName());
        return e;
    }

    const BlackMisc::CLogCategoryList &CApplication::getLogCategories()
    {
        static const CLogCategoryList l({ CLogCategory("swift.application"), CLogCategory("swift." + executable())});
        return l;
    }

    // ---------------------------------------------------------------------------------
    // Parsing
    // ---------------------------------------------------------------------------------

    bool CApplication::addParserOption(const QCommandLineOption &option)
    {
        return this->m_parser.addOption(option);
    }

    bool CApplication::addParserOptions(const QList<QCommandLineOption> &options)
    {
        return this->m_parser.addOptions(options);
    }

    void CApplication::addDBusAddressOption()
    {
        this->m_cmdDBusAddress = QCommandLineOption({ "dbus", "dbusaddress" },
                                 QCoreApplication::translate("application", "DBus address (session, system, P2P IP e.g. 192.168.23.5)"),
                                 "dbusaddress");
        this->addParserOption(this->m_cmdDBusAddress);
    }

    void CApplication::addVatlibOptions()
    {
        this->addParserOptions(CNetworkVatlib::getCmdLineOptions());
    }

    QString CApplication::getCmdDBusAddressValue() const
    {
        if (this->isParserOptionSet(this->m_cmdDBusAddress))
        {
            const QString v(this->getParserValue(m_cmdDBusAddress));
            const QString dBusAddress(CDBusServer::normalizeAddress(v));
            return dBusAddress;
        }
        else
        {
            return "";
        }
    }

    QString CApplication::getCmdSwiftPrivateSharedDir() const
    {
        return this->m_parser.value(this->m_cmdSharedDir);
    }

    bool CApplication::isParserOptionSet(const QString &option) const
    {
        return this->m_parser.isSet(option);
    }

    bool CApplication::isInstallerOptionSet() const
    {
        return this->isParserOptionSet("installer");
    }

    bool CApplication::isParserOptionSet(const QCommandLineOption &option) const
    {
        return this->m_parser.isSet(option);
    }

    QString CApplication::getParserValue(const QString &option) const
    {
        return this->m_parser.value(option).trimmed();
    }

    QString CApplication::getParserValue(const QCommandLineOption &option) const
    {
        return this->m_parser.value(option).trimmed();
    }

    bool CApplication::parseAndStartupCheck()
    {
        if (this->m_parsed) { return m_parsed; } // already done

        // checks
        if (CBuildConfig::isLifetimeExpired())
        {
            this->cmdLineErrorMessage("Program exired " + CBuildConfig::getEol().toString());
            return false;
        }

        const QStringList verifyErrors = CDirectoryUtils::verifyRuntimeDirectoriesAndFiles();
        if (!verifyErrors.isEmpty())
        {
            this->cmdLineErrorMessage("Missing runtime directories/files: " + verifyErrors.join(", "));
            return false;
        }

        if (this->m_singleApplication && this->m_alreadyRunning)
        {
            this->cmdLineErrorMessage("Program must only run once");
            return false;
        }

        // we call parse because we also want to display a GUI error message when applicable
        const QStringList args(QCoreApplication::instance()->arguments());
        if (!this->m_parser.parse(args))
        {
            this->cmdLineErrorMessage(this->m_parser.errorText());
            return false;
        }

        // help/version
        if (this->m_parser.isSet(this->m_cmdHelp))
        {
            // Important: parser help will already stop application
            this->cmdLineHelpMessage();
            return false;
        }
        if (this->m_parser.isSet(this->m_cmdVersion))
        {
            // Important: version will already stop application
            this->cmdLineVersionMessage();
            return false;
        }

        // dev.
        this->m_devEnv = this->initIsRunningInDeveloperEnvironment();

        // Hookin, other parsing
        if (!this->parsingHookIn()) { return false; }

        // setup reader
        this->m_startSetupReader = this->m_setupReader->parseCmdLineArguments();
        this->m_parsed = true;
        return true;
    }

    bool CApplication::cmdLineErrorMessage(const QString &errorMessage, bool retry) const
    {
        Q_UNUSED(retry); // only works with UI version
        fputs(qPrintable(errorMessage), stderr);
        fputs("\n\n", stderr);
        fputs(qPrintable(this->m_parser.helpText()), stderr);
        return false;
    }

    bool CApplication::cmdLineErrorMessage(const CStatusMessageList &msgs, bool retry) const
    {
        Q_UNUSED(retry); // only works with UI version
        if (msgs.isEmpty()) { return false; }
        if (!msgs.hasErrorMessages())  { return false; }
        CApplication::cmdLineErrorMessage(
            msgs.toFormattedQString(true)
        );
        return false;
    }

    void CApplication::cmdLineHelpMessage()
    {
        this->m_parser.showHelp(); // terminates
        Q_UNREACHABLE();
    }

    void CApplication::cmdLineVersionMessage() const
    {
        printf("%s %s\n", qPrintable(QCoreApplication::applicationName()), qPrintable(QCoreApplication::applicationVersion()));
    }

    // ---------------------------------------------------------------------------------
    // Contexts
    // ---------------------------------------------------------------------------------

    bool CApplication::supportsContexts() const
    {
        if (this->m_shutdown) { return false; }
        if (this->m_coreFacade.isNull()) { return false; }
        if (!this->m_coreFacade->getIContextApplication()) { return false; }
        return (!this->m_coreFacade->getIContextApplication()->isEmptyObject());
    }

    const IContextNetwork *CApplication::getIContextNetwork() const
    {
        if (!supportsContexts()) { return nullptr; }
        return this->m_coreFacade->getIContextNetwork();
    }

    const IContextAudio *CApplication::getIContextAudio() const
    {
        if (!supportsContexts()) { return nullptr; }
        return this->m_coreFacade->getIContextAudio();
    }

    const IContextApplication *CApplication::getIContextApplication() const
    {
        if (!supportsContexts()) { return nullptr; }
        return this->m_coreFacade->getIContextApplication();
    }

    const IContextOwnAircraft *CApplication::getIContextOwnAircraft() const
    {
        if (!supportsContexts()) { return nullptr; }
        return this->m_coreFacade->getIContextOwnAircraft();
    }

    const IContextSimulator *CApplication::getIContextSimulator() const
    {
        if (!supportsContexts()) { return nullptr; }
        return this->m_coreFacade->getIContextSimulator();
    }

    IContextNetwork *CApplication::getIContextNetwork()
    {
        if (!supportsContexts()) { return nullptr; }
        return this->m_coreFacade->getIContextNetwork();
    }

    IContextAudio *CApplication::getIContextAudio()
    {
        if (!supportsContexts()) { return nullptr; }
        return this->m_coreFacade->getIContextAudio();
    }

    IContextApplication *CApplication::getIContextApplication()
    {
        if (!supportsContexts()) { return nullptr; }
        return this->m_coreFacade->getIContextApplication();
    }

    IContextOwnAircraft *CApplication::getIContextOwnAircraft()
    {
        if (!supportsContexts()) { return nullptr; }
        return this->m_coreFacade->getIContextOwnAircraft();
    }

    IContextSimulator *CApplication::getIContextSimulator()
    {
        if (!supportsContexts()) { return nullptr; }
        return this->m_coreFacade->getIContextSimulator();
    }

    // ---------------------------------------------------------------------------------
    // Setup
    // ---------------------------------------------------------------------------------

    CUrlList CApplication::getVatsimMetarUrls() const
    {
        if (this->m_shutdown) { return CUrlList(); }
        if (this->m_webDataServices)
        {
            const CUrlList urls(this->m_webDataServices->getVatsimMetarUrls());
            if (!urls.empty()) { return urls; }
        }
        if (this->m_setupReader)
        {
            return this->m_setupReader->getSetup().getVatsimMetarsUrls();
        }
        return CUrlList();
    }

    CUrlList CApplication::getVatsimDataFileUrls() const
    {
        if (this->m_shutdown) { return CUrlList(); }
        if (this->m_webDataServices)
        {
            const CUrlList urls(this->m_webDataServices->getVatsimDataFileUrls());
            if (!urls.empty()) { return urls; }
        }
        if (this->m_setupReader)
        {
            return this->m_setupReader->getSetup().getVatsimDataFileUrls();
        }
        return CUrlList();
    }

#ifdef BLACK_USE_CRASHPAD
    base::FilePath qstringToFilePath(const QString &str)
    {
#   ifdef Q_OS_WIN
        return base::FilePath(str.toStdWString());
#   else
        return base::FilePath(str.toStdString());
#   endif
    }
#endif

    BlackMisc::CStatusMessageList CApplication::initCrashHandler()
    {
#ifdef BLACK_USE_CRASHPAD
        // No crash handling for unit tests
        if (isUnitTest()) { return CStatusMessage(this).info("No crash handler for unit tests"); }

        static const QString crashpadHandler(CBuildConfig::isRunningOnWindowsNtPlatform() ? "swift_crashpad_handler.exe" : "swift_crashpad_handler");
        static const QString handler = CFileUtils::appendFilePaths(CDirectoryUtils::binDirectory(), crashpadHandler);
        static const QString crashpadPath = CDirectoryUtils::crashpadDirectory();
        static const QString database = CFileUtils::appendFilePaths(crashpadPath, "/database");
        static const QString metrics = CFileUtils::appendFilePaths(crashpadPath, "/metrics");

        if (!QFileInfo::exists(handler))
        {
            return CStatusMessage(this).warning("%1 not found. Cannot init crash handler!") << handler;
        }

        const CUrl serverUrl = this->getGlobalSetup().getCrashReportServerUrl();
        std::map<std::string, std::string> annotations;

        // Caliper (mini-breakpad-server) annotations
        annotations["prod"] = executable().toStdString();
        annotations["ver"] = CBuildConfig::getVersionString().toStdString();

        QDir().mkpath(database);
        m_crashReportDatabase = CrashReportDatabase::Initialize(qstringToFilePath(database));
        auto settings = m_crashReportDatabase->GetSettings();
        settings->SetUploadsEnabled(CBuildConfig::isReleaseBuild() && m_crashDumpUploadEnabled.getThreadLocal());
        m_crashpadClient = std::make_unique<CrashpadClient>();
        m_crashpadClient->StartHandler(qstringToFilePath(handler), qstringToFilePath(database), qstringToFilePath(metrics),
                                       serverUrl.getFullUrl().toStdString(), annotations, {}, false, true);
        return CStatusMessage(this).info("Using crash handler");
#else
        return CStatusMessage(this).info("Not using crash handler");
#endif
    }

    void CApplication::crashDumpUploadEnabledChanged()
    {
#ifdef BLACK_USE_CRASHPAD
        if (!m_crashReportDatabase) { return; }
        auto settings = m_crashReportDatabase->GetSettings();
        settings->SetUploadsEnabled(CBuildConfig::isReleaseBuild() && m_crashDumpUploadEnabled.getThreadLocal());
#endif
    }

    QNetworkReply *CApplication::httpRequestImpl(const QNetworkRequest &request, const BlackMisc::CSlot<void (QNetworkReply *)> &callback, int maxRedirects, std::function<QNetworkReply *(QNetworkAccessManager &, const QNetworkRequest &)> requestOrPostMethod)
    {
        if (this->m_shutdown) { return nullptr; }
        if (!this->isNetworkAccessible()) { return nullptr; }
        QWriteLocker locker(&m_accessManagerLock);
        Q_ASSERT_X(QCoreApplication::instance()->thread() == m_accessManager->thread(), Q_FUNC_INFO, "Network manager supposed to be in main thread");
        if (QThread::currentThread() != this->m_accessManager->thread())
        {
            QTimer::singleShot(0, this, std::bind(&CApplication::httpRequestImpl, this, request, callback, maxRedirects, requestOrPostMethod));
            return nullptr; // not yet started
        }

        Q_ASSERT_X(QThread::currentThread() == m_accessManager->thread(), Q_FUNC_INFO, "Network manager thread mismatch");
        QNetworkRequest copiedRequest(request); // no QObject
        CNetworkUtils::ignoreSslVerification(copiedRequest);
        CNetworkUtils::setSwiftUserAgent(copiedRequest);

        // If URL is one of the shared urls, add swift client SSL certificate
        CNetworkUtils::setSwiftClientSslCertificate(copiedRequest, getGlobalSetup().getSwiftSharedUrls());

        QNetworkReply *reply = requestOrPostMethod(*this->m_accessManager, copiedRequest);
        reply->setProperty("started", QVariant(QDateTime::currentMSecsSinceEpoch()));
        if (callback)
        {
            connect(reply, &QNetworkReply::finished, callback.object(), [ = ]
            {
                // Called when finished!
                // QNetworkRequest::FollowRedirectsAttribute would allow auto redirect, but we use our approach as it gives us better control
                // \fixme: Check again on Qt 5.9: Added redirects policy to QNetworkAccessManager (ManualRedirectsPolicy, NoLessSafeRedirectsPolicy, SameOriginRedirectsPolicy, UserVerifiedRedirectsPolicy)
                const bool isRedirect = CNetworkUtils::isHttpStatusRedirect(reply);
                if (isRedirect && maxRedirects > 0)
                {
                    const QUrl redirectUrl = CNetworkUtils::getHttpRedirectUrl(reply);
                    if (!redirectUrl.isEmpty())
                    {
                        QNetworkRequest redirectRequest(redirectUrl);
                        const int redirectsLeft = maxRedirects - 1;
                        QTimer::singleShot(0, this, std::bind(&CApplication::httpRequestImpl, this, redirectRequest, callback, redirectsLeft, requestOrPostMethod));
                        return;
                    }
                }
                // called when there are no more callbacks
                callback(reply);

            }, Qt::QueuedConnection);
        }
        return reply;
    }
} // ns

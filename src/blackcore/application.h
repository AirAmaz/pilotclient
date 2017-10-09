/* Copyright (C) 2016
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file

#ifndef BLACKCORE_APPLICATION_H
#define BLACKCORE_APPLICATION_H

#include "blackcore/blackcoreexport.h"
#include "blackcore/cookiemanager.h"
#include "blackcore/corefacadeconfig.h"
#include "blackcore/data/globalsetup.h"
#include "blackcore/db/databasereaderconfig.h"
#include "blackcore/application/applicationsettings.h"
#include "blackcore/webreaderflags.h"
#include "blackmisc/db/distributionlist.h"
#include "blackmisc/network/urllist.h"
#include "blackmisc/network/networkutils.h"
#include "blackmisc/slot.h"
#include "blackmisc/applicationinfolist.h"
#include "blackmisc/statusmessagelist.h"

#include <QByteArray>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QList>
#include <QNetworkAccessManager>
#include <QObject>
#include <QReadWriteLock>
#include <QScopedPointer>
#include <QString>
#include <QStringList>
#include <atomic>
#include <functional>

#if defined(Q_CC_MSVC) || defined(Q_OS_OSX) // Crashpad only supported on MSVC and MacOS/X
#define BLACK_USE_CRASHPAD
#endif

class QHttpMultiPart;
class QNetworkReply;
class QNetworkRequest;

namespace BlackMisc
{
    class CFileLogger;
    class CLogCategoryList;
}

namespace crashpad
{
    class CrashpadClient;
    class CrashReportDatabase;
}

namespace BlackCore
{
    class CCoreFacade;
    class CSetupReader;
    class CWebDataServices;
    namespace Context
    {
        class IContextApplication;
        class IContextAudio;
        class IContextNetwork;
        class IContextOwnAircraft;
        class IContextSimulator;
    }
    namespace Db { class CNetworkWatchdog; }

    /*!
     * Our runtime. Normally one instance is to be initialized at the beginning of main, and thereafter
     * it can be used everywhere via QApplication::instance
     *
     * - A swift standard cmd line parser is part of the application.
     *   Hence cmd arguments can be optained any time / everywhere when required.
     *   Also some standard swift cmd arguments do not need to be re-implemented for each swift application.
     * - The core facade (aka core runtime) is now part of the application. It can be started via cmd line arguments.
     * - Settings are loaded
     * - Setup is loaded (load the so called bootstrap file) to find servers and other resources
     * - Update information (new swift versions etc.) are loaded
     * - If applicable VATSIM status data (where are the VATSIM files?) are loaded
     * - An end of lifetime can be specified, aka time bombing
     *
     * \sa BlackGui::CGuiApplication for the GUI version of application
     */
    class BLACKCORE_EXPORT CApplication : public QObject
    {
        Q_OBJECT

    public:
        //! Similar to \sa QCoreApplication::instance() returns the single instance
        static CApplication *instance();

        //! Own log categories
        static const BlackMisc::CLogCategoryList &getLogCategories();

        //! Constructor
        CApplication(BlackMisc::CApplicationInfo::Application application, bool init = true);

        //! Constructor
        CApplication(const QString &applicationName = executable(), BlackMisc::CApplicationInfo::Application application = BlackMisc::CApplicationInfo::Unknown, bool init = true);

        //! Destructor
        virtual ~CApplication();

        //! Application information
        BlackMisc::CApplicationInfo getApplicationInfo() const;

        //! Information about all running apps (including this one only if exec() has already been called)
        static BlackMisc::CApplicationInfoList getRunningApplications();

        //! Is application running?
        static bool isApplicationRunning(BlackMisc::CApplicationInfo::Application application);

        //! True if this swift application is already running (including different versions)
        bool isAlreadyRunning() const;

        //! Is application shutting down?
        //! \threadsafe
        bool isShuttingDown() const;

        //! Application name and version
        const QString &getApplicationNameAndVersion() const;

        //! Version, name beta and dev info
        const QString &getApplicationNameVersionBetaDev() const;

        //! Force single application (only one instance)
        void setSingleApplication(bool singleApplication);

        //! swift application running
        BlackMisc::CApplicationInfo::Application getSwiftApplication() const;

        //! Executable names for the given applications
        QString getExecutableForApplication(BlackMisc::CApplicationInfo::Application application) const;

        //! Start the launcher
        bool startLauncher();

        //! Unit test?
        bool isUnitTest() const;

        //! Global setup
        //! \threadsafe
        BlackCore::Data::CGlobalSetup getGlobalSetup() const;

        //! Distributions
        //! \threadsafe
        BlackMisc::Db::CDistributionList getDistributionInfo() const;

        //! Delete all cookies from cookie manager
        void deleteAllCookies();

        //! Get the watchdog
        //! \remark mostly for UNIT tests etc, normally not meant to be used directly
        Db::CNetworkWatchdog *getNetworkWatchdog() const;

        //! Allows to mark the DB as "up" or "down"
        //! \see BlackCore::Db::CNetworkWatchdog::setDbAccessibility
        void setSwiftDbAccessibility(bool accessible);

        //! \copydoc BlackCore::Db::CNetworkWatchdog::triggerCheck
        int triggerNetworkChecks();

        //! Is network accessible
        bool isNetworkAccessible() const;

        //! \copydoc BlackCore::Db::CNetworkWatchdog::isInternetAccessible
        bool isInternetAccessible() const;

        //! \copydoc BlackCore::Db::CNetworkWatchdog::isSwiftDbAccessible
        bool isSwiftDbAccessible() const;

        //! \copydoc BlackCore::Db::CNetworkWatchdog::hasWorkingSharedUrl
        bool hasWorkingSharedUrl() const;

        //! \copydoc BlackCore::Db::CNetworkWatchdog::getWorkingSharedUrl
        BlackMisc::Network::CUrl getWorkingSharedUrl() const;

        //! Access to access manager
        //! \remark supposed to be used only in special cases
        const QNetworkAccessManager *getNetworkAccessManager() const { return m_accessManager; }

        //! Last setup URL (successfully read)
        //! \threadsafe
        QString getLastSuccesfulSetupUrl() const;

        //! Last distribution URL (successfully read)
        //! \threadsafe
        QString getLastSuccesfulDistributionUrl() const;

        //! Reload setup and version
        BlackMisc::CStatusMessageList requestReloadOfSetupAndVersion();

        //! Web data services available?
        //! \threadsafe
        bool hasWebDataServices() const;

        //! Get the web data services
        //! \remark use hasWebDataServices to test if services are available
        CWebDataServices *getWebDataServices() const;

        //! Currently running in application thread?
        bool isApplicationThread() const;

        //! String with beta, dev. and version
        const QString &versionStringDetailed() const;

        //! swift info string
        const QString &swiftVersionString() const;

        //! swift info string
        const char *swiftVersionChar();

        //! Running in dev.environment?
        bool isRunningInDeveloperEnvironment() const { return this->m_devEnv; }

        //! Signal startup automatically or individually
        void setSignalStartupAutomatically(bool enabled);

        //! Info string
        QString getEnvironmentInfoString(const QString &separator) const;

        //! To string
        QString getInfoString(const QString &separator) const;

        //! Unsaved settings
        bool hasUnsavedSettings() const;

        //! Automatically and always save settings
        void setSettingsAutoSave(bool autoSave);

        //! All unsaved settings
        QStringList getUnsavedSettingsKeys() const;

        //! Save all settings
        BlackMisc::CStatusMessage saveSettingsByKey(const QStringList &keys);

        //! Directory for temporary files
        QString getTemporaryDirectory() const;

        //! Stop and restart application
        void restartApplication();

        //! Register as running
        //! \note Normally done automatically when CApplication::exec is called
        static bool registerAsRunning();

        //! Run event loop
        static int exec();

        //! Exit application, perform graceful shutdown and exit
        static void exit(int retcode = 0);

        //! Similar to QCoreApplication::arguments
        static QStringList arguments();

        //! Process all events for some time
        //! \remark unlike QCoreApplication::processEvents this will spend at least the given time in the function, using QThread::msleep
        //! \sa BlackMisc::CEventLoop
        static void processEventsFor(int milliseconds);

        //! Clear the caches
        //! \return all cache files
        static QStringList clearCaches();

        // ----------------------- cmd line args / parsing ----------------------------------------

        //! \name cmd line args and parsing of command line options
        //! @{

        //! \copydoc QCommandLineParser::addOption
        bool addParserOption(const QCommandLineOption &option);

        //! \copydoc QCommandLineParser::addOptions
        bool addParserOptions(const QList<QCommandLineOption> &options);

        //! CMD line argument for DBus address
        void addDBusAddressOption();

        //! DBus address from CMD line, otherwise ""
        QString getCmdDBusAddressValue() const;

        //! Add the VATLIB options
        void addVatlibOptions();

        //! Private resource dir for developer's own resource files
        QString getCmdSwiftPrivateSharedDir() const;

        //! Delegates to QCommandLineParser::isSet
        bool isParserOptionSet(const QString &option) const;

        //! Installer called?
        bool isInstallerOptionSet() const;

        //! Delegates to QCommandLineParser::isSet
        bool isParserOptionSet(const QCommandLineOption &option) const;

        //! Delegates to QCommandLineParser::value
        QString getParserValue(const QString &option) const;

        //! Delegates to QCommandLineParser::value
        QString getParserValue(const QCommandLineOption &option) const;

        //! Parses and handles the standard options such as help, version, parse error
        //! \note in some cases (error, version, help) application is terminated during this step
        //! \sa parsingHookIn
        //! \return true means to continue, false to stop
        bool parseAndStartupCheck();

        //! Combined function
        //! \see parseAndStartupCheck
        //! \see synchronizeSetup
        virtual bool parseAndSynchronizeSetup(int timeoutMs = BlackMisc::Network::CNetworkUtils::getLongTimeoutMs());

        //! Display error message
        virtual bool cmdLineErrorMessage(const QString &cmdLineErrorMessage, bool retry = false) const;

        //! Display error message
        virtual bool cmdLineErrorMessage(const BlackMisc::CStatusMessageList &msgs, bool retry = false) const;

        //! Arguments to be passed to another swift appplication
        QStringList inheritedArguments(bool withVatlibArgs = true) const;

        //! cmd line arguments as string
        virtual QString cmdLineArgumentsAsString(bool withExecutable = true);
        //! @}

        // ----------------------- contexts ----------------------------------------

        //! \name Context / core facade related
        //! @{

        //! Supports contexts
        //! \remark checks the real availability of the contexts, so it can happen that we want to use contexts, and they are not yet initialized (false here)
        //! \sa m_useContexts we use or we will use contexts
        bool supportsContexts() const;

        //! The core facade config
        const CCoreFacadeConfig &getCoreFacadeConfig() const { return m_coreFacadeConfig; }

        //! Init the contexts part and start core facade
        //! \sa coreFacadeStarted
        //! \remark requires setup loaded
        BlackMisc::CStatusMessageList useContexts(const CCoreFacadeConfig &coreConfig);

        //! Init web data services and start them
        //! \sa webDataServicesStarted
        //! \remark requires setup loaded
        BlackMisc::CStatusMessageList useWebDataServices(const CWebReaderFlags::WebReader webReader, const BlackCore::Db::CDatabaseReaderConfigList &dbReaderConfig);

        //! Get the facade
        CCoreFacade *getCoreFacade() { return m_coreFacade.data(); }

        //! Get the facade
        const CCoreFacade *getCoreFacade() const { return m_coreFacade.data(); }
        //! @}

        //! \name Direct access to contexts if a CCoreFacade has been initialized
        //! @{
        const Context::IContextNetwork *getIContextNetwork() const;
        const Context::IContextAudio *getIContextAudio() const;
        const Context::IContextApplication *getIContextApplication() const;
        const Context::IContextOwnAircraft *getIContextOwnAircraft() const;
        const Context::IContextSimulator *getIContextSimulator() const;
        Context::IContextNetwork *getIContextNetwork();
        Context::IContextAudio *getIContextAudio();
        Context::IContextApplication *getIContextApplication();
        Context::IContextOwnAircraft *getIContextOwnAircraft();
        Context::IContextSimulator *getIContextSimulator();
        //! @}

        // ----------------------- setup data ---------------------------------
        //! Read and wait for setup
        //! \sa waitForSetup
        BlackMisc::CStatusMessageList synchronizeSetup(int timeoutMs = BlackMisc::Network::CNetworkUtils::getLongTimeoutMs());

        //! Setup reader?
        bool hasSetupReader() const;

        //! Access to setup reader
        //! \remark supposed to be used only in special cases
        BlackCore::CSetupReader *getSetupReader() const;

        //! Setup already synchronized
        bool isSetupAvailable() const;

        //! Consolidated version of METAR URLs, either from CGlobalSetup or CVatsimSetup
        //! \threadsafe
        BlackMisc::Network::CUrlList getVatsimMetarUrls() const;

        //! Consolidated version of data file URLs, either from CGlobalSetup or CVatsimSetup
        //! \threadsafe
        BlackMisc::Network::CUrlList getVatsimDataFileUrls() const;

        //! Graceful shutdown
        virtual void gracefulShutdown();

        //! Start services, if not yet parsed call CApplication::parse
        virtual bool start();

        // ------------------------- network -----------------------------------------------

    public:
        static constexpr int NoRedirects = -1;        //!< network request not allowing redirects
        static constexpr int NoLogRequestId = -1;     //!< network request without logging
        static constexpr int DefaultMaxRedirects = 2; //!< network request, default for max.redirects

        //! Request to get network reply
        //! \threadsafe
        QNetworkReply *getFromNetwork(const BlackMisc::Network::CUrl &url,
                                      const BlackMisc::CSlot<void(QNetworkReply *)> &callback, int maxRedirects = DefaultMaxRedirects);

        //! Request to get network reply, supporting BlackMisc::Network::CUrlLog
        //! \threadsafe
        QNetworkReply *getFromNetwork(const BlackMisc::Network::CUrl &url, int logId,
                                      const BlackMisc::CSlot<void(QNetworkReply *)> &callback, int maxRedirects = DefaultMaxRedirects);

        //! Request to get network reply
        //! \threadsafe
        QNetworkReply *getFromNetwork(const QNetworkRequest &request,
                                      const BlackMisc::CSlot<void(QNetworkReply *)> &callback, int maxRedirects = DefaultMaxRedirects);

        //! Request to get network reply, supporting BlackMisc::Network::CUrlLog
        //! \threadsafe
        QNetworkReply *getFromNetwork(const QNetworkRequest &request, int logId,
                                      const BlackMisc::CSlot<void(QNetworkReply *)> &callback, int maxRedirects = DefaultMaxRedirects);

        //! Post to network
        //! \threadsafe
        QNetworkReply *postToNetwork(const QNetworkRequest &request, int logId, const QByteArray &data,
                                     const BlackMisc::CSlot<void(QNetworkReply *)> &callback);

        //! Post to network
        //! \note This method takes ownership over \c multiPart.
        //! \threadsafe
        QNetworkReply *postToNetwork(const QNetworkRequest &request, int logId, QHttpMultiPart *multiPart,
                                     const BlackMisc::CSlot<void(QNetworkReply *)> &callback);

        //! Request to get network repy using HTTP's HEADER method
        //! \threadsafe
        QNetworkReply *headerFromNetwork(const BlackMisc::Network::CUrl &url,
                                         const BlackMisc::CSlot<void(QNetworkReply *)> &callback, int maxRedirects = NoRedirects);

        //! Request to get network repy using HTTP's HEADER method
        //! \threadsafe
        QNetworkReply *headerFromNetwork(const QNetworkRequest &request,
                                         const BlackMisc::CSlot<void(QNetworkReply *)> &callback, int maxRedirects = NoRedirects);

        //! Download file from network and store it as passed
        //! \threadsafe
        QNetworkReply *downloadFromNetwork(const BlackMisc::Network::CUrl &url, const QString &saveAsFileName,
                                           const BlackMisc::CSlot<void(const BlackMisc::CStatusMessage &)> &callback, int maxRedirects = DefaultMaxRedirects);

    signals:
        //! Setup available (cache, web load, ..) or failed to load setup
        void setupHandlingCompleted(bool success);

        //! Update info available (cache, web load)
        void distributionInfoAvailable(bool success);

        //! Startup has been completed
        //! \remark needs to be triggered by application when it think it is done
        //! \fixme http://doc.qt.io/qt-5/signalsandslots.html#signals recommends signals be only emitted by their own class
        void startUpCompleted(bool success);

        //! Facade started
        void coreFacadeStarted();

        //! Web data services started
        void webDataServicesStarted(bool success);

        //! Internet accessibility changed
        void changedInternetAccessibility(bool accessible);

        //! DB accessibility changed
        void changedSwiftDbAccessibility(bool accessible);

    protected:
        //! Setup read/synchronized
        void setupHandlingIsCompleted(bool available);

        //! Wait for setup data by calling the event loop and waiting until everything is ready
        //! \remark requires parsing upfront
        BlackMisc::CStatusMessageList waitForSetup(int timeoutMs = BlackMisc::Network::CNetworkUtils::getLongTimeoutMs());

        //! Startup completed
        virtual void onStartUpCompleted();

        //! Init class, allows to init from BlackGui::CGuiApplication as well (pseudo virtual)
        void init(bool withMetadata);

        //! Display help message
        virtual void cmdLineHelpMessage();

        //! Display version message
        virtual void cmdLineVersionMessage() const;

        //! Can be used to parse specialized arguments
        virtual bool parsingHookIn() { return true; }

        //! Can be used to start special services
        virtual BlackMisc::CStatusMessageList startHookIn() { return BlackMisc::CStatusMessageList(); }

        //! Flag set or explicitly set to true
        bool isSet(const QCommandLineOption &option) const;

        //! Severe issue during startup, most likely it does not make sense to continue
        //! \note call this here if the parsing stage is over and reaction to a runtime issue is needed
        void severeStartupProblem(const BlackMisc::CStatusMessage &message);

        //! Start the core facade
        //! \note does nothing when setup is not yet loaded
        BlackMisc::CStatusMessageList startCoreFacadeAndWebDataServices();

        //! Start the web data services
        //! \note does nothing when setup is not yet loaded
        BlackMisc::CStatusMessageList startWebDataServices();

        //! executable name
        static const QString &executable();

        //! Register metadata
        static void registerMetadata();

        // cmd parsing
        QCommandLineParser m_parser;                           //!< cmd parser
        QCommandLineOption m_cmdHelp {"help"};                 //!< help option
        QCommandLineOption m_cmdVersion {"version"};           //!< version option
        QCommandLineOption m_cmdDBusAddress {"empty"};         //!< DBus address
        QCommandLineOption m_cmdDevelopment {"dev"};           //!< Development flag
        QCommandLineOption m_cmdSharedDir {"shared"};          //!< Shared directory
        QCommandLineOption m_cmdClearCache {"clearcache"};     //!< Clear cache
        QCommandLineOption m_cmdTestCrashpad {"testcrashpad"}; //!< Test a crasphpad upload
        bool               m_parsed  = false;                  //!< Parsing accomplished?
        bool               m_started = false;                  //!< started with success?
        bool               m_singleApplication = true;         //!< only one instance of that application
        bool               m_alreadyRunning = false;           //!< Application already running

    private:
        //! Problem with network access manager
        void onChangedNetworkAccessibility(QNetworkAccessManager::NetworkAccessibility accessible);

        //! Changed internet accessibility
        void onChangedInternetAccessibility(bool accessible);

        //! Changed swift DB accessibility
        void onChangedSwiftDbAccessibility(bool accessible);

        //! init logging system
        void initLogging();

        //! Init parser
        void initParser();

        //! Dev.environment
        bool initIsRunningInDeveloperEnvironment() const;

        //! Async. start when setup is loaded
        BlackMisc::CStatusMessageList asyncWebAndContextStart();

        //! Implementation for getFromNetwork(), postToNetwork() and headerFromNetwork()
        //! \return QNetworkReply reply will only be returned, if the QNetworkAccessManager is in the same thread
        QNetworkReply *httpRequestImpl(const QNetworkRequest &request,
                                       int logId,
                                       const BlackMisc::CSlot<void(QNetworkReply *)> &callback,
                                       int maxRedirects,
                                       std::function<QNetworkReply *(QNetworkAccessManager &, const QNetworkRequest &)> requestOrPostMethod);

        QNetworkAccessManager                   *m_accessManager = nullptr;   //!< single network access manager
        BlackMisc::CApplicationInfo::Application m_application = BlackMisc::CApplicationInfo::Unknown; //!< Application if specified
        QScopedPointer<CCoreFacade>              m_coreFacade;                //!< core facade if any
        QScopedPointer<CSetupReader>             m_setupReader;               //!< setup reader
        QScopedPointer<CWebDataServices>         m_webDataServices;           //!< web data services
        QScopedPointer<Db::CNetworkWatchdog>     m_networkWatchDog;           //!< checking DB/internet access
        QScopedPointer<BlackMisc::CFileLogger>   m_fileLogger;                //!< file logger
        CCookieManager                           m_cookieManager;             //!< single cookie manager for our access manager
        QString                                  m_applicationName;           //!< application name
        QReadWriteLock                           m_accessManagerLock;         //!< lock to make access manager access threadsafe
        CCoreFacadeConfig                        m_coreFacadeConfig;          //!< Core facade config if any
        CWebReaderFlags::WebReader               m_webReadersUsed;            //!< Readers to be used
        Db::CDatabaseReaderConfigList            m_dbReaderConfig;            //!< Load or used caching?
        std::atomic<bool>                        m_shutdown { false };        //!< is being shutdown?
        bool                                     m_useContexts = false;       //!< use contexts
        bool                                     m_useWebData = false;        //!< use web data
        bool                                     m_signalStartup = true;      //!< signal startup automatically
        bool                                     m_devEnv = false;            //!< dev. environment
        bool                                     m_unitTest = false;          //!< is UNIT test
        bool                                     m_autoSaveSettings = true;   //!< automatically saving all settings

        // -------------- crashpad -----------------
        BlackMisc::CStatusMessageList initCrashHandler();
        void crashDumpUploadEnabledChanged();

#ifdef BLACK_USE_CRASHPAD
        std::unique_ptr<crashpad::CrashpadClient> m_crashpadClient;
        std::unique_ptr<crashpad::CrashReportDatabase> m_crashReportDatabase;
        BlackMisc::CSettingReadOnly<BlackCore::Application::TCrashDumpUploadEnabled> m_crashDumpUploadEnabled { this, &CApplication::crashDumpUploadEnabledChanged };
#endif
    };
} // namespace

//! Single instance of application object
extern BLACKCORE_EXPORT BlackCore::CApplication *sApp;

#endif // guard

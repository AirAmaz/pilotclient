/* Copyright (C) 2015
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "blackcore/application.h"
#include "blackcore/setupreader.h"
#include "blackmisc/verify.h"
#include "blackmisc/compare.h"
#include "blackmisc/fileutils.h"
#include "blackmisc/directoryutils.h"
#include "blackmisc/logcategory.h"
#include "blackmisc/logcategorylist.h"
#include "blackmisc/logmessage.h"
#include "blackmisc/network/networkutils.h"
#include "blackmisc/network/url.h"
#include "blackmisc/statusmessage.h"

#include <QByteArray>
#include <QFile>
#include <QNetworkReply>
#include <QScopedPointer>
#include <QScopedPointerDeleteLater>
#include <QTimer>
#include <QUrl>
#include <QtGlobal>

using namespace BlackMisc;
using namespace BlackMisc::Db;
using namespace BlackMisc::Network;
using namespace BlackCore;
using namespace BlackCore::Data;

namespace BlackCore
{
    CSetupReader::CSetupReader(QObject *parent) : QObject(parent),
        m_cmdBootstrapUrl
    {
        { "url", "bootstrapurl" },
        QCoreApplication::translate("application", "bootstrap URL, e.g. https://datastore.swift-project.org/shared"),
        "bootstrapurl", (sApp->isUnitTest()) ? unitTestBootstrapUrl() : ""
    },  m_cmdBootstrapMode
    {
        { "bmode", "bootstrapmode" },
        QCoreApplication::translate("application", "bootstrap mode: explicit, implicit, cache(-only)"),
        "bootstrapmode", "explicit"
    }
    { }

    QList<QCommandLineOption> CSetupReader::getCmdLineOptions() const
    {
        return  QList<QCommandLineOption>
        {
            {
                this->m_cmdBootstrapUrl,
                this->m_cmdBootstrapMode
            }
        };
    }

    CStatusMessageList CSetupReader::asyncLoad()
    {
        CStatusMessageList msgs;
        if (!this->m_localSetupFileValue.isEmpty())
        {
            msgs = this->readLocalBootstrapFile(this->m_localSetupFileValue);
            msgs.push_back(this->manageSetupAvailability(false, msgs.isSuccess()));
            return msgs;
        }

        this->m_setup.synchronize(); // make sure it is loaded
        const CGlobalSetup cachedSetup = m_setup.get();
        const bool cacheAvailable = cachedSetup.wasLoaded();
        msgs.push_back(cacheAvailable ?
                       CStatusMessage(this, CStatusMessage::SeverityInfo , "Cached setup synchronized and contains data") :
                       CStatusMessage(this, CStatusMessage::SeverityInfo , "Cached setup synchronized, but no data in cache")
                      );
        if (this->m_bootstrapMode == CacheOnly)
        {
            this->m_distributionUrls = cachedSetup.getSwiftDistributionFileUrls();
            msgs.push_back(cacheAvailable ?
                           CStatusMessage(this, CStatusMessage::SeverityInfo, "Cache only setup, using it as it is") :
                           CStatusMessage(this, CStatusMessage::SeverityError, "Cache only setup, but cache is empty")
                          );
            msgs.push_back(this->manageSetupAvailability(false, false));
            return msgs;
        }

        this->m_bootstrapUrls.clear(); // clean up previous values

        // web URL
        if (!this->m_bootstrapUrlFileValue.isEmpty())
        {
            // start with the one from cmd args
            this->m_bootstrapUrls.push_front(CUrl(this->m_bootstrapUrlFileValue));
        }

        // if ever loaded add those URLs
        if (cacheAvailable)
        {
            if (this->m_bootstrapMode != Explicit)
            {
                // also use previously cached URLs
                const CUrlList bootstrapCacheUrls(cachedSetup.getSwiftBootstrapFileUrls());
                this->m_bootstrapUrls.push_back(bootstrapCacheUrls);
                msgs.push_back(bootstrapCacheUrls.isEmpty() ?
                               CStatusMessage(this, CStatusMessage::SeverityWarning, "No bootstrap URLs in cache") :
                               CStatusMessage(this, CStatusMessage::SeverityInfo, "Adding " + QString::number(bootstrapCacheUrls.size()) + " bootstrap URLs from cache"));
            }
        }
        else
        {
            msgs.push_back(CStatusMessage(this, CStatusMessage::SeverityInfo, "Empty cache, will not add URLs from cache"));
        }

        this->m_bootstrapUrls.removeDuplicates(); // clean up
        if (this->m_bootstrapUrls.isEmpty())
        {
            // after all still empty
            msgs.push_back(CStatusMessage(this, CStatusMessage::SeverityInfo, "Your log files are here: " + CDirectoryUtils::logDirectory()));
            msgs.push_back(CStatusMessage(this, CStatusMessage::SeverityError, "No bootstrap URLs, cannot load setup"));
        }
        else
        {
            CStatusMessageList readMsgs = this->triggerReadSetup(); // async loading
            if (cacheAvailable && readMsgs.isFailure())
            {
                // error, but cache is available, we can continue
                readMsgs.clampSeverity(CStatusMessage::SeverityWarning);
                msgs.push_back(CStatusMessage(this, CStatusMessage::SeverityWarning, "Loading setup failed, but cache is available, will continue"));
                msgs.push_back(readMsgs);
            }
            else
            {
                msgs.push_back(readMsgs);
            }
        }
        return msgs;
    }

    bool CSetupReader::parseCmdLineArguments()
    {
        // copy vars at beginning to simplify a threadsafe version in the future
        const QString cmdLineBootstrapUrl = this->getCmdLineBootstrapUrl();
        BootstrapMode bootstrapMode = stringToEnum(sApp->getParserValue(this->m_cmdBootstrapMode));
        const bool ignoreCmdBootstrapUrl = m_ignoreCmdBootstrapUrl;
        const bool checkCmdBootstrapUrl = m_checkCmdBootstrapUrl;
        const QString bootstrapUrlFileValue = CGlobalSetup::buildBootstrapFileUrl(cmdLineBootstrapUrl);
        QString localSetupFileValue;
        const QUrl url(bootstrapUrlFileValue);
        const QString urlString(url.toString());
        bool ok = false;

        if (urlString.isEmpty() && bootstrapMode == Explicit)
        {
            bootstrapMode = Implicit; // no URL, we use implicit mode
        }

        do
        {
            // check on local file
            if (url.isLocalFile())
            {
                localSetupFileValue = url.toLocalFile();
                const QFile f(localSetupFileValue);
                if (!f.exists())
                {
                    sApp->cmdLineErrorMessage(QString("File '%1' does not exist)").arg(localSetupFileValue));
                    break;
                }
            }

            // check on explicit URL
            if (bootstrapMode == Explicit)
            {
                if (!url.isLocalFile())
                {
                    bool retry = false;

                    // "retry" possible in some cases
                    do
                    {
                        if (ignoreCmdBootstrapUrl || !checkCmdBootstrapUrl || CNetworkUtils::canConnect(url, CNetworkUtils::getLongTimeoutMs()))
                        {
                            ok = true;
                            break;
                        }
                        retry = sApp->cmdLineErrorMessage(QString("URL '%1' not reachable").arg(urlString), true);
                    }
                    while (retry);
                }
            }
        }
        while (false);

        m_localSetupFileValue = localSetupFileValue;
        m_bootstrapUrlFileValue = bootstrapUrlFileValue;
        m_bootstrapMode = bootstrapMode;
        return ok;
    }

    void CSetupReader::gracefulShutdown()
    {
        this->m_shutdown = true;
    }

    void CSetupReader::readSetup()
    {
        const CStatusMessageList msgs(this->triggerReadSetup());
        if (!msgs.isSuccess())
        {
            CLogMessage::preformatted(msgs);
        }
    }

    CStatusMessageList CSetupReader::triggerReadSetup()
    {
        if (this->m_shutdown) { return CStatusMessage(this, CStatusMessage::SeverityError, "shutdown"); }
        if (!sApp->isInternetAccessible())
        {
            const CStatusMessage m(this, CStatusMessage::SeverityInfo, "No network/internet, will try to recover");
            CStatusMessageList msgs(m);
            msgs.push_back(this->manageSetupAvailability(false, false));
            this->setLastSetupReadErrorMessages(msgs);
            return msgs;
        }

        const CUrl url(this->m_bootstrapUrls.obtainNextWorkingUrl());
        if (url.isEmpty())
        {
            const CStatusMessage m(this, CStatusMessage::SeverityError,
                                   "Cannot read setup, URLs: " + this->m_bootstrapUrls.toQString() +
                                   " failed URLs: " + this->m_bootstrapUrls.getFailedUrls().toQString());
            CStatusMessageList msgs(m);
            msgs.push_back(CNetworkUtils::createNetworkReport(sApp->getNetworkAccessManager()));
            msgs.push_back(this->manageSetupAvailability(false, false));
            this->setLastSetupReadErrorMessages(msgs);
            return msgs;
        }
        const CStatusMessage m(this, CStatusMessage::SeverityInfo, "Start reading URL: " + url.toQString());
        sApp->getFromNetwork(url.toNetworkRequest(), { this, &CSetupReader::parseSetupFile });
        this->setLastSetupReadErrorMessages(m); // clear errors
        return m;
    }

    void CSetupReader::readDistributionInfo()
    {
        const CUrl url(this->m_distributionUrls.obtainNextWorkingUrl());
        if (url.isEmpty())
        {
            CLogMessage(this).warning("Cannot read update info, URLs: '%1', failed URLs: '%2'") << this->m_distributionUrls << this->m_distributionUrls.getFailedUrls();
            CLogMessage::preformatted(CNetworkUtils::createNetworkReport(sApp->getNetworkAccessManager()));
            this->manageDistributionsInfoAvailability(false);
            return;
        }
        if (m_shutdown) { return; }
        sApp->getFromNetwork(url.toNetworkRequest(), { this, &CSetupReader::parseDistributionsFile});
    }

    void CSetupReader::setupChanged()
    {
        // settings have changed on disk
    }

    CSetupReader::BootstrapMode CSetupReader::stringToEnum(const QString &s)
    {
        const QString bsm(s.toLower().trimmed());
        if (bsm.startsWith("expl")) { return Explicit; }
        if (bsm.startsWith("cache")) { return CacheOnly; }
        return Implicit;
    }

    const QString &CSetupReader::unitTestBootstrapUrl()
    {
        static const QString url("https://datastore.swift-project.org/shared");
        return url;
    }

    CStatusMessageList CSetupReader::readLocalBootstrapFile(const QString &fileName)
    {
        if (fileName.isEmpty()) { return CStatusMessage(this).error("No file name for local bootstrap file"); }
        QString fn;
        const QFile file(fileName);
        if (!file.exists())
        {
            // relative name?
            QString dir(sApp->getCmdSwiftPrivateSharedDir());
            if (dir.isEmpty()) { return CStatusMessage(this).error("Empty shared directory '%1' for bootstrap file") << dir; }

            // no version for local files, as those come with the current code
            fn = CFileUtils::appendFilePaths(dir, "bootstrap/bootstrap.json");
        }
        else
        {
            fn = fileName;
        }

        const QString content(CFileUtils::readFileToString(fn));
        if (content.isEmpty()) { return CStatusMessage(this).error("File '%1' not existing or empty") << fn; }

        try
        {
            CGlobalSetup s;
            s.convertFromJson(content);
            s.setDevelopment(true);
            m_setup.set(s);
            return CStatusMessage(this).info("Setup cache updated from local file '%1'") << fn;
        }
        catch (const CJsonException &ex)
        {
            return ex.toStatusMessage(this, QString("Parsing local setup file '%1'").arg(fn));
        }
    }

    void CSetupReader::parseSetupFile(QNetworkReply *nwReplyPtr)
    {
        // wrap pointer, make sure any exit cleans up reply
        // required to use delete later as object is created in a different thread
        QScopedPointer<QNetworkReply, QScopedPointerDeleteLater> nwReply(nwReplyPtr);
        if (m_shutdown) { return; }

        const QUrl url(nwReply->url());
        const QString urlString(url.toString());
        const QString replyMessage(nwReply->errorString());

        if (nwReply->error() == QNetworkReply::NoError)
        {
            qint64 lastModified = CNetworkUtils::lastModifiedMsSinceEpoch(nwReply.data());
            QString setupJson(nwReplyPtr->readAll());
            nwReplyPtr->close();
            if (setupJson.isEmpty())
            {
                CLogMessage(this).info("No bootstrap setup file at '%1'") << urlString;
                // try next URL
            }
            else
            {
                try
                {
                    const CGlobalSetup currentSetup = m_setup.get();
                    CGlobalSetup loadedSetup;
                    loadedSetup.convertFromJson(setupJson);
                    loadedSetup.markAsLoaded(true);
                    const CUrl sharedUrl(loadedSetup.getCorrespondingSharedUrl(url));
                    if (!sharedUrl.isEmpty()) { emit this->successfullyReadSharedUrl(sharedUrl); }

                    if (lastModified > 0 && lastModified > loadedSetup.getMSecsSinceEpoch()) { loadedSetup.setMSecsSinceEpoch(lastModified); }
                    bool sameVersionLoaded = (loadedSetup == currentSetup);
                    if (sameVersionLoaded)
                    {
                        this->m_distributionUrls = currentSetup.getSwiftDistributionFileUrls(); // defaults
                        CLogMessage(this).info("Same setup version loaded from '%1' as already in data cache '%2'") << urlString << m_setup.getFilename();
                        CLogMessage::preformatted(this->manageSetupAvailability(true));
                        return; // success
                    }

                    // in the past I used to do a timestamp comparison here and skipped further setting
                    // with changed files from a different URL this was wrongly assuming outdated loaded files and was removed
                    const CStatusMessage m = m_setup.set(loadedSetup, loadedSetup.getMSecsSinceEpoch());
                    CLogMessage::preformatted(m);
                    if (m.isSeverityInfoOrLess())
                    {
                        // no issue with cache
                        this->m_distributionUrls = loadedSetup.getSwiftDistributionFileUrls();
                        CLogMessage(this).info("Loaded setup from '%1'") << urlString;
                        CLogMessage(this).info("Setup: Updated data cache in '%1'") << this->m_setup.getFilename();
                        {
                            QWriteLocker l(&m_lockSetup);
                            m_lastSuccessfulSetupUrl = urlString;
                        }
                    }
                    CLogMessage::preformatted(this->manageSetupAvailability(true));
                    return;
                }
                catch (const CJsonException &ex)
                {
                    // we downloaded an unparsable JSON file.
                    // as we control those files something is wrong
                    const QString errorMsg = QString("Setup file loaded from '%1' cannot be parsed").arg(urlString);
                    const CStatusMessage msg = ex.toStatusMessage(this, errorMsg);
                    CLogMessage::preformatted(msg);

                    // in dev. I get notified, in productive code I try next URL
                    // by falling thru
                    BLACK_VERIFY_X(false, Q_FUNC_INFO, errorMsg.toLocal8Bit().constData());
                }
            } // json empty
        } // no error
        else
        {
            // network error, log as warning as we will read again if possible
            // however, store as error because this will be a possible root cause if nothing else is
            nwReply->abort();
            const CStatusMessage msg = CStatusMessage(this).error("Reading setup failed '%1' '%2' (can possibly be fixed by reading from another server afterwards)") << replyMessage << urlString;
            CLogMessage::preformatted(msg);
            this->setLastSetupReadErrorMessages(msg);
        }

        // try next one if any
        if (this->m_bootstrapUrls.addFailedUrl(url))
        {
            m_distributionUrls.addFailedHost(url); // the same host will likely fail for distributions
            QTimer::singleShot(500, this, &CSetupReader::readSetup);
        }
        else
        {
            const CStatusMessageList msgs = this->manageSetupAvailability(false);
            CLogMessage::preformatted(msgs);
        }
    }

    void CSetupReader::parseDistributionsFile(QNetworkReply *nwReplyPtr)
    {
        // wrap pointer, make sure any exit cleans up reply
        // required to use delete later as object is created in a different thread
        QScopedPointer<QNetworkReply, QScopedPointerDeleteLater> nwReply(nwReplyPtr);
        if (m_shutdown) { return; }

        const QUrl url(nwReply->url());
        const QString urlString(url.toString());
        const QString replyMessage(nwReply->errorString());

        if (nwReply->error() == QNetworkReply::NoError)
        {
            const qint64 lastModified = CNetworkUtils::lastModifiedMsSinceEpoch(nwReply.data());
            const QString distributionJson(nwReplyPtr->readAll());
            nwReplyPtr->close();
            if (distributionJson.isEmpty())
            {
                CLogMessage(this).info("No distribution file content");
                // try next URL
            }
            else
            {
                try
                {
                    const CDistributionList loadedDistributions = CDistributionList::fromDatabaseJson(distributionJson);
                    if (loadedDistributions.isEmpty())
                    {
                        CLogMessage(this).error("Loading of distribution yielded no data");
                        this->manageDistributionsInfoAvailability(false);
                    }
                    else
                    {
                        CStatusMessage m = m_distributions.set(loadedDistributions, lastModified);
                        if (m.isFailure())
                        {
                            m.addCategories(getLogCategories());
                            CLogMessage::preformatted(m);
                            this->manageDistributionsInfoAvailability(false);
                        }
                        else
                        {
                            {
                                QWriteLocker l(&m_lockDistribution);
                                m_lastSuccessfulDistributionUrl = urlString;
                            }
                            CLogMessage(this).info("Distribution info loaded from '%1") << urlString;
                            CLogMessage(this).info("Distribution info: Updated data cache in '%1'") << m_distributions.getFilename();
                            this->manageDistributionsInfoAvailability(true);
                        } // cache
                    }
                    return;
                }
                catch (const CJsonException &ex)
                {
                    // we downloaded an unparsable JSON file.
                    // as we control those files something is wrong
                    const QString errorMsg = QString("Distribution file loaded from '%1' cannot be parsed").arg(urlString);
                    const CStatusMessage msg = ex.toStatusMessage(this, errorMsg);
                    CLogMessage::preformatted(msg);

                    // in dev. I get notified, in productive code I try next URL by falling thru
                    BLACK_VERIFY_X(false, Q_FUNC_INFO, errorMsg.toLocal8Bit().constData());
                }
            } // json empty
        } // no error
        else
        {
            // network error
            nwReply->abort();
            CLogMessage(this).error("Reading update info failed %1 %2 (can possibly be fixed by reading from another server afterwards)") << replyMessage << urlString;
        }

        // try next one if any
        if (this->m_distributionUrls.addFailedUrl(url))
        {
            QTimer::singleShot(500, this, &CSetupReader::readDistributionInfo);
        }
        else
        {
            const CStatusMessageList msgs = this->manageSetupAvailability(false);
            CLogMessage::preformatted(msgs);
        }
    }

    const CLogCategoryList &CSetupReader::getLogCategories()
    {
        static const CLogCategoryList cats({ CLogCategory("swift.setupreader"), CLogCategory::webservice(), CLogCategory::startup()});
        return cats;
    }

    bool CSetupReader::hasCmdLineBootstrapUrl() const
    {
        return !this->getCmdLineBootstrapUrl().isEmpty();
    }

    QString CSetupReader::getCmdLineBootstrapUrl() const
    {
        if (m_ignoreCmdBootstrapUrl) return "";
        return sApp->getParserValue(this->m_cmdBootstrapUrl);
    }

    void CSetupReader::setIgnoreCmdLineBootstrapUrl(bool ignore)
    {
        m_ignoreCmdBootstrapUrl = ignore;
        this->parseCmdLineArguments(); // T156 this part not threadsafe, currently not a real problem as setup reader runs in main thread
    }

    CGlobalSetup CSetupReader::getSetup() const
    {
        return m_setup.get();
    }

    bool CSetupReader::hasCachedSetup() const
    {
        const CGlobalSetup cachedSetup = m_setup.get();
        const bool cacheAvailable = cachedSetup.wasLoaded();
        return cacheAvailable;
    }

    QDateTime CSetupReader::getSetupCacheTimestamp() const
    {
        return m_setup.getTimestamp();
    }

    bool CSetupReader::prefillCacheWithLocalResourceBootstrapFile()
    {
        if (m_shutdown) { return false; }
        this->m_setup.synchronize(); // make sure it is loaded
        const CGlobalSetup cachedSetup = m_setup.get();
        const bool cacheAvailable = cachedSetup.wasLoaded();
        if (cacheAvailable)
        {
            CLogMessage(this).info("Setup cache prefill (bootstrap already cached, no prefill needed");
            return false;
        }
        const QString fn = CDirectoryUtils::bootstrapResourceFilePath();
        const CStatusMessageList msgs = this->readLocalBootstrapFile(fn);
        CLogMessage::preformatted(msgs);
        return true;
    }

    QString CSetupReader::getLastSuccessfulSetupUrl() const
    {
        QReadLocker l(&m_lockSetup);
        return m_lastSuccessfulSetupUrl;
    }

    CDistributionList CSetupReader::getDistributionInfo() const
    {
        return m_distributions.get();
    }

    bool CSetupReader::hasCachedDistributionInfo() const
    {
        const CDistributionList distributions = m_distributions.get();
        return !distributions.isEmpty();
    }

    QDateTime CSetupReader::getDistributionCacheTimestamp() const
    {
        return m_distributions.getTimestamp();
    }

    QString CSetupReader::getLastSuccessfulDistributionUrl() const
    {
        QReadLocker l(&m_lockDistribution);
        return m_lastSuccessfulDistributionUrl;
    }

    CStatusMessageList CSetupReader::getLastSetupReadErrorMessages() const
    {
        QReadLocker l(&m_lockSetup);
        return this->m_setupReadErrorMsgs;
    }

    const QString &CSetupReader::getBootstrapUrlFile() const
    {
        if (!m_localSetupFileValue.isEmpty()) { return m_localSetupFileValue; }
        return m_bootstrapUrlFileValue;
    }

    QString CSetupReader::getBootstrapModeAsString() const
    {
        switch (m_bootstrapMode)
        {
        case CacheOnly: return "cache only";
        case Explicit: return "explicit";
        case Implicit: return "implicit";
        default: break;
        }
        return "";
    }

    void CSetupReader::setLastSetupReadErrorMessages(const CStatusMessageList &messages)
    {
        QWriteLocker l(&m_lockSetup);
        this->m_setupReadErrorMsgs = messages.getErrorMessages();
    }

    CStatusMessageList CSetupReader::manageSetupAvailability(bool webRead, bool localRead)
    {
        Q_ASSERT_X(!(webRead && localRead), Q_FUNC_INFO, "Local and web read together seems to be wrong");
        CStatusMessageList msgs;
        if (webRead)
        {
            msgs.push_back(CStatusMessage(this).info("Setup loaded from web, will trigger read of distribution information"));
            QTimer::singleShot(500, this, &CSetupReader::readDistributionInfo);
        }
        if (localRead)
        {
            msgs.push_back(CStatusMessage(this).info("Setup loaded locally, will trigger read of distribution information"));
            QTimer::singleShot(500, this, &CSetupReader::readDistributionInfo);
        }

        bool available = false;
        if (webRead || localRead)
        {
            available = true;
        }
        else
        {
            bool cacheAvailable = this->m_setup.get().wasLoaded();
            available = cacheAvailable && this->m_bootstrapMode != Explicit;
        }

        if (available && !webRead && !localRead)
        {
            msgs.push_back(CStatusMessage(this, CStatusMessage::SeverityInfo, "Setup available, but not updated this time"));
        }
        else if (!available)
        {
            msgs.push_back(CStatusMessage(this, CStatusMessage::SeverityError, "Setup not available"));
            if (this->m_bootstrapMode == Explicit)
            {
                msgs.push_back(CStatusMessage(this, CStatusMessage::SeverityError, "Mode is 'explicit', likely URL '" + this->m_bootstrapUrlFileValue + "' is not reachable"));
            }
        }
        this->m_setupAvailable = available;
        emit this->setupHandlingCompleted(available);

        if (!webRead && !localRead)
        {
            msgs.push_back(CStatusMessage(this).warning("Since setup was not updated this time, will not start loading of update information"));
            this->manageDistributionsInfoAvailability(false);
        }
        return msgs;
    }

    void CSetupReader::manageDistributionsInfoAvailability(bool webRead)
    {
        if (webRead)
        {
            this->m_distributionInfoAvailable = true;
            emit distributionInfoAvailable(true);
        }
        else
        {
            const bool cached = this->m_distributions.isSaved();
            this->m_distributionInfoAvailable = cached;
            emit distributionInfoAvailable(cached);
        }
    }
} // namespace

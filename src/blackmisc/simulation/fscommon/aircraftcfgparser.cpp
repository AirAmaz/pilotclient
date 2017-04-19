/* Copyright (C) 2015
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "blackcore/db/databaseutils.h"
#include "blackmisc/simulation/aircraftmodelutils.h"
#include "blackmisc/simulation/fscommon/aircraftcfgentries.h"
#include "blackmisc/simulation/fscommon/aircraftcfgparser.h"
#include "blackmisc/simulation/fscommon/fscommonutil.h"
#include "blackmisc/fileutils.h"
#include "blackmisc/logmessage.h"
#include "blackmisc/statusmessagelist.h"
#include "blackmisc/worker.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFileInfoList>
#include <QFlags>
#include <QIODevice>
#include <QList>
#include <QMetaType>
#include <QSettings>
#include <QTextStream>
#include <Qt>
#include <QtGlobal>
#include <atomic>
#include <tuple>

using namespace BlackMisc;
using namespace BlackMisc::Simulation;
using namespace BlackMisc::Simulation::FsCommon;
using namespace BlackMisc::Network;
using namespace BlackCore::Db;

namespace BlackMisc
{
    namespace Simulation
    {
        namespace FsCommon
        {
            // response for async. loading
            using LoaderResponse = std::tuple<CAircraftCfgEntriesList, CAircraftModelList, BlackMisc::CStatusMessageList, bool>;

            CAircraftCfgParser::CAircraftCfgParser(const CSimulatorInfo &simInfo) :
                IAircraftModelLoader(simInfo)
            { }

            std::unique_ptr<CAircraftCfgParser> CAircraftCfgParser::createModelLoader(const CSimulatorInfo &simInfo)
            {
                return std::make_unique<CAircraftCfgParser>(simInfo);
            }

            CAircraftCfgParser::~CAircraftCfgParser()
            {
                // that should be safe as long as the worker uses deleteLater (which it does)
                if (this->m_parserWorker) { this->m_parserWorker->waitForFinished(); }
            }

            void CAircraftCfgParser::startLoadingFromDisk(LoadMode mode, const ModelConsolidation &modelConsolidation, const QString &directory)
            {
                static const CStatusMessage statusLoadingOk(this, CStatusMessage::SeverityInfo, "Aircraft config parser loaded data");
                static const CStatusMessage statusLoadingError(this, CStatusMessage::SeverityError, "Aircraft config parser did not load data");

                const CSimulatorInfo simulator = this->getSimulator();
                const QString modelDirectory = CFileUtils::fixWindowsUncPath(
                                                   directory.isEmpty() ? this->m_settings.getFirstModelDirectoryOrDefault(simulator) : directory
                                               ); // expect only one directory
                const QStringList excludedDirectoryPatterns(this->m_settings.getModelExcludeDirectoryPatternsOrDefault(simulator)); // copy

                if (mode.testFlag(LoadInBackground))
                {
                    if (m_parserWorker && !m_parserWorker->isFinished()) { return; }
                    m_parserWorker = BlackMisc::CWorker::fromTask(this, "CAircraftCfgParser::changeDirectory",
                                     [this, modelDirectory, excludedDirectoryPatterns, simulator, modelConsolidation]()
                    {
                        bool ok = false;
                        CStatusMessageList msgs;
                        const auto aircraftCfgEntriesList = this->performParsing(modelDirectory, excludedDirectoryPatterns, msgs, &ok);
                        CAircraftModelList models;
                        if (ok)
                        {
                            models = aircraftCfgEntriesList.toAircraftModelList(simulator, true, msgs);
                            if (modelConsolidation) { modelConsolidation(models, true); }
                        }
                        return std::make_tuple(aircraftCfgEntriesList, models, msgs, ok);
                    });
                    m_parserWorker->thenWithResult<LoaderResponse>(this, [this, simulator](const LoaderResponse & tuple)
                    {
                        const bool ok = std::get<3>(tuple);
                        this->m_loadingMessages = std::get<2>(tuple);
                        if (ok)
                        {
                            this->m_parsedCfgEntriesList = std::get<0>(tuple);
                            const CAircraftModelList models(std::get<1>(tuple));
                            const bool hasData = !models.isEmpty();
                            if (hasData)
                            {
                                this->setCachedModels(models, simulator); // not thread safe
                            }
                            // currently I treat no data as error
                            emit this->loadingFinished(hasData ? statusLoadingOk : statusLoadingError, simulator);
                        }
                        else
                        {
                            CStatusMessage status = this->m_loadingMessages.toSingleMessage();
                            status.setSeverity(CStatusMessage::SeverityError);
                            emit this->loadingFinished(status, simulator);
                        }
                    });
                }
                else if (mode == LoadDirectly)
                {
                    bool ok;
                    CStatusMessageList msgs;
                    this->m_parsedCfgEntriesList = performParsing(modelDirectory, excludedDirectoryPatterns, msgs, &ok);
                    CAircraftModelList models(this->m_parsedCfgEntriesList.toAircraftModelList(simulator, true, msgs));
                    this->m_loadingMessages = msgs;
                    const bool hasData = !models.isEmpty();
                    if (hasData)
                    {
                        this->setCachedModels(models); // not thread safe
                    }
                    // currently I treat no data as error
                    emit this->loadingFinished(hasData ? statusLoadingOk : statusLoadingError, simulator);
                }
            }

            bool CAircraftCfgParser::isLoadingFinished() const
            {
                return !m_parserWorker || m_parserWorker->isFinished();
            }

            bool CAircraftCfgParser::areModelFilesUpdated() const
            {
                const QDateTime cacheTs(getCacheTimestamp());
                if (!cacheTs.isValid()) { return true; }
                return CFileUtils::containsFileNewerThan(
                           cacheTs,
                           this->m_settings.getFirstModelDirectoryOrDefault(this->getSimulator()),
                           true, { fileFilter() }, this->getModelExcludeDirectoryPatterns());
            }

            CAircraftCfgEntriesList CAircraftCfgParser::performParsing(const QString &directory, const QStringList &excludeDirectories, CStatusMessageList &messages, bool *ok)
            {
                //
                // function has to be threadsafe
                //

                *ok = false;
                if (m_cancelLoading) { return CAircraftCfgEntriesList(); }

                // excluded?
                if (CFileUtils::isExcludedDirectory(directory, excludeDirectories))
                {
                    CLogMessage(this).debug() << "Skipping directory " << directory;
                    *ok = true;
                    return CAircraftCfgEntriesList();
                }

                // set directory with name filters, get aircraft.cfg and sub directories
                const QDir dir(directory, fileFilter(), QDir::Name, QDir::Files | QDir::AllDirs | QDir::NoDotAndDotDot);
                if (!dir.exists())
                {
                    *ok = true;
                    return CAircraftCfgEntriesList(); // can happen if there are shortcuts or linked dirs not available
                }

                const QString currentDir = dir.absolutePath();
                CAircraftCfgEntriesList result;

                // Dirs last is crucial,since I will break recursion on "aircraft.cfg" level
                const QFileInfoList files = dir.entryInfoList(QDir::Files | QDir::AllDirs | QDir::NoDotAndDotDot, QDir::DirsLast);
                for (const auto &fileInfo : files)
                {
                    if (m_cancelLoading) { return CAircraftCfgEntriesList(); }
                    if (fileInfo.isDir())
                    {
                        const QString nextDir = fileInfo.absoluteFilePath();
                        if (currentDir.startsWith(nextDir, Qt::CaseInsensitive)) { continue; } // do not go up
                        if (dir == currentDir) { continue; } // do not recursively call same directory

                        bool dirOk;
                        const CAircraftCfgEntriesList subList(performParsing(nextDir, excludeDirectories, messages, &dirOk));
                        if (dirOk)
                        {
                            result.push_back(subList);
                        }
                        else
                        {
                            CLogMessage(this).warning("Parsing failed for %1") << nextDir;
                        }
                    }
                    else
                    {
                        // due to the filter we expect only "aircraft.cfg" here
                        // remark: in a 1st version I have used QSettings to parse to file as ini file
                        // unfortunately some files are malformed which could end up in wrong data

                        const QString fileName = fileInfo.absoluteFilePath();
                        QFile file(fileName);
                        if (!file.open(QFile::ReadOnly | QFile::Text))
                        {
                            CLogMessage(this).warning("Unable to read file %1") << fileName;
                            continue;
                        }
                        QTextStream in(&file);
                        QList<CAircraftCfgEntries> tempEntries;

                        // parse through the file
                        QString atcType;
                        QString atcModel;
                        QString fltSection("[FLTSIM.0]");
                        int fltsimCounter = 0;
                        FileSection currentSection = Unknown;
                        bool isRotorcraftPath = fileName.contains("rotorcraft", Qt::CaseInsensitive);

                        while (!in.atEnd())
                        {
                            const QString lineFixed(in.readLine().trimmed());
                            if (lineFixed.isEmpty()) { continue; }
                            if (lineFixed.startsWith("["))
                            {
                                if (lineFixed.startsWith("[GENERAL]", Qt::CaseInsensitive)) { currentSection = General; continue; }
                                if (lineFixed.startsWith(fltSection, Qt::CaseInsensitive))
                                {
                                    CAircraftCfgEntries e(fileName, fltsimCounter);
                                    if (isRotorcraftPath)
                                    {
                                        e.setRotorcraft(true);
                                    }
                                    tempEntries.append(e);
                                    currentSection = Fltsim;
                                    fltSection = QString("[FLTSIM.%1]").arg(++fltsimCounter);
                                    continue;
                                }
                                currentSection = Unknown;
                                continue;
                            }
                            switch (currentSection)
                            {
                            case General:
                                {
                                    if (lineFixed.startsWith("//")) { break; }
                                    if (atcType.isEmpty() || atcModel.isEmpty())
                                    {
                                        const QString c = getFixedIniLineContent(lineFixed);
                                        if (lineFixed.startsWith("atc_type", Qt::CaseInsensitive))
                                        {
                                            atcType = c;
                                        }
                                        else if (lineFixed.startsWith("atc_model", Qt::CaseInsensitive))
                                        {
                                            atcModel = c;
                                        }
                                    }
                                }
                                break;
                            case Fltsim:
                                {
                                    if (lineFixed.startsWith("//")) { break; }
                                    CAircraftCfgEntries &e = tempEntries[tempEntries.size() - 1];
                                    if (lineFixed.startsWith("atc_", Qt::CaseInsensitive))
                                    {
                                        if (lineFixed.startsWith("atc_parking_codes", Qt::CaseInsensitive))
                                        {
                                            e.setAtcParkingCode(getFixedIniLineContent(lineFixed));
                                        }
                                        else if (lineFixed.startsWith("atc_airline", Qt::CaseInsensitive))
                                        {
                                            e.setAtcAirline(getFixedIniLineContent(lineFixed));
                                        }
                                        else if (lineFixed.startsWith("atc_id_color", Qt::CaseInsensitive))
                                        {
                                            e.setAtcIdColor(getFixedIniLineContent(lineFixed));
                                        }
                                    }
                                    else if (lineFixed.startsWith("ui_", Qt::CaseInsensitive))
                                    {
                                        if (lineFixed.startsWith("ui_manufacturer", Qt::CaseInsensitive))
                                        {
                                            e.setUiManufacturer(getFixedIniLineContent(lineFixed));
                                        }
                                        else if (lineFixed.startsWith("ui_typerole", Qt::CaseInsensitive))
                                        {
                                            bool r = getFixedIniLineContent(lineFixed).toLower().contains("rotor");
                                            e.setRotorcraft(r);
                                        }
                                        else if (lineFixed.startsWith("ui_type", Qt::CaseInsensitive))
                                        {
                                            e.setUiType(getFixedIniLineContent(lineFixed));
                                        }
                                        else if (lineFixed.startsWith("ui_variation", Qt::CaseInsensitive))
                                        {
                                            e.setUiVariation(getFixedIniLineContent(lineFixed));
                                        }
                                    }
                                    else if (lineFixed.startsWith("description", Qt::CaseInsensitive))
                                    {
                                        e.setDescription(getFixedIniLineContent(lineFixed));
                                    }
                                    else if (lineFixed.startsWith("texture", Qt::CaseInsensitive))
                                    {
                                        e.setTexture(getFixedIniLineContent(lineFixed));
                                    }
                                    else if (lineFixed.startsWith("createdBy", Qt::CaseInsensitive))
                                    {
                                        e.setCreatedBy(getFixedIniLineContent(lineFixed));
                                    }
                                    else if (lineFixed.startsWith("sim", Qt::CaseInsensitive))
                                    {
                                        e.setSimName(getFixedIniLineContent(lineFixed));
                                    }
                                    else if (lineFixed.startsWith("title", Qt::CaseInsensitive))
                                    {
                                        e.setTitle(getFixedIniLineContent(lineFixed));
                                    }
                                }
                                break;
                            default:
                            case Unknown: break;
                            }
                        } // all lines
                        file.close();

                        // store all entries
                        QDateTime fileTimestamp(fileInfo.lastModified());
                        if (!fileTimestamp.isValid() || fileInfo.created() > fileTimestamp)
                        {
                            fileTimestamp = fileInfo.created();
                        }
                        Q_ASSERT_X(fileTimestamp.isValid(), Q_FUNC_INFO, "Missing file timestamp");
                        for (const CAircraftCfgEntries &e : as_const(tempEntries))
                        {
                            if (e.getTitle().isEmpty())
                            {
                                CLogMessage(this).info("FS model in %1, index %2 has no title") << fileName << e.getIndex();
                                continue;
                            }
                            CAircraftCfgEntries newEntries(e);
                            newEntries.setAtcModel(atcModel);
                            newEntries.setAtcType(atcType);
                            newEntries.setUtcTimestamp(fileTimestamp);
                            result.push_back(newEntries);
                        }
                        *ok = true;
                        return result; // do not go any deeper in file tree, we found aircraft.cfg
                    }
                }

                // all files finished,
                // normally reached when no aircraft.cfg is found
                *ok = true;
                return result;
            }

            QString CAircraftCfgParser::fixedStringContent(const QSettings &settings, const QString &key)
            {
                return fixedStringContent(settings.value(key));
            }

            QString CAircraftCfgParser::fixedStringContent(const QVariant &qv)
            {
                if (qv.isNull() || !qv.isValid())
                {
                    return ""; // normal when there is no settings value
                }
                else if (static_cast<QMetaType::Type>(qv.type()) == QMetaType::QStringList)
                {
                    const QStringList l = qv.toStringList();
                    return l.join(",").trimmed();
                }
                else if (static_cast<QMetaType::Type>(qv.type()) == QMetaType::QString)
                {
                    return qv.toString().trimmed();
                }
                Q_ASSERT(false);
                return "";
            }

            QString CAircraftCfgParser::getFixedIniLineContent(const QString &line)
            {
                if (line.isEmpty()) { return ""; }
                int index = line.indexOf('=');
                if (index < 0) { return ""; }
                if (line.length() < index  + 1) { return ""; }

                QString content(line.midRef(index + 1).trimmed().toString());

                // fix "" strings, some are malformed and just contain " at beginning, not at the end
                if (content.endsWith('"')) { content.remove(content.size() - 1 , 1); }
                if (content.startsWith('"')) { content.remove(0 , 1); }

                // fix C style linebreaks
                content.replace("\\n", " ");
                content.replace("\\t", " ");
                return content;
            }

            const QString &CAircraftCfgParser::fileFilter()
            {
                static const QString f("aircraft.cfg");
                return f;
            }
        } // ns
    } // ns
} // ns

Q_DECLARE_METATYPE(BlackMisc::Simulation::FsCommon::LoaderResponse)

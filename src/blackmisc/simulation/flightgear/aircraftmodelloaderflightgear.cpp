#include "aircraftmodelloaderflightgear.h"
#include "blackmisc/simulation/aircraftmodel.h"
#include <QDirIterator>
namespace BlackMisc{
    namespace Simulation {
    namespace Flightgear {

    bool AircraftModelLoaderFlightgear::isLoadingFinished() const
    {
        return !m_parserWorker || m_parserWorker->isFinished();;
    }

    AircraftModelLoaderFlightgear::AircraftModelLoaderFlightgear(QObject *parent) : Simulation::IAircraftModelLoader (Simulation::CSimulatorInfo::fg(), parent)
        {
        std::cout << "Test";
    }

    void AircraftModelLoaderFlightgear::updateInstalledModels(const CAircraftModelList &models)
    {
        this->setModelsForSimulator(models, CSimulatorInfo::fg());
        const CStatusMessage m = CStatusMessage(this, CStatusMessage::SeverityInfo, u"Flightgear updated '%1' models") << models.size();
        m_loadingMessages.push_back(m);
    }

        Simulation::CAircraftModelList AircraftModelLoaderFlightgear::parseFlyableAirplanes(const QString &rootDirectory, const QStringList &excludeDirectories)
        {
            Q_UNUSED(excludeDirectories);
            if (rootDirectory.isEmpty()) { return {}; }

            Simulation::CAircraftModelList installedModels;

            QDir searchPath(rootDirectory, fileFilterFlyable());
            QDirIterator aircraftIt(searchPath, QDirIterator::Subdirectories | QDirIterator::FollowSymlinks);


            while (aircraftIt.hasNext()) {
                aircraftIt.next();
                if (CFileUtils::isExcludedDirectory(aircraftIt.fileInfo(), excludeDirectories, Qt::CaseInsensitive)) { continue; }

                Simulation::CAircraftModel model;
                model.setAircraftIcaoCode(QString::fromStdString("A320"));
                model.setDescription(QString::fromStdString("Flyable"));
                model.setName(QString::fromStdString("ModelName"));
                model.setModelType(CAircraftModel::TypeOwnSimulatorModel);
                model.setSimulator(CSimulatorInfo::fg());
                model.setFileDetailsAndTimestamp(aircraftIt.fileInfo());
                model.setModelMode(CAircraftModel::Include);

                addUniqueModel(model,installedModels);

            }

            return installedModels;
        }

        CAircraftModelList AircraftModelLoaderFlightgear::parseAIAirplanes(const QString &rootDirectory, const QStringList &excludeDirectories)
        {
            Q_UNUSED(excludeDirectories);
            if (rootDirectory.isEmpty()) { return {}; }

            Simulation::CAircraftModelList installedModels;

            QDir searchPath(rootDirectory, fileFilterAI());
            QDirIterator aircraftIt(searchPath, QDirIterator::Subdirectories | QDirIterator::FollowSymlinks);


            while (aircraftIt.hasNext()) {
                aircraftIt.next();
                if (CFileUtils::isExcludedDirectory(aircraftIt.fileInfo(), excludeDirectories, Qt::CaseInsensitive)) { continue; }
                //QString base = "main";
                //if(base.compare(aircraftIt.fileName())){ continue;}

                Simulation::CAircraftModel model;
                model.setAircraftIcaoCode(QString::fromStdString("A320"));
                model.setDescription(QString::fromStdString("AI"));
                model.setName(QString::fromStdString("ModelName"));
                model.setModelType(CAircraftModel::TypeOwnSimulatorModel);
                model.setSimulator(CSimulatorInfo::fg());
                model.setFileDetailsAndTimestamp(aircraftIt.fileInfo());
                model.setModelMode(CAircraftModel::Include);

                addUniqueModel(model,installedModels);

            }

            return installedModels;
        }

        const QString &AircraftModelLoaderFlightgear::fileFilterFlyable()
        {
            static const QString f("*-set.xml");
            return f;
        }

        const QString &AircraftModelLoaderFlightgear::fileFilterAI()
        {
            static const QString f("*.xml");
            return f;

        }

        void AircraftModelLoaderFlightgear::addUniqueModel(const CAircraftModel &model, CAircraftModelList &models)
        {
            //TODO Add check
            models.push_back(model);
        }

        CAircraftModelList AircraftModelLoaderFlightgear::performParsing(const QStringList &rootDirectories, const QStringList &excludeDirectories)
        {
            CAircraftModelList allModels;
            for (const QString &rootDirectory : rootDirectories)
            {
                //TODO Make paths variable
                allModels.push_back(parseAIAirplanes("X:/Flightsim/Flightgear/2018.3/data/AI/Aircraft", excludeDirectories));
                allModels.push_back(parseFlyableAirplanes("X:/Flightsim/Flightgear/2018.3/data/Aircraft", excludeDirectories));
            }

            return allModels;
        }

        void AircraftModelLoaderFlightgear::startLoadingFromDisk(IAircraftModelLoader::LoadMode mode, const IAircraftModelLoader::ModelConsolidationCallback &modelConsolidation, const QStringList &modelDirectories)
        {
            const CSimulatorInfo simulator = CSimulatorInfo::fg();
            const QStringList modelDirs = this->getInitializedModelDirectories(modelDirectories, simulator);
            const QStringList excludedDirectoryPatterns(m_settings.getModelExcludeDirectoryPatternsOrDefault(simulator)); // copy

            if (m_parserWorker && !m_parserWorker->isFinished()){ return; }
            emit this->diskLoadingStarted(simulator, mode);

            m_parserWorker = CWorker::fromTask(this, "CAircraftModelLoaderFlightgear::performParsing",
                                                                   [this, modelDirs, excludedDirectoryPatterns, modelConsolidation]()
                                {
                                    auto models = this->performParsing(modelDirs, excludedDirectoryPatterns);
                                    if (modelConsolidation) { modelConsolidation(models, true); }
                                    return models;
                                });
                                m_parserWorker->thenWithResult<CAircraftModelList>(this, [ = ](const auto & models)
                                {
                                    this->updateInstalledModels(models);
                                    m_loadingMessages.freezeOrder();
                                    emit this->loadingFinished(m_loadingMessages, simulator, ParsedData);
                                });
        }


        }
    }

}


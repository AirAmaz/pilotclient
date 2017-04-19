/* Copyright (C) 2013
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "blackcore/aircraftmatcher.h"
#include "blackcore/application.h"
#include "blackcore/webdataservices.h"
#include "blackmisc/simulation/simulatedaircraft.h"
#include "blackmisc/simulation/matchingutils.h"
#include "blackmisc/aviation/aircrafticaocode.h"
#include "blackmisc/aviation/airlineicaocode.h"
#include "blackmisc/aviation/callsign.h"
#include "blackmisc/aviation/livery.h"
#include "blackmisc/logcategory.h"
#include "blackmisc/logcategorylist.h"
#include "blackmisc/logmessage.h"
#include "blackmisc/statusmessagelist.h"

#include <QList>
#include <QStringList>
#include <QtGlobal>
#include <QStringBuilder>

using namespace BlackMisc;
using namespace BlackMisc::Simulation;
using namespace BlackMisc::Network;
using namespace BlackMisc::Aviation;

namespace BlackCore
{
    const CLogCategoryList &CAircraftMatcher::getLogCategories()
    {
        static const BlackMisc::CLogCategoryList cats { CLogCategory::matching() };
        return cats;
    }

    CAircraftMatcher::CAircraftMatcher(MatchingMode matchingMode, QObject *parent) :
        QObject(parent),
        m_matchingMode(matchingMode)
    { }

    CAircraftMatcher::~CAircraftMatcher()
    { }

    CAircraftModel CAircraftMatcher::getClosestMatch(const CSimulatedAircraft &remoteAircraft, CStatusMessageList *log) const
    {
        const CAircraftModelList modelSet(this->m_modelSet); // Models for this matching
        const MatchingMode mode = this->m_matchingMode;

        static const QString format("hh:mm:ss.zzz");
        CMatchingUtils::addLogDetailsToList(log, remoteAircraft, QString("--- Matching: UTC %1 ---").arg(QDateTime::currentDateTimeUtc().toString(format)));
        CMatchingUtils::addLogDetailsToList(log, remoteAircraft, QString("Matching uses model set of %1 models").arg(modelSet.size()), getLogCategories());
        CMatchingUtils::addLogDetailsToList(log, remoteAircraft, QString("Input model: '%1' '%2'").arg(remoteAircraft.getCallsignAsString(), remoteAircraft.getModel().toQString()), getLogCategories());

        // Before I really search I check some special conditions
        // 1) Manually set model (by user)
        // 2) No model set at all
        // 3) Exact match by model string

        // Manually set string?
        CAircraftModel matchedModel;
        bool resolvedInPrephase = false;
        if (remoteAircraft.getModel().hasManuallySetString())
        {
            // the user did a manual mapping "by hand", so he really should know what he is doing
            // no matching
            CMatchingUtils::addLogDetailsToList(log, remoteAircraft, "Manually set model " + remoteAircraft.getModelString(), getLogCategories());
            matchedModel = remoteAircraft.getModel();
            resolvedInPrephase = true;
        }
        else if (modelSet.isEmpty())
        {
            CMatchingUtils::addLogDetailsToList(log, remoteAircraft, "No models for matching, using default", getLogCategories(), CStatusMessage::SeverityError);
            matchedModel = this->getDefaultModel();
            resolvedInPrephase = true;
        }
        else if (remoteAircraft.hasModelString())
        {
            // try to find in installed models by model string
            if (mode.testFlag(ByModelString))
            {
                matchedModel = matchByExactModelString(remoteAircraft, modelSet, log);
                if (matchedModel.hasModelString())
                {
                    CMatchingUtils::addLogDetailsToList(log, remoteAircraft, "Exact match by model string '" + matchedModel.getModelStringAndDbKey() + "'", getLogCategories(), CStatusMessage::SeverityError);
                    resolvedInPrephase = true;
                }
            }
            else
            {
                CMatchingUtils::addLogDetailsToList(log, remoteAircraft, "Skipping model string match", getLogCategories());
            }
        }

        if (!resolvedInPrephase)
        {
            // matchedModel = this->getClosestMatchSearchImplementation(mode, modelSet, remoteAircraft, log);
            matchedModel = this->getClosestMatchScoreImplementation(mode, modelSet, remoteAircraft, log);
        }

        // copy over callsign validate
        matchedModel.setCallsign(remoteAircraft.getCallsign());

        Q_ASSERT_X(!matchedModel.getCallsign().isEmpty(), Q_FUNC_INFO, "Missing callsign");
        Q_ASSERT_X(matchedModel.hasModelString(), Q_FUNC_INFO, "Missing model string");
        Q_ASSERT_X(matchedModel.getModelType() != CAircraftModel::TypeUnknown, Q_FUNC_INFO, "Missing model type");

        CMatchingUtils::addLogDetailsToList(log, remoteAircraft, QString("--- Matching end: UTC %1 ---").arg(QDateTime::currentDateTimeUtc().toString(format)));
        return matchedModel;
    }

    CAircraftModel CAircraftMatcher::reverselLookupModel(const CCallsign &callsign, const QString &networkAircraftIcao, const QString &networkAirlineIcao, const QString &networkLiveryInfo, const QString &networkModelString, CAircraftModel::ModelType type, CStatusMessageList *log)
    {
        CLivery livery;
        livery.setAirlineIcaoCode(networkAirlineIcao);
        CAircraftModel model(networkModelString, type, "", CAircraftIcaoCode(networkAircraftIcao), livery);
        model.setCallsign(callsign);
        model = CAircraftMatcher::reverselLookupModel(model, networkLiveryInfo, log);
        return model;
    }

    CAircraftModel CAircraftMatcher::reverselLookupModel(const CAircraftModel &modelToLookup, const QString &networkLiveryInfo, CStatusMessageList *log)
    {
        Q_ASSERT_X(sApp, Q_FUNC_INFO, "Missing sApp");
        Q_ASSERT_X(sApp->getWebDataServices(), Q_FUNC_INFO, "No web services");

        // already DB model?
        CAircraftModel model(modelToLookup); // copy
        if (modelToLookup.isLoadedFromDb()) { return modelToLookup; }

        // --- now I try to fill in as many DB data as possible ---
        // 1) This will unify data where possible
        // 2) I have full information of what the other pilot flies where possible
        // 3) This is not model matching here (!), it is a process of getting the most accurate data from that fuzzy information I get via FSD
        //
        // Reverse lookup, use DB data wherever possible
        // 1) If I cannot resolve the ICAO codes here, they are either wrong (most likely in most cases) or
        // 2) not in the DB yet

        const CCallsign callsign(model.getCallsign());
        const QStringList liveryModelStrings = CAircraftModel::splitNetworkLiveryString(networkLiveryInfo);
        const QString modelString(modelToLookup.hasModelString() ? modelToLookup.getModelString() : liveryModelStrings[1]);
        QString liveryCode(liveryModelStrings[0]);

        if (!modelString.isEmpty())
        {
            // if we find the model here we have a fully defined DB model
            const CAircraftModel modelFromDb(sApp->getWebDataServices()->getModelForModelString(modelString));
            if (modelFromDb.hasValidDbKey())
            {
                model = modelFromDb;
                if (log) { CMatchingUtils::addLogDetailsToList(log, callsign, QString("Reverse lookup found DB model '%1' for '%2'").arg(modelFromDb.getModelStringAndDbKey(), callsign.toQString()), getLogCategories()); }
            }
            else
            {
                if (log) { CMatchingUtils::addLogDetailsToList(log, callsign, QString("Reverse lookup did not find model for '%1'").arg(modelString), getLogCategories()); }
            }
        }

        // lookup if model is not yet from DB
        if (!model.isLoadedFromDb())
        {
            // check if livery is already from DB
            if (!model.getLivery().isLoadedFromDb())
            {
                CAirlineIcaoCode airlineIcaoCode(model.getAirlineIcaoCode());
                if (!airlineIcaoCode.isLoadedFromDb())
                {
                    airlineIcaoCode = CAircraftMatcher::reverseLookupAirlineIcao(airlineIcaoCode.getDesignator(), callsign, log);
                }

                // try to match by livery
                if (liveryCode.isEmpty() && airlineIcaoCode.hasValidDesignator())
                {
                    // we create a standard livery code, then we try to find based on this
                    liveryCode = CLivery::getStandardCode(airlineIcaoCode);
                }

                if (CLivery::isValidCombinedCode(liveryCode))
                {
                    // search livery by combined code
                    const CLivery reverseLivery(sApp->getWebDataServices()->getLiveryForCombinedCode(liveryCode));
                    if (reverseLivery.hasValidDbKey())
                    {
                        // we have found a livery in the DB
                        model.setLivery(reverseLivery);
                        if (log) { CMatchingUtils::addLogDetailsToList(log, callsign, QString("Reverse lookup of livery '%1'").arg(reverseLivery.getCombinedCodePlusInfo()), getLogCategories()); }
                    }
                    else
                    {
                        // no livery data found
                        if (log) { CMatchingUtils::addLogDetailsToList(log, callsign, QString("Reverse lookup of livery '%1' yielded no result").arg(reverseLivery.getCombinedCodePlusInfo()), getLogCategories()); }
                    }
                } // livery lookup

                // if no DB livery yet, create own livery
                if (!model.hasValidDbKey() && !model.getLivery().hasValidDbKey())
                {
                    // create a pseudo livery, try to find airline first
                    if (airlineIcaoCode.hasValidDesignator())
                    {
                        const CLivery liveryDummy(CLivery::getStandardCode(airlineIcaoCode), airlineIcaoCode, "Generated");
                        model.setLivery(liveryDummy);
                        if (log) { CMatchingUtils::addLogDetailsToList(log, callsign, QString("Generated livery, set livery `%1`").arg(liveryDummy.getCombinedCodePlusInfo())); }
                    }
                } // pseudo livery
            } // livery from DB

            // aircraft ICAO if not from DB yet
            if (!model.getAircraftIcaoCode().hasValidDbKey())
            {
                CAircraftIcaoCode aircraftIcaoUsedForLookup(model.getAircraftIcaoCode());
                if (!aircraftIcaoUsedForLookup.isLoadedFromDb())
                {
                    aircraftIcaoUsedForLookup = CAircraftMatcher::reverseLookupAircraftIcao(aircraftIcaoUsedForLookup.getDesignator(), callsign, log);
                    if (aircraftIcaoUsedForLookup.isLoadedFromDb())
                    {
                        if (log) { CMatchingUtils::addLogDetailsToList(log, callsign, QString("Set aircraft ICAO to '%1' from DB").arg(aircraftIcaoUsedForLookup.getCombinedIcaoStringWithKey())); }
                    }
                    else
                    {
                        // no DB data, we update as much as possible
                        if (log) { CMatchingUtils::addLogDetailsToList(log, callsign, QString("Reverse lookup, ICAO '%1' not resolved from DB").arg(aircraftIcaoUsedForLookup.getDesignator())); }
                    }
                }
            }
        } // model from DB

        model.setCallsign(callsign);
        model.setModelType(modelToLookup.getModelType());

        if (log)
        {
            CMatchingUtils::addLogDetailsToList(log, callsign, QString("Using model: ICAO '%1', livery '%2', model '%3', type '%4'").
                                                arg(model.getAircraftIcaoCode().getCombinedIcaoStringWithKey(),
                                                    model.getLivery().getCombinedCodePlusInfo(),
                                                    model.getModelString(), model.getModelTypeAsString()));
        }
        return model;
    }

    CAircraftIcaoCode CAircraftMatcher::reverseLookupAircraftIcao(const QString &icaoDesignator, const CCallsign &callsign, CStatusMessageList *log)
    {
        Q_ASSERT_X(sApp, Q_FUNC_INFO, "Missing sApp");
        Q_ASSERT_X(sApp->getWebDataServices(), Q_FUNC_INFO, "No web services");

        const QString designator(icaoDesignator.trimmed().toUpper());
        CAircraftIcaoCodeList groupedIcaos = sApp->getWebDataServices()->getAircraftIcaoCodesForDesignator(icaoDesignator).groupByDesignatorAndManufacturer();
        if (groupedIcaos.isEmpty())
        {
            CAircraftIcaoCode icao(designator);

            // sometimes from network we receive something like "CESSNA C172"
            if (CAircraftIcaoCode::isValidDesignator(designator))
            {
                CMatchingUtils::addLogDetailsToList(log, callsign, QString("Reverse lookup of ICAO '%1' did not find anything, using smart search").arg(designator), CAircraftMatcher::getLogCategories());
                icao = sApp->getWebDataServices()->smartAircraftIcaoSelector(icaoDesignator);
            }
            else
            {
                CMatchingUtils::addLogDetailsToList(log, callsign, QString("Reverse lookup of invalid ICAO code '%1' did not find anything so far").arg(designator), CAircraftMatcher::getLogCategories());
                QStringList parts(icaoDesignator.split(' '));
                for (const QString &p : parts)
                {
                    CMatchingUtils::addLogDetailsToList(log, callsign, QString("Trying parts, now reverse lookup of ICAO '%1' using smart search").arg(p), CAircraftMatcher::getLogCategories());
                    icao = sApp->getWebDataServices()->smartAircraftIcaoSelector(p);
                    if (icao.isLoadedFromDb()) break;
                }
            }
            if (icao.isLoadedFromDb())
            {
                // smart search found DB data
                groupedIcaos = CAircraftIcaoCodeList({icao});
            }
            else
            {
                CMatchingUtils::addLogDetailsToList(log, callsign, QString("No DB data for ICAO '%1', valid ICAO?").arg(designator), CAircraftMatcher::getLogCategories());
                return CAircraftIcaoCode(icaoDesignator);
            }
        }

        if (groupedIcaos.size() < 1)
        {
            CMatchingUtils::addLogDetailsToList(log, callsign, QString("Reverse lookup of ICAO '%1'', nothing found").arg(designator), CAircraftMatcher::getLogCategories());
            return CAircraftIcaoCode(icaoDesignator);
        }
        else if (groupedIcaos.size() == 1)
        {
            const CAircraftIcaoCode icao(groupedIcaos.front());
            CMatchingUtils::addLogDetailsToList(log, callsign, QString("Reverse lookup of ICAO '%1'', found one manufacturer '%2'").arg(designator, icao.getDesignatorManufacturer()), CAircraftMatcher::getLogCategories());
            return icao;
        }
        else
        {
            // copy over those data I am sure about
            CAircraftIcaoCode icao = groupedIcaos.front();
            icao = CAircraftIcaoCode(icao.getDesignator(), icao.getIataCode(), "", "", icao.getWtc(), icao.isRealWorld(), icao.isLegacyAircraft(), icao.isMilitary(), 10);
            CMatchingUtils::addLogDetailsToList(log, callsign, QString("Reverse lookup of ICAO '%1'', found >1 manufacturer (ambiguous), using ICAO '%2'").arg(designator, icao.toQString()), CAircraftMatcher::getLogCategories());
            return icao;
        }
        return CAircraftIcaoCode(icaoDesignator);
    }

    CAirlineIcaoCode BlackCore::CAircraftMatcher::reverseLookupAirlineIcao(const QString &icaoDesignator, const CCallsign &callsign, CStatusMessageList *log)
    {
        Q_ASSERT_X(sApp, Q_FUNC_INFO, "Missing sApp");
        Q_ASSERT_X(sApp->getWebDataServices(), Q_FUNC_INFO, "No web services");

        const QString designator(icaoDesignator.trimmed().toUpper());
        const CAirlineIcaoCode icao = sApp->getWebDataServices()->smartAirlineIcaoSelector(designator, callsign);
        if (log)
        {
            if (icao.hasValidDbKey()) { CMatchingUtils::addLogDetailsToList(log, callsign, QString("Reverse lookup of airline '%1' and callsign '%2' found '%3'").arg(designator, callsign.asString(), icao.getDesignator()), CAircraftMatcher::getLogCategories()); }
            else { CMatchingUtils::addLogDetailsToList(log, callsign, QString("Reverse lookup of airline '%1' and callsign '%2', nothing found").arg(designator, callsign.asString()), CAircraftMatcher::getLogCategories()); }
        }
        return icao;
    }

    CLivery CAircraftMatcher::reverseLookupStandardLivery(const CAirlineIcaoCode &airline, const CCallsign &callsign, CStatusMessageList *log)
    {
        Q_ASSERT_X(sApp, Q_FUNC_INFO, "Missing sApp");
        Q_ASSERT_X(sApp->getWebDataServices(), Q_FUNC_INFO, "No web services");

        if (!airline.hasValidDesignator())
        {
            if (log) { CMatchingUtils::addLogDetailsToList(log, callsign, "Reverse lookup of standard livery skipped, no airline designator", CAircraftMatcher::getLogCategories(), CStatusMessage::SeverityWarning); }
            return CLivery();
        }

        const CLivery livery = sApp->getWebDataServices()->getStdLiveryForAirlineCode(airline);
        if (log)
        {
            if (livery.hasValidDbKey()) { CMatchingUtils::addLogDetailsToList(log, callsign, QString("Reverse lookup of standard livery for '%1' found '%2'").arg(airline.getDesignator(), livery.getCombinedCode()), CAircraftMatcher::getLogCategories()); }
            else { CMatchingUtils::addLogDetailsToList(log, callsign, QString("Not standard livery for airline '%1'").arg(airline.getDesignator()), CAircraftMatcher::getLogCategories()); }
        }
        return livery;
    }

    CAirlineIcaoCode CAircraftMatcher::callsignToAirline(const CCallsign &callsign, CStatusMessageList *log)
    {
        if (callsign.isEmpty() || !sApp || !sApp->getWebDataServices()) { return CAirlineIcaoCode(); }
        const CAirlineIcaoCode icao = sApp->getWebDataServices()->findBestMatchByCallsign(callsign);

        if (icao.hasValidDesignator())
        {
            CMatchingUtils::addLogDetailsToList(log, callsign, QString("Turned callsign %1 into airline %2").arg(callsign.asString(), icao.getDesignator()), getLogCategories());
        }
        else
        {
            CMatchingUtils::addLogDetailsToList(log, callsign, QString("Cannot turn callsign %1 into airline").arg(callsign.asString()), getLogCategories());
        }
        return icao;
    }

    int CAircraftMatcher::setModelSet(const CAircraftModelList &models, const CSimulatorInfo &simulatorHint)
    {
        CAircraftModelList modelsCleaned(models);
        const int r1 = modelsCleaned.removeAllWithoutModelString();
        const int r2 = modelsCleaned.removeIfExcluded();
        if ((r1 + r2) > 0)
        {
            CLogMessage(this).warning("Removed models for matcher, without string '%1', excluded '%2'") << r1 << r2;
        }
        if (modelsCleaned.isEmpty())
        {
            CLogMessage(this).error("No models for matching, swift without a model set will not work");
        }
        else
        {
            CLogMessage(this).info("Set %1 models in matcher, simulator '%2'") << modelsCleaned.size() << simulatorHint.toQString();
        }
        this->m_modelSet = modelsCleaned;
        this->m_simulator = simulatorHint;
        this->m_modelSetInfo = QString("Set: '%1' entries: %2").arg(simulatorHint.toQString()).arg(modelsCleaned.size());
        return models.size();
    }

    const CAircraftModel &CAircraftMatcher::getDefaultModel() const
    {
        return m_defaultModel;
    }

    void CAircraftMatcher::setDefaultModel(const BlackMisc::Simulation::CAircraftModel &defaultModel)
    {
        m_defaultModel = defaultModel;
        m_defaultModel.setModelType(CAircraftModel::TypeModelMatchingDefaultModel);
    }

    CMatchingStatistics CAircraftMatcher::getCurrentStatistics() const
    {
        return this->m_statistics;
    }

    void CAircraftMatcher::clearMatchingStatistics()
    {
        this->m_statistics.clear();
    }

    void CAircraftMatcher::evaluateStatisticsEntry(const QString &sessionId, const CCallsign &callsign, const QString &aircraftIcao, const QString &airlineIcao, const QString &livery)
    {
        Q_UNUSED(livery);
        Q_ASSERT_X(sApp && sApp->hasWebDataServices(), Q_FUNC_INFO, "Missing web data services");
        if (this->m_modelSet.isEmpty()) { return; } // ignore empty sets to not create silly stats
        if (sessionId.isEmpty()) { return; }
        if (aircraftIcao.isEmpty()) { return; }

        QString description;
        if (!sApp->getWebDataServices()->getAircraftIcaoCodeForDesignator(aircraftIcao).isLoadedFromDb())
        {
            description = QString("ICAO: '%1' not known, typo?").arg(aircraftIcao);
        }

        // resolve airline, mostly needed because of vPilot not sending airline icao codes in version 1
        QString airlineIcaoChecked(airlineIcao.trimmed().toUpper());
        if (airlineIcao.isEmpty())
        {
            const CAirlineIcaoCode al = CAircraftMatcher::reverseLookupAirlineIcao(airlineIcao, callsign);
            if (al.isLoadedFromDb())
            {
                airlineIcaoChecked = al.getDesignator();
            }
        }

        CMatchingStatisticsEntry::EntryType type = CMatchingStatisticsEntry::Missing;
        if (airlineIcaoChecked.isEmpty())
        {
            type = this->m_modelSet.containsModelsWithAircraftAndAirlineIcaoDesignator(aircraftIcao, airlineIcao) ?
                   CMatchingStatisticsEntry::Found :
                   CMatchingStatisticsEntry::Missing;
        }
        else
        {
            type = this->m_modelSet.containsModelsWithAircraftAndAirlineIcaoDesignator(aircraftIcao, airlineIcao) ?
                   CMatchingStatisticsEntry::Found :
                   CMatchingStatisticsEntry::Missing;

        }
        this->m_statistics.addAircraftAirlineCombination(type, sessionId, this->m_modelSetInfo, description, aircraftIcao, airlineIcao);
    }

    CAircraftModel CAircraftMatcher::getClosestMatchSearchImplementation(MatchingMode mode, const BlackMisc::Simulation::CAircraftModelList &modelSet, const CSimulatedAircraft &remoteAircraft, CStatusMessageList *log) const
    {
        BlackMisc::Simulation::CAircraftModelList matchedModels(modelSet);
        CAircraftModel matchedModel(remoteAircraft.getModel());
        // primary reduction
        bool reduced = false;
        do
        {
            // by livery, then by ICAO
            if (mode.testFlag(ByLivery))
            {
                matchedModels = ifPossibleReduceByLiveryAndIcaoCode(remoteAircraft, matchedModels, reduced, log);
                if (reduced) { break; }
            }
            else if (log)
            {
                CMatchingUtils::addLogDetailsToList(log, remoteAircraft, "Skipping livery reduction", getLogCategories());
            }

            // by ICAO data from set
            if (mode.testFlag(ByIcaoData))
            {
                // if already matched by livery skip
                matchedModels = ifPossibleReduceByIcaoData(remoteAircraft, matchedModels, false, reduced, log);
                if (reduced) { break; }
            }
            else if (log)
            {
                CMatchingUtils::addLogDetailsToList(log, remoteAircraft, "Skipping ICAO reduction", getLogCategories());
            }

            // family
            if (mode.testFlag(ByFamily))
            {
                QString family = remoteAircraft.getAircraftIcaoCode().getFamily();
                matchedModels = ifPossibleReduceByFamily(remoteAircraft, family, matchedModels, "real family", reduced, log);
                if (reduced) { break; }

                // scenario: the ICAO actually is the family
                family = remoteAircraft.getAircraftIcaoCodeDesignator();
                matchedModels = ifPossibleReduceByFamily(remoteAircraft, family, matchedModels, "ICAO treated as family", reduced, log);
                if (reduced) { break; }
            }
            else if (log)
            {
                CMatchingUtils::addLogDetailsToList(log, remoteAircraft, "Skipping family match", getLogCategories());
            }

            // combined code
            if (mode.testFlag(ByCombinedType))
            {
                matchedModels = ifPossibleReduceByCombinedCode(remoteAircraft, matchedModels, reduced, log);
                if (reduced) { break; }
            }
            else if (log)
            {
                CMatchingUtils::addLogDetailsToList(log, remoteAircraft, "Skipping combined code match", getLogCategories());
            }
        }
        while (false);

        // the last resort is to use the combined type
        if (mode.testFlag(ByCombinedType) && matchedModels.isEmpty() && remoteAircraft.getModel().getAircraftIcaoCode().hasValidCombinedType())
        {
            const QString combinedType(remoteAircraft.getModel().getAircraftIcaoCode().getCombinedType());
            CMatchingUtils::addLogDetailsToList(log, remoteAircraft, "Searching by combined type '" + combinedType + "'", getLogCategories());
            matchedModels = matchedModels.findByCombinedType(combinedType);
            if (!matchedModels.isEmpty())
            {
                CMatchingUtils::addLogDetailsToList(log, remoteAircraft, "Found " + QString::number(matchedModels.size()) + " by combined type '" + combinedType + "'", getLogCategories());
            }
        }

        // here we have a list of possible models, we reduce/refine further
        if (!matchedModels.isEmpty())
        {
            const bool military = remoteAircraft.getModel().isMilitary();
            matchedModels = ifPossibleReduceByManufacturer(remoteAircraft, matchedModels, "2nd pass", reduced, log);
            matchedModels = ifPossibleReduceByMilitaryFlag(remoteAircraft, military, matchedModels, reduced, log);
        }

        // expect first to be the right one in order
        if (matchedModels.isEmpty())
        {
            matchedModel = getDefaultModel();
            CMatchingUtils::addLogDetailsToList(log, remoteAircraft, "Using default model (nothing else found)", getLogCategories());
        }
        else
        {
            matchedModel = matchedModels.front();
            CMatchingUtils::addLogDetailsToList(log, remoteAircraft, "Using first of " + QString::number(matchedModels.size()) + " models", getLogCategories());
        }
        return matchedModel;
    }

    CAircraftModel CAircraftMatcher::getClosestMatchScoreImplementation(MatchingMode mode, const CAircraftModelList &modelSet, const CSimulatedAircraft &remoteAircraft, CStatusMessageList *log) const
    {
        Q_UNUSED(mode);
        CAircraftModelList usedModelSet;
        static const bool NoZeroScores = true;
        static const bool PreferColorLiveries = true;
        static const bool DoNotMindAboutColorLiveries = false;

        // VTOL
        ScoredModels map;
        if (remoteAircraft.isVtol() && modelSet.contains(&CAircraftModel::isVtol, true))
        {
            usedModelSet = modelSet.findBy(&CAircraftModel::isVtol, true);
        }
        else
        {
            usedModelSet = modelSet;
        }

        // first decide what set to use for scoring, it should not be too large
        if (remoteAircraft.hasAircraftAndAirlineDesignator() && usedModelSet.containsModelsWithAircraftAndAirlineIcaoDesignator(remoteAircraft.getAircraftIcaoCodeDesignator(), remoteAircraft.getAirlineIcaoCodeDesignator()))
        {
            // Aircraft and airline ICAO
            const CAircraftModelList byAircraftAndAirline(usedModelSet.findByIcaoDesignators(remoteAircraft.getAircraftIcaoCode(), remoteAircraft.getAirlineIcaoCode()));
            CMatchingUtils::addLogDetailsToList(log, remoteAircraft, QString("Using reduced set of %1 models by aircraft/airline ICAOs '%2'/'%3' for scoring").arg(byAircraftAndAirline.size()).arg(remoteAircraft.getAircraftIcaoCode().getDesignatorDbKey(), remoteAircraft.getAirlineIcaoCode().getVDesignatorDbKey()), getLogCategories());

            // we have airline and aircraft ICAO, so use airline liveries
            map = byAircraftAndAirline.scoreFull(remoteAircraft.getModel(), DoNotMindAboutColorLiveries, NoZeroScores, log);
        }
        else if (remoteAircraft.hasAircraftDesignator() && usedModelSet.contains(&CAircraftModel::getAircraftIcaoCodeDesignator, remoteAircraft.getAircraftIcaoCodeDesignator()))
        {
            // Aircraft ICAO only
            const CAircraftModelList byAircraft(usedModelSet.findByIcaoDesignators(remoteAircraft.getAircraftIcaoCode(), CAirlineIcaoCode()));
            CMatchingUtils::addLogDetailsToList(log, remoteAircraft, QString("Using reduced set of %1 models by aircraft ICAO '%2' (no airline) for scoring").arg(byAircraft.size()).arg(remoteAircraft.getAircraftIcaoCode().getDesignatorDbKey()), getLogCategories());

            // we do not have an airline for that aircraft, prefer color liveries
            // this could also become a setting
            map = byAircraft.scoreFull(remoteAircraft.getModel(), PreferColorLiveries, NoZeroScores, log);
        }
        else if (remoteAircraft.getAircraftIcaoCode().hasValidCombinedType() && usedModelSet.containsCombinedType(remoteAircraft.getAircraftIcaoCode().getCombinedType()))
        {
            const CAircraftModelList byAircraft(usedModelSet.findByCombinedType(remoteAircraft.getAircraftIcaoCode().getCombinedType()));
            CMatchingUtils::addLogDetailsToList(log, remoteAircraft, QString("Using reduced set of %1 models by combined type '%2' for scoring").arg(byAircraft.size()).arg(remoteAircraft.getAircraftIcaoCode().getCombinedType()), getLogCategories());

            // No aircraft / airline ICAO, we prefer color liveries
            map = byAircraft.scoreFull(remoteAircraft.getModel(), PreferColorLiveries, NoZeroScores, log);
        }
        else
        {
            CMatchingUtils::addLogDetailsToList(log, remoteAircraft, QString("Using set with %1 models").arg(usedModelSet.size()), getLogCategories());

            // Poor men`s matching
            map = usedModelSet.scoreFull(remoteAircraft.getModel(), PreferColorLiveries, NoZeroScores, log);
        }

        CAircraftModel matchedModel;
        if (map.isEmpty())
        {
            matchedModel = getDefaultModel();
            CMatchingUtils::addLogDetailsToList(log, remoteAircraft, "Using default model (nothing else found)", getLogCategories());
        }
        else
        {
            const int maxScore = map.lastKey();
            const CAircraftModelList maxScoreAircraft(map.values(maxScore));
            CMatchingUtils::addLogDetailsToList(log, remoteAircraft, QString("Scores: %1").arg(scoresToString(map).replace("\n", "<br>")), getLogCategories());
            CMatchingUtils::addLogDetailsToList(log, remoteAircraft, QString("Scoring with score %1 out of %2 models yielded %3 models").arg(maxScore).arg(map.size()).arg(maxScoreAircraft.size()), getLogCategories());

            const CAircraftModel pickedModel = (maxScoreAircraft.size() > 1) ?
                                               maxScoreAircraft.randomElements(1).front() :
                                               maxScoreAircraft.front();
            CMatchingUtils::addLogDetailsToList(log, remoteAircraft, QString("Picked with score %1: %2").arg(maxScore).arg(pickedModel.toQString()), getLogCategories());
            return pickedModel;
        }
        return matchedModel;
    }

    CAircraftModel CAircraftMatcher::matchByExactModelString(const CSimulatedAircraft &remoteAircraft, const CAircraftModelList &models, CStatusMessageList *log)
    {
        if (remoteAircraft.getModelString().isEmpty())
        {
            if (log) { CMatchingUtils::addLogDetailsToList(log, remoteAircraft, "No model string, no exact match possible"); }
            return CAircraftModel();
        }

        CAircraftModel model = models.findFirstByModelStringOrDefault(remoteAircraft.getModelString());
        if (log)
        {
            if (model.hasModelString())
            {
                CMatchingUtils::addLogDetailsToList(log, remoteAircraft, QString("Found exact match for '%1'").arg(model.getModelString()));
            }
            else
            {
                CMatchingUtils::addLogDetailsToList(log, remoteAircraft, QString("No exact match for '%1'").arg(remoteAircraft.getModelString()));
            }
        }
        model.setModelType(CAircraftModel::TypeModelMatching);
        model.setCallsign(remoteAircraft.getCallsign());
        return model;
    }

    CAircraftModelList CAircraftMatcher::ifPossibleReduceByLiveryAndIcaoCode(const CSimulatedAircraft &remoteAircraft, const CAircraftModelList &inList, bool &reduced, CStatusMessageList *log)
    {
        reduced = false;
        if (!remoteAircraft.getLivery().hasCombinedCode())
        {
            if (log) { CMatchingUtils::addLogDetailsToList(log, remoteAircraft, "No livery code, no reduction possible", getLogCategories()); }
            return inList;
        }

        const CAircraftModelList byLivery(
            inList.findByAircraftDesignatorAndLiveryCombinedCode(
                remoteAircraft.getLivery().getCombinedCode(),
                remoteAircraft.getAircraftIcaoCodeDesignator()
            ));

        if (byLivery.isEmpty())
        {
            if (log) { CMatchingUtils::addLogDetailsToList(log, remoteAircraft, "Not found by livery code " + remoteAircraft.getLivery().getCombinedCode(), getLogCategories()); }
            return inList;
        }
        reduced = true;
        return byLivery;
    }

    CAircraftModelList CAircraftMatcher::ifPossibleReduceByIcaoData(const CSimulatedAircraft &remoteAircraft, const CAircraftModelList &inList, bool ignoreAirline, bool &reduced, CStatusMessageList *log)
    {
        reduced = false;
        if (inList.isEmpty())
        {
            if (log) { CMatchingUtils::addLogDetailsToList(log, remoteAircraft, "Empty list, skipping step", getLogCategories()); }
            return inList;
        }

        if (!remoteAircraft.hasAircraftDesignator())
        {
            if (log) { CMatchingUtils::addLogDetailsToList(log, remoteAircraft, "No aircraft designator, skipping step", getLogCategories()); }
            return inList;
        }

        BlackMisc::Simulation::CAircraftModelList searchModels(inList.findByIcaoDesignators(
                    remoteAircraft.getAircraftIcaoCode(),
                    ignoreAirline ? CAirlineIcaoCode() : remoteAircraft.getAirlineIcaoCode()));

        const bool searchModelsEmpty = searchModels.isEmpty();
        if (!searchModelsEmpty)
        {
            if (log)
            {
                CMatchingUtils::addLogDetailsToList(log, remoteAircraft,
                                                    "Possible aircraft " + QString::number(searchModels.size()) +
                                                    ", found by ICAO " + remoteAircraft.getAircraftIcaoCodeDesignator() + " " +
                                                    (ignoreAirline ? "" : remoteAircraft.getAirlineIcaoCodeDesignator()), getLogCategories());
            }
            reduced = true;
            return searchModels;
        }
        else
        {
            if (!ignoreAirline && remoteAircraft.hasAircraftAndAirlineDesignator())
            {
                // we have searched by aircraft and airline, but not found anything
                if (log)
                {
                    CMatchingUtils::addLogDetailsToList(log, remoteAircraft,
                                                        "Not found by ICAO '" +
                                                        remoteAircraft.getAircraftIcaoCodeDesignator() + "' '" + remoteAircraft.getAirlineIcaoCodeDesignator() +
                                                        "', relaxing to only ICAO " + remoteAircraft.getAircraftIcaoCodeDesignator());
                }
                // recursive lookup by ignoring airline
                return ifPossibleReduceByIcaoData(remoteAircraft, inList, true, reduced, log);
            }
            if (log) { CMatchingUtils::addLogDetailsToList(log, remoteAircraft, "Not found by ICAO " + remoteAircraft.getAircraftIcaoCodeDesignator() + " " + remoteAircraft.getAirlineIcaoCodeDesignator()); }
        }
        return inList;
    }

    CAircraftModelList CAircraftMatcher::ifPossibleReduceByFamily(const CSimulatedAircraft &remoteAircraft, const QString &family, const CAircraftModelList &inList, const QString &hint, bool &reduced, CStatusMessageList *log)
    {
        // Use an algorithm to find the best match
        reduced = true;
        if (family.isEmpty())
        {
            if (log) { CMatchingUtils::addLogDetailsToList(log, remoteAircraft, "No family, skipping step (" + hint + ")", getLogCategories()); }
            return inList;
        }
        if (inList.isEmpty())
        {
            if (log) { CMatchingUtils::addLogDetailsToList(log, remoteAircraft, "No models for family match (" + hint + ")", getLogCategories()); }
            return inList;
        }

        CAircraftModelList found(inList.findByFamily(family));
        if (found.isEmpty())
        {
            if (log) { CMatchingUtils::addLogDetailsToList(log, remoteAircraft, "Not found by family " + family + " (" + hint + ")"); }
            return inList;
        }

        reduced = true;
        CMatchingUtils::addLogDetailsToList(log, remoteAircraft, "Found by family " + family + " (" + hint + ") size " + QString::number(found.size()), getLogCategories());
        return found;
    }

    CAircraftModelList CAircraftMatcher::ifPossibleReduceByManufacturer(const CSimulatedAircraft &remoteAircraft, const CAircraftModelList &inList, const QString &info, bool &reduced, CStatusMessageList *log)
    {
        reduced = false;
        if (inList.isEmpty())
        {
            if (log) { CMatchingUtils::addLogDetailsToList(log , remoteAircraft, info + " " + "Empty input list, cannot reduce", getLogCategories()); }
            return inList;
        }

        const QString m = remoteAircraft.getAircraftIcaoCode().getManufacturer();
        if (m.isEmpty())
        {
            if (log) { CMatchingUtils::addLogDetailsToList(log , remoteAircraft, info + " No manufacturer, cannot reduce " + QString::number(inList.size()) +  " entries", getLogCategories()); }
            return inList;
        }

        const CAircraftModelList outList(inList.findByManufacturer(m));
        if (outList.isEmpty())
        {
            if (log) { CMatchingUtils::addLogDetailsToList(log , remoteAircraft, info + " Not found " + m + ", cannot reduce", getLogCategories()); }
            return inList;
        }

        if (log) { CMatchingUtils::addLogDetailsToList(log , remoteAircraft, info + " Reduced by " + m + " results: " + QString::number(outList.size()), getLogCategories()); }
        reduced = true;
        return outList;
    }

    CAircraftModelList CAircraftMatcher::ifPossibleReduceByAirline(const CSimulatedAircraft &remoteAircraft, const CAircraftModelList &inList, const QString &info, bool &reduced, CStatusMessageList *log)
    {
        reduced = false;
        if (inList.isEmpty())
        {
            if (log) { CMatchingUtils::addLogDetailsToList(log , remoteAircraft, info + " " + "Empty input list, cannot reduce", getLogCategories()); }
            return inList;
        }

        if (!remoteAircraft.hasAirlineDesignator())
        {
            if (log) { CMatchingUtils::addLogDetailsToList(log , remoteAircraft, info + " " + "No airline, cannot reduce " + QString::number(inList.size()) +  " entries", getLogCategories()); }
            return inList;
        }

        const CAircraftModelList outList(inList.findByIcaoDesignators(CAircraftIcaoCode(), remoteAircraft.getAirlineIcaoCode()));
        if (outList.isEmpty())
        {
            if (log) { CMatchingUtils::addLogDetailsToList(log , remoteAircraft, info + " Cannot reduce by " + remoteAircraft.getAirlineIcaoCodeDesignator() + " results: " + QString::number(outList.size()), getLogCategories()); }
            return inList;
        }

        if (log) { CMatchingUtils::addLogDetailsToList(log , remoteAircraft, info + " Reduced reduce by " + remoteAircraft.getAirlineIcaoCodeDesignator() + " to " + QString::number(outList.size()), getLogCategories()); }
        reduced = true;
        return outList;
    }

    CAircraftModelList CAircraftMatcher::ifPossibleReduceByCombinedCode(const CSimulatedAircraft &remoteAircraft, const CAircraftModelList &inList, bool &reduced, CStatusMessageList *log)
    {
        reduced = false;
        if (!remoteAircraft.getAircraftIcaoCode().hasValidCombinedType())
        {
            if (log) { CMatchingUtils::addLogDetailsToList(log, remoteAircraft, "No valid combined code", getLogCategories()); }
            return inList;
        }

        const QString cc = remoteAircraft.getAircraftIcaoCode().getCombinedType();
        CAircraftModelList byCombinedCode(inList.findByCombinedType(cc));
        if (byCombinedCode.isEmpty())
        {
            if (log) { CMatchingUtils::addLogDetailsToList(log, remoteAircraft, "Not found by combined code " + cc, getLogCategories()); }
            return inList;
        }

        if (log) { CMatchingUtils::addLogDetailsToList(log, remoteAircraft, "Found by combined code " + cc + ", possible " + QString::number(byCombinedCode.size()), getLogCategories()); }
        if (byCombinedCode.size() > 1)
        {
            byCombinedCode = ifPossibleReduceByAirline(remoteAircraft, byCombinedCode, "Combined code", reduced, log);
            byCombinedCode = ifPossibleReduceByManufacturer(remoteAircraft, byCombinedCode, "Combined code", reduced, log);
            reduced = true;
        }
        return byCombinedCode;
    }

    CAircraftModelList CAircraftMatcher::ifPossibleReduceByMilitaryFlag(const BlackMisc::Simulation::CSimulatedAircraft &remoteAircraft, bool military, const CAircraftModelList &inList, bool &reduced, CStatusMessageList *log)
    {
        reduced = false;
        CAircraftModelList byMilitaryFlag(inList.findByMilitaryFlag(military));
        const QString mil(military ? "military" : "civilian");
        if (byMilitaryFlag.isEmpty())
        {
            if (log) { CMatchingUtils::addLogDetailsToList(log, remoteAircraft, "Not found by " + mil, getLogCategories()); }
            return inList;
        }

        if (log)
        {
            if (inList.size() > byMilitaryFlag.size())
            {
                CMatchingUtils::addLogDetailsToList(log, remoteAircraft, "Reduced to " + mil + " aircraft, size " + QString::number(byMilitaryFlag.size()), getLogCategories());
            }
            else
            {
                CMatchingUtils::addLogDetailsToList(log, remoteAircraft, "Not reduced by " + mil + ", size " + QString::number(byMilitaryFlag.size()), getLogCategories());
            }
        }
        return byMilitaryFlag;
    }

    QString CAircraftMatcher::scoresToString(const ScoredModels &scores, int lastElements)
    {
        if (scores.isEmpty()) { return ""; }
        QMapIterator<int, CAircraftModel> i(scores);
        i.toBack();
        int c = 0;
        QString str;
        while (i.hasPrevious() && c++ < lastElements)
        {
            i.previous();
            const CAircraftModel m(i.value());
            if (!str.isEmpty())
            {
                str += '\n';
            }
            str += QString::number(c) %
                   QLatin1String(": score: ") %
                   QString::number(i.key()) %
                   QLatin1String(" model: '") %
                   m.getModelString() %
                   QLatin1String("' aircraft: '") %
                   m.getAircraftIcaoCodeDesignator() %
                   QLatin1String("' livery: '") %
                   m.getLivery().getCombinedCodePlusInfo() %
                   QLatin1Char('\'');
        }
        return str;
    }
} // namespace

/* Copyright (C) 2015
 * swift Project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file

#ifndef BLACKCORE_CONTEXT_CONTEXTSIMULATOR_EMPTY_H
#define BLACKCORE_CONTEXT_CONTEXTSIMULATOR_EMPTY_H

#include "blackcoreexport.h"
#include "contextsimulator.h"
#include "blackmisc/logmessage.h"

namespace BlackCore
{
    namespace Context
    {
        //! Empty context, used during shutdown/initialization
        class BLACKCORE_EXPORT CContextSimulatorEmpty : public IContextSimulator
        {
            Q_OBJECT

        public:
            //! Constructor
            CContextSimulatorEmpty(CCoreFacade *runtime) : IContextSimulator(CCoreFacadeConfig::NotUsed, runtime) {}

        public slots:
            //! \copydoc IContextSimulator::getSimulatorPluginInfo()
            virtual BlackMisc::Simulation::CSimulatorPluginInfo getSimulatorPluginInfo() const override
            {
                logEmptyContextWarning(Q_FUNC_INFO);
                return BlackMisc::Simulation::CSimulatorPluginInfo();
            }

            //! \copydoc IContextSimulator::getAvailableSimulatorPlugins()
            virtual BlackMisc::Simulation::CSimulatorPluginInfoList getAvailableSimulatorPlugins() const override
            {
                logEmptyContextWarning(Q_FUNC_INFO);
                return BlackMisc::Simulation::CSimulatorPluginInfoList();
            }

            //! \copydoc IContextSimulator::startSimulatorPlugin()
            virtual bool startSimulatorPlugin(const BlackMisc::Simulation::CSimulatorPluginInfo &simulatorInfo) override
            {
                Q_UNUSED(simulatorInfo);
                logEmptyContextWarning(Q_FUNC_INFO);
                return false;
            }

            //! \copydoc IContextSimulator::getSimulatorStatus()
            virtual int getSimulatorStatus() const override
            {
                logEmptyContextWarning(Q_FUNC_INFO);
                return 0;
            }

            //! \copydoc IContextSimulator::stopSimulatorPlugin
            virtual void stopSimulatorPlugin(const BlackMisc::Simulation::CSimulatorPluginInfo &simulatorInfo) override
            {
                Q_UNUSED(simulatorInfo);
                logEmptyContextWarning(Q_FUNC_INFO);
            }

            //! \copydoc IContextSimulator::getAirportsInRange
            virtual BlackMisc::Aviation::CAirportList getAirportsInRange() const override
            {
                logEmptyContextWarning(Q_FUNC_INFO);
                return BlackMisc::Aviation::CAirportList();
            }

            //! \copydoc IContextSimulator::getModelSet
            virtual BlackMisc::Simulation::CAircraftModelList getModelSet() const override
            {
                logEmptyContextWarning(Q_FUNC_INFO);
                return BlackMisc::Simulation::CAircraftModelList();
            }

            //! \copydoc IContextSimulator::simulatorsWithInitializedModelSet
            virtual BlackMisc::Simulation::CSimulatorInfo simulatorsWithInitializedModelSet() const override
            {
                logEmptyContextWarning(Q_FUNC_INFO);
                return BlackMisc::Simulation::CSimulatorInfo();
            }

            //! \copydoc IContextSimulator::getModelSetStrings
            virtual QStringList getModelSetStrings() const override
            {
                logEmptyContextWarning(Q_FUNC_INFO);
                return QStringList();
            }

            //! \copydoc IContextSimulator::getModelSetStrings
            virtual QStringList getModelSetCompleterStrings(bool sorted) const override
            {
                Q_UNUSED(sorted);
                logEmptyContextWarning(Q_FUNC_INFO);
                return QStringList();
            }

            //! \copydoc IContextSimulator::getModelSetModelsStartingWith
            virtual BlackMisc::Simulation::CAircraftModelList getModelSetModelsStartingWith(const QString modelString) const override
            {
                Q_UNUSED(modelString);
                logEmptyContextWarning(Q_FUNC_INFO);
                return BlackMisc::Simulation::CAircraftModelList();
            }

            //! \copydoc IContextSimulator::getModelSetCount
            virtual int getModelSetCount() const override
            {
                logEmptyContextWarning(Q_FUNC_INFO);
                return 0;
            }

            //! \copydoc IContextSimulator::getSimulatorInternals
            virtual BlackMisc::Simulation::CSimulatorInternals getSimulatorInternals() const override
            {
                logEmptyContextWarning(Q_FUNC_INFO);
                return BlackMisc::Simulation::CSimulatorInternals();
            }

            //! \copydoc IContextSimulator::setTimeSynchronization
            virtual bool setTimeSynchronization(bool enable, const BlackMisc::PhysicalQuantities::CTime &offset) override
            {
                Q_UNUSED(enable);
                Q_UNUSED(offset);
                logEmptyContextWarning(Q_FUNC_INFO);
                return false;
            }

            //! \copydoc IContextSimulator::isTimeSynchronized
            virtual bool isTimeSynchronized() const override
            {
                logEmptyContextWarning(Q_FUNC_INFO);
                return false;
            }

            //! \copydoc ISimulator::getInterpolationAndRenderingSetup
            virtual BlackMisc::Simulation::CInterpolationAndRenderingSetup getInterpolationAndRenderingSetup() const override
            {
                logEmptyContextWarning(Q_FUNC_INFO);
                return BlackMisc::Simulation::CInterpolationAndRenderingSetup();
            }

            //! \copydoc ISimulator::setInterpolationAndRenderingSetup
            virtual void setInterpolationAndRenderingSetup(const BlackMisc::Simulation::CInterpolationAndRenderingSetup &setup) override
            {
                Q_UNUSED(setup);
                logEmptyContextWarning(Q_FUNC_INFO);
            }

            //! \copydoc IContextSimulator::getTimeSynchronizationOffset
            virtual BlackMisc::PhysicalQuantities::CTime getTimeSynchronizationOffset() const override
            {
                logEmptyContextWarning(Q_FUNC_INFO);
                return BlackMisc::PhysicalQuantities::CTime();
            }

            //! \copydoc IContextSimulator::iconForModel
            virtual BlackMisc::CPixmap iconForModel(const QString &modelString) const override
            {
                Q_UNUSED(modelString);
                logEmptyContextWarning(Q_FUNC_INFO);
                return BlackMisc::CPixmap();
            }

            //! \copydoc IContextSimulator::highlightAircraft
            virtual void highlightAircraft(const BlackMisc::Simulation::CSimulatedAircraft &aircraftToHighlight, bool enableHighlight, const BlackMisc::PhysicalQuantities::CTime &displayTime) override
            {
                Q_UNUSED(aircraftToHighlight);
                Q_UNUSED(enableHighlight);
                Q_UNUSED(displayTime);
                logEmptyContextWarning(Q_FUNC_INFO);
            }

            //! \copydoc IContextSimulator::resetToModelMatchingAircraft
            virtual bool resetToModelMatchingAircraft(const BlackMisc::Aviation::CCallsign &callsign) override
            {
                Q_UNUSED(callsign);
                return false;
            }

            //! \copydoc IContextSimulator::requestWeatherGrid
            virtual void requestWeatherGrid(const BlackMisc::Weather::CWeatherGrid &weatherGrid, const BlackMisc::CIdentifier &identifier) override
            {
                Q_UNUSED(weatherGrid);
                Q_UNUSED(identifier);
                logEmptyContextWarning(Q_FUNC_INFO);
            }

            //! \copydoc IContextSimulator::getMatchingMessages
            virtual BlackMisc::CStatusMessageList getMatchingMessages(const BlackMisc::Aviation::CCallsign &callsign) const override
            {
                Q_UNUSED(callsign);
                logEmptyContextWarning(Q_FUNC_INFO);
                return BlackMisc::CStatusMessageList();
            }

            //! \copydoc IContextSimulator::enableMatchingMessages
            virtual void enableMatchingMessages(bool enable) override
            {
                Q_UNUSED(enable);
                logEmptyContextWarning(Q_FUNC_INFO);
            }

            //! \copydoc IContextSimulator::isMatchingMessagesEnabled
            virtual bool isMatchingMessagesEnabled() const override
            {
                logEmptyContextWarning(Q_FUNC_INFO);
                return false;
            }
        };
    } // namespace
} // namespace

#endif // guard

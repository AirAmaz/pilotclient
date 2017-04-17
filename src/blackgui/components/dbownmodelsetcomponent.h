/* Copyright (C) 2016
 * swift project Community / Contributors
 *
 * This file is part of swift Project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file

#ifndef BLACKGUI_COMPONENTS_DBOWNMODELSETCOMPONENT_H
#define BLACKGUI_COMPONENTS_DBOWNMODELSETCOMPONENT_H

#include "blackgui/components/dbmappingcomponentaware.h"
#include "blackgui/menus/menudelegate.h"
#include "blackmisc/simulation/modelsettings.h"
#include "blackmisc/simulation/aircraftmodelinterfaces.h"
#include "blackmisc/simulation/aircraftmodellist.h"
#include "blackmisc/simulation/aircraftmodelsetloader.h"
#include "blackmisc/simulation/simulatorinfo.h"
#include "blackmisc/statusmessage.h"

#include <QFrame>
#include <QList>
#include <QObject>
#include <QScopedPointer>

class QAction;
class QWidget;

namespace Ui { class CDbOwnModelSetComponent; }
namespace BlackMisc { namespace Simulation { class CAircraftModel; } }
namespace BlackGui
{
    namespace Menus { class CMenuActions; }
    namespace Views { class CAircraftModelView; }
    namespace Components
    {
        class CDbMappingComponent;
        class CDbOwnModelSetDialog;

        /*!
         * Handling of the own model set
         */
        class CDbOwnModelSetComponent :
            public QFrame,
            public CDbMappingComponentAware,
            public BlackMisc::Simulation::IModelsSetable,
            public BlackMisc::Simulation::IModelsUpdatable,
            public BlackMisc::Simulation::IModelsPerSimulatorSetable,
            public BlackMisc::Simulation::IModelsPerSimulatorUpdatable,
            public BlackMisc::Simulation::ISimulatorSelectable
        {
            Q_OBJECT
            Q_INTERFACES(BlackGui::Components::CDbMappingComponentAware)
            Q_INTERFACES(BlackMisc::Simulation::IModelsSetable)
            Q_INTERFACES(BlackMisc::Simulation::IModelsUpdatable)
            Q_INTERFACES(BlackMisc::Simulation::IModelsPerSimulatorSetable)
            Q_INTERFACES(BlackMisc::Simulation::IModelsPerSimulatorUpdatable)
            Q_INTERFACES(BlackMisc::Simulation::ISimulatorSelectable)

        public:
            //! Constructor
            explicit CDbOwnModelSetComponent(QWidget *parent = nullptr);

            //! Destructor
            virtual ~CDbOwnModelSetComponent();

            //! Corresponding view
            Views::CAircraftModelView *view() const;

            //! Add to model set
            BlackMisc::CStatusMessage addToModelSet(const BlackMisc::Simulation::CAircraftModelList &models, const BlackMisc::Simulation::CSimulatorInfo &simulator);

            //! Add to model set
            BlackMisc::CStatusMessage addToModelSet(const BlackMisc::Simulation::CAircraftModel &model, const BlackMisc::Simulation::CSimulatorInfo &simulator);

            //! Current model set for simulator CDbOwnModelSetComponent::getModelSetSimulator
            const BlackMisc::Simulation::CAircraftModelList &getModelSet() const;

            //! Model set is for simulator
            const BlackMisc::Simulation::CSimulatorInfo getModelSetSimulator() const;

            //! Simulator
            void setModelSetSimulator(const BlackMisc::Simulation::CSimulatorInfo &simulator);

            //! \copydoc CDbMappingComponentAware::setMappingComponent
            virtual void setMappingComponent(CDbMappingComponent *component) override;

            //! \name Implementations of the models interfaces
            //! @{
            virtual void setModels(const BlackMisc::Simulation::CAircraftModelList &models) override  { this->setModelSet(models, this->getModelSetSimulator()); }
            virtual void updateModels(const BlackMisc::Simulation::CAircraftModelList &models) override  { this->replaceOrAddModelSet(models, this->getModelSetSimulator()); }
            virtual void setModels(const BlackMisc::Simulation::CAircraftModelList &models, const BlackMisc::Simulation::CSimulatorInfo &simulator) override  { this->setModelSet(models, simulator); }
            virtual void updateModels(const BlackMisc::Simulation::CAircraftModelList &models, const BlackMisc::Simulation::CSimulatorInfo &simulator) override  { this->replaceOrAddModelSet(models, simulator); }
            virtual BlackMisc::Simulation::CSimulatorInfo getSelectedSimulator() const override { return this->getModelSetSimulator(); }
            //! @}

        public slots:
            //! Set the model set for a given simulator
            void setModelSet(const BlackMisc::Simulation::CAircraftModelList &models, const BlackMisc::Simulation::CSimulatorInfo &simulator);

            //! Replace or add models provided for a given simulator
            void replaceOrAddModelSet(const BlackMisc::Simulation::CAircraftModelList &models, const BlackMisc::Simulation::CSimulatorInfo &simulator);

        private slots:
            //! Tab has been changed
            void ps_tabIndexChanged(int index);

            //! Button was clicked
            void ps_buttonClicked();

            //! Change current simulator
            void ps_changeSimulator(const BlackMisc::Simulation::CSimulatorInfo &simulator);

            //! Simulator has been changed (in loader)
            void ps_onSimulatorChanged(const BlackMisc::Simulation::CSimulatorInfo &simulator);

            //! View has changed row count
            void ps_onRowCountChanged(int count, bool withFilter);

            //! JSON data have been loaded from disk
            void ps_onJsonDataLoaded(const BlackMisc::Simulation::CSimulatorInfo &simulator);

            //! Preferences changed
            void ps_distributorPreferencesChanged();

            //! Model settings changed
            void ps_modelSettingsChanged();

            //! Model (of view) has been changed
            void ps_viewModelChanged();

        private:
            //! Default file name
            void setSaveFileName(const BlackMisc::Simulation::CSimulatorInfo &sim);

            //! Update view to current models
            void updateViewToCurrentModels();

            //! Create new set
            void createNewSet();

            //! Show the airline/aircraft matrix
            void showAirlineAircraftMatrix() const;

            //! Update distributor order
            void updateDistributorOrder(const BlackMisc::Simulation::CSimulatorInfo &simulator);

            QScopedPointer<Ui::CDbOwnModelSetComponent>    ui;
            QScopedPointer<CDbOwnModelSetDialog>           m_modelSetDialog;
            BlackMisc::Simulation::CAircraftModelSetLoader m_modelSetLoader { this };
            BlackMisc::CSettingReadOnly<BlackMisc::Simulation::TDistributorListPreferences> m_distributorPreferences { this, &CDbOwnModelSetComponent::ps_distributorPreferencesChanged }; //!< distributor preferences
            BlackMisc::CSettingReadOnly<BlackMisc::Simulation::TModel> m_modelSettings { this, &CDbOwnModelSetComponent::ps_modelSettingsChanged }; //!< settings for models

            // -------------------------- custom menus -----------------------------------

            //! The menu for loading and handling own models for mapping tasks
            //! \note This is specific for that very component
            class CLoadModelsMenu : public BlackGui::Menus::IMenuDelegate
            {
            public:
                //! Constructor
                CLoadModelsMenu(CDbOwnModelSetComponent *ownModelSetComponent, bool separator = true) :
                    BlackGui::Menus::IMenuDelegate(ownModelSetComponent, separator)
                {}

                //! \copydoc IMenuDelegate::customMenu
                virtual void customMenu(BlackGui::Menus::CMenuActions &menuActions) override;

            private:
                QList<QAction *> m_setActions;
                QList<QAction *> m_setNewActions;
            };
        };
    } // ns
} // ns

#endif // guard

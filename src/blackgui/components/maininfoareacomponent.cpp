/* Copyright (C) 2014
 * swift project Community / Contributors
 *
 * This file is part of swift Project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "maininfoareacomponent.h"
#include "ui_maininfoareacomponent.h"
#include "allmaininfoareacomponents.h"
#include "../stylesheetutility.h"
#include "../guiutility.h"
#include "blackmisc/icons.h"

using namespace BlackMisc;
using namespace BlackGui;

namespace BlackGui
{
    namespace Components
    {
        CMainInfoAreaComponent::CMainInfoAreaComponent(QWidget *parent) :
            CInfoArea(parent),
            ui(new Ui::CMainInfoAreaComponent)
        {
            ui->setupUi(this);
            initInfoArea(); // init base class
            this->setWindowIcon(CIcons::swift24());
        }

        CMainInfoAreaComponent::~CMainInfoAreaComponent()
        { }

        CAtcStationComponent *CMainInfoAreaComponent::getAtcStationComponent()
        {
            return this->ui->comp_AtcStations;
        }

        CAircraftComponent *CMainInfoAreaComponent::getAircraftComponent()
        {
            return this->ui->comp_Aircraft;
        }

        CMappingComponent *CMainInfoAreaComponent::getMappingComponet()
        {
            return this->ui->comp_Mappings;
        }

        CUserComponent *CMainInfoAreaComponent::getUserComponent()
        {
            return this->ui->comp_Users;
        }

        CCockpitComponent *CMainInfoAreaComponent::getCockpitComponent()
        {
            return this->ui->comp_Cockpit;
        }

        CFlightPlanComponent *CMainInfoAreaComponent::getFlightPlanComponent()
        {
            return this->ui->comp_FlightPlan;
        }

        CSettingsComponent *CMainInfoAreaComponent::getSettingsComponent()
        {
            return this->ui->comp_Settings;
        }

        CLogComponent *CMainInfoAreaComponent::getLogComponent()
        {
            return this->ui->comp_Log;
        }

        CSimulatorComponent *CMainInfoAreaComponent::getSimulatorComponent()
        {
            return this->ui->comp_Simulator;
        }

        CTextMessageComponent *CMainInfoAreaComponent::getTextMessageComponent()
        {
            return this->ui->comp_TextMessages;
        }

        void CMainInfoAreaComponent::displayLog()
        {
            this->ui->comp_Log->displayLog();
            this->selectArea(InfoAreaLog);
        }

        void CMainInfoAreaComponent::displayConsole()
        {
            this->ui->comp_Log->displayConsole();
            this->selectArea(InfoAreaLog);
        }

        QSize CMainInfoAreaComponent::getPreferredSizeWhenFloating(int areaIndex) const
        {
            InfoArea area = static_cast<InfoArea>(areaIndex);
            switch (area)
            {
            case InfoAreaCockpit:
            case InfoAreaAircraft:
            case InfoAreaAtc:
            case InfoAreaUsers:
            case InfoAreaLog:
            case InfoAreaSimulator:
                return QSize(400, 300);
            case InfoAreaMappings:
            case InfoAreaSettings:
            case InfoAreaTextMessages:
                return QSize(600, 400);
                break;
            case InfoAreaFlightPlan:
                return QSize(625, 500);
            default:
                return QSize(400, 300);
            }
        }

        void CMainInfoAreaComponent::selectSettingsTab(int index)
        {
            this->selectArea(InfoAreaSettings);
            this->ui->comp_Settings->setSettingsTab(static_cast<CSettingsComponent::SettingTab>(index));
        }

        const QPixmap &CMainInfoAreaComponent::indexToPixmap(int areaIndex) const
        {
            InfoArea area = static_cast<InfoArea>(areaIndex);
            switch (area)
            {
            case InfoAreaCockpit:
                return CIcons::appCockpit16();
            case InfoAreaUsers:
                return CIcons::appUsers16();
            case InfoAreaWeather:
                return CIcons::appWeather16();
            case InfoAreaAtc:
                return CIcons::appAtc16();
            case InfoAreaAircraft:
                return CIcons::appAircraft16();
            case InfoAreaSettings:
                return CIcons::appSettings16();
            case InfoAreaFlightPlan:
                return CIcons::appFlightPlan16();
            case InfoAreaTextMessages:
                return CIcons::appTextMessages16();
            case InfoAreaSimulator:
                return CIcons::appSimulator16();
            case InfoAreaMappings:
                return CIcons::appMappings16();
            case InfoAreaLog:
                return CIcons::appLog16();
            default:
                return CIcons::empty();
            }
        }
    } // namespace
} // namespace

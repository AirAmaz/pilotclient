/* Copyright (C) 2015
 * swift project Community / Contributors
 *
 * This file is part of swift Project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "blackconfig/buildconfig.h"
#include "blackcore/webdataservices.h"
#include "blackgui/components/datainfoareacomponent.h"
#include "blackgui/components/dbaircrafticaocomponent.h"
#include "blackgui/components/dbairlineicaocomponent.h"
#include "blackgui/components/dbcountrycomponent.h"
#include "blackgui/components/dbdistributorcomponent.h"
#include "blackgui/components/dbliverycomponent.h"
#include "blackgui/components/dbmodelcomponent.h"
#include "blackgui/guiapplication.h"
#include "blackmisc/icons.h"
#include "blackmisc/logmessage.h"
#include "blackmisc/network/entityflags.h"
#include "blackmisc/statusmessage.h"
#include "blackmisc/verify.h"
#include "ui_datainfoareacomponent.h"

#include <QDateTime>
#include <QIcon>
#include <QTabWidget>
#include <QtGlobal>

using namespace BlackConfig;
using namespace BlackMisc;
using namespace BlackMisc::Network;
using namespace BlackGui;
using namespace BlackGui::Components;

namespace BlackGui
{
    namespace Components
    {
        CDataInfoAreaComponent::CDataInfoAreaComponent(QWidget *parent) :
            CInfoArea(parent),
            ui(new Ui::CDataInfoAreaComponent)
        {
            ui->setupUi(this);
            this->initInfoArea(); // init base class
            this->setWindowIcon(CIcons::swiftDatabase24());
            this->ps_setTabBarPosition(QTabWidget::North);
        }

        CDataInfoAreaComponent::~CDataInfoAreaComponent()
        { }

        CDbModelComponent *CDataInfoAreaComponent::getModelComponent() const
        {
            return ui->comp_DbModels;
        }

        CDbLiveryComponent *CDataInfoAreaComponent::getLiveryComponent() const
        {
            return ui->comp_DbLiveries;
        }

        CDbDistributorComponent *CDataInfoAreaComponent::getDistributorComponent() const
        {
            return ui->comp_DbDistributors;
        }

        CDbAircraftIcaoComponent *CDataInfoAreaComponent::getAircraftComponent() const
        {
            return ui->comp_DbAircraftIcao;
        }

        CDbAirlineIcaoComponent *CDataInfoAreaComponent::getAirlineComponent() const
        {
            return ui->comp_DbAirlineIcao;
        }

        CDbCountryComponent *CDataInfoAreaComponent::getCountryComponent() const
        {
            return ui->comp_DbCountries;
        }

        bool CDataInfoAreaComponent::writeDbDataToResourceDir() const
        {
            if (!sGui || !sGui->getWebDataServices()->canConnectSwiftDb())
            {
                CLogMessage(this).warning("No connection to DB yet, no new data loaded which can be written");
                return false;
            }

            // write to disk
            bool s = sGui->getWebDataServices()->writeDbDataToDisk(CBuildConfig::getSwiftStaticDbFilesDir());
            if (s)
            {
                CLogMessage(this).info("Written DB data");
            }
            else
            {
                CLogMessage(this).error("Cannot write DB data");
            }
            return s;
        }

        bool CDataInfoAreaComponent::readDbDataFromResourceDir()
        {
            bool s = sGui &&
                     sGui->getWebDataServices()->readDbDataFromDisk(CBuildConfig::getSwiftStaticDbFilesDir(), true);

            // info
            if (s)
            {
                CLogMessage(this).info("Read DB data from directory: %1") << CBuildConfig::getSwiftStaticDbFilesDir();
                ui->comp_DbAircraftIcao->showLoadIndicator();
                ui->comp_DbAirlineIcao->showLoadIndicator();
                ui->comp_DbCountries->showLoadIndicator();
                ui->comp_DbDistributors->showLoadIndicator();
                ui->comp_DbLiveries->showLoadIndicator();
                ui->comp_DbModels->showLoadIndicator();
            }
            else
            {
                CLogMessage(this).error("Failed to load DB data");
            }
            return s;
        }

        QSize CDataInfoAreaComponent::getPreferredSizeWhenFloating(int areaIndex) const
        {
            InfoArea area = static_cast<InfoArea>(areaIndex);
            switch (area)
            {
            case InfoAreaAircraftIcao:
            case InfoAreaAirlineIcao:
            case InfoAreaLiveries:
            case InfoAreaModels:
            case InfoAreaCountries:
            default:
                return QSize(800, 600);
            }
        }

        const QPixmap &CDataInfoAreaComponent::indexToPixmap(int areaIndex) const
        {
            InfoArea area = static_cast<InfoArea>(areaIndex);
            switch (area)
            {
            case InfoAreaAircraftIcao:
                return CIcons::appAircraftIcao16();
            case InfoAreaAirlineIcao:
                return CIcons::appAirlineIcao16();
            case InfoAreaLiveries:
                return CIcons::appLiveries16();
            case InfoAreaDistributors:
                return CIcons::appDistributors16();
            case InfoAreaModels:
                return CIcons::appModels16();
            case InfoAreaCountries:
                return CIcons::appCountries16();
            default:
                return CIcons::empty();
            }
        }

        void CDataInfoAreaComponent::requestUpdateOfAllDbData()
        {
            sGui->getWebDataServices()->triggerReloadFromDb(CEntityFlags::AllDbEntitiesNoInfoObjectsNoAirports, QDateTime());
        }

        void CDataInfoAreaComponent::requestUpdatedData(CEntityFlags::Entity entity)
        {
            bool requested = false;
            if (entity.testFlag(CEntityFlags::ModelEntity))
            {
                ui->comp_DbModels->requestUpdatedData();
                requested = true;
            }

            // sanity
            BLACK_VERIFY_X(requested, Q_FUNC_INFO, "Entity not supported");
            Q_UNUSED(requested);
        }
    } // ns
} // ns

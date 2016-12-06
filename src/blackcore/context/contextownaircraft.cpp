/* Copyright (C) 2013
 * swift Project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "blackcore/application.h"
#include "blackcore/webdataservices.h"
#include "blackcore/context/contextownaircraft.h"
#include "blackcore/context/contextownaircraftempty.h"
#include "blackcore/context/contextownaircraftimpl.h"
#include "blackcore/context/contextownaircraftproxy.h"
#include "blackmisc/aviation/aircraftsituation.h"
#include "blackmisc/dbusserver.h"

using namespace BlackMisc::Aviation;
using namespace BlackMisc::PhysicalQuantities;
using namespace BlackMisc::Geo;
using namespace BlackMisc::Simulation;

namespace BlackCore
{
    namespace Context
    {
        IContextOwnAircraft *IContextOwnAircraft::create(CCoreFacade *parent, CCoreFacadeConfig::ContextMode mode, BlackMisc::CDBusServer *server, QDBusConnection &conn)
        {
            switch (mode)
            {
            case CCoreFacadeConfig::Local:
            case CCoreFacadeConfig::LocalInDbusServer:
                return (new CContextOwnAircraft(mode, parent))->registerWithDBus(server);
            case CCoreFacadeConfig::Remote:
                return new CContextOwnAircraftProxy(BlackMisc::CDBusServer::coreServiceName(), conn, mode, parent);
            case CCoreFacadeConfig::NotUsed:
            default:
                return new CContextOwnAircraftEmpty(parent);
            }
        }

        const BlackMisc::Aviation::CAircraftSituation &IContextOwnAircraft::getDefaultSituation()
        {
            static const CAircraftSituation situation(
                CCoordinateGeodetic(
                    CLatitude::fromWgs84("N 049° 18' 17"),
                    CLongitude::fromWgs84("E 008° 27' 05"),
                    CLength(0, CLengthUnit::m())),
                CAltitude(312, CAltitude::MeanSeaLevel, CLengthUnit::ft())
            );
            return situation;
        }

        BlackMisc::Simulation::CAircraftModel IContextOwnAircraft::getDefaultOwnAircraftModel()
        {
            // if all fails
            static const CAircraftModel defaultModel(
                "", CAircraftModel::TypeOwnSimulatorModel, "default model",
                CAircraftIcaoCode("C172", "L1P", "Cessna", "172", "L", true, false, false, 0));

            // create one from DB data
            if (sApp && sApp->hasWebDataServices())
            {
                static const CAircraftIcaoCode icao = sApp->getWebDataServices()->getAircraftIcaoCodeForDesignator("C172");
                static const CLivery livery = sApp->getWebDataServices()->getLiveryForCombinedCode("_CC_WHITE_WHITE");
                static const CAircraftModel model("", CAircraftModel::TypeOwnSimulatorModel, icao, livery);
                return model;
            }
            return defaultModel;
        }
    } // namespace
} // namespace

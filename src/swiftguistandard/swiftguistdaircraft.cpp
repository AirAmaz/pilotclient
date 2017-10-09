/* Copyright (C) 2013
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "blackcore/context/contextownaircraft.h"
#include "blackgui/guiapplication.h"
#include "blackmisc/compare.h"
#include "blackmisc/geo/coordinategeodetic.h"
#include "blackmisc/geo/latitude.h"
#include "blackmisc/geo/longitude.h"
#include "blackmisc/pq/length.h"
#include "blackmisc/pq/units.h"
#include "blackmisc/simulation/simulatedaircraft.h"
#include "swiftguistd.h"

#include <QString>

namespace BlackMisc { namespace Aviation { class CAltitude; } }

using namespace BlackCore;
using namespace BlackMisc;
using namespace BlackGui;
using namespace BlackMisc::Network;
using namespace BlackMisc::Aviation;
using namespace BlackMisc::Simulation;
using namespace BlackMisc::PhysicalQuantities;
using namespace BlackMisc::Geo;
using namespace BlackMisc::Audio;

bool SwiftGuiStd::ps_reloadOwnAircraft()
{
    if (!this->m_contextNetworkAvailable) { return false; }

    // check for changed aircraft
    bool changed = false;
    CSimulatedAircraft loadedAircraft = sGui->getIContextOwnAircraft()->getOwnAircraft();
    if (loadedAircraft != m_ownAircraft)
    {
        m_ownAircraft = loadedAircraft;
        changed = true;
    }
    return changed;
}

void SwiftGuiStd::setTestPosition(const QString &wgsLatitude, const QString &wgsLongitude, const CAltitude &altitude)
{
    CCoordinateGeodetic coordinate(
        CLatitude::fromWgs84(wgsLatitude),
        CLongitude::fromWgs84(wgsLongitude),
        CAltitude(0, CLengthUnit::m()));

    m_ownAircraft.setPosition(coordinate);
    m_ownAircraft.setAltitude(altitude);
    sGui->getIContextOwnAircraft()->updateOwnPosition(coordinate, altitude);
}

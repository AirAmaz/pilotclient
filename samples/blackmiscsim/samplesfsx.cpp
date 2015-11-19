/* Copyright (C) 2014
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "samplesfsx.h"
#include "blackmisc/simulation/fsx/simconnectutilities.h"
#include "blackmisc/blackmiscfreefunctions.h"
#include <QDebug>

using namespace BlackMisc::Simulation::Fsx;

namespace BlackSimTest
{

    /*
     * Samples
     */
    void CSamplesFsx::samplesMisc(QTextStream &streamOut)
    {
        BlackMisc::registerMetadata();
        streamOut << CSimConnectUtilities::simConnectExceptionToString(CSimConnectUtilities::SIMCONNECT_EXCEPTION_ALREADY_SUBSCRIBED) << endl;
        streamOut << CSimConnectUtilities::simConnectExceptionToString(CSimConnectUtilities::SIMCONNECT_EXCEPTION_ILLEGAL_OPERATION) << endl;
        streamOut << CSimConnectUtilities::simConnectSurfaceTypeToString(CSimConnectUtilities::Bituminus) << endl;
    }
} // namespace

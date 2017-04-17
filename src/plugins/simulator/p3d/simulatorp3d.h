/* Copyright (C) 2017
 * swift Project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file

#ifndef BLACKSIMPLUGIN_SIMULATOR_P3D_H
#define BLACKSIMPLUGIN_SIMULATOR_P3D_H

#include "../fsxcommon/simulatorfsxcommon.h"

namespace BlackSimPlugin
{
    namespace P3D
    {
        //! P3D Simulator Implementation
        class CSimulatorP3D : public BlackSimPlugin::FsxCommon::CSimulatorFsxCommon
        {
            Q_OBJECT

        public:
            //! Constructor, parameters as in \sa BlackCore::ISimulatorFactory::create
            CSimulatorP3D(
                const BlackMisc::Simulation::CSimulatorPluginInfo &info,
                BlackMisc::Simulation::IOwnAircraftProvider *ownAircraftProvider,
                BlackMisc::Simulation::IRemoteAircraftProvider *remoteAircraftProvider,
                BlackMisc::Weather::IWeatherGridProvider *weatherGridProvider,
                QObject *parent = nullptr);
        };

        //! Listener for P3D
        class CSimulatorP3DListener : public BlackSimPlugin::FsxCommon::CSimulatorFsxCommonListener
        {
            Q_OBJECT

        public:
            //! Constructor
            using CSimulatorFsxCommonListener::CSimulatorFsxCommonListener;
        };
    } // ns
} // ns

#endif // guard

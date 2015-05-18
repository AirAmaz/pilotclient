/* Copyright (C) 2013
 * swift Project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file

#ifndef BLACKMISC_SIMULATION_SIMULATORINFOLIST_H
#define BLACKMISC_SIMULATION_SIMULATORINFOLIST_H

#include "simulatorplugininfo.h"
#include "blackmisc/blackmiscexport.h"
#include "blackmisc/sequence.h"
#include "blackmisc/collection.h"
#include <QStringList>

namespace BlackMisc
{
    namespace Simulation
    {
        //! Value object encapsulating a list of SimulatorInfo objects.
        class BLACKMISC_EXPORT CSimulatorPluginInfoList :
            public BlackMisc::CSequence<CSimulatorPluginInfo>,
            public BlackMisc::Mixin::MetaType<CSimulatorPluginInfoList>
        {
        public:
            BLACKMISC_DECLARE_USING_MIXIN_METATYPE(CSimulatorPluginInfoList)

            //! Default constructor
            CSimulatorPluginInfoList();

            //! Construct from a base class object.
            CSimulatorPluginInfoList(const CSequence<CSimulatorPluginInfo> &other);

            //! Is simulator supported
            bool supportsSimulator(const QString &simulator);

            //! String list with meaningful representations
            QStringList toStringList(bool i18n = false) const;

        };
    } // ns
} // ns

Q_DECLARE_METATYPE(BlackMisc::Simulation::CSimulatorPluginInfoList)
Q_DECLARE_METATYPE(BlackMisc::CCollection<BlackMisc::Simulation::CSimulatorPluginInfo>)
Q_DECLARE_METATYPE(BlackMisc::CSequence<BlackMisc::Simulation::CSimulatorPluginInfo>)

#endif // guard

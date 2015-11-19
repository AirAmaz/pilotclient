/* Copyright (C) 2013
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file

#ifndef BLACKMISC_SIMULATION_FSCOMMON_AIRCRAFTCFGLIST_H
#define BLACKMISC_SIMULATION_FSCOMMON_AIRCRAFTCFGLIST_H

#include "blackmisc/blackmiscexport.h"
#include "blackmisc/simulation/fscommon/aircraftcfgentries.h"
#include "blackmisc/simulation/aircraftmodellist.h"
#include "blackmisc/sequence.h"
#include "blackmisc/collection.h"
#include <QDir>
#include <QVector>
#include <QDebug>
#include <QSettings>

namespace BlackMisc
{
    namespace Simulation
    {
        namespace FsCommon
        {
            //! Utility, providing FS aircraft.cfg entries
            class BLACKMISC_EXPORT CAircraftCfgEntriesList :
                public BlackMisc::CSequence<CAircraftCfgEntries>,
                public BlackMisc::Mixin::MetaType<CAircraftCfgEntriesList>
            {
            public:
                BLACKMISC_DECLARE_USING_MIXIN_METATYPE(CAircraftCfgEntriesList)

                //! Constructor
                CAircraftCfgEntriesList() {}

                //! Virtual destructor
                virtual ~CAircraftCfgEntriesList() {}

                //! Contains model with title?
                bool containsModelWithTitle(const QString &title, Qt::CaseSensitivity caseSensitivity = Qt::CaseInsensitive);

                //! All titles (aka model names)
                QStringList getTitles(bool sorted = false) const;

                //! As aircraft models
                BlackMisc::Simulation::CAircraftModelList toAircraftModelList() const;

                //! As aircraft models for simulator
                BlackMisc::Simulation::CAircraftModelList toAircraftModelList(const BlackMisc::Simulation::CSimulatorInfo &simInfo) const;

                //! Ambiguous titles
                QStringList detectAmbiguousTitles() const;

                //! Find by title
                CAircraftCfgEntriesList findByTitle(const QString &title, Qt::CaseSensitivity caseSensitivity = Qt::CaseInsensitive) const;

            private:
                //! Section within file
                enum FileSection
                {
                    General,
                    Fltsim,
                    Unknown
                };
            };
        } // namespace
    } // namespace
} // namespace

Q_DECLARE_METATYPE(BlackMisc::Simulation::FsCommon::CAircraftCfgEntriesList)
Q_DECLARE_METATYPE(BlackMisc::CCollection<BlackMisc::Simulation::FsCommon::CAircraftCfgEntries>)
Q_DECLARE_METATYPE(BlackMisc::CSequence<BlackMisc::Simulation::FsCommon::CAircraftCfgEntries>)

#endif // guard

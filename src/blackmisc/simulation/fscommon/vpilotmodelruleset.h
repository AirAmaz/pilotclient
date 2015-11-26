/* Copyright (C) 2015
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file

#ifndef BLACKMISC_SIMULATION_FSCOMMON_VPILOTMODELRULELIST_H
#define BLACKMISC_SIMULATION_FSCOMMON_VPILOTMODELRULELIST_H

#include "blackmisc/blackmiscexport.h"
#include "blackmisc/simulation/fscommon/vpilotmodelrule.h"
#include "blackmisc/simulation/aircraftmodellist.h"
#include "blackmisc/collection.h"
#include "blackmisc/sequence.h"

namespace BlackMisc
{
    namespace Simulation
    {
        namespace FsCommon
        {
            //! Value object reading a set of vPilot rules
            class BLACKMISC_EXPORT CVPilotModelRuleSet :
                public CCollection<CVPilotModelRule>,
                public BlackMisc::Mixin::MetaType<CVPilotModelRuleSet>
            {
            public:
                BLACKMISC_DECLARE_USING_MIXIN_METATYPE(CVPilotModelRuleSet)

                //! Default constructor.
                CVPilotModelRuleSet() = default;

                //! Construct from a base class object.
                CVPilotModelRuleSet(const CCollection<CVPilotModelRule> &other);

                //! Find by model string
                CVPilotModelRuleSet findByModelName(const QString &modelName) const;

                //! Find first by model string
                CVPilotModelRule findFirstByModelName(const QString &modelName) const;

                //! Find models starting with
                CVPilotModelRuleSet findModelsStartingWith(const QString &modelName) const;

                //! Model strings
                QStringList getSortedModelNames() const;

                //! List of distributors
                QStringList getSortedDistributors() const;

                //! Removed given models
                int removeModels(const QStringList &modelsToBeRemoved);

                //! Keep given models (if in list)
                int keepModels(const QStringList &modelsToBeKept);

                //! To aircraft models
                //! \note slow operation, can take a while
                BlackMisc::Simulation::CAircraftModelList toAircraftModels() const;

            private:
                //! Convert values to upper case
                static QStringList toUpper(const QStringList &stringList);
            };

        } // namespace
    } //namespace
} // namespace

Q_DECLARE_METATYPE(BlackMisc::Simulation::FsCommon::CVPilotModelRuleSet)
Q_DECLARE_METATYPE(BlackMisc::CCollection<BlackMisc::Simulation::FsCommon::CVPilotModelRule>)
Q_DECLARE_METATYPE(BlackMisc::CSequence<BlackMisc::Simulation::FsCommon::CVPilotModelRule>)

#endif //guard

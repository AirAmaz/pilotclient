/* Copyright (C) 2015
 * swift Project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "blackmisc/aviation/callsignobjectlist.h"
#include "blackmisc/predicates.h"
#include "blackmisc/aviation/atcstationlist.h"
#include "blackmisc/aviation/aircraftsituationlist.h"
#include "blackmisc/aviation/aircraftpartslist.h"
#include "blackmisc/network/clientlist.h"
#include "blackmisc/simulation/simulatedaircraftlist.h"

namespace BlackMisc
{
    namespace Aviation
    {
        template <class OBJ, class CONTAINER>
        ICallsignObjectList<OBJ, CONTAINER>::ICallsignObjectList()
        { }

        template <class OBJ, class CONTAINER>
        const CONTAINER &ICallsignObjectList<OBJ, CONTAINER>::container() const
        {
            return static_cast<const CONTAINER &>(*this);
        }

        template <class OBJ, class CONTAINER>
        CONTAINER &ICallsignObjectList<OBJ, CONTAINER>::container()
        {
            return static_cast<CONTAINER &>(*this);
        }

        template <class OBJ, class CONTAINER>
        bool ICallsignObjectList<OBJ, CONTAINER>::containsCallsign(const CCallsign &callsign) const
        {
            return this->container().contains(&OBJ::getCallsign, callsign);
        }

        template <class OBJ, class CONTAINER>
        int ICallsignObjectList<OBJ, CONTAINER>::applyIfCallsign(const CCallsign &callsign, const CPropertyIndexVariantMap &variantMap)
        {
            return this->container().applyIf(&OBJ::getCallsign, callsign, variantMap);
        }

        template <class OBJ, class CONTAINER>
        CCallsignSet ICallsignObjectList<OBJ, CONTAINER>::getCallsigns() const
        {
            CCallsignSet cs;
            for (const OBJ &obj : this->container())
            {
                cs.push_back(obj.getCallsign());
            }
            return cs;
        }

        template <class OBJ, class CONTAINER>
        CONTAINER ICallsignObjectList<OBJ, CONTAINER>::findByCallsign(const CCallsign &callsign) const
        {
            return this->container().findBy(&OBJ::getCallsign, callsign);
        }

        template <class OBJ, class CONTAINER>
        CONTAINER ICallsignObjectList<OBJ, CONTAINER>::findByCallsigns(const CCallsignSet &callsigns) const
        {
            return this->container().findBy(Predicates::MemberIsAnyOf(&OBJ::getCallsign, callsigns));
        }

        template <class OBJ, class CONTAINER>
        OBJ ICallsignObjectList<OBJ, CONTAINER>::findFirstByCallsign(const CCallsign &callsign, const OBJ &ifNotFound) const
        {
            return this->container().findFirstByOrDefault(&OBJ::getCallsign, callsign, ifNotFound);
        }

        template <class OBJ, class CONTAINER>
        OBJ ICallsignObjectList<OBJ, CONTAINER>::findLastByCallsign(const CCallsign &callsign, const OBJ &ifNotFound) const
        {
            for (auto current = container().end(); current != container().begin() ; /* Do nothing */)
            {
                --current;
                if (current->getCallsign() == callsign) { return *current; }
            }
            return ifNotFound;
        }

        template <class OBJ, class CONTAINER>
        CONTAINER ICallsignObjectList<OBJ, CONTAINER>::findBySuffix(const QString &suffix) const
        {
            CONTAINER r;
            if (suffix.isEmpty()) { return r; }
            QString sfxUpper(suffix.trimmed().toUpper());
            r = this->container().findBy([ = ](const OBJ & csObj) -> bool
            {
                return (csObj.getCallsign().getSuffix() == sfxUpper);
            });
            return r;
        }

        template <class OBJ, class CONTAINER>
        int ICallsignObjectList<OBJ, CONTAINER>::firstIndexOfCallsign(const CCallsign &callsign)
        {
            for (int i = 0; i < this->container().size(); i++)
            {
                if (this->container()[i].getCallsign() == callsign) { return i; }
            }
            return -1;
        }

        template <class OBJ, class CONTAINER>
        int ICallsignObjectList<OBJ, CONTAINER>::removeByCallsign(const CCallsign &callsign)
        {
            return this->container().removeIf(&OBJ::getCallsign, callsign);
        }

        template <class OBJ, class CONTAINER>
        QMap<QString, int> ICallsignObjectList<OBJ, CONTAINER>::getSuffixes() const
        {
            QMap<QString, int> r;
            for (const OBJ &csObj : this->container())
            {
                const QString s = csObj.getCallsign().getSuffix();
                if (s.isEmpty()) { continue; }
                if (r.contains(s))
                {
                    r[s] = r[s] + 1;
                }
                else
                {
                    r.insert(s, 1);
                }
            }
            return r;
        }

        template <class OBJ, class CONTAINER>
        QHash<CCallsign, CONTAINER> ICallsignObjectList<OBJ, CONTAINER>::splitPerCallsign() const
        {
            CONTAINER copyContainer(container());
            copyContainer.sortByCallsign();
            QHash<CCallsign, CONTAINER> result;
            CCallsign cs;
            for (const OBJ &csObj : copyContainer)
            {
                if (csObj.getCallsign().isEmpty())
                {
                    Q_ASSERT(false); // there should be no empty callsigns
                    continue;
                }
                if (cs != csObj.getCallsign())
                {
                    cs = csObj.getCallsign();
                    CONTAINER perCallsign({ csObj });
                    result.insert(cs, perCallsign);
                }
                else
                {
                    result[cs].push_back(csObj);
                }
            }
            return result;
        }

        template <class OBJ, class CONTAINER>
        void ICallsignObjectList<OBJ, CONTAINER>::sortByCallsign()
        {
            container().sortBy(&OBJ::getCallsign);
        }

        template <class OBJ, class CONTAINER>
        int ICallsignObjectList<OBJ, CONTAINER>::incrementalUpdateOrAdd(const OBJ &objectBeforeChanges, const CPropertyIndexVariantMap &changedValues)
        {
            int c;
            const CCallsign cs = objectBeforeChanges.getCallsign();
            if (this->containsCallsign(cs))
            {
                if (changedValues.isEmpty()) { return 0; }
                c = this->container().applyIf(&OBJ::getCallsign, cs, changedValues);
            }
            else
            {
                c = 1;
                if (changedValues.isEmpty())
                {
                    this->container().push_back(objectBeforeChanges);
                }
                else
                {
                    OBJ objectAdded(objectBeforeChanges);
                    objectAdded.apply(changedValues);
                    this->container().push_back(objectAdded);
                }
            }
            return c;
        }

        // see here for the reason of thess forward instantiations
        // http://www.parashift.com/c++-faq/separate-template-class-defn-from-decl.html
        template class ICallsignObjectList<BlackMisc::Aviation::CAtcStation, BlackMisc::Aviation::CAtcStationList>;
        template class ICallsignObjectList<BlackMisc::Aviation::CAircraftSituation, BlackMisc::Aviation::CAircraftSituationList>;
        template class ICallsignObjectList<BlackMisc::Simulation::CSimulatedAircraft, BlackMisc::Simulation::CSimulatedAircraftList>;
        template class ICallsignObjectList<BlackMisc::Network::CClient, BlackMisc::Network::CClientList>;

    } // namespace
} // namespace


/* Copyright (C) 2013
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "blackmisc/simulation/simulatedaircraft.h"
#include "blackmisc/simulation/simulatedaircraftlist.h"
#include "blackmisc/aviation/aircrafticaocode.h"
#include "blackmisc/aviation/airlineicaocode.h"
#include "blackmisc/aviation/callsign.h"
#include "blackmisc/compare.h"
#include "blackmisc/metaclassprivate.h"
#include "blackmisc/network/user.h"
#include "blackmisc/predicates.h"
#include "blackmisc/range.h"

#include <QString>
#include <tuple>

using namespace BlackMisc;
using namespace BlackMisc::Aviation;
using namespace BlackMisc::PhysicalQuantities;
using namespace BlackMisc::Network;

namespace BlackMisc
{
    namespace Simulation
    {
        CSimulatedAircraftList::CSimulatedAircraftList() { }

        CSimulatedAircraftList::CSimulatedAircraftList(const CSequence<CSimulatedAircraft> &other) :
            CSequence<CSimulatedAircraft>(other)
        { }

        CUserList CSimulatedAircraftList::getPilots() const
        {
            return this->findBy(Predicates::MemberValid(&CSimulatedAircraft::getPilot)).transform(Predicates::MemberTransform(&CSimulatedAircraft::getPilot));
        }

        CSimulatedAircraftList CSimulatedAircraftList::findByEnabled(bool enabled) const
        {
            return this->findBy(&CSimulatedAircraft::isEnabled, enabled);
        }

        CSimulatedAircraftList CSimulatedAircraftList::findByRendered(bool rendered) const
        {
            return this->findBy(&CSimulatedAircraft::isRendered, rendered);
        }

        CSimulatedAircraftList CSimulatedAircraftList::findByVtol(bool vtol) const
        {
            return this->findBy(&CSimulatedAircraft::isVtol, vtol);
        }

        CCallsignSet CSimulatedAircraftList::getCallsignsWithSynchronizedParts() const
        {
            CCallsignSet csl;
            for (const CSimulatedAircraft &aircraft : (*this))
            {
                if (!aircraft.isPartsSynchronized()) { continue; }
                csl.push_back(aircraft.getCallsign());
            }
            return csl;
        }

        bool CSimulatedAircraftList::updateWithVatsimDataFileData(CSimulatedAircraft &aircraftToBeUpdated) const
        {
            if (this->isEmpty()) return false;
            if (aircraftToBeUpdated.hasValidRealName() && aircraftToBeUpdated.hasValidId() && aircraftToBeUpdated.hasAircraftAndAirlineDesignator()) { return false; }

            CSimulatedAircraft currentDataFileAircraft = this->findFirstByCallsign(aircraftToBeUpdated.getCallsign());
            if (currentDataFileAircraft.getCallsign().isEmpty()) return false;

            CUser user = aircraftToBeUpdated.getPilot();
            user.updateMissingParts(currentDataFileAircraft.getPilot());
            aircraftToBeUpdated.setPilot(user);

            CAircraftIcaoCode aircraftIcao = aircraftToBeUpdated.getAircraftIcaoCode();
            CAirlineIcaoCode airlineIcao = aircraftToBeUpdated.getAirlineIcaoCode();
            aircraftIcao.updateMissingParts(currentDataFileAircraft.getAircraftIcaoCode());
            airlineIcao.updateMissingParts(currentDataFileAircraft.getAirlineIcaoCode());
            aircraftToBeUpdated.setIcaoCodes(aircraftIcao, airlineIcao);
            return true;
        }

        void CSimulatedAircraftList::markAllAsNotRendered()
        {
            for (CSimulatedAircraft &aircraft : (*this))
            {
                if (!aircraft.isRendered()) { continue; }
                aircraft.setRendered(false);
            }
        }

        int CSimulatedAircraftList::setRendered(const CCallsign &callsign, bool rendered, bool onlyFirst)
        {
            int c = 0;
            for (CSimulatedAircraft &aircraft : (*this))
            {
                if (aircraft.getCallsign() != callsign) { continue; }
                aircraft.setRendered(rendered);
                c++;
                if (onlyFirst) break;
            }
            return c;
        }

        int CSimulatedAircraftList::setAircraftModel(const CCallsign &callsign, const CAircraftModel &model, bool onlyFirst)
        {
            int c = 0;
            for (CSimulatedAircraft &aircraft : (*this))
            {
                if (aircraft.getCallsign() != callsign) { continue; }
                aircraft.setModel(model);
                c++;
                if (onlyFirst) break;
            }
            return c;
        }

        int CSimulatedAircraftList::setAircraftParts(const CCallsign &callsign, const CAircraftParts &parts, bool onlyFirst)
        {
            int c = 0;
            for (CSimulatedAircraft &aircraft : (*this))
            {
                if (aircraft.getCallsign() != callsign) { continue; }
                aircraft.setParts(parts);
                aircraft.setPartsSynchronized(true);
                c++;
                if (onlyFirst) break;
            }
            return c;
        }

        int CSimulatedAircraftList::setGroundElevation(const CCallsign &callsign, const CAltitude &elevation, bool onlyFirst)
        {
            int c = 0;
            for (CSimulatedAircraft &aircraft : (*this))
            {
                if (aircraft.getCallsign() != callsign) { continue; }
                aircraft.setGroundElevation(elevation);
                c++;
                if (onlyFirst) break;
            }
            return c;
        }

        bool CSimulatedAircraftList::isEnabled(const CCallsign &callsign) const
        {
            for (const CSimulatedAircraft &aircraft : (*this))
            {
                if (aircraft.getCallsign() != callsign) { continue; }
                return aircraft.isEnabled();
            }
            return false;
        }

        bool CSimulatedAircraftList::isRendered(const CCallsign &callsign) const
        {
            for (const CSimulatedAircraft &aircraft : (*this))
            {
                if (aircraft.getCallsign() != callsign) { continue; }
                return aircraft.isRendered();
            }
            return false;
        }

        bool CSimulatedAircraftList::replaceOrAddByCallsign(const CSimulatedAircraft &aircraft)
        {
            const CCallsign cs(aircraft.getCallsign());
            if (cs.isEmpty()) { return false; }

            if (this->containsCallsign(cs))
            {
                int c = this->replaceIf(&CSimulatedAircraft::getCallsign, cs, aircraft);
                return c > 0;
            }
            this->push_back(aircraft);
            return true;
        }
    } // namespace
} // namespace

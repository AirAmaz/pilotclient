/* Copyright (C) 2013
 * swift Project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "simconnectobject.h"
#include "blackmisc/simulation/interpolatormulti.h"

using namespace BlackMisc::Aviation;
using namespace BlackMisc::Simulation;

namespace BlackSimPlugin
{
    namespace FsxCommon
    {
        CSimConnectObject::CSimConnectObject()
        { }

        CSimConnectObject::CSimConnectObject(const BlackMisc::Simulation::CSimulatedAircraft &aircraft,
                                             DWORD requestId,
                                             CInterpolationLogger *logger) :
            m_aircraft(aircraft), m_requestId(requestId), m_validRequestId(true),
            m_interpolator(QSharedPointer<CInterpolatorMulti>::create(aircraft.getCallsign()))
        {
            m_interpolator->attachLogger(logger);
        }

        void CSimConnectObject::addAircraftParts(const CAircraftParts &parts)
        {
            Q_ASSERT(m_interpolator);
            m_interpolator->addAircraftParts(parts);
        }

        void CSimConnectObject::addAircraftSituation(const CAircraftSituation &situation)
        {
            Q_ASSERT(m_interpolator);
            m_interpolator->addAircraftSituation(situation);
        }

        void CSimConnectObject::invalidatePartsAsSent()
        {
            DataDefinitionRemoteAircraftPartsWithoutLights dd;
            dd.resetToInvalid();
            m_partsAsSent = dd;
        }

        bool CSimConnectObject::isPendingAdded() const
        {
            return !this->hasValidRequestAndObjectId() || !this->m_confirmedAdded;
        }

        bool CSimConnectObject::isConfirmedAdded() const
        {
            Q_ASSERT_X(!m_confirmedAdded || this->hasValidRequestAndObjectId(), Q_FUNC_INFO, "confirmed but invalid ids");
            return m_confirmedAdded;
        }

        void CSimConnectObject::setConfirmedAdded(bool confirm)
        {
            m_confirmedAdded = confirm;
            m_aircraft.setRendered(true);
        }

        void CSimConnectObject::setPendingRemoved(bool pending)
        {
            m_pendingRemoved = pending;
            m_aircraft.setRendered(false);
        }

        bool CSimConnectObject::hasValidRequestAndObjectId() const
        {
            return this->hasValidRequestId() && this->hasValidObjectId();
        }

        void CSimConnectObject::toggleInterpolatorMode()
        {
            Q_ASSERT(m_interpolator);
            this->m_interpolator->toggleMode();
        }

        bool CSimConnectObject::setInterpolatorMode(CInterpolatorMulti::Mode mode)
        {
            Q_ASSERT(m_interpolator);
            return this->m_interpolator->setMode(mode);
        }

        bool CSimConnectObjects::setSimConnectObjectIdForRequestId(DWORD requestId, DWORD objectId, bool resetSentParts)
        {
            // First check, if this request id belongs to us
            auto it = std::find_if(this->begin(), this->end(), [requestId](const CSimConnectObject & obj) { return obj.getRequestId() == requestId; });
            if (it == this->end()) { return false; }

            // belongs to us
            it->setObjectId(objectId);
            if (resetSentParts) { it->invalidatePartsAsSent(); }
            return true;
        }

        CCallsign CSimConnectObjects::getCallsignForObjectId(DWORD objectId) const
        {
            return getSimObjectForObjectId(objectId).getCallsign();
        }

        CSimConnectObject CSimConnectObjects::getSimObjectForObjectId(DWORD objectId) const
        {
            for (const CSimConnectObject &simObject : this->values())
            {
                if (simObject.getObjectId() == objectId) { return simObject; }
            }
            return CSimConnectObject();
        }

        CSimConnectObject CSimConnectObjects::getSimObjectForRequestId(DWORD requestId) const
        {
            for (const CSimConnectObject &simObject : this->values())
            {
                if (simObject.getRequestId() == requestId) { return simObject; }
            }
            return CSimConnectObject();
        }

        bool CSimConnectObjects::isKnownSimObjectId(DWORD objectId) const
        {
            const CSimConnectObject simObject(getSimObjectForObjectId(objectId));
            return simObject.hasValidRequestAndObjectId() && objectId == simObject.getObjectId();
        }

        bool CSimConnectObjects::containsPendingAdd() const
        {
            for (const CSimConnectObject &simObject : this->values())
            {
                if (simObject.isPendingAdded()) { return true; }
            }
            return false;
        }

        void CSimConnectObjects::toggleInterpolatorModes()
        {
            for (const CCallsign &cs : this->keys())
            {
                (*this)[cs].toggleInterpolatorMode();
            }
        }

        void CSimConnectObjects::toggleInterpolatorMode(const CCallsign &callsign)
        {
            if (!this->contains(callsign)) { return; }
            (*this)[callsign].toggleInterpolatorMode();
        }

        int CSimConnectObjects::setInterpolatorModes(CInterpolatorMulti::Mode mode)
        {
            int c = 0;
            for (const CCallsign &cs : this->keys())
            {
                if ((*this)[cs].setInterpolatorMode(mode)) c++;
            }
            return c;
        }
    } // namespace
} // namespace

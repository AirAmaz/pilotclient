/* Copyright (C) 2015
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "blackmisc/simulation/remoteaircraftprovider.h"

using namespace BlackMisc::Aviation;
using namespace BlackMisc::Geo;

namespace BlackMisc
{
    namespace Simulation
    {
        bool IRemoteAircraftProvider::isAircraftInRange(const CCallsign &callsign) const
        {
            if (callsign.isEmpty()) { return false; }
            return this->getAircraftInRange().containsCallsign(callsign);
        }

        CSimulatedAircraftList CRemoteAircraftAware::getAircraftInRange() const
        {
            Q_ASSERT_X(this->m_remoteAircraftProvider, Q_FUNC_INFO, "No object available");
            return this->m_remoteAircraftProvider->getAircraftInRange();
        }

        bool CRemoteAircraftAware::isAircraftInRange(const CCallsign &callsign) const
        {
            Q_ASSERT_X(this->m_remoteAircraftProvider, Q_FUNC_INFO, "No object available");
            return this->m_remoteAircraftProvider->isAircraftInRange(callsign);
        }

        int CRemoteAircraftAware::getAircraftInRangeCount() const
        {
            Q_ASSERT_X(this->m_remoteAircraftProvider, Q_FUNC_INFO, "No object available");
            return this->m_remoteAircraftProvider->getAircraftInRangeCount();
        }

        CCallsignSet CRemoteAircraftAware::getAircraftInRangeCallsigns() const
        {
            Q_ASSERT_X(this->m_remoteAircraftProvider, Q_FUNC_INFO, "No object available");
            return this->m_remoteAircraftProvider->getAircraftInRangeCallsigns();
        }

        CSimulatedAircraft CRemoteAircraftAware::getAircraftInRangeForCallsign(const CCallsign &callsign) const
        {
            Q_ASSERT_X(this->m_remoteAircraftProvider, Q_FUNC_INFO, "No object available");
            return this->m_remoteAircraftProvider->getAircraftInRangeForCallsign(callsign);
        }

        CAircraftModel CRemoteAircraftAware::getAircraftInRangeModelForCallsign(const CCallsign &callsign) const
        {
            Q_ASSERT_X(this->m_remoteAircraftProvider, Q_FUNC_INFO, "No object available");
            return this->m_remoteAircraftProvider->getAircraftInRangeModelForCallsign(callsign);
        }

        CAirspaceAircraftSnapshot CRemoteAircraftAware::getLatestAirspaceAircraftSnapshot() const
        {
            Q_ASSERT_X(this->m_remoteAircraftProvider, Q_FUNC_INFO, "No object available");
            return this->m_remoteAircraftProvider->getLatestAirspaceAircraftSnapshot();
        }

        CAircraftSituationList CRemoteAircraftAware::remoteAircraftSituations(const BlackMisc::Aviation::CCallsign &callsign) const
        {
            Q_ASSERT_X(this->m_remoteAircraftProvider, Q_FUNC_INFO, "No object available");
            return this->m_remoteAircraftProvider->remoteAircraftSituations(callsign);
        }

        CAircraftPartsList CRemoteAircraftAware::remoteAircraftParts(const CCallsign &callsign, qint64 cutoffTimeBefore) const
        {
            Q_ASSERT_X(this->m_remoteAircraftProvider, Q_FUNC_INFO, "No object available");
            return this->m_remoteAircraftProvider->remoteAircraftParts(callsign, cutoffTimeBefore);
        }

        CCallsignSet CRemoteAircraftAware::remoteAircraftSupportingParts() const
        {
            Q_ASSERT_X(this->m_remoteAircraftProvider, Q_FUNC_INFO, "No object available");
            return this->m_remoteAircraftProvider->remoteAircraftSupportingParts();
        }

        int CRemoteAircraftAware::remoteAircraftSituationsCount(const CCallsign &callsign) const
        {
            Q_ASSERT_X(this->m_remoteAircraftProvider, Q_FUNC_INFO, "No object available");
            return this->m_remoteAircraftProvider->remoteAircraftSituationsCount(callsign);
        }

        bool CRemoteAircraftAware::updateAircraftModel(const Aviation::CCallsign &callsign, const CAircraftModel &model, const CIdentifier &originator)
        {
            Q_ASSERT_X(this->m_remoteAircraftProvider, Q_FUNC_INFO, "No object available");
            return this->m_remoteAircraftProvider->updateAircraftModel(callsign, model, originator);
        }

        bool CRemoteAircraftAware::updateAircraftNetworkModel(const CCallsign &callsign, const CAircraftModel &model, const CIdentifier &originator)
        {
            Q_ASSERT_X(this->m_remoteAircraftProvider, Q_FUNC_INFO, "No object available");
            return this->m_remoteAircraftProvider->updateAircraftNetworkModel(callsign, model, originator);
        }

        bool CRemoteAircraftAware::updateAircraftRendered(const CCallsign &callsign, bool rendered)
        {
            Q_ASSERT_X(this->m_remoteAircraftProvider, Q_FUNC_INFO, "No object available");
            return this->m_remoteAircraftProvider->updateAircraftRendered(callsign, rendered);
        }

        bool CRemoteAircraftAware::updateAircraftGroundElevation(const CCallsign &callsign, const CElevationPlane &elevation)
        {
            Q_ASSERT_X(this->m_remoteAircraftProvider, Q_FUNC_INFO, "No object available");
            return this->m_remoteAircraftProvider->updateAircraftGroundElevation(callsign, elevation);
        }

        void CRemoteAircraftAware::updateMarkAllAsNotRendered()
        {
            Q_ASSERT_X(this->m_remoteAircraftProvider, Q_FUNC_INFO, "No object available");
            this->m_remoteAircraftProvider->updateMarkAllAsNotRendered();
        }

        bool CRemoteAircraftAware::isRemoteAircraftSupportingParts(const CCallsign &callsign) const
        {
            Q_ASSERT_X(this->m_remoteAircraftProvider, Q_FUNC_INFO, "No object available");
            return this->m_remoteAircraftProvider->isRemoteAircraftSupportingParts(callsign);
        }

        bool CRemoteAircraftAware::updateAircraftEnabled(const Aviation::CCallsign &callsign, bool enabledForRedering)
        {
            Q_ASSERT_X(this->m_remoteAircraftProvider, Q_FUNC_INFO, "No object available");
            return this->m_remoteAircraftProvider->updateAircraftEnabled(callsign, enabledForRedering);
        }

        void IRemoteAircraftProvider::removeOutdatedParts(CAircraftPartsList &partsList)
        {
            // remove outdated parts (but never remove the most recent one)
            const auto predicate = [now = partsList.front().getMSecsSinceEpoch()](const auto & p) { return p.getMSecsSinceEpoch() >= now - PartsPerCallsignMaxAgeInSeconds * 1000; };
            const auto newEnd = std::find_if(partsList.rbegin(), partsList.rend(), predicate).base();
            partsList.erase(newEnd, partsList.end());
        }
    } // namespace
} // namespace

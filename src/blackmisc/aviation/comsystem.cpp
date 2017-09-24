/* Copyright (C) 2013
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "blackmisc/aviation/comsystem.h"
#include "blackmisc/dbus.h"
#include "blackmisc/math/mathutils.h"

#include <QDBusMetaType>
#include <QtDebug>
#include <QtGlobal>

using namespace BlackMisc::PhysicalQuantities;
using namespace BlackMisc::Math;

namespace BlackMisc
{
    namespace Aviation
    {
        void CComSystem::registerMetadata()
        {
            Mixin::MetaType<CComSystem>::registerMetadata();
            qDBusRegisterMetaType<ChannelSpacing>();
            qDBusRegisterMetaType<ComUnit>();
        }

        void CComSystem::setFrequencyActiveMHz(double frequencyMHz)
        {
            const CFrequency f(frequencyMHz, CFrequencyUnit::MHz());
            this->setFrequencyActive(f);
        }

        void CComSystem::setFrequencyStandbyMHz(double frequencyMHz)
        {
            const CFrequency f(frequencyMHz, CFrequencyUnit::MHz());
            this->setFrequencyStandby(f);
        }

        void CComSystem::setFrequencyActive(const CFrequency &frequency)
        {
            if (frequency == this->getFrequencyActive()) { return; } // save all the comparisons / rounding
            CFrequency fRounded(frequency);
            roundToChannelSpacing(fRounded, this->m_channelSpacing);
            this->CModulator::setFrequencyActive(fRounded);
        }

        void CComSystem::setFrequencyStandby(const CFrequency &frequency)
        {
            if (frequency == this->getFrequencyStandby()) { return; } // save all the comparisons / rounding
            CFrequency fRounded(frequency);
            roundToChannelSpacing(fRounded, this->m_channelSpacing);
            this->CModulator::setFrequencyStandby(fRounded);
        }

        bool CComSystem::isActiveFrequencyWithin8_33kHzChannel(const CFrequency &comFrequency) const
        {
            return isWithinChannelSpacing(this->getFrequencyActive(), comFrequency, ChannelSpacing8_33KHz);
        }

        bool CComSystem::isActiveFrequencyWithin25kHzChannel(const CFrequency &comFrequency) const
        {
            return isWithinChannelSpacing(this->getFrequencyActive(), comFrequency, ChannelSpacing25KHz);
        }

        void CComSystem::setActiveUnicom()
        {
            this->toggleActiveStandby();
            this->setFrequencyActive(BlackMisc::PhysicalQuantities::CPhysicalQuantitiesConstants::FrequencyUnicom());
        }

        void CComSystem::setActiveInternationalAirDistress()
        {
            this->toggleActiveStandby();
            this->setFrequencyActive(BlackMisc::PhysicalQuantities::CPhysicalQuantitiesConstants::FrequencyInternationalAirDistress());
        }

        CComSystem CComSystem::getCom1System(double activeFrequencyMHz, double standbyFrequencyMHz)
        {
            return CComSystem(CModulator::NameCom1(), BlackMisc::PhysicalQuantities::CFrequency(activeFrequencyMHz, BlackMisc::PhysicalQuantities::CFrequencyUnit::MHz()), BlackMisc::PhysicalQuantities::CFrequency(standbyFrequencyMHz < 0 ? activeFrequencyMHz : standbyFrequencyMHz, BlackMisc::PhysicalQuantities::CFrequencyUnit::MHz()));
        }

        CComSystem CComSystem::getCom1System(const CFrequency &activeFrequency, const CFrequency &standbyFrequency)
        {
            return CComSystem(CModulator::NameCom1(), activeFrequency, standbyFrequency.isNull() ? activeFrequency : standbyFrequency);
        }

        CComSystem CComSystem::getCom2System(double activeFrequencyMHz, double standbyFrequencyMHz)
        {
            return CComSystem(CModulator::NameCom2(), BlackMisc::PhysicalQuantities::CFrequency(activeFrequencyMHz, BlackMisc::PhysicalQuantities::CFrequencyUnit::MHz()), BlackMisc::PhysicalQuantities::CFrequency(standbyFrequencyMHz < 0 ? activeFrequencyMHz : standbyFrequencyMHz, BlackMisc::PhysicalQuantities::CFrequencyUnit::MHz()));
        }

        CComSystem CComSystem::getCom2System(const CFrequency &activeFrequency, const CFrequency &standbyFrequency)
        {
            return CComSystem(CModulator::NameCom2(), activeFrequency, standbyFrequency.isNull() ? activeFrequency : standbyFrequency);
        }

        bool CComSystem::isValidCivilAviationFrequency(const CFrequency &f)
        {
            if (f.isNull()) return false;
            double fr = f.valueRounded(BlackMisc::PhysicalQuantities::CFrequencyUnit::MHz(), 3);
            return fr >= 118.0 && fr <= 136.975;
        }

        bool CComSystem::isValidMilitaryFrequency(const CFrequency &f)
        {
            if (f.isNull()) return false;
            double fr = f.valueRounded(BlackMisc::PhysicalQuantities::CFrequencyUnit::MHz(), 3);
            return fr >= 220.0 && fr <= 399.95;
        }

        bool CComSystem::isValidComFrequency(const CFrequency &f)
        {
            return isValidCivilAviationFrequency(f) || isValidMilitaryFrequency(f);
        }

        void CComSystem::roundToChannelSpacing(PhysicalQuantities::CFrequency &frequency, ChannelSpacing channelSpacing)
        {
            double channelSpacingKHz = CComSystem::channelSpacingToFrequencyKHz(channelSpacing);
            double f = frequency.valueRounded(CFrequencyUnit::kHz(), 0);
            quint32 d = static_cast<quint32>(f / channelSpacingKHz);
            frequency.switchUnit(CFrequencyUnit::MHz());
            double f0 = frequency.valueRounded(CFrequencyUnit::MHz(), 3);
            double f1 = CMathUtils::round(d * (channelSpacingKHz / 1000.0), 3);
            double f2 = CMathUtils::round((d + 1) * (channelSpacingKHz / 1000.0), 3);
            bool down = qAbs(f1 - f0) < qAbs(f2 - f0); // which is the closest value
            frequency.setCurrentUnitValue(down ? f1 : f2);
        }

        bool CComSystem::isWithinChannelSpacing(const CFrequency &setFrequency, const CFrequency &compareFrequency, CComSystem::ChannelSpacing channelSpacing)
        {
            if (setFrequency == compareFrequency) return true; // shortcut for many of such comparisons
            double channelSpacingKHz = 0.5 * CComSystem::channelSpacingToFrequencyKHz(channelSpacing);
            double compareFrequencyKHz = compareFrequency.value(CFrequencyUnit::kHz());
            double setFrequencyKHz = setFrequency.value(CFrequencyUnit::kHz());
            return (setFrequencyKHz - channelSpacingKHz < compareFrequencyKHz) &&
                   (setFrequencyKHz + channelSpacingKHz > compareFrequencyKHz);
        }

        double CComSystem::channelSpacingToFrequencyKHz(ChannelSpacing channelSpacing)
        {
            switch (channelSpacing)
            {
            case ChannelSpacing50KHz: return 50.0;
            case ChannelSpacing25KHz: return 25.0;
            case ChannelSpacing8_33KHz: return 25.0 / 3.0;
            default: qFatal("Wrong channel spacing"); return 0.0; // return just supressing compiler warning
            }
        }
    } // namespace
} // namespace

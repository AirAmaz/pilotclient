/* Copyright (C) 2013
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "blackmisc/pq/pqstring.h"
#include "blackmisc/pq/acceleration.h"
#include "blackmisc/pq/angle.h"
#include "blackmisc/pq/frequency.h"
#include "blackmisc/pq/length.h"
#include "blackmisc/pq/mass.h"
#include "blackmisc/pq/measurementunit.h"
#include "blackmisc/pq/pressure.h"
#include "blackmisc/pq/speed.h"
#include "blackmisc/pq/temperature.h"
#include "blackmisc/pq/time.h"
#include "blackmisc/pq/units.h"

#include <QLocale>
#include <QRegularExpression>
#include <QThreadStorage>
#include <QtDebug>

namespace BlackMisc
{
    namespace PhysicalQuantities
    {

        QString CPqString::convertToQString(bool /** i18n **/) const
        {
            return this->m_string;
        }

        CVariant CPqString::parseToVariant(const QString &value, SeparatorMode mode)
        {
            CVariant v;

            // fine tuning of the string
            QString vs = value.trimmed().simplified();

            // check
            if (vs.isEmpty()) { return v; }

            thread_local const QRegularExpression regex("([-+]?[0-9]*[\\.,]?[0-9]+)\\s*(\\D*)$");
            auto match = regex.match(value);
            if (!match.hasMatch()) { return v; } // not a valid number
            QString unit = match.captured(2).trimmed();
            QString number = QString(value).replace(unit, "");
            unit = unit.trimmed(); // trim after replace, not before

            if (unit.isEmpty() || number.isEmpty()) { return v; }
            bool success;
            double numberD = parseNumber(number, success, mode);
            if (!success) {return v; }

            if (CMeasurementUnit::isValidUnitSymbol<CAccelerationUnit>(unit))
            {
                CAcceleration pq(numberD, CMeasurementUnit::unitFromSymbol<CAccelerationUnit>(unit, false));
                return CVariant::from(pq);
            }

            if (CMeasurementUnit::isValidUnitSymbol<CAngleUnit>(unit))
            {
                CAngle pq(numberD, CMeasurementUnit::unitFromSymbol<CAngleUnit>(unit, false));
                return CVariant::from(pq);
            }

            if (CMeasurementUnit::isValidUnitSymbol<CFrequencyUnit>(unit))
            {
                CFrequency pq(numberD, CMeasurementUnit::unitFromSymbol<CFrequencyUnit>(unit, false));
                return CVariant::from(pq);
            }

            if (CMeasurementUnit::isValidUnitSymbol<CLengthUnit>(unit))
            {
                CLength pq(numberD, CMeasurementUnit::unitFromSymbol<CLengthUnit>(unit, false));
                return CVariant::from(pq);
            }

            if (CMeasurementUnit::isValidUnitSymbol<CMassUnit>(unit))
            {
                CMass pq(numberD, CMeasurementUnit::unitFromSymbol<CMassUnit>(unit, false));
                return CVariant::from(pq);
            }

            if (CMeasurementUnit::isValidUnitSymbol<CPressureUnit>(unit))
            {
                CPressure pq(numberD, CMeasurementUnit::unitFromSymbol<CPressureUnit>(unit, false));
                return CVariant::from(pq);
            }

            if (CMeasurementUnit::isValidUnitSymbol<CSpeedUnit>(unit))
            {
                CSpeed pq(numberD, CMeasurementUnit::unitFromSymbol<CSpeedUnit>(unit, false));
                return CVariant::from(pq);
            }

            if (CMeasurementUnit::isValidUnitSymbol<CTimeUnit>(unit))
            {
                CTime pq(numberD, CMeasurementUnit::unitFromSymbol<CTimeUnit>(unit, false));
                return CVariant::from(pq);
            }

            if (CMeasurementUnit::isValidUnitSymbol<CTemperatureUnit>(unit))
            {
                CTemperature pq(numberD, CMeasurementUnit::unitFromSymbol<CTemperatureUnit>(unit, false));
                return CVariant::from(pq);
            }
            return v;
        }

        double CPqString::parseNumber(const QString &number, bool &success, CPqString::SeparatorMode mode)
        {
            double numberD = -1;
            switch (mode)
            {
            case SeparatorsLocale:
                numberD = QLocale::system().toDouble(number, &success);
                break;
            case SeparatorsCLocale:
                numberD = number.toDouble(&success);
                break;
            case SeparatorsBestGuess:
                numberD = number.toDouble(&success);
                if (!success) { numberD = QLocale::system().toDouble(number, &success); }
                break;
            default:
                qFatal("Wrong mode");
                break;
            }
            return numberD;
        }

    } // namespace
} // namespace

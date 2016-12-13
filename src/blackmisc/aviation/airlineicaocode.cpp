/* Copyright (C) 2013
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "airlineicaocode.h"
#include "callsign.h"
#include "blackmisc/db/datastoreutility.h"
#include "blackmisc/comparefunctions.h"
#include "blackmisc/icons.h"
#include "blackmisc/logcategory.h"
#include "blackmisc/logcategorylist.h"
#include "blackmisc/propertyindex.h"
#include "blackmisc/statusmessage.h"
#include "blackmisc/stringutils.h"
#include "blackmisc/variant.h"

#include <QJsonValue>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QThreadStorage>
#include <Qt>
#include <QtGlobal>

using namespace BlackMisc;
using namespace BlackMisc::Db;

namespace BlackMisc
{
    namespace Aviation
    {
        CAirlineIcaoCode::CAirlineIcaoCode(const QString &airlineDesignator)
            : m_designator(airlineDesignator.trimmed().toUpper())
        {
            if (this->m_designator.length() == 4)
            {
                this->setDesignator(this->m_designator);
            }
        }

        CAirlineIcaoCode::CAirlineIcaoCode(const QString &airlineDesignator, const QString &airlineName, const BlackMisc::CCountry &country, const QString &telephony, bool virtualAirline, bool operating)
            : m_designator(airlineDesignator.trimmed().toUpper()), m_name(airlineName), m_country(country), m_telephonyDesignator(telephony), m_isVa(virtualAirline), m_isOperating(operating)
        {
            if (this->m_designator.length() == 4)
            {
                this->setDesignator(this->m_designator);
            }
        }

        const QString CAirlineIcaoCode::getVDesignator() const
        {
            if (!isVirtualAirline()) { return this->m_designator; }
            return QString("V").append(this->m_designator);
        }

        QString CAirlineIcaoCode::getVDesignatorDbKey() const
        {
            if (this->isLoadedFromDb())
            {
                return this->getVDesignator() + " " + this->getDbKeyAsStringInParentheses();
            }
            else
            {
                return this->getVDesignator();
            }
        }

        void CAirlineIcaoCode::setDesignator(const QString &icaoDesignator)
        {
            this->m_designator = icaoDesignator.trimmed().toUpper();
            if (this->m_designator.length() == 4 && this->m_designator.startsWith("V"))
            {
                // a virtual designator was provided
                this->setVirtualAirline(true);
                this->m_designator = this->m_designator.right(3);
            }
        }

        QString CAirlineIcaoCode::getDesignatorNameCountry() const
        {
            QString s(this->getDesignator());
            if (this->hasName()) { s = s.append(" ").append(this->getName()); }
            if (this->hasValidCountry()) { s = s.append(" ").append(this->getCountryIso()); }
            return s.trimmed();
        }

        QString CAirlineIcaoCode::getSimplifiedName() const
        {
            return BlackMisc::simplifyNameForSearch(this->getName());
        }

        bool CAirlineIcaoCode::hasValidCountry() const
        {
            return this->m_country.isValid();
        }

        bool CAirlineIcaoCode::hasValidDesignator() const
        {
            return isValidAirlineDesignator(m_designator);
        }

        bool CAirlineIcaoCode::hasIataCode() const
        {
            return !this->m_iataCode.isEmpty();
        }

        bool CAirlineIcaoCode::matchesDesignator(const QString &designator) const
        {
            if (designator.isEmpty()) { return false; }
            return designator.trimmed().toUpper() == this->m_designator;
        }

        bool CAirlineIcaoCode::matchesVDesignator(const QString &designator) const
        {
            if (designator.isEmpty()) { return false; }
            return designator.trimmed().toUpper() == this->getVDesignator();
        }

        bool CAirlineIcaoCode::matchesIataCode(const QString &iata) const
        {
            if (iata.isEmpty()) { return false; }
            return iata.trimmed().toUpper() == this->m_iataCode;
        }

        bool CAirlineIcaoCode::matchesDesignatorOrIataCode(const QString &candidate) const
        {
            if (candidate.isEmpty()) { return false; }
            return this->matchesDesignator(candidate) || this->matchesIataCode(candidate);
        }

        bool CAirlineIcaoCode::matchesVDesignatorOrIataCode(const QString &candidate) const
        {
            if (candidate.isEmpty()) { return false; }
            return this->matchesVDesignator(candidate) || this->matchesIataCode(candidate);
        }

        bool CAirlineIcaoCode::matchesNamesOrTelephonyDesignator(const QString &candidate) const
        {
            const QString cand(candidate.toUpper().trimmed());
            if (this->getName().contains(cand, Qt::CaseInsensitive) || this->getTelephonyDesignator().contains(cand, Qt::CaseInsensitive)) { return true; }
            return this->isContainedInSimplifiedName(candidate);
        }

        bool CAirlineIcaoCode::isContainedInSimplifiedName(const QString &candidate) const
        {
            if (candidate.isEmpty() || !this->hasName()) { return false; }
            auto simplifiedName = makeRange(getName().begin(), getName().end()).findBy([](QChar c) { return c.isLetter(); });
            auto it = std::search(simplifiedName.begin(), simplifiedName.end(), candidate.begin(), candidate.end(), [](QChar a, QChar b) { return a.toUpper() == b.toUpper(); });
            return it != simplifiedName.end();
        }

        bool CAirlineIcaoCode::hasSimplifiedName() const
        {
            return this->hasName() && !this->getSimplifiedName().isEmpty();
        }

        bool CAirlineIcaoCode::hasCompleteData() const
        {
            return this->hasValidDesignator() && this->hasValidCountry() && this->hasName();
        }

        CIcon CAirlineIcaoCode::toIcon() const
        {
            if (hasValidDesignator())
            {
                // relative to images
                return CIcon("airlines/" + m_designator.toLower() + ".png", this->convertToQString());
            }
            else
            {
                return CIcon::iconByIndex(CIcons::StandardIconEmpty);
            }
        }

        QString CAirlineIcaoCode::convertToQString(bool i18n) const
        {
            Q_UNUSED(i18n);
            QString s(this->m_designator);
            if (this->m_name.isEmpty()) { return ""; }
            if (!this->m_name.isEmpty()) { s.append(" (").append(this->m_name).append(")"); }

            s.append(" Op: ").append(boolToYesNo(this->isOperating()));
            s.append(" VA: ").append(boolToYesNo(this->isVirtualAirline()));
            s.append(" Mil: ").append(boolToYesNo(this->isMilitary()));

            return s;
        }

        CVariant CAirlineIcaoCode::propertyByIndex(const BlackMisc::CPropertyIndex &index) const
        {
            if (index.isMyself()) { return CVariant::from(*this); }
            if (IDatastoreObjectWithIntegerKey::canHandleIndex(index)) { return IDatastoreObjectWithIntegerKey::propertyByIndex(index); }
            ColumnIndex i = index.frontCasted<ColumnIndex>();
            switch (i)
            {
            case IndexAirlineDesignator:
                return CVariant::fromValue(this->m_designator);
            case IndexIataCode:
                return CVariant::fromValue(this->m_iataCode);
            case IndexAirlineCountryIso:
                return CVariant::fromValue(this->getCountryIso());
            case IndexAirlineCountry:
                return this->m_country.propertyByIndex(index.copyFrontRemoved());
            case IndexAirlineName:
                return CVariant::fromValue(this->m_name);
            case IndexTelephonyDesignator:
                return CVariant::fromValue(this->m_telephonyDesignator);
            case IndexIsVirtualAirline:
                return CVariant::fromValue(this->m_isVa);
            case IndexIsOperating:
                return CVariant::fromValue(this->m_isOperating);
            case IndexIsMilitary:
                return CVariant::fromValue(this->m_isMilitary);
            case IndexDesignatorNameCountry:
                return CVariant::fromValue(this->getDesignatorNameCountry());
            default:
                return CValueObject::propertyByIndex(index);
            }
        }

        void CAirlineIcaoCode::setPropertyByIndex(const CPropertyIndex &index, const CVariant &variant)
        {
            if (index.isMyself()) { (*this) = variant.to<CAirlineIcaoCode>(); return; }
            if (IDatastoreObjectWithIntegerKey::canHandleIndex(index)) { IDatastoreObjectWithIntegerKey::setPropertyByIndex(index, variant); return; }
            ColumnIndex i = index.frontCasted<ColumnIndex>();
            switch (i)
            {
            case IndexAirlineDesignator:
                this->setDesignator(variant.value<QString>());
                break;
            case IndexIataCode:
                this->setIataCode(variant.value<QString>());
                break;
            case IndexAirlineCountry:
                this->setCountry(variant.value<CCountry>());
                break;
            case IndexAirlineName:
                this->setName(variant.value<QString>());
                break;
            case IndexTelephonyDesignator:
                this->setTelephonyDesignator(variant.value<QString>());
                break;
            case IndexIsVirtualAirline:
                this->setVirtualAirline(variant.toBool());
                break;
            case IndexIsOperating:
                this->setOperating(variant.toBool());
                break;
            case IndexIsMilitary:
                this->setMilitary(variant.toBool());
                break;
            default:
                CValueObject::setPropertyByIndex(index, variant);
                break;
            }
        }

        int CAirlineIcaoCode::comparePropertyByIndex(const CPropertyIndex &index, const CAirlineIcaoCode &compareValue) const
        {
            if (index.isMyself()) { return m_designator.compare(compareValue.getDesignator(), Qt::CaseInsensitive); }
            if (IDatastoreObjectWithIntegerKey::canHandleIndex(index)) { return IDatastoreObjectWithIntegerKey::comparePropertyByIndex(index, compareValue);}
            const ColumnIndex i = index.frontCasted<ColumnIndex>();
            switch (i)
            {
            case IndexAirlineDesignator:
                return this->m_designator.compare(compareValue.getDesignator());
            case IndexIataCode:
                return this->m_iataCode.compare(compareValue.getIataCode());
            case IndexAirlineCountry:
                return this->m_country.comparePropertyByIndex(index.copyFrontRemoved(), compareValue.getCountry());
            case IndexDesignatorNameCountry:
                return this->m_country.getName().compare(compareValue.getCountry().getName(), Qt::CaseInsensitive);
            case IndexAirlineName:
                return this->m_name.compare(compareValue.getName(), Qt::CaseInsensitive);
            case IndexTelephonyDesignator:
                return this->m_telephonyDesignator.compare(compareValue.getTelephonyDesignator(), Qt::CaseInsensitive);
            case IndexIsVirtualAirline:
                return Compare::compare(this->isVirtualAirline(), compareValue.isVirtualAirline());
            case IndexIsOperating:
                return Compare::compare(this->isOperating(), compareValue.isOperating());
            case IndexIsMilitary:
                return Compare::compare(this->isMilitary(), compareValue.isMilitary());
            default:
                break;
            }
            Q_ASSERT_X(false, Q_FUNC_INFO, "No compare function");
            return 0;
        }

        CStatusMessageList CAirlineIcaoCode::validate() const
        {
            static const CLogCategoryList cats(CLogCategoryList(this).join({ CLogCategory::validation() }));
            CStatusMessageList msgs;
            if (!hasValidDesignator()) { msgs.push_back(CStatusMessage(cats, CStatusMessage::SeverityError, "Airline: missing designator")); }
            if (!hasValidCountry()) { msgs.push_back(CStatusMessage(cats, CStatusMessage::SeverityError, "Airline: missing country")); }
            if (!hasName()) { msgs.push_back(CStatusMessage(cats, CStatusMessage::SeverityError, "Airline: no name")); }
            return msgs;
        }

        bool CAirlineIcaoCode::isValidAirlineDesignator(const QString &airline)
        {
            // allow 2 chars for IATA
            if (airline.length() < 2 || airline.length() > 5) { return false; }
            auto chars = makeRange(airline.begin(), airline.end());
            if (chars.containsBy([](QChar c) { return !c.isUpper() && !c.isDigit(); })) { return false; }
            return true;
        }

        QSet<QString> CAirlineIcaoCode::specialValidDesignators()
        {
            static const QSet<QString> valid({ "VV", "VM"});
            return valid;
        }

        QString CAirlineIcaoCode::normalizeDesignator(const QString candidate)
        {
            QString n(candidate.trimmed().toUpper());
            n = n.left(indexOfChar(n, [](QChar c) { return c.isSpace(); }));
            return removeChars(n, [](QChar c) { return !c.isLetterOrNumber(); });
        }

        QString CAirlineIcaoCode::getCombinedStringWithKey() const
        {
            QString s(getVDesignator());
            if (s.isEmpty()) s = "????";
            if (hasName()) { s = s.append(" ").append(getName()); }
            return s.append(" ").append(getDbKeyAsStringInParentheses());
        }

        CAirlineIcaoCode CAirlineIcaoCode::thisOrCallsignCode(const CCallsign &callsign) const
        {
            if (this->hasValidDbKey()) { return *this; }
            if (callsign.isEmpty()) { return *this; }
            const QString callsignAirline = callsign.getAirlineSuffix();
            if (callsignAirline.isEmpty()) { return *this; }
            if (callsignAirline == this->m_designator) { return *this; }

            const CAirlineIcaoCode callsignIcao(callsignAirline);
            if (this->m_designator.isEmpty()) { return callsignIcao; }

            // here we have 2 possible codes
            if (callsignIcao.isVirtualAirline())
            {

                if (callsignIcao.getDesignator().endsWith(this->m_designator))
                {
                    // callsign ICAO is virtual airline of myself, this is more accurate
                    return callsignIcao;
                }
            }
            return *this;
        }

        QString CAirlineIcaoCode::getNameWithKey() const
        {
            if (!hasValidDbKey()) { return getName(); }
            if (hasName())
            {
                return QString(getName()).append(" ").append(getDbKeyAsStringInParentheses());
            }
            else
            {
                return getDbKeyAsStringInParentheses();
            }
        }

        void CAirlineIcaoCode::updateMissingParts(const CAirlineIcaoCode &otherIcaoCode)
        {
            if (!this->hasValidDbKey() && otherIcaoCode.hasValidDbKey())
            {
                // we have no DB data, but the other one has
                // so we change roles. We take the DB object as base, and update our parts
                CAirlineIcaoCode copy(otherIcaoCode);
                copy.updateMissingParts(*this);
                *this = copy;
                return;
            }

            if (!this->hasValidDesignator()) { this->setDesignator(otherIcaoCode.getDesignator()); }
            if (!this->hasValidCountry()) { this->setCountry(otherIcaoCode.getCountry()); }
            if (!this->hasName()) { this->setName(otherIcaoCode.getName()); }
            if (!this->hasTelephonyDesignator()) { this->setTelephonyDesignator(otherIcaoCode.getTelephonyDesignator()); }
            if (!this->hasValidDbKey())
            {
                this->setDbKey(otherIcaoCode.getDbKey());
                this->setUtcTimestamp(otherIcaoCode.getUtcTimestamp());
            }
        }

        QString CAirlineIcaoCode::asHtmlSummary() const
        {
            return this->getCombinedStringWithKey();
        }

        int CAirlineIcaoCode::calculateScore(const CAirlineIcaoCode &otherCode) const
        {
            const bool bothFromDb = otherCode.isLoadedFromDb() && this->isLoadedFromDb();
            if (bothFromDb && this->getDbKey() == otherCode.getDbKey())
            {
                return 100;
            }
            int score = 0;
            if (otherCode.hasValidDesignator() && this->getDesignator() == otherCode.getDesignator())
            {
                score += 60;
            }

            if (bothFromDb && this->isVirtualAirline() == otherCode.isVirtualAirline())
            {
                score += 20;
            }
            if (this->hasName() && this->getName() == otherCode.getName())
            {
                score += 20;
            }
            else if (this->hasTelephonyDesignator() && this->getTelephonyDesignator() == otherCode.getTelephonyDesignator())
            {
                score += 15;
            }
            else if (this->hasSimplifiedName() && this->getSimplifiedName() == otherCode.getSimplifiedName())
            {
                score += 10;
            }
            return score;
        }

        CAirlineIcaoCode CAirlineIcaoCode::fromDatabaseJson(const QJsonObject &json, const QString &prefix)
        {
            if (!existsKey(json, prefix))
            {
                // when using relationship, this can be null (e.g. for color liveries)
                return CAirlineIcaoCode();
            }

            QString designator(json.value(prefix + "designator").toString());
            if (!CAirlineIcaoCode::isValidAirlineDesignator(designator))
            {
                designator = CAirlineIcaoCode::normalizeDesignator(designator);
            }

            const QString iata(json.value(prefix + "iata").toString());
            const QString telephony(json.value(prefix + "callsign").toString());
            const QString name(json.value(prefix + "name").toString());
            const QString countryIso(json.value(prefix + "country").toString());
            const QString countryName(json.value(prefix + "countryname").toString());
            const bool va = CDatastoreUtility::dbBoolStringToBool(json.value(prefix + "va").toString());
            const bool operating = CDatastoreUtility::dbBoolStringToBool(json.value(prefix + "operating").toString());
            const bool military = CDatastoreUtility::dbBoolStringToBool(json.value(prefix + "military").toString());

            CAirlineIcaoCode code(
                designator, name,
                CCountry(countryIso, countryName),
                telephony, va, operating
            );
            code.setIataCode(iata);
            code.setMilitary(military);
            code.setKeyAndTimestampFromDatabaseJson(json, prefix);
            return code;
        }
    } // namespace
} // namespace

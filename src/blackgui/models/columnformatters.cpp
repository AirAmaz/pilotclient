/* Copyright (C) 2013
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "columnformatters.h"
#include "blackmisc/geo/latitude.h"
#include "blackmisc/variant.h"
#include "blackmisc/iconlist.h"
#include "blackmisc/icons.h"

using namespace BlackMisc;

namespace BlackGui
{
    namespace Models
    {

        Qt::ItemFlags CDefaultFormatter::flags(Qt::ItemFlags flags, bool editable) const
        {
            return editable ? (flags | Qt::ItemIsEditable) : (flags ^ Qt::ItemIsEditable);
        }

        CVariant CDefaultFormatter::displayRole(const CVariant &dataCVariant) const
        {
            return keepStandardTypesConvertToStringOtherwise(dataCVariant);
        }

        CVariant CDefaultFormatter::editRole(const CVariant &dataCVariant) const
        {
            return keepStandardTypesConvertToStringOtherwise(dataCVariant);
        }

        CVariant CDefaultFormatter::tooltipRole(const CVariant &value) const
        {
            if (static_cast<QMetaType::Type>(value.type()) == QMetaType::QString) { return value; }
            return value.toQString(m_useI18n);
        }

        CVariant CDefaultFormatter::decorationRole(const CVariant &dataCVariant) const
        {
            // direct return if type is already correct
            if (static_cast<QMetaType::Type>(dataCVariant.type()) == QMetaType::QPixmap) { return dataCVariant; }
            if (static_cast<QMetaType::Type>(dataCVariant.type()) == QMetaType::QIcon) { return dataCVariant; }

            // convert to pixmap
            if (static_cast<QMetaType::Type>(dataCVariant.type()) == QMetaType::QImage)
            {
                QImage img = dataCVariant.value<QImage>();
                return CVariant::from(QPixmap::fromImage(img));
            }

            // Our CIcon class
            if (dataCVariant.canConvert<BlackMisc::CIcon>())
            {
                BlackMisc::CIcon i = dataCVariant.value<BlackMisc::CIcon>();
                return CVariant::from(i.toPixmap());
            }

            // nope
            return CVariant::from(QPixmap());
        }

        CVariant CDefaultFormatter::alignmentRole() const
        {
            if (!this->hasAlignment())
            {
                return CVariant::from(alignDefault()); // default
            }
            else
            {
                return CVariant::from(m_alignment);
            }
        }

        CVariant CDefaultFormatter::checkStateRole(const CVariant &value) const
        {
            bool b = value.toBool();
            Qt::CheckState cs = b ? Qt::Checked : Qt::Unchecked;
            return CVariant::fromValue(static_cast<int>(cs));
        }

        CVariant CDefaultFormatter::data(int role, const CVariant &inputData) const
        {
            Qt::ItemDataRole roleEnum = static_cast<Qt::ItemDataRole>(role);

            // always supported
            if (roleEnum == Qt::TextAlignmentRole) return { alignmentRole() };

            // check
            if (role == Qt::UserRole) { return CDefaultFormatter::displayRole(inputData); } // just as data provider
            if (this->m_supportedRoles.isEmpty()) { return CVariant(); }
            if (!this->m_supportedRoles.contains(role)) { return CVariant(); }
            switch (roleEnum)
            {
            case Qt::DisplayRole:
                // formatted to standard types or string
                return displayRole(inputData);
            case Qt::EditRole:
                // formatted to standard types or string
                return editRole(inputData);
            case Qt::ToolTipRole:
                // formatted to string
                return tooltipRole(inputData);
            case Qt::DecorationRole:
                // formatted as pixmap, icon, or color
                return decorationRole(inputData);
            case Qt::CheckStateRole:
                // as Qt check state
                return checkStateRole(inputData);
            default:
                break;
            }
            return CVariant();
        }

        int CDefaultFormatter::alignDefault()
        {
            return alignLeftVCenter();
        }

        CVariant CDefaultFormatter::keepStandardTypesConvertToStringOtherwise(const CVariant &inputData) const
        {
            if (static_cast<QMetaType::Type>(inputData.type()) == QMetaType::QString) { return inputData; }
            if (static_cast<QMetaType::Type>(inputData.type()) == QMetaType::Bool)  { return inputData; }
            if (static_cast<QMetaType::Type>(inputData.type()) == QMetaType::Int) { return inputData; }
            return inputData.toQString(m_useI18n);
        }

        CVariant CPixmapFormatter::displayRole(const CVariant &dataCVariant) const
        {
            Q_UNUSED(dataCVariant);
            Q_ASSERT_X(false, "CPixmapFormatter", "this role should be disabled with pixmaps");
            return CVariant();
        }

        CVariant CPixmapFormatter::tooltipRole(const CVariant &dataCVariant) const
        {
            if (dataCVariant.isNull()) return "";
            if (dataCVariant.canConvert<BlackMisc::CIcon>())
            {
                BlackMisc::CIcon icon = dataCVariant.value<BlackMisc::CIcon>();
                return icon.getDescriptiveText();
            }
            return "";
        }

        CVariant CValueObjectFormatter::displayRole(const CVariant &valueObject) const
        {
            return CVariant(valueObject.toQString(m_useI18n));
        }

        CVariant CValueObjectFormatter::decorationRole(const CVariant &valueObject) const
        {
            return CVariant(valueObject.toPixmap());
        }

        CDateTimeFormatter::CDateTimeFormatter(const QString &formatString, int alignment, bool i18n) :
            CDefaultFormatter(alignment, i18n, { Qt::DisplayRole }), m_formatString(formatString)
        {
            // void
        }

        CVariant CDateTimeFormatter::displayRole(const CVariant &dateTime) const
        {
            if (dateTime.isNull()) return "";
            if (static_cast<QMetaType::Type>(dateTime.type()) == QMetaType::QDateTime)
            {
                QDateTime dt = dateTime.value<QDateTime>();
                return dt.toString(m_formatString);
            }
            else if (static_cast<QMetaType::Type>(dateTime.type()) == QMetaType::QDate)
            {
                QDate d = dateTime.value<QDate>();
                return d.toString(m_formatString);
            }
            else if (static_cast<QMetaType::Type>(dateTime.type()) == QMetaType::QTime)
            {
                QTime t = dateTime.value<QTime>();
                return t.toString(m_formatString);
            }
            else if (static_cast<QMetaType::Type>(dateTime.type()) == QMetaType::Int)
            {
                //! \todo potential risk if int is not qint64
                QDateTime t = QDateTime::fromMSecsSinceEpoch(dateTime.toInt());
                return t.toString(m_formatString);
            }
            else
            {
                Q_ASSERT_X(false, "formatQVariant", "No QDate, QTime or QDateTime");
                return "";
            }
        }

        CVariant CAirspaceDistanceFormatter::displayRole(const CVariant &dataCVariant) const
        {
            if (dataCVariant.canConvert<BlackMisc::PhysicalQuantities::CLength>())
            {
                // special treatment for some cases
                BlackMisc::PhysicalQuantities::CLength l = dataCVariant.value<BlackMisc::PhysicalQuantities::CLength>();
                if (!l.isNull() && (l.isPositiveWithEpsilonConsidered() || l.isZeroEpsilonConsidered()))
                {
                    return CPhysiqalQuantiyFormatter::displayRole(dataCVariant);
                }
                else
                {
                    return "";
                }
            }
            else
            {
                Q_ASSERT_X(false, "CAirspaceDistanceFormatter::formatQVariant", "No CLength class");
                return "";
            }
        }

        CVariant CComFrequencyFormatter::displayRole(const CVariant &dataCVariant) const
        {
            if (dataCVariant.canConvert<BlackMisc::PhysicalQuantities::CFrequency>())
            {
                // speical treatment for some cases
                BlackMisc::PhysicalQuantities::CFrequency f = dataCVariant.value<BlackMisc::PhysicalQuantities::CFrequency>();
                if (BlackMisc::Aviation::CComSystem::isValidComFrequency(f))
                {
                    return CPhysiqalQuantiyFormatter::displayRole(dataCVariant);
                }
                else
                {
                    return "";
                }
            }
            else
            {
                Q_ASSERT_X(false, "CAviationComFrequencyFormatter::formatQVariant", "No CFrequency class");
                return "";
            }
        }

        CVariant CAircraftSpeedFormatter::displayRole(const CVariant &dataCVariant) const
        {
            // special treatment for some cases
            BlackMisc::PhysicalQuantities::CSpeed s = dataCVariant.value<BlackMisc::PhysicalQuantities::CSpeed>();
            if (!s.isNull() && (s.isPositiveWithEpsilonConsidered() || s.isZeroEpsilonConsidered()))
            {
                return CPhysiqalQuantiyFormatter::displayRole(dataCVariant);
            }
            else
            {
                return "";
            }
        }

        CVariant CStringFormatter::displayRole(const CVariant &dataCVariant) const
        {
            if (dataCVariant.canConvert<QString>()) { return dataCVariant; }
            Q_ASSERT_X(false, "CStringFormatter", "no string value");
            return CVariant();
        }

        Qt::ItemFlags CDelegateFormatter::flags(Qt::ItemFlags flags, bool editable) const
        {
            flags = CDefaultFormatter::flags(flags, editable);
            return flags;
        }

        CVariant CBoolTextFormatter::displayRole(const CVariant &dataCVariant) const
        {
            if (dataCVariant.canConvert<bool>())
            {
                bool v = dataCVariant.toBool();
                return v ? CVariant(m_trueName) : CVariant(m_falseName);
            }
            Q_ASSERT_X(false, "CBoolTextFormatter", "no boolean value");
            return CVariant();
        }

        Qt::ItemFlags CBoolTextFormatter::flags(Qt::ItemFlags flags, bool editable) const
        {
            return CDefaultFormatter::flags(flags, editable);
        }

        CBoolLedFormatter::CBoolLedFormatter(int alignment) : CBoolLedFormatter("on", "off", alignment)
        { }

        CBoolLedFormatter::CBoolLedFormatter(const QString &onName, const QString &offName, int alignment) :
            CBoolTextFormatter(alignment, onName, offName, rolesDecorationAndToolTip())
        {
            CLedWidget *led = ledDefault();
            led->setOn(true);
            this->m_pixmapOnLed = led->asPixmap();
            led->setOn(false);
            this->m_pixmapOffLed = led->asPixmap();
            delete led;
        }

        CVariant CBoolLedFormatter::displayRole(const CVariant &dataCVariant) const
        {
            Q_UNUSED(dataCVariant);
            Q_ASSERT_X(false, "CBoolLedFormatter", "this role should be disabled with led boolean");
            return CVariant();
        }

        CVariant CBoolLedFormatter::decorationRole(const CVariant &dataCVariant) const
        {
            if (dataCVariant.canConvert<bool>())
            {
                bool v = dataCVariant.toBool();
                return CVariant::from(v ? m_pixmapOnLed : m_pixmapOffLed);
            }
            Q_ASSERT_X(false, "CBoolLedFormatter", "no boolean value");
            return CVariant();
        }

        CBoolIconFormatter::CBoolIconFormatter(int alignment) :
            CBoolIconFormatter(CIcons::StandardIconTick16, CIcons::StandardIconEmpty16, "on", "off", alignment)
        { }

        CBoolIconFormatter::CBoolIconFormatter(CIcons::IconIndex onIcon, CIcons::IconIndex offIcon, const QString &onName, const QString &offName, int alignment) :
            CBoolIconFormatter(CIconList::iconByIndex(onIcon), CIconList::iconByIndex(offIcon), onName, offName, alignment)
        { }

        CBoolIconFormatter::CBoolIconFormatter(const CIcon &onIcon, const CIcon &offIcon, const QString &onName, const QString &offName, int alignment) :
            CBoolTextFormatter(alignment, onName, offName, rolesDecorationAndToolTip()), m_iconOn(onIcon), m_iconOff(offIcon)
        {
            this->m_iconOn.setDescriptiveText(onName);
            this->m_iconOff.setDescriptiveText(offName);
        }

        CVariant CBoolIconFormatter::displayRole(const CVariant &dataCVariant) const
        {
            Q_UNUSED(dataCVariant);
            Q_ASSERT_X(false, "CBoolIconFormatter", "this role should be disabled with icon boolean");
            return CVariant();
        }

        CVariant CBoolIconFormatter::decorationRole(const CVariant &dataCVariant) const
        {
            if (dataCVariant.canConvert<bool>())
            {
                bool v = dataCVariant.toBool();
                return CVariant::from(v ? m_iconOn.toPixmap() : m_iconOff.toPixmap());
            }
            Q_ASSERT_X(false, "CBoolIconFormatter", "no boolean value");
            return CVariant();
        }

        CVariant CBoolIconFormatter::tooltipRole(const CVariant &dataCVariant) const
        {
            return CBoolTextFormatter::displayRole(dataCVariant);
        }

    } // namespace
} // namespace

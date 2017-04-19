/* Copyright (C) 2013
 * swift Project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file

#ifndef BLACKMISC_AVIATION_TRACK_H
#define BLACKMISC_AVIATION_TRACK_H

#include "blackmisc/blackmiscexport.h"
#include "blackmisc/compare.h"
#include "blackmisc/dbus.h"
#include "blackmisc/dictionary.h"
#include "blackmisc/json.h"
#include "blackmisc/metaclass.h"
#include "blackmisc/pq/angle.h"
#include "blackmisc/pq/units.h"
#include "blackmisc/propertyindexvariantmap.h"
#include "blackmisc/stringutils.h"
#include "blackmisc/variant.h"

#include <QMetaType>
#include <QString>
#include <QtGlobal>

namespace BlackMisc
{
    namespace Aviation
    {

        /*!
         * \brief Track as used in aviation, can be true or magnetic Track
         * \remarks Intentionally allowing +/- BlackMisc::PhysicalQuantities::CAngle ,
         *          and >= / <= CAngle.
         */
        class BLACKMISC_EXPORT CTrack :
            public PhysicalQuantities::CAngle,
            public Mixin::MetaType<CTrack>,
            public Mixin::EqualsByMetaClass<CTrack>,
            public Mixin::CompareByMetaClass<CTrack>,
            public Mixin::HashByMetaClass<CTrack>,
            public Mixin::DBusByMetaClass<CTrack>,
            public Mixin::JsonByMetaClass<CTrack>,
            public Mixin::String<CTrack>,
            public Mixin::Index<CTrack>
        {
        public:
            //! Base type
            using base_type = PhysicalQuantities::CAngle;

            BLACKMISC_DECLARE_USING_MIXIN_METATYPE(CTrack)
            BLACKMISC_DECLARE_USING_MIXIN_STRING(CTrack)
            BLACKMISC_DECLARE_USING_MIXIN_DBUS(CTrack)
            BLACKMISC_DECLARE_USING_MIXIN_JSON(CTrack)
            BLACKMISC_DECLARE_USING_MIXIN_INDEX(CTrack)

            /*!
             * Enum type to distinguish between true north and magnetic north
             */
            enum ReferenceNorth
            {
                Magnetic = 0,   //!< magnetic north
                True = 1        //!< true north
            };

            //! \brief Default constructor: 0 Track magnetic
            CTrack() : CAngle(0, BlackMisc::PhysicalQuantities::CAngleUnit::rad()), m_north(Magnetic) {}

            //! \brief Constructor
            CTrack(double value, ReferenceNorth north, const BlackMisc::PhysicalQuantities::CAngleUnit &unit) : CAngle(value, unit), m_north(north) {}

            //! \brief Constructor by CAngle
            CTrack(const BlackMisc::PhysicalQuantities::CAngle &track, ReferenceNorth north) : CAngle(track), m_north(north) {}

            //! \brief Magnetic Track?
            bool isMagneticTrack() const
            {
                (void)QT_TRANSLATE_NOOP("Aviation", "magnetic");
                return Magnetic == this->m_north;
            }

            //! \brief True Track?
            bool isTrueTrack() const
            {
                (void)QT_TRANSLATE_NOOP("Aviation", "true");
                return True == this->m_north;
            }

            //! \brief Get reference north (magnetic or true)
            ReferenceNorth getReferenceNorth() const { return m_north; }

            //! \copydoc BlackMisc::Mixin::String::toQString
            QString convertToQString(bool i18n = false) const;

        private:
            ReferenceNorth m_north; //!< magnetic or true?

            BLACK_METACLASS(
                CTrack,
                BLACK_METAMEMBER(north)
            );
        };
    }
}

Q_DECLARE_METATYPE(BlackMisc::Aviation::CTrack)

#endif // guard

/* Copyright (C) 2016
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file

#ifndef BLACKCORE_VATSIM_SETTINGS_H
#define BLACKCORE_VATSIM_SETTINGS_H

#include "blackcore/blackcoreexport.h"
#include "blackmisc/settingscache.h"
#include "blackmisc/valueobject.h"
#include "blackmisc/pq/time.h"
#include "blackmisc/network/serverlist.h"

namespace BlackCore
{
    namespace Vatsim
    {
        /*!
         * Virtual air traffic servers
         */
        struct TTrafficServers : public BlackMisc::TSettingTrait<BlackMisc::Network::CServerList>
        {
            //! \copydoc BlackMisc::TSettingTrait::key
            static const char *key() { return "network/trafficservers"; }
        };

        /*!
         * Currently selected virtual air traffic server
         */
        struct TCurrentTrafficServer : public BlackMisc::TSettingTrait<BlackMisc::Network::CServer>
        {
            //! \copydoc BlackMisc::TSettingTrait::key
            static const char *key() { return "network/currenttrafficserver"; }

            //! \copydoc BlackMisc::TSettingTrait::defaultValue
            static const BlackMisc::Network::CServer &defaultValue()
            {
                using namespace BlackMisc::Network;
                static const CServer dv("Testserver", "Client project testserver", "fsd.swift-project.org", 6809, CUser("guest", "Guest Client project", "", "guest"));
                return dv;
            }
        };

        /*!
         * Settings used with readers
         */
        class BLACKCORE_EXPORT CReaderSettings : public BlackMisc::CValueObject<BlackCore::Vatsim::CReaderSettings>
        {
        public:
            //! Properties by index
            enum ColumnIndex
            {
                IndexInitialTime = BlackMisc::CPropertyIndex::GlobalIndexCSettingsReaders,
                IndexPeriodicTime,
                IndexNeverUpdate
            };

            //! Default constructor.
            CReaderSettings();

            //! Simplified constructor
            CReaderSettings(const BlackMisc::PhysicalQuantities::CTime &initialTime, const BlackMisc::PhysicalQuantities::CTime &periodicTime, bool neverUpdate = false);

            //! Get time
            const BlackMisc::PhysicalQuantities::CTime &getInitialTime() const { return m_initialTime; }

            //! Set time
            void setInitialTime(const BlackMisc::PhysicalQuantities::CTime &time) { m_initialTime = time; }

            //! Get time
            const BlackMisc::PhysicalQuantities::CTime &getPeriodicTime() const { return m_periodicTime; }

            //! Set time
            void setPeriodicTime(const BlackMisc::PhysicalQuantities::CTime &time) { m_periodicTime = time; }

            //! Never ever update?
            bool isNeverUpdate() const { return m_neverUpdate; }

            //! \copydoc BlackMisc::Mixin::Index::propertyByIndex
            BlackMisc::CVariant propertyByIndex(const BlackMisc::CPropertyIndex &index) const;

            //! \copydoc BlackMisc::Mixin::Index::setPropertyByIndex
            void setPropertyByIndex(const BlackMisc::CPropertyIndex &index, const BlackMisc::CVariant &variant);

            //! \copydoc BlackMisc::Mixin::String::toQString
            QString convertToQString(bool i18n = false) const;

            //! Settings used when a reader is manually triggered and never updates
            static const CReaderSettings &neverUpdateSettings();

        private:
            BlackMisc::PhysicalQuantities::CTime m_initialTime { 30.0, BlackMisc::PhysicalQuantities::CTimeUnit::s()};
            BlackMisc::PhysicalQuantities::CTime m_periodicTime { 30.0, BlackMisc::PhysicalQuantities::CTimeUnit::s()};
            bool m_neverUpdate = false;

            BLACK_METACLASS(
                CReaderSettings,
                BLACK_METAMEMBER(initialTime),
                BLACK_METAMEMBER(periodicTime),
                BLACK_METAMEMBER(neverUpdate)
            );
        };

        //! Reader settings
        struct TVatsimBookings : public BlackMisc::TSettingTrait<CReaderSettings>
        {
            //! \copydoc BlackCore::TSettingTrait::key
            static const char *key() { return "vatsimreaders/bookings"; }

            //! \copydoc BlackCore::TSettingTrait::defaultValue
            static const BlackCore::Vatsim::CReaderSettings &defaultValue()
            {
                static const BlackCore::Vatsim::CReaderSettings reader {{30.0, BlackMisc::PhysicalQuantities::CTimeUnit::s()}, {120.0, BlackMisc::PhysicalQuantities::CTimeUnit::s()}};
                return reader;
            }
        };

        //! Reader settings
        struct TVatsimDataFile : public BlackMisc::TSettingTrait<CReaderSettings>
        {
            //! \copydoc BlackCore::TSettingTrait::key
            static const char *key() { return "vatsimreaders/datafile"; }

            //! \copydoc BlackCore::TSettingTrait::defaultValue
            static const BlackCore::Vatsim::CReaderSettings &defaultValue()
            {
                static const BlackCore::Vatsim::CReaderSettings reader {{25.0, BlackMisc::PhysicalQuantities::CTimeUnit::s()}, {120.0, BlackMisc::PhysicalQuantities::CTimeUnit::s()}};
                return reader;
            }
        };

        //! Reader settings
        struct TVatsimMetars : public BlackMisc::TSettingTrait<CReaderSettings>
        {
            //! \copydoc BlackCore::TSettingTrait::key
            static const char *key() { return "vatsimreaders/metars"; }

            //! \copydoc BlackCore::TSettingTrait::defaultValue
            static const BlackCore::Vatsim::CReaderSettings &defaultValue()
            {
                static const BlackCore::Vatsim::CReaderSettings reader {{35.0, BlackMisc::PhysicalQuantities::CTimeUnit::s()}, {300.0, BlackMisc::PhysicalQuantities::CTimeUnit::s()}};
                return reader;
            }
        };
    } // ns
} // ns

Q_DECLARE_METATYPE(BlackCore::Vatsim::CReaderSettings)
Q_DECLARE_METATYPE(BlackMisc::CCollection<BlackCore::Vatsim::CReaderSettings>)
Q_DECLARE_METATYPE(BlackMisc::CSequence<BlackCore::Vatsim::CReaderSettings>)

#endif

/* Copyright (C) 2013
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file

#ifndef BLACKMISC_PROPERTYINDEX_H
#define BLACKMISC_PROPERTYINDEX_H

#include "blackmisc/blackmiscexport.h"
#include "blackmisc/compare.h"
#include "blackmisc/dbus.h"
#include "blackmisc/dictionary.h"
#include "blackmisc/json.h"
#include "blackmisc/metaclass.h"
#include "blackmisc/stringutils.h"
#include "blackmisc/typetraits.h"
#include "blackmisc/variant.h"

#include <QList>
#include <QMetaType>
#include <QString>
#include <initializer_list>
#include <type_traits>

namespace BlackMisc
{
    class CPropertyIndex;

    namespace Private
    {
        //! \private
        template <class T, class X>
        int compareByProperty(const T &a, const T &b, const CPropertyIndex &index, std::true_type, X)
        {
            return a.comparePropertyByIndex(index, b);
        }
        //! \private
        template <class T>
        int compareByProperty(const T &a, const T &b, const CPropertyIndex &index, std::false_type, std::true_type)
        {
            return compare(a.propertyByIndex(index), b.propertyByIndex(index));
        }
        //! \private
        template <class T>
        int compareByProperty(const T &, const T &, const CPropertyIndex &, std::false_type, std::false_type)
        {
            qFatal("Not implemented");
            return 0;
        }
    }

    /*!
     * Property index. The index can be nested, that's why it is a sequence
     * (e.g. PropertyIndexPilot, PropertyIndexRealname).
     */
    class BLACKMISC_EXPORT CPropertyIndex :
        public Mixin::MetaType<CPropertyIndex>,
        public Mixin::HashByMetaClass<CPropertyIndex>,
        public Mixin::DBusByMetaClass<CPropertyIndex>,
        public Mixin::JsonByMetaClass<CPropertyIndex>,
        public Mixin::EqualsByMetaClass<CPropertyIndex>,
        public Mixin::LessThanByMetaClass<CPropertyIndex>,
        public Mixin::CompareByMetaClass<CPropertyIndex>,
        public Mixin::String<CPropertyIndex>
    {
        // In the first trial I have used CSequence<int> as base class. This has created too much circular dependencies of the headers
        // CIndexVariantMap is used in CValueObject, CPropertyIndex in CIndexVariantMap

    public:
        //! Global index, make sure the indexes are unqiue (for using them in class hierarchy)
        enum GlobalIndex
        {
            GlobalIndexCValueObject                     =    10,
            GlobalIndexCPhysicalQuantity                =   100,
            GlobalIndexCStatusMessage                   =   200,
            GlobalIndexCNameVariantPair                 =   300,
            GlobalIndexITimestampBased                  =   400,
            GlobalIndexIOrderable                       =   500,
            GlobalIndexINullable                        =   600,
            GlobalIndexCIdentifier                      =   700,
            GlobalIndexCRgbColor                        =   800,
            GlobalIndexCCountry                         =   900,
            GlobalIndexCCallsign                        =  1000,
            GlobalIndexCAircraftSituation               =  1100,
            GlobalIndexCAtcStation                      =  1200,
            GlobalIndexCAirport                         =  1300,
            GlobalIndexCAircraftParts                   =  1400,
            GlobalIndexCAircraftLights                  =  1500,
            GlobalIndexCLivery                          =  1600,
            GlobalIndexCComSystem                       =  2000,
            GlobalIndexCModulator                       =  2100,
            GlobalIndexCTransponder                     =  2200,
            GlobalIndexCAircraftIcaoData                =  2500,
            GlobalIndexCAircraftIcaoCode                =  2600,
            GlobalIndexCAirlineIcaoCode                 =  2700,
            GlobalIndexCAirportIcaoCode                 =  2800,
            GlobalIndexCMetar                           =  4000,
            GlobalIndexCCloudLayer                      =  4100,
            GlobalIndexCPresentWeather                  =  4200,
            GlobalIndexCWindLayer                       =  4300,
            GlobalIndexCTemperatureLayer                =  4400,
            GlobalIndexCGridPoint                       =  4500,
            GlobalIndexCVisibilityLayer                 =  4600,
            GlobalIndexCWeatherScenario                 =  4700,
            GlobalIndexICoordinateGeodetic              =  5000,
            GlobalIndexICoordinateWithRelativePosition  =  5100,
            GlobalIndexCCoordinateGeodetic              =  5200,
            GlobalIndexCElevationPlane                  =  5300,
            GlobalIndexCClient                          =  6000,
            GlobalIndexClientCapabilities               =  6100, //!< used with map key
            GlobalIndexCUser                            =  6200,
            GlobalIndexCAuthenticatedUser               =  6300,
            GlobalIndexCRole                            =  6400,
            GlobalIndexCServer                          =  6500,
            GlobalIndexCFsdSetup                        =  6600,
            GlobalIndexCUrl                             =  6700,
            GlobalIndexCRemoteFile                      =  6800,
            GlobalIndexCAircraftModel                   =  7000,
            GlobalIndexCSimulatedAircraft               =  7100,
            GlobalIndexCTextMessage                     =  7200,
            GlobalIndexCSimulatorInternals              =  7300,
            GlobalIndexCSimulatorSettings               =  7400,
            GlobalIndexCSwiftPluignSettings             =  7500,
            GlobalIndexCSimulatorMessageSettings        =  7600,
            GlobalIndexCModelSettings                   =  7700,
            GlobalIndexCAircraftCfgEntries              =  7800,
            GlobalIndexCDistributor                     =  7900,
            GlobalIndexCMatchingStatisticsEntry         =  8000,
            GlobalIndexCVPilotModelRule                 =  9000,
            GlobalIndexCVoiceRoom                       = 10000,
            GlobalIndexCSettingKeyboardHotkey           = 11000,
            GlobalIndexIDatastoreInteger                = 12000,
            GlobalIndexIDatastoreString                 = 12100,
            GlobalIndexCDbInfo                          = 12200,
            GlobalIndexCGlobalSetup                     = 13000,
            GlobalIndexCDistribution                    = 13100,
            GlobalIndexCVatsimSetup                     = 13200,
            GlobalIndexCLauncherSetup                   = 13300,
            GlobalIndexCGuiStateDbOwnModelsComponent    = 14000,
            GlobalIndexCGuiStateDbOwnModelSetComponent  = 14100,
            GlobalIndexCDockWidgetSettings              = 14200,
            GlobalIndexCNavigatorSettings               = 14300,
            GlobalIndexCSettingsReaders                 = 14400,
            GlobalIndexCViewUpdateSettings              = 14500,
            GlobalIndexCGeneralGuiSettings              = 14600,
            GlobalIndexCTextMessageSettings             = 14700,
            GlobalIndexCAtcStationsSettings             = 14800,
            GlobalIndexCInterpolatioRenderingSetup      = 16000,
            GlobalIndexCInterpolationHints              = 16100,
            GlobalIndexSwiftPilotClient                 = 17000,
            GlobalIndexSwiftCore                        = 17100,
            GlobalIndexSwiftLauncher                    = 17200,
            GlobalIndexLineNumber                       = 20000, //!< pseudo index for line numbers
        };

        //! Default constructor.
        CPropertyIndex() = default;

        //! Non nested index
        CPropertyIndex(int singleProperty);

        //! Initializer list constructor
        CPropertyIndex(std::initializer_list<int> il);

        //! Construct from a base class object.
        CPropertyIndex(const QList<int> &indexes);

        //! From string
        CPropertyIndex(const QString &indexes);

        //! Copy with first element removed
        CPropertyIndex copyFrontRemoved() const;

        //! Is nested index?
        bool isNested() const;

        //! Myself index, used with nesting
        bool isMyself() const;

        //! Empty?
        bool isEmpty() const;

        //! Index list
        QList<int> indexList() const;

        //! Shift existing indexes to right and insert given index at front
        void prepend(int newLeftIndex);

        //! Contains index?
        bool contains(int index) const;

        //! Compare with index given by enum
        template<class EnumType> bool contains(EnumType ev) const
        {
            static_assert(std::is_enum<EnumType>::value, "Argument must be an enum");
            return this->contains(static_cast<int>(ev));
        }

        //! Front to integer
        int frontToInt() const;

        //! Starts with given index?
        bool startsWith(int index) const;

        //! First element casted to given type, usually the PropertIndex enum
        template<class CastType> CastType frontCasted() const
        {
            static_assert(std::is_enum<CastType>::value || std::is_integral<CastType>::value, "CastType must be an enum or integer");
            return static_cast<CastType>(frontToInt());
        }

        //! Compare with index given by enum
        template<class EnumType> bool startsWithPropertyIndexEnum(EnumType ev) const
        {
            static_assert(std::is_enum<EnumType>::value, "Argument must be an enum");
            return this->startsWith(static_cast<int>(ev));
        }

        //! Return a predicate function which can compare two objects based on this index
        auto comparator() const
        {
            return [index = *this](const auto & a, const auto & b)
            {
                using T = std::decay_t<decltype(a)>;
                return Private::compareByProperty(a, b, index, THasCompareByPropertyIndex<T>(), THasPropertyByIndex<T>());
            };
        }

        //! \copydoc BlackMisc::Mixin::String::toQString
        QString convertToQString(bool i18n = false) const;

    protected:
        //! Parse indexes from string
        void parseFromString(const QString &indexes);

    private:
        QString m_indexString; //! I use a little trick here, the string is used with the tupel system, as it provides all operators, hash ..

        //! Convert list to string
        void setIndexStringByList(const QList<int> &list);

        BLACK_METACLASS(
            CPropertyIndex,
            BLACK_METAMEMBER(indexString)
        );
    };
} //namespace

Q_DECLARE_METATYPE(BlackMisc::CPropertyIndex)

#endif //guard

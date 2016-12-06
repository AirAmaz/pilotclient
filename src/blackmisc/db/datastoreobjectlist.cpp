/* Copyright (C) 2015
 * swift Project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "blackmisc/db/datastoreobjectlist.h"
#include "blackmisc/predicates.h"
#include "blackmisc/countrylist.h"
#include "blackmisc/aviation/airport.h"
#include "blackmisc/aviation/airportlist.h"
#include "blackmisc/aviation/liverylist.h"
#include "blackmisc/aviation/aircrafticaocodelist.h"
#include "blackmisc/aviation/airlineicaocodelist.h"
#include "blackmisc/db/dbinfolist.h"
#include "blackmisc/simulation/aircraftmodellist.h"
#include "blackmisc/simulation/distributorlist.h"

#include <QJsonObject>
#include <QJsonValue>
#include <algorithm>
#include <iterator>
#include <type_traits>

namespace BlackMisc
{
    namespace Db
    {
        template <class OBJ, class CONTAINER, typename KEYTYPE>
        IDatastoreObjectList<OBJ, CONTAINER, KEYTYPE>::IDatastoreObjectList()
        {
            constexpr bool hasIntegerKey = std::is_base_of<IDatastoreObjectWithIntegerKey, OBJ>::value && std::is_same<int, KEYTYPE>::value;
            constexpr bool hasStringKey = std::is_base_of<IDatastoreObjectWithStringKey, OBJ>::value && std::is_base_of<QString, KEYTYPE>::value;
            static_assert(hasIntegerKey || hasStringKey, "ObjectType needs to implement IDatastoreObjectWithXXXXKey and have appropriate KeyType");
        }

        template <class OBJ, class CONTAINER, typename KEYTYPE>
        OBJ IDatastoreObjectList<OBJ, CONTAINER, KEYTYPE>::findByKey(KEYTYPE key, const OBJ &notFound) const
        {
            return this->container().findFirstByOrDefault(&OBJ::getDbKey, key, notFound);
        }

        template <class OBJ, class CONTAINER, typename KEYTYPE>
        OBJ IDatastoreObjectList<OBJ, CONTAINER, KEYTYPE>::maxKeyObject() const
        {
            if (this->container().isEmpty()) { return OBJ(); }
            const OBJ max = *std::max_element(this->container().begin(), this->container().end(), [](const OBJ & obj1, const OBJ & obj2)
            {
                bool v1 = obj1.hasValidDbKey();
                bool v2 = obj2.hasValidDbKey();
                if (v1 && v2)
                {
                    return obj1.getDbKey() < obj2.getDbKey();
                }
                return v2;
            });
            return max;
        }

        template <class OBJ, class CONTAINER, typename KEYTYPE>
        void IDatastoreObjectList<OBJ, CONTAINER, KEYTYPE>::sortByKey()
        {
            this->container().sort(BlackMisc::Predicates::MemberLess(&OBJ::getDbKey));
        }

        template <class OBJ, class CONTAINER, typename KEYTYPE>
        QSet<KEYTYPE> IDatastoreObjectList<OBJ, CONTAINER, KEYTYPE>::toDbKeySet() const
        {
            QSet<KEYTYPE> keys;
            for (const OBJ &obj : ITimestampObjectList<OBJ, CONTAINER>::container())
            {
                if (!obj.hasValidDbKey()) { continue; }
                keys.insert(obj.getDbKey());
            }
            return keys;
        }

        template <class OBJ, class CONTAINER, typename KEYTYPE>
        KEYTYPE IDatastoreObjectList<OBJ, CONTAINER, KEYTYPE>::getMaxKey(bool *ok) const
        {
            QSet<KEYTYPE> keys(this->toDbKeySet());
            if (keys.isEmpty())
            {
                if (ok) { *ok = false; }
                return KEYTYPE();
            }
            KEYTYPE max = *std::max_element(keys.begin(), keys.end());
            if (ok) { *ok = true; }
            return max;
        }

        template <class OBJ, class CONTAINER, typename KEYTYPE>
        int IDatastoreObjectList<OBJ, CONTAINER, KEYTYPE>::removeObjectsWithKeys(const QSet<KEYTYPE> &keys)
        {
            if (keys.isEmpty()) { return 0; }
            if (this->container().isEmpty()) { return 0; }
            CONTAINER newValues;
            for (const OBJ &obj : ITimestampObjectList<OBJ, CONTAINER>::container())
            {
                if (keys.contains(obj.getDbKey())) { continue; }
                newValues.push_back(obj);
            }
            int delta = this->container().size() - newValues.size();
            this->container() = newValues;
            return delta;
        }

        template <class OBJ, class CONTAINER, typename KEYTYPE>
        int IDatastoreObjectList<OBJ, CONTAINER, KEYTYPE>::removeObjectsWithoutDbKey()
        {
            if (this->container().isEmpty()) { return 0; }
            CONTAINER newValues;
            for (const OBJ &obj : ITimestampObjectList<OBJ, CONTAINER>::container())
            {
                if (!obj.hasValidDbKey()) { continue; }
                newValues.push_back(obj);
            }
            int delta = this->container().size() - newValues.size();
            this->container() = newValues;
            return delta;
        }

        template <class OBJ, class CONTAINER, typename KEYTYPE>
        int IDatastoreObjectList<OBJ, CONTAINER, KEYTYPE>::replaceOrAddObjectsByKey(const CONTAINER &container)
        {
            if (container.isEmpty()) { return 0; }
            if (this->container().isEmpty())
            {
                this->container() = container;
                return this->container().size();
            }
            CONTAINER newValues(this->container());
            const QSet<KEYTYPE> keys(container.toDbKeySet());
            newValues.removeObjectsWithKeys(keys);
            int removeSize = newValues.size(); // size after removing data
            newValues.push_back(container);
            this->container() = newValues;
            return newValues.size() - removeSize;
        }

        template <class OBJ, class CONTAINER, typename KEYTYPE>
        CONTAINER IDatastoreObjectList<OBJ, CONTAINER, KEYTYPE>::fromDatabaseJson(const QJsonArray &array)
        {
            CONTAINER container;
            for (const QJsonValue &value : array)
            {
                container.push_back(OBJ::fromDatabaseJson(value.toObject()));
            }
            return container;
        }

        template <class OBJ, class CONTAINER, typename KEYTYPE>
        CONTAINER IDatastoreObjectList<OBJ, CONTAINER, KEYTYPE>::fromDatabaseJson(const QJsonArray &array, const QString &prefix)
        {
            CONTAINER container;
            for (const QJsonValue &value : array)
            {
                container.push_back(OBJ::fromDatabaseJson(value.toObject(), prefix));
            }
            return container;
        }

        // see here for the reason of thess forward instantiations
        // http://www.parashift.com/c++-faq/separate-template-class-defn-from-decl.html
        template class IDatastoreObjectList<BlackMisc::Aviation::CLivery, BlackMisc::Aviation::CLiveryList, int>;
        template class IDatastoreObjectList<BlackMisc::Aviation::CAircraftIcaoCode, BlackMisc::Aviation::CAircraftIcaoCodeList, int>;
        template class IDatastoreObjectList<BlackMisc::Aviation::CAirlineIcaoCode, BlackMisc::Aviation::CAirlineIcaoCodeList, int>;
        template class IDatastoreObjectList<BlackMisc::Db::CDbInfo, BlackMisc::Db::CDbInfoList, int>;
        template class IDatastoreObjectList<BlackMisc::Simulation::CAircraftModel, BlackMisc::Simulation::CAircraftModelList, int>;
        template class IDatastoreObjectList<BlackMisc::Simulation::CDistributor, BlackMisc::Simulation::CDistributorList, QString>;
        template class IDatastoreObjectList<BlackMisc::CCountry, BlackMisc::CCountryList, QString>;
        template class IDatastoreObjectList<BlackMisc::Aviation::CAirport, BlackMisc::Aviation::CAirportList, int>;

    } // ns
} // ns

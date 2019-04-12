/* Copyright (C) 2013
 * swift Project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution. No part of swift project, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 */

#include "registermetadata.h"
#include "blackmisc/simulation/registermetadatasimulation.h"
#include "blackmisc/weather/registermetadataweather.h"
#include "blackmisc/network/registermetadatanetwork.h"
#include "blackmisc/db/registermetadatadb.h"
#include "blackmisc/audio/registermetadataaudio.h"
#include "blackmisc/aviation/registermetadataaviation.h"
#include "blackmisc/input/registermetadatainput.h"
#include "blackmisc/geo/registermetadatageo.h"
#include "blackmisc/pq/registermetadatapq.h"

#include "blackmisc/applicationinfolist.h"
#include "blackmisc/countrylist.h"
#include "blackmisc/crashsettings.h"
#include "blackmisc/dbus.h"
#include "blackmisc/directories.h"
#include "blackmisc/iconlist.h"
#include "blackmisc/identifierlist.h"
#include "blackmisc/logcategorylist.h"
#include "blackmisc/logpattern.h"
#include "blackmisc/namevariantpair.h"
#include "blackmisc/namevariantpairlist.h"
#include "blackmisc/pixmap.h"
#include "blackmisc/platformset.h"
#include "blackmisc/processinfo.h"
#include "blackmisc/propertyindexlist.h"
#include "blackmisc/propertyindexvariantmap.h"
#include "blackmisc/rgbcolor.h"
#include "blackmisc/sequence.h"
#include "blackmisc/statusmessagelist.h"
#include "blackmisc/valuecache.h"
#include "blackmisc/valueobject.h"
#include "blackmisc/variantlist.h"
#include "blackmisc/variantmap.h"

#include <QDBusMetaType>
#include <QMetaType>
#include <QtGlobal>

void initBlackMiscResourcesImpl()
{
    // cannot be declared within namespace, see docu
    Q_INIT_RESOURCE(blackmisc);
}

namespace BlackMisc
{
    void registerMetadata()
    {
        initBlackMiscResourcesImpl();

        CApplicationInfo::registerMetadata();
        CApplicationInfoList::registerMetadata();
        CCountry::registerMetadata();
        CCountryList::registerMetadata();
        CDirectories::registerMetadata();
        CIcon::registerMetadata();
        CIconList::registerMetadata();
        CIdentifier::registerMetadata();
        CIdentifierList::registerMetadata();
        CLogCategory::registerMetadata();
        CLogCategoryList::registerMetadata();
        CLogPattern::registerMetadata();
        CNameVariantPair::registerMetadata();
        CNameVariantPairList::registerMetadata();
        CPixmap::registerMetadata();
        CPlatform::registerMetadata();
        CPlatformSet::registerMetadata();
        CProcessInfo::registerMetadata();
        CPropertyIndex::registerMetadata();
        CPropertyIndex::registerMetadata();
        CPropertyIndexList::registerMetadata();
        CPropertyIndexVariantMap::registerMetadata();
        CRgbColor::registerMetadata();
        CStatusMessage::registerMetadata();
        CStatusMessageList::registerMetadata();
        CStrongStringView::registerMetadata();
        CValueCachePacket::registerMetadata();
        CVariant::registerMetadata();
        CVariantList::registerMetadata();
        CVariantMap::registerMetadata();

        // sub namespaces
        Audio::registerMetadata();
        Aviation::registerMetadata();
        Db::registerMetadata();
        Geo::registerMetadata();
        Input::registerMetadata();
        Network::registerMetadata();
        PhysicalQuantities::registerMetadata();
        Simulation::registerMetadata();
        Settings::CCrashSettings::registerMetadata();
        Weather::registerMetadata();

        // needed by XSwiftBus proxy class
        qDBusRegisterMetaType<CSequence<double>>();
        qRegisterMetaTypeStreamOperators<CSequence<double>>();
    }
} // ns

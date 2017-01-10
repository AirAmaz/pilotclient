/* Copyright (C) 2013
 * swift Project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "blackcore/webreaderflags.h"
#include "blackgui/guiapplication.h"
#include "blackmisc/icons.h"
#include "swiftdata.h"

#include <stdlib.h>
#include <QApplication>
#include <QtGlobal>

using namespace BlackMisc;
using namespace BlackCore;
using namespace BlackCore::Db;
using namespace BlackGui;

int main(int argc, char *argv[])
{
    CGuiApplication::highDpiScreenSupport();
    QApplication qa(argc, argv);
    Q_UNUSED(qa);
    CGuiApplication a("swift mapping tool", CApplicationInfo::MappingTool, CIcons::swiftDatabase48());
    a.setSignalStartupAutomatically(false); // application will signal startup on its own
    a.splashScreen(CIcons::swiftDatabase256());
    a.parse();
    a.useWebDataServices(BlackCore::CWebReaderFlags::AllSwiftDbReaders, CDatabaseReaderConfigList::forMappingTool());
    if (!a.start()) { return EXIT_FAILURE; }
    CSwiftData w;
    w.show();
    int r = a.exec();
    return r;
}

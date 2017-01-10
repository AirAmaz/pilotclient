/* Copyright (C) 2016
 * swift Project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "dbus.h"

#ifdef Q_OS_WIN
#include <QDBusConnection>
#include <qt_windows.h>

// https://blogs.msdn.microsoft.com/oldnewthing/20131105-00/?p=2733
// See https://bugreports.qt.io/browse/QTBUG-53031 for more details
// why this is necessary.
void preventQtDBusDllUnload()
{
    // Only Qt 5.8.0 is affected.
    if (qVersion() != QByteArray("5.8.0")) { return; }

    static HMODULE dbusDll;
    GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                      GET_MODULE_HANDLE_EX_FLAG_PIN,
                      reinterpret_cast<LPCTSTR >(&QDBusConnection::staticMetaObject),
                      &dbusDll);
    Q_ASSERT(dbusDll);
}
#else
void preventQtDBusDllUnload()
{ }
#endif

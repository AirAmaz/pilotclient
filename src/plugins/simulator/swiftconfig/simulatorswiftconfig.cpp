/* Copyright (C) 2015
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "simulatorswiftconfig.h"
#include "swiftconfigwidget.h"

namespace BlackSimPlugin
{
    namespace Swift
    {
        CSimulatorSwiftConfig::CSimulatorSwiftConfig(QObject *parent) : QObject(parent)
        {
            // void
        }

        BlackGui::CPluginConfigWindow *CSimulatorSwiftConfig::createConfigWindow(QWidget *parent)
        {
            return new CSwiftConfigWidget(parent);
        }
    }
}

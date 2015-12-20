/* Copyright (C) 2015
 * swift Project Community / Contributors
 *
 * This file is part of swift Project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "airlineicaoview.h"
#include <QHeaderView>

using namespace BlackMisc;
using namespace BlackGui::Models;

namespace BlackGui
{
    namespace Views
    {
        CAirlineIcaoCodeView::CAirlineIcaoCodeView(QWidget *parent) : CViewBase(parent)
        {
            this->m_menus |= MenuBackend;
            this->standardInit(new CAirlineIcaoCodeListModel(this));
        }
    }
} // namespace

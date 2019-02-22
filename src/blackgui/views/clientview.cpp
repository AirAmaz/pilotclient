/* Copyright (C) 2013
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution. No part of swift project, including this file, may be copied, modified, propagated,
 * or distributed except according to the terms contained in the LICENSE file.
 */

#include "blackgui/models/clientlistmodel.h"
#include "blackgui/views/clientview.h"

using namespace BlackMisc;
using namespace BlackGui::Models;

namespace BlackGui
{
    namespace Views
    {
        CClientView::CClientView(QWidget *parent) : CViewBase(parent)
        {
            this->standardInit(new CClientListModel(this));
        }
    } // namespace
} // namespace

/* Copyright (C) 2013
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "blackgui/models/statusmessagelistmodel.h"
#include "blackgui/views/statusmessageview.h"
#include "blackgui/filters/statusmessagefilterdialog.h"
#include <QFlags>

using namespace BlackMisc;
using namespace BlackGui::Models;
using namespace BlackGui::Filters;

namespace BlackGui
{
    namespace Views
    {
        CStatusMessageView::CStatusMessageView(QWidget *parent) : CViewBase(parent)
        {
            m_menus |= MenuClear;
            this->menuRemoveItems(MenuRefresh | MenuBackend | MenuToggleSelectionMode);
            m_acceptRowSelected = true;
            this->standardInit(new CStatusMessageListModel(this));
        }

        void CStatusMessageView::setMode(CStatusMessageListModel::Mode mode)
        {
            this->derivedModel()->setMode(mode);
        }

        void CStatusMessageView::addFilterDialog()
        {
            if (this->getFilterDialog()) { return; } // already existing
            this->setFilterDialog(new CStatusMessageFilterDialog(this));
        }

        void CStatusMessageView::keepLatest(int desiredSize)
        {
            if (desiredSize >= this->rowCount()) { return; }
            if (desiredSize < 1)
            {
                this->clear();
                return;
            }

            CStatusMessageList msgs = this->container();
            msgs.keepLatest(desiredSize);
            this->updateContainerMaybeAsync(msgs);
        }

        CStatusMessageFilterDialog *CStatusMessageView::getFilterDialog() const
        {
            return qobject_cast<CStatusMessageFilterDialog *>(this->getFilterWidget());
        }
    } // namespace
} // namespace

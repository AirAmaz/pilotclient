/* Copyright (C) 2013
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "statusmessageview.h"
#include <QHeaderView>
#include <QMenu>

using namespace BlackMisc;
using namespace BlackGui::Models;

namespace BlackGui
{
    namespace Views
    {
        /*
         * Constructor
         */
        CStatusMessageView::CStatusMessageView(QWidget *parent) : CViewBase(parent), m_contextMenu(nullptr)
        {
            this->standardInit(new CStatusMessageListModel(this));
            this->setContextMenuPolicy(Qt::CustomContextMenu);
            this->m_contextMenu = new QMenu(this);
            this->m_contextMenu->addAction("Clear");
            connect(this, &QTableView::customContextMenuRequested, this, &CStatusMessageView::contextMenu);
        }

        /*
         * Message list context menu
         */
        void CStatusMessageView::contextMenu(const QPoint &position)
        {
            // position for most widgets
            QPoint globalPosition = this->mapToGlobal(position);
            QAction *selectedItem = this->m_contextMenu->exec(globalPosition);
            if (selectedItem)
            {
                // http://forum.technical-assistance.co.uk/sndvol32exe-command-line-parameters-vt1348.html
                const QList<QAction *> actions = this->m_contextMenu->actions();
                if (selectedItem == actions.at(0))
                {
                    this->clear();
                    this->resizeColumnsToContents();
                }
            }
        }
    } // namespace
} // namespace

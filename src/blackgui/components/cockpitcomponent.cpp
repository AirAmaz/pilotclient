/* Copyright (C) 2013
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "blackgui/components/cockpitcomcomponent.h"
#include "blackgui/components/cockpitcomponent.h"
#include "blackgui/components/cockpitinfoareacomponent.h"
#include "blackgui/dockwidgetinfoarea.h"
#include "blackgui/showhidebar.h"
#include "ui_cockpitcomponent.h"

#include <QDockWidget>
#include <QtGlobal>

namespace BlackGui
{
    namespace Components
    {
        CCockpitComponent::CCockpitComponent(QWidget *parent) :
            QWidget(parent),
            CEnableForDockWidgetInfoArea(),
            ui(new Ui::CCockpitComponent)
        {
            ui->setupUi(this);
            this->m_minHeightInfoArea = ui->comp_CockpitInfoArea->minimumHeight();
            connect(ui->wip_CockpitComPanelShowHideBar, &BlackGui::CShowHideBar::toggleShowHide, this, &CCockpitComponent::ps_onToggleShowHideDetails);
        }

        CCockpitComponent::~CCockpitComponent()
        { }

        bool CCockpitComponent::setParentDockWidgetInfoArea(CDockWidgetInfoArea *parentDockableWidget)
        {
            Q_ASSERT(parentDockableWidget);
            bool ok = CEnableForDockWidgetInfoArea::setParentDockWidgetInfoArea(parentDockableWidget);
            if (ok && parentDockableWidget)
            {
                ok = connect(parentDockableWidget, &QDockWidget::topLevelChanged, this, &CCockpitComponent::ps_onToggleFloating);
            }
            return ok;
        }

        bool CCockpitComponent::isInfoAreaShown() const
        {
            return ui->wip_CockpitComPanelShowHideBar->isShown();
        }

        void CCockpitComponent::setSelectedTransponderModeStateIdent()
        {
            ui->comp_CockpitComComponent->setSelectedTransponderModeStateIdent();
        }

        void CCockpitComponent::ps_onToggleShowHideDetails(bool show)
        {
            // use the toggle method to set the sizes
            this->toggleShowHideDetails(show, true);
        }

        void CCockpitComponent::toggleShowHideDetails(bool show, bool considerCurrentSize)
        {
            Q_ASSERT(this->isParentDockWidgetFloating()); // show hide should not be visible if docked
            Q_ASSERT(this->window());
            if (!this->isParentDockWidgetFloating()) { return; }

            // manually setting size, all other approaches failed
            static const QSize defaultSizeShown(300, 400);
            static const QSize defaultSizeHidden(300, 150);

            // keep old size
            QSize manuallySetSize = this->window()->size();

            // hide area
            ui->comp_CockpitInfoArea->setVisible(show);

            // adjust size
            if (show)
            {
                ui->comp_CockpitInfoArea->setMinimumHeight(m_minHeightInfoArea);
                if (this->m_sizeFloatingShown.isValid())
                {
                    this->window()->resize(m_sizeFloatingShown);
                    if (considerCurrentSize) { this->m_sizeFloatingHidden = manuallySetSize;  } // for next time
                }
                else
                {
                    // manually setting size, all other approaches failed
                    this->window()->resize(defaultSizeShown);
                    this->m_sizeFloatingShown = this->window()->size();
                }
            }
            else
            {
                ui->comp_CockpitInfoArea->setMinimumHeight(0);
                this->window()->setMinimumSize(defaultSizeHidden);
                if (this->m_sizeFloatingHidden.isValid())
                {
                    this->window()->resize(m_sizeFloatingHidden);
                    if (considerCurrentSize) { this->m_sizeFloatingShown = manuallySetSize; }
                }
                else
                {
                    // manually setting size, all other approaches failed
                    this->window()->resize(defaultSizeHidden);
                    this->m_sizeFloatingHidden = this->window()->size();
                }
            }
        }

        void CCockpitComponent::ps_onToggleFloating(bool floating)
        {
            ui->wip_CockpitComPanelShowHideBar->setVisible(floating);
            if (floating)
            {
                // use the toggle method to set the sizes
                this->toggleShowHideDetails(this->isInfoAreaShown(), false);
            }
            else
            {
                const QSize sizeMinimum(200, 100); // set when docked, must fit into parent info area
                ui->comp_CockpitInfoArea->setVisible(true);
                this->window()->setMinimumSize(sizeMinimum);
            }
        }
    } // namespace
} // namespace

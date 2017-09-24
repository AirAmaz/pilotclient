/* Copyright (C) 2013
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file

#ifndef BLACKGUI_COMPONENTS_COCKPITCOMPONENT_H
#define BLACKGUI_COMPONENTS_COCKPITCOMPONENT_H

#include "blackgui/blackguiexport.h"
#include "blackgui/components/enablefordockwidgetinfoarea.h"
#include <QObject>
#include <QScopedPointer>
#include <QSize>
#include <QWidget>

namespace Ui { class CCockpitComponent; }
namespace BlackGui
{
    class CDockWidgetInfoArea;

    namespace Components
    {
        //! Cockpit component: COM unit, show / hide bar, voice rooms
        class BLACKGUI_EXPORT CCockpitComponent :
            public QWidget,
            public CEnableForDockWidgetInfoArea
        {
            Q_OBJECT

        public:
            //! Constructor
            explicit CCockpitComponent(QWidget *parent = nullptr);

            //! Destructor
            virtual ~CCockpitComponent();

            //! \copydoc CEnableForDockWidgetInfoArea::setParentDockWidgetInfoArea
            virtual bool setParentDockWidgetInfoArea(BlackGui::CDockWidgetInfoArea *parentDockableWidget) override;

            //! Is the info area shown?
            bool isInfoAreaShown() const;

        public slots:
            //! \copydoc BlackGui::Components::CTransponderModeSelector::setSelectedTransponderModeStateIdent
            void setSelectedTransponderModeStateIdent();

            //! Show the audio ui
            void showAudio();

        private slots:
            //! Show or hide cockpit details
            void ps_onToggleShowHideDetails(bool show);

            //! Toggle floating
            void ps_onToggleFloating(bool floating);

        private:
            // toggle area on show/hide details
            void toggleShowHideDetails(bool show, bool considerCurrentSize);

            QScopedPointer<Ui::CCockpitComponent> ui;
            QSize m_sizeFloatingShown;    //! size when info area is shown
            QSize m_sizeFloatingHidden;   //! size when info area is hidden
            int m_minHeightInfoArea = -1; //! minimum height of the info area
        };
    } // namespace
} // namespace

#endif // guard

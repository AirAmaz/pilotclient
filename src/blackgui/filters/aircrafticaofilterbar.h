/* Copyright (C) 2015
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file

#ifndef BLACKGUI_FILTERS_AIRCRAFTICAOFILTERBAR_H
#define BLACKGUI_FILTERS_AIRCRAFTICAOFILTERBAR_H

#include "blackgui/blackguiexport.h"
#include "blackgui/filters/filterwidget.h"
#include "blackgui/models/modelfilter.h"

#include <QObject>
#include <QScopedPointer>
#include <memory>

class QWidget;

namespace BlackMisc
{
    namespace Aviation
    {
        class CAircraftIcaoCode;
        class CAircraftIcaoCodeList;
    }
}
namespace Ui { class CAircraftIcaoFilterBar; }

namespace BlackGui
{
    namespace Filters
    {
        /*!
         * Aircraft ICAO filter bar
         */
        class BLACKGUI_EXPORT CAircraftIcaoFilterBar :
            public CFilterWidget,
            public BlackGui::Models::IModelFilterProvider<BlackMisc::Aviation::CAircraftIcaoCodeList>
        {
            Q_OBJECT

        public:
            //! Constructor
            explicit CAircraftIcaoFilterBar(QWidget *parent = nullptr);

            //! Destructor
            ~CAircraftIcaoFilterBar();

            //! \copydoc Models::IModelFilterProvider::createModelFilter
            std::unique_ptr<BlackGui::Models::IModelFilter<BlackMisc::Aviation::CAircraftIcaoCodeList> > createModelFilter() const override;

            //! Filter by ICAO object as default values
            void filter(const BlackMisc::Aviation::CAircraftIcaoCode &icao);

            //! Hide the description
            void hideDescriptionField(bool hide);

        public slots:
            //! \copydoc CFilterWidget::onRowCountChanged
            virtual void onRowCountChanged(int count, bool withFilter) override;

        protected:
            //! \copydoc CFilterWidget::clearForm
            void clearForm() override;

        private:
            QScopedPointer<Ui::CAircraftIcaoFilterBar> ui;
        };

    } // ns
} // ns

#endif // guard

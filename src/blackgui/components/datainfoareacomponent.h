/* Copyright (C) 2014
 * swift project Community / Contributors
 *
 * This file is part of swift Project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file

#ifndef BLACKGUI_DATAINFOAREACOMPONENT_H
#define BLACKGUI_DATAINFOAREACOMPONENT_H

#include "blackgui/blackguiexport.h"
#include "blackgui/infoarea.h"
#include "blackmisc/network/webdataservicesprovider.h"
#include <QMainWindow>
#include <QScopedPointer>

namespace Ui { class CDataInfoAreaComponent; }

namespace BlackGui
{
    namespace Components
    {
        class CDataMappingComponent;
        class CDbAircraftIcaoComponent;
        class CDbAirlineIcaoComponent;
        class CDbModelComponent;
        class CDbDistributorComponent;
        class CDbLiveryComponent;
        class CDbCountryComponent;

        /**
         * Info area containing the DB data (models, liveries ...)
         */
        class BLACKGUI_EXPORT CDataInfoAreaComponent :
            public BlackGui::CInfoArea,
            public BlackMisc::Network::CWebDataServicesAware
        {
            Q_OBJECT

        public:
            //! Info areas
            enum InfoArea
            {
                // index must match tab index!
                InfoAreaModels        = 0,
                InfoAreaLiveries      = 1,
                InfoAreaDistributors  = 2,
                InfoAreaAircraftIcao  = 3,
                InfoAreaAirlineIcao   = 4,
                InfoAreaCountries     = 5,
                InfoAreaNone          = -1
            };

            //! Constructor
            explicit CDataInfoAreaComponent(QWidget *parent = nullptr);

            //! Destructor
            ~CDataInfoAreaComponent();

            //! DB model component
            CDbModelComponent *getModelComponent() const;

            //! DB livery component
            CDbLiveryComponent *getLiveryComponent() const;

            //! DB distributor component
            CDbDistributorComponent *getDistributorComponent() const;

            //! DB aircraft ICAO component
            CDbAircraftIcaoComponent *getAircraftComponent() const;

            //! DB airline ICAO component
            CDbAirlineIcaoComponent *getAirlineComponent() const;

            //! DB country component
            CDbCountryComponent *getCountryComponent() const;

            //! Set data reader
            virtual void setProvider(BlackMisc::Network::IWebDataServicesProvider *webDataReader) override;

        public slots:
            //! Write to resource dir
            bool writeDbDataToResourceDir() const;

            //! Load from resource dir
            bool readDbDataFromResourceDir();

            //! Load new data (based on timestamp, incremental)
            void requestUpdatedData(BlackMisc::Network::CEntityFlags::Entity entity);

        protected:
            //! \copydoc CInfoArea::getPreferredSizeWhenFloating
            virtual QSize getPreferredSizeWhenFloating(int areaIndex) const override;

            //! \copydoc CInfoArea::indexToPixmap
            virtual const QPixmap &indexToPixmap(int areaIndex) const override;

        private:
            QScopedPointer <Ui::CDataInfoAreaComponent> ui;
        };

    } // ns
} // ns

#endif // guard

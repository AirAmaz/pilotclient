/* Copyright (C) 2016
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file

#ifndef BLACKMISC_WEATHER_GRIDPOINT_H
#define BLACKMISC_WEATHER_GRIDPOINT_H

#include "blackmisc/blackmiscexport.h"
#include "blackmisc/geo/coordinategeodetic.h"
#include "blackmisc/metaclass.h"
#include "blackmisc/pq/pressure.h"
#include "blackmisc/pq/units.h"
#include "blackmisc/propertyindex.h"
#include "blackmisc/valueobject.h"
#include "blackmisc/variant.h"
#include "blackmisc/weather/cloudlayerlist.h"
#include "blackmisc/weather/temperaturelayerlist.h"
#include "blackmisc/weather/visibilitylayerlist.h"
#include "blackmisc/weather/windlayerlist.h"

#include <QMetaType>
#include <QString>

namespace BlackMisc
{
    namespace Weather
    {
        /*!
         * Value object for a cloud layer
         */
        class BLACKMISC_EXPORT CGridPoint : public CValueObject<CGridPoint>
        {
        public:
            //! Properties by index
            enum ColumnIndex
            {
                IndexIdentifier = BlackMisc::CPropertyIndex::GlobalIndexCGridPoint,
                IndexPosition,
                IndexCloudLayers,
                IndexTemperatureLayers,
                IndexWindLayers,
                IndexSurfacePressure
            };

            //! Default constructor.
            CGridPoint() = default;

            //! Constructor
            CGridPoint(const QString &identifier,
                       const Geo::CCoordinateGeodetic &position);

            //! Constructor
            CGridPoint(const QString &identifier,
                       const Geo::CCoordinateGeodetic &position,
                       const CCloudLayerList &cloudLayers,
                       const CTemperatureLayerList &temperatureLayers,
                       const CVisibilityLayerList &visibilityLayers,
                       const CWindLayerList &windLayers,
                       const PhysicalQuantities::CPressure &surfacePressure);

            //! Set identifier
            void setIdentifier(const QString &identifier) { m_identifier = identifier; }

            //! Get identifier
            QString getIdentifier() const { return m_identifier; }

            //! Set position
            void setPosition(const BlackMisc::Geo::CCoordinateGeodetic &position) { m_position = position; }

            //! Get position
            const BlackMisc::Geo::CCoordinateGeodetic getPosition() const { return m_position; }

            //! Set cloud layers
            void setCloudLayers(const CCloudLayerList &cloudLayers) { m_cloudLayers = cloudLayers; }

            //! Get cloud layers
            CCloudLayerList getCloudLayers() const { return m_cloudLayers; }

            //! Set temperature layers
            void setTemperatureLayers(const CTemperatureLayerList &temperatureLayers) { m_temperatureLayers = temperatureLayers; }

            //! Get temperature layers
            CTemperatureLayerList getTemperatureLayers() const { return m_temperatureLayers; }

            //! Set visibility layers
            void setVisibilityLayers(const CVisibilityLayerList &visibilityLayers) { m_visibilityLayers = visibilityLayers; }

            //! Get visibility layers
            CVisibilityLayerList getVisibilityLayers() const { return m_visibilityLayers; }

            //! Set wind layers
            void setWindLayers(const CWindLayerList &windLayers) { m_windLayers = windLayers; }

            //! Get wind layers
            CWindLayerList getWindLayers() const { return m_windLayers; }

            //! Copies all weather data from other without modifying identifier and position.
            void copyWeatherDataFrom(const CGridPoint &other);

            //! Set surface pressure
            void setSurfacePressure(const PhysicalQuantities::CPressure &pressure) { m_surfacePressure = pressure; }

            //! Get surface pressure
            PhysicalQuantities::CPressure getSurfacePressure() const { return m_surfacePressure; }

            //! \copydoc BlackMisc::Mixin::Index::propertyByIndex
            CVariant propertyByIndex(const BlackMisc::CPropertyIndex &index) const;

            //! \copydoc BlackMisc::Mixin::Index::setPropertyByIndex
            void setPropertyByIndex(const BlackMisc::CPropertyIndex &index, const CVariant &variant);

            //! \copydoc BlackMisc::Mixin::String::toQString
            QString convertToQString(bool i18n = false) const;

        private:
            // Identifier is intentionally string based. MSFS uses ICAO but others don't.
            QString m_identifier;
            BlackMisc::Geo::CCoordinateGeodetic m_position;
            CCloudLayerList m_cloudLayers;
            CTemperatureLayerList m_temperatureLayers;
            CVisibilityLayerList m_visibilityLayers;
            CWindLayerList m_windLayers;
            PhysicalQuantities::CPressure m_surfacePressure = { 1013.25, PhysicalQuantities::CPressureUnit::hPa() };

            BLACK_METACLASS(
                CGridPoint,
                BLACK_METAMEMBER(identifier),
                BLACK_METAMEMBER(position),
                BLACK_METAMEMBER(cloudLayers),
                BLACK_METAMEMBER(temperatureLayers),
                BLACK_METAMEMBER(visibilityLayers),
                BLACK_METAMEMBER(windLayers),
                BLACK_METAMEMBER(surfacePressure)
            );
        };
    } // namespace
} // namespace

Q_DECLARE_METATYPE(BlackMisc::Weather::CGridPoint)

#endif // guard

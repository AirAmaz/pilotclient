/* Copyright (C) 2016
 * swift project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file

#ifndef BLACKWXPLUGIN_GFS_H
#define BLACKWXPLUGIN_GFS_H

#include "g2clib/grib2.h"
#include "blackmisc/weather/gridpoint.h"
#include "blackmisc/worker.h"
#include "blackcore/weatherdata.h"
#include <QReadWriteLock>
#include <QHash>
#include <QVector>
#include <QUrl>
#include <QByteArray>
#include <QNetworkReply>
#include <QNetworkAccessManager>
#include <QPointer>
#include <array>

namespace BlackMisc { namespace PhysicalQuantities { class CTemperature; }}
namespace BlackWxPlugin
{
    namespace Gfs
    {
        /*!
         * GFS implemenation
         */
        class CWeatherDataGfs : public BlackCore::IWeatherData
        {
            Q_OBJECT

        public:
            //! Constructor
            CWeatherDataGfs(QObject *parent = nullptr);

            //! Destructor
            virtual ~CWeatherDataGfs();

            //! \copydoc BlackCore::IWeatherData::fetchWeatherData
            virtual void fetchWeatherData(const BlackMisc::Weather::CWeatherGrid &grid,
                                          const BlackMisc::PhysicalQuantities::CLength &range) override;

            //! \copydoc BlackCore::IWeatherData::getWeatherData()
            virtual BlackMisc::Weather::CWeatherGrid getWeatherData() const override;

        private slots:
            //! Asyncronous fetching finished
            //! \threadsafe
            void ps_fetchingWeatherDataFinished();

            //! Parsing
            void ps_parseGfsFile(QNetworkReply *reply);

        private:
            enum Grib2CloudLevel
            {
                LowCloud,
                MiddleCloud,
                HighCloud
            };

            enum Grib2ParameterCode
            {
                UNKNOWN,
                TMP,
                RH,
                UGRD,
                VGRD,
                PRATE,
                PRES,
                PRMSL,
                TCDC,
                CRAIN,
                CSNOW
            };

            enum Grib2FixedSurfaceTypes
            {
                GroundOrWaterSurface = 1,
                IsobaricSurface = 100,
                LowCloudBottomLevel = 212,
                LowCloudTopLevel = 213,
                LowCloudLayer = 214,
                MiddleCloudBottomLevel = 222,
                MiddleCloudTopLevel = 223,
                MiddleCloudLayer = 224,
                HighCloudBottomLevel = 232,
                HighCloudTopLevel = 233,
                HighCloudLayer = 234
            };

            struct Grib2ParameterValue
            {
                Grib2ParameterValue() = default;
                Grib2ParameterValue(Grib2ParameterCode code_, const QString &name_, const QString &unit_) : code(code_), name(name_), unit(unit_) {}
                Grib2ParameterCode code = UNKNOWN;
                QString name;
                QString unit;
            };

            struct GfsIsobaricLayer
            {
                double temperature = 0.0;
                double relativeHumidity = 0.0;
                double windU = 0.0;
                double windV = 0.0;
            };

            struct GfsCloudLayer
            {
                double bottomLevel = 0.0;
                double topLevel = 0.0;
                double totalCoverage = 0.0;
            };

            struct GfsGridPoint
            {
                double latitude = 0.0;
                double longitude = 0.0;
                int fieldPosition = 0;
                QHash<int, GfsCloudLayer> cloudLayers;
                QHash<double, GfsIsobaricLayer> isobaricLayers;
                double surfaceRain = 0;
                double surfaceSnow = 0;
                double surfacePrecipitationRate = 0;
                double surfacePressure = 0;
                double surfaceTemperature = 0;
            };

            QUrl getDownloadUrl() const;

            void parseGfsFileImpl(const QByteArray &gribData);
            void findNextGribMessage(unsigned char *buffer, g2int size, g2int iseek, g2int *lskip, g2int *lgrib);
            void createWeatherGrid(const gribfield *gfld);
            void handleProductDefinitionTemplate40(const gribfield *gfld);
            void handleProductDefinitionTemplate48(const gribfield *gfld);
            void setTemperature(const g2float *fld, double level);
            void setHumidity(const g2float *fld, double level);
            void setWindV(const g2float *fld, double level);
            void setWindU(const g2float *fld, double level);
            void setCloudCoverage(const g2float *fld, int level);
            void setCloudLevel(const g2float *fld, int surfaceType, int level);
            void setCloudPressure(const g2float *fld, double level);
            void setSurfaceRain(const g2float *fld);
            void setSurfaceSnow(const g2float *fld);
            void setPrecipitationRate(const g2float *fld);

            BlackMisc::PhysicalQuantities::CTemperature calculateDewPoint(const BlackMisc::PhysicalQuantities::CTemperature &temperature, double relativeHumidity);

            BlackMisc::Weather::CWeatherGrid m_grid;
            BlackMisc::PhysicalQuantities::CLength m_maxRange;

            mutable QReadWriteLock m_lockData;
            QByteArray m_gribData;

            QVector<GfsGridPoint> m_gfsWeatherGrid;
            BlackMisc::Weather::CWeatherGrid m_weatherGrid;

            QPointer<BlackMisc::CWorker> m_parseGribFileWorker; //!< worker will destroy itself, so weak pointer

            using Grib2ParameterKey = std::array<g2int, 2>;
            using Grib2ParameterTable = QMap<Grib2ParameterKey, Grib2ParameterValue>;
            static const Grib2ParameterTable m_grib2ParameterTable;
        };

        //! Factory for creating CWeatherDataGfs instance
        class CWeatherDataGfsFactory : public QObject, public BlackCore::IWeatherDataFactory
        {
            Q_OBJECT
            Q_PLUGIN_METADATA(IID "org.swift-project.blackcore.weatherdata" FILE "weatherdatagfs.json")
            Q_INTERFACES(BlackCore::IWeatherDataFactory)

        public:
            //! \copydoc BlackCore::IWeatherDataFactory::create()
            virtual BlackCore::IWeatherData *create(QObject *parent = nullptr) override;

        };

    } // ns
} // ns

#endif // guard

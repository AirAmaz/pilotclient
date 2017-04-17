/* Copyright (C) 2014
 * swift project community / contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#ifndef BLACKMISCTEST_TESTGEO_H
#define BLACKMISCTEST_TESTGEO_H

//! \cond PRIVATE_TESTS
//! \file
//! \ingroup testblackmisc

#include <QObject>

namespace BlackMiscTest
{
    //! Geo classes tests
    class CTestGeo : public QObject
    {
        Q_OBJECT

    public:
        //! Standard test case constructor
        explicit CTestGeo(QObject *parent = nullptr) : QObject(parent) {}

    private slots:
        //! Basic unit tests for geo classes
        void geoBasics();

        //! CCoordinateGeodetic unit tests
        void coordinateGeodetic();
    };
} // namespace

//! \endcond

#endif // guard

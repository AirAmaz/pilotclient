/* Copyright (C) 2018
 * swift project community / contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#ifndef BLACKMISCTEST_TESTAIRCRAFTPARTS_H
#define BLACKMISCTEST_TESTAIRCRAFTPARTS_H

//! \cond PRIVATE_TESTS
//! \file
//! \ingroup testblackmisc

#include "blackmisc/aviation/aircraftparts.h"
#include <QObject>

namespace BlackMiscTest
{
    //! Geo classes tests
    class CTestAircraftParts : public QObject
    {
        Q_OBJECT

    public:
        //! Standard test case constructor
        explicit CTestAircraftParts(QObject *parent = nullptr) : QObject(parent) {}

    private slots:
        //! Test ground flag
        void groundFlag();

    private:
        //! Test parts
        BlackMisc::Aviation::CAircraftParts testParts1() const;
    };
} // namespace

//! \endcond

#endif // guard

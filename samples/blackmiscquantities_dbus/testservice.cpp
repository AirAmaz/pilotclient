/*  Copyright (C) 2013 VATSIM Community / authors
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "testservice.h"

namespace BlackMiscTest
{

const QString Testservice::ServiceName = QString(BLACKMISCKTEST_SERVICENAME);
const QString Testservice::ServicePath = QString(BLACKMISCKTEST_SERVICEPATH);

/*
 * Constructor
 */
Testservice::Testservice(QObject *parent) : QObject(parent)
{
    // void
}

/*
 * Slot to receive messages
 */
void Testservice::receiveStringMessage(const QString &message)
{
    qDebug() << "Pid:" << TestserviceTool::getPid() << "Received message:" << message;
}

/*
 * Receive variant
 */
void Testservice::receiveVariant(const QDBusVariant &variant)
{
    QVariant qv = variant.variant();
    qDebug() << "Pid:" << TestserviceTool::getPid() << "Received variant:" << qv;
}

/*
 * Receive speed
 */
void Testservice::receiveSpeed(const BlackMisc::PhysicalQuantities::CSpeed &speed)
{
    qDebug() << "Pid:" << TestserviceTool::getPid() << "Received speed:" << speed;
}

/*
 * Receive COM unit
 */
void Testservice::receiveComUnit(const BlackMisc::Aviation::CComSystem &comUnit)
{
    qDebug() << "Pid:" << TestserviceTool::getPid() << "Received COM:" << comUnit;
}


} // namespace

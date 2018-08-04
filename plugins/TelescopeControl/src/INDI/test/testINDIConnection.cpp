/*
 * Copyright (C) 2017 Alessandro Siniscalchi <asiniscalchi@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */

#include "testINDIConnection.hpp"

#include <limits>
#include <cmath>

#include "INDIConnection.hpp"
#include "indibase/basedevice.h"

void TestINDIConnection::deafultCoordinates()
{
    INDIConnection::Coordinates position;
    QVERIFY(std::abs(position.RA - 0.0) < std::numeric_limits<double>::epsilon());
    QVERIFY(std::abs(position.DEC - 0.0) < std::numeric_limits<double>::epsilon());
}

void TestINDIConnection::defaultPosition()
{
    INDIConnection instance;
    QVERIFY(instance.position() == INDIConnection::Coordinates());
}

void TestINDIConnection::initialConnectionStatus()
{
    INDIConnection instance;
    QVERIFY(instance.isDeviceConnected() == false);
}

void TestINDIConnection::setPositionNotConnected()
{
    INDIConnection::Coordinates position;
    position.DEC = 0.1;
    position.RA = 0.2;

    INDIConnection instance;
    instance.setPosition(position);
    QVERIFY(instance.position() == INDIConnection::Coordinates());
}

void TestINDIConnection::listDevices()
{
    INDIConnection instance;
    QVERIFY(instance.devices().empty());

    INDI::BaseDevice device;
    device.setDeviceName("dummy");

    instance.newDevice(&device);
    QVERIFY(instance.devices().size() == 1);
    instance.removeDevice(&device);
    QVERIFY(instance.devices().empty());
}

QTEST_MAIN(TestINDIConnection)


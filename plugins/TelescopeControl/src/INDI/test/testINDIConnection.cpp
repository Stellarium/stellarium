#include "testINDIConnection.hpp"

#include <limits>
#include <cmath>

#include "INDIConnection.hpp"
#include "indibase/basedevice.h"

void testINDIConnection::deafultCoordinates()
{
    INDIConnection::Coordinates position;
    QVERIFY(std::abs(position.RA - 0.0) < std::numeric_limits<double>::epsilon());
    QVERIFY(std::abs(position.DEC - 0.0) < std::numeric_limits<double>::epsilon());
}

void testINDIConnection::defaultPosition()
{
    INDIConnection instance;
    QVERIFY(instance.position() == INDIConnection::Coordinates());
}

void testINDIConnection::initialConnectionStatus()
{
    INDIConnection instance;
    QVERIFY(instance.isDeviceConnected() == false);
}

void testINDIConnection::setPositionNotConnected()
{
    INDIConnection::Coordinates position;
    position.DEC = 0.1;
    position.RA = 0.2;

    INDIConnection instance;
    instance.setPosition(position);
    QVERIFY(instance.position() == INDIConnection::Coordinates());
}

void testINDIConnection::listDevices()
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

QTEST_MAIN(testINDIConnection)


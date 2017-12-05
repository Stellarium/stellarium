#include "testINDIConnection.hpp"

#include <limits>
#include <cmath>

#include "INDIConnection.hpp"

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
    QVERIFY(instance.isConnected() == false);
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

QTEST_MAIN(testINDIConnection)


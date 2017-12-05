#include "testINDIConnection.hpp"

#include <limits>

#include "INDIConnection.hpp"

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


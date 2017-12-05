#include "testINDIConnection.hpp"

#include <limits>

#include "INDIConnection.hpp"

void testINDIConnection::defaultPosition()
{
    INDIConnection instance;
    QVERIFY(instance.position() == INDIConnection::Coordinates());
}

QTEST_MAIN(testINDIConnection)


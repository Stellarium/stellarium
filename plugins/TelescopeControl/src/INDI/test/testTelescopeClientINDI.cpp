#include "testTelescopeClientINDI.hpp"

#include "TelescopeClientINDI.hpp"

void testTelescopeClientINDI::isConnectedInitialValue()
{
	TelescopeClientINDI client;
	QVERIFY(client.isConnected() == false);
}

QTEST_MAIN(testTelescopeClientINDI)


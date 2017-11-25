#include <iostream>

#include <libindi/basedevice.h>

#include "servers/INDIConnection.hpp"

int main (int argc, char *argv[])
{
    INDIConnection connection;

    connection.setServer("localhost", 7624);
    //connection->watchDevice(MYCCD);
    if (!connection.connectServer())
        return -1;

    std::cout << "Press any key to terminate the client.\n";
    std::string term;
    std::cin >> term;

    return 0;
}

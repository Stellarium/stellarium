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

    while(1)
    {
        INDIConnection::Coordinates coord = connection.position();

        std::cout << std::endl;
        std::string stringDEC;
        std::string stringRA;
        std::cout << "RA : ";
        std::cin >> stringRA;
        std::cout << "DEC : ";
        std::cin >> stringDEC;

        coord.RA = std::atof(stringRA.c_str());
        coord.DEC = std::atof(stringDEC.c_str());

        connection.setPosition(coord);
    }

    return 0;
}

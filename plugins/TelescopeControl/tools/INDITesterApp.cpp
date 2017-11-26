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

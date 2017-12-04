/*
 DSUSB Driver for GPhoto

 Copyright (C) 2017 Jasem Mutlaq (mutlaqja@ikarustech.com)

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

*/

#pragma once

#include <indiusbdevice.h>

#include <stdint.h>

class DSUSBDriver : public INDI::USBDevice
{
  public:
    DSUSBDriver(const char *device);
    ~DSUSBDriver() {}

    bool isConnected() { return connected; }
    bool openShutter();
    bool closeShutter();

  private:
    bool readState();

    uint8_t infoByte = 0;
    char device[MAXINDIDEVICE];
    bool connected = false;
};

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

#include "dsusbdriver.h"

#include <indilogger.h>
#include <unistd.h>
#include <string.h>

#define SHUTTER_ON     0x01
#define FOCUS_ON       0x02
#define LED_RED        0x10
#define LED_ON         0x20

DSUSBDriver::DSUSBDriver(const char *device)
{
    strncpy(this->device, device, MAXINDIDEVICE);

    DEBUGDEVICE(device, INDI::Logger::DBG_DEBUG, "Finding DSUSB 0x134A, 0x9021 ...");
    // Try to see if it is DSUSB
    dev = FindDevice(0x134A, 0x9021, 0);
    // DSUSB2?
    if (dev == NULL)
    {
        DEBUGDEVICE(device, INDI::Logger::DBG_DEBUG, "Finding DSUSB 0x134A, 0x9026 ...");
        dev = FindDevice(0x134A, 0x9026, 0);
    }

    if (dev)
    {
        int rc    = Open();
        connected = (rc != -1);
        DEBUGDEVICE(device, INDI::Logger::DBG_DEBUG, "Connected to DSUSB!");
        readState();
    }
}

bool DSUSBDriver::readState()
{
    int rc = ReadBulk(&infoByte, 1, 1000);
    DEBUGFDEVICE(device, INDI::Logger::DBG_DEBUG, "RC: %d - Info Byte: %#02X", rc, infoByte);

    return (rc == 1);
}

bool DSUSBDriver::openShutter()
{
    uint8_t command = 0;

    DEBUGDEVICE(device, INDI::Logger::DBG_DEBUG, "DSUSB Opening Shutter ...");

    // First assert focus then assert shutter as per Doug recommendations from Shoestring Astronomy
    // LED = ON, COLOR = GREEN
    command = (LED_ON | FOCUS_ON);
    DEBUGDEVICE(device, INDI::Logger::DBG_DEBUG, "Asserting Focus ...");
    DEBUGFDEVICE(device, INDI::Logger::DBG_DEBUG, "CMD <%#02X>", command);
    WriteBulk(&command, 1, 1000);

    // Wait 100 ms
    usleep(100000);

    // Assert Shutter
    command |= SHUTTER_ON;
    DEBUGDEVICE(device, INDI::Logger::DBG_DEBUG, "Asserting Shutter ...");
    DEBUGFDEVICE(device, INDI::Logger::DBG_DEBUG, "CMD <%#02X>", command);
    WriteBulk(&command, 1, 1000);

    return true;
}

bool DSUSBDriver::closeShutter()
{
    uint8_t command = 0;

    DEBUGDEVICE(device, INDI::Logger::DBG_DEBUG, "DSUSB Closing Shutter ...");

    // First deassert shutter then deassert focus as per Doug recommendations from Shoestring Astronomy
    // LED = ON, COLOR = RED
    command = (LED_ON | LED_RED | FOCUS_ON);
    DEBUGDEVICE(device, INDI::Logger::DBG_DEBUG, "Deasserting Shutter ...");
    DEBUGFDEVICE(device, INDI::Logger::DBG_DEBUG, "CMD <%#02X>", command);
    WriteBulk(&command, 1, 1000);

    // Wait 100 ms
    usleep(100000);

    // Deassert Focus
    command = (LED_ON | LED_RED);
    DEBUGDEVICE(device, INDI::Logger::DBG_DEBUG, "Deasserting Focus ...");
    DEBUGFDEVICE(device, INDI::Logger::DBG_DEBUG, "CMD <%#02X>", command);
    WriteBulk(&command, 1, 1000);

    return true;
}

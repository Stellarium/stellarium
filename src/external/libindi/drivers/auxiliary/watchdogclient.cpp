/*******************************************************************************
  Copyright(c) 2015 Jasem Mutlaq. All rights reserved.

  INDI Watchdog Client.

  The clients communicates with INDI server to put devices in a safe state for shutdown
  INDI Watchdog driver.

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by the Free
  Software Foundation; either version 2 of the License, or (at your option)
  any later version.

  This program is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
  more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.

  The full GNU General Public License is included in this distribution in the
  file called LICENSE.
*******************************************************************************/

#include "watchdogclient.h"

#include <cstring>
#include <memory>

/**************************************************************************************
**
***************************************************************************************/
WatchDogClient::WatchDogClient()
{
    isReady = isRunning = mountOnline = domeOnline = false;
    mountParkSP = domeParkSP = nullptr;
}

/**************************************************************************************
**
***************************************************************************************/
WatchDogClient::~WatchDogClient()
{
}

/**************************************************************************************
**
***************************************************************************************/
void WatchDogClient::newDevice(INDI::BaseDevice *dp)
{
    IDLog("Receiving new device: %s\n", dp->getDeviceName());

    if (dome.empty() || std::string(dp->getDeviceName()) == dome)
        domeOnline = true;
    if (mount.empty() || std::string(dp->getDeviceName()) == mount)
        mountOnline = true;

    isReady = (domeOnline && mountOnline);
}

/**************************************************************************************
**
*************************************************************************************/
void WatchDogClient::newProperty(INDI::Property *property)
{
    if (!strcmp(property->getName(), "TELESCOPE_PARK"))
        mountParkSP = property->getSwitch();
    else if (!strcmp(property->getName(), "DOME_PARK"))
        domeParkSP = property->getSwitch();
}

/**************************************************************************************
**
***************************************************************************************/
void WatchDogClient::setMount(const std::string &value)
{
    mount = value;
    watchDevice(mount.c_str());
}

/**************************************************************************************
**
***************************************************************************************/
void WatchDogClient::setDome(const std::string &value)
{
    dome = value;
    watchDevice(dome.c_str());
}

/**************************************************************************************
**
***************************************************************************************/
bool WatchDogClient::parkDome()
{
    if (domeParkSP == nullptr)
        return false;

    ISwitch *sw = IUFindSwitch(domeParkSP, "PARK");

    if (sw == nullptr)
        return false;

    IUResetSwitch(domeParkSP);
    sw->s = ISS_ON;

    domeParkSP->s = IPS_BUSY;

    sendNewSwitch(domeParkSP);

    return true;
}

/**************************************************************************************
**
***************************************************************************************/
bool WatchDogClient::parkMount()
{
    if (mountParkSP == nullptr)
        return false;

    ISwitch *sw = IUFindSwitch(mountParkSP, "PARK");

    if (sw == nullptr)
        return false;

    IUResetSwitch(mountParkSP);
    sw->s = ISS_ON;

    mountParkSP->s = IPS_BUSY;

    sendNewSwitch(mountParkSP);

    return true;
}

/**************************************************************************************
**
***************************************************************************************/
IPState WatchDogClient::getDomeParkState()
{
    return domeParkSP->s;
}

/**************************************************************************************
**
***************************************************************************************/
IPState WatchDogClient::getMountParkState()
{
    return mountParkSP->s;
}

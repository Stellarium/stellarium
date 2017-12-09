/*******************************************************************************
  Copyright(c) 2017 Jasem Mutlaq. All rights reserved.

  INDI SkySafari Client for INDI Mounts.

  The clients communicates with INDI server to control the mount from SkySafari

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

#include "skysafariclient.h"

#include <cmath>
#include <cstring>

/**************************************************************************************
**
***************************************************************************************/
SkySafariClient::SkySafariClient()
{
    isReady = mountOnline = false;
}

/**************************************************************************************
**
***************************************************************************************/
SkySafariClient::~SkySafariClient()
{
}

/**************************************************************************************
**
***************************************************************************************/
void SkySafariClient::newDevice(INDI::BaseDevice *dp)
{
    IDLog("Receiving %s Device...\n", dp->getDeviceName());

    if (std::string(dp->getDeviceName()) == mount)
        mountOnline = true;

    if (mountOnline)
        isReady = true;
}

/**************************************************************************************
**
*************************************************************************************/
void SkySafariClient::newProperty(INDI::Property *property)
{
    if (!strcmp(property->getName(), "TELESCOPE_PARK"))
        mountParkSP = property->getSwitch();
    else if (!strcmp(property->getName(), "EQUATORIAL_EOD_COORD"))
        eqCoordsNP = property->getNumber();
    else if (!strcmp(property->getName(), "GEOGRAPHIC_COORD"))
        geoCoordsNP = property->getNumber();
    else if (!strcmp(property->getName(), "ON_COORD_SET"))
        gotoModeSP = property->getSwitch();
    else if (!strcmp(property->getName(), "TELESCOPE_ABORT_MOTION"))
        abortSP = property->getSwitch();
    else if (!strcmp(property->getName(), "TELESCOPE_SLEW_RATE"))
        slewRateSP = property->getSwitch();
    else if (!strcmp(property->getName(), "TELESCOPE_MOTION_NS"))
        motionNSSP = property->getSwitch();
    else if (!strcmp(property->getName(), "TELESCOPE_MOTION_WE"))
        motionWESP = property->getSwitch();
    else if (!strcmp(property->getName(), "TIME_UTC"))
        timeUTC = property->getText();
}

/**************************************************************************************
**
***************************************************************************************/
void SkySafariClient::setMount(const std::string &value)
{
    mount = value;
    watchDevice(mount.c_str());
}

/**************************************************************************************
**
***************************************************************************************/
bool SkySafariClient::parkMount()
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
IPState SkySafariClient::getMountParkState()
{
    return mountParkSP->s;
}

/**************************************************************************************
**
***************************************************************************************/
bool SkySafariClient::sendEquatorialCoords()
{
    if (eqCoordsNP == nullptr)
        return false;

    eqCoordsNP->s = IPS_BUSY;
    sendNewNumber(eqCoordsNP);
    return true;
}

/**************************************************************************************
**
***************************************************************************************/
bool SkySafariClient::sendGeographicCoords()
{
    if (geoCoordsNP == nullptr)
        return false;

    geoCoordsNP->s = IPS_BUSY;
    sendNewNumber(geoCoordsNP);
    return true;
}

/**************************************************************************************
**
***************************************************************************************/
bool SkySafariClient::sendGotoMode()
{
    if (gotoModeSP == nullptr)
        return false;

    sendNewSwitch(gotoModeSP);
    return true;
}

/**************************************************************************************
**
***************************************************************************************/
bool SkySafariClient::abort()
{
    if (abortSP == nullptr)
        return false;

    abortSP->sp[0].s = ISS_ON;

    sendNewSwitch(abortSP);
    return true;
}

/**************************************************************************************
** We get 0 to 3 which we have to map to whatever supported by mount, if any
***************************************************************************************/
bool SkySafariClient::setSlewRate(int slewRate)
{
    if (slewRateSP == nullptr)
        return false;

    int maxSlewRate = slewRateSP->nsp - 1;

    int finalSlewRate = slewRate;

    // If slew rate is betwee min and max, we intepolate
    if (slewRate > 0 && slewRate < maxSlewRate)
        finalSlewRate = static_cast<int>(ceil(slewRate * maxSlewRate / 3.0));

    IUResetSwitch(slewRateSP);
    slewRateSP->sp[finalSlewRate].s = ISS_ON;

    sendNewSwitch(slewRateSP);

    return true;
}

/**************************************************************************************
**
***************************************************************************************/
bool SkySafariClient::setMotionNS()
{
    if (motionNSSP == nullptr)
        return false;

    sendNewSwitch(motionNSSP);

    return true;
}

/**************************************************************************************
**
***************************************************************************************/
bool SkySafariClient::setMotionWE()
{
    if (motionWESP == nullptr)
        return false;

    sendNewSwitch(motionWESP);

    return true;
}

/**************************************************************************************
**
***************************************************************************************/
bool SkySafariClient::setTimeUTC()
{
    if (timeUTC == nullptr)
        return false;

    sendNewText(timeUTC);

    return true;
}

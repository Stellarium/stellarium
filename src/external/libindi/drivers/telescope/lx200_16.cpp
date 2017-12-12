/*
    LX200 16"
    Copyright (C) 2003 Jasem Mutlaq (mutlaqja@ikarustech.com)

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

#include "lx200_16.h"

#include "indicom.h"
#include "lx200driver.h"

#include <cmath>
#include <cstring>
#include <unistd.h>

#define LX16_TAB "GPS/16 inch Features"

LX200_16::LX200_16() : LX200GPS()
{
    MaxReticleFlashRate = 3;
}

const char *LX200_16::getDefaultName()
{
    return (const char *)"LX200 16";
}

bool LX200_16::initProperties()
{
    LX200GPS::initProperties();

    IUFillSwitch(&FanStatusS[0], "On", "", ISS_OFF);
    IUFillSwitch(&FanStatusS[1], "Off", "", ISS_OFF);
    IUFillSwitchVector(&FanStatusSP, FanStatusS, 2, getDeviceName(), "Fan", "", LX16_TAB, IP_RW, ISR_1OFMANY, 0,
                       IPS_IDLE);

    IUFillSwitch(&HomeSearchS[0], "Save Home", "", ISS_OFF);
    IUFillSwitch(&HomeSearchS[1], "Set Home", "", ISS_OFF);
    IUFillSwitchVector(&HomeSearchSP, HomeSearchS, 2, getDeviceName(), "Home", "", LX16_TAB, IP_RW, ISR_1OFMANY, 0,
                       IPS_IDLE);

    IUFillSwitch(&FieldDeRotatorS[0], "On", "", ISS_OFF);
    IUFillSwitch(&FieldDeRotatorS[1], "Off", "", ISS_OFF);
    IUFillSwitchVector(&FieldDeRotatorSP, FieldDeRotatorS, 2, getDeviceName(), "Field De-Rotator", "", LX16_TAB, IP_RW,
                       ISR_1OFMANY, 0, IPS_IDLE);

    IUFillNumber(&HorizontalCoordsN[0], "ALT", "Alt  D:M:S", "%10.6m", -90., 90.0, 0.0, 0);
    IUFillNumber(&HorizontalCoordsN[1], "AZ", "Az D:M:S", "%10.6m", 0.0, 360.0, 0.0, 0);
    IUFillNumberVector(&HorizontalCoordsNP, HorizontalCoordsN, 2, getDeviceName(), "HORIZONTAL_COORD",
                       "Horizontal Coord", MAIN_CONTROL_TAB, IP_RW, 0, IPS_IDLE);

    return true;
}

void LX200_16::ISGetProperties(const char *dev)
{
    if (dev != nullptr && strcmp(dev, getDeviceName()) != 0)
        return;

    // process parent first
    LX200GPS::ISGetProperties(dev);

    if (isConnected())
    {
        defineNumber(&HorizontalCoordsNP);
        defineSwitch(&FanStatusSP);
        defineSwitch(&HomeSearchSP);
        defineSwitch(&FieldDeRotatorSP);
    }
}

bool LX200_16::updateProperties()
{
    // process parent first
    LX200GPS::updateProperties();

    if (isConnected())
    {
        defineNumber(&HorizontalCoordsNP);
        defineSwitch(&FanStatusSP);
        defineSwitch(&HomeSearchSP);
        defineSwitch(&FieldDeRotatorSP);
    }
    else
    {
        deleteProperty(HorizontalCoordsNP.name);
        deleteProperty(FanStatusSP.name);
        deleteProperty(HomeSearchSP.name);
        deleteProperty(FieldDeRotatorSP.name);
    }

    return true;
}

bool LX200_16::ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    double newAlt = 0, newAz = 0;

    if (dev != nullptr && strcmp(dev, getDeviceName()) == 0)
    {
        if (!strcmp(name, HorizontalCoordsNP.name))
        {
            int i = 0, nset = 0;

            for (nset = i = 0; i < n; i++)
            {
                INumber *horp = IUFindNumber(&HorizontalCoordsNP, names[i]);
                if (horp == &HorizontalCoordsN[0])
                {
                    newAlt = values[i];
                    nset += newAlt >= -90. && newAlt <= 90.0;
                }
                else if (horp == &HorizontalCoordsN[1])
                {
                    newAz = values[i];
                    nset += newAz >= 0. && newAz <= 360.0;
                }
            }

            if (nset == 2)
            {
                if (!isSimulation() && (setObjAz(PortFD, newAz) < 0 || setObjAlt(PortFD, newAlt) < 0))
                {
                    HorizontalCoordsNP.s = IPS_ALERT;
                    IDSetNumber(&HorizontalCoordsNP, "Error setting Alt/Az.");
                    return false;
                }
                targetAZ  = newAz;
                targetALT = newAlt;

                return handleAltAzSlew();
            }
            else
            {
                HorizontalCoordsNP.s = IPS_ALERT;
                IDSetNumber(&HorizontalCoordsNP, "Altitude or Azimuth missing or invalid");
                return false;
            }
        }
    }

    LX200GPS::ISNewNumber(dev, name, values, names, n);
    return true;
}

bool LX200_16::ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    int index = 0;

    if (dev != nullptr && strcmp(dev, getDeviceName()) == 0)
    {
        if (!strcmp(name, FanStatusSP.name))
        {
            IUResetSwitch(&FanStatusSP);
            IUUpdateSwitch(&FanStatusSP, states, names, n);
            index = IUFindOnSwitchIndex(&FanStatusSP);

            if (index == 0)
            {
                if (turnFanOn(PortFD) < 0)
                {
                    FanStatusSP.s = IPS_ALERT;
                    IDSetSwitch(&FanStatusSP, "Error changing fan status.");
                    return false;
                }
            }
            else
            {
                if (turnFanOff(PortFD) < 0)
                {
                    FanStatusSP.s = IPS_ALERT;
                    IDSetSwitch(&FanStatusSP, "Error changing fan status.");
                    return false;
                }
            }

            FanStatusSP.s = IPS_OK;
            IDSetSwitch(&FanStatusSP, index == 0 ? "Fan is ON" : "Fan is OFF");
            return true;
        }

        if (!strcmp(name, HomeSearchSP.name))
        {
            int ret = 0;

            IUResetSwitch(&HomeSearchSP);
            IUUpdateSwitch(&HomeSearchSP, states, names, n);
            index = IUFindOnSwitchIndex(&HomeSearchSP);

            if (index == 0)
                ret = seekHomeAndSave(PortFD);
            else
                ret = seekHomeAndSet(PortFD);

            HomeSearchSP.s = IPS_BUSY;
            IDSetSwitch(&HomeSearchSP, index == 0 ? "Seek Home and Save" : "Seek Home and Set");
            return true;
        }

        if (!strcmp(name, FieldDeRotatorSP.name))
        {
            int ret = 0;

            IUResetSwitch(&FieldDeRotatorSP);
            IUUpdateSwitch(&FieldDeRotatorSP, states, names, n);
            index = IUFindOnSwitchIndex(&FieldDeRotatorSP);

            if (index == 0)
                ret = turnFieldDeRotatorOn(PortFD);
            else
                ret = turnFieldDeRotatorOff(PortFD);

            FieldDeRotatorSP.s = IPS_OK;
            IDSetSwitch(&FieldDeRotatorSP, index == 0 ? "Field deRotator is ON" : "Field deRotator is OFF");
            return true;
        }
    }

    return LX200GPS::ISNewSwitch(dev, name, states, names, n);
}

bool LX200_16::handleAltAzSlew()
{
    char altStr[64], azStr[64];

    if (HorizontalCoordsNP.s == IPS_BUSY)
    {
        abortSlew(PortFD);

        // sleep for 100 mseconds
        usleep(100000);
    }

    if (!isSimulation() && slewToAltAz(PortFD))
    {
        HorizontalCoordsNP.s = IPS_ALERT;
        IDSetNumber(&HorizontalCoordsNP, "Slew is not possible.");
        return false;
    }

    HorizontalCoordsNP.s = IPS_BUSY;
    fs_sexa(azStr, targetAZ, 2, 3600);
    fs_sexa(altStr, targetALT, 2, 3600);

    TrackState = SCOPE_SLEWING;
    IDSetNumber(&HorizontalCoordsNP, "Slewing to Alt %s - Az %s", altStr, azStr);
    return true;
}

bool LX200_16::ReadScopeStatus()
{
    int searchResult = 0;
    double dx, dy;

    LX200Generic::ReadScopeStatus();

    switch (HomeSearchSP.s)
    {
        case IPS_IDLE:
            break;

        case IPS_BUSY:

            if (isSimulation())
                searchResult = 1;
            else if (getHomeSearchStatus(PortFD, &searchResult) < 0)
            {
                HomeSearchSP.s = IPS_ALERT;
                IDSetSwitch(&HomeSearchSP, "Error updating home search status.");
                return false;
            }

            if (searchResult == 0)
            {
                HomeSearchSP.s = IPS_ALERT;
                IDSetSwitch(&HomeSearchSP, "Home search failed.");
            }
            else if (searchResult == 1)
            {
                HomeSearchSP.s = IPS_OK;
                IDSetSwitch(&HomeSearchSP, "Home search successful.");
            }
            else if (searchResult == 2)
                IDSetSwitch(&HomeSearchSP, "Home search in progress...");
            else
            {
                HomeSearchSP.s = IPS_ALERT;
                IDSetSwitch(&HomeSearchSP, "Home search error.");
            }
            break;

        case IPS_OK:
            break;
        case IPS_ALERT:
            break;
    }

    switch (HorizontalCoordsNP.s)
    {
        case IPS_IDLE:
            break;

        case IPS_BUSY:

            if (isSimulation())
            {
                currentAZ  = targetAZ;
                currentALT = targetALT;
                TrackState = SCOPE_TRACKING;
                return true;
            }
            if (getLX200Az(PortFD, &currentAZ) < 0 || getLX200Alt(PortFD, &currentALT) < 0)
            {
                HorizontalCoordsNP.s = IPS_ALERT;
                IDSetNumber(&HorizontalCoordsNP, "Error geting Alt/Az.");
                return false;
            }

            dx = targetAZ - currentAZ;
            dy = targetALT - currentALT;

            HorizontalCoordsNP.np[0].value = currentALT;
            HorizontalCoordsNP.np[1].value = currentAZ;

            // accuracy threshold (3'), can be changed as desired.
            if (fabs(dx) <= 0.05 && fabs(dy) <= 0.05)
            {
                HorizontalCoordsNP.s = IPS_OK;
                currentAZ            = targetAZ;
                currentALT           = targetALT;
                IDSetNumber(&HorizontalCoordsNP, "Slew is complete.");
            }
            else
                IDSetNumber(&HorizontalCoordsNP, nullptr);
            break;

        case IPS_OK:
            break;

        case IPS_ALERT:
            break;
    }

    return true;
}

void LX200_16::getBasicData()
{
    LX200GPS::getBasicData();

    if (!isSimulation())
    {
        getLX200Az(PortFD, &currentAZ);
        getLX200Alt(PortFD, &currentALT);
        HorizontalCoordsNP.np[0].value = currentALT;
        HorizontalCoordsNP.np[1].value = currentAZ;
        IDSetNumber(&HorizontalCoordsNP, nullptr);
    }
}

/*
    LX200 GPS
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

#include "lx200gps.h"

#include "lx200driver.h"

#include <cstring>
#include <unistd.h>

#define GPS_TAB "Extended GPS Features"

LX200GPS::LX200GPS() : LX200Autostar()
{
    MaxReticleFlashRate = 9;
}

const char *LX200GPS::getDefaultName()
{
    return (const char *)"LX200 GPS";
}

bool LX200GPS::initProperties()
{
    LX200Autostar::initProperties();

    IUFillSwitch(&GPSPowerS[0], "On", "", ISS_OFF);
    IUFillSwitch(&GPSPowerS[1], "Off", "", ISS_OFF);
    IUFillSwitchVector(&GPSPowerSP, GPSPowerS, 2, getDeviceName(), "GPS Power", "", GPS_TAB, IP_RW, ISR_1OFMANY, 0,
                       IPS_IDLE);

    IUFillSwitch(&GPSStatusS[0], "Sleep", "", ISS_OFF);
    IUFillSwitch(&GPSStatusS[1], "Wake Up", "", ISS_OFF);
    IUFillSwitch(&GPSStatusS[2], "Restart", "", ISS_OFF);
    IUFillSwitchVector(&GPSStatusSP, GPSStatusS, 3, getDeviceName(), "GPS Status", "", GPS_TAB, IP_RW, ISR_1OFMANY, 0,
                       IPS_IDLE);

    IUFillSwitch(&GPSUpdateS[0], "Update GPS", "", ISS_OFF);
    IUFillSwitch(&GPSUpdateS[1], "Update Client", "", ISS_OFF);
    IUFillSwitchVector(&GPSUpdateSP, GPSUpdateS, 2, getDeviceName(), "GPS System", "", GPS_TAB, IP_RW, ISR_1OFMANY, 0,
                       IPS_IDLE);

    IUFillSwitch(&AltDecPecS[0], "Enable", "", ISS_OFF);
    IUFillSwitch(&AltDecPecS[1], "Disable", "", ISS_OFF);
    IUFillSwitchVector(&AltDecPecSP, AltDecPecS, 2, getDeviceName(), "Alt/Dec PEC", "", GPS_TAB, IP_RW, ISR_1OFMANY, 0,
                       IPS_IDLE);

    IUFillSwitch(&AzRaPecS[0], "Enable", "", ISS_OFF);
    IUFillSwitch(&AzRaPecS[1], "Disable", "", ISS_OFF);
    IUFillSwitchVector(&AzRaPecSP, AzRaPecS, 2, getDeviceName(), "Az/RA PEC", "", GPS_TAB, IP_RW, ISR_1OFMANY, 0,
                       IPS_IDLE);

    IUFillSwitch(&SelenSyncS[0], "Sync", "", ISS_OFF);
    IUFillSwitchVector(&SelenSyncSP, SelenSyncS, 1, getDeviceName(), "Selenographic Sync", "", GPS_TAB, IP_RW,
                       ISR_ATMOST1, 0, IPS_IDLE);

    IUFillSwitch(&AltDecBacklashS[0], "Activate", "", ISS_OFF);
    IUFillSwitchVector(&AltDecBacklashSP, AltDecBacklashS, 1, getDeviceName(), "Alt/Dec Anti-backlash", "", GPS_TAB,
                       IP_RW, ISR_ATMOST1, 0, IPS_IDLE);

    IUFillSwitch(&AzRaBacklashS[0], "Activate", "", ISS_OFF);
    IUFillSwitchVector(&AzRaBacklashSP, AzRaBacklashS, 1, getDeviceName(), "Az/Ra Anti-backlash", "", GPS_TAB, IP_RW,
                       ISR_ATMOST1, 0, IPS_IDLE);

    IUFillSwitch(&OTAUpdateS[0], "Update", "", ISS_OFF);
    IUFillSwitchVector(&OTAUpdateSP, OTAUpdateS, 1, getDeviceName(), "OTA Update", "", GPS_TAB, IP_RW, ISR_ATMOST1, 0,
                       IPS_IDLE);

    IUFillNumber(&OTATempN[0], "Temp", "", "%03g", -200.0, 500.0, 0.0, 0);
    IUFillNumberVector(&OTATempNP, OTATempN, 1, getDeviceName(), "OTA Temp (C)", "", GPS_TAB, IP_RO, 0, IPS_IDLE);

    return true;
}

void LX200GPS::ISGetProperties(const char *dev)
{
    if (dev != nullptr && strcmp(dev, getDeviceName()) != 0)
        return;

    // process parent first
    LX200Autostar::ISGetProperties(dev);

    if (isConnected())
    {
        defineSwitch(&GPSPowerSP);
        defineSwitch(&GPSStatusSP);
        defineSwitch(&GPSUpdateSP);
        defineSwitch(&AltDecPecSP);
        defineSwitch(&AzRaPecSP);
        defineSwitch(&SelenSyncSP);
        defineSwitch(&AltDecBacklashSP);
        defineSwitch(&AzRaBacklashSP);
        defineNumber(&OTATempNP);
        defineSwitch(&OTAUpdateSP);
    }
}

bool LX200GPS::updateProperties()
{
    LX200Autostar::updateProperties();

    if (isConnected())
    {
        defineSwitch(&GPSPowerSP);
        defineSwitch(&GPSStatusSP);
        defineSwitch(&GPSUpdateSP);
        defineSwitch(&AltDecPecSP);
        defineSwitch(&AzRaPecSP);
        defineSwitch(&SelenSyncSP);
        defineSwitch(&AltDecBacklashSP);
        defineSwitch(&AzRaBacklashSP);
        defineNumber(&OTATempNP);
        defineSwitch(&OTAUpdateSP);
    }
    else
    {
        deleteProperty(GPSPowerSP.name);
        deleteProperty(GPSStatusSP.name);
        deleteProperty(GPSUpdateSP.name);
        deleteProperty(AltDecPecSP.name);
        deleteProperty(AzRaPecSP.name);
        deleteProperty(SelenSyncSP.name);
        deleteProperty(AltDecBacklashSP.name);
        deleteProperty(AzRaBacklashSP.name);
        deleteProperty(OTATempNP.name);
        deleteProperty(OTAUpdateSP.name);
    }

    return true;
}

bool LX200GPS::ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    int index = 0;
    char msg[64];

    if (dev != nullptr && strcmp(dev, getDeviceName()) == 0)
    {
        /* GPS Power */
        if (!strcmp(name, GPSPowerSP.name))
        {
            int ret = 0;

            if (IUUpdateSwitch(&GPSPowerSP, states, names, n) < 0)
                return false;

            index = IUFindOnSwitchIndex(&GPSPowerSP);
            if (index == 0)
                ret = turnGPSOn(PortFD);
            else
                ret = turnGPSOff(PortFD);

            GPSPowerSP.s = IPS_OK;
            IDSetSwitch(&GPSPowerSP, index == 0 ? "GPS System is ON" : "GPS System is OFF");
            return true;
        }

        /* GPS Status Update */
        if (!strcmp(name, GPSStatusSP.name))
        {
            int ret = 0;

            if (IUUpdateSwitch(&GPSStatusSP, states, names, n) < 0)
                return false;

            index = IUFindOnSwitchIndex(&GPSStatusSP);

            if (index == 0)
            {
                ret = gpsSleep(PortFD);
                strncpy(msg, "GPS system is in sleep mode.", 64);
            }
            else if (index == 1)
            {
                ret = gpsWakeUp(PortFD);
                strncpy(msg, "GPS system is reactivated.", 64);
            }
            else
            {
                ret = gpsRestart(PortFD);
                strncpy(msg, "GPS system is restarting...", 64);
                sendScopeTime();
                sendScopeLocation();
            }

            GPSStatusSP.s = IPS_OK;
            IDSetSwitch(&GPSStatusSP, "%s", msg);
            return true;
        }

        /* GPS Update */
        if (!strcmp(name, GPSUpdateSP.name))
        {
            if (IUUpdateSwitch(&GPSUpdateSP, states, names, n) < 0)
                return false;

            index = IUFindOnSwitchIndex(&GPSUpdateSP);

            GPSUpdateSP.s = IPS_OK;

            if (index == 0)
            {
                IDSetSwitch(&GPSUpdateSP, "Updating GPS system. This operation might take few minutes to complete...");
                if (updateGPS_System(PortFD))
                {
                    IDSetSwitch(&GPSUpdateSP, "GPS system update successful.");
                    sendScopeTime();
                    sendScopeLocation();
                }
                else
                {
                    GPSUpdateSP.s = IPS_IDLE;
                    IDSetSwitch(&GPSUpdateSP, "GPS system update failed.");
                }
            }
            else
            {
                sendScopeTime();
                sendScopeLocation();
                IDSetSwitch(&GPSUpdateSP, "Client time and location is synced to LX200 GPS Data.");
            }
            return true;
        }

        /* Alt Dec Periodic Error correction */
        if (!strcmp(name, AltDecPecSP.name))
        {
            int ret = 0;

            if (IUUpdateSwitch(&AltDecPecSP, states, names, n) < 0)
                return false;

            index = IUFindOnSwitchIndex(&AltDecPecSP);

            if (index == 0)
            {
                ret = enableDecAltPec(PortFD);
                strncpy(msg, "Alt/Dec Compensation Enabled.", 64);
            }
            else
            {
                ret = disableDecAltPec(PortFD);
                strncpy(msg, "Alt/Dec Compensation Disabled.", 64);
            }

            AltDecPecSP.s = IPS_OK;
            IDSetSwitch(&AltDecPecSP, "%s", msg);

            return true;
        }

        /* Az RA periodic error correction */
        if (!strcmp(name, AzRaPecSP.name))
        {
            int ret = 0;

            if (IUUpdateSwitch(&AzRaPecSP, states, names, n) < 0)
                return false;

            index = IUFindOnSwitchIndex(&AzRaPecSP);

            if (index == 0)
            {
                ret = enableRaAzPec(PortFD);
                strncpy(msg, "Ra/Az Compensation Enabled.", 64);
            }
            else
            {
                ret = disableRaAzPec(PortFD);
                strncpy(msg, "Ra/Az Compensation Disabled.", 64);
            }

            AzRaPecSP.s = IPS_OK;
            IDSetSwitch(&AzRaPecSP, "%s", msg);

            return true;
        }

        if (!strcmp(name, AltDecBacklashSP.name))
        {
            int ret = 0;

            ret = activateAltDecAntiBackSlash(PortFD);
            AltDecBacklashSP.s = IPS_OK;
            IDSetSwitch(&AltDecBacklashSP, "Alt/Dec Anti-backlash enabled");
            return true;
        }

        if (!strcmp(name, AzRaBacklashSP.name))
        {
            int ret = 0;

            ret = activateAzRaAntiBackSlash(PortFD);
            AzRaBacklashSP.s = IPS_OK;
            IDSetSwitch(&AzRaBacklashSP, "Az/Ra Anti-backlash enabled");
            return true;
        }

        if (!strcmp(name, OTAUpdateSP.name))
        {
            IUResetSwitch(&OTAUpdateSP);

            if (getOTATemp(PortFD, &OTATempNP.np[0].value) < 0)
            {
                OTAUpdateSP.s = IPS_ALERT;
                OTATempNP.s   = IPS_ALERT;
                IDSetNumber(&OTATempNP, "Error: OTA temperature read timed out.");
                return false;
            }
            else
            {
                OTAUpdateSP.s = IPS_OK;
                OTATempNP.s   = IPS_OK;
                IDSetNumber(&OTATempNP, nullptr);
                IDSetSwitch(&OTAUpdateSP, nullptr);
                return true;
            }
        }
    }

    return LX200Autostar::ISNewSwitch(dev, name, states, names, n);
}

bool LX200GPS::updateTime(ln_date *utc, double utc_offset)
{
    ln_zonedate ltm;

    if (isSimulation())
        return true;

    JD = ln_get_julian_day(utc);

    DEBUGF(INDI::Logger::DBG_DEBUG, "New JD is %.2f", JD);

    ln_date_to_zonedate(utc, &ltm, utc_offset * 3600);

    DEBUGF(INDI::Logger::DBG_DEBUG, "Local time is %02d:%02d:%02g", ltm.hours, ltm.minutes, ltm.seconds);

    // Set Local Time
    if (setLocalTime24(ltm.hours, ltm.minutes, ltm.seconds) == false)
    {
        DEBUG(INDI::Logger::DBG_ERROR, "Error setting local time time.");
        return false;
    }

    // UTC Date, it's not Local for LX200GPS
    if (setLocalDate(utc->days, utc->months, utc->years) == false)
    {
        DEBUG(INDI::Logger::DBG_ERROR, "Error setting UTC date.");
        return false;
    }

    // Meade defines UTC Offset as the offset ADDED to local time to yield UTC, which
    // is the opposite of the standard definition of UTC offset!
    if (setUTCOffset(utc_offset) == false)
    {
        DEBUG(INDI::Logger::DBG_ERROR, "Error setting UTC Offset.");
        return false;
    }

    DEBUG(INDI::Logger::DBG_SESSION, "Time updated, updating planetary data...");
    return true;
}

bool LX200GPS::UnPark()
{
    int ret = 0;

    ret = initTelescope(PortFD);
    TrackState = SCOPE_IDLE;
    return true;
}

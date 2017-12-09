/*******************************************************************************
  Copyright(c) 2015 Jasem Mutlaq. All rights reserved.

  INDI GPS Device Class

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

#include "indigps.h"

#include <cstring>

#define POLLMS  2000

namespace INDI
{

bool GPS::initProperties()
{
    DefaultDevice::initProperties();

    IUFillNumber(&PeriodN[0], "PERIOD", "Period (s)", "%.f", 0, 3600, 60.0, 0);
    IUFillNumberVector(&PeriodNP, PeriodN, 1, getDeviceName(), "GPS_REFRESH_PERIOD", "Refresh", MAIN_CONTROL_TAB, IP_RW, 0, IPS_IDLE);

    IUFillSwitch(&RefreshS[0], "REFRESH", "GPS", ISS_OFF);
    IUFillSwitchVector(&RefreshSP, RefreshS, 1, getDeviceName(), "GPS_REFRESH", "Refresh", MAIN_CONTROL_TAB, IP_RW,
                       ISR_ATMOST1, 0, IPS_IDLE);

    IUFillNumber(&LocationN[LOCATION_LATITUDE], "LAT", "Lat (dd:mm:ss)", "%010.6m", -90, 90, 0, 0.0);
    IUFillNumber(&LocationN[LOCATION_LONGITUDE], "LONG", "Lon (dd:mm:ss)", "%010.6m", 0, 360, 0, 0.0);
    IUFillNumber(&LocationN[LOCATION_ELEVATION], "ELEV", "Elevation (m)", "%g", -200, 10000, 0, 0);
    IUFillNumberVector(&LocationNP, LocationN, 3, getDeviceName(), "GEOGRAPHIC_COORD", "Location", MAIN_CONTROL_TAB,
                       IP_RO, 60, IPS_IDLE);

    IUFillText(&TimeT[0], "UTC", "UTC Time", nullptr);
    IUFillText(&TimeT[1], "OFFSET", "UTC Offset", nullptr);
    IUFillTextVector(&TimeTP, TimeT, 2, getDeviceName(), "TIME_UTC", "UTC", MAIN_CONTROL_TAB, IP_RO, 60, IPS_IDLE);

    updatePeriodMS = PeriodN[0].value;

    return true;
}

bool GPS::updateProperties()
{
    DefaultDevice::updateProperties();

    if (isConnected())
    {
        // Update GPS and send values to client
        IPState state = updateGPS();

        LocationNP.s = state;
        defineNumber(&LocationNP);
        TimeTP.s = state;
        defineText(&TimeTP);
        RefreshSP.s = state;
        defineSwitch(&RefreshSP);
        defineNumber(&PeriodNP);

        if (state != IPS_OK)
        {
            if (state == IPS_BUSY)
                DEBUG(Logger::DBG_SESSION, "GPS fix is in progress...");

            timerID = SetTimer(POLLMS);
        }
        else if (PeriodN[0].value > 0)
            timerID = SetTimer(PeriodN[0].value);
    }
    else
    {
        deleteProperty(LocationNP.name);
        deleteProperty(TimeTP.name);
        deleteProperty(RefreshSP.name);
        deleteProperty(PeriodNP.name);

        if (timerID > 0)
        {
            RemoveTimer(timerID);
            timerID = -1;
        }
    }

    return true;
}

void GPS::TimerHit()
{
    if (!isConnected())
    {
        timerID = SetTimer(POLLMS);
        return;
    }

    IPState state = updateGPS();

    LocationNP.s = state;
    TimeTP.s = state;
    RefreshSP.s = state;

    switch (state)
    {
        // Ok
        case IPS_OK:        
            IDSetNumber(&LocationNP, nullptr);
            IDSetText(&TimeTP, nullptr);
            // We got data OK, but if we are required to update once in a while, we'll call it.
            if (PeriodN[0].value > 0)
                timerID = SetTimer(PeriodN[0].value*1000);
            return;
            break;

        // GPS fix is in progress or alert
        case IPS_ALERT:
            IDSetNumber(&LocationNP, nullptr);
            IDSetText(&TimeTP, nullptr);            
            break;

        default:
            break;
    }

    timerID = SetTimer(POLLMS);
}

IPState GPS::updateGPS()
{
    DEBUG(Logger::DBG_ERROR, "updateGPS() must be implemented in GPS device child class to update TIME_UTC and "
                                   "GEOGRAPHIC_COORD properties.");
    return IPS_ALERT;
}

bool GPS::ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    if (dev != nullptr && strcmp(dev, getDeviceName()) == 0)
    {
        if (!strcmp(name, RefreshSP.name))
        {
            RefreshS[0].s = ISS_OFF;
            RefreshSP.s   = IPS_OK;
            IDSetSwitch(&RefreshSP, nullptr);

            // Manual trigger
            TimerHit();
        }
    }

    return DefaultDevice::ISNewSwitch(dev, name, states, names, n);
}

bool GPS::ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    if (dev != nullptr && strcmp(dev, getDeviceName()) == 0)
    {
        if (strcmp(name, PeriodNP.name) == 0)
        {
            double prevPeriod = PeriodN[0].value;
            IUUpdateNumber(&PeriodNP, values, names, n);
            // Do not remove timer if GPS update is still in progress
            if (timerID > 0 && RefreshSP.s != IPS_BUSY)
            {
                RemoveTimer(timerID);
                timerID = -1;
            }

            if (PeriodN[0].value == 0)
            {
                DEBUG(Logger::DBG_SESSION, "GPS Update Timer disabled.");
            }
            else
            {
                timerID = SetTimer(PeriodN[0].value*1000);
                if (prevPeriod == 0)
                    DEBUG(Logger::DBG_SESSION, "GPS Update Timer enabled.");
            }

            PeriodNP.s = IPS_OK;
            IDSetNumber(&PeriodNP, nullptr);

            return true;
        }
    }

    return DefaultDevice::ISNewNumber(dev, name, values, names, n);
}

bool GPS::saveConfigItems(FILE *fp)
{
    DefaultDevice::saveConfigItems(fp);

    IUSaveConfigNumber(fp, &PeriodNP);
    return true;
}
}

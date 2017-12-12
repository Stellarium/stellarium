/*******************************************************************************
  Copyright(c) 2012 Jasem Mutlaq. All rights reserved.

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

#include "gpusb.h"

#include "gpdriver.h"

#include <memory>
#include <cstring>
#include <unistd.h>

#define POLLMS 250

// We declare an auto pointer to gpGuide.
std::unique_ptr<GPUSB> gpGuide(new GPUSB());

void ISGetProperties(const char *dev)
{
    gpGuide->ISGetProperties(dev);
}

void ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    gpGuide->ISNewSwitch(dev, name, states, names, n);
}

void ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n)
{
    gpGuide->ISNewText(dev, name, texts, names, n);
}

void ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    gpGuide->ISNewNumber(dev, name, values, names, n);
}

void ISNewBLOB(const char *dev, const char *name, int sizes[], int blobsizes[], char *blobs[], char *formats[],
               char *names[], int n)
{
    INDI_UNUSED(dev);
    INDI_UNUSED(name);
    INDI_UNUSED(sizes);
    INDI_UNUSED(blobsizes);
    INDI_UNUSED(blobs);
    INDI_UNUSED(formats);
    INDI_UNUSED(names);
    INDI_UNUSED(n);
}
void ISSnoopDevice(XMLEle *root)
{
    INDI_UNUSED(root);
}

GPUSB::GPUSB()
{
    driver = new GPUSBDriver();
    WEDir = NSDir = 0;
    InWEPulse = InNSPulse = false;
    WEPulseRequest = NSPulseRequest = 0;
    WEtimerID = NStimerID = 0;
}

GPUSB::~GPUSB()
{
    //dtor

    delete (driver);
}

const char *GPUSB::getDefaultName()
{
    return (const char *)"GPUSB";
}

bool GPUSB::Connect()
{
    driver->setDebug(isDebug());

    bool rc = driver->Connect();

    if (rc)
        DEBUG(INDI::Logger::DBG_SESSION, "GPUSB is online.");
    else
        DEBUG(INDI::Logger::DBG_ERROR, "Error: cannot find GPUSB device.");

    return rc;
}

bool GPUSB::Disconnect()
{
    DEBUG(INDI::Logger::DBG_SESSION, "GPSUSB is offline.");

    return driver->Disconnect();
}

bool GPUSB::initProperties()
{
    initGuiderProperties(getDeviceName(), MAIN_CONTROL_TAB);

    addDebugControl();

    return INDI::DefaultDevice::initProperties();
}

bool GPUSB::updateProperties()
{
    INDI::DefaultDevice::updateProperties();

    if (isConnected())
    {
        defineNumber(&GuideNSNP);
        defineNumber(&GuideWENP);
    }
    else
    {
        deleteProperty(GuideNSNP.name);
        deleteProperty(GuideWENP.name);
    }

    return true;
}

void GPUSB::ISGetProperties(const char *dev)
{
    INDI::DefaultDevice::ISGetProperties(dev);
}

bool GPUSB::ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    if (dev != nullptr && strcmp(dev, getDeviceName()) == 0)
    {
        if (!strcmp(name, GuideNSNP.name) || !strcmp(name, GuideWENP.name))
        {
            processGuiderProperties(name, values, names, n);
            return true;
        }
    }

    return INDI::DefaultDevice::ISNewNumber(dev, name, values, names, n);
}

bool GPUSB::ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    return INDI::DefaultDevice::ISNewSwitch(dev, name, states, names, n);
}

bool GPUSB::ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n)
{
    return INDI::DefaultDevice::ISNewText(dev, name, texts, names, n);
}

bool GPUSB::ISSnoopDevice(XMLEle *root)
{
    return INDI::DefaultDevice::ISSnoopDevice(root);
}

void GPUSB::debugTriggered(bool enable)
{
    driver->setDebug(enable);
}

float GPUSB::CalcWEPulseTimeLeft()
{
    double timesince;
    double timeleft;
    struct timeval now { 0, 0 };
    gettimeofday(&now, nullptr);

    timesince = (double)(now.tv_sec * 1000.0 + now.tv_usec / 1000) -
                (double)(WEPulseStart.tv_sec * 1000.0 + WEPulseStart.tv_usec / 1000);
    timesince = timesince / 1000;

    timeleft = WEPulseRequest - timesince;
    return timeleft;
}

float GPUSB::CalcNSPulseTimeLeft()
{
    double timesince;
    double timeleft;
    struct timeval now { 0, 0 };
    gettimeofday(&now, nullptr);

    timesince = (double)(now.tv_sec * 1000.0 + now.tv_usec / 1000) -
                (double)(NSPulseStart.tv_sec * 1000.0 + NSPulseStart.tv_usec / 1000);
    timesince = timesince / 1000;

    timeleft = NSPulseRequest - timesince;
    return timeleft;
}

void GPUSB::TimerHit()
{
    float timeleft;

    if (InWEPulse)
    {
        timeleft = CalcWEPulseTimeLeft();

        if (timeleft < 1.0)
        {
            if (timeleft > 0.25)
            {
                //  a quarter of a second or more
                //  just set a tighter timer
                WEtimerID = SetTimer(250);
            }
            else
            {
                if (timeleft > 0.07)
                {
                    //  use an even tighter timer
                    WEtimerID = SetTimer(50);
                }
                else
                {
                    //  it's real close now, so spin on it
                    while (timeleft > 0)
                    {
                        int slv;
                        slv = 100000 * timeleft;
                        //IDLog("usleep %d\n",slv);
                        usleep(slv);
                        timeleft = CalcWEPulseTimeLeft();
                    }

                    driver->stopPulse(WEDir);
                    InWEPulse = false;

                    // If we have another pulse, keep going
                    if (!InNSPulse)
                        SetTimer(250);
                }
            }
        }
        else if (!InNSPulse)
        {
            WEtimerID = SetTimer(250);
        }
    }

    if (InNSPulse)
    {
        timeleft = CalcNSPulseTimeLeft();

        if (timeleft < 1.0)
        {
            if (timeleft > 0.25)
            {
                //  a quarter of a second or more
                //  just set a tighter timer
                NStimerID = SetTimer(250);
            }
            else
            {
                if (timeleft > 0.07)
                {
                    //  use an even tighter timer
                    NStimerID = SetTimer(50);
                }
                else
                {
                    //  it's real close now, so spin on it
                    while (timeleft > 0)
                    {
                        int slv;
                        slv = 100000 * timeleft;
                        //IDLog("usleep %d\n",slv);
                        usleep(slv);
                        timeleft = CalcNSPulseTimeLeft();
                    }

                    driver->stopPulse(NSDir);
                    InNSPulse = false;
                }
            }
        }
        else
        {
            NStimerID = SetTimer(250);
        }
    }
}

IPState GPUSB::GuideNorth(float ms)
{
    RemoveTimer(NStimerID);

    driver->startPulse(GPUSB_NORTH);

    NSDir = GPUSB_NORTH;

    DEBUG(INDI::Logger::DBG_DEBUG, "Starting NORTH guide");

    if (ms <= POLLMS)
    {
        usleep(ms * 1000);

        driver->stopPulse(GPUSB_NORTH);
        return IPS_OK;
    }

    NSPulseRequest = ms / 1000.0;
    gettimeofday(&NSPulseStart, nullptr);
    InNSPulse = true;

    NStimerID = SetTimer(ms - 50);

    return IPS_BUSY;
}

IPState GPUSB::GuideSouth(float ms)
{
    RemoveTimer(NStimerID);

    driver->startPulse(GPUSB_SOUTH);

    DEBUG(INDI::Logger::DBG_DEBUG, "Starting SOUTH guide");

    NSDir = GPUSB_SOUTH;

    if (ms <= POLLMS)
    {
        usleep(ms * 1000);

        driver->stopPulse(GPUSB_SOUTH);
        return IPS_OK;
    }

    NSPulseRequest = ms / 1000.0;
    gettimeofday(&NSPulseStart, nullptr);
    InNSPulse = true;

    NStimerID = SetTimer(ms - 50);

    return IPS_BUSY;
}

IPState GPUSB::GuideEast(float ms)
{
    RemoveTimer(WEtimerID);

    driver->startPulse(GPUSB_EAST);

    DEBUG(INDI::Logger::DBG_DEBUG, "Starting EAST guide");

    WEDir = GPUSB_EAST;

    if (ms <= POLLMS)
    {
        usleep(ms * 1000);

        driver->stopPulse(GPUSB_EAST);
        return IPS_OK;
    }

    WEPulseRequest = ms / 1000.0;
    gettimeofday(&WEPulseStart, nullptr);
    InWEPulse = true;

    WEtimerID = SetTimer(ms - 50);

    return IPS_BUSY;
}

IPState GPUSB::GuideWest(float ms)
{
    RemoveTimer(WEtimerID);

    driver->startPulse(GPUSB_WEST);

    DEBUG(INDI::Logger::DBG_DEBUG, "Starting WEST guide");

    WEDir = GPUSB_WEST;

    if (ms <= POLLMS)
    {
        usleep(ms * 1000);

        driver->stopPulse(GPUSB_WEST);
        return IPS_OK;
    }

    WEPulseRequest = ms / 1000.0;
    gettimeofday(&WEPulseStart, nullptr);
    InWEPulse = true;

    WEtimerID = SetTimer(ms - 50);

    return IPS_BUSY;
}

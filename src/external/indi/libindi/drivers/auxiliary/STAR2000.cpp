/*******************************************************************************
  created 2014 G. Schmidt

  derived from gpusb code from Jasem Mutlaq

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

#include "STAR2000.h"

#include "STAR2kdriver.h"
#include "indistandardproperty.h"

#include <cstring>
#include <memory>

#include <unistd.h>

#define POLLMS 250

// We declare an auto pointer to gpGuide.
std::unique_ptr<STAR2000> s2kGuide(new STAR2000());

void ISGetProperties(const char *dev)
{
    s2kGuide->ISGetProperties(dev);
}

void ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    s2kGuide->ISNewSwitch(dev, name, states, names, n);
}

void ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n)
{
    s2kGuide->ISNewText(dev, name, texts, names, n);
}

void ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    s2kGuide->ISNewNumber(dev, name, values, names, n);
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

const char *STAR2000::getDefaultName()
{
    return (const char *)"STAR2000";
}

bool STAR2000::Connect()
{
    bool rc = false;

    if (isConnected())
        return true;

    rc = Connect(PortT[0].text);

    if (rc)
        SetTimer(POLLMS);

    return rc;
}

bool STAR2000::Connect(char *port)
{
    if (isSimulation())
    {
        IDMessage(getDeviceName(), "Simulated STAR2000 box is online.");
        return true;
    }

    if (ConnectSTAR2k(port) < 0)
    {
        IDMessage(getDeviceName(),
                  "Error connecting to port %s. Make sure you have BOTH write and read permission to your port.", port);
        return false;
    }

    IDMessage(getDeviceName(), "STAR2000 box is online.");

    return true;
}

bool STAR2000::Disconnect()
{
    IDMessage(getDeviceName(), "STAR200 box is offline.");

    if (!isSimulation())
        DisconnectSTAR2k();

    return true;
}

bool STAR2000::initProperties()
{
    bool rc = INDI::DefaultDevice::initProperties();

    IUFillText(&PortT[0], "PORT", "Port", "/dev/ttyUSB0");
    IUFillTextVector(&PortTP, PortT, 1, getDeviceName(), INDI::SP::DEVICE_PORT, "Ports", OPTIONS_TAB, IP_RW, 60, IPS_IDLE);

    initGuiderProperties(getDeviceName(), MAIN_CONTROL_TAB);
    addDebugControl();

    setDriverInterface(TELESCOPE_INTERFACE);

    return (rc);
}

bool STAR2000::updateProperties()
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

void STAR2000::ISGetProperties(const char *dev)
{
    INDI::DefaultDevice::ISGetProperties(dev);
    defineText(&PortTP);
    loadConfig(true, INDI::SP::DEVICE_PORT);
}

bool STAR2000::ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    if (dev != nullptr && strcmp(dev, getDeviceName()) == 0)
    {
        if (strcmp(name, GuideNSNP.name) == 0 || strcmp(name, GuideWENP.name) == 0)
        {
            processGuiderProperties(name, values, names, n);
            return true;
        }
    }

    return INDI::DefaultDevice::ISNewNumber(dev, name, values, names, n);
}

bool STAR2000::ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    return INDI::DefaultDevice::ISNewSwitch(dev, name, states, names, n);
}

bool STAR2000::ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n)
{
    if (strcmp(name, PortTP.name) == 0)
    {
        PortTP.s = IPS_OK;
        IUUpdateText(&PortTP, texts, names, n);
        IDSetText(&PortTP, nullptr);

        return true;
    }

    return INDI::DefaultDevice::ISNewText(dev, name, texts, names, n);
}

bool STAR2000::ISSnoopDevice(XMLEle *root)
{
    return INDI::DefaultDevice::ISSnoopDevice(root);
}

bool STAR2000::saveConfigItems(FILE *fp)
{
    IUSaveConfigText(fp, &PortTP);
    return true;
}

float STAR2000::CalcWEPulseTimeLeft()
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

float STAR2000::CalcNSPulseTimeLeft()
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

void STAR2000::TimerHit()
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

                    StopPulse(WEDir);
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

                    StopPulse(NSDir);
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

IPState STAR2000::GuideNorth(float ms)
{
    RemoveTimer(NStimerID);

    StartPulse(NORTH);

    NSDir = NORTH;

    DEBUG(INDI::Logger::DBG_DEBUG, "Starting NORTH guide");

    if (ms <= POLLMS)
    {
        usleep(ms * 1000);

        StopPulse(NORTH);

        return IPS_OK;
    }

    NSPulseRequest = ms / 1000.0;
    gettimeofday(&NSPulseStart, nullptr);
    InNSPulse = true;

    NStimerID = SetTimer(ms - 50);

    return IPS_BUSY;
}

IPState STAR2000::GuideSouth(float ms)
{
    RemoveTimer(NStimerID);

    StartPulse(SOUTH);

    DEBUG(INDI::Logger::DBG_DEBUG, "Starting SOUTH guide");

    NSDir = SOUTH;

    if (ms <= POLLMS)
    {
        usleep(ms * 1000);

        StopPulse(SOUTH);

        return IPS_OK;
    }

    NSPulseRequest = ms / 1000.0;
    gettimeofday(&NSPulseStart, nullptr);
    InNSPulse = true;

    NStimerID = SetTimer(ms - 50);

    return IPS_BUSY;
}

IPState STAR2000::GuideEast(float ms)
{
    RemoveTimer(WEtimerID);

    StartPulse(EAST);

    DEBUG(INDI::Logger::DBG_DEBUG, "Starting EAST guide");

    WEDir = EAST;

    if (ms <= POLLMS)
    {
        usleep(ms * 1000);

        StopPulse(EAST);

        return IPS_OK;
    }

    WEPulseRequest = ms / 1000.0;
    gettimeofday(&WEPulseStart, nullptr);
    InWEPulse = true;

    WEtimerID = SetTimer(ms - 50);

    return IPS_BUSY;
}

IPState STAR2000::GuideWest(float ms)
{
    RemoveTimer(WEtimerID);

    StartPulse(WEST);

    DEBUG(INDI::Logger::DBG_DEBUG, "Starting WEST guide");

    WEDir = WEST;

    if (ms <= POLLMS)
    {
        usleep(ms * 1000);

        StopPulse(WEST);

        return IPS_OK;
    }

    WEPulseRequest = ms / 1000.0;
    gettimeofday(&WEPulseStart, nullptr);
    InWEPulse = true;

    WEtimerID = SetTimer(ms - 50);

    return IPS_BUSY;
}

#if 0
    Sky Commander INDI driver
    Copyright (C) 2005 Jasem Mutlaq (mutlaqja@ikarustech.com)

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

#endif

#include "lx200driver.h"

#include "indidevapi.h"
#include "indicom.h"

#ifndef _WIN32
#include <termios.h>
#endif

#define mydev                "Sky Commander"
#define BASIC_GROUP          "Main Control"
#define POLLMS               1000
#define currentRA            eq[0].value
#define currentDEC           eq[1].value
#define SKYCOMMANDER_TIMEOUT 5

static void ISPoll(void *);
static void ISInit(void);
static void connectTelescope(void);

int fd;

static ISwitch PowerS[] = { { "CONNECT", "Connect", ISS_OFF, 0, 0 }, { "DISCONNECT", "Disconnect", ISS_ON, 0, 0 } };
ISwitchVectorProperty PowerSP = { mydev, "CONNECTION", "Connection", BASIC_GROUP,    IP_RW, ISR_1OFMANY,
                                  0,     IPS_IDLE,     PowerS,       NARRAY(PowerS), "",    0 };

static IText PortT[]              = { { "PORT", "Port", 0, 0, 0, 0 } };
static ITextVectorProperty PortTP = { mydev,    "DEVICE_PORT", "Ports",       BASIC_GROUP, IP_RW, 0,
                                      IPS_IDLE, PortT,         NARRAY(PortT), "",          0 };

/* equatorial position */
INumber eq[] = {
    { "RA", "RA  H:M:S", "%10.6m", 0., 24., 0., 0., 0, 0, 0 },
    { "DEC", "Dec D:M:S", "%10.6m", -90., 90., 0., 0., 0, 0, 0 },
};
INumberVectorProperty eqNum = {
    mydev, "EQUATORIAL_EOD_COORD", "Equatorial JNow", BASIC_GROUP, IP_RO, 0, IPS_IDLE, eq, NARRAY(eq), "", 0
};

void ISInit(void)
{
    static int isInit = 0;

    if (isInit)
        return;

    isInit = 1;
    fd     = -1;

    IEAddTimer(POLLMS, ISPoll, NULL);
}

void ISGetProperties(const char *dev)
{
    ISInit();

    dev = dev;

    IDDefSwitch(&PowerSP, NULL);
    IDDefText(&PortTP, NULL);
    IDDefNumber(&eqNum, NULL);
}

void ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    ISInit();

    dev = dev;

    if (!strcmp(name, PowerSP.name))
    {
        IUResetSwitch(&PowerSP);
        IUUpdateSwitch(&PowerSP, states, names, n);
        connectTelescope();
        return;
    }
}

void ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n)
{
    ISInit();

    dev   = dev;
    names = names;
    n     = n;

    if (!strcmp(name, PortTP.name))
    {
        IUSaveText(&PortT[0], texts[0]);
        PortTP.s = IPS_OK;
        IDSetText(&PortTP, NULL);
        return;
    }
}
void ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    dev    = dev;
    name   = name;
    values = values;
    names  = names;
    n      = n;
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

int updateSkyCommanderCoord(int fd, double *ra, double *dec)
{
    char coords[16];
    char CR[1] = { (char)0x0D };
    float RA = 0.0, DEC = 0.0;
    int error_type;
    int nbytes_read = 0;

    error_type = write(fd, CR, 1);

    error_type = tty_read(fd, coords, 16, SKYCOMMANDER_TIMEOUT, &nbytes_read);
    /*read_ret = portRead(coords, 16, LX200_TIMEOUT);*/
    tcflush(fd, TCIFLUSH);

    nbytes_read = sscanf(coords, " %g %g", &RA, &DEC);

    if (nbytes_read < 2)
    {
        IDLog("Error in Sky commander number format [%s], exiting.\n", coords);
        return error_type;
    }

    *ra  = RA;
    *dec = DEC;

    return 0;
}

void ISPoll(void *p)
{
    p = p;

    if (PowerS[0].s == ISS_ON)
    {
        switch (eqNum.s)
        {
            case IPS_IDLE:
            case IPS_OK:
            case IPS_BUSY:
                if (updateSkyCommanderCoord(fd, &currentRA, &currentDEC) < 0)
                {
                    eqNum.s = IPS_ALERT;
                    IDSetNumber(&eqNum, "Unknown error while reading telescope coordinates");
                    IDLog("Unknown error while reading telescope coordinates\n");
                    break;
                }

                IDSetNumber(&eqNum, NULL);
                break;

            case IPS_ALERT:
                break;
        }
    }

    IEAddTimer(POLLMS, ISPoll, NULL);
}

void connectTelescope(void)
{
    switch (PowerS[0].s)
    {
        case ISS_ON:
            if (tty_connect(PortT[0].text, 9600, 8, 0, 1, &fd) != TTY_OK)
            {
                PowerSP.s = IPS_ALERT;
                IUResetSwitch(&PowerSP);
                IDSetSwitch(&PowerSP, "Error connecting to port %s", PortT[0].text);
                return;
            }

            PowerSP.s = IPS_OK;
            IDSetSwitch(&PowerSP, "Sky Commander is online.");
            break;

        case ISS_OFF:
            tty_disconnect(fd);
            IUResetSwitch(&PowerSP);
            eqNum.s = PortTP.s = PowerSP.s = IPS_IDLE;
            IDSetSwitch(&PowerSP, "Sky Commander is offline.");
            IDSetText(&PortTP, NULL);
            IDSetNumber(&eqNum, NULL);
            break;
    }
}

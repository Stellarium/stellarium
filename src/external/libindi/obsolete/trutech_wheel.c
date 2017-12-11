#if 0
   True Technology Filter Wheel
    Copyright (C) 2006 Jasem Mutlaq (mutlaqja AT ikarustech DOT com)

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

#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include <stdarg.h>
#include <cmath>
#include <unistd.h>
#include <ctime>
#include <fcntl.h>
#include <cerrno>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "indidevapi.h"
#include "eventloop.h"
#include "indicom.h"

void ISInit(void);
void getBasicData(void);
void ISPoll(void *);
void handleExposure(void *);
void connectFilter(void);
int manageDefaults(char errmsg[]);
int checkPowerS(ISwitchVectorProperty *sp);
int checkPowerN(INumberVectorProperty *np);
int checkPowerT(ITextVectorProperty *tp);
int getOnSwitch(ISwitchVectorProperty *sp);
int isFilterConnected(void);

static int targetFilter;
static int fd;
const unsigned char COMM_PRE  = 0x01;
const unsigned char COMM_INIT = 0xA5;
const unsigned char COMM_FILL = 0x20;

#define mydev                "TruTech Wheel"
#define MAIN_GROUP           "Main Control"
#define currentFilter        FilterPositionN[0].value
#define POLLMS               3000
#define DEFAULT_FILTER_COUNT 5
#define MAX_FILTER_COUNT     10
#define ERRMSG_SIZE          1024

#define CMD_SIZE 5
#define CMD_JUNK 64
#define CMD_RESP 15

#define FILTER_TIMEOUT 15 /* 15 Seconds before timeout */
#define FIRST_FILTER   1

#define DEBUG_ON      0
#define SIMULATION_ON 0

/*INDI controls */

/* Connect/Disconnect */
static ISwitch PowerS[] = { { "CONNECT", "Connect", ISS_OFF, 0, 0 }, { "DISCONNECT", "Disconnect", ISS_ON, 0, 0 } };
static ISwitchVectorProperty PowerSP = { mydev, "CONNECTION", "Connection", MAIN_GROUP,     IP_RW, ISR_1OFMANY,
                                         60,    IPS_IDLE,     PowerS,       NARRAY(PowerS), "",    0 };

/* Connection Port */
static IText PortT[]              = { { "PORT", "Port", 0, 0, 0, 0 } };
static ITextVectorProperty PortTP = { mydev,    "DEVICE_PORT", "Ports",       MAIN_GROUP, IP_RW, 0,
                                      IPS_IDLE, PortT,         NARRAY(PortT), "",         0 };

/* Home/Learn Swtich */
static ISwitch HomeS[]              = { { "Find", "", ISS_OFF, 0, 0 } };
static ISwitchVectorProperty HomeSP = { mydev, "HOME",   "",    MAIN_GROUP,    IP_RW, ISR_1OFMANY,
                                        60,    IPS_IDLE, HomeS, NARRAY(HomeS), "",    0 };

/* Filter Count */
static INumber FilterCountN[] = { { "Count", "", "%2.0f", 0, MAX_FILTER_COUNT, 1, DEFAULT_FILTER_COUNT, 0, 0, 0 } };
static INumberVectorProperty FilterCountNP = { mydev,        "Filter Count",       "", MAIN_GROUP, IP_RW, 0, IPS_IDLE,
                                               FilterCountN, NARRAY(FilterCountN), "", 0 };

/* Filter Position */
static INumber FilterPositionN[] = { { "FILTER_SLOT_VALUE", "Active Filter", "%2.0f", 1, DEFAULT_FILTER_COUNT, 1, 1, 0,
                                       0, 0 } };
static INumberVectorProperty FilterPositionNP = {
    mydev, "FILTER_SLOT", "Filter", MAIN_GROUP, IP_RW, 0, IPS_IDLE, FilterPositionN, NARRAY(FilterPositionN), "", 0
};

/* send client definitions of all properties */
void ISInit()
{
    static int isInit = 0;

    if (isInit)
        return;

    targetFilter = -1;
    fd           = -1;

    isInit = 1;
}

void ISGetProperties(const char *dev)
{
    ISInit();

    if (dev != nullptr && strcmp(mydev, dev))
        return;

    /* Main Control */
    IDDefSwitch(&PowerSP, NULL);
    IDDefText(&PortTP, NULL);
    IDDefNumber(&FilterCountNP, NULL);
    IDDefSwitch(&HomeSP, NULL);
    IDDefNumber(&FilterPositionNP, NULL);
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

void ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    /* ignore if not ours */
    if (dev != nullptr && strcmp(dev, mydev))
        return;

    ISInit();

    /* Connection */
    if (!strcmp(name, PowerSP.name))
    {
        IUUpdateSwitch(&PowerSP, states, names, n);
        connectFilter();
        return;
    }

    /* Home Search */
    if (!strcmp(name, HomeSP.name))
    {
        int err;
        int nbytes           = 0;
        unsigned char type   = 0x03;
        unsigned char chksum = COMM_INIT + type + COMM_FILL;
        unsigned char filter_command[CMD_SIZE];
        //snprintf(filter_command, CMD_SIZE,  "%c%c%c%c%c", COMM_PRE, COMM_INIT, type, COMM_FILL, chksum);
        snprintf(filter_command, CMD_SIZE, "%c%c%c%c", COMM_INIT, type, COMM_FILL, chksum);

        if (checkPowerS(&HomeSP))
            return;

        if (!SIMULATION_ON)
            err = tty_write(fd, filter_command, CMD_SIZE, &nbytes);

        if (DEBUG_ON)
        {
            IDLog("Home Search Command (int): #%d#%d#%d#%d#\n", COMM_INIT, type, COMM_FILL, chksum);
            IDLog("Home Search Command (char): #%c#%c#%c#%c#\n", COMM_INIT, type, COMM_FILL, chksum);
        }

        /* Send Home Command */
        if (err != TTY_OK)
        {
            char error_message[ERRMSG_SIZE];
            tty_error_msg(err, error_message, ERRMSG_SIZE);

            HomeSP.s = IPS_ALERT;
            IDSetSwitch(&HomeSP, "Sending command Home to filter failed. %s", error_message);
            IDLog("Sending command Home to filter failed. %s\n", error_message);

            return;
        }

        FilterPositionN[0].value = 1;
        FilterPositionNP.s       = IPS_OK;
        HomeSP.s                 = IPS_OK;
        IDSetSwitch(&HomeSP, "Filter set to HOME.");
        IDSetNumber(&FilterPositionNP, NULL);
    }
}

void ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n)
{
    ISInit();

    /* ignore if not ours */
    if (dev != nullptr && strcmp(mydev, dev))
        return;

    if (!strcmp(name, PortTP.name))
    {
        if (IUUpdateText(&PortTP, texts, names, n))
            return;

        PortTP.s = IPS_OK;
        IDSetText(&PortTP, NULL);
    }
}

void ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    long err;
    INumber *np;

    /* ignore if not ours */
    if (dev != nullptr && strcmp(dev, mydev))
        return;

    ISInit();

    if (!strcmp(FilterPositionNP.name, name))
    {
        if (!isFilterConnected())
        {
            IDMessage(mydev, "Filter is not connected.");
            FilterPositionNP.s = IPS_IDLE;
            IDSetNumber(&FilterPositionNP, NULL);
            return;
        }

        np = IUFindNumber(&FilterPositionNP, names[0]);
        if (np == &FilterPositionN[0])
        {
            targetFilter         = values[0];
            int nbytes           = 0;
            unsigned char type   = 0x01;
            unsigned char chksum = COMM_INIT + type + (unsigned char)targetFilter;
            /*char filter_command[5] = { COMM_PRE, COMM_INIT, type, targetFilter, chksum };*/
            unsigned char filter_command[CMD_SIZE];

            if (targetFilter < FilterPositionN[0].min || targetFilter > FilterPositionN[0].max)
            {
                FilterPositionNP.s = IPS_ALERT;
                IDSetNumber(&FilterPositionNP, "Error: valid range of filter is from %d to %d",
                            (int)FilterPositionN[0].min, (int)FilterPositionN[0].max);
                return;
            }

            IUUpdateNumber(&FilterPositionNP, values, names, n);

            //snprintf(filter_command, CMD_SIZE,  "%c%c%c%c%c", COMM_PRE, COMM_INIT, type, targetFilter, chksum);
            snprintf(filter_command, CMD_SIZE, "%c%c%c%c", COMM_INIT, type, targetFilter, chksum);

            if (DEBUG_ON)
            {
                IDLog("Filter Position Command (int): #%d#%d#%d#%d#\n", COMM_INIT, type, targetFilter, chksum);
                IDLog("Filter Position Command (char): #%c#%c#%c#%c#\n", COMM_INIT, type, targetFilter, chksum);
            }

            if (!SIMULATION_ON)
                err = tty_write(fd, filter_command, CMD_SIZE, &nbytes);

            FilterPositionNP.s = IPS_OK;
            IDSetNumber(&FilterPositionNP, "Setting current filter to slot %d", targetFilter);
        }
        else
        {
            FilterPositionNP.s = IPS_IDLE;
            IDSetNumber(&FilterPositionNP, NULL);
        }

        return;
    }

    if (!strcmp(FilterCountNP.name, name))
    {
        if (!isFilterConnected())
        {
            IDMessage(mydev, "Filter is not connected.");
            FilterCountNP.s = IPS_IDLE;
            IDSetNumber(&FilterCountNP, NULL);
            return;
        }

        np = IUFindNumber(&FilterCountNP, names[0]);
        if (np == &FilterCountN[0])
        {
            if (IUUpdateNumber(&FilterCountNP, values, names, n) < 0)
                return;

            FilterPositionN[0].min = 1;
            FilterPositionN[0].max = (int)FilterCountN[0].value;
            IUUpdateMinMax(&FilterPositionNP);

            FilterCountNP.s = IPS_OK;
            IDSetNumber(&FilterCountNP, "Updated number of available filters to %d", ((int)FilterCountN[0].value));
        }
        else
        {
            FilterCountNP.s = IPS_IDLE;
            IDSetNumber(&FilterCountNP, NULL);
        }

        return;
    }
}

int getOnSwitch(ISwitchVectorProperty *sp)
{
    int i = 0;
    for (i = 0; i < sp->nsp; i++)
    {
        if (sp->sp[i].s == ISS_ON)
            return i;
    }

    return -1;
}

int checkPowerS(ISwitchVectorProperty *sp)
{
    if (PowerSP.s != IPS_OK)
    {
        if (!strcmp(sp->label, ""))
            IDMessage(mydev, "Cannot change property %s while the wheel is offline.", sp->name);
        else
            IDMessage(mydev, "Cannot change property %s while the wheel is offline.", sp->label);

        sp->s = IPS_IDLE;
        IDSetSwitch(sp, NULL);
        return -1;
    }

    return 0;
}

int checkPowerN(INumberVectorProperty *np)
{
    if (PowerSP.s != IPS_OK)
    {
        if (!strcmp(np->label, ""))
            IDMessage(mydev, "Cannot change property %s while the wheel is offline.", np->name);
        else
            IDMessage(mydev, "Cannot change property %s while the wheel is offline.", np->label);

        np->s = IPS_IDLE;
        IDSetNumber(np, NULL);
        return -1;
    }

    return 0;
}

int checkPowerT(ITextVectorProperty *tp)
{
    if (PowerSP.s != IPS_OK)
    {
        if (!strcmp(tp->label, ""))
            IDMessage(mydev, "Cannot change property %s while the wheel is offline.", tp->name);
        else
            IDMessage(mydev, "Cannot change property %s while the wheel is offline.", tp->label);

        tp->s = IPS_IDLE;
        IDSetText(tp, NULL);
        return -1;
    }

    return 0;
}

void connectFilter()
{
    int err;
    char errmsg[ERRMSG_SIZE];

    switch (PowerS[0].s)
    {
        case ISS_ON:
            if (SIMULATION_ON)
            {
                PowerSP.s = IPS_OK;
                IDSetSwitch(&PowerSP, "Simulated Filter Wheel is online.");
                return;
            }

            if ((err = tty_connect(PortT[0].text, 9600, 8, 0, 1, &fd)) != TTY_OK)
            {
                PowerSP.s = IPS_ALERT;
                IDSetSwitch(
                    &PowerSP,
                    "Error: cannot connect to %s. Make sure the filter is connected and you have access to the port.",
                    PortT[0].text);
                tty_error_msg(err, errmsg, ERRMSG_SIZE);
                IDLog("Error: %s\n", errmsg);
                return;
            }

            PowerSP.s = IPS_OK;
            IDSetSwitch(&PowerSP, "Filter Wheel is online. True Technology filter wheel suffers from several bugs. "
                                  "Please refer to http://indi.sf.net/profiles/trutech.html for more details.");
            break;

        case ISS_OFF:
            if (SIMULATION_ON)
            {
                PowerSP.s = IPS_OK;
                IDSetSwitch(&PowerSP, "Simulated Filter Wheel is offline.");
                return;
            }

            if ((err = tty_disconnect(fd)) != TTY_OK)
            {
                PowerSP.s = IPS_ALERT;
                IDSetSwitch(&PowerSP, "Error: cannot disconnect.");
                tty_error_msg(err, errmsg, ERRMSG_SIZE);
                IDLog("Error: %s\n", errmsg);
                return;
            }

            PowerSP.s = IPS_IDLE;
            IDSetSwitch(&PowerSP, "Filter Wheel is offline.");
            break;
    }
}

/* isFilterConnected: return 1 if we have a connection, 0 otherwise */
int isFilterConnected(void)
{
    return ((PowerS[0].s == ISS_ON) ? 1 : 0);
}

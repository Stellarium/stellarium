#if 0
    FLI WHEEL
    INDI Interface for Finger Lakes Instruments Filter Wheels
    Copyright (C) 2005 Gaetano Vocca (yagvoc-web AT yahoo DOT it)
	Based on fli_ccd by Jasem Mutlaq (mutlaqja AT ikarustech DOT com)

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
#include <netinet/in.h>
#include <netdb.h>

#include "libfli.h"
#include "indidevapi.h"
#include "eventloop.h"
#include "indicom.h"

void ISInit(void);
void getBasicData(void);
void ISPoll(void *);
void handleExposure(void *);
void connectFilter(void);
int findwheel(flidomain_t domain);
int manageDefaults(char errmsg[]);
int checkPowerS(ISwitchVectorProperty *sp);
int checkPowerN(INumberVectorProperty *np);
int checkPowerT(ITextVectorProperty *tp);
int getOnSwitch(ISwitchVectorProperty *sp);
int isFilterConnected(void);

double min(void);
double max(void);

extern char *me;
extern int errno;

#define mydev "FLI Wheel"

#define MAIN_GROUP "Main Control"

#define LAST_FILTER  14 /* Max slot index */
#define FIRST_FILTER 0  /* Min slot index */

#define currentFilter FilterN[0].value

#define POLLMS     1000
#define LIBVERSIZ  1024
#define PREFIXSIZ  64
#define PIPEBUFSIZ 8192
#define FRAME_ILEN 64

typedef struct
{
    flidomain_t domain;
    char *dname;
    char *name;
    char *model;
    long HWRevision;
    long FWRevision;
    long current_filter;
    long filter_count;
    long home;
} cam_t;

static flidev_t fli_dev;
static cam_t *FLIWheel;
static int portSwitchIndex;
static int simulation;
static int targetFilter;

long int Domains[] = { FLIDOMAIN_USB, FLIDOMAIN_SERIAL, FLIDOMAIN_PARALLEL_PORT, FLIDOMAIN_INET };

/*INDI controls */

/* Connect/Disconnect */
static ISwitch PowerS[] = { { "CONNECT", "Connect", ISS_OFF, 0, 0 }, { "DISCONNECT", "Disconnect", ISS_ON, 0, 0 } };
static ISwitchVectorProperty PowerSP = { mydev, "CONNECTION", "Connection", MAIN_GROUP,     IP_RW, ISR_1OFMANY,
                                         60,    IPS_IDLE,     PowerS,       NARRAY(PowerS), "",    0 };

/* Types of Ports */
static ISwitch PortS[]              = { { "USB", "", ISS_ON, 0, 0 },
                           { "Serial", "", ISS_OFF, 0, 0 },
                           { "Parallel", "", ISS_OFF, 0, 0 },
                           { "INet", "", ISS_OFF, 0, 0 } };
static ISwitchVectorProperty PortSP = { mydev, "Port Type", "",    MAIN_GROUP,    IP_RW, ISR_1OFMANY,
                                        0,     IPS_IDLE,    PortS, NARRAY(PortS), "",    0 };

/* Filter control */
static INumber FilterN[] = { { "FILTER_SLOT_VALUE", "Active Filter", "%2.0f", FIRST_FILTER, LAST_FILTER, 1, 0, 0, 0,
                               0 } };
static INumberVectorProperty FilterNP = { mydev,    "FILTER_SLOT", "Filter",        MAIN_GROUP, IP_RW, 0,
                                          IPS_IDLE, FilterN,       NARRAY(FilterN), "",         0 };

/* send client definitions of all properties */
void ISInit()
{
    static int isInit = 0;

    if (isInit)
        return;

    /* USB by default {USB, SERIAL, PARALLEL, INET} */
    portSwitchIndex = 0;

    targetFilter = 0;

    /* No Simulation by default */
    simulation = 0;

    /* Enable the following for simulation mode */
    /*simulation = 1;
        IDLog("WARNING: Simulation is on\n");*/

    IEAddTimer(POLLMS, ISPoll, NULL);

    isInit = 1;
}

void ISGetProperties(const char *dev)
{
    ISInit();

    if (dev != nullptr && strcmp(mydev, dev))
        return;

    /* Main Control */
    IDDefSwitch(&PowerSP, NULL);
    IDDefSwitch(&PortSP, NULL);
    IDDefNumber(&FilterNP, NULL);
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

    /* Port type */
    if (!strcmp(name, PortSP.name))
    {
        PortSP.s = IPS_IDLE;
        IUResetSwitch(&PortSP);
        IUUpdateSwitch(&PortSP, states, names, n);
        portSwitchIndex = getOnSwitch(&PortSP);

        PortSP.s = IPS_OK;
        IDSetSwitch(&PortSP, NULL);
        return;
    }

    /* Connection */
    if (!strcmp(name, PowerSP.name))
    {
        IUResetSwitch(&PowerSP);
        IUUpdateSwitch(&PowerSP, states, names, n);
        connectFilter();
        return;
    }
}

void ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n)
{
    ISInit();

    /* ignore if not ours */
    if (dev != nullptr && strcmp(mydev, dev))
        return;

    /* suppress warning */
    n     = n;
    dev   = dev;
    name  = name;
    names = names;
    texts = texts;
}

void ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    long err;
    INumber *np;
    long newFilter;

    n = n;

    /* ignore if not ours */
    if (dev != nullptr && strcmp(dev, mydev))
        return;

    ISInit();

    if (!strcmp(FilterNP.name, name))
    {
        if (simulation)
        {
            targetFilter = values[0];
            FilterNP.s   = IPS_BUSY;
            IDSetNumber(&FilterNP, "Setting current filter to slot %d", targetFilter);
            IDLog("Setting current filter to slot %d\n", targetFilter);
            return;
        }

        if (!isFilterConnected())
        {
            IDMessage(mydev, "Device not connected.");
            FilterNP.s = IPS_IDLE;
            IDSetNumber(&FilterNP, NULL);
            return;
        }

        targetFilter = values[0];

        np = IUFindNumber(&FilterNP, names[0]);

        if (!np)
        {
            FilterNP.s = IPS_ALERT;
            IDSetNumber(&FilterNP, "Unknown error. %s is not a member of %s property.", names[0], name);
            return;
        }

        if (targetFilter < FIRST_FILTER || targetFilter > FLIWheel->filter_count - 1)
        {
            FilterNP.s = IPS_ALERT;
            IDSetNumber(&FilterNP, "Error: valid range of filter is from %d to %d", FIRST_FILTER, LAST_FILTER);
            return;
        }

        FilterNP.s = IPS_BUSY;
        IDSetNumber(&FilterNP, "Setting current filter to slot %d", targetFilter);
        IDLog("Setting current filter to slot %d\n", targetFilter);

        if ((err = FLISetFilterPos(fli_dev, targetFilter)))
        {
            FilterNP.s = IPS_ALERT;
            IDSetNumber(&FilterNP, "FLISetFilterPos() failed. %s.", strerror((int)-err));
            IDLog("FLISetFilterPos() failed. %s.", strerror((int)-err));
            return;
        }

        /* Check current filter position */
        if ((err = FLIGetFilterPos(fli_dev, &newFilter)))
        {
            FilterNP.s = IPS_ALERT;
            IDSetNumber(&FilterNP, "FLIGetFilterPos() failed. %s.", strerror((int)-err));
            IDLog("FLIGetFilterPos() failed. %s.\n", strerror((int)-err));
            return;
        }

        if (newFilter == targetFilter)
        {
            FLIWheel->current_filter = targetFilter;
            FilterN[0].value         = FLIWheel->current_filter;
            FilterNP.s               = IPS_OK;
            IDSetNumber(&FilterNP, "Filter set to slot #%d", targetFilter);
            return;
        }

        return;
    }
}

/* Retrieves basic data from the Wheel upon connection like temperature, array size, firmware..etc */
void getBasicData()
{
    char buff[2048];
    long err;

    if ((err = FLIGetModel(fli_dev, buff, 2048)))
    {
        IDMessage(mydev, "FLIGetModel() failed. %s.", strerror((int)-err));
        IDLog("FLIGetModel() failed. %s.\n", strerror((int)-err));
        return;
    }
    else
    {
        if ((FLIWheel->model = malloc(sizeof(char) * 2048)) == NULL)
        {
            IDMessage(mydev, "malloc() failed.");
            IDLog("malloc() failed.");
            return;
        }

        strcpy(FLIWheel->model, buff);
    }

    if ((err = FLIGetHWRevision(fli_dev, &FLIWheel->HWRevision)))
    {
        IDMessage(mydev, "FLIGetHWRevision() failed. %s.", strerror((int)-err));
        IDLog("FLIGetHWRevision() failed. %s.\n", strerror((int)-err));

        return;
    }

    if ((err = FLIGetFWRevision(fli_dev, &FLIWheel->FWRevision)))
    {
        IDMessage(mydev, "FLIGetFWRevision() failed. %s.", strerror((int)-err));
        IDLog("FLIGetFWRevision() failed. %s.\n", strerror((int)-err));
        return;
    }

    if ((err = FLIGetFilterCount(fli_dev, &FLIWheel->filter_count)))
    {
        IDMessage(mydev, "FLIGetFilterCount() failed. %s.", strerror((int)-err));
        IDLog("FLIGetFilterCount() failed. %s.\n", strerror((int)-err));
        return;
    }

    IDLog("The filter count is %ld\n", FLIWheel->filter_count);

    FilterN[0].max = FLIWheel->filter_count - 1;
    FilterNP.s     = IPS_OK;

    IUUpdateMinMax(&FilterNP);
    IDSetNumber(&FilterNP, "Setting basic data");

    IDLog("Exiting getBasicData()\n");
}

int manageDefaults(char errmsg[])
{
    long err;

    /*IDLog("Resetting filter wheel to slot %d\n", 0);
	FLIWheel->home = 0;
	if (( err = FLISetFilterPos(fli_dev, 0)))
	{
		IDMessage(mydev, "FLISetFilterPos() failed. %s.", strerror((int)-err));
		IDLog("FLISetFilterPos() failed. %s.\n", strerror((int)-err));
		return (int)-err;
	}*/

    if ((err = FLIGetFilterPos(fli_dev, &FLIWheel->current_filter)))
    {
        IDMessage(mydev, "FLIGetFilterPos() failed. %s.", strerror((int)-err));
        IDLog("FLIGetFilterPos() failed. %s.\n", strerror((int)-err));
        return (int)-err;
    }

    IDLog("The current filter is %ld\n", FLIWheel->current_filter);

    FilterN[0].value = FLIWheel->current_filter;
    IDSetNumber(&FilterNP, "Storing defaults");

    /* Success */
    return 0;
}

void ISPoll(void *p)
{
    static int simMTC = 5;

    if (!isFilterConnected())
    {
        IEAddTimer(POLLMS, ISPoll, NULL);
        return;
    }

    switch (FilterNP.s)
    {
        case IPS_IDLE:
        case IPS_OK:
            break;

        case IPS_BUSY:
            /* Simulate that it takes 5 seconds to change slot */
            if (simulation)
            {
                simMTC--;
                if (simMTC == 0)
                {
                    simMTC        = 5;
                    currentFilter = targetFilter;
                    FilterNP.s    = IPS_OK;
                    IDSetNumber(&FilterNP, "Filter set to slot #%2.0f", currentFilter);
                    break;
                }
                IDSetNumber(&FilterNP, NULL);
                break;
            }

            /*if (( err = FLIGetFilterPos(fli_dev, &currentFilter)))
	{
                FilterNP.s = IPS_ALERT;
		IDSetNumber(&FilterNP, "FLIGetFilterPos() failed. %s.", strerror((int)-err));
		IDLog("FLIGetFilterPos() failed. %s.\n", strerror((int)-err));
		return;
	}

        if (targetFilter == currentFilter)
        {
		FLIWheel->current_filter = currentFilter;
		FilterNP.s = IPS_OK;
		IDSetNumber(&FilterNP, "Filter set to slot #%2.0f", currentFilter);
                return;
        }
       
        IDSetNumber(&FilterNP, NULL);*/
            break;

        case IPS_ALERT:
            break;
    }

    IEAddTimer(POLLMS, ISPoll, NULL);
}

int getOnSwitch(ISwitchVectorProperty *sp)
{
    int i = 0;
    for (i = 0; i < sp->nsp; i++)
    {
        /*IDLog("Switch %s is %s\n", sp->sp[i].name, sp->sp[i].s == ISS_ON ? "On" : "Off");*/
        if (sp->sp[i].s == ISS_ON)
            return i;
    }

    return -1;
}

int checkPowerS(ISwitchVectorProperty *sp)
{
    if (simulation)
        return 0;

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
    if (simulation)
        return 0;

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
    if (simulation)
        return 0;

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
    long err;
    char errmsg[ERRMSG_SIZE];

    /* USB by default {USB, SERIAL, PARALLEL, INET} */
    switch (PowerS[0].s)
    {
        case ISS_ON:

            if (simulation)
            {
                /* Success! */
                PowerS[0].s = ISS_ON;
                PowerS[1].s = ISS_OFF;
                PowerSP.s   = IPS_OK;
                IDSetSwitch(&PowerSP, "Simulation Wheel is online.");
                IDLog("Simulation Wheel is online.\n");
                return;
            }

            IDLog("Current portSwitch is %d\n", portSwitchIndex);
            IDLog("Attempting to find the device in domain %ld\n", Domains[portSwitchIndex]);

            if (findwheel(Domains[portSwitchIndex]))
            {
                PowerSP.s   = IPS_IDLE;
                PowerS[0].s = ISS_OFF;
                PowerS[1].s = ISS_ON;
                IDSetSwitch(&PowerSP, "Error: no wheels were detected.");
                IDLog("Error: no wheels were detected.\n");
                return;
            }

            if ((err = FLIOpen(&fli_dev, FLIWheel->name, FLIWheel->domain | FLIDEVICE_FILTERWHEEL)))
            {
                PowerSP.s   = IPS_IDLE;
                PowerS[0].s = ISS_OFF;
                PowerS[1].s = ISS_ON;
                IDSetSwitch(&PowerSP, "Error: FLIOpen() failed. %s.", strerror((int)-err));
                IDLog("Error: FLIOpen() failed. %s.\n", strerror((int)-err));
                return;
            }

            /* Success! */
            PowerS[0].s = ISS_ON;
            PowerS[1].s = ISS_OFF;
            PowerSP.s   = IPS_OK;
            IDSetSwitch(&PowerSP, "Wheel is online. Retrieving basic data.");
            IDLog("Wheel is online. Retrieving basic data.\n");
            getBasicData();

            if (manageDefaults(errmsg))
            {
                IDMessage(mydev, errmsg, NULL);
                IDLog("%s", errmsg);
                return;
            }

            break;

        case ISS_OFF:

            if (simulation)
            {
                PowerS[0].s = ISS_OFF;
                PowerS[1].s = ISS_ON;
                PowerSP.s   = IPS_IDLE;
                IDSetSwitch(&PowerSP, "Wheel is offline.");
                return;
            }

            PowerS[0].s = ISS_OFF;
            PowerS[1].s = ISS_ON;
            PowerSP.s   = IPS_IDLE;
            if ((err = FLIClose(fli_dev)))
            {
                PowerSP.s = IPS_ALERT;
                IDSetSwitch(&PowerSP, "Error: FLIClose() failed. %s.", strerror((int)-err));
                IDLog("Error: FLIClose() failed. %s.\n", strerror((int)-err));
                return;
            }
            IDSetSwitch(&PowerSP, "Wheel is offline.");
            break;
    }
}

/* isFilterConnected: return 1 if we have a connection, 0 otherwise */
int isFilterConnected(void)
{
    if (simulation)
        return 1;

    return ((PowerS[0].s == ISS_ON) ? 1 : 0);
}

int findwheel(flidomain_t domain)
{
    char **devlist;
    long err;

    IDLog("In find Camera, the domain is %ld\n", domain);

    if ((err = FLIList(domain | FLIDEVICE_FILTERWHEEL, &devlist)))
    {
        IDLog("FLIList() failed. %s\n", strerror((int)-err));
        return -1;
    }

    if (devlist != NULL && devlist[0] != NULL)
    {
        int i;

        IDLog("Trying to allocate memory to FLIWheel\n");
        if ((FLIWheel = malloc(sizeof(cam_t))) == NULL)
        {
            IDLog("malloc() failed.\n");
            return -1;
        }

        for (i = 0; devlist[i] != NULL; i++)
        {
            int j;

            for (j = 0; devlist[i][j] != '\0'; j++)
                if (devlist[i][j] == ';')
                {
                    devlist[i][j] = '\0';
                    break;
                }
        }

        FLIWheel->domain = domain;

        /* Each driver handles _only_ one camera for now */
        switch (domain)
        {
            case FLIDOMAIN_PARALLEL_PORT:
                FLIWheel->dname = strdup("parallel port");
                break;

            case FLIDOMAIN_USB:
                FLIWheel->dname = strdup("USB");
                break;

            case FLIDOMAIN_SERIAL:
                FLIWheel->dname = strdup("serial");
                break;

            case FLIDOMAIN_INET:
                FLIWheel->dname = strdup("inet");
                break;

            default:
                FLIWheel->dname = strdup("Unknown domain");
        }

        IDLog("Domain set OK\n");

        FLIWheel->name = strdup(devlist[0]);

        if ((err = FLIFreeList(devlist)))
        {
            IDLog("FLIFreeList() failed. %s.\n", strerror((int)-err));
            return -1;
        }

    } /* end if */
    else
    {
        if ((err = FLIFreeList(devlist)))
        {
            IDLog("FLIFreeList() failed. %s.\n", strerror((int)-err));
            return -1;
        }

        return -1;
    }

    IDLog("Findcam() finished successfully.\n");
    return 0;
}

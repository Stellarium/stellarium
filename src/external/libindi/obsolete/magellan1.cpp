#if 0
MAGELLAN Generic
Copyright (C) 2011 Onno Hommes (ohommes@alumni.cmu.edu)

This library is free software;
you can redistribute it and / or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation;
either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY;
without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library;
if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110 - 1301  USA

#endif

#include "magellan1.h"

#include "magellandriver.h"

Magellan1 *telescope = nullptr;

extern char *me;

#define COMM_GROUP  "Communication"
#define BASIC_GROUP "Position"

#define MAGELLAN_TRACK 0
#define MAGELLAN_SYNC  1

/* Handy Macros */
#define currentRA  EquatorialCoordsRN[0].value
#define currentDEC EquatorialCoordsRN[1].value

static void ISPoll(void *);
static void retryConnection(void *);

/*INDI Propertries */

/**********************************************************************************************/
/************************************ GROUP: Communication ************************************/
/**********************************************************************************************/

/********************************************
 Property: Connection
*********************************************/
static ISwitch ConnectS[] = { { "CONNECT", "Connect", ISS_OFF, 0, 0 }, { "DISCONNECT", "Disconnect", ISS_ON, 0, 0 } };
ISwitchVectorProperty ConnectSP = { mydev, "CONNECTION", "Connection", COMM_GROUP,       IP_RW, ISR_1OFMANY,
                                    0,     IPS_IDLE,     ConnectS,     NARRAY(ConnectS), "",    0 };

/********************************************
 Property: Device Port
*********************************************/
/*wildi removed static */
static IText PortT[]       = { { "PORT", "Port", 0, 0, 0, 0 } };
ITextVectorProperty PortTP = { mydev,    "DEVICE_PORT", "Ports",       COMM_GROUP, IP_RW, 0,
                               IPS_IDLE, PortT,         NARRAY(PortT), "",         0 };

/**********************************************************************************************/
/************************************ GROUP: Position Display**********************************/
/**********************************************************************************************/

/********************************************
 Property: Equatorial Coordinates JNow
 Perm: RO
*********************************************/
INumber EquatorialCoordsRN[]              = { { "RA", "RA  H:M:S", "%10.6m", 0., 24., 0., 0., 0, 0, 0 },
                                 { "DEC", "Dec D:M:S", "%10.6m", -90., 90., 0., 0., 0, 0, 0 } };
INumberVectorProperty EquatorialCoordsRNP = {
    mydev,    "EQUATORIAL_EOD_COORD", "Equatorial JNow",          BASIC_GROUP, IP_RO, 120,
    IPS_IDLE, EquatorialCoordsRN,     NARRAY(EquatorialCoordsRN), "",          0
};

/*****************************************************************************************************/
/**************************************** END PROPERTIES *********************************************/
/*****************************************************************************************************/

/* send client definitions of all properties */
void ISInit()
{
    static int isInit = 0;

    if (isInit)
        return;
    if (telescope == nullptr)
    {
        IUSaveText(&PortT[0], "/dev/ttyS0");
        telescope = new Magellan1();
        telescope->setCurrentDeviceName(mydev);
    }

    isInit = 1;
    IEAddTimer(POLLMS, ISPoll, nullptr);
}

void ISGetProperties(const char *dev)
{
    ISInit();
    telescope->ISGetProperties(dev);
}

void ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    ISInit();
    telescope->ISNewSwitch(dev, name, states, names, n);
}

void ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n)
{
    ISInit();
    telescope->ISNewText(dev, name, texts, names, n);
}

void ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    ISInit();
    telescope->ISNewNumber(dev, name, values, names, n);
}

void ISPoll(void *p)
{
    telescope->ISPoll();
    IEAddTimer(POLLMS, ISPoll, nullptr);
    p = p;
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
    telescope->ISSnoopDevice(root);
}

/**************************************************
*** MAGELLAN1 Implementation
***************************************************/

Magellan1::Magellan1()
{
    currentSiteNum = 1;
    trackingMode   = MAGELLAN_TRACK_DEFAULT;
    lastSet        = -1;
    fault          = false;
    simulation     = false;
    currentSet     = 0;
    fd             = -1;

    // Children call parent routines, this is the default
    IDLog("Initializing from MAGELLAN device...\n");
    IDLog("Driver Version: 2011-07-28\n");
}

Magellan1::~Magellan1()
{
}

void Magellan1::setCurrentDeviceName(const char *devName)
{
    strcpy(thisDevice, devName);
}

void Magellan1::ISGetProperties(const char *dev)
{
    if (dev != nullptr && strcmp(thisDevice, dev))
        return;

    // COMM_GROUP
    IDDefSwitch(&ConnectSP, nullptr);
    IDDefText(&PortTP, nullptr);

    // POSITION_GROUP
    IDDefNumber(&EquatorialCoordsRNP, nullptr);

    /* Send the basic data to the new client if the previous client(s) are already connected. */
    if (ConnectSP.s == IPS_OK)
        getBasicData();
}

void Magellan1::ISSnoopDevice(XMLEle *root)
{
    INDI_UNUSED(root);
}

void Magellan1::ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n)
{
    IText *tp;

    /* Ignore other devices */
    if (strcmp(dev, thisDevice))
        return;

    /* See if the port is updated */
    if (!strcmp(name, PortTP.name))
    {
        PortTP.s = IPS_OK;
        tp       = IUFindText(&PortTP, names[0]);
        if (!tp)
            return;

        IUSaveText(&PortTP.tp[0], texts[0]);
        IDSetText(&PortTP, nullptr);
        return;
    }
}

void Magellan1::ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    INDI_UNUSED(dev);
    INDI_UNUSED(name);
    INDI_UNUSED(values);
    INDI_UNUSED(names);
    INDI_UNUSED(n);
}

void Magellan1::ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    INDI_UNUSED(names);

    /* Ignore other devices */
    if (strcmp(thisDevice, dev))
        return;

    // FIRST Switch ALWAYS for Connection
    if (!strcmp(name, ConnectSP.name))
    {
        bool connectionEstablished = (ConnectS[0].s == ISS_ON);
        if (IUUpdateSwitch(&ConnectSP, states, names, n) < 0)
            return;
        if ((connectionEstablished && ConnectS[0].s == ISS_ON) || (!connectionEstablished && ConnectS[1].s == ISS_ON))
        {
            ConnectSP.s = IPS_OK;
            IDSetSwitch(&ConnectSP, nullptr);
            return;
        }
        connectTelescope();
        return;
    }
}

void Magellan1::handleError(ISwitchVectorProperty *svp, int err, const char *msg)
{
    svp->s = IPS_ALERT;

    /* First check to see if the telescope is connected */
    if (check_magellan_connection(fd))
    {
        /* The telescope is off locally */
        ConnectS[0].s = ISS_OFF;
        ConnectS[1].s = ISS_ON;
        ConnectSP.s   = IPS_BUSY;
        IDSetSwitch(&ConnectSP, "Telescope is not responding to commands, will retry in 10 seconds.");

        IDSetSwitch(svp, nullptr);
        IEAddTimer(10000, retryConnection, &fd);
        return;
    }

    /* If the error is a time out, then the device doesn't support this property or busy*/
    if (err == -2)
    {
        svp->s = IPS_ALERT;
        IDSetSwitch(svp, "Device timed out. Current device may be busy or does not support %s. Will retry again.", msg);
    }
    else
        /* Changing property failed, user should retry. */
        IDSetSwitch(svp, "%s failed.", msg);

    fault = true;
}

void Magellan1::handleError(INumberVectorProperty *nvp, int err, const char *msg)
{
    nvp->s = IPS_ALERT;

    /* First check to see if the telescope is connected */
    if (check_magellan_connection(fd))
    {
        /* The telescope is off locally */
        ConnectS[0].s = ISS_OFF;
        ConnectS[1].s = ISS_ON;
        ConnectSP.s   = IPS_BUSY;
        IDSetSwitch(&ConnectSP, "Telescope is not responding to commands, will retry in 10 seconds.");

        IDSetNumber(nvp, nullptr);
        IEAddTimer(10000, retryConnection, &fd);
        return;
    }

    /* If the error is a time out, then the device doesn't support this property */
    if (err == -2)
    {
        nvp->s = IPS_ALERT;
        IDSetNumber(nvp, "Device timed out. Current device may be busy or does not support %s. Will retry again.", msg);
    }
    else
        /* Changing property failed, user should retry. */
        IDSetNumber(nvp, "%s failed.", msg);

    fault = true;
}

void Magellan1::handleError(ITextVectorProperty *tvp, int err, const char *msg)
{
    tvp->s = IPS_ALERT;

    /* First check to see if the telescope is connected */
    if (check_magellan_connection(fd))
    {
        /* The telescope is off locally */
        ConnectS[0].s = ISS_OFF;
        ConnectS[1].s = ISS_ON;
        ConnectSP.s   = IPS_BUSY;
        IDSetSwitch(&ConnectSP, "Telescope is not responding to commands, will retry in 10 seconds.");

        IDSetText(tvp, nullptr);
        IEAddTimer(10000, retryConnection, &fd);
        return;
    }

    /* If the error is a time out, then the device doesn't support this property */
    if (err == -2)
    {
        tvp->s = IPS_ALERT;
        IDSetText(tvp, "Device timed out. Current device may be busy or does not support %s. Will retry again.", msg);
    }

    else
    {
        /* Changing property failed, user should retry. */
        IDSetText(tvp, "%s failed.", msg);
    }

    fault = true;
}

void Magellan1::correctFault()
{
    fault = false;
    IDMessage(thisDevice, "Telescope is online.");
}

bool Magellan1::isTelescopeOn()
{
    return (ConnectSP.sp[0].s == ISS_ON);
}

static void retryConnection(void *p)
{
    int fd = *((int *)p);

    if (check_magellan_connection(fd))
    {
        ConnectSP.s = IPS_IDLE;
        IDSetSwitch(&ConnectSP, "The connection to the telescope is lost.");
        return;
    }

    ConnectS[0].s = ISS_ON;
    ConnectS[1].s = ISS_OFF;
    ConnectSP.s   = IPS_OK;

    IDSetSwitch(&ConnectSP, "The connection to the telescope has been resumed.");
}

void Magellan1::ISPoll()
{
    int err = 0;

    if (!isTelescopeOn())
        return;

    if ((err = getMAGELLANRA(fd, &currentRA)) < 0 || (err = getMAGELLANDEC(fd, &currentDEC)) < 0)
    {
        EquatorialCoordsRNP.s = IPS_ALERT;
        IDSetNumber(&EquatorialCoordsRNP, nullptr);
        handleError(&EquatorialCoordsRNP, err, "Getting RA/DEC");
        return;
    }

    if (fault)
        correctFault();

    EquatorialCoordsRNP.s = IPS_OK;

    lastRA  = currentRA;
    lastDEC = currentDEC;
    IDSetNumber(&EquatorialCoordsRNP, nullptr);
}

void Magellan1::getBasicData()
{
    char calendarDate[32];
    int err;

    /* Magellan 1 Get Calendar Date As a Test (always 1/1/96) */
    if ((err = getCalendarDate(fd, calendarDate)) < 0)
        IDMessage(thisDevice, "Failed to retrieve calendar date from device.");
    else
        IDMessage(thisDevice, "Successfully retrieved calendar date from device.");

    /* Only 24 Time format on Magellan and you cant get to the local time
       which you can set in Magellan I */
    timeFormat = MAGELLAN_24;

    if ((err = getMAGELLANRA(fd, &targetRA)) < 0 || (err = getMAGELLANDEC(fd, &targetDEC)) < 0)
    {
        EquatorialCoordsRNP.s = IPS_ALERT;
        IDSetNumber(&EquatorialCoordsRNP, nullptr);
        handleError(&EquatorialCoordsRNP, err, "Getting RA/DEC");
        return;
    }

    if (fault)
        correctFault();

    EquatorialCoordsRNP.np[0].value = targetRA;
    EquatorialCoordsRNP.np[1].value = targetDEC;

    EquatorialCoordsRNP.s = IPS_OK;
    IDSetNumber(&EquatorialCoordsRNP, nullptr);
}

void Magellan1::connectTelescope()
{
    switch (ConnectSP.sp[0].s)
    {
        case ISS_ON:

            /* Magellan I only has 1200 buad, 8 data bits, 0 parity and 1 stop bit */
            if (tty_connect(PortTP.tp[0].text, 1200, 8, 0, 1, &fd) != TTY_OK)
            {
                ConnectS[0].s = ISS_OFF;
                ConnectS[1].s = ISS_ON;
                IDSetSwitch(
                    &ConnectSP,
                    "Error connecting to port %s. Make sure you have BOTH write and read permission to your port.\n",
                    PortTP.tp[0].text);
                return;
            }
            if (check_magellan_connection(fd))
            {
                ConnectS[0].s = ISS_OFF;
                ConnectS[1].s = ISS_ON;
                IDSetSwitch(&ConnectSP, "Error connecting to Telescope. Telescope is offline.");
                return;
            }

#ifdef INDI_DEBUG
            IDLog("Telescope test successful.\n");
#endif

            ConnectSP.s = IPS_OK;
            IDSetSwitch(&ConnectSP, "Telescope is online. Retrieving basic data...");
            getBasicData();
            break;

        case ISS_OFF:
            ConnectS[0].s = ISS_OFF;
            ConnectS[1].s = ISS_ON;
            ConnectSP.s   = IPS_IDLE;
            IDSetSwitch(&ConnectSP, "Telescope is offline.");
            IDLog("Telescope is offline.");
            tty_disconnect(fd);
            break;
    }
}

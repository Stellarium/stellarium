#if 0
IEQ45 Basic Driver
Copyright (C) 2011 Nacho Mas (mas.ignacio@gmail.com). Only litle changes
from lx200basic made it by Jasem Mutlaq (mutlaqja@ikarustech.com)

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

#include "ieq45.h"

#include "config.h"
#include "ieq45driver.h"
#include "indicom.h"

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <unistd.h>


#include <memory>

/* Our telescope auto pointer */
std::unique_ptr<IEQ45Basic> telescope(new IEQ45Basic());

const int POLLMS  = 100;     // Period of update, 1 second.
const char *mydev = "IEQ45"; // Name of our device.

const char *BASIC_GROUP   = "Main Control"; // Main Group
const char *OPTIONS_GROUP = "Options";      // Options Group

/* Handy Macros */
#define currentRA  EquatorialCoordsRN[0].value
#define currentDEC EquatorialCoordsRN[1].value

static void ISPoll(void *);
static void retry_connection(void *);

/**************************************************************************************
** Send client definitions of all properties.
***************************************************************************************/
void ISInit()
{
    static int isInit = 0;

    if (isInit)
        return;

    if (telescope.get() == 0)
        telescope.reset(new IEQ45Basic());

    isInit = 1;

    IEAddTimer(POLLMS, ISPoll, nullptr);
}

/**************************************************************************************
**
***************************************************************************************/
void ISGetProperties(const char *dev)
{
    ISInit();
    telescope->ISGetProperties(dev);
}

/**************************************************************************************
**
***************************************************************************************/
void ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    ISInit();
    telescope->ISNewSwitch(dev, name, states, names, n);
}

/**************************************************************************************
**
***************************************************************************************/
void ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n)
{
    ISInit();
    telescope->ISNewText(dev, name, texts, names, n);
}

/**************************************************************************************
**
***************************************************************************************/
void ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    ISInit();
    telescope->ISNewNumber(dev, name, values, names, n);
}

/**************************************************************************************
**
***************************************************************************************/
void ISPoll(void *p)
{
    INDI_UNUSED(p);

    telescope->ISPoll();
    IEAddTimer(POLLMS, ISPoll, nullptr);
}

/**************************************************************************************
**
***************************************************************************************/
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

/**************************************************************************************
**
***************************************************************************************/
void ISSnoopDevice(XMLEle *root)
{
    INDI_UNUSED(root);
}

/**************************************************************************************
** IEQ45 Basic constructor
***************************************************************************************/
IEQ45Basic::IEQ45Basic()
{
    init_properties();

    lastSet    = -1;
    fd         = -1;
    simulation = false;
    lastRA     = 0;
    lastDEC    = 0;
    currentSet = 0;

    IDLog("Initializing from IEQ45 device...\n");
    IDLog("Driver Version: 0.1 (2011-11-07)\n");

    enable_simulation(false);
}

/**************************************************************************************
**
***************************************************************************************/
IEQ45Basic::~IEQ45Basic()
{
}

/**************************************************************************************
** Initialize all properties & set default values.
***************************************************************************************/
void IEQ45Basic::init_properties()
{
    // Connection#include <stdio.h>
#include <string.h>
    IUFillSwitch(&ConnectS[0], "CONNECT", "Connect", ISS_OFF);
    IUFillSwitch(&ConnectS[1], "DISCONNECT", "Disconnect", ISS_ON);
    IUFillSwitchVector(&ConnectSP, ConnectS, NARRAY(ConnectS), mydev, "CONNECTION", "Connection", BASIC_GROUP, IP_RW,
                       ISR_1OFMANY, 60, IPS_IDLE);

    // Coord Set
    IUFillSwitch(&OnCoordSetS[0], "SLEW", "Slew", ISS_ON);
    IUFillSwitch(&OnCoordSetS[1], "SYNC", "Sync", ISS_OFF);
    IUFillSwitchVector(&OnCoordSetSP, OnCoordSetS, NARRAY(OnCoordSetS), mydev, "ON_COORD_SET", "On Set", BASIC_GROUP,
                       IP_RW, ISR_1OFMANY, 0, IPS_IDLE);

    //Track MODE
    IUFillSwitch(&TrackModeS[0], "SIDEREAL", "Sidereal", ISS_ON);
    IUFillSwitch(&TrackModeS[1], "LUNAR", "Lunar", ISS_OFF);
    IUFillSwitch(&TrackModeS[2], "SOLAR", "Solar", ISS_OFF);
    IUFillSwitch(&TrackModeS[3], "ZERO", "Stop", ISS_OFF);
    IUFillSwitchVector(&TrackModeSP, TrackModeS, NARRAY(TrackModeS), mydev, "TRACK_MODE", "Track Mode", BASIC_GROUP,
                       IP_RW, ISR_1OFMANY, 0, IPS_IDLE);

    // Abort
    IUFillSwitch(&AbortSlewS[0], "ABORT", "Abort", ISS_OFF);
    IUFillSwitchVector(&AbortSlewSP, AbortSlewS, NARRAY(AbortSlewS), mydev, "ABORT_MOTION", "Abort Slew/Track",
                       BASIC_GROUP, IP_RW, ISR_ATMOST1, 0, IPS_IDLE);

    // Port
    IUFillText(&PortT[0], "PORT", "Port", "/dev/ttyS0");
    IUFillTextVector(&PortTP, PortT, NARRAY(PortT), mydev, "DEVICE_PORT", "Ports", BASIC_GROUP, IP_RW, 0, IPS_IDLE);

    // Equatorial Coords
    IUFillNumber(&EquatorialCoordsRN[0], "RA", "RA  H:M:S", "%10.6m", 0., 24., 0., 0.);
    IUFillNumber(&EquatorialCoordsRN[1], "DEC", "Dec D:M:S", "%10.6m", -90., 90., 0., 0.);
    IUFillNumberVector(&EquatorialCoordsRNP, EquatorialCoordsRN, NARRAY(EquatorialCoordsRN), mydev,
                       "EQUATORIAL_EOD_COORD", "Equatorial JNow", BASIC_GROUP, IP_RW, 0, IPS_IDLE);
}

/**************************************************************************************
** Define IEQ45 Basic properties to clients.
***************************************************************************************/
void IEQ45Basic::ISGetProperties(const char *dev)
{
    if (dev != nullptr && strcmp(mydev, dev))
        return;

    // Main Control
    IDDefSwitch(&ConnectSP, nullptr);
    IDDefText(&PortTP, nullptr);
    IDDefNumber(&EquatorialCoordsRNP, nullptr);
    IDDefSwitch(&OnCoordSetSP, nullptr);
    IDDefSwitch(&TrackModeSP, nullptr);
    IDDefSwitch(&AbortSlewSP, nullptr);
}

/**************************************************************************************
** Process Text properties
***************************************************************************************/
void IEQ45Basic::ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n)
{
    // Ignore if not ours
    if (strcmp(dev, mydev))
        return;

    // ===================================
    // Port Name
    // ===================================
    if (!strcmp(name, PortTP.name))
    {
        if (IUUpdateText(&PortTP, texts, names, n) < 0)
            return;

        PortTP.s = IPS_OK;
        IDSetText(&PortTP, nullptr);
        return;
    }

    if (is_connected() == false)
    {
        IDMessage(mydev, "IEQ45 is offline. Please connect before issuing any commands.");
        reset_all_properties();
        return;
    }
}

/**************************************************************************************
**
***************************************************************************************/
void IEQ45Basic::ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    // Ignore if not ours
    if (strcmp(dev, mydev))
        return;

    if (is_connected() == false)
    {
        IDMessage(mydev, "IEQ45 is offline. Please connect before issuing any commands.");
        reset_all_properties();
        return;
    }

    // ===================================
    // Equatorial Coords
    // ===================================
    if (!strcmp(name, EquatorialCoordsRNP.name))
    {
        int i = 0, nset = 0, error_code = 0;
        double newRA = 0, newDEC = 0;

        for (nset = i = 0; i < n; i++)
        {
            INumber *eqp = IUFindNumber(&EquatorialCoordsRNP, names[i]);
            if (eqp == &EquatorialCoordsRN[0])
            {
                newRA = values[i];
                nset += newRA >= 0 && newRA <= 24.0;
            }
            else if (eqp == &EquatorialCoordsRN[1])
            {
                newDEC = values[i];
                nset += newDEC >= -90.0 && newDEC <= 90.0;
            }
        }

        if (nset == 2)
        {
            char RAStr[32], DecStr[32];

            fs_sexa(RAStr, newRA, 2, 3600);
            fs_sexa(DecStr, newDEC, 2, 3600);

#ifdef INDI_DEBUG
            IDLog("We received JNow RA %g - DEC %g\n", newRA, newDEC);
            IDLog("We received JNow RA %s - DEC %s\n", RAStr, DecStr);
#endif

            if (!simulation &&
                ((error_code = setObjectRA(fd, newRA)) < 0 || (error_code = setObjectDEC(fd, newDEC)) < 0))
            {
                handle_error(&EquatorialCoordsRNP, error_code, "Setting RA/DEC");
                return;
            }

            targetRA  = newRA;
            targetDEC = newDEC;

            if (process_coords() == false)
            {
                EquatorialCoordsRNP.s = IPS_ALERT;
                IDSetNumber(&EquatorialCoordsRNP, nullptr);
            }
        } // end nset
        else
        {
            EquatorialCoordsRNP.s = IPS_ALERT;
            IDSetNumber(&EquatorialCoordsRNP, "RA or Dec missing or invalid");
        }

        return;
    } /* end EquatorialCoordsWNP */
}

/**************************************************************************************
**
***************************************************************************************/
void IEQ45Basic::ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    // ignore if not ours //
    if (strcmp(mydev, dev))
        return;

    // ===================================
    // Connect Switch
    // ===================================
    if (!strcmp(name, ConnectSP.name))
    {
        if (IUUpdateSwitch(&ConnectSP, states, names, n) < 0)
            return;

        connect_telescope();
        return;
    }

    if (is_connected() == false)
    {
        IDMessage(mydev, "IEQ45 is offline. Please connect before issuing any commands.");
        reset_all_properties();
        return;
    }

    // ===================================
    // Coordinate Set
    // ===================================
    if (!strcmp(name, OnCoordSetSP.name))
    {
        if (IUUpdateSwitch(&OnCoordSetSP, states, names, n) < 0)
            return;

        currentSet     = get_switch_index(&OnCoordSetSP);
        OnCoordSetSP.s = IPS_OK;
        IDSetSwitch(&OnCoordSetSP, nullptr);
    }

    // ===================================
    // Track Mode Set
    // ===================================
    if (!strcmp(name, TrackModeSP.name))
    {
        if (IUUpdateSwitch(&TrackModeSP, states, names, n) < 0)
            return;

        int trackMode = get_switch_index(&TrackModeSP);
        selectTrackingMode(fd, trackMode);
        TrackModeSP.s = IPS_OK;
        IDSetSwitch(&TrackModeSP, nullptr);
    }

    // ===================================
    // Abort slew
    // ===================================
    if (!strcmp(name, AbortSlewSP.name))
    {
        IUResetSwitch(&AbortSlewSP);
        abortSlew(fd);

        if (EquatorialCoordsRNP.s == IPS_BUSY)
        {
            AbortSlewSP.s         = IPS_OK;
            EquatorialCoordsRNP.s = IPS_IDLE;
            IDSetSwitch(&AbortSlewSP, "Slew aborted.");
            IDSetNumber(&EquatorialCoordsRNP, nullptr);
        }

        return;
    }
}

/**************************************************************************************
** Retry connecting to the telescope on error. Give up if there is no hope.
***************************************************************************************/
void IEQ45Basic::handle_error(INumberVectorProperty *nvp, int err, const char *msg)
{
    nvp->s = IPS_ALERT;

    /* First check to see if the telescope is connected */
    if (check_IEQ45_connection(fd))
    {
        /* The telescope is off locally */
        ConnectS[0].s = ISS_OFF;
        ConnectS[1].s = ISS_ON;
        ConnectSP.s   = IPS_BUSY;
        IDSetSwitch(&ConnectSP, "Telescope is not responding to commands, will retry in 10 seconds.");

        IDSetNumber(nvp, nullptr);
        IEAddTimer(10000, retry_connection, &fd);
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

/**************************************************************************************
** Set all properties to idle and reset most switches to clean state.
***************************************************************************************/
void IEQ45Basic::reset_all_properties()
{
    ConnectSP.s           = IPS_IDLE;
    OnCoordSetSP.s        = IPS_IDLE;
    TrackModeSP.s         = IPS_IDLE;
    AbortSlewSP.s         = IPS_IDLE;
    PortTP.s              = IPS_IDLE;
    EquatorialCoordsRNP.s = IPS_IDLE;

    IUResetSwitch(&OnCoordSetSP);
    IUResetSwitch(&TrackModeSP);
    IUResetSwitch(&AbortSlewSP);

    OnCoordSetS[0].s = ISS_ON;
    TrackModeS[0].s  = ISS_ON;
    ConnectS[0].s    = ISS_OFF;
    ConnectS[1].s    = ISS_ON;

    IDSetSwitch(&ConnectSP, nullptr);
    IDSetSwitch(&OnCoordSetSP, nullptr);
    IDSetSwitch(&TrackModeSP, nullptr);
    IDSetSwitch(&AbortSlewSP, nullptr);
    IDSetText(&PortTP, nullptr);
    IDSetNumber(&EquatorialCoordsRNP, nullptr);
}

/**************************************************************************************
**
***************************************************************************************/
void IEQ45Basic::correct_fault()
{
    fault = false;
    IDMessage(mydev, "Telescope is online.");
}

/**************************************************************************************
**
***************************************************************************************/
bool IEQ45Basic::is_connected()
{
    if (simulation)
        return true;

    return (ConnectSP.sp[0].s == ISS_ON);
}

/**************************************************************************************
**
***************************************************************************************/
static void retry_connection(void *p)
{
    int fd = *((int *)p);

    if (check_IEQ45_connection(fd))
        telescope->connection_lost();
    else
        telescope->connection_resumed();
}

/**************************************************************************************
**
***************************************************************************************/
void IEQ45Basic::ISPoll()
{
    if (is_connected() == false || simulation)
        return;

    double dx, dy;
    int error_code = 0;

    switch (EquatorialCoordsRNP.s)
    {
        case IPS_IDLE:
            getIEQ45RA(fd, &currentRA);
            getIEQ45DEC(fd, &currentDEC);
            if (fabs(currentRA - lastRA) > 0.01 || fabs(currentDEC - lastDEC) > 0.01)
            {
                lastRA  = currentRA;
                lastDEC = currentDEC;
                IDSetNumber(&EquatorialCoordsRNP, nullptr);
                IDLog("IDLE update coord\n");
            }
            break;

        case IPS_BUSY:
            getIEQ45RA(fd, &currentRA);
            getIEQ45DEC(fd, &currentDEC);
            dx = targetRA - currentRA;
            dy = targetDEC - currentDEC;

            // Wait until acknowledged or within threshold
            if (fabs(dx) <= (3 / (900.0)) && fabs(dy) <= (3 / 60.0))
            {
                lastRA  = currentRA;
                lastDEC = currentDEC;
                IUResetSwitch(&OnCoordSetSP);
                OnCoordSetSP.s        = IPS_OK;
                EquatorialCoordsRNP.s = IPS_OK;
                IDSetNumber(&EquatorialCoordsRNP, nullptr);

                switch (currentSet)
                {
                    case IEQ45_SLEW:
                        OnCoordSetSP.sp[IEQ45_SLEW].s = ISS_ON;
                        IDSetSwitch(&OnCoordSetSP, "Slew is complete.");
                        break;

                    case IEQ45_SYNC:
                        break;
                }
            }
            else
                IDSetNumber(&EquatorialCoordsRNP, nullptr);
            break;

        case IPS_OK:

            if ((error_code = getIEQ45RA(fd, &currentRA)) < 0 || (error_code = getIEQ45DEC(fd, &currentDEC)) < 0)
            {
                handle_error(&EquatorialCoordsRNP, error_code, "Getting RA/DEC");
                return;
            }

            if (fault == true)
                correct_fault();

            if ((currentRA != lastRA) || (currentDEC != lastDEC))
            {
                lastRA  = currentRA;
                lastDEC = currentDEC;
                //IDLog("IPS_OK update coords %g %g\n",currentRA,currentDEC);
                IDSetNumber(&EquatorialCoordsRNP, nullptr);
            }
            break;

        case IPS_ALERT:
            break;
    }
}

/**************************************************************************************
**
***************************************************************************************/
bool IEQ45Basic::process_coords()
{
    int error_code;
    char syncString[256];
    char RAStr[32], DecStr[32];
    double dx, dy;

    switch (currentSet)
    {
        // Slew
        case IEQ45_SLEW:
            lastSet = IEQ45_SLEW;
            if (EquatorialCoordsRNP.s == IPS_BUSY)
            {
                IDLog("Aborting Slew\n");
                abortSlew(fd);

                // sleep for 100 mseconds
                usleep(100000);
            }

            if (!simulation && (error_code = Slew(fd)))
            {
                slew_error(error_code);
                return false;
            }

            EquatorialCoordsRNP.s = IPS_BUSY;
            fs_sexa(RAStr, targetRA, 2, 3600);
            fs_sexa(DecStr, targetDEC, 2, 3600);
            IDSetNumber(&EquatorialCoordsRNP, "Slewing to JNow RA %s - DEC %s", RAStr, DecStr);
            IDLog("Slewing to JNow RA %s - DEC %s\n", RAStr, DecStr);
            break;

        // Sync
        case IEQ45_SYNC:
            lastSet               = IEQ45_SYNC;
            EquatorialCoordsRNP.s = IPS_IDLE;

            if (!simulation && (error_code = Sync(fd, syncString) < 0))
            {
                IDSetNumber(&EquatorialCoordsRNP, "Synchronization failed.");
                return false;
            }

            if (simulation)
            {
                EquatorialCoordsRN[0].value = EquatorialCoordsRN[0].value;
                EquatorialCoordsRN[1].value = EquatorialCoordsRN[1].value;
            }

            EquatorialCoordsRNP.s = IPS_OK;
            IDLog("Synchronization successful %s\n", syncString);
            IDSetNumber(&EquatorialCoordsRNP, "Synchronization successful.");
            break;
    }

    return true;
}

/**************************************************************************************
**
***************************************************************************************/
int IEQ45Basic::get_switch_index(ISwitchVectorProperty *sp)
{
    for (int i = 0; i < sp->nsp; i++)
        if (sp->sp[i].s == ISS_ON)
            return i;

    return -1;
}

/**************************************************************************************
**
***************************************************************************************/
void IEQ45Basic::connect_telescope()
{
    switch (ConnectSP.sp[0].s)
    {
        case ISS_ON:

            if (simulation)
            {
                ConnectSP.s = IPS_OK;
                IDSetSwitch(&ConnectSP, "Simulated telescope is online.");
                return;
            }

            if (tty_connect(PortT[0].text, 9600, 8, 0, 1, &fd) != TTY_OK)
            {
                ConnectS[0].s = ISS_OFF;
                ConnectS[1].s = ISS_ON;
                IDSetSwitch(
                    &ConnectSP,
                    "Error connecting to port %s. Make sure you have BOTH read and write permission to the port.",
                    PortT[0].text);
                return;
            }

            if (check_IEQ45_connection(fd))
            {
                ConnectS[0].s = ISS_OFF;
                ConnectS[1].s = ISS_ON;
                IDSetSwitch(&ConnectSP, "Error connecting to Telescope. Telescope is offline.");
                return;
            }

            ConnectSP.s = IPS_OK;
            IDSetSwitch(&ConnectSP, "Telescope is online. Retrieving basic data...");
            get_initial_data();
            break;

        case ISS_OFF:
            ConnectS[0].s = ISS_OFF;
            ConnectS[1].s = ISS_ON;
            ConnectSP.s   = IPS_IDLE;
            if (simulation)
            {
                IDSetSwitch(&ConnectSP, "Simulated Telescope is offline.");
                return;
            }
            IDSetSwitch(&ConnectSP, "Telescope is offline.");
            IDLog("Telescope is offline.");

            tty_disconnect(fd);
            break;
    }
}

/**************************************************************************************
**
***************************************************************************************/
void IEQ45Basic::get_initial_data()
{
    // Make sure short
    checkIEQ45Format(fd);

    // Get current RA/DEC
    getIEQ45RA(fd, &currentRA);
    getIEQ45DEC(fd, &currentDEC);

    IDSetNumber(&EquatorialCoordsRNP, nullptr);
}

/**************************************************************************************
**
***************************************************************************************/
void IEQ45Basic::slew_error(int slewCode)
{
    OnCoordSetSP.s = IPS_IDLE;
    IDLog("Aborting Slew\n");
    abortSlew(fd);
    if (slewCode == 1)
        IDSetSwitch(&OnCoordSetSP, "Object below horizon.");
    else if (slewCode == 2)
        IDSetSwitch(&OnCoordSetSP, "Object below the minimum elevation limit.");
    else
        IDSetSwitch(&OnCoordSetSP, "Slew failed.");
}

/**************************************************************************************
**
***************************************************************************************/
void IEQ45Basic::enable_simulation(bool enable)
{
    simulation = enable;

    if (simulation)
        IDLog("Warning: Simulation is activated.\n");
    else
        IDLog("Simulation is disabled.\n");
}

/**************************************************************************************
**
***************************************************************************************/
void IEQ45Basic::connection_lost()
{
    ConnectSP.s = IPS_IDLE;
    IDSetSwitch(&ConnectSP, "The connection to the telescope is lost.");
    return;
}

/**************************************************************************************
**
***************************************************************************************/
void IEQ45Basic::connection_resumed()
{
    ConnectS[0].s = ISS_ON;
    ConnectS[1].s = ISS_OFF;
    ConnectSP.s   = IPS_OK;

    IDSetSwitch(&ConnectSP, "The connection to the telescope has been resumed.");
}

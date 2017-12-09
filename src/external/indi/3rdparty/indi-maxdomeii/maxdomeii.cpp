/*
    MaxDome II 
    Copyright (C) 2009 Ferran Casarramona (ferran.casarramona@gmail.com)

    Migrated to INDI::Dome by Jasem Mutlaq (2014)

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

#include "maxdomeii.h"

#include "config.h"
#include "maxdomeiidriver.h"  // MaxDome II Command Set

#include <connectionplugins/connectionserial.h>

#include <memory>

#include <math.h>
#include <string.h>
#include <unistd.h>

const int POLLMS = 1000; // Period of update, 1 second.

// We declare an auto pointer to dome.
std::unique_ptr<MaxDomeII> dome(new MaxDomeII());

void ISGetProperties(const char *dev)
{
    dome->ISGetProperties(dev);
}

void ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int num)
{
    dome->ISNewSwitch(dev, name, states, names, num);
}

void ISNewText(const char *dev, const char *name, char *texts[], char *names[], int num)
{
    dome->ISNewText(dev, name, texts, names, num);
}

void ISNewNumber(const char *dev, const char *name, double values[], char *names[], int num)
{
    dome->ISNewNumber(dev, name, values, names, num);
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
    dome->ISSnoopDevice(root);
}

MaxDomeII::MaxDomeII()
{
    nTicksPerTurn               = 360;
    nCurrentTicks               = 0;
    nParkPosition               = 0.0;
    nHomeAzimuth                = 0.0;
    nHomeTicks                  = 0;
    nCloseShutterBeforePark     = 0;
    nTimeSinceShutterStart      = -1; // No movement has started
    nTimeSinceAzimuthStart      = -1; // No movement has started
    nTargetAzimuth              = -1; //Target azimuth not established
    nTimeSinceLastCommunication = 0;

    SetDomeCapability(DOME_CAN_ABORT | DOME_CAN_ABS_MOVE | DOME_HAS_SHUTTER);

    setVersion(INDI_MAXDOMEII_VERSION_MAJOR, INDI_MAXDOMEII_VERSION_MINOR);
}

bool MaxDomeII::SetupParms()
{
    DomeAbsPosN[0].value = 0;

    IDSetNumber(&DomeAbsPosNP, NULL);
    IDSetNumber(&DomeParamNP, NULL);

    return true;
}

bool MaxDomeII::Handshake()
{
    return (Ack_MaxDomeII(PortFD) == 0);
}

MaxDomeII::~MaxDomeII()
{
    prev_az = prev_alt = 0;
}

const char *MaxDomeII::getDefaultName()
{
    return (char *)"MaxDome II";
}

bool MaxDomeII::initProperties()
{
    INDI::Dome::initProperties();

    IUFillNumber(&HomeAzimuthN[0], "HOME_AZIMUTH", "Home azimuth", "%5.2f", 0., 360., 0., nHomeAzimuth);
    IUFillNumberVector(&HomeAzimuthNP, HomeAzimuthN, NARRAY(HomeAzimuthN), getDeviceName(), "HOME_AZIMUTH",
                       "Home azimuth", OPTIONS_TAB, IP_RW, 0, IPS_IDLE);

    // Ticks per turn
    IUFillNumber(&TicksPerTurnN[0], "TICKS_PER_TURN", "Ticks per turn", "%5.2f", 0., 360., 0., nTicksPerTurn);
    IUFillNumberVector(&TicksPerTurnNP, TicksPerTurnN, NARRAY(TicksPerTurnN), getDeviceName(), "TICKS_PER_TURN",
                       "Ticks per turn", OPTIONS_TAB, IP_RW, 0, IPS_IDLE);

    // Park position
    IUFillNumber(&ShutterOperationAzimuthN[0], "SOp_AZIMUTH", "Azimuth", "%5.2f", 0., 360., 0., nParkPosition);
    IUFillNumberVector(&ShutterOperationAzimuthNP, ShutterOperationAzimuthN, NARRAY(ShutterOperationAzimuthN),
                       getDeviceName(), "SHUTTER_OPERATION_AZIMUTH", "Shutter operation azimuth", OPTIONS_TAB, IP_RW, 0,
                       IPS_IDLE);

    // Park on shutter
    IUFillSwitch(&ShutterConflictS[0], "MOVE", "Move", ISS_ON);
    IUFillSwitch(&ShutterConflictS[1], "NO_MOVE", "No move", ISS_OFF);
    IUFillSwitchVector(&ShutterConflictSP, ShutterConflictS, NARRAY(ShutterConflictS), getDeviceName(),
                       "AZIMUTH_ON_SHUTTER", "Azimuth on operating shutter", OPTIONS_TAB, IP_RW, ISR_1OFMANY, 0,
                       IPS_IDLE);

    // Shutter mode
    IUFillSwitch(&ShutterModeS[0], "FULL", "Open full", ISS_ON);
    IUFillSwitch(&ShutterModeS[1], "UPPER", "Open upper only", ISS_OFF);
    IUFillSwitchVector(&ShutterModeSP, ShutterModeS, NARRAY(ShutterModeS), getDeviceName(), "SHUTTER_MODE",
                       "Shutter open mode", MAIN_CONTROL_TAB, IP_RW, ISR_1OFMANY, 0, IPS_IDLE);

    // Home - Home command
    IUFillSwitch(&HomeS[0], "HOME", "Home", ISS_OFF);
    IUFillSwitchVector(&HomeSP, HomeS, NARRAY(HomeS), getDeviceName(), "HOME_MOTION", "Home dome", MAIN_CONTROL_TAB,
                       IP_RW, ISR_ATMOST1, 0, IPS_IDLE);

    // Watch Dog
    IUFillNumber(&WatchDogN[0], "WATCH_DOG_TIME", "Watch dog time", "%5.2f", 0., 3600., 0., 0.);
    IUFillNumberVector(&WatchDogNP, WatchDogN, NARRAY(WatchDogN), getDeviceName(), "WATCH_DOG_TIME_SET",
                       "Watch dog time set", OPTIONS_TAB, IP_RW, 0, IPS_IDLE);

    // Set default baud rate to 19200
    serialConnection->setDefaultBaudRate(Connection::Serial::B_19200);

    return true;
}

bool MaxDomeII::updateProperties()
{
    INDI::Dome::updateProperties();

    if (isConnected())
    {
        defineNumber(&HomeAzimuthNP);
        defineNumber(&TicksPerTurnNP);
        defineNumber(&ShutterOperationAzimuthNP);
        defineSwitch(&ShutterConflictSP);
        defineSwitch(&ShutterModeSP);
        defineSwitch(&HomeSP);
        defineNumber(&WatchDogNP);

        SetupParms();
    }
    else
    {
        deleteProperty(HomeAzimuthNP.name);
        deleteProperty(TicksPerTurnNP.name);
        deleteProperty(ShutterOperationAzimuthNP.name);
        deleteProperty(ShutterConflictSP.name);
        deleteProperty(ShutterModeSP.name);
        deleteProperty(HomeSP.name);
        deleteProperty(WatchDogNP.name);
    }

    return true;
}

bool MaxDomeII::saveConfigItems(FILE *fp)
{
    IUSaveConfigNumber(fp, &HomeAzimuthNP);
    IUSaveConfigNumber(fp, &TicksPerTurnNP);
    IUSaveConfigNumber(fp, &ShutterOperationAzimuthNP);
    IUSaveConfigSwitch(fp, &ShutterConflictSP);
    IUSaveConfigSwitch(fp, &ShutterModeSP);

    return INDI::Dome::saveConfigItems(fp);
}

bool MaxDomeII::Disconnect()
{
    Disconnect_MaxDomeII(PortFD);

    return INDI::Dome::Disconnect();
}

void MaxDomeII::TimerHit()
{
    SH_Status nShutterStatus;
    AZ_Status nAzimuthStatus;
    unsigned nHomePosition;
    float nAz;
    int nError;
    int nRetry = 1;

    if (isConnected() == false)
        return; //  No need to reset timer if we are not connected anymore

    nError = Status_MaxDomeII(PortFD, &nShutterStatus, &nAzimuthStatus, &nCurrentTicks, &nHomePosition);
    handle_driver_error(&nError, &nRetry); // This is a timer, we will not repeat in order to not delay the execution.

    // Increment movment time counter
    if (nTimeSinceShutterStart >= 0)
        nTimeSinceShutterStart++;
    if (nTimeSinceAzimuthStart >= 0)
        nTimeSinceAzimuthStart++;

    // Watch dog
    nTimeSinceLastCommunication++;
    if (WatchDogNP.np[0].value > 0 && WatchDogNP.np[0].value <= nTimeSinceLastCommunication)
    {
        // Close Shutter if it is not
        if (nShutterStatus != Ss_CLOSED)
        {
            DomeShutterSP.s = ControlShutter(SHUTTER_CLOSE);
            IDSetSwitch(&DomeShutterSP, "Closing shutter due watch dog");
        }
    }

    if (getWeatherState() == IPS_ALERT)
    {
        // Close Shutter if it is not
        if (nShutterStatus != Ss_CLOSED)
        {
            DomeShutterSP.s = ControlShutter(SHUTTER_CLOSE);
            IDSetSwitch(&DomeShutterSP, "Closing shutter due Weather conditions");
        }
    }

    if (!nError)
    {
        // Shutter
        switch (nShutterStatus)
        {
            case Ss_CLOSED:
                if (DomeShutterS[1].s == ISS_ON) // Close Shutter
                {
                    if (DomeShutterSP.s == IPS_BUSY || DomeShutterSP.s == IPS_ALERT)
                    {
                        DomeShutterSP.s        = IPS_OK; // Shutter close movement ends.
                        nTimeSinceShutterStart = -1;
                        IDSetSwitch(&DomeShutterSP, "Shutter is closed");
                    }
                }
                else
                {
                    if (nTimeSinceShutterStart >= 0)
                    { // A movement has started. Warn but don't change
                        if (nTimeSinceShutterStart >= 4)
                        {
                            DomeShutterSP.s = IPS_ALERT; // Shutter close movement ends.
                            IDSetSwitch(&DomeShutterSP, "Shutter still closed");
                        }
                    }
                    else
                    { // For some reason (manual operation?) the shutter has closed
                        DomeShutterSP.s   = IPS_IDLE;
                        DomeShutterS[1].s = ISS_ON;
                        DomeShutterS[0].s = ISS_OFF;
                        IDSetSwitch(&DomeShutterSP, "Unexpected shutter closed");
                    }
                }
                break;
            case Ss_OPENING:
                if (DomeShutterS[0].s == ISS_OFF) // not opening Shutter
                {                                 // For some reason the shutter is opening (manual operation?)
                    DomeShutterSP.s   = IPS_ALERT;
                    DomeShutterS[1].s = ISS_OFF;
                    DomeShutterS[0].s = ISS_OFF;
                    IDSetSwitch(&DomeShutterSP, "Unexpected shutter opening");
                }
                else if (nTimeSinceShutterStart < 0)
                { // For some reason the shutter is opening (manual operation?)
                    DomeShutterSP.s        = IPS_ALERT;
                    nTimeSinceShutterStart = 0;
                    IDSetSwitch(&DomeShutterSP, "Unexpected shutter opening");
                }
                else if (DomeShutterSP.s == IPS_ALERT)
                { // The alert has corrected
                    DomeShutterSP.s = IPS_BUSY;
                    IDSetSwitch(&DomeShutterSP, "Shutter is opening");
                }
                break;
            case Ss_OPEN:
                if (DomeShutterS[0].s == ISS_ON) // Open Shutter
                {
                    if (DomeShutterSP.s == IPS_BUSY || DomeShutterSP.s == IPS_ALERT)
                    {
                        DomeShutterSP.s        = IPS_OK; // Shutter open movement ends.
                        nTimeSinceShutterStart = -1;
                        IDSetSwitch(&DomeShutterSP, "Shutter is open");
                    }
                }
                else
                {
                    if (nTimeSinceShutterStart >= 0)
                    { // A movement has started. Warn but don't change
                        if (nTimeSinceShutterStart >= 4)
                        {
                            DomeShutterSP.s = IPS_ALERT; // Shutter open movement alert.
                            IDSetSwitch(&DomeShutterSP, "Shutter still open");
                        }
                    }
                    else
                    { // For some reason (manual operation?) the shutter has open
                        DomeShutterSP.s   = IPS_IDLE;
                        DomeShutterS[1].s = ISS_ON;
                        DomeShutterS[0].s = ISS_OFF;
                        IDSetSwitch(&DomeShutterSP, "Unexpected shutter open");
                    }
                }
                break;
            case Ss_CLOSING:
                if (DomeShutterS[1].s == ISS_OFF) // Not closing Shutter
                {                                 // For some reason the shutter is closing (manual operation?)
                    DomeShutterSP.s   = IPS_ALERT;
                    DomeShutterS[1].s = ISS_ON;
                    DomeShutterS[0].s = ISS_OFF;
                    IDSetSwitch(&DomeShutterSP, "Unexpected shutter closing");
                }
                else if (nTimeSinceShutterStart < 0)
                { // For some reason the shutter is opening (manual operation?)
                    DomeShutterSP.s        = IPS_ALERT;
                    nTimeSinceShutterStart = 0;
                    IDSetSwitch(&DomeShutterSP, "Unexpected shutter closing");
                }
                else if (DomeShutterSP.s == IPS_ALERT)
                { // The alert has corrected
                    DomeShutterSP.s = IPS_BUSY;
                    IDSetSwitch(&DomeShutterSP, "Shutter is closing");
                }
                break;
            case Ss_ABORTED:
            case Ss_ERROR:
            default:
                if (nTimeSinceShutterStart >= 0)
                {
                    DomeShutterSP.s        = IPS_ALERT; // Shutter movement aborted.
                    DomeShutterS[1].s      = ISS_OFF;
                    DomeShutterS[0].s      = ISS_OFF;
                    nTimeSinceShutterStart = -1;
                    IDSetSwitch(&DomeShutterSP, "Unknown shutter status");
                }
                break;
        }

        // Azimuth
        nAz = TicksToAzimuth(nCurrentTicks);
        if (DomeAbsPosN[0].value != nAz)
        { // Only refresh position if it changed
            DomeAbsPosN[0].value = nAz;
            //sprintf(buf,"%d", nCurrentTicks);
            IDSetNumber(&DomeAbsPosNP, NULL);
        }

        switch (nAzimuthStatus)
        {
            case As_IDLE:
            case As_IDEL2:
                if (nTimeSinceAzimuthStart > 3)
                {
                    if (nTargetAzimuth >= 0 &&
                        AzimuthDistance(nTargetAzimuth, nCurrentTicks) > 3) // Maximum difference allowed: 3 ticks
                    {
                        DomeAbsPosNP.s         = IPS_ALERT;
                        nTimeSinceAzimuthStart = -1;
                        IDSetNumber(&DomeAbsPosNP, "Could not position right");
                    }
                    else
                    { // Succesfull end of movement
                        if (DomeAbsPosNP.s != IPS_OK)
                        {
                            setDomeState(DOME_SYNCED);
                            nTimeSinceAzimuthStart = -1;
                            DEBUG(INDI::Logger::DBG_SESSION, "Dome is on target position");
                        }
                        if (HomeS[0].s == ISS_ON)
                        {
                            HomeS[0].s             = ISS_OFF;
                            HomeSP.s               = IPS_OK;
                            nTimeSinceAzimuthStart = -1;
                            IDSetSwitch(&HomeSP, "Dome is homed");
                        }
                    }
                }
                break;
            case As_MOVING_WE:
            case As_MOVING_EW:
                if (nTimeSinceAzimuthStart < 0)
                {
                    nTimeSinceAzimuthStart = 0;
                    nTargetAzimuth         = -1;
                    DomeAbsPosNP.s         = IPS_ALERT;
                    IDSetNumber(&DomeAbsPosNP, "Unexpected dome moving");
                }
                break;
            case As_ERROR:
                if (nTimeSinceAzimuthStart >= 0)
                {
                    DomeAbsPosNP.s         = IPS_ALERT;
                    nTimeSinceAzimuthStart = -1;
                    nTargetAzimuth         = -1;
                    IDSetNumber(&DomeAbsPosNP, "Dome Error");
                }
            default:
                break;
        }
    }
    else
    {
        DEBUGF(INDI::Logger::DBG_DEBUG, "Error: %s. Please reconnect and try again.", ErrorMessages[-nError]);
        return;
    }

    SetTimer(POLLMS);
    return;
}

IPState MaxDomeII::MoveAbs(double newAZ)
{
    double currAZ = 0;
    int newPos = 0, nDir = 0;
    int error;
    int nRetry = 3;

    currAZ = DomeAbsPosN[0].value;

    // Take the shortest path
    if (newAZ > currAZ)
    {
        if (newAZ - currAZ > 180.0)
            nDir = MAXDOMEII_WE_DIR;
        else
            nDir = MAXDOMEII_EW_DIR;
    }
    else
    {
        if (currAZ - newAZ > 180.0)
            nDir = MAXDOMEII_EW_DIR;
        else
            nDir = MAXDOMEII_WE_DIR;
    }

    newPos = AzimuthToTicks(newAZ);

    while (nRetry)
    {
        error = Goto_Azimuth_MaxDomeII(PortFD, nDir, newPos);
        handle_driver_error(&error, &nRetry);
    }

    if (error != 0)
        return IPS_ALERT;

    nTargetAzimuth         = newPos;
    nTimeSinceAzimuthStart = 0; // Init movement timer

    // It will take a few cycles to reach final position
    return IPS_BUSY;
}

bool MaxDomeII::Abort()
{
    int error  = 0;
    int nRetry = 3;

    while (nRetry)
    {
        error = Abort_Azimuth_MaxDomeII(PortFD);
        handle_driver_error(&error, &nRetry);
    }

    while (nRetry)
    {
        error = Abort_Shutter_MaxDomeII(PortFD);
        handle_driver_error(&error, &nRetry);
    }

    DomeAbsPosNP.s = IPS_IDLE;
    IDSetNumber(&DomeAbsPosNP, NULL);

    // If we abort while in the middle of opening/closing shutter, alert.
    if (DomeShutterSP.s == IPS_BUSY)
    {
        DomeShutterSP.s = IPS_ALERT;
        IDSetSwitch(&DomeShutterSP, "Shutter operation aborted.");
        return false;
    }

    return true;
}

/**************************************************************************************
**
***************************************************************************************/
bool MaxDomeII::ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    // Ignore if not ours
    if (strcmp(dev, getDeviceName()))
        return false;

    nTimeSinceLastCommunication = 0;

    // ===================================
    // TicksPerTurn
    // ===================================
    if (!strcmp(name, TicksPerTurnNP.name))
    {
        double nVal;
        char cLog[255];
        int error;
        int nRetry = 3;

        if (IUUpdateNumber(&TicksPerTurnNP, values, names, n) < 0)
            return false;

        nVal = values[0];
        if (nVal >= 100 && nVal <= 500)
        {
            while (nRetry)
            {
                error = SetTicksPerCount_MaxDomeII(PortFD, nVal);
                handle_driver_error(&error, &nRetry);
            }
            if (error >= 0)
            {
                sprintf(cLog, "New Ticks Per Turn set: %lf", nVal);
                nTicksPerTurn    = nVal;
                nHomeTicks       = floor(0.5 + nHomeAzimuth * nTicksPerTurn / 360.0); // Calculate Home ticks again
                TicksPerTurnNP.s = IPS_OK;
                TicksPerTurnNP.np[0].value = nVal;
                IDSetNumber(&TicksPerTurnNP, "%s", cLog);
                return true;
            }
            else
            {
                DEBUGF(INDI::Logger::DBG_ERROR, "MAX DOME II: %s", ErrorMessages[-error]);
                TicksPerTurnNP.s = IPS_ALERT;
                IDSetNumber(&TicksPerTurnNP, NULL);
            }

            return false;
        }

        // Incorrect value.
        TicksPerTurnNP.s = IPS_ALERT;
        IDSetNumber(&TicksPerTurnNP, "Invalid Ticks Per Turn");

        return false;
    }

    // ===================================
    // HomeAzimuth
    // ===================================
    if (!strcmp(name, HomeAzimuthNP.name))
    {
        double nVal;
        char cLog[255];

        if (IUUpdateNumber(&HomeAzimuthNP, values, names, n) < 0)
            return false;

        nVal = values[0];
        if (nVal >= 0 && nVal <= 360)
        {
            sprintf(cLog, "New home azimuth set: %lf", nVal);
            nHomeAzimuth              = nVal;
            nHomeTicks                = floor(0.5 + nHomeAzimuth * nTicksPerTurn / 360.0);
            HomeAzimuthNP.s           = IPS_OK;
            HomeAzimuthNP.np[0].value = nVal;
            IDSetNumber(&HomeAzimuthNP, "%s", cLog);
            return true;
        }
        // Incorrect value.
        HomeAzimuthNP.s = IPS_ALERT;
        IDSetNumber(&HomeAzimuthNP, "Invalid home azimuth");

        return false;
    }

    // ===================================
    // Watch dog
    // ===================================
    if (!strcmp(name, WatchDogNP.name))
    {
        double nVal;
        char cLog[255];

        if (IUUpdateNumber(&WatchDogNP, values, names, n) < 0)
            return false;

        nVal = values[0];
        if (nVal >= 0 && nVal <= 3600)
        {
            sprintf(cLog, "New watch dog set: %lf", nVal);
            WatchDogNP.s           = IPS_OK;
            WatchDogNP.np[0].value = nVal;
            IDSetNumber(&WatchDogNP, "%s", cLog);
            return true;
        }
        // Incorrect value.
        WatchDogNP.s = IPS_ALERT;
        IDSetNumber(&WatchDogNP, "Invalid watch dog time");

        return false;
    }

    // ===================================
    // Shutter operation azimuth
    // ===================================
    if (!strcmp(name, ShutterOperationAzimuthNP.name))
    {
        double nVal;
        IPState error;

        if (IUUpdateNumber(&ShutterOperationAzimuthNP, values, names, n) < 0)
            return false;

        nVal = values[0];
        if (nVal >= 0 && nVal < 360)
        {
            error = ConfigurePark(nCloseShutterBeforePark, nVal);

            if (error == IPS_OK)
            {
                nParkPosition                         = nVal;
                ShutterOperationAzimuthNP.s           = IPS_OK;
                ShutterOperationAzimuthNP.np[0].value = nVal;
                IDSetNumber(&ShutterOperationAzimuthNP, "New shutter operation azimuth set");
            }
            else
            {
                ShutterOperationAzimuthNP.s = IPS_ALERT;
                IDSetNumber(&ShutterOperationAzimuthNP, "%s", ErrorMessages[-error]);
            }

            return true;
        }
        // Incorrect value.
        ShutterOperationAzimuthNP.s = IPS_ALERT;
        IDSetNumber(&ShutterOperationAzimuthNP, "Invalid shutter operation azimuth position");

        return false;
    }

    return INDI::Dome::ISNewNumber(dev, name, values, names, n);
}

/**************************************************************************************
**
***************************************************************************************/
bool MaxDomeII::ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    // ignore if not ours //
    if (strcmp(getDeviceName(), dev))
        return false;

    nTimeSinceLastCommunication = 0;

    // ===================================
    // Home
    // ===================================
    if (!strcmp(name, HomeSP.name))
    {
        if (IUUpdateSwitch(&HomeSP, states, names, n) < 0)
            return false;

        int error;
        int nRetry = 3;

        while (nRetry)
        {
            error = Home_Azimuth_MaxDomeII(PortFD);
            handle_driver_error(&error, &nRetry);
        }
        nTimeSinceAzimuthStart = 0;
        nTargetAzimuth         = -1;
        if (error)
        {
            DEBUGF(INDI::Logger::DBG_ERROR, "Error Homing Azimuth (%s).", ErrorMessages[-error]);
            HomeSP.s = IPS_ALERT;
            IDSetSwitch(&HomeSP, "Error Homing Azimuth");
            return false;
        }
        HomeSP.s = IPS_BUSY;
        IDSetSwitch(&HomeSP, "Homing dome");

        return true;
    }

    // ===================================
    // Conflict on Shutter operation
    // ===================================
    if (!strcmp(name, ShutterConflictSP.name))
    {
        if (IUUpdateSwitch(&ShutterConflictSP, states, names, n) < 0)
            return false;

        int error;
        int nCSBP;

        if (ShutterConflictS[0].s == ISS_ON)
        {
            nCSBP = 1;
        }
        else
        {
            nCSBP = 0;
        }

        error = ConfigurePark(nCSBP, nParkPosition);

        if (error == IPS_OK)
        {
            ShutterConflictSP.s = IPS_OK;
            IDSetSwitch(&ShutterConflictSP, "New shutter operation conflict set");
        }
        else
        {
            ShutterConflictSP.s = IPS_ALERT;
            IDSetSwitch(&ShutterConflictSP, "%s", ErrorMessages[-error]);
        }

        return true;
    }

    return INDI::Dome::ISNewSwitch(dev, name, states, names, n);
}

/**************************************************************************************
**
***************************************************************************************/
int MaxDomeII::AzimuthDistance(int nPos1, int nPos2)
{
    int nDif = std::abs(nPos1 - nPos2);

    if (nDif > nTicksPerTurn / 2)
        nDif = nTicksPerTurn - nDif;

    return nDif;
}

/**************************************************************************************
 **
 ***************************************************************************************/
double MaxDomeII::TicksToAzimuth(int nTicks)
{
    double nAz;

    nAz = nHomeAzimuth + nTicks * 360.0 / nTicksPerTurn;
    while (nAz < 0)
        nAz += 360;
    while (nAz >= 360)
        nAz -= 360;

    return nAz;
}
/**************************************************************************************
 **
 ***************************************************************************************/
int MaxDomeII::AzimuthToTicks(double nAzimuth)
{
    int nTicks;

    nTicks = floor(0.5 + (nAzimuth - nHomeAzimuth) * nTicksPerTurn / 360.0);
    while (nTicks > nTicksPerTurn)
        nTicks -= nTicksPerTurn;
    while (nTicks < 0)
        nTicks += nTicksPerTurn;

    return nTicks;
}

/**************************************************************************************
 **
 ***************************************************************************************/

int MaxDomeII::handle_driver_error(int *error, int *nRetry)
{
    (*nRetry)--;
    switch (*error)
    {
        case 0: // No error. Continue
            *nRetry = 0;
            break;
        case -5: // can't connect
            // This error can happen if port connection is lost, i.e. a usb-serial reconnection
            // Reconnect
            DEBUG(INDI::Logger::DBG_ERROR, "MAX DOME II: Reconnecting ...");
            Connect();
            if (PortFD < 0)
                *nRetry = 0; // Can't open the port. Don't retry anymore.
            break;

        default: // Do nothing in all other errors.
            DEBUGF(INDI::Logger::DBG_ERROR, "Error on command: (%s).", ErrorMessages[-*error]);
            break;
    }

    return *nRetry;
}

/************************************************************************************
*
* ***********************************************************************************/
IPState MaxDomeII::ConfigurePark(int nCSBP, double ParkAzimuth)
{
    int error  = 0;
    int nRetry = 3;

    // Only update park position if there is change
    if (ParkAzimuth != nParkPosition || nCSBP != nCloseShutterBeforePark)
    {
        while (nRetry)
        {
            error = SetPark_MaxDomeII(PortFD, nCSBP, AzimuthToTicks(ParkAzimuth));
            handle_driver_error(&error, &nRetry);
        }
        if (error >= 0)
        {
            nParkPosition           = ParkAzimuth;
            nCloseShutterBeforePark = nCSBP;
            DEBUGF(INDI::Logger::DBG_SESSION, "New park position set. %d %d", nCSBP, AzimuthToTicks(ParkAzimuth));
        }
        else
        {
            DEBUGF(INDI::Logger::DBG_ERROR, "MAX DOME II: %s", ErrorMessages[-error]);
            return IPS_ALERT;
        }
    }

    return IPS_OK;
}

/************************************************************************************
 *
* ***********************************************************************************/
bool MaxDomeII::SetCurrentPark()
{
    SetAxis1Park(DomeAbsPosN[0].value);
    return true;
}
/************************************************************************************
 *
* ***********************************************************************************/

bool MaxDomeII::SetDefaultPark()
{
    // By default set position to 0
    SetAxis1Park(0);
    return true;
}

/************************************************************************************
 *
* ***********************************************************************************/
IPState MaxDomeII::ControlShutter(ShutterOperation operation)
{
    int error  = 0;
    int nRetry = 3;

    if (operation == SHUTTER_CLOSE)
    {
        while (nRetry)
        {
            error = Close_Shutter_MaxDomeII(PortFD);
            handle_driver_error(&error, &nRetry);
        }
        nTimeSinceShutterStart = 0; // Init movement timer
        if (error)
        {
            DEBUGF(INDI::Logger::DBG_ERROR, "Error closing shutter (%s).", ErrorMessages[-error]);
            return IPS_ALERT;
        }
        return IPS_BUSY;
    }
    else
    {
        if (ShutterModeS[0].s == ISS_ON)
        { // Open Shutter
            while (nRetry)
            {
                error = Open_Shutter_MaxDomeII(PortFD);
                handle_driver_error(&error, &nRetry);
            }
            nTimeSinceShutterStart = 0; // Init movement timer
            if (error)
            {
                DEBUGF(INDI::Logger::DBG_ERROR, "Error opening shutter (%s).", ErrorMessages[-error]);
                return IPS_ALERT;
            }
            return IPS_BUSY;
        }
        else
        { // Open upper shutter only
            while (nRetry)
            {
                error = Open_Upper_Shutter_Only_MaxDomeII(PortFD);
                handle_driver_error(&error, &nRetry);
            }
            nTimeSinceShutterStart = 0; // Init movement timer
            if (error)
            {
                DEBUGF(INDI::Logger::DBG_ERROR, "Error opening upper shutter only (%s).", ErrorMessages[-error]);
                return IPS_ALERT;
            }
            return IPS_BUSY;
        }
    }

    return IPS_ALERT;
}

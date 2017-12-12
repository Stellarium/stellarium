/*******************************************************************************
 Baader Planetarium Dome INDI Driver

 Copyright(c) 2014 Jasem Mutlaq. All rights reserved.

 Baader Dome INDI Driver

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Library General Public
 License version 2 as published by the Free Software Foundation.
 .
 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Library General Public License for more details.
 .
 You should have received a copy of the GNU Library General Public License
 along with this library; see the file COPYING.LIB.  If not, write to
 the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 Boston, MA 02110-1301, USA.
*******************************************************************************/

#include "baader_dome.h"

#include "indicom.h"

#include <cmath>
#include <cstring>
#include <memory>
#include <termios.h>

// We declare an auto pointer to BaaderDome.
std::unique_ptr<BaaderDome> baaderDome(new BaaderDome());

#define POLLMS            1000 /* Update frequency 1000 ms */
#define DOME_CMD          9    /* Dome command in bytes */
#define DOME_BUF          16   /* Dome command buffer */
#define DOME_TIMEOUT      3    /* 3 seconds comm timeout */

#define SIM_SHUTTER_TIMER 5.0 /* Simulated Shutter closes/open in 5 seconds */
#define SIM_FLAP_TIMER    5.0 /* Simulated Flap closes/open in 3 seconds */
#define SIM_DOME_HI_SPEED 5.0 /* Simulated dome speed 5.0 degrees per second, constant */
#define SIM_DOME_LO_SPEED 0.5 /* Simulated dome speed 0.5 degrees per second, constant */

void ISPoll(void *p);

void ISGetProperties(const char *dev)
{
    baaderDome->ISGetProperties(dev);
}

void ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    baaderDome->ISNewSwitch(dev, name, states, names, n);
}

void ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n)
{
    baaderDome->ISNewText(dev, name, texts, names, n);
}

void ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    baaderDome->ISNewNumber(dev, name, values, names, n);
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
    baaderDome->ISSnoopDevice(root);
}

BaaderDome::BaaderDome()
{
    targetAz         = 0;
    shutterState     = SHUTTER_UNKNOWN;
    flapStatus       = FLAP_UNKNOWN;
    simShutterStatus = SHUTTER_CLOSED;
    simFlapStatus    = FLAP_CLOSED;

    status           = DOME_UNKNOWN;
    targetShutter    = SHUTTER_CLOSE;
    targetFlap       = FLAP_CLOSE;
    calibrationStage = CALIBRATION_UNKNOWN;

    SetDomeCapability(DOME_CAN_ABORT | DOME_CAN_ABS_MOVE | DOME_CAN_REL_MOVE | DOME_CAN_PARK | DOME_HAS_SHUTTER |
                      DOME_HAS_VARIABLE_SPEED);
}

bool BaaderDome::initProperties()
{
    INDI::Dome::initProperties();

    IUFillSwitch(&CalibrateS[0], "Start", "", ISS_OFF);
    IUFillSwitchVector(&CalibrateSP, CalibrateS, 1, getDeviceName(), "Calibrate", "", MAIN_CONTROL_TAB, IP_RW,
                       ISR_ATMOST1, 0, IPS_IDLE);

    IUFillSwitch(&DomeFlapS[0], "FLAP_OPEN", "Open", ISS_OFF);
    IUFillSwitch(&DomeFlapS[1], "FLAP_CLOSE", "Close", ISS_ON);
    IUFillSwitchVector(&DomeFlapSP, DomeFlapS, 2, getDeviceName(), "DOME_FLAP", "Flap", MAIN_CONTROL_TAB, IP_RW,
                       ISR_1OFMANY, 60, IPS_OK);

    SetParkDataType(PARK_AZ);

    addAuxControls();

    return true;
}

/************************************************************************************
 *
* ***********************************************************************************/
bool BaaderDome::SetupParms()
{
    targetAz = 0;

    if (UpdatePosition())
        IDSetNumber(&DomeAbsPosNP, nullptr);

    if (UpdateShutterStatus())
        IDSetSwitch(&DomeShutterSP, nullptr);

    if (UpdateFlapStatus())
        IDSetSwitch(&DomeFlapSP, nullptr);

    if (InitPark())
    {
        // If loading parking data is successful, we just set the default parking values.
        SetAxis1ParkDefault(0);
    }
    else
    {
        // Otherwise, we set all parking data to default in case no parking data is found.
        SetAxis1Park(0);
        SetAxis1ParkDefault(0);
    }

    return true;
}

/************************************************************************************
 *
* ***********************************************************************************/
bool BaaderDome::Handshake()
{
    return Ack();
}

/************************************************************************************
 *
* ***********************************************************************************/
const char *BaaderDome::getDefaultName()
{
    return (const char *)"Baader Dome";
}

/************************************************************************************
 *
* ***********************************************************************************/
bool BaaderDome::updateProperties()
{
    INDI::Dome::updateProperties();

    if (isConnected())
    {
        defineSwitch(&DomeFlapSP);
        defineSwitch(&CalibrateSP);

        SetupParms();
    }
    else
    {
        deleteProperty(DomeFlapSP.name);
        deleteProperty(CalibrateSP.name);
    }

    return true;
}

/************************************************************************************
 *
* ***********************************************************************************/
bool BaaderDome::ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    if (dev != nullptr && strcmp(dev, getDeviceName()) == 0)
    {
        if (strcmp(name, CalibrateSP.name) == 0)
        {
            IUResetSwitch(&CalibrateSP);

            if (status == DOME_READY)
            {
                CalibrateSP.s = IPS_OK;
                DEBUG(INDI::Logger::DBG_SESSION, "Dome is already calibrated.");
                IDSetSwitch(&CalibrateSP, nullptr);
                return true;
            }

            if (CalibrateSP.s == IPS_BUSY)
            {
                Abort();
                DEBUG(INDI::Logger::DBG_SESSION, "Calibration aborted.");
                status        = DOME_UNKNOWN;
                CalibrateSP.s = IPS_IDLE;
                IDSetSwitch(&CalibrateSP, nullptr);
                return true;
            }

            status = DOME_CALIBRATING;

            DEBUG(INDI::Logger::DBG_SESSION, "Starting calibration procedure...");

            calibrationStage = CALIBRATION_STAGE1;

            calibrationStart = DomeAbsPosN[0].value;

            // Goal of procedure is to reach south point to hit sensor
            calibrationTarget1 = calibrationStart + 179;
            if (calibrationTarget1 > 360)
                calibrationTarget1 -= 360;

            if (MoveAbs(calibrationTarget1) == IPS_IDLE)
            {
                CalibrateSP.s = IPS_ALERT;
                DEBUG(INDI::Logger::DBG_ERROR, "Calibration failue due to dome motion failure.");
                status = DOME_UNKNOWN;
                IDSetSwitch(&CalibrateSP, nullptr);
                return false;
            }

            DomeAbsPosNP.s = IPS_BUSY;
            CalibrateSP.s  = IPS_BUSY;
            DEBUGF(INDI::Logger::DBG_SESSION, "Calibration is in progress. Moving to position %g.", calibrationTarget1);
            IDSetSwitch(&CalibrateSP, nullptr);
            return true;
        }

        if (strcmp(name, DomeFlapSP.name) == 0)
        {
            int ret        = 0;
            int prevStatus = IUFindOnSwitchIndex(&DomeFlapSP);
            IUUpdateSwitch(&DomeFlapSP, states, names, n);
            int FlapDome = IUFindOnSwitchIndex(&DomeFlapSP);

            // No change of status, let's return
            if (prevStatus == FlapDome)
            {
                DomeFlapSP.s = IPS_OK;
                IDSetSwitch(&DomeFlapSP, nullptr);
            }

            // go back to prev status in case of failure
            IUResetSwitch(&DomeFlapSP);
            DomeFlapS[prevStatus].s = ISS_ON;

            if (FlapDome == 0)
                ret = ControlDomeFlap(FLAP_OPEN);
            else
                ret = ControlDomeFlap(FLAP_CLOSE);

            if (ret == 0)
            {
                DomeFlapSP.s = IPS_OK;
                IUResetSwitch(&DomeFlapSP);
                DomeFlapS[FlapDome].s = ISS_ON;
                IDSetSwitch(&DomeFlapSP, "Flap is %s.", (FlapDome == 0 ? "open" : "closed"));
                return true;
            }
            else if (ret == 1)
            {
                DomeFlapSP.s = IPS_BUSY;
                IUResetSwitch(&DomeFlapSP);
                DomeFlapS[FlapDome].s = ISS_ON;
                IDSetSwitch(&DomeFlapSP, "Flap is %s...", (FlapDome == 0 ? "opening" : "closing"));
                return true;
            }

            DomeFlapSP.s = IPS_ALERT;
            IDSetSwitch(&DomeFlapSP, "Flap failed to %s.", (FlapDome == 0 ? "open" : "close"));
            return false;
        }
    }

    return INDI::Dome::ISNewSwitch(dev, name, states, names, n);
}

/************************************************************************************
 *
* ***********************************************************************************/
bool BaaderDome::Ack()
{
    int nbytes_written = 0, nbytes_read = 0, rc = -1;
    char errstr[MAXRBUF];
    char resp[DOME_BUF];
    char status[DOME_BUF];

    sim = isSimulation();

    tcflush(PortFD, TCIOFLUSH);

    if (!sim && (rc = tty_write(PortFD, "d#getflap", DOME_CMD, &nbytes_written)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "d#getflap Ack error: %s.", errstr);
        return false;
    }

    DEBUG(INDI::Logger::DBG_DEBUG, "CMD (d#getflap)");

    if (sim)
    {
        strncpy(resp, "d#flapclo", DOME_BUF);
        nbytes_read = DOME_CMD;
    }
    else if ((rc = tty_read(PortFD, resp, DOME_CMD, DOME_TIMEOUT, &nbytes_read)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "Ack error: %s.", errstr);
        return false;
    }

    resp[nbytes_read] = '\0';

    DEBUGF(INDI::Logger::DBG_DEBUG, "RES (%s)", resp);

    rc = sscanf(resp, "d#%s", status);

    if (rc > 0)
        return true;

    return false;
}

/************************************************************************************
 *
* ***********************************************************************************/
bool BaaderDome::UpdateShutterStatus()
{
    int nbytes_written = 0, nbytes_read = 0, rc = -1;
    char errstr[MAXRBUF];
    char resp[DOME_BUF];
    char status[DOME_BUF];

    tcflush(PortFD, TCIOFLUSH);

    if (!sim && (rc = tty_write(PortFD, "d#getshut", DOME_CMD, &nbytes_written)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "d#getshut UpdateShutterStatus error: %s.", errstr);
        return false;
    }

    DEBUG(INDI::Logger::DBG_DEBUG, "CMD (d#getshut)");

    if (sim)
    {
        if (simShutterStatus == SHUTTER_CLOSED)
            strncpy(resp, "d#shutclo", DOME_CMD);
        else if (simShutterStatus == SHUTTER_OPENED)
            strncpy(resp, "d#shutope", DOME_CMD);
        else if (simShutterStatus == SHUTTER_MOVING)
            strncpy(resp, "d#shutrun", DOME_CMD);
        nbytes_read = DOME_CMD;
    }
    else if ((rc = tty_read(PortFD, resp, DOME_CMD, DOME_TIMEOUT, &nbytes_read)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "UpdateShutterStatus error: %s.", errstr);
        return false;
    }

    resp[nbytes_read] = '\0';

    DEBUGF(INDI::Logger::DBG_DEBUG, "RES (%s)", resp);

    rc = sscanf(resp, "d#shut%s", status);

    if (rc > 0)
    {
        DomeShutterSP.s = IPS_OK;
        IUResetSwitch(&DomeShutterSP);

        if (strcmp(status, "ope") == 0)
        {
            if (shutterState == SHUTTER_MOVING && targetShutter == SHUTTER_OPEN)
                DEBUGF(INDI::Logger::DBG_SESSION, "%s", GetShutterStatusString(SHUTTER_OPENED));

            shutterState                 = SHUTTER_OPENED;
            DomeShutterS[SHUTTER_OPEN].s = ISS_ON;
        }
        else if (strcmp(status, "clo") == 0)
        {
            if (shutterState == SHUTTER_MOVING && targetShutter == SHUTTER_CLOSE)
                DEBUGF(INDI::Logger::DBG_SESSION, "%s", GetShutterStatusString(SHUTTER_CLOSED));

            shutterState                  = SHUTTER_CLOSED;
            DomeShutterS[SHUTTER_CLOSE].s = ISS_ON;
        }
        else if (strcmp(status, "run") == 0)
        {
            shutterState    = SHUTTER_MOVING;
            DomeShutterSP.s = IPS_BUSY;
        }
        else
        {
            shutterState    = SHUTTER_UNKNOWN;
            DomeShutterSP.s = IPS_ALERT;
            DEBUGF(INDI::Logger::DBG_ERROR, "Unknown Shutter status: %s.", resp);
        }
        return true;
    }
    return false;
}

/************************************************************************************
 *
* ***********************************************************************************/
bool BaaderDome::UpdatePosition()
{
    int nbytes_written = 0, nbytes_read = 0, rc = -1;
    char errstr[MAXRBUF];
    char resp[DOME_BUF];
    unsigned short domeAz = 0;

    tcflush(PortFD, TCIOFLUSH);

    if (!sim && (rc = tty_write(PortFD, "d#getazim", DOME_CMD, &nbytes_written)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "d#getazim UpdatePosition error: %s.", errstr);
        return false;
    }

    DEBUG(INDI::Logger::DBG_DEBUG, "CMD (d#getazim)");

    if (sim)
    {
        if (status == DOME_READY || calibrationStage == CALIBRATION_COMPLETE)
            snprintf(resp, DOME_BUF, "d#azr%04d", MountAzToDomeAz(DomeAbsPosN[0].value));
        else
            snprintf(resp, DOME_BUF, "d#azi%04d", MountAzToDomeAz(DomeAbsPosN[0].value));
        nbytes_read = DOME_CMD;
    }
    else if ((rc = tty_read(PortFD, resp, DOME_CMD, DOME_TIMEOUT, &nbytes_read)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "UpdatePosition error: %s.", errstr);
        return false;
    }

    resp[nbytes_read] = '\0';

    DEBUGF(INDI::Logger::DBG_DEBUG, "RES (%s)", resp);

    rc = sscanf(resp, "d#azr%hu", &domeAz);

    if (rc > 0)
    {
        if (calibrationStage == CALIBRATION_UNKNOWN)
        {
            status           = DOME_READY;
            calibrationStage = CALIBRATION_COMPLETE;
            DEBUG(INDI::Logger::DBG_SESSION, "Dome is calibrated.");
            CalibrateSP.s = IPS_OK;
            IDSetSwitch(&CalibrateSP, nullptr);
        }
        else if (status == DOME_CALIBRATING)
        {
            status           = DOME_READY;
            calibrationStage = CALIBRATION_COMPLETE;
            DEBUG(INDI::Logger::DBG_SESSION, "Calibration complete.");
            CalibrateSP.s = IPS_OK;
            IDSetSwitch(&CalibrateSP, nullptr);
        }

        DomeAbsPosN[0].value = DomeAzToMountAz(domeAz);
        return true;
    }
    else
    {
        rc = sscanf(resp, "d#azi%hu", &domeAz);
        if (rc > 0)
        {
            DomeAbsPosN[0].value = DomeAzToMountAz(domeAz);
            return true;
        }
    }
    return false;
}

/************************************************************************************
 *
* ***********************************************************************************/
unsigned short BaaderDome::MountAzToDomeAz(double mountAz)
{
    int domeAz = 0;

    domeAz = (mountAz)*10.0 - 1800;

    if (mountAz >= 0 && mountAz <= 179.9)
        domeAz += 3600;

    if (domeAz > 3599)
        domeAz = 3599;
    else if (domeAz < 0)
        domeAz = 0;

    return ((unsigned short)(domeAz));
}

/************************************************************************************
 *
* ***********************************************************************************/
double BaaderDome::DomeAzToMountAz(unsigned short domeAz)
{
    double mountAz = 0;

    mountAz = ((double)(domeAz + 1800)) / 10.0;

    if (domeAz >= 1800)
        mountAz -= 360;

    if (mountAz > 360)
        mountAz -= 360;
    else if (mountAz < 0)
        mountAz += 360;

    return mountAz;
}

/************************************************************************************
 *
* ***********************************************************************************/
void BaaderDome::TimerHit()
{
    if (!isConnected())
        return; //  No need to reset timer if we are not connected anymore

    UpdatePosition();

    if (DomeAbsPosNP.s == IPS_BUSY)
    {
        if (sim)
        {
            double speed = 0;
            if (fabs(targetAz - DomeAbsPosN[0].value) > SIM_DOME_HI_SPEED)
                speed = SIM_DOME_HI_SPEED;
            else
                speed = SIM_DOME_LO_SPEED;

            if (DomeRelPosNP.s == IPS_BUSY)
            {
                // CW
                if (DomeMotionS[0].s == ISS_ON)
                    DomeAbsPosN[0].value += speed;
                // CCW
                else
                    DomeAbsPosN[0].value -= speed;
            }
            else
            {
                if (targetAz > DomeAbsPosN[0].value)
                {
                    DomeAbsPosN[0].value += speed;
                }
                else if (targetAz < DomeAbsPosN[0].value)
                {
                    DomeAbsPosN[0].value -= speed;
                }
            }

            if (DomeAbsPosN[0].value < DomeAbsPosN[0].min)
                DomeAbsPosN[0].value += DomeAbsPosN[0].max;
            if (DomeAbsPosN[0].value > DomeAbsPosN[0].max)
                DomeAbsPosN[0].value -= DomeAbsPosN[0].max;
        }

        if (fabs(targetAz - DomeAbsPosN[0].value) < DomeParamN[0].value)
        {
            DomeAbsPosN[0].value = targetAz;
            DEBUG(INDI::Logger::DBG_SESSION, "Dome reached requested azimuth angle.");

            if (status != DOME_CALIBRATING)
            {
                if (getDomeState() == DOME_PARKING)
                    SetParked(true);
                else if (getDomeState() == DOME_UNPARKING)
                    SetParked(false);
                else
                    setDomeState(DOME_SYNCED);
            }

            if (status == DOME_CALIBRATING)
            {
                if (calibrationStage == CALIBRATION_STAGE1)
                {
                    DEBUG(INDI::Logger::DBG_SESSION, "Calibration stage 1 complete. Starting stage 2...");
                    calibrationTarget2 = DomeAbsPosN[0].value + 2;
                    calibrationStage   = CALIBRATION_STAGE2;
                    MoveAbs(calibrationTarget2);
                    DomeAbsPosNP.s = IPS_BUSY;
                }
                else if (calibrationStage == CALIBRATION_STAGE2)
                {
                    DEBUGF(INDI::Logger::DBG_SESSION,
                           "Calibration stage 2 complete. Returning to initial position %g...", calibrationStart);
                    calibrationStage = CALIBRATION_STAGE3;
                    MoveAbs(calibrationStart);
                    DomeAbsPosNP.s = IPS_BUSY;
                }
                else if (calibrationStage == CALIBRATION_STAGE3)
                {
                    calibrationStage = CALIBRATION_COMPLETE;
                    DEBUG(INDI::Logger::DBG_SESSION, "Dome reached initial position.");
                }
            }
        }

        IDSetNumber(&DomeAbsPosNP, nullptr);
    }
    else
        IDSetNumber(&DomeAbsPosNP, nullptr);

    UpdateShutterStatus();

    if (sim && DomeShutterSP.s == IPS_BUSY)
    {
        if (simShutterTimer-- <= 0)
        {
            simShutterTimer  = 0;
            simShutterStatus = (targetShutter == SHUTTER_OPEN) ? SHUTTER_OPENED : SHUTTER_CLOSED;
        }
    }
    else
        IDSetSwitch(&DomeShutterSP, nullptr);

    UpdateFlapStatus();

    if (sim && DomeFlapSP.s == IPS_BUSY)
    {
        if (simFlapTimer-- <= 0)
        {
            simFlapTimer  = 0;
            simFlapStatus = (targetFlap == FLAP_OPEN) ? FLAP_OPENED : FLAP_CLOSED;
        }
    }
    else
        IDSetSwitch(&DomeFlapSP, nullptr);

    SetTimer(POLLMS);
}

/************************************************************************************
 *
* ***********************************************************************************/
IPState BaaderDome::MoveAbs(double az)
{
    int nbytes_written = 0, nbytes_read = 0, rc = -1;
    char errstr[MAXRBUF];
    char cmd[DOME_BUF];
    char resp[DOME_BUF];

    if (status == DOME_UNKNOWN)
    {
        DEBUG(INDI::Logger::DBG_WARNING, "Dome is not calibrated. Please calibrate dome before issuing any commands.");
        return IPS_ALERT;
    }

    targetAz = az;

    snprintf(cmd, DOME_BUF, "d#azi%04d", MountAzToDomeAz(targetAz));

    tcflush(PortFD, TCIOFLUSH);

    if (!sim && (rc = tty_write(PortFD, cmd, DOME_CMD, &nbytes_written)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s MoveAbsDome error: %s.", cmd, errstr);
        return IPS_ALERT;
    }

    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD (%s)", cmd);

    if (sim)
    {
        strncpy(resp, "d#gotmess", DOME_CMD);
        nbytes_read = DOME_CMD;
    }
    else if ((rc = tty_read(PortFD, resp, DOME_CMD, DOME_TIMEOUT, &nbytes_read)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "MoveAbsDome error: %s.", errstr);
        return IPS_ALERT;
    }

    resp[nbytes_read] = '\0';

    DEBUGF(INDI::Logger::DBG_DEBUG, "RES (%s)", resp);

    if (strcmp(resp, "d#gotmess") == 0)
        return IPS_BUSY;

    return IPS_ALERT;
}

/************************************************************************************
 *
* ***********************************************************************************/
IPState BaaderDome::MoveRel(double azDiff)
{
    targetAz = DomeAbsPosN[0].value + azDiff;

    if (targetAz < DomeAbsPosN[0].min)
        targetAz += DomeAbsPosN[0].max;
    if (targetAz > DomeAbsPosN[0].max)
        targetAz -= DomeAbsPosN[0].max;

    // It will take a few cycles to reach final position
    return MoveAbs(targetAz);
}

/************************************************************************************
 *
* ***********************************************************************************/
IPState BaaderDome::Park()
{
    targetAz = GetAxis1Park();

    return MoveAbs(targetAz);
}

/************************************************************************************
 *
* ***********************************************************************************/
IPState BaaderDome::UnPark()
{
    return IPS_OK;
}

/************************************************************************************
 *
* ***********************************************************************************/
IPState BaaderDome::ControlShutter(ShutterOperation operation)
{
    int nbytes_written = 0, nbytes_read = 0, rc = -1;
    char errstr[MAXRBUF];
    char cmd[DOME_BUF];
    char resp[DOME_BUF];

    memset(cmd, 0, sizeof(cmd));

    if (operation == SHUTTER_OPEN)
    {
        targetShutter = operation;
        strncpy(cmd, "d#opeshut", DOME_CMD);
    }
    else
    {
        targetShutter = operation;
        strncpy(cmd, "d#closhut", DOME_CMD);
    }

    tcflush(PortFD, TCIOFLUSH);

    if (!sim && (rc = tty_write(PortFD, cmd, DOME_CMD, &nbytes_written)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s ControlDomeShutter error: %s.", cmd, errstr);
        return IPS_ALERT;
    }

    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD (%s)", cmd);

    if (sim)
    {
        simShutterTimer = SIM_SHUTTER_TIMER;
        strncpy(resp, "d#gotmess", DOME_CMD);
        nbytes_read = DOME_CMD;
    }
    else if ((rc = tty_read(PortFD, resp, DOME_CMD, DOME_TIMEOUT, &nbytes_read)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "ControlDomeShutter error: %s.", errstr);
        return IPS_ALERT;
    }

    resp[nbytes_read] = '\0';

    DEBUGF(INDI::Logger::DBG_DEBUG, "RES (%s)", resp);

    if (strcmp(resp, "d#gotmess") == 0)
    {
        shutterState = simShutterStatus = SHUTTER_MOVING;
        return IPS_BUSY;
    }
    return IPS_ALERT;
}

/************************************************************************************
 *
* ***********************************************************************************/
bool BaaderDome::Abort()
{
    DEBUGF(INDI::Logger::DBG_SESSION, "Attempting to abort dome motion by stopping at %g", DomeAbsPosN[0].value);
    MoveAbs(DomeAbsPosN[0].value);
    return true;
}

/************************************************************************************
 *
* ***********************************************************************************/
const char *BaaderDome::GetFlapStatusString(FlapStatus status)
{
    switch (status)
    {
        case FLAP_OPENED:
            return "Flap is open.";
            break;
        case FLAP_CLOSED:
            return "Flap is closed.";
            break;
        case FLAP_MOVING:
            return "Flap is in motion.";
            break;
        case FLAP_UNKNOWN:
        default:
            return "Flap status is unknown.";
            break;
    }
}

/************************************************************************************
 *
* ***********************************************************************************/
int BaaderDome::ControlDomeFlap(FlapOperation operation)
{
    int nbytes_written = 0, nbytes_read = 0, rc = -1;
    char errstr[MAXRBUF];
    char cmd[DOME_BUF];
    char resp[DOME_BUF];

    memset(cmd, 0, sizeof(cmd));

    if (operation == FLAP_OPEN)
    {
        targetFlap = operation;
        strncpy(cmd, "d#opeflap", DOME_CMD);
    }
    else
    {
        targetFlap = operation;
        strncpy(cmd, "d#cloflap", DOME_CMD);
    }

    tcflush(PortFD, TCIOFLUSH);

    if (!sim && (rc = tty_write(PortFD, cmd, DOME_CMD, &nbytes_written)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s ControlDomeFlap error: %s.", cmd, errstr);
        return -1;
    }

    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD (%s)", cmd);

    if (sim)
    {
        simFlapTimer = SIM_FLAP_TIMER;
        strncpy(resp, "d#gotmess", DOME_CMD);
        nbytes_read = DOME_CMD;
    }
    else if ((rc = tty_read(PortFD, resp, DOME_CMD, DOME_TIMEOUT, &nbytes_read)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "ControlDomeFlap error: %s.", errstr);
        return -1;
    }

    resp[nbytes_read] = '\0';

    DEBUGF(INDI::Logger::DBG_DEBUG, "RES (%s)", resp);

    if (strcmp(resp, "d#gotmess") == 0)
    {
        flapStatus = simFlapStatus = FLAP_MOVING;
        return 1;
    }
    return -1;
}

/************************************************************************************
 *
* ***********************************************************************************/
bool BaaderDome::UpdateFlapStatus()
{
    int nbytes_written = 0, nbytes_read = 0, rc = -1;
    char errstr[MAXRBUF];
    char resp[DOME_BUF];
    char status[DOME_BUF];

    tcflush(PortFD, TCIOFLUSH);

    if (!sim && (rc = tty_write(PortFD, "d#getflap", DOME_CMD, &nbytes_written)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "d#getflap UpdateflapStatus error: %s.", errstr);
        return false;
    }

    DEBUG(INDI::Logger::DBG_DEBUG, "CMD (d#getflap)");

    if (sim)
    {
        if (simFlapStatus == FLAP_CLOSED)
            strncpy(resp, "d#flapclo", DOME_CMD);
        else if (simFlapStatus == FLAP_OPENED)
            strncpy(resp, "d#flapope", DOME_CMD);
        else if (simFlapStatus == FLAP_MOVING)
            strncpy(resp, "d#flaprun", DOME_CMD);
        nbytes_read = DOME_CMD;
    }
    else if ((rc = tty_read(PortFD, resp, DOME_CMD, DOME_TIMEOUT, &nbytes_read)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "UpdateflapStatus error: %s.", errstr);
        return false;
    }

    resp[nbytes_read] = '\0';

    DEBUGF(INDI::Logger::DBG_DEBUG, "RES (%s)", resp);

    rc = sscanf(resp, "d#flap%s", status);

    if (rc > 0)
    {
        DomeFlapSP.s = IPS_OK;
        IUResetSwitch(&DomeFlapSP);

        if (strcmp(status, "ope") == 0)
        {
            if (flapStatus == FLAP_MOVING && targetFlap == FLAP_OPEN)
                DEBUGF(INDI::Logger::DBG_SESSION, "%s", GetFlapStatusString(FLAP_OPENED));

            flapStatus             = FLAP_OPENED;
            DomeFlapS[FLAP_OPEN].s = ISS_ON;
        }
        else if (strcmp(status, "clo") == 0)
        {
            if (flapStatus == FLAP_MOVING && targetFlap == FLAP_CLOSE)
                DEBUGF(INDI::Logger::DBG_SESSION, "%s", GetFlapStatusString(FLAP_CLOSED));

            flapStatus              = FLAP_CLOSED;
            DomeFlapS[FLAP_CLOSE].s = ISS_ON;
        }
        else if (strcmp(status, "run") == 0)
        {
            flapStatus   = FLAP_MOVING;
            DomeFlapSP.s = IPS_BUSY;
        }
        else
        {
            flapStatus   = FLAP_UNKNOWN;
            DomeFlapSP.s = IPS_ALERT;
            DEBUGF(INDI::Logger::DBG_ERROR, "Unknown flap status: %s.", resp);
        }
        return true;
    }
    return false;
}

/************************************************************************************
 *
* ***********************************************************************************/
bool BaaderDome::SaveEncoderPosition()
{
    int nbytes_written = 0, nbytes_read = 0, rc = -1;
    char errstr[MAXRBUF];
    char cmd[DOME_BUF];
    char resp[DOME_BUF];

    strncpy(cmd, "d#encsave", DOME_CMD);

    tcflush(PortFD, TCIOFLUSH);

    if (!sim && (rc = tty_write(PortFD, cmd, DOME_CMD, &nbytes_written)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s SaveEncoderPosition error: %s.", cmd, errstr);
        return false;
    }

    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD (%s)", cmd);

    if (sim)
    {
        strncpy(resp, "d#gotmess", DOME_CMD);
        nbytes_read = DOME_CMD;
    }
    else if ((rc = tty_read(PortFD, resp, DOME_CMD, DOME_TIMEOUT, &nbytes_read)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "SaveEncoderPosition error: %s.", errstr);
        return false;
    }

    resp[nbytes_read] = '\0';

    DEBUGF(INDI::Logger::DBG_DEBUG, "RES (%s)", resp);

    return strcmp(resp, "d#gotmess") == 0;
}

/************************************************************************************
 *
* ***********************************************************************************/
bool BaaderDome::saveConfigItems(FILE *fp)
{
    // Only save if calibration is complete
    if (calibrationStage == CALIBRATION_COMPLETE)
        SaveEncoderPosition();

    return INDI::Dome::saveConfigItems(fp);
}

/************************************************************************************
 *
* ***********************************************************************************/
bool BaaderDome::SetCurrentPark()
{
    SetAxis1Park(DomeAbsPosN[0].value);
    return true;
}
/************************************************************************************
 *
* ***********************************************************************************/

bool BaaderDome::SetDefaultPark()
{
    // By default set position to 90
    SetAxis1Park(90);
    return true;
}

/*
    Moonlite Focuser
    Copyright (C) 2013 Jasem Mutlaq (mutlaqja@ikarustech.com)

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

#include "moonlite.h"

#include "indicom.h"

#include <cmath>
#include <cstring>
#include <memory>

#include <termios.h>
#include <unistd.h>

#define MOONLITE_TIMEOUT 3

#define POLLMS 250

std::unique_ptr<MoonLite> moonLite(new MoonLite());

void ISGetProperties(const char *dev)
{
    moonLite->ISGetProperties(dev);
}

void ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    moonLite->ISNewSwitch(dev, name, states, names, n);
}

void ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n)
{
    moonLite->ISNewText(dev, name, texts, names, n);
}

void ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    moonLite->ISNewNumber(dev, name, values, names, n);
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
    moonLite->ISSnoopDevice(root);
}

MoonLite::MoonLite()
{
    // Can move in Absolute & Relative motions, can AbortFocuser motion, and has variable speed.
    SetFocuserCapability(FOCUSER_CAN_ABS_MOVE | FOCUSER_CAN_REL_MOVE | FOCUSER_CAN_ABORT | FOCUSER_HAS_VARIABLE_SPEED);
}

bool MoonLite::initProperties()
{
    INDI::Focuser::initProperties();

    FocusSpeedN[0].min   = 1;
    FocusSpeedN[0].max   = 5;
    FocusSpeedN[0].value = 1;

    /* Step Mode */
    IUFillSwitch(&StepModeS[0], "Half Step", "", ISS_OFF);
    IUFillSwitch(&StepModeS[1], "Full Step", "", ISS_ON);
    IUFillSwitchVector(&StepModeSP, StepModeS, 2, getDeviceName(), "Step Mode", "", OPTIONS_TAB, IP_RW, ISR_1OFMANY, 0,
                       IPS_IDLE);

    /* Focuser temperature */
    IUFillNumber(&TemperatureN[0], "TEMPERATURE", "Celsius", "%6.2f", -50, 70., 0., 0.);
    IUFillNumberVector(&TemperatureNP, TemperatureN, 1, getDeviceName(), "FOCUS_TEMPERATURE", "Temperature",
                       MAIN_CONTROL_TAB, IP_RO, 0, IPS_IDLE);

    // Maximum Travel
    IUFillNumber(&MaxTravelN[0], "MAXTRAVEL", "Maximum travel", "%6.0f", 1., 60000., 0., 10000.);
    IUFillNumberVector(&MaxTravelNP, MaxTravelN, 1, getDeviceName(), "FOCUS_MAXTRAVEL", "Max. travel", OPTIONS_TAB,
                       IP_RW, 0, IPS_IDLE);

    // Temperature Settings
    IUFillNumber(&TemperatureSettingN[0], "Calibration", "", "%6.2f", -20, 20, 0.5, 0);
    IUFillNumber(&TemperatureSettingN[1], "Coefficient", "", "%6.2f", -20, 20, 0.5, 0);
    IUFillNumberVector(&TemperatureSettingNP, TemperatureSettingN, 2, getDeviceName(), "Temperature Settings", "",
                       OPTIONS_TAB, IP_RW, 0, IPS_IDLE);

    // Compensate for temperature
    IUFillSwitch(&TemperatureCompensateS[0], "Enable", "", ISS_OFF);
    IUFillSwitch(&TemperatureCompensateS[1], "Disable", "", ISS_ON);
    IUFillSwitchVector(&TemperatureCompensateSP, TemperatureCompensateS, 2, getDeviceName(), "Temperature Compensate",
                       "", MAIN_CONTROL_TAB, IP_RW, ISR_1OFMANY, 0, IPS_IDLE);

    // Sync
    IUFillNumber(&SyncN[0], "FOCUS_SYNC_OFFSET", "Offset", "%6.0f", 0, 60000., 0., 0.);
    IUFillNumberVector(&SyncNP, SyncN, 1, getDeviceName(), "FOCUS_SYNC", "Sync", MAIN_CONTROL_TAB, IP_RW, 0, IPS_IDLE);

    /* Relative and absolute movement */
    FocusRelPosN[0].min   = 0.;
    FocusRelPosN[0].max   = 30000.;
    FocusRelPosN[0].value = 0;
    FocusRelPosN[0].step  = 1000;

    FocusAbsPosN[0].min   = 0.;
    FocusAbsPosN[0].max   = 60000.;
    FocusAbsPosN[0].value = 0;
    FocusAbsPosN[0].step  = 1000;

    addDebugControl();

    updatePeriodMS = POLLMS;

    return true;
}

bool MoonLite::updateProperties()
{
    INDI::Focuser::updateProperties();

    if (isConnected())
    {
        defineNumber(&TemperatureNP);
        defineNumber(&MaxTravelNP);
        defineSwitch(&StepModeSP);
        defineNumber(&TemperatureSettingNP);
        defineSwitch(&TemperatureCompensateSP);
        defineNumber(&SyncNP);

        GetFocusParams();

        DEBUG(INDI::Logger::DBG_SESSION, "MoonLite paramaters updated, focuser ready for use.");
    }
    else
    {
        deleteProperty(TemperatureNP.name);
        deleteProperty(MaxTravelNP.name);
        deleteProperty(StepModeSP.name);
        deleteProperty(TemperatureSettingNP.name);
        deleteProperty(TemperatureCompensateSP.name);
        deleteProperty(SyncNP.name);
    }

    return true;
}

bool MoonLite::Handshake()
{
    if (Ack())
    {
        DEBUG(INDI::Logger::DBG_SESSION, "MoonLite is online. Getting focus parameters...");
        return true;
    }

    DEBUG(INDI::Logger::DBG_SESSION,
          "Error retreiving data from MoonLite, please ensure MoonLite controller is powered and the port is correct.");
    return false;
}

const char *MoonLite::getDefaultName()
{
    return "MoonLite";
}

bool MoonLite::Ack()
{
    int nbytes_written = 0, nbytes_read = 0, rc = -1;
    char errstr[MAXRBUF];
    char resp[5]={0};
    short pos = -1;

    tcflush(PortFD, TCIOFLUSH);

    if ((rc = tty_write(PortFD, ":GP#", 4, &nbytes_written)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "updatePostion error: %s.", errstr);
        return false;
    }

    if ((rc = tty_read(PortFD, resp, 5, 2, &nbytes_read)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "updatePostion error: %s.", errstr);
        return false;
    }

    tcflush(PortFD, TCIOFLUSH);

    rc = sscanf(resp, "%hX#", &pos);

    return rc > 0;
}

bool MoonLite::updateStepMode()
{
    int nbytes_written = 0, nbytes_read = 0, rc = -1;
    char errstr[MAXRBUF];
    char resp[4]={0};

    tcflush(PortFD, TCIOFLUSH);

    if ((rc = tty_write(PortFD, ":GH#", 4, &nbytes_written)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "updateStepMode error: %s.", errstr);
        return false;
    }

    if ((rc = tty_read(PortFD, resp, 3, MOONLITE_TIMEOUT, &nbytes_read)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "updateStepMode error: %s.", errstr);
        return false;
    }

    tcflush(PortFD, TCIOFLUSH);

    resp[3] = '\0';
    IUResetSwitch(&StepModeSP);

    if (strcmp(resp, "FF#") == 0)
        StepModeS[0].s = ISS_ON;
    else if (strcmp(resp, "00#") == 0)
        StepModeS[1].s = ISS_ON;
    else
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "Unknown error: focuser step value (%s)", resp);
        return false;
    }

    return true;
}

bool MoonLite::updateTemperature()
{
    int nbytes_written = 0, nbytes_read = 0, rc = -1;
    char errstr[MAXRBUF];
    char resp[5]={0};
    unsigned int temp;

    tcflush(PortFD, TCIOFLUSH);

    tty_write(PortFD, ":C#", 3, &nbytes_written);

    if ((rc = tty_write(PortFD, ":GT#", 4, &nbytes_written)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "updateTemperature error: %s.", errstr);
        return false;
    }

    if ((rc = tty_read(PortFD, resp, 5, MOONLITE_TIMEOUT, &nbytes_read)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "updateTemperature error: %s.", errstr);
        return false;
    }

    tcflush(PortFD, TCIOFLUSH);

    rc = sscanf(resp, "%X#", &temp);

    if (rc > 0)
    {
        TemperatureN[0].value = ((int)temp) / 2.0;
    }
    else
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "Unknown error: focuser temperature value (%s)", resp);
        return false;
    }

    return true;
}

bool MoonLite::updatePosition()
{
    int nbytes_written = 0, nbytes_read = 0, rc = -1;
    char errstr[MAXRBUF];
    char resp[5]={0};
    int pos = -1;

    tcflush(PortFD, TCIOFLUSH);

    if ((rc = tty_write(PortFD, ":GP#", 4, &nbytes_written)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "updatePostion error: %s.", errstr);
        return false;
    }

    if ((rc = tty_read(PortFD, resp, 5, MOONLITE_TIMEOUT, &nbytes_read)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "updatePostion error: %s.", errstr);
        return false;
    }

    tcflush(PortFD, TCIOFLUSH);

    rc = sscanf(resp, "%X#", &pos);

    if (rc > 0)
    {
        FocusAbsPosN[0].value = pos;
    }
    else
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "Unknown error: focuser position value (%s)", resp);
        return false;
    }

    return true;
}

bool MoonLite::updateSpeed()
{
    int nbytes_written = 0, nbytes_read = 0, rc = -1;
    char errstr[MAXRBUF];
    char resp[3]={0};
    short speed;

    tcflush(PortFD, TCIOFLUSH);

    if ((rc = tty_write(PortFD, ":GD#", 4, &nbytes_written)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "updateSpeed error: %s.", errstr);
        return false;
    }

    if ((rc = tty_read(PortFD, resp, 3, MOONLITE_TIMEOUT, &nbytes_read)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "updateSpeed error: %s.", errstr);
        return false;
    }

    tcflush(PortFD, TCIOFLUSH);

    rc = sscanf(resp, "%hX#", &speed);

    if (rc > 0)
    {
        int focus_speed = -1;
        while (speed > 0)
        {
            speed >>= 1;
            focus_speed++;
        }

        currentSpeed         = focus_speed;
        FocusSpeedN[0].value = focus_speed;
    }
    else
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "Unknown error: focuser speed value (%s)", resp);
        return false;
    }

    return true;
}

bool MoonLite::isMoving()
{
    int nbytes_written = 0, nbytes_read = 0, rc = -1;
    char errstr[MAXRBUF];
    char resp[4]={0};

    tcflush(PortFD, TCIOFLUSH);

    if ((rc = tty_write(PortFD, ":GI#", 4, &nbytes_written)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "isMoving error: %s.", errstr);
        return false;
    }

    if ((rc = tty_read(PortFD, resp, 3, MOONLITE_TIMEOUT, &nbytes_read)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "isMoving error: %s.", errstr);
        return false;
    }

    tcflush(PortFD, TCIOFLUSH);

    resp[3] = '\0';
    if (strcmp(resp, "01#") == 0)
        return true;
    else if (strcmp(resp, "00#") == 0)
        return false;

    DEBUGF(INDI::Logger::DBG_ERROR, "Unknown error: isMoving value (%s)", resp);
    return false;
}

bool MoonLite::setTemperatureCalibration(double calibration)
{
    int nbytes_written = 0, rc = -1;
    char errstr[MAXRBUF];
    char cmd[7]={0};
    int cal = calibration * 2;

    snprintf(cmd, 7, ":PO%02X#", cal);

    tcflush(PortFD, TCIOFLUSH);

    if ((rc = tty_write(PortFD, cmd, 6, &nbytes_written)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "setTemperatureCalibration error: %s.", errstr);
        return false;
    }

    return true;
}

bool MoonLite::setTemperatureCoefficient(double coefficient)
{
    int nbytes_written = 0, rc = -1;
    char errstr[MAXRBUF];
    char cmd[7]={0};

    int coeff = coefficient * 2;

    snprintf(cmd, 7, ":SC%02X#", coeff);

    tcflush(PortFD, TCIOFLUSH);

    if ((rc = tty_write(PortFD, cmd, 6, &nbytes_written)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "setTemperatureCoefficient error: %s.", errstr);
        return false;
    }

    return true;
}

bool MoonLite::sync(uint16_t offset)
{
    int nbytes_written = 0, rc = -1;
    char errstr[MAXRBUF];
    char cmd[9]={0};

    snprintf(cmd, 9, ":SP%04X#", offset);

    // Set Position
    if ((rc = tty_write(PortFD, cmd, 8, &nbytes_written)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "reset error: %s.", errstr);
        return false;
    }

    return true;
}

bool MoonLite::MoveFocuser(unsigned int position)
{
    int nbytes_written = 0, rc = -1;
    char errstr[MAXRBUF];
    char cmd[9]={0};

    if (position < FocusAbsPosN[0].min || position > FocusAbsPosN[0].max)
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "Requested position value out of bound: %d", position);
        return false;
    }

    /*if (fabs(position - FocusAbsPosN[0].value) > MaxTravelN[0].value)
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "Requested position value of %d exceeds maximum travel limit of %g", position, MaxTravelN[0].value);
        return false;
    }*/

    snprintf(cmd, 9, ":SN%04X#", position);

    // Set Position
    if ((rc = tty_write(PortFD, cmd, 8, &nbytes_written)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "setPosition error: %s.", errstr);
        return false;
    }

    // MoveFocuser to Position
    if ((rc = tty_write(PortFD, ":FG#", 4, &nbytes_written)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "MoveFocuser error: %s.", errstr);
        return false;
    }

    return true;
}

bool MoonLite::setStepMode(FocusStepMode mode)
{
    int nbytes_written = 0, rc = -1;
    char errstr[MAXRBUF];
    char cmd[4]={0};

    tcflush(PortFD, TCIOFLUSH);

    if (mode == FOCUS_HALF_STEP)
        strncpy(cmd, ":SH#", 4);
    else
        strncpy(cmd, ":SF#", 4);

    if ((rc = tty_write(PortFD, cmd, 4, &nbytes_written)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "setStepMode error: %s.", errstr);
        return false;
    }

    return true;
}

bool MoonLite::setSpeed(unsigned short speed)
{
    int nbytes_written = 0, rc = -1;
    char errstr[MAXRBUF];
    char cmd[7]={0};

    int hex_value = 1;

    hex_value <<= speed;

    snprintf(cmd, 7, ":SD%02X#", hex_value);

    if ((rc = tty_write(PortFD, cmd, 6, &nbytes_written)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "setSpeed error: %s.", errstr);
        return false;
    }

    return true;
}

bool MoonLite::setTemperatureCompensation(bool enable)
{
    int nbytes_written = 0, rc = -1;
    char errstr[MAXRBUF];
    char cmd[4]={0};

    tcflush(PortFD, TCIOFLUSH);

    if (enable)
        strncpy(cmd, ":+#", 4);
    else
        strncpy(cmd, ":-#", 4);

    if ((rc = tty_write(PortFD, cmd, 3, &nbytes_written)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "setTemperatureCompensation error: %s.", errstr);
        return false;
    }

    return true;
}

bool MoonLite::ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    if (dev != nullptr && strcmp(dev, getDeviceName()) == 0)
    {
        // Focus Step Mode
        if (strcmp(StepModeSP.name, name) == 0)
        {
            bool rc          = false;
            int current_mode = IUFindOnSwitchIndex(&StepModeSP);

            IUUpdateSwitch(&StepModeSP, states, names, n);

            int target_mode = IUFindOnSwitchIndex(&StepModeSP);

            if (current_mode == target_mode)
            {
                StepModeSP.s = IPS_OK;
                IDSetSwitch(&StepModeSP, nullptr);
            }

            if (target_mode == 0)
                rc = setStepMode(FOCUS_HALF_STEP);
            else
                rc = setStepMode(FOCUS_FULL_STEP);

            if (!rc)
            {
                IUResetSwitch(&StepModeSP);
                StepModeS[current_mode].s = ISS_ON;
                StepModeSP.s              = IPS_ALERT;
                IDSetSwitch(&StepModeSP, nullptr);
                return false;
            }

            StepModeSP.s = IPS_OK;
            IDSetSwitch(&StepModeSP, nullptr);
            return true;
        }

        if (strcmp(TemperatureCompensateSP.name, name) == 0)
        {
            int last_index = IUFindOnSwitchIndex(&TemperatureCompensateSP);
            IUUpdateSwitch(&TemperatureCompensateSP, states, names, n);

            bool rc = setTemperatureCompensation((TemperatureCompensateS[0].s == ISS_ON));

            if (!rc)
            {
                TemperatureCompensateSP.s = IPS_ALERT;
                IUResetSwitch(&TemperatureCompensateSP);
                TemperatureCompensateS[last_index].s = ISS_ON;
                IDSetSwitch(&TemperatureCompensateSP, nullptr);
                return false;
            }

            TemperatureCompensateSP.s = IPS_OK;
            IDSetSwitch(&TemperatureCompensateSP, nullptr);
            return true;
        }
    }

    return INDI::Focuser::ISNewSwitch(dev, name, states, names, n);
}

bool MoonLite::ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    if (dev != nullptr && strcmp(dev, getDeviceName()) == 0)
    {
        if (strcmp(name, SyncNP.name) == 0)
        {
            IUUpdateNumber(&SyncNP, values, names, n);
            if (sync(SyncN[0].value))
                SyncNP.s = IPS_OK;
            else
                SyncNP.s = IPS_ALERT;
            IDSetNumber(&SyncNP, nullptr);
            return true;
        }

        if (strcmp(name, MaxTravelNP.name) == 0)
        {
            IUUpdateNumber(&MaxTravelNP, values, names, n);
            MaxTravelNP.s = IPS_OK;
            IDSetNumber(&MaxTravelNP, nullptr);
            return true;
        }

        if (strcmp(name, TemperatureSettingNP.name) == 0)
        {
            IUUpdateNumber(&TemperatureSettingNP, values, names, n);
            if (!setTemperatureCalibration(TemperatureSettingN[0].value) ||
                !setTemperatureCoefficient(TemperatureSettingN[1].value))
            {
                TemperatureSettingNP.s = IPS_ALERT;
                IDSetNumber(&TemperatureSettingNP, nullptr);
                return false;
            }

            TemperatureSettingNP.s = IPS_OK;
            IDSetNumber(&TemperatureSettingNP, nullptr);
            return true;
        }
    }

    return INDI::Focuser::ISNewNumber(dev, name, values, names, n);
}

void MoonLite::GetFocusParams()
{
    if (updatePosition())
        IDSetNumber(&FocusAbsPosNP, nullptr);

    if (updateTemperature())
        IDSetNumber(&TemperatureNP, nullptr);

    if (updateSpeed())
        IDSetNumber(&FocusSpeedNP, nullptr);

    if (updateStepMode())
        IDSetSwitch(&StepModeSP, nullptr);
}

bool MoonLite::SetFocuserSpeed(int speed)
{
    bool rc = false;

    rc = setSpeed(speed);

    if (!rc)
        return false;

    currentSpeed = speed;

    FocusSpeedNP.s = IPS_OK;
    IDSetNumber(&FocusSpeedNP, nullptr);

    return true;
}

IPState MoonLite::MoveFocuser(FocusDirection dir, int speed, uint16_t duration)
{
    if (speed != (int)currentSpeed)
    {
        bool rc = setSpeed(speed);

        if (!rc)
            return IPS_ALERT;
    }

    gettimeofday(&focusMoveStart, nullptr);
    focusMoveRequest = duration / 1000.0;

    if (dir == FOCUS_INWARD)
        MoveFocuser(0);
    else
        MoveFocuser(FocusAbsPosN[0].value + MaxTravelN[0].value - 1);

    if (duration <= POLLMS)
    {
        usleep(duration * 1000);
        AbortFocuser();
        return IPS_OK;
    }

    return IPS_BUSY;
}

IPState MoonLite::MoveAbsFocuser(uint32_t targetTicks)
{
    targetPos = targetTicks;

    bool rc = false;

    rc = MoveFocuser(targetPos);

    if (!rc)
        return IPS_ALERT;

    FocusAbsPosNP.s = IPS_BUSY;

    return IPS_BUSY;
}

IPState MoonLite::MoveRelFocuser(FocusDirection dir, uint32_t ticks)
{
    double newPosition = 0;
    bool rc            = false;

    if (dir == FOCUS_INWARD)
        newPosition = FocusAbsPosN[0].value - ticks;
    else
        newPosition = FocusAbsPosN[0].value + ticks;

    rc = MoveFocuser(newPosition);

    if (!rc)
        return IPS_ALERT;

    FocusRelPosN[0].value = ticks;
    FocusRelPosNP.s       = IPS_BUSY;

    return IPS_BUSY;
}

void MoonLite::TimerHit()
{
    if (!isConnected())
    {
        SetTimer(POLLMS);
        return;
    }

    bool rc = updatePosition();
    if (rc)
    {
        if (fabs(lastPos - FocusAbsPosN[0].value) > 5)
        {
            IDSetNumber(&FocusAbsPosNP, nullptr);
            lastPos = FocusAbsPosN[0].value;
        }
    }

    rc = updateTemperature();
    if (rc)
    {
        if (fabs(lastTemperature - TemperatureN[0].value) >= 0.5)
        {
            IDSetNumber(&TemperatureNP, nullptr);
            lastTemperature = TemperatureN[0].value;
        }
    }

    if (FocusTimerNP.s == IPS_BUSY)
    {
        float remaining = CalcTimeLeft(focusMoveStart, focusMoveRequest);

        if (remaining <= 0)
        {
            FocusTimerNP.s       = IPS_OK;
            FocusTimerN[0].value = 0;
            AbortFocuser();
        }
        else
            FocusTimerN[0].value = remaining * 1000.0;

        IDSetNumber(&FocusTimerNP, nullptr);
    }

    if (FocusAbsPosNP.s == IPS_BUSY || FocusRelPosNP.s == IPS_BUSY)
    {
        if (!isMoving())
        {
            FocusAbsPosNP.s = IPS_OK;
            FocusRelPosNP.s = IPS_OK;
            IDSetNumber(&FocusAbsPosNP, nullptr);
            IDSetNumber(&FocusRelPosNP, nullptr);
            lastPos = FocusAbsPosN[0].value;
            DEBUG(INDI::Logger::DBG_SESSION, "Focuser reached requested position.");
        }
    }

    SetTimer(POLLMS);
}

bool MoonLite::AbortFocuser()
{
    int nbytes_written;
    if (tty_write(PortFD, ":FQ#", 4, &nbytes_written) == TTY_OK)
    {
        FocusAbsPosNP.s = IPS_IDLE;
        FocusRelPosNP.s = IPS_IDLE;
        IDSetNumber(&FocusAbsPosNP, nullptr);
        IDSetNumber(&FocusRelPosNP, nullptr);
        return true;
    }
    else
        return false;
}

float MoonLite::CalcTimeLeft(timeval start, float req)
{
    double timesince;
    double timeleft;
    struct timeval now { 0, 0 };
    gettimeofday(&now, nullptr);

    timesince =
        (double)(now.tv_sec * 1000.0 + now.tv_usec / 1000) - (double)(start.tv_sec * 1000.0 + start.tv_usec / 1000);
    timesince = timesince / 1000;
    timeleft  = req - timesince;
    return timeleft;
}

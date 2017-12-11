/*
    Optec Pyrix Rotator
    Copyright (C) 2017 Jasem Mutlaq (mutlaqja@ikarustech.com)

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

#include "pyxis.h"

#include "indicom.h"
#include "connectionplugins/connectionserial.h"

#include <cmath>
#include <cstring>
#include <memory>
#include <termios.h>

#define PYXIS_TIMEOUT 3
#define PYRIX_BUF 7
#define PYRIX_CMD 6
#define POLLMS 1000
#define SETTINGS_TAB    "Settings"

std::unique_ptr<Pyxis> pyxis(new Pyxis());

void ISGetProperties(const char *dev)
{
    pyxis->ISGetProperties(dev);
}

void ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    pyxis->ISNewSwitch(dev, name, states, names, n);
}

void ISNewText(	const char *dev, const char *name, char *texts[], char *names[], int n)
{
    pyxis->ISNewText(dev, name, texts, names, n);
}

void ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    pyxis->ISNewNumber(dev, name, values, names, n);
}

void ISNewBLOB (const char *dev, const char *name, int sizes[], int blobsizes[], char *blobs[],
                char *formats[], char *names[], int n)
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

void ISSnoopDevice (XMLEle *root)
{
    pyxis->ISSnoopDevice(root);
}

Pyxis::Pyxis()
{
    // We do not have absolute ticks
    SetRotatorCapability(ROTATOR_CAN_HOME | ROTATOR_CAN_REVERSE);

    setRotatorConnection(CONNECTION_SERIAL);
}

bool Pyxis::initProperties()
{
    INDI::Rotator::initProperties();

    // Rotation Rate
    IUFillNumber(&RotationRateN[0], "RATE", "Rate", "%.f", 0, 99, 10, 8);
    IUFillNumberVector(&RotationRateNP, RotationRateN, 1, getDeviceName(), "ROTATION_RATE", "Rotation", SETTINGS_TAB, IP_RW, 0, IPS_IDLE);

    // Stepping
    IUFillSwitch(&SteppingS[FULL_STEP], "FULL_STEP", "Full", ISS_OFF);
    IUFillSwitch(&SteppingS[HALF_STEP], "HALF_STEP", "Half", ISS_OFF);
    IUFillSwitchVector(&SteppingSP, SteppingS, 2, getDeviceName(), "STEPPING_RATE", "Stepping", SETTINGS_TAB, IP_RW, ISR_ATMOST1, 0, IPS_IDLE);

    // Power
    IUFillSwitch(&PowerS[POWER_SLEEP], "POWER_SLEEP", "Sleep", ISS_OFF);
    IUFillSwitch(&PowerS[POWER_WAKEUP], "POWER_WAKEUP", "Wake Up", ISS_OFF);
    IUFillSwitchVector(&PowerSP, PowerS, 2, getDeviceName(), "POWER_STATE", "Power", SETTINGS_TAB, IP_RW, ISR_ATMOST1, 0, IPS_IDLE);

    updatePeriodMS = POLLMS;

    serialConnection->setDefaultBaudRate(Connection::Serial::B_19200);

    return true;
}

bool Pyxis::Handshake()
{
    if (Ack())
        return true;

    DEBUG(INDI::Logger::DBG_SESSION, "Error retreiving data from Pyrix, please ensure Pyrix controller is powered and the port is correct.");
    return false;
}

const char * Pyxis::getDefaultName()
{
    return "Pyxis";
}

bool Pyxis::updateProperties()
{
    INDI::Rotator::updateProperties();

    if (isConnected())
    {
        defineNumber(&RotationRateNP);
        defineSwitch(&SteppingSP);
        defineSwitch(&PowerSP);

        queryParams();
    }
    else
    {
        deleteProperty(RotationRateNP.name);
        deleteProperty(SteppingSP.name);
        deleteProperty(PowerSP.name);
    }

    return true;
}

void Pyxis::queryParams()
{
    ////////////////////////////////////////////
    // Reverse Parameter
    ////////////////////////////////////////////
    int dir = getReverseStatus();
    IUResetSwitch(&ReverseRotatorSP);
    ReverseRotatorSP.s = IPS_OK;
    if (dir == 0)
        ReverseRotatorS[REVERSE_DISABLED].s = ISS_ON;
    else if (dir == 1)
        ReverseRotatorS[REVERSE_ENABLED].s = ISS_ON;
    else
        ReverseRotatorSP.s = IPS_ALERT;

    IDSetSwitch(&ReverseRotatorSP, nullptr);

}

bool Pyxis::Ack()
{
    const char *cmd = "CCLINK";
    char res[1] = {0};

    int nbytes_written = 0, nbytes_read = 0, rc = -1;
    char errstr[MAXRBUF];

    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD <%s>", cmd);

    tcflush(PortFD, TCIOFLUSH);

    if ( (rc = tty_write(PortFD, cmd, PYRIX_CMD, &nbytes_written)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s: %s.", __FUNCTION__, errstr);
        return false;
    }

    if ( (rc = tty_read(PortFD, res, 1, PYXIS_TIMEOUT, &nbytes_read)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s error: %s.", __FUNCTION__, errstr);
        return false;
    }

    DEBUGF(INDI::Logger::DBG_DEBUG, "RES <%c>", res[0]);

    tcflush(PortFD, TCIOFLUSH);

    if (res[0] != '!')
    {
        DEBUG(INDI::Logger::DBG_ERROR, "Cannot establish communication. Check power is on and homing is complete.");
        return false;
    }

    return true;
}

bool Pyxis::ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    if (dev != nullptr && strcmp(dev, getDeviceName()) == 0)
    {
        if (!strcmp(name, RotationRateNP.name))
        {
           bool rc = setRotationRate(static_cast<uint8_t>(values[0]));
           if (rc)
           {
               RotationRateNP.s = IPS_OK;
               RotationRateN[0].value = values[0];
           }
           else
               RotationRateNP.s = IPS_ALERT;

           IDSetNumber(&RotationRateNP, nullptr);
           return true;
        }
    }

    return Rotator::ISNewNumber(dev, name, values, names, n);

}

bool Pyxis::ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    if (dev != nullptr && strcmp(dev, getDeviceName()) == 0)
    {
        /////////////////////////////////////////////////////
        // Stepping
        ////////////////////////////////////////////////////
        if (!strcmp(name, SteppingSP.name))
        {
            bool rc = false;
            if (!strcmp(IUFindOnSwitchName(states, names, n), SteppingS[FULL_STEP].name))
                rc = setSteppingMode(FULL_STEP);
            else
                rc = setSteppingMode(HALF_STEP);


            if (rc)
            {
                IUUpdateSwitch(&SteppingSP, states, names, n);
                SteppingSP.s = IPS_OK;
            }
            else
                SteppingSP.s = IPS_ALERT;

            IDSetSwitch(&SteppingSP, nullptr);
            return true;
        }

        /////////////////////////////////////////////////////
        // Power
        ////////////////////////////////////////////////////
        if (!strcmp(name, PowerSP.name))
        {
            bool rc = false;
            if (!strcmp(IUFindOnSwitchName(states, names, n), PowerS[POWER_WAKEUP].name))
            {
                // If not sleeping
                if (PowerS[POWER_SLEEP].s == ISS_OFF)
                {
                    PowerSP.s = IPS_OK;
                    DEBUG(INDI::Logger::DBG_WARNING, "Controller is not in sleep mode.");
                    IDSetSwitch(&PowerSP, nullptr);
                    return true;
                }

                rc = wakeupController();

                if (rc)
                {
                    IUResetSwitch(&PowerSP);
                    PowerSP.s = IPS_OK;
                    DEBUG(INDI::Logger::DBG_SESSION, "Controller is awake.");
                }
                else
                    PowerSP.s = IPS_ALERT;

                IDSetSwitch(&PowerSP, nullptr);
                return true;
            }
            else
            {
                bool rc = sleepController();
                IUResetSwitch(&PowerSP);
                if (rc)
                {
                   PowerSP.s = IPS_OK;
                   PowerS[POWER_SLEEP].s = ISS_ON;
                   DEBUG(INDI::Logger::DBG_SESSION, "Controller in sleep mode. No functions can be used until controller is waken up.");
                }
                else
                    PowerSP.s = IPS_ALERT;

                IDSetSwitch(&PowerSP, nullptr);
                return true;
            }

            return true;
        }
    }

    return Rotator::ISNewSwitch(dev, name, states, names, n);
}

bool Pyxis::setSteppingMode(uint8_t mode)
{
    char cmd[PYRIX_BUF] = {0};

    int nbytes_written = 0, rc = -1;
    char errstr[MAXRBUF];

    snprintf(cmd, PYRIX_BUF, "CZ%dxxx", mode);

    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD <%s>", cmd);

    tcflush(PortFD, TCIOFLUSH);

    if ( (rc = tty_write(PortFD, cmd, PYRIX_CMD, &nbytes_written)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s: %s.", __FUNCTION__, errstr);
        return false;
    }

    return true;
}

bool Pyxis::setRotationRate(uint8_t rate)
{
    char cmd[PYRIX_BUF] = {0};

    int nbytes_written = 0, rc = -1;
    char errstr[MAXRBUF];

    snprintf(cmd, PYRIX_BUF, "CTxx%02d", rate);

    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD <%s>", cmd);

    tcflush(PortFD, TCIOFLUSH);

    if ( (rc = tty_write(PortFD, cmd, PYRIX_CMD, &nbytes_written)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s: %s.", __FUNCTION__, errstr);
        return false;
    }

    tcflush(PortFD, TCIOFLUSH);

    return true;
}

bool Pyxis::sleepController()
{
    const char *cmd = "CSLEEP";

    int nbytes_written = 0, rc = -1;
    char errstr[MAXRBUF];

    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD <%s>", cmd);

    tcflush(PortFD, TCIOFLUSH);

    if ( (rc = tty_write(PortFD, cmd, PYRIX_CMD, &nbytes_written)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s: %s.", __FUNCTION__, errstr);
        return false;
    }

    return true;
}

bool Pyxis::wakeupController()
{
    const char *cmd = "CWAKEUP";
    char res[1] = { 0 };

    int nbytes_written = 0, nbytes_read = 0, rc = -1;
    char errstr[MAXRBUF];

    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD <%s>", cmd);

    tcflush(PortFD, TCIOFLUSH);

    if ( (rc = tty_write(PortFD, cmd, PYRIX_CMD, &nbytes_written)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s: %s.", __FUNCTION__, errstr);
        return false;
    }

    if ( (rc = tty_read(PortFD, res, 1, PYXIS_TIMEOUT, &nbytes_read)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s error: %s.", __FUNCTION__, errstr);
        return false;
    }

    tcflush(PortFD, TCIOFLUSH);

    DEBUGF(INDI::Logger::DBG_DEBUG, "RES <%c>", res[0]);

    return (res[0] == '!');
}

IPState Pyxis::HomeRotator()
{
    const char *cmd = "CHOMES";

    int nbytes_written = 0, rc = -1;
    char errstr[MAXRBUF];

    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD <%s>", cmd);

    tcflush(PortFD, TCIOFLUSH);

    if ( (rc = tty_write(PortFD, cmd, PYRIX_CMD, &nbytes_written)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s: %s.", __FUNCTION__, errstr);
        return IPS_ALERT;
    }

    return IPS_BUSY;
}

IPState Pyxis::MoveRotator(double angle)
{
    char cmd[PYRIX_BUF] = {0};

    int nbytes_written = 0, rc = -1;
    char errstr[MAXRBUF];

    targetPA = static_cast<uint16_t>(round(angle));

    if (targetPA > 359)
        targetPA = 0;

    snprintf(cmd, PYRIX_BUF, "CPA%03d", targetPA);

    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD <%s>", cmd);

    tcflush(PortFD, TCIOFLUSH);

    if ( (rc = tty_write(PortFD, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s: %s.", __FUNCTION__, errstr);
        return IPS_ALERT;
    }

    return IPS_BUSY;
}

bool Pyxis::ReverseRotator(bool enabled)
{
    char cmd[PYRIX_BUF] = {0};

    int nbytes_written = 0, rc = -1;
    char errstr[MAXRBUF];

    snprintf(cmd, PYRIX_BUF, "CD%dxxx", enabled ? 1 : 0);

    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD <%s>", cmd);

    tcflush(PortFD, TCIOFLUSH);

    if ( (rc = tty_write(PortFD, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s: %s.", __FUNCTION__, errstr);
        return false;
    }

    return true;
}

void Pyxis::TimerHit()
{
    if (!isConnected() || PowerS[POWER_SLEEP].s == ISS_ON)
    {
        SetTimer(updatePeriodMS);
        return;
    }    

    if (HomeRotatorSP.s == IPS_BUSY)
    {
        if (isMotionComplete())
        {
            HomeRotatorSP.s = IPS_OK;
            HomeRotatorS[0].s = ISS_OFF;
            IDSetSwitch(&HomeRotatorSP, nullptr);
            DEBUG(INDI::Logger::DBG_SESSION, "Homing is complete.");
        }
        else
        {
            // Fast timer
            SetTimer(100);
            return;
        }
    }
    else if (GotoRotatorNP.s == IPS_BUSY)
    {
        if (isMotionComplete())
        {
            GotoRotatorNP.s = IPS_OK;
        }
        else
        {
            // Fast timer
            SetTimer(100);
            return;
        }
        //if (PA == targetPA)
    }

    uint16_t PA = 0;
    if (getPA(PA) && (PA != static_cast<uint16_t>(GotoRotatorN[0].value)))
    {
        GotoRotatorN[0].value = PA;
        IDSetNumber(&GotoRotatorNP, nullptr);
    }

    SetTimer(updatePeriodMS);
}

bool Pyxis::isMotionComplete()
{
    int nbytes_read = 0, rc = -1;
    char errstr[MAXRBUF];
    char res[1] = { 0 };

    if ( (rc = tty_read(PortFD, res, 1, PYXIS_TIMEOUT, &nbytes_read)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s error: %s.", __FUNCTION__, errstr);
        return false;
    }

    DEBUGF(INDI::Logger::DBG_DEBUG, "RES <%c>", res[0]);

    // Homing still in progress
    if (res[0] == '!')
        return false;
    // Homing is complete
    else if (res[0] == 'F')
        return true;
    // Error
    else if (HomeRotatorSP.s == IPS_BUSY)
    {
        HomeRotatorS[0].s = ISS_OFF;
        HomeRotatorSP.s = IPS_ALERT;
        DEBUG(INDI::Logger::DBG_ERROR, "Homing failed. Check possible jam.");
        tcflush(PortFD, TCIOFLUSH);
    }

    return false;
}

bool Pyxis::getPA(uint16_t &PA)
{
    const char *cmd = "CGETPA";
    char res[4] = {0};

    int nbytes_written = 0, nbytes_read = 0, rc = -1;
    char errstr[MAXRBUF];

    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD <%s>", cmd);

    tcflush(PortFD, TCIOFLUSH);

    if ( (rc = tty_write(PortFD, cmd, PYRIX_CMD, &nbytes_written)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s: %s.", __FUNCTION__, errstr);
        return false;
    }

    if ( (rc = tty_read(PortFD, res, 3, PYXIS_TIMEOUT, &nbytes_read)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s error: %s.", __FUNCTION__, errstr);
        return false;
    }

    tcflush(PortFD, TCIOFLUSH);

    DEBUGF(INDI::Logger::DBG_DEBUG, "RES <%s>", res);

    if (res[0] == '!')
        return false;

    PA = atoi(res);

    return true;
}

int Pyxis::getReverseStatus()
{
    const char *cmd = "CMREAD";
    char res[1] = {0};

    int nbytes_written = 0, nbytes_read = 0, rc = -1;
    char errstr[MAXRBUF];

    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD <%s>", cmd);

    tcflush(PortFD, TCIOFLUSH);

    if ( (rc = tty_write(PortFD, cmd, PYRIX_CMD, &nbytes_written)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s: %s.", __FUNCTION__, errstr);
        return -1;
    }

    if ( (rc = tty_read(PortFD, res, 1, PYXIS_TIMEOUT, &nbytes_read)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s error: %s.", __FUNCTION__, errstr);
        return -1;
    }

    tcflush(PortFD, TCIOFLUSH);

    DEBUGF(INDI::Logger::DBG_DEBUG, "RES <%c>", res[0]);

    // Subtract from '0' to get actual number (0 or 1)
    return (res[0] - 0x30);
}

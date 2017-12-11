/*
  NStep Focuser
  Copyright (c) 2016 Cloudmakers, s. r. o.
  All Rights Reserved.

  Thanks to Gene Nolan and Leon Palmer for their support.

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

#include "nstep.h"

#include "indicom.h"

#include <cstring>
#include <memory>

#define POLLMS 2000

#define currentSpeed            SpeedN[0].value
#define currentPosition         FocusAbsPosN[0].value
#define currentTemperature      TemperatureN[0].value
#define currentRelativeMovement FocusRelPosN[0].value
#define currentAbsoluteMovement FocusAbsPosN[0].value

std::unique_ptr<NSTEP> nstep(new NSTEP());

void ISGetProperties(const char *dev)
{
    nstep->ISGetProperties(dev);
}

void ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    nstep->ISNewSwitch(dev, name, states, names, n);
}

void ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n)
{
    nstep->ISNewText(dev, name, texts, names, n);
}

void ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    nstep->ISNewNumber(dev, name, values, names, n);
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
    nstep->ISSnoopDevice(root);
}

NSTEP::NSTEP()
{    
    setVersion(1, 0);
    SetFocuserCapability(FOCUSER_CAN_ABORT | FOCUSER_CAN_ABS_MOVE | FOCUSER_CAN_REL_MOVE);
}

NSTEP::~NSTEP()
{
    if (isConnected())
        NSTEP::Disconnect();
}

bool NSTEP::initProperties()
{
    INDI::Focuser::initProperties();

    addDebugControl();
    addSimulationControl();

    FocusAbsPosN[0].min  = -999999;
    FocusAbsPosN[0].max  = 999999;
    FocusAbsPosN[0].step = 1;

    FocusRelPosN[0].min  = -999;
    FocusRelPosN[0].max  = 999;
    FocusRelPosN[0].step = 1;

    FocusSpeedN[0].min  = 1;
    FocusSpeedN[0].max  = 254;
    FocusSpeedN[0].step = 1;

    FocusMotionSP.r = ISR_1OFMANY;

    IUFillSwitch(&TempCompS[0], "ENABLED", "Temperature compensation enabled", ISS_OFF);
    IUFillSwitch(&TempCompS[1], "DISABLED", "Temperature compensation disabled", ISS_ON);
    IUFillSwitchVector(&TempCompSP, TempCompS, 2, getDeviceName(), "COMPENSATION_MODE", "Compensation mode",
                       MAIN_CONTROL_TAB, IP_RW, ISR_1OFMANY, 0, IPS_OK);

    IUFillNumber(&TempCompN[0], "TEMP_CHANGE", "Temperature change", "%.1f", -99, 99, 0.1, 0);
    IUFillNumber(&TempCompN[1], "TEMP_MOVE", "Compensation move", "%.0f", 0, 999, 1, 0);
    IUFillNumberVector(&TempCompNP, TempCompN, 2, getDeviceName(), "COMPENSATION_SETTING", "Compensation settings",
                       MAIN_CONTROL_TAB, IP_RW, 0, IPS_OK);

    IUFillNumber(&TempN[0], "TEMPERATURE", "Temperature", "%.1f", 0, 999, 0, 0);
    IUFillNumberVector(&TempNP, TempN, 1, getDeviceName(), "TEMPERATURE", "Temperature", MAIN_CONTROL_TAB, IP_RO, 0,
                       IPS_OK);

    IUFillSwitch(&SteppingPhaseS[0], "0", "0", ISS_ON);
    IUFillSwitch(&SteppingPhaseS[1], "1", "1", ISS_OFF);
    IUFillSwitch(&SteppingPhaseS[2], "2", "2", ISS_OFF);
    IUFillSwitchVector(&SteppingPhaseSP, SteppingPhaseS, 3, getDeviceName(), "PHASE_WIRING", "Phase wiring",
                       MAIN_CONTROL_TAB, IP_RW, ISR_1OFMANY, 0, IPS_OK);

    IUFillSwitch(&SteppingModeS[0], "WAVE", "Wave", ISS_OFF);
    IUFillSwitch(&SteppingModeS[1], "HALF", "Half", ISS_OFF);
    IUFillSwitch(&SteppingModeS[2], "FULL", "Full", ISS_ON);
    IUFillSwitchVector(&SteppingModeSP, SteppingModeS, 3, getDeviceName(), "STEPPING_MODE", "Stepping mode",
                       MAIN_CONTROL_TAB, IP_RW, ISR_1OFMANY, 0, IPS_OK);
    steppingMode = '2';

    return true;
}

bool NSTEP::updateProperties()
{
    if (isConnected())
    {
        command(":CC1", nullptr, 0);
        if (command(":RP", buf, 7))
        {
            sscanf(buf, "%ld", &position);
            FocusAbsPosN[0].value = position;
            defineNumber(&FocusAbsPosNP);
        }
        else
        {
            DEBUG(INDI::Logger::DBG_ERROR, "Failed to read position");
        }
        if (command(":RT", buf, 4))
        {
            sscanf(buf, "%d", &temperature);
            if (temperature != -888)
            {
                TempN[0].value = temperature / 10.0;
                defineNumber(&TempNP);
                if (command(":RA", buf, 4))
                {
                    int value;
                    sscanf(buf, "%d", &value);
                    TempCompN[0].value = value / 10.0;
                    defineNumber(&TempCompNP);
                }
                else
                {
                    DEBUG(INDI::Logger::DBG_ERROR, "Failed to read temperature change for compensation");
                }
                if (command(":RB", buf, 3))
                {
                    int value;
                    sscanf(buf, "%d", &value);
                    TempCompN[1].value = value;
                    defineNumber(&TempCompNP);
                }
                else
                {
                    DEBUG(INDI::Logger::DBG_ERROR, "Failed to read temperature step for compensation");
                }
                if (command(":RG", buf, 1))
                {
                    char value     = *buf;
                    TempCompS[0].s = value == '2' ? ISS_ON : ISS_OFF;
                    TempCompS[1].s = value == '0' ? ISS_ON : ISS_OFF;
                    defineSwitch(&TempCompSP);
                }
                else
                {
                    DEBUG(INDI::Logger::DBG_ERROR, "Failed to read compensation mode");
                }
            }
            else
            {
                DEBUG(INDI::Logger::DBG_ERROR, "Temperature sensor is not connected");
            }
        }
        else
        {
            DEBUG(INDI::Logger::DBG_ERROR, "Failed to read temperature");
        }
        if (command(":RS", buf, 3))
        {
            int value;
            sscanf(buf, "%d", &value);
            FocusSpeedN[0].min = value;
            if (command(":RO", buf, 3))
            {
                int value;
                sscanf(buf, "%d", &value);
                FocusSpeedN[0].value = value;
                defineNumber(&FocusSpeedNP);
            }
            else
            {
                DEBUG(INDI::Logger::DBG_ERROR, "Failed to read step rate");
            }
        }
        else
        {
            DEBUG(INDI::Logger::DBG_ERROR, "Failed to read max step rate");
        }
        if (command(":RW", buf, 1))
        {
            steppingPhase        = *buf;
            SteppingPhaseS[0].s = steppingPhase == '0' ? ISS_ON : ISS_OFF;
            SteppingPhaseS[1].s = steppingPhase == '1' ? ISS_ON : ISS_OFF;
            SteppingPhaseS[2].s = steppingPhase == '2' ? ISS_ON : ISS_OFF;
            defineSwitch(&SteppingPhaseSP);
        }
        else
        {
            DEBUG(INDI::Logger::DBG_ERROR, "Failed to read stepping phase");
        }
    defineSwitch(&SteppingModeSP);
    }
    else
    {
        deleteProperty(FocusAbsPosNP.name);
        deleteProperty(TempNP.name);
        deleteProperty(TempCompNP.name);
        deleteProperty(TempCompSP.name);
        deleteProperty(FocusSpeedNP.name);
        deleteProperty(SteppingModeSP.name);
        deleteProperty(SteppingPhaseSP.name);
    }
    return INDI::Focuser::updateProperties();
}

bool NSTEP::Handshake()
{
    if (isSimulation())
    {
        DEBUG(INDI::Logger::DBG_SESSION, "NStep simulation is connected.");
        return true;
    }

    char b = 0x06;
    int actual = 0;
    int rc = tty_write(PortFD, &b, 1, &actual);

    if (rc == TTY_OK)
    {
        rc = tty_read(PortFD, &b, 1, 5, &actual);
        if (rc == TTY_OK && b == 'S')
            return true;
    }
    return false;
}

const char *NSTEP::getDefaultName()
{
    return "Rigelsys NStep";
}

bool NSTEP::command(const char *request, char *response, int count)
{
    DEBUGF(INDI::Logger::DBG_DEBUG, "Write [%s]", request);
    if (isSimulation())
    {
        if (strcmp(request, ":RT") == 0)
        {
            strncpy(response, "+150", 5);
            DEBUGF(INDI::Logger::DBG_DEBUG, "Read [%s]", response);
            return true;
        }
        if (strcmp(request, ":RP") == 0)
        {
            sprintf(response, "%+07ld", sim_position);
            DEBUGF(INDI::Logger::DBG_DEBUG, "Read [%s]", response);
            return true;
        }
        if (strcmp(request, ":RS") == 0)
        {
            strncpy(response, "100", 4);
            DEBUGF(INDI::Logger::DBG_DEBUG, "Read [%s]", response);
            return true;
        }
        if (strcmp(request, ":RO") == 0)
        {
            strncpy(response, "001", 4);
            DEBUGF(INDI::Logger::DBG_DEBUG, "Read [%s]", response);
            return true;
        }
        if (strcmp(request, ":RA") == 0)
        {
            strncpy(response, "+010", 5);
            DEBUGF(INDI::Logger::DBG_DEBUG, "Read [%s]", response);
            return true;
        }
        if (strcmp(request, ":RB") == 0)
        {
            strncpy(response, "005", 4);
            DEBUGF(INDI::Logger::DBG_DEBUG, "Read [%s]", response);
            return true;
        }
        if (strcmp(request, ":RG") == 0)
        {
            strncpy(response, "2", 2);
            DEBUGF(INDI::Logger::DBG_DEBUG, "Read [%s]", response);
            return true;
        }
        if (strcmp(request, ":RW") == 0)
        {
            strncpy(response, "0", 2);
            DEBUGF(INDI::Logger::DBG_DEBUG, "Read [%s]", response);
            return true;
        }
        if (strcmp(request, "S") == 0)
        {
            strncpy(response, "0", 2);
            DEBUGF(INDI::Logger::DBG_DEBUG, "Read [%s]", response);
            return true;
        }
        return true;
    }
    int actual, total;
    pthread_mutex_lock(&lock);
    int rc = TTY_OK;

    if (request != nullptr)
        rc = tty_write(PortFD, request, strlen(request), &actual);
    if (rc == TTY_OK && response != nullptr)
    {
        total = 0;
        while (rc == TTY_OK && count > 0)
        {
            rc = tty_read(PortFD, response + total, count, 5, &actual);
            total += actual;
            count -= actual;
        }
        response[total] = 0;
    }
    if (rc != TTY_OK)
    {
        char message[MAXRBUF];
        tty_error_msg(rc, message, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s", message);
        pthread_mutex_unlock(&lock);
        return false;
    }
    DEBUGF(INDI::Logger::DBG_DEBUG, "Read [%s]", response);
    pthread_mutex_unlock(&lock);
    return true;
}

bool NSTEP::ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    if (dev != nullptr && strcmp(dev, getDeviceName()) == 0)
    {
        if (strcmp(name, TempCompSP.name) == 0)
        {
            IUUpdateSwitch(&TempCompSP, states, names, n);
            TempCompSP.s = IPS_OK;
            if (TempCompS[0].s == ISS_ON)
            {
                if (!command(":TA2", nullptr, 0))
                {
                    TempCompSP.s = IPS_ALERT;
                }
                if (!command(":TC30#", nullptr, 0))
                {
                    TempCompSP.s = IPS_ALERT;
                }
            }
            else
            {
                if (!command(":TA0", nullptr, 0))
                {
                    TempCompSP.s = IPS_ALERT;
                }
            }
            IDSetSwitch(&TempCompSP, nullptr);
            return true;
        }
        if (strcmp(name, SteppingPhaseSP.name) == 0)
        {
            IUUpdateSwitch(&SteppingPhaseSP, states, names, n);
            SteppingPhaseSP.s = IPS_OK;
            if (SteppingPhaseS[0].s == ISS_ON)
            {
                steppingPhase = '0';
                if (!command(":CW0", nullptr, 0))
                {
                    SteppingPhaseSP.s = IPS_ALERT;
                }
            }
            else if (SteppingPhaseS[1].s == ISS_ON)
            {
                steppingPhase = '1';
                if (!command(":CW1", nullptr, 0))
                {
                    SteppingPhaseSP.s = IPS_ALERT;
                }
            }
            else if (SteppingPhaseS[2].s == ISS_ON)
            {
                steppingPhase = '2';
                if (!command(":CW2", nullptr, 0))
                {
                    SteppingPhaseSP.s = IPS_ALERT;
                }
            }
            IDSetSwitch(&SteppingPhaseSP, nullptr);
            return true;
        }
        if (strcmp(name, SteppingModeSP.name) == 0)
        {
            IUUpdateSwitch(&SteppingModeSP, states, names, n);
            SteppingModeSP.s = IPS_OK;
            if (SteppingModeS[0].s == ISS_ON)
            {
                steppingMode = '0';
            }
            else if (SteppingModeS[1].s == ISS_ON)
            {
                steppingMode = '1';
            }
            else if (SteppingModeS[2].s == ISS_ON)
            {
                steppingMode = '2';
            }
            IDSetSwitch(&SteppingModeSP, nullptr);
            return true;
        }
    }
    return INDI::Focuser::ISNewSwitch(dev, name, states, names, n);
}

bool NSTEP::ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    if (dev != nullptr && strcmp(dev, getDeviceName()) == 0)
    {
        if (strcmp(name, TempCompNP.name) == 0)
        {
            IUUpdateNumber(&TempCompNP, values, names, n);
            PresetNP.s = IPS_OK;
            sprintf(buf, ":TT%+04d#", (int)(TempCompN[0].value * 10));
            if (!command(buf, nullptr, 0))
            {
                PresetNP.s = IPS_ALERT;
            }
            sprintf(buf, ":TS%03d#", (int)(TempCompN[1].value));
            if (!command(buf, nullptr, 0))
            {
                PresetNP.s = IPS_ALERT;
            }
            IDSetNumber(&TempCompNP, nullptr);
            return true;
        }
    }
    return INDI::Focuser::ISNewNumber(dev, name, values, names, n);
}

IPState NSTEP::moveFocuserRelative(FocusDirection dir, unsigned int ticks)
{
    unsigned int originalTicks = ticks;
    unsigned int subTicks;

    if (isSimulation())
    {
        if (dir == FOCUS_INWARD)
            sim_position -= ticks;
        else
            sim_position += ticks;
        return IPS_BUSY;
    }

    do
    {
        if (ticks > 999)
        {
            subTicks = 999;
            ticks -= 999;
        }
        else
        {
            subTicks = ticks;
            ticks = 0;
        }

        sprintf(buf, ":F%c%c%03d#", dir == FOCUS_INWARD ? '0' : '1', steppingMode, subTicks);
        DEBUGF(INDI::Logger::DBG_DEBUG, "About to send command %s", buf);
        if (!command(buf, nullptr, 0))
        {
            FocusAbsPosNP.s = IPS_ALERT;
            IDSetNumber(&FocusAbsPosNP, nullptr);
            return IPS_ALERT;
        }
        do
        {
            if (!command("S", buf, 1))
            {
                FocusAbsPosNP.s = IPS_ALERT;
                IDSetNumber(&FocusAbsPosNP, nullptr);
                return IPS_ALERT;
            }
        } while (*buf != '0');

    } while (ticks > 0);

    FocusAbsPosNP.s = IPS_BUSY;
    if (dir == FOCUS_INWARD)
        currentPosition -= originalTicks;
    else
        currentPosition += originalTicks;

    IDSetNumber(&FocusAbsPosNP, nullptr);
    return IPS_BUSY;
}

IPState NSTEP::MoveAbsFocuser(uint32_t targetTicks)
{
    DEBUGF(INDI::Logger::DBG_SESSION, "Focuser is moving to requested position %d", targetTicks);

    unsigned int newAbsPos = 0;
    IPState retCode        = IPS_ALERT;

    if ((targetTicks - currentPosition) >= 0)
    {
        newAbsPos = targetTicks - currentPosition;
        retCode   = moveFocuserRelative(FOCUS_OUTWARD, newAbsPos);
    }
    else if ((targetTicks - currentPosition) < 0)
    {
        newAbsPos = currentPosition - targetTicks;
        retCode   = moveFocuserRelative(FOCUS_INWARD, newAbsPos);
    }

    return retCode;
}

IPState NSTEP::MoveRelFocuser(FocusDirection dir, unsigned int ticks)
{
    return moveFocuserRelative(dir, ticks);
}

bool NSTEP::AbortFocuser()
{
    sprintf(buf, ":F1%c000#", steppingMode);
    return command(buf, nullptr, 0);
}

bool NSTEP::SetFocuserSpeed(int speed)
{
    sprintf(buf, ":CS%03d#", speed);
    return command(buf, buf, 2);
}

void NSTEP::TimerHit()
{
    if (isConnected())
    {
        if (command(":RT", buf, 4))
        {
            int tmp;
            sscanf(buf, "%d", &tmp);
            if (tmp != temperature)
            {
                temperature    = tmp;
                TempN[0].value = temperature / 10.0;
                IDSetNumber(&TempNP, nullptr);
            }
        }
        else
        {
            DEBUG(INDI::Logger::DBG_ERROR, "Failed to read temperature");
        }

        if (command("S", buf, 1))
        {
            if (*buf == '0' && FocusAbsPosNP.s == IPS_BUSY)
            {
                FocusAbsPosNP.s = IPS_OK;
                IDSetNumber(&FocusAbsPosNP, nullptr);
                FocusRelPosNP.s = IPS_OK;
                IDSetNumber(&FocusRelPosNP, nullptr);
            }
        }
        SetTimer(POLLMS);
    }
}

bool NSTEP::saveConfigItems(FILE *fp)
{
    IUSaveConfigSwitch(fp, &TempCompSP);
    IUSaveConfigSwitch(fp, &SteppingModeSP);
    IUSaveConfigNumber(fp, &TempCompNP);
    return INDI::Focuser::saveConfigItems(fp);
}

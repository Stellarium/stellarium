/*******************************************************************************
  Copyright(c) 2015 Jasem Mutlaq. All rights reserved.

 PerfectStar Focuser

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

#include "perfectstar.h"

#include <cmath>
#include <cstring>
#include <memory>

#define POLLMS              1000 /* 1000 ms */
#define PERFECTSTAR_TIMEOUT 1000 /* 1000 ms */

#define FOCUS_SETTINGS_TAB "Settings"

// We declare an auto pointer to PerfectStar.
std::unique_ptr<PerfectStar> perfectStar(new PerfectStar());

void ISPoll(void *p);

void ISGetProperties(const char *dev)
{
    perfectStar->ISGetProperties(dev);
}

void ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    perfectStar->ISNewSwitch(dev, name, states, names, n);
}

void ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n)
{
    perfectStar->ISNewText(dev, name, texts, names, n);
}

void ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    perfectStar->ISNewNumber(dev, name, values, names, n);
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
    perfectStar->ISSnoopDevice(root);
}

PerfectStar::PerfectStar()
{
    SetFocuserCapability(FOCUSER_CAN_ABS_MOVE | FOCUSER_CAN_REL_MOVE | FOCUSER_CAN_ABORT);
    setFocuserConnection(CONNECTION_NONE);
}

bool PerfectStar::Connect()
{
    sim = isSimulation();

    if (sim)
    {
        SetTimer(POLLMS);
        return true;
    }

    handle = hid_open(0x04D8, 0xF812, nullptr);

    if (handle == nullptr)
    {
        DEBUG(INDI::Logger::DBG_ERROR, "No PerfectStar focuser found.");
        return false;
    }
    else
        SetTimer(POLLMS);

    return (handle != nullptr);
}

bool PerfectStar::Disconnect()
{
    if (!sim)
    {
        hid_close(handle);
        hid_exit();
    }

    return true;
}

const char *PerfectStar::getDefaultName()
{
    return (const char *)"PerfectStar";
}

bool PerfectStar::initProperties()
{
    INDI::Focuser::initProperties();

    // Max Position
    IUFillNumber(&MaxPositionN[0], "Steps", "", "%.f", 0, 500000, 0., 10000);
    IUFillNumberVector(&MaxPositionNP, MaxPositionN, 1, getDeviceName(), "Max Position", "", FOCUS_SETTINGS_TAB, IP_RW,
                       0, IPS_IDLE);

    // Sync to a particular position
    IUFillNumber(&SyncN[0], "Ticks", "", "%.f", 0, 100000, 100., 0.);
    IUFillNumberVector(&SyncNP, SyncN, 1, getDeviceName(), "Sync", "", MAIN_CONTROL_TAB, IP_RW, 0, IPS_IDLE);

    FocusAbsPosN[0].min = SyncN[0].min = 0;
    FocusAbsPosN[0].max = SyncN[0].max = MaxPositionN[0].value;
    FocusAbsPosN[0].step = SyncN[0].step = MaxPositionN[0].value / 50.0;
    FocusAbsPosN[0].value                = 0;

    FocusRelPosN[0].max   = (FocusAbsPosN[0].max - FocusAbsPosN[0].min) / 2;
    FocusRelPosN[0].step  = FocusRelPosN[0].max / 100.0;
    FocusRelPosN[0].value = 100;

    addSimulationControl();

    return true;
}

bool PerfectStar::updateProperties()
{
    INDI::Focuser::updateProperties();

    if (isConnected())
    {
        defineNumber(&SyncNP);
        defineNumber(&MaxPositionNP);
    }
    else
    {
        deleteProperty(SyncNP.name);
        deleteProperty(MaxPositionNP.name);
    }

    return true;
}

void PerfectStar::TimerHit()
{
    if (!isConnected())
        return;

    uint32_t currentTicks = 0;

    bool rc = getPosition(&currentTicks);

    if (rc)
        FocusAbsPosN[0].value = currentTicks;

    getStatus(&status);

    if (FocusAbsPosNP.s == IPS_BUSY || FocusRelPosNP.s == IPS_BUSY)
    {
        if (sim)
        {
            if (FocusAbsPosN[0].value < targetPosition)
                simPosition += 500;
            else
                simPosition -= 500;

            if (std::abs((int64_t)simPosition - (int64_t)targetPosition) < 500)
            {
                FocusAbsPosN[0].value = targetPosition;
                simPosition           = FocusAbsPosN[0].value;
                status                = PS_NOOP;
            }

            FocusAbsPosN[0].value = simPosition;
        }

        if (status == PS_HALT && targetPosition == FocusAbsPosN[0].value)
        {
            if (FocusRelPosNP.s == IPS_BUSY)
            {
                FocusRelPosNP.s = IPS_OK;
                IDSetNumber(&FocusRelPosNP, nullptr);
            }

            FocusAbsPosNP.s = IPS_OK;
            DEBUG(INDI::Logger::DBG_DEBUG, "Focuser reached target position.");
        }
        else if (status == PS_NOOP)
        {
            if (FocusRelPosNP.s == IPS_BUSY)
            {
                FocusRelPosNP.s = IPS_OK;
                IDSetNumber(&FocusRelPosNP, nullptr);
            }

            FocusAbsPosNP.s = IPS_OK;
            DEBUG(INDI::Logger::DBG_SESSION, "Focuser reached home position.");
        }
    }

    IDSetNumber(&FocusAbsPosNP, nullptr);

    SetTimer(POLLMS);
}

bool PerfectStar::ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    if (dev != nullptr && strcmp(dev, getDeviceName()) == 0)
    {
        // Max Travel
        if (strcmp(MaxPositionNP.name, name) == 0)
        {
            IUUpdateNumber(&MaxPositionNP, values, names, n);

            if (MaxPositionN[0].value > 0)
            {
                FocusAbsPosN[0].min = SyncN[0].min = 0;
                FocusAbsPosN[0].max = SyncN[0].max = MaxPositionN[0].value;
                FocusAbsPosN[0].step = SyncN[0].step = MaxPositionN[0].value / 50.0;

                FocusRelPosN[0].max  = (FocusAbsPosN[0].max - FocusAbsPosN[0].min) / 2;
                FocusRelPosN[0].step = FocusRelPosN[0].max / 100.0;
                FocusRelPosN[0].min  = 0;

                IUUpdateMinMax(&FocusAbsPosNP);
                IUUpdateMinMax(&FocusRelPosNP);
                IUUpdateMinMax(&SyncNP);

                DEBUGF(INDI::Logger::DBG_SESSION, "Focuser absolute limits: min (%g) max (%g)", FocusAbsPosN[0].min,
                       FocusAbsPosN[0].max);
            }

            MaxPositionNP.s = IPS_OK;
            IDSetNumber(&MaxPositionNP, nullptr);
            return true;
        }

        // Sync
        if (strcmp(SyncNP.name, name) == 0)
        {
            IUUpdateNumber(&SyncNP, values, names, n);
            if (!sync(SyncN[0].value))
                SyncNP.s = IPS_ALERT;
            else
                SyncNP.s = IPS_OK;

            IDSetNumber(&SyncNP, nullptr);
            return true;
        }
    }

    return INDI::Focuser::ISNewNumber(dev, name, values, names, n);
}

IPState PerfectStar::MoveAbsFocuser(uint32_t targetTicks)
{
    bool rc = setPosition(targetTicks);

    if (!rc)
        return IPS_ALERT;

    targetPosition = targetTicks;

    rc = setStatus(PS_GOTO);

    if (!rc)
        return IPS_ALERT;

    FocusAbsPosNP.s = IPS_BUSY;

    return IPS_BUSY;
}

IPState PerfectStar::MoveRelFocuser(FocusDirection dir, uint32_t ticks)
{
    uint32_t finalTicks = FocusAbsPosN[0].value + (ticks * (dir == FOCUS_INWARD ? -1 : 1));

    return MoveAbsFocuser(finalTicks);
}

bool PerfectStar::setPosition(uint32_t ticks)
{
    int rc = 0;
    unsigned char command[3];
    unsigned char response[3];

    // 20 bit resolution position. 4 high bits + 16 lower bits

    // Send 4 high bits first
    command[0] = 0x28;
    command[1] = (ticks & 0x40000) >> 16;

    DEBUGF(INDI::Logger::DBG_DEBUG, "Set Position (%ld)", ticks);
    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD (%02X %02X)", command[0], command[1]);

    if (sim)
        rc = 2;
    else
        rc = hid_write(handle, command, 2);

    if (rc < 0)
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "setPosition: Error writing to device (%s)", hid_error(handle));
        return false;
    }

    if (sim)
    {
        rc          = 2;
        response[0] = 0x28;
        response[1] = command[1];
    }
    else
        rc = hid_read_timeout(handle, response, 2, PERFECTSTAR_TIMEOUT);

    if (rc < 0)
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "setPosition: Error reading from device (%s)", hid_error(handle));
        return false;
    }

    DEBUGF(INDI::Logger::DBG_DEBUG, "RES (%02X %02X)", response[0], response[1]);

    // Send lower 16 bit
    command[0] = 0x20;
    // Low Byte
    command[1] = ticks & 0xFF;
    // High Byte
    command[2] = (ticks & 0xFF00) >> 8;

    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD (%02X %02X %02X)", command[0], command[1], command[2]);

    if (sim)
        rc = 3;
    else
        rc = hid_write(handle, command, 3);

    if (rc < 0)
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "setPosition: Error writing to device (%s)", hid_error(handle));
        return false;
    }

    if (sim)
    {
        rc          = 3;
        response[0] = command[0];
        response[1] = command[1];
        response[2] = command[2];
    }
    else
        rc = hid_read_timeout(handle, response, 3, PERFECTSTAR_TIMEOUT);

    if (rc < 0)
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "setPosition: Error reading from device (%s)", hid_error(handle));
        return false;
    }

    DEBUGF(INDI::Logger::DBG_DEBUG, "RES (%02X %02X %02X)", response[0], response[1], response[2]);

    targetPosition = ticks;

    // TODO add checking later
    return true;
}

bool PerfectStar::getPosition(uint32_t *ticks)
{
    int rc       = 0;
    uint32_t pos = 0;
    unsigned char command[1];
    unsigned char response[3];

    // 20 bit resolution position. 4 high bits + 16 lower bits

    // Get 4 high bits first
    command[0] = 0x29;

    DEBUG(INDI::Logger::DBG_DEBUG, "Get Position (High 4 bits)");
    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD (%02X)", command[0]);

    if (sim)
        rc = 2;
    else
        rc = hid_write(handle, command, 1);

    if (rc < 0)
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "getPosition: Error writing to device (%s)", hid_error(handle));
        return false;
    }

    if (sim)
    {
        rc          = 2;
        response[0] = command[0];
        response[1] = simPosition >> 16;
    }
    else
        rc = hid_read_timeout(handle, response, 2, PERFECTSTAR_TIMEOUT);

    if (rc < 0)
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "getPosition: Error reading from device (%s)", hid_error(handle));
        return false;
    }

    DEBUGF(INDI::Logger::DBG_DEBUG, "RES (%02X %02X)", response[0], response[1]);

    // Store 4 high bits part of a 20 bit number
    pos = response[1] << 16;

    // Get 16 lower bits
    command[0] = 0x21;

    DEBUG(INDI::Logger::DBG_DEBUG, "Get Position (Lower 16 bits)");
    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD (%02X)", command[0]);

    if (sim)
        rc = 1;
    else
        rc = hid_write(handle, command, 1);

    if (rc < 0)
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "getPosition: Error writing to device (%s)", hid_error(handle));
        return false;
    }

    if (sim)
    {
        rc          = 3;
        response[0] = command[0];
        response[1] = simPosition & 0xFF;
        response[2] = (simPosition & 0xFF00) >> 8;
    }
    else
        rc = hid_read_timeout(handle, response, 3, PERFECTSTAR_TIMEOUT);

    if (rc < 0)
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "getPosition: Error reading from device (%s)", hid_error(handle));
        return false;
    }

    DEBUGF(INDI::Logger::DBG_DEBUG, "RES (%02X %02X %02X)", response[0], response[1], response[2]);

    // Res[1] is lower byte and Res[2] is high byte. Combine them and add them to ticks.
    pos |= response[1] | response[2] << 8;

    *ticks = pos;

    DEBUGF(INDI::Logger::DBG_DEBUG, "Position: %ld", pos);

    return true;
}

bool PerfectStar::setStatus(PS_STATUS targetStatus)
{
    int rc = 0;
    unsigned char command[2];
    unsigned char response[3];

    command[0] = 0x10;
    command[1] = (targetStatus == PS_HALT) ? 0xFF : targetStatus;

    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD (%02X %02X)", command[0], command[1]);

    if (sim)
        rc = 2;
    else
        rc = hid_write(handle, command, 2);

    if (rc < 0)
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "setStatus: Error writing to device (%s)", hid_error(handle));
        return false;
    }

    if (sim)
    {
        rc          = 3;
        response[0] = command[0];
        response[1] = 0;
        response[2] = command[1];
        status      = targetStatus;
        // Convert Goto to either "moving in" or "moving out" status
        if (status == PS_GOTO)
        {
            // Moving in state
            if (targetPosition < FocusAbsPosN[0].value)
                status = PS_IN;
            else
                // Moving out state
                status = PS_OUT;
        }
    }
    else
        rc = hid_read_timeout(handle, response, 3, PERFECTSTAR_TIMEOUT);

    if (rc < 0)
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "setStatus: Error reading from device (%s)", hid_error(handle));
        return false;
    }

    DEBUGF(INDI::Logger::DBG_DEBUG, "RES (%02X %02X %02X)", response[0], response[1], response[2]);

    if (response[1] == 0xFF)
    {
        DEBUG(INDI::Logger::DBG_ERROR, "setStatus: Invalid state change.");
        return false;
    }

    return true;
}

bool PerfectStar::getStatus(PS_STATUS *currentStatus)
{
    int rc = 0;
    unsigned char command[1];
    unsigned char response[2];

    command[0] = 0x11;

    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD (%02X)", command[0]);

    if (sim)
        rc = 1;
    else
        rc = hid_write(handle, command, 1);

    if (rc < 0)
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "getStatus: Error writing to device (%s)", hid_error(handle));
        return false;
    }

    if (sim)
    {
        rc          = 2;
        response[0] = command[0];
        response[1] = status;
        // Halt/SetPos is state = 0 "not moving".
        if (response[1] == PS_HALT || response[1] == PS_SETPOS)
            response[1] = 0;
    }
    else
        rc = hid_read_timeout(handle, response, 2, PERFECTSTAR_TIMEOUT);

    if (rc < 0)
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "getStatus: Error reading from device (%s)", hid_error(handle));
        return false;
    }

    DEBUGF(INDI::Logger::DBG_DEBUG, "RES (%02X %02X)", response[0], response[1]);

    switch (response[1])
    {
        case 0:
            *currentStatus = PS_HALT;
            DEBUG(INDI::Logger::DBG_DEBUG, "State: Not moving.");
            break;

        case 1:
            *currentStatus = PS_IN;
            DEBUG(INDI::Logger::DBG_DEBUG, "State: Moving in.");
            break;

        case 3:
            *currentStatus = PS_GOTO;
            DEBUG(INDI::Logger::DBG_DEBUG, "State: Goto.");
            break;

        case 2:
            *currentStatus = PS_OUT;
            DEBUG(INDI::Logger::DBG_DEBUG, "State: Moving out.");
            break;

        case 5:
            *currentStatus = PS_LOCKED;
            DEBUG(INDI::Logger::DBG_DEBUG, "State: Locked.");
            break;

        default:
            DEBUGF(INDI::Logger::DBG_WARNING, "Warning: Unknown status (%d)", response[1]);
            return false;
            break;
    }

    return true;
}

bool PerfectStar::AbortFocuser()
{
    return setStatus(PS_HALT);
}

bool PerfectStar::sync(uint32_t ticks)
{
    bool rc = setPosition(ticks);

    if (!rc)
        return false;

    simPosition = ticks;

    rc = setStatus(PS_SETPOS);

    return rc;
}

bool PerfectStar::saveConfigItems(FILE *fp)
{
    INDI::Focuser::saveConfigItems(fp);

    IUSaveConfigNumber(fp, &MaxPositionNP);

    return true;
}

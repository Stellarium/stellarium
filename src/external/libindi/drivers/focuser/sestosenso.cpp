/*
    SestoSenso Focuser
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

#include "sestosenso.h"

#include "indicom.h"

#include <cmath>
#include <cstring>
#include <memory>

#include <termios.h>
#include <unistd.h>

#define SESTOSENSO_TIMEOUT 3

#define POLLMS 500

std::unique_ptr<SestoSenso> sesto(new SestoSenso());

void ISGetProperties(const char *dev)
{
    sesto->ISGetProperties(dev);
}

void ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    sesto->ISNewSwitch(dev, name, states, names, n);
}

void ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n)
{
    sesto->ISNewText(dev, name, texts, names, n);
}

void ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    sesto->ISNewNumber(dev, name, values, names, n);
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
    sesto->ISSnoopDevice(root);
}

SestoSenso::SestoSenso()
{
    // Can move in Absolute & Relative motions, can AbortFocuser motion.
    SetFocuserCapability(FOCUSER_CAN_ABS_MOVE | FOCUSER_CAN_REL_MOVE | FOCUSER_CAN_ABORT);
}

bool SestoSenso::initProperties()
{
    INDI::Focuser::initProperties();

    // Firmware Information
    IUFillText(&FirmwareT[0], "VERSION", "Version", "");
    IUFillTextVector(&FirmwareTP, FirmwareT, 1, getDeviceName(), "FOCUS_FIRMWARE", "Firmware", MAIN_CONTROL_TAB, IP_RO, 0, IPS_IDLE);

    // Focuser temperature
    IUFillNumber(&TemperatureN[0], "TEMPERATURE", "Celsius", "%6.2f", -50, 70., 0., 0.);
    IUFillNumberVector(&TemperatureNP, TemperatureN, 1, getDeviceName(), "FOCUS_TEMPERATURE", "Temperature", MAIN_CONTROL_TAB, IP_RO, 0, IPS_IDLE);

    // Sync
    IUFillNumber(&SyncN[0], "FOCUS_SYNC_OFFSET", "Offset", "%6.0f", 0, 60000., 0., 0.);
    IUFillNumberVector(&SyncNP, SyncN, 1, getDeviceName(), "FOCUS_SYNC", "Sync", MAIN_CONTROL_TAB, IP_RW, 0, IPS_IDLE);

    // Relative and absolute movement
    FocusRelPosN[0].min   = 0.;
    FocusRelPosN[0].max   = 50000.;
    FocusRelPosN[0].value = 0;
    FocusRelPosN[0].step  = 1000;

    FocusAbsPosN[0].min   = 0.;
    FocusAbsPosN[0].max   = 100000.;
    FocusAbsPosN[0].value = 0;
    FocusAbsPosN[0].step  = 1000;

    addAuxControls();

    updatePeriodMS = POLLMS;

    return true;
}

bool SestoSenso::updateProperties()
{
    INDI::Focuser::updateProperties();

    if (isConnected())
    {
        defineNumber(&SyncNP);
        defineNumber(&TemperatureNP);
        defineText(&FirmwareTP);

        DEBUG(INDI::Logger::DBG_SESSION, "SestoSenso paramaters updated, focuser ready for use.");

        GetFocusParams();
    }
    else
    {
        deleteProperty(SyncNP.name);
        deleteProperty(TemperatureNP.name);
        deleteProperty(FirmwareTP.name);
    }

    return true;
}

bool SestoSenso::Handshake()
{
    if (Ack())
    {
        DEBUG(INDI::Logger::DBG_SESSION, "SestoSenso is online. Getting focus parameters...");
        return true;
    }

    DEBUG(INDI::Logger::DBG_SESSION,
          "Error retreiving data from SestoSenso, please ensure SestoSenso controller is powered and the port is correct.");
    return false;
}

const char *SestoSenso::getDefaultName()
{
    return "Sesto Senso";
}

bool SestoSenso::Ack()
{
    int nbytes_written = 0, nbytes_read = 0, rc = -1;
    char errstr[MAXRBUF];
    char resp[16]={0};

    DEBUG(INDI::Logger::DBG_DEBUG, "CMD <#QF!>");

    if (isSimulation())
    {
        strncpy(resp, "1.0 Simulation", 16);
        nbytes_read = strlen(resp) + 1;
    }
    else
    {
        tcflush(PortFD, TCIOFLUSH);

        if ((rc = tty_write(PortFD, "#QF!", 4, &nbytes_written)) != TTY_OK)
        {
            tty_error_msg(rc, errstr, MAXRBUF);
            DEBUGF(INDI::Logger::DBG_ERROR, "%s error: %s.", __FUNCTION__, errstr);
            return false;
        }

        if ((rc = tty_read_section(PortFD, resp, 0xD, SESTOSENSO_TIMEOUT, &nbytes_read)) != TTY_OK)
        {
            tty_error_msg(rc, errstr, MAXRBUF);
            DEBUGF(INDI::Logger::DBG_ERROR, "%s error: %s.", __FUNCTION__, errstr);
            return false;
        }

        tcflush(PortFD, TCIOFLUSH);
    }

    resp[nbytes_read-1] = '\0';

    DEBUGF(INDI::Logger::DBG_DEBUG, "RES <%s>", resp);

    IUSaveText(&FirmwareT[0], resp);

    return true;
}

bool SestoSenso::updateTemperature()
{
    int nbytes_written = 0, nbytes_read = 0, rc = -1;
    char errstr[MAXRBUF];
    char resp[16]={0};

    DEBUG(INDI::Logger::DBG_DEBUG, "CMD <#QT!>");

    if (isSimulation())
    {
        strncpy(resp, "23.45", 16);
        nbytes_read = strlen(resp) + 1;
    }
    else
    {
        tcflush(PortFD, TCIOFLUSH);

        if ((rc = tty_write(PortFD, "#QT!", 4, &nbytes_written)) != TTY_OK)
        {
            tty_error_msg(rc, errstr, MAXRBUF);
            DEBUGF(INDI::Logger::DBG_ERROR, "%s error: %s.", __FUNCTION__, errstr);
            TemperatureNP.s = IPS_ALERT;
            return false;
        }

        if ((rc = tty_read_section(PortFD, resp, 0xD, SESTOSENSO_TIMEOUT, &nbytes_read)) != TTY_OK)
        {
            tty_error_msg(rc, errstr, MAXRBUF);
            DEBUGF(INDI::Logger::DBG_ERROR, "%s error: %s.", __FUNCTION__, errstr);
            TemperatureNP.s = IPS_ALERT;
            return false;
        }

        tcflush(PortFD, TCIOFLUSH);
    }

    resp[nbytes_read-1] = '\0';

    DEBUGF(INDI::Logger::DBG_DEBUG, "RES <%s>", resp);

    TemperatureN[0].value = atof(resp);
    TemperatureNP.s = (TemperatureN[0].value == 99.00) ? IPS_IDLE : IPS_OK;

    return true;
}

bool SestoSenso::updatePosition()
{
    int nbytes_written = 0, nbytes_read = 0, rc = -1;
    char errstr[MAXRBUF];
    char resp[16]={0};

    DEBUG(INDI::Logger::DBG_DEBUG, "CMD <#QP!>");

    if (isSimulation())
    {
        snprintf(resp, 16, "%d", static_cast<uint32_t>(FocusAbsPosN[0].value));
        nbytes_read = strlen(resp)+1;
    }
    else
    {
        tcflush(PortFD, TCIOFLUSH);

        if ((rc = tty_write(PortFD, "#QP!", 4, &nbytes_written)) != TTY_OK)
        {
            tty_error_msg(rc, errstr, MAXRBUF);
            DEBUGF(INDI::Logger::DBG_ERROR, "%s error: %s.", __FUNCTION__, errstr);
            FocusAbsPosNP.s = IPS_ALERT;
            return false;
        }

        if ((rc = tty_read_section(PortFD, resp, 0xD, SESTOSENSO_TIMEOUT, &nbytes_read)) != TTY_OK)
        {
            tty_error_msg(rc, errstr, MAXRBUF);
            DEBUGF(INDI::Logger::DBG_ERROR, "%s error: %s.", __FUNCTION__, errstr);
            FocusAbsPosNP.s = IPS_ALERT;
            return false;
        }

        tcflush(PortFD, TCIOFLUSH);
    }

    resp[nbytes_read-1] = '\0';

    DEBUGF(INDI::Logger::DBG_DEBUG, "RES <%s>", resp);

    FocusAbsPosN[0].value = atoi(resp);
    FocusAbsPosNP.s = IPS_OK;

    return true;
}

bool SestoSenso::isMotionComplete()
{
    int nbytes_read = 0, rc = -1;
    char errstr[MAXRBUF];
    char resp[16]={0};

    if (isSimulation())
    {
        int32_t nextPos = FocusAbsPosN[0].value;
        int32_t targPos = static_cast<int32_t>(targetPos);

        if (targPos > nextPos)
            nextPos += 250;
        else if (targPos < nextPos)
            nextPos -= 250;

        if (abs(nextPos-targPos) < 250)
            nextPos = targetPos;
        else if (nextPos < 0)
            nextPos = 0;
        else if (nextPos > FocusAbsPosN[0].max)
            nextPos = FocusAbsPosN[0].max;

        snprintf(resp, 16, "%d", nextPos);
        nbytes_read = strlen(resp)+1;
    }
    else
    {
        if ((rc = tty_read_section(PortFD, resp, 0xD, SESTOSENSO_TIMEOUT, &nbytes_read)) != TTY_OK)
        {
            tty_error_msg(rc, errstr, MAXRBUF);
            DEBUGF(INDI::Logger::DBG_ERROR, "%s error: %s.", __FUNCTION__, errstr);
            return false;
        }
    }

    resp[nbytes_read-1] = '\0';

    if (!strcmp(resp, "GTok!"))
        return true;

    uint32_t newPos = atoi(resp);
    FocusAbsPosN[0].value = newPos;

    if (newPos == targetPos)
        return true;

    return false;
}

bool SestoSenso::sync(uint32_t newPosition)
{
    int nbytes_written = 0, rc = -1;
    char errstr[MAXRBUF];
    char cmd[16]={0};

    snprintf(cmd, 16, "#SP%d!", newPosition);

    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD <%s>", cmd);

    if (isSimulation())
    {
        FocusAbsPosN[0].value = newPosition;
    }
    else
    {
        tcflush(PortFD, TCIOFLUSH);

        // Sync
        if ((rc = tty_write(PortFD, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
        {
            tty_error_msg(rc, errstr, MAXRBUF);
            DEBUGF(INDI::Logger::DBG_ERROR, "%s error: %s.", __FUNCTION__, errstr);
            return false;
        }
    }

    return isCommandOK("SP");
}

bool SestoSenso::ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
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
    }

    return INDI::Focuser::ISNewNumber(dev, name, values, names, n);
}

IPState SestoSenso::MoveAbsFocuser(uint32_t targetTicks)
{
    targetPos = targetTicks;

    int nbytes_written = 0, rc = -1;
    char errstr[MAXRBUF];
    char cmd[16]={0};

    snprintf(cmd, 16, "#GT%d!", targetTicks);
    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD <%s>", cmd);

    if (isSimulation() == false)
    {
        if ((rc = tty_write(PortFD, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
        {
            tty_error_msg(rc, errstr, MAXRBUF);
            DEBUGF(INDI::Logger::DBG_ERROR, "%s error: %s.", __FUNCTION__, errstr);
            return IPS_ALERT;
        }
    }

    return IPS_BUSY;

    /*if (isCommandOK("GT"))
    {
        FocusAbsPosNP.s = IPS_BUSY;
        return IPS_BUSY;
    }

    return IPS_ALERT;
    */
}

IPState SestoSenso::MoveRelFocuser(FocusDirection dir, uint32_t ticks)
{
    double newPosition = 0;
    bool rc            = false;

    if (dir == FOCUS_INWARD)
        newPosition = FocusAbsPosN[0].value - ticks;
    else
        newPosition = FocusAbsPosN[0].value + ticks;

    rc = MoveAbsFocuser(newPosition);

    if (!rc)
        return IPS_ALERT;

    FocusRelPosN[0].value = ticks;
    FocusRelPosNP.s       = IPS_BUSY;

    return IPS_BUSY;
}

bool SestoSenso::AbortFocuser()
{
    if (isSimulation())
        return true;

    int nbytes_written;
    if (tty_write(PortFD, "#MA!", 4, &nbytes_written) == TTY_OK)
    {
        FocusAbsPosNP.s = IPS_IDLE;
        FocusRelPosNP.s = IPS_IDLE;
        IDSetNumber(&FocusAbsPosNP, nullptr);
        IDSetNumber(&FocusRelPosNP, nullptr);
    }

    return isCommandOK("MA");
}

bool SestoSenso::isCommandOK(const char *cmd)
{
    int nbytes_read = 0, rc = -1;
    char errstr[MAXRBUF], resp[16];

    char expectedResp[16];
    snprintf(expectedResp, 16, "%sok!", cmd);

    if (isSimulation())
    {
        strncpy(resp, expectedResp, 16);
        nbytes_read = strlen(resp)+1;
    }
    else
    {
        if ((rc = tty_read_section(PortFD, resp, 0xD, SESTOSENSO_TIMEOUT, &nbytes_read)) != TTY_OK)
        {
            tty_error_msg(rc, errstr, MAXRBUF);
            DEBUGF(INDI::Logger::DBG_ERROR, "%s error for command %s: %s.", __FUNCTION__, cmd, errstr);
            return false;
        }
    }

    resp[nbytes_read-1] = '\0';

    DEBUGF(INDI::Logger::DBG_DEBUG, "RES <%s>", resp);

    return (!strcmp(resp, expectedResp));
}

void SestoSenso::TimerHit()
{
    if (!isConnected())
    {
        SetTimer(POLLMS);
        return;
    }

    if (FocusAbsPosNP.s == IPS_BUSY || FocusRelPosNP.s == IPS_BUSY)
    {
        if (isMotionComplete())
        {
            FocusAbsPosNP.s = IPS_OK;
            FocusRelPosNP.s = IPS_OK;
            IDSetNumber(&FocusRelPosNP, nullptr);
            IDSetNumber(&FocusAbsPosNP, nullptr);
            DEBUG(INDI::Logger::DBG_SESSION, "Focuser reached requested position.");
        }
        else
            IDSetNumber(&FocusAbsPosNP, nullptr);

        lastPos = FocusAbsPosN[0].value;
        SetTimer(POLLMS);
        return;
    }

    bool rc = updatePosition();
    if (rc)
    {
        if (lastPos != FocusAbsPosN[0].value)
        {
            IDSetNumber(&FocusAbsPosNP, nullptr);
            lastPos = FocusAbsPosN[0].value;
        }
    }

    rc = updateTemperature();
    if (rc)
    {
        if (fabs(lastTemperature - TemperatureN[0].value) >= 0.1)
        {
            IDSetNumber(&TemperatureNP, nullptr);
            lastTemperature = TemperatureN[0].value;
        }
    }

    SetTimer(POLLMS);
}

void SestoSenso::GetFocusParams()
{
    if (updatePosition())
        IDSetNumber(&FocusAbsPosNP, nullptr);

    if (updateTemperature())
        IDSetNumber(&TemperatureNP, nullptr);
}

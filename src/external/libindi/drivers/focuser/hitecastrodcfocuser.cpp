/*******************************************************************************
  Copyright(c) 2016 Andy Kirkham. All rights reserved.

 HitecAstroDCFocuser Focuser

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

#include "hitecastrodcfocuser.h"

#include <cstring>
#include <memory>

#define POLLMS         500   /* 0.5s */
#define HID_TIMEOUT    10000 /* 10s */
#define FUDGE_FACTOR_H 1000
#define FUDGE_FACTOR_L 885

#define FOCUS_SETTINGS_TAB "Settings"

std::unique_ptr<HitecAstroDCFocuser> hitecastroDcFocuser(new HitecAstroDCFocuser());

void ISPoll(void *p);

void ISGetProperties(const char *dev)
{
    hitecastroDcFocuser->ISGetProperties(dev);
}

void ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    hitecastroDcFocuser->ISNewSwitch(dev, name, states, names, n);
}

void ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n)
{
    hitecastroDcFocuser->ISNewText(dev, name, texts, names, n);
}

void ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    hitecastroDcFocuser->ISNewNumber(dev, name, values, names, n);
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
    hitecastroDcFocuser->ISSnoopDevice(root);
}

HitecAstroDCFocuser::HitecAstroDCFocuser() : _handle(nullptr)
{
    SetFocuserCapability(FOCUSER_CAN_REL_MOVE); // | FOCUSER_HAS_VARIABLE_SPEED);
    setFocuserConnection(CONNECTION_NONE);
}

HitecAstroDCFocuser::~HitecAstroDCFocuser()
{
    if (_handle != nullptr)
    {
        hid_close(_handle);
        _handle = nullptr;
    }
}

bool HitecAstroDCFocuser::Connect()
{
    sim = isSimulation();

    if (sim)
    {
        SetTimer(POLLMS);
        return true;
    }

    if (hid_init() != 0)
    {
        DEBUG(INDI::Logger::DBG_ERROR, "hid_init() failed.");
    }

    _handle = hid_open(0x04D8, 0xFAC2, nullptr);

    DEBUG(INDI::Logger::DBG_DEBUG, _handle ? "HitecAstroDCFocuser opened." : "HitecAstroDCFocuser failed.");

    if (_handle != nullptr)
    {
        DEBUG(INDI::Logger::DBG_SESSION,
              "Experimental driver. Report issues to https://github.com/A-j-K/hitecastrodcfocuser/issues");
        SetTimer(POLLMS);
        return true;
    }

    DEBUGF(INDI::Logger::DBG_ERROR, "Failed to connect to focuser: %s", hid_error(_handle));
    return false;
}

bool HitecAstroDCFocuser::Disconnect()
{
    if (!sim && _handle != nullptr)
    {
        hid_close(_handle);
        _handle = nullptr;
    }

    DEBUG(INDI::Logger::DBG_DEBUG, "HitecAstroDCFocuser closed.");
    return true;
}

const char *HitecAstroDCFocuser::getDefaultName()
{
    return (const char *)"HitecAstro DC";
}

bool HitecAstroDCFocuser::initProperties()
{
    INDI::Focuser::initProperties();

    //IDMessage(getDeviceName(), "HitecAstroDCFocuser::initProperties()");

    addDebugControl();
    addSimulationControl();

    IUFillNumber(&MaxPositionN[0], "Steps", "", "%.f", 0, 500000, 0., 10000);
    IUFillNumberVector(&MaxPositionNP, MaxPositionN, 1, getDeviceName(), "MAX_POSITION", "Max position",
                       FOCUS_SETTINGS_TAB, IP_RW, 0, IPS_IDLE);

    IUFillNumber(&SlewSpeedN[0], "Steps/sec", "", "%.f", 1, 100, 0., 50);
    IUFillNumberVector(&SlewSpeedNP, SlewSpeedN, 1, getDeviceName(), "SLEW_SPEED", "Slew speed", MAIN_CONTROL_TAB,
                       IP_RW, 0, IPS_IDLE);

    IUFillSwitch(&ReverseDirectionS[0], "ENABLED", "Reverse direction", ISS_OFF);
    IUFillSwitchVector(&ReverseDirectionSP, ReverseDirectionS, 1, getDeviceName(), "REVERSE_DIRECTION",
                       "Reverse direction", OPTIONS_TAB, IP_RW, ISR_NOFMANY, 0, IPS_IDLE);

    FocusSpeedN[0].value = 100.;
    FocusSpeedN[0].min   = 1.;
    FocusSpeedN[0].max   = 100.;
    FocusSpeedN[0].value = 100.;

    FocusAbsPosN[0].min   = 0;
    FocusAbsPosN[0].max   = MaxPositionN[0].value;
    FocusAbsPosN[0].step  = MaxPositionN[0].value / 50.0;
    FocusAbsPosN[0].value = 0;

    FocusRelPosN[0].min   = 1;
    FocusRelPosN[0].max   = (FocusAbsPosN[0].max - FocusAbsPosN[0].min) / 2;
    FocusRelPosN[0].step  = FocusRelPosN[0].max / 100.0;
    FocusRelPosN[0].value = 100;

    return true;
}

bool HitecAstroDCFocuser::updateProperties()
{
    bool f = INDI::Focuser::updateProperties();

    if (isConnected())
    {
        //defineNumber(&MaxPositionNP);
        defineNumber(&SlewSpeedNP);
        defineSwitch(&ReverseDirectionSP);
    }
    else
    {
        //deleteProperty(MaxPositionNP.name);
        deleteProperty(SlewSpeedNP.name);
        deleteProperty(ReverseDirectionSP.name);
    }

    return f;
}

void HitecAstroDCFocuser::TimerHit()
{
    if (_state == SLEWING && _duration > 0)
    {
        --_duration;
        if (_duration == 0)
        {
            int rc;
            unsigned char command[8]={0};
            _state = IDLE;
            memset(command, 0, 8);
            command[0] = _stop;
            rc         = hid_write(_handle, command, 8);
            if (rc < 0)
            {
                DEBUGF(INDI::Logger::DBG_DEBUG, "::MoveFocuser() fail (%s)", hid_error(_handle));
            }
            hid_read_timeout(_handle, command, 8, 1000);
        }
    }
    SetTimer(1);
}

bool HitecAstroDCFocuser::ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    if (dev != nullptr && strcmp(dev, getDeviceName()) == 0)
    {
        if (strcmp(name, ReverseDirectionSP.name) == 0)
        {
            IUUpdateSwitch(&ReverseDirectionSP, states, names, n);
            ReverseDirectionSP.s = IPS_OK;
            IDSetSwitch(&ReverseDirectionSP, nullptr);
            return true;
        }
    }
    return INDI::Focuser::ISNewSwitch(dev, name, states, names, n);
}

bool HitecAstroDCFocuser::ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    if (dev != nullptr && strcmp(dev, getDeviceName()) == 0)
    {
        if (strcmp(name, MaxPositionNP.name) == 0)
        {
            IUUpdateNumber(&MaxPositionNP, values, names, n);
            MaxPositionNP.s = IPS_OK;
            IDSetNumber(&MaxPositionNP, nullptr);
            return true;
        }
        if (strcmp(name, SlewSpeedNP.name) == 0)
        {
            if (values[0] > 100)
            {
                SlewSpeedNP.s = IPS_ALERT;
                return false;
            }
            IUUpdateNumber(&SlewSpeedNP, values, names, n);
            SlewSpeedNP.s = IPS_OK;
            IDSetNumber(&SlewSpeedNP, nullptr);
            return true;
        }
    }
    return INDI::Focuser::ISNewNumber(dev, name, values, names, n);
}

IPState HitecAstroDCFocuser::MoveRelFocuser(FocusDirection dir, uint32_t ticks)
{
    int rc, speed = (int)SlewSpeedN[0].value; //_slew_speed;
//    int32_t iticks = ticks;
    unsigned char command[8]={0};
    IPState rval;

    DEBUGF(INDI::Logger::DBG_DEBUG, "::move() begin %d ticks at speed %d", ticks, speed);

    if (_handle == nullptr)
    {
        DEBUG(INDI::Logger::DBG_DEBUG, "::move() bad handle");
        return IPS_ALERT;
    }

    FocusRelPosNP.s = IPS_BUSY;
    IDSetNumber(&FocusRelPosNP, nullptr);

    // JM 2017-03-16: iticks is not used, FIXME.
//    if (dir == FOCUS_INWARD)
//    {
//        iticks = ticks * -1;
//    }

    if (ReverseDirectionS[0].s == ISS_ON)
    {
        dir = dir == FOCUS_INWARD ? FOCUS_OUTWARD : FOCUS_INWARD;
    }

    if (speed > 100)
    {
        DEBUGF(INDI::Logger::DBG_DEBUG, "::move() over speed %d, limiting to 100", ticks, speed);
        speed = 100;
    }

    ticks *= FUDGE_FACTOR_H;
    ticks /= FUDGE_FACTOR_L;

    memset(command, 0, 8);
    command[0] = dir == FOCUS_INWARD ? 0x50 : 0x52;
    command[1] = (unsigned char)((ticks >> 8) & 0xFF);
    command[2] = (unsigned char)(ticks & 0xFF);
    command[3] = 0x05;
    command[4] = (unsigned char)(speed & 0xFF);
    command[5] = 0;
    command[6] = 0;
    command[7] = 0;

    DEBUGF(INDI::Logger::DBG_DEBUG, "==> TX %2.2x %2.2x%2.2x %2.2x %2.2x %2.2x%2.2x%2.2x", command[0], command[1],
           command[2], command[3], command[4], command[5], command[6], command[7]);

    rc = hid_write(_handle, command, 8);
    if (rc < 0)
    {
        DEBUGF(INDI::Logger::DBG_DEBUG, "::MoveRelFocuser() fail (%s)", hid_error(_handle));
        return IPS_ALERT;
    }

    FocusRelPosNP.s = IPS_BUSY;

    memset(command, 0, 8);
    hid_read_timeout(_handle, command, 8, HID_TIMEOUT);
    DEBUGF(INDI::Logger::DBG_DEBUG, "==> RX %2.2x %2.2x%2.2x %2.2x %2.2x %2.2x%2.2x%2.2x", command[0], command[1],
           command[2], command[3], command[4], command[5], command[6], command[7]);

    rval = command[1] == 0x21 ? IPS_OK : IPS_ALERT;

    FocusRelPosNP.s = rval;
    IDSetNumber(&FocusRelPosNP, nullptr);

    return rval;
}

IPState HitecAstroDCFocuser::MoveFocuser(FocusDirection dir, int speed, uint16_t duration)
{
    int rc;
    unsigned char command[8]={0};
    IPState rval;

    DEBUGF(INDI::Logger::DBG_DEBUG, "::MoveFocuser(%d %d %d)", dir, speed, duration);

    if (_handle == nullptr)
    {
        return IPS_ALERT;
    }

    FocusSpeedNP.s = IPS_BUSY;
    IDSetNumber(&FocusSpeedNP, nullptr);

    if (ReverseDirectionS[0].s == ISS_ON)
    {
        dir = dir == FOCUS_INWARD ? FOCUS_OUTWARD : FOCUS_INWARD;
    }

    if (speed > 100)
    {
        DEBUGF(INDI::Logger::DBG_DEBUG, "::MoveFocuser() over speed %d, limiting to 100", speed);
        speed = 100;
    }

    _stop = dir == FOCUS_INWARD ? 0xB0 : 0xBA;

    memset(command, 0, 8);
    command[0] = dir == FOCUS_INWARD ? 0x54 : 0x56;
    command[1] = (unsigned char)((speed >> 8) & 0xFF);
    command[2] = (unsigned char)(speed & 0xFF);
    command[3] = 0x05;
    command[4] = 0;
    command[5] = 0;
    command[6] = 0;
    command[7] = 0;

    DEBUGF(INDI::Logger::DBG_DEBUG, "==> TX %2.2x %2.2x%2.2x %2.2x %2.2x %2.2x%2.2x%2.2x", command[0], command[1],
           command[2], command[3], command[4], command[5], command[6], command[7]);

    rc = hid_write(_handle, command, 8);
    if (rc < 0)
    {
        DEBUGF(INDI::Logger::DBG_DEBUG, "::MoveFocuser() fail (%s)", hid_error(_handle));
        return IPS_ALERT;
    }

    memset(command, 0, 8);
    hid_read_timeout(_handle, command, 8, HID_TIMEOUT);
    DEBUGF(INDI::Logger::DBG_DEBUG, "==> RX %2.2x %2.2x%2.2x %2.2x %2.2x %2.2x%2.2x%2.2x", command[0], command[1],
           command[2], command[3], command[4], command[5], command[6], command[7]);

    rval = command[1] == 0x24 ? IPS_OK : IPS_ALERT;

    FocusSpeedNP.s = rval;
    IDSetNumber(&FocusSpeedNP, nullptr);

    _duration = duration;
    _state    = SLEWING;

    return IPS_BUSY;
}

bool HitecAstroDCFocuser::saveConfigItems(FILE *fp)
{
    INDI::Focuser::saveConfigItems(fp);

    IUSaveConfigNumber(fp, &MaxPositionNP);
    IUSaveConfigNumber(fp, &SlewSpeedNP);
    IUSaveConfigSwitch(fp, &ReverseDirectionSP);

    return true;
}

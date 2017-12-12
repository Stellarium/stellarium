/*******************************************************************************
  Copyright(c) 2012 Jasem Mutlaq. All rights reserved.

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

#include "focus_simulator.h"

#include <cmath>
#include <memory>
#include <cstring>
#include <unistd.h>

// We declare an auto pointer to focusSim.
std::unique_ptr<FocusSim> focusSim(new FocusSim());

// Focuser takes 100 microsecond to move for each step, completing 100,000 steps in 10 seconds
#define FOCUS_MOTION_DELAY 100

void ISPoll(void *p);

void ISGetProperties(const char *dev)
{
    focusSim->ISGetProperties(dev);
}

void ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    focusSim->ISNewSwitch(dev, name, states, names, n);
}

void ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n)
{
    focusSim->ISNewText(dev, name, texts, names, n);
}

void ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    focusSim->ISNewNumber(dev, name, values, names, n);
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
    focusSim->ISSnoopDevice(root);
}

/************************************************************************************
 *
************************************************************************************/
FocusSim::FocusSim()
{
    SetFocuserCapability(FOCUSER_CAN_ABS_MOVE | FOCUSER_CAN_REL_MOVE | FOCUSER_HAS_VARIABLE_SPEED);
}

/************************************************************************************
 *
************************************************************************************/
bool FocusSim::Connect()
{
    SetTimer(1000);
    return true;
}

/************************************************************************************
 *
************************************************************************************/
bool FocusSim::Disconnect()
{
    return true;
}

/************************************************************************************
 *
************************************************************************************/
const char *FocusSim::getDefaultName()
{
    return (const char *)"Focuser Simulator";
}

/************************************************************************************
 *
************************************************************************************/
void FocusSim::ISGetProperties(const char *dev)
{
    if (dev != nullptr && strcmp(dev, getDeviceName()) != 0)
        return;

    INDI::Focuser::ISGetProperties(dev);

    defineSwitch(&ModeSP);
    loadConfig(true, "Mode");
}

/************************************************************************************
 *
************************************************************************************/
bool FocusSim::initProperties()
{
    INDI::Focuser::initProperties();

    IUFillNumber(&SeeingN[0], "SIM_SEEING", "arcseconds", "%4.2f", 0, 60, 0, 3.5);
    IUFillNumberVector(&SeeingNP, SeeingN, 1, getDeviceName(), "SEEING_SETTINGS", "Seeing", MAIN_CONTROL_TAB, IP_RW, 60,
                       IPS_IDLE);

    IUFillNumber(&FWHMN[0], "SIM_FWHM", "arcseconds", "%4.2f", 0, 60, 0, 7.5);
    IUFillNumberVector(&FWHMNP, FWHMN, 1, getDeviceName(), "FWHM", "FWHM", MAIN_CONTROL_TAB, IP_RO, 60, IPS_IDLE);

    IUFillSwitch(&ModeS[MODE_ALL], "All", "All", ISS_ON);
    IUFillSwitch(&ModeS[MODE_ABSOLUTE], "Absolute", "Absolute", ISS_OFF);
    IUFillSwitch(&ModeS[MODE_RELATIVE], "Relative", "Relative", ISS_OFF);
    IUFillSwitch(&ModeS[MODE_TIMER], "Timer", "Timer", ISS_OFF);
    IUFillSwitchVector(&ModeSP, ModeS, MODE_COUNT, getDeviceName(), "Mode", "Mode", MAIN_CONTROL_TAB, IP_RW,
                       ISR_1OFMANY, 60, IPS_IDLE);

    initTicks = sqrt(FWHMN[0].value - SeeingN[0].value) / 0.75;

    FocusSpeedN[0].min   = 1;
    FocusSpeedN[0].max   = 5;
    FocusSpeedN[0].step  = 1;
    FocusSpeedN[0].value = 1;

    FocusAbsPosN[0].value = FocusAbsPosN[0].max / 2;

    internalTicks = FocusAbsPosN[0].value;

    return true;
}

/************************************************************************************
 *
************************************************************************************/
bool FocusSim::updateProperties()
{
    INDI::Focuser::updateProperties();

    if (isConnected())
    {
        defineNumber(&SeeingNP);
        defineNumber(&FWHMNP);
    }
    else
    {
        deleteProperty(SeeingNP.name);
        deleteProperty(FWHMNP.name);
    }

    return true;
}

/************************************************************************************
 *
************************************************************************************/
bool FocusSim::ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    if (dev != nullptr && strcmp(dev, getDeviceName()) == 0)
    {
        // Modes
        if (strcmp(ModeSP.name, name) == 0)
        {
            IUUpdateSwitch(&ModeSP, states, names, n);
            uint32_t cap = 0;
            int index    = IUFindOnSwitchIndex(&ModeSP);

            switch (index)
            {
                case MODE_ALL:
                    cap = FOCUSER_CAN_ABS_MOVE | FOCUSER_CAN_REL_MOVE | FOCUSER_HAS_VARIABLE_SPEED;
                    break;

                case MODE_ABSOLUTE:
                    cap = FOCUSER_CAN_ABS_MOVE;
                    break;

                case MODE_RELATIVE:
                    cap = FOCUSER_CAN_REL_MOVE;
                    break;

                case MODE_TIMER:
                    cap = FOCUSER_HAS_VARIABLE_SPEED;
                    break;

                default:
                    ModeSP.s = IPS_ALERT;
                    IDSetSwitch(&ModeSP, "Unknown mode index %d", index);
                    return true;
            }

            SetFocuserCapability(cap);
            ModeSP.s = IPS_OK;
            IDSetSwitch(&ModeSP, nullptr);
            return true;
        }
    }

    return INDI::Focuser::ISNewSwitch(dev, name, states, names, n);
}

/************************************************************************************
 *
************************************************************************************/
bool FocusSim::ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    if (dev != nullptr && strcmp(dev, getDeviceName()) == 0)
    {
        if (strcmp(name, "SEEING_SETTINGS") == 0)
        {
            SeeingNP.s = IPS_OK;
            IUUpdateNumber(&SeeingNP, values, names, n);

            IDSetNumber(&SeeingNP, nullptr);
            return true;
        }
    }

    // Let INDI::Focuser handle any other number properties
    return INDI::Focuser::ISNewNumber(dev, name, values, names, n);
}

/************************************************************************************
 *
************************************************************************************/
IPState FocusSim::MoveFocuser(FocusDirection dir, int speed, uint16_t duration)
{
    double mid         = (FocusAbsPosN[0].max - FocusAbsPosN[0].min) / 2;
    int mode           = IUFindOnSwitchIndex(&ModeSP);
    double targetTicks = ((dir == FOCUS_INWARD) ? -1 : 1) * (speed * duration);

    internalTicks += targetTicks;

    if (mode == MODE_ALL)
    {
        if (internalTicks < FocusAbsPosN[0].min || internalTicks > FocusAbsPosN[0].max)
        {
            internalTicks -= targetTicks;
            DEBUG(INDI::Logger::DBG_ERROR, "Cannot move focuser in this direction any further.");
            return IPS_ALERT;
        }
    }

    // simulate delay in motion as the focuser moves to the new position
    usleep(duration * 1000);

    double ticks = initTicks + (internalTicks - mid) / 5000.0;

    FWHMN[0].value = 0.5625 * ticks * ticks + SeeingN[0].value;

    DEBUGF(INDI::Logger::DBG_DEBUG, "TIMER Current internal ticks: %g FWHM ticks: %g FWHM: %g", internalTicks, ticks,
           FWHMN[0].value);

    if (mode == MODE_ALL)
    {
        FocusAbsPosN[0].value = internalTicks;
        IDSetNumber(&FocusAbsPosNP, nullptr);
    }

    if (FWHMN[0].value < SeeingN[0].value)
        FWHMN[0].value = SeeingN[0].value;

    IDSetNumber(&FWHMNP, nullptr);

    return IPS_OK;
}

/************************************************************************************
 *
************************************************************************************/
IPState FocusSim::MoveAbsFocuser(uint32_t targetTicks)
{
    double mid = (FocusAbsPosN[0].max - FocusAbsPosN[0].min) / 2;

    internalTicks = targetTicks;

    // Limit to +/- 10 from initTicks
    double ticks = initTicks + (targetTicks - mid) / 5000.0;

    // simulate delay in motion as the focuser moves to the new position
    usleep(std::abs((int)(targetTicks - FocusAbsPosN[0].value) * FOCUS_MOTION_DELAY));

    FocusAbsPosN[0].value = targetTicks;

    FWHMN[0].value = 0.5625 * ticks * ticks + SeeingN[0].value;

    DEBUGF(INDI::Logger::DBG_DEBUG, "ABS Current internal ticks: %g FWHM ticks: %g FWHM: %g", internalTicks, ticks,
           FWHMN[0].value);

    if (FWHMN[0].value < SeeingN[0].value)
        FWHMN[0].value = SeeingN[0].value;

    IDSetNumber(&FWHMNP, nullptr);

    return IPS_OK;
}

/************************************************************************************
 *
************************************************************************************/
IPState FocusSim::MoveRelFocuser(FocusDirection dir, uint32_t ticks)
{
    double mid = (FocusAbsPosN[0].max - FocusAbsPosN[0].min) / 2;
    int mode   = IUFindOnSwitchIndex(&ModeSP);

    if (mode == MODE_ALL || mode == MODE_ABSOLUTE)
    {
        uint32_t targetTicks = FocusAbsPosN[0].value + (ticks * (dir == FOCUS_INWARD ? -1 : 1));

        FocusAbsPosNP.s = IPS_BUSY;
        IDSetNumber(&FocusAbsPosNP, nullptr);

        return MoveAbsFocuser(targetTicks);
    }

    internalTicks += (dir == FOCUS_INWARD ? -1 : 1) * static_cast<int32_t>(ticks);

    ticks = initTicks + (internalTicks - mid) / 5000.0;

    DEBUGF(INDI::Logger::DBG_DEBUG, "REL Current internal ticks: %g FWHM ticks: %g FWHM: %g", internalTicks, ticks,
           FWHMN[0].value);

    FWHMN[0].value = 0.5625 * ticks * ticks + SeeingN[0].value;

    if (FWHMN[0].value < SeeingN[0].value)
        FWHMN[0].value = SeeingN[0].value;

    IDSetNumber(&FWHMNP, nullptr);

    return IPS_OK;
}

/************************************************************************************
 *
************************************************************************************/
bool FocusSim::SetFocuserSpeed(int speed)
{
    INDI_UNUSED(speed);
    return true;
}

/*
    Focuser Interface
    Copyright (C) 2011 Jasem Mutlaq (mutlaqja@ikarustech.com)

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

#include "indifocuserinterface.h"

#include "indilogger.h"

#include <cstring>

namespace INDI
{

void FocuserInterface::initFocuserProperties(const char *deviceName, const char *groupName)
{
    strncpy(focuserName, deviceName, MAXINDIDEVICE);

    IUFillNumber(&FocusSpeedN[0], "FOCUS_SPEED_VALUE", "Focus Speed", "%3.0f", 0.0, 255.0, 1.0, 255.0);
    IUFillNumberVector(&FocusSpeedNP, FocusSpeedN, 1, deviceName, "FOCUS_SPEED", "Speed", groupName, IP_RW, 60, IPS_OK);

    IUFillNumber(&FocusTimerN[0], "FOCUS_TIMER_VALUE", "Focus Timer (ms)", "%4.0f", 0.0, 5000.0, 50.0, 1000.0);
    IUFillNumberVector(&FocusTimerNP, FocusTimerN, 1, deviceName, "FOCUS_TIMER", "Timer", groupName, IP_RW, 60, IPS_OK);
    lastTimerValue = 1000.0;

    IUFillSwitch(&FocusMotionS[0], "FOCUS_INWARD", "Focus In", ISS_ON);
    IUFillSwitch(&FocusMotionS[1], "FOCUS_OUTWARD", "Focus Out", ISS_OFF);
    IUFillSwitchVector(&FocusMotionSP, FocusMotionS, 2, deviceName, "FOCUS_MOTION", "Direction", groupName, IP_RW,
                       ISR_1OFMANY, 60, IPS_OK);

    // Driver can define those to clients if there is support
    IUFillNumber(&FocusAbsPosN[0], "FOCUS_ABSOLUTE_POSITION", "Ticks", "%4.0f", 0.0, 100000.0, 1000.0, 0);
    IUFillNumberVector(&FocusAbsPosNP, FocusAbsPosN, 1, deviceName, "ABS_FOCUS_POSITION", "Absolute Position",
                       groupName, IP_RW, 60, IPS_OK);

    IUFillNumber(&FocusRelPosN[0], "FOCUS_RELATIVE_POSITION", "Ticks", "%4.0f", 0.0, 100000.0, 1000.0, 0);
    IUFillNumberVector(&FocusRelPosNP, FocusRelPosN, 1, deviceName, "REL_FOCUS_POSITION", "Relative Position",
                       groupName, IP_RW, 60, IPS_OK);

    IUFillSwitch(&AbortS[0], "ABORT", "Abort", ISS_OFF);
    IUFillSwitchVector(&AbortSP, AbortS, 1, deviceName, "FOCUS_ABORT_MOTION", "Abort Motion", groupName, IP_RW,
                       ISR_ATMOST1, 60, IPS_IDLE);
}

bool FocuserInterface::processFocuserNumber(const char *dev, const char *name, double values[], char *names[],
                                                  int n)
{
    //  This is for our device
    //  Now lets see if it's something we process here
    if (strcmp(name, "FOCUS_TIMER") == 0)
    {
        FocusDirection dir;
        int speed;
        int t;

        //  first we get all the numbers just sent to us
        IUUpdateNumber(&FocusTimerNP, values, names, n);

        //  Now lets find what we need for this move
        speed = FocusSpeedN[0].value;

        if (FocusMotionS[0].s == ISS_ON)
            dir = FOCUS_INWARD;
        else
            dir = FOCUS_OUTWARD;

        t              = FocusTimerN[0].value;
        lastTimerValue = t;

        FocusTimerNP.s = MoveFocuser(dir, speed, t);
        IDSetNumber(&FocusTimerNP, nullptr);
        return true;
    }

    if (strcmp(name, "FOCUS_SPEED") == 0)
    {
        FocusSpeedNP.s    = IPS_OK;
        int current_speed = FocusSpeedN[0].value;
        IUUpdateNumber(&FocusSpeedNP, values, names, n);

        if (SetFocuserSpeed(FocusSpeedN[0].value) == false)
        {
            FocusSpeedN[0].value = current_speed;
            FocusSpeedNP.s       = IPS_ALERT;
        }

        //  Update client display
        IDSetNumber(&FocusSpeedNP, nullptr);
        return true;
    }

    if (strcmp(name, "ABS_FOCUS_POSITION") == 0)
    {
        int newPos = (int)values[0];

        if (newPos < FocusAbsPosN[0].min)
        {
            FocusAbsPosNP.s = IPS_ALERT;
            IDSetNumber(&FocusAbsPosNP, nullptr);
            DEBUGFDEVICE(dev, Logger::DBG_ERROR, "Requested position out of bound. Focus minimum position is %g",
                         FocusAbsPosN[0].min);
            return false;
        }
        else if (newPos > FocusAbsPosN[0].max)
        {
            FocusAbsPosNP.s = IPS_ALERT;
            IDSetNumber(&FocusAbsPosNP, nullptr);
            DEBUGFDEVICE(dev, Logger::DBG_ERROR, "Requested position out of bound. Focus maximum position is %g",
                         FocusAbsPosN[0].max);
            return false;
        }

        IPState ret;

        if ((ret = MoveAbsFocuser(newPos)) == IPS_OK)
        {
            FocusAbsPosNP.s = IPS_OK;
            IUUpdateNumber(&FocusAbsPosNP, values, names, n);
            DEBUGFDEVICE(dev, Logger::DBG_SESSION, "Focuser moved to position %d", newPos);
            IDSetNumber(&FocusAbsPosNP, nullptr);
            return true;
        }
        else if (ret == IPS_BUSY)
        {
            FocusAbsPosNP.s = IPS_BUSY;
            DEBUGFDEVICE(dev, Logger::DBG_SESSION, "Focuser is moving to position %d", newPos);
            IDSetNumber(&FocusAbsPosNP, nullptr);
            return true;
        }

        FocusAbsPosNP.s = IPS_ALERT;
        DEBUGDEVICE(dev, Logger::DBG_ERROR, "Focuser failed to move to new requested position.");
        IDSetNumber(&FocusAbsPosNP, nullptr);
        return false;
    }

    if (strcmp(name, "REL_FOCUS_POSITION") == 0)
    {
        int newPos = (int)values[0];

        if (newPos <= 0)
        {
            DEBUGDEVICE(dev, Logger::DBG_ERROR, "Relative ticks value must be greater than zero.");
            FocusRelPosNP.s = IPS_ALERT;
            IDSetNumber(&FocusRelPosNP, nullptr);
            return false;
        }

        IPState ret;

        if (CanAbsMove())
        {
            if (FocusMotionS[0].s == ISS_ON)
            {
                if (FocusAbsPosN[0].value - newPos < FocusAbsPosN[0].min)
                {
                    FocusRelPosNP.s = IPS_ALERT;
                    IDSetNumber(&FocusRelPosNP, nullptr);
                    DEBUGFDEVICE(dev, Logger::DBG_ERROR,
                                 "Requested position out of bound. Focus minimum position is %g", FocusAbsPosN[0].min);
                    return false;
                }
            }
            else
            {
                if (FocusAbsPosN[0].value + newPos > FocusAbsPosN[0].max)
                {
                    FocusRelPosNP.s = IPS_ALERT;
                    IDSetNumber(&FocusRelPosNP, nullptr);
                    DEBUGFDEVICE(dev, Logger::DBG_ERROR,
                                 "Requested position out of bound. Focus maximum position is %g", FocusAbsPosN[0].max);
                    return false;
                }
            }
        }

        if ((ret = MoveRelFocuser((FocusMotionS[0].s == ISS_ON ? FOCUS_INWARD : FOCUS_OUTWARD), newPos)) == IPS_OK)
        {
            FocusRelPosNP.s = FocusAbsPosNP.s = IPS_OK;
            IUUpdateNumber(&FocusRelPosNP, values, names, n);
            IDSetNumber(&FocusRelPosNP, "Focuser moved %d steps %s", newPos,
                        FocusMotionS[0].s == ISS_ON ? "inward" : "outward");
            IDSetNumber(&FocusAbsPosNP, nullptr);
            return true;
        }
        else if (ret == IPS_BUSY)
        {
            IUUpdateNumber(&FocusRelPosNP, values, names, n);
            FocusRelPosNP.s = FocusAbsPosNP.s = IPS_BUSY;
            IDSetNumber(&FocusAbsPosNP, "Focuser is moving %d steps %s...", newPos,
                        FocusMotionS[0].s == ISS_ON ? "inward" : "outward");
            IDSetNumber(&FocusAbsPosNP, nullptr);
            return true;
        }

        FocusRelPosNP.s = IPS_ALERT;
        DEBUGDEVICE(dev, Logger::DBG_ERROR, "Focuser failed to move to new requested position.");
        IDSetNumber(&FocusRelPosNP, nullptr);
        return false;
    }

    return false;
}

bool FocuserInterface::processFocuserSwitch(const char *dev, const char *name, ISState *states, char *names[],
                                                  int n)
{
    INDI_UNUSED(dev);
    //  This one is for us
    if (strcmp(name, "FOCUS_MOTION") == 0)
    {
        //  client is telling us what to do with focus direction
        FocusMotionSP.s = IPS_OK;
        IUUpdateSwitch(&FocusMotionSP, states, names, n);
        //  Update client display
        IDSetSwitch(&FocusMotionSP, nullptr);

        return true;
    }

    if (strcmp(name, "FOCUS_ABORT_MOTION") == 0)
    {
        IUResetSwitch(&AbortSP);

        if (AbortFocuser())
        {
            AbortSP.s = IPS_OK;
            if (CanAbsMove() && FocusAbsPosNP.s != IPS_IDLE)
            {
                FocusAbsPosNP.s = IPS_IDLE;
                IDSetNumber(&FocusAbsPosNP, nullptr);
            }
            if (CanRelMove() && FocusRelPosNP.s != IPS_IDLE)
            {
                FocusRelPosNP.s = IPS_IDLE;
                IDSetNumber(&FocusRelPosNP, nullptr);
            }
        }
        else
            AbortSP.s = IPS_ALERT;

        IDSetSwitch(&AbortSP, nullptr);
        return true;
    }

    return false;
}

IPState FocuserInterface::MoveFocuser(FocusDirection dir, int speed, uint16_t duration)
{
    INDI_UNUSED(dir);
    INDI_UNUSED(speed);
    INDI_UNUSED(duration);
    // Must be implemented by child class
    return IPS_ALERT;
}

IPState FocuserInterface::MoveRelFocuser(FocusDirection dir, uint32_t ticks)
{
    INDI_UNUSED(dir);
    INDI_UNUSED(ticks);
    // Must be implemented by child class
    return IPS_ALERT;
}

IPState FocuserInterface::MoveAbsFocuser(uint32_t ticks)
{
    INDI_UNUSED(ticks);
    // Must be implemented by child class
    return IPS_ALERT;
}

bool FocuserInterface::AbortFocuser()
{
    //  This should be a virtual function, because the low level hardware class
    //  must override this
    DEBUGDEVICE(focuserName, Logger::DBG_ERROR, "Focuser does not support abort motion.");
    return false;
}

bool FocuserInterface::SetFocuserSpeed(int speed)
{
    INDI_UNUSED(speed);

    //  This should be a virtual function, because the low level hardware class
    //  must override this
    DEBUGDEVICE(focuserName, Logger::DBG_ERROR, "Focuser does not support variable speed.");
    return false;
}

void FocuserInterface::SetFocuserCapability(uint32_t cap)
{
    capability = cap;
}
}

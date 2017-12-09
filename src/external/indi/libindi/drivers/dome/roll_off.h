/*******************************************************************************
 Copyright(c) 2014 Jasem Mutlaq. All rights reserved.

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

#pragma once

#include "indidome.h"

class RollOff : public INDI::Dome
{
  public:
    RollOff();
    virtual ~RollOff() = default;

    virtual bool initProperties();
    const char *getDefaultName();
    bool updateProperties();
    virtual bool ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n);
    virtual bool saveConfigItems(FILE *fp);
    virtual bool ISSnoopDevice(XMLEle *root);

  protected:
    bool Connect();
    bool Disconnect();

    void TimerHit();

    virtual IPState Move(DomeDirection dir, DomeMotionCommand operation);
    virtual IPState Park();
    virtual IPState UnPark();
    virtual bool Abort();

    virtual bool getFullOpenedLimitSwitch();
    virtual bool getFullClosedLimitSwitch();

  private:
    bool SetupParms();
    float CalcTimeLeft(timeval);

    ISState fullOpenLimitSwitch { ISS_ON };
    ISState fullClosedLimitSwitch { ISS_OFF };
    double MotionRequest { 0 };
    struct timeval MotionStart { 0, 0 };
};

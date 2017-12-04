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

#pragma once

#include "indirotator.h"

class Pyxis : public INDI::Rotator
{
  public:

    Pyxis();
    virtual ~Pyxis() = default;

    virtual bool Handshake();
    const char * getDefaultName();
    virtual bool initProperties();
    virtual bool updateProperties();

    virtual bool ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n);
    virtual bool ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n);

  protected:
    // Rotator Overrides
    virtual IPState HomeRotator();
    virtual IPState MoveRotator(double angle);
    virtual bool ReverseRotator(bool enabled);

    // Misc.
    virtual void TimerHit();

  private:    
    // Check if connection is OK
    bool Ack();
    bool isMotionComplete();
    bool getPA(uint16_t & PA);
    int getReverseStatus();
    bool setSteppingMode(uint8_t mode);
    bool setRotationRate(uint8_t rate);
    bool sleepController();
    bool wakeupController();

    void queryParams();

    // Rotation Rate
    INumber RotationRateN[1];
    INumberVectorProperty RotationRateNP;

    // Stepping
    ISwitch SteppingS[2];
    ISwitchVectorProperty SteppingSP;
    enum { FULL_STEP, HALF_STEP};

    // Power
    ISwitch PowerS[2];
    ISwitchVectorProperty PowerSP;
    enum { POWER_SLEEP, POWER_WAKEUP};

    uint16_t targetPA = {0};
};

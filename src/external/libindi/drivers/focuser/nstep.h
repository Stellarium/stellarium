/*
  NStep Focuser
  Copyright (c) 2016 Cloudmakers, s. r. o.
  All Rights Reserved.

  Thanks to Rigel Systems, especially Gene Nolan and Leon Palmer,
  for their support in writing this driver.

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

#include "indifocuser.h"

class NSTEP : public INDI::Focuser
{
  public:
    NSTEP();
    ~NSTEP();

    virtual bool Handshake();
    const char *getDefaultName();

    bool initProperties();
    bool updateProperties();

    bool ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n);
    bool ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n);
    void TimerHit();
    IPState MoveRelFocuser(FocusDirection dir, unsigned int ticks);
    IPState MoveAbsFocuser(uint32_t targetTicks);
    bool AbortFocuser();
    bool SetFocuserSpeed(int speed);
    bool saveConfigItems(FILE *fp);

  private:
    bool command(const char *request, char *response, int count);

    IPState moveFocuserRelative(FocusDirection dir, unsigned int ticks);

    char buf[MAXRBUF];
    long sim_position { 0 };
    long position { 0 };
    int temperature { 0 };
    char steppingMode { 0 };
    char steppingPhase { 0 };
    pthread_mutex_t lock;

    INumber TempN[1];
    INumberVectorProperty TempNP;
    ISwitch TempCompS[2];
    ISwitchVectorProperty TempCompSP;
    INumber TempCompN[2];
    INumberVectorProperty TempCompNP;
    ISwitch SteppingModeS[3];
    ISwitchVectorProperty SteppingModeSP;
    ISwitch SteppingPhaseS[3];
    ISwitchVectorProperty SteppingPhaseSP;
};

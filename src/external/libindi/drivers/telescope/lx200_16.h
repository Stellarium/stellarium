/*
    LX200 16"
    Copyright (C) 2003 Jasem Mutlaq (mutlaqja@ikarustech.com)

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

#include "lx200gps.h"

class LX200_16 : public LX200GPS
{
  public:
    LX200_16();
    ~LX200_16() {}

    const char *getDefaultName();
    bool initProperties();
    bool updateProperties();
    void ISGetProperties(const char *dev);
    bool ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n);
    bool ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n);
    bool ReadScopeStatus();
    void getBasicData();
    bool handleAltAzSlew();

  protected:
    ISwitchVectorProperty FieldDeRotatorSP;
    ISwitch FieldDeRotatorS[2];

    ISwitchVectorProperty HomeSearchSP;
    ISwitch HomeSearchS[2];

    ISwitchVectorProperty FanStatusSP;
    ISwitch FanStatusS[2];

    INumberVectorProperty HorizontalCoordsNP;
    INumber HorizontalCoordsN[2];

  private:
    double targetAZ, targetALT;
    double currentAZ, currentALT;
};

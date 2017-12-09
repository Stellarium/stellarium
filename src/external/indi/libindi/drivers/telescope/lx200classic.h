/*
    LX200 Classic
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

#include "lx200generic.h"

class LX200Classic : public LX200Generic
{
  public:
    LX200Classic();
    ~LX200Classic() {}

    const char *getDefaultName();
    bool initProperties();
    void ISGetProperties(const char *dev);
    bool updateProperties();
    bool ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n);
    bool ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n);

  protected:
    ITextVectorProperty ObjectInfoTP;
    IText ObjectInfoT[1];

    ISwitchVectorProperty StarCatalogSP;
    ISwitch StarCatalogS[3];

    ISwitchVectorProperty DeepSkyCatalogSP;
    ISwitch DeepSkyCatalogS[7];

    ISwitchVectorProperty SolarSP;
    ISwitch SolarS[10];

    INumberVectorProperty ObjectNoNP;
    INumber ObjectNoN[1];

    INumberVectorProperty MaxSlewRateNP;
    INumber MaxSlewRateN[1];

    INumberVectorProperty ElevationLimitNP;
    INumber ElevationLimitN[2];

  private:
    int currentCatalog;
    int currentSubCatalog;
};

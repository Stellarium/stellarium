/*
    LX200 GPS
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

#include "lx200autostar.h"

class LX200GPS : public LX200Autostar
{
  public:
    LX200GPS();
    ~LX200GPS() {}

    const char *getDefaultName();
    bool initProperties();
    bool updateProperties();
    void ISGetProperties(const char *dev);
    bool ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n);
    virtual bool updateTime(ln_date *utc, double utc_offset);

  protected:
    virtual bool UnPark();

    ISwitchVectorProperty GPSPowerSP;
    ISwitch GPSPowerS[2];

    ISwitchVectorProperty GPSStatusSP;
    ISwitch GPSStatusS[3];

    ISwitchVectorProperty GPSUpdateSP;
    ISwitch GPSUpdateS[2];

    ISwitchVectorProperty AltDecPecSP;
    ISwitch AltDecPecS[2];

    ISwitchVectorProperty AzRaPecSP;
    ISwitch AzRaPecS[2];

    ISwitchVectorProperty SelenSyncSP;
    ISwitch SelenSyncS[1];

    ISwitchVectorProperty AltDecBacklashSP;
    ISwitch AltDecBacklashS[1];

    ISwitchVectorProperty AzRaBacklashSP;
    ISwitch AzRaBacklashS[1];

    ISwitchVectorProperty OTAUpdateSP;
    ISwitch OTAUpdateS[1];

    INumberVectorProperty OTATempNP;
    INumber OTATempN[1];
};

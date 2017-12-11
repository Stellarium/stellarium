/*
    LX200 OnStep
    based on LX200 Classic azwing (alain@zwingelstein.org)
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

#define UnParkOnStep(fd)   write(fd, "#:hR#", 5)            // azwing
#define setParkOnStep(fd)  write(fd, "#:hQ#", 5)            // azwing
#define EnaTrackOnStep(fd) write(fd, "#:Te#", 5)            // azwing
#define DisTrackOnStep(fd) write(fd, "#:Td#", 5)            // azwing
#define ReticPlus(fd)      write(fd, "#:B+#", 5)            // azwing
#define ReticMoins(fd)     write(fd, "#:B-#", 5)            // azwing
#define getStatus(fd, x)   getCommandString(fd, x, "#:GU#") // azwing

class LX200_OnStep : public LX200Generic
{
  public:
    LX200_OnStep();
    ~LX200_OnStep() {}

    virtual const char *getDefaultName() override;
    virtual bool initProperties() override;
    virtual void ISGetProperties(const char *dev) override;
    virtual bool updateProperties() override;
    virtual bool ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n) override;
    virtual bool ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n) override;

  protected:
    virtual void getBasicData() override;
    virtual bool UnPark() override;
    virtual bool SetTrackEnabled(bool enabled) override;

    virtual bool setLocalDate(uint8_t days, uint8_t months, uint8_t years) override;

    bool sendOnStepCommand(const char *cmd);

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

    ITextVectorProperty VersionTP;
    IText VersionT[5];

    // Enable / Disable Tracking
    //ISwitchVectorProperty EnaTrackSP;
    //ISwitch EnaTrackS[1];
    int IsTracking = 0;

    // Reticle +/- Buttons
    ISwitchVectorProperty ReticSP;
    ISwitch ReticS[2];

    char OnStepStatus[160];
    char OSStat[16];
    void OnStepStat();

  private:
    int currentCatalog;
    int currentSubCatalog;
};

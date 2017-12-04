/* Copyright 2014 Geehalel (geehalel AT gmail DOT com) */
/* This file is part of the Skywatcher Protocol INDI driver.

    The Skywatcher Protocol INDI driver is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    The Skywatcher Protocol INDI driver is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with the Skywatcher Protocol INDI driver.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <inditelescope.h>

#include <vector>

typedef struct horizonpoint
{
    double az;
    double alt;
} horizonpoint;

bool cmphorizonpoint(horizonpoint h1, horizonpoint h2);

class HorizonLimits
{
  protected:
  private:
    INDI::Telescope *telescope;
    ITextVectorProperty *HorizonLimitsDataFileTP;
    IBLOBVectorProperty *HorizonLimitsDataFitsBP;
    INumberVectorProperty *HorizonLimitsPointNP;
    ISwitchVectorProperty *HorizonLimitsTraverseSP;
    ISwitchVectorProperty *HorizonLimitsManageSP;
    ISwitchVectorProperty *HorizonLimitsFileOperationSP;
    ISwitchVectorProperty *HorizonLimitsOnLimitSP;
    ISwitchVectorProperty *HorizonLimitsLimitGotoSP;

    std::vector<horizonpoint> *horizon;
    int horizonindex;

    char *WriteDataFile(const char *filename);
    char *LoadDataFile(const char *filename);
    char errorline[128];
    char *sline;
    bool HorizonInitialized;
    
  public:
    HorizonLimits(INDI::Telescope *);
    virtual ~HorizonLimits();

    const char *getDeviceName(); // used for logger

    virtual bool initProperties();
    virtual void ISGetProperties();
    virtual bool updateProperties();
    virtual bool ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n);
    virtual bool ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n);
    virtual bool ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n);
    virtual bool ISNewBLOB(const char *dev, const char *name, int sizes[], int blobsizes[], char *blobs[],
                           char *formats[], char *names[], int n);

    virtual void Init();
    virtual void Reset();
    virtual bool inLimits(double az, double alt);
    virtual bool inGotoLimits(double az, double alt);
    virtual bool checkLimits(double az, double alt, INDI::Telescope::TelescopeStatus status, bool ingoto);
};

/*
    GotoNova INDI driver

    Copyright (C) 2017 Jasem Mutlaq

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

class LX200GotoNova : public LX200Generic
{
  public:
    LX200GotoNova();
    ~LX200GotoNova() {}

    virtual bool updateProperties() override;
    virtual bool initProperties() override;

    virtual bool ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n) override;

  protected:
    virtual const char *getDefaultName() override;

    virtual void getBasicData() override;
    virtual bool checkConnection() override;
    virtual bool isSlewComplete() override;

    virtual bool ReadScopeStatus() override;

    virtual bool SetSlewRate(int index) override;
    virtual bool SetTrackMode(uint8_t mode) override;
    virtual bool Goto(double, double) override;
    virtual bool Sync(double ra, double dec) override;
    virtual bool updateTime(ln_date *utc, double utc_offset) override;
    virtual bool updateLocation(double latitude, double longitude, double elevation) override;

    virtual bool saveConfigItems(FILE *fp) override;

    virtual bool Park() override;
    virtual bool UnPark() override;

  private:
    int setGotoNovaStandardProcedure(int fd, const char *data);
    void setGuidingEnabled(bool enable);
    int GotonovaSyncCMR(char *matchedObject);

    // Settings
    int setGotoNovaLatitude(double Lat);
    int setGotoNovaLongitude(double Long);
    int setGotoNovaUTCOffset(double hours);
    int setCalenderDate(int fd, int dd, int mm, int yy);

    // Motion
    int slewGotoNova();    

    // Park
    int setGotoNovaParkPosition(int position);

    // Track Mode
    int setGotoNovaTrackMode(int mode);
    int getGotoNovaTrackMode(int *mode);

    // Guide Rate
    int setGotoNovaGuideRate(int rate);
    int getGotoNovaGuideRate(int *rate);

    // Pier Side
    void syncSideOfPier();

    // Simulation
    void mountSim();

    // Custom Parking Position
    ISwitch ParkPositionS[5];
    ISwitchVectorProperty ParkPositionSP;
    enum { PS_NORTH_POLE, PS_LEFT_VERTICAL, PS_LEFT_HORIZON, PS_RIGHT_VERTICAL, PS_RIGHT_HORIZON };

    // Sync type
    ISwitch SyncCMRS[2];
    ISwitchVectorProperty SyncCMRSP;
    enum { USE_REGULAR_SYNC, USE_CMR_SYNC };


    /* Guide Rate */
    ISwitch GuideRateS[4];
    ISwitchVectorProperty GuideRateSP;

    bool isGuiding=false;
};

/*
    Astro-Physics INDI driver

    Copyright (C) 2014 Jasem Mutlaq

    Based on INDI Astrophysics Driver by Markus Wildi

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

#define SYNCCM  0
#define SYNCCMR 1

#define NOTESTABLISHED      0
#define ESTABLISHED         1
#define MOUNTNOTINITIALIZED 0
#define MOUNTINITIALIZED    1

class LX200AstroPhysics : public LX200Generic
{
  public:
    LX200AstroPhysics();
    ~LX200AstroPhysics() {}

    typedef enum { MCV_G, MCV_H, MCV_I, MCV_J, MCV_L, MCV_UNKNOWN} ControllerVersion;
    typedef enum { GTOCP1, GTOCP2, GTOCP3, GTOCP4, GTOCP_UNKNOWN} ServoVersion;

    virtual bool ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n) override;
    virtual bool ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n) override;
    virtual void ISGetProperties(const char *dev) override;

  protected:
    virtual const char *getDefaultName() override;
    virtual bool initProperties() override;
    virtual bool updateProperties() override;

    virtual bool ReadScopeStatus() override;
    virtual bool Handshake() override;
    virtual bool Disconnect() override;

    // Parking
    virtual bool SetCurrentPark() override;
    virtual bool SetDefaultPark() override;
    virtual bool Park() override;
    virtual bool UnPark() override;

    virtual bool Sync(double ra, double dec) override;
    virtual bool Goto(double, double) override;
    virtual bool updateTime(ln_date *utc, double utc_offset) override;
    virtual bool updateLocation(double latitude, double longitude, double elevation) override;
    virtual bool SetSlewRate(int index) override;

    virtual int  SendPulseCmd(int direction, int duration_msec) override;

    virtual bool getUTFOffset(double *offset) override;

    // Tracking
    virtual bool SetTrackMode(uint8_t mode) override;
    virtual bool SetTrackEnabled(bool enabled) override;
    virtual bool SetTrackRate(double raRate, double deRate) override;

    virtual bool saveConfigItems(FILE *fp) override;

    virtual void debugTriggered(bool enable) override;

    ISwitch StartUpS[2];
    ISwitchVectorProperty StartUpSP;

    INumber HourangleCoordsN[2];
    INumberVectorProperty HourangleCoordsNP;

    INumber HorizontalCoordsN[2];
    INumberVectorProperty HorizontalCoordsNP;

    ISwitch APSlewSpeedS[3];
    ISwitchVectorProperty APSlewSpeedSP;

    ISwitch SwapS[2];
    ISwitchVectorProperty SwapSP;

    ISwitch SyncCMRS[2];
    ISwitchVectorProperty SyncCMRSP;
    enum { USE_REGULAR_SYNC, USE_CMR_SYNC };

    ISwitch APGuideSpeedS[3];
    ISwitchVectorProperty APGuideSpeedSP;

    IText VersionT[1];
    ITextVectorProperty VersionInfo;

    IText DeclinationAxisT[1];
    ITextVectorProperty DeclinationAxisTP;

    INumber SlewAccuracyN[2];
    INumberVectorProperty SlewAccuracyNP;

  private:
    bool isMountInit();
    bool setBasicDataPart0();
    bool setBasicDataPart1();

    // Side of pier
    void syncSideOfPier();

    bool timeUpdated=false, locationUpdated=false;
    ControllerVersion controllerType = MCV_UNKNOWN;
    ServoVersion servoType = GTOCP_UNKNOWN;
    uint8_t initStatus = MOUNTNOTINITIALIZED;
};

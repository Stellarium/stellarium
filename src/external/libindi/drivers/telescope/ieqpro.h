/*
    INDI IEQ Pro driver

    Copyright (C) 2015 Jasem Mutlaq

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

#include "ieqprodriver.h"
#include "indiguiderinterface.h"
#include "inditelescope.h"

class IEQPro : public INDI::Telescope, public INDI::GuiderInterface
{
  public:

    IEQPro();
    ~IEQPro() = default;

    virtual bool ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n) override;
    virtual bool ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n) override;

  protected:
    virtual const char *getDefaultName() override;

    virtual bool Handshake() override;

    virtual bool initProperties() override;
    virtual bool updateProperties() override;

    virtual bool ReadScopeStatus() override;

    virtual bool MoveNS(INDI_DIR_NS dir, TelescopeMotionCommand command) override;
    virtual bool MoveWE(INDI_DIR_WE dir, TelescopeMotionCommand command) override;

    virtual bool saveConfigItems(FILE *fp) override;

    virtual bool Park() override;
    virtual bool UnPark() override;

    virtual bool Sync(double ra, double dec) override;
    virtual bool Goto(double, double) override;
    virtual bool Abort() override;

    virtual bool updateTime(ln_date *utc, double utc_offset) override;
    virtual bool updateLocation(double latitude, double longitude, double elevation) override;

    virtual void debugTriggered(bool enable) override;
    virtual void simulationTriggered(bool enable) override;

    // Parking
    virtual bool SetCurrentPark() override;
    virtual bool SetDefaultPark() override;

    // Track Mode
    virtual bool SetTrackMode(uint8_t mode) override;

    // Track Rate
    virtual bool SetTrackRate(double raRate, double deRate) override;

    // Track On/Off
    virtual bool SetTrackEnabled(bool enabled) override;

    // Slew Rate
    virtual bool SetSlewRate(int index) override;

    // Sim
    void mountSim();

    // Guide
    virtual IPState GuideNorth(float ms) override;
    virtual IPState GuideSouth(float ms) override;
    virtual IPState GuideEast(float ms) override;
    virtual IPState GuideWest(float ms) override;

  private:
    /**
        * @brief getStartupData Get initial mount info on startup.
        */
    void getStartupData();

    /* Firmware */
    IText FirmwareT[5];
    ITextVectorProperty FirmwareTP;

    /* Tracking Mode */
    //ISwitchVectorProperty TrackModeSP;
    //ISwitch TrackModeS[4];

    /* Custom Tracking Rate */
    //INumber CustomTrackRateN[1];
    //INumberVectorProperty CustomTrackRateNP;

    /* GPS Status */
    ISwitch GPSStatusS[3];
    ISwitchVectorProperty GPSStatusSP;

    /* Time Source */
    ISwitch TimeSourceS[3];
    ISwitchVectorProperty TimeSourceSP;

    /* Hemisphere */
    ISwitch HemisphereS[2];
    ISwitchVectorProperty HemisphereSP;

    /* Home Control */
    ISwitch HomeS[3];
    ISwitchVectorProperty HomeSP;

    /* Guide Rate */
    INumber GuideRateN[1];
    INumberVectorProperty GuideRateNP;

    unsigned int DBG_SCOPE;
    double currentRA, currentDEC;
    double targetRA, targetDEC;

    IEQInfo scopeInfo;
    FirmwareInfo firmwareInfo;
};

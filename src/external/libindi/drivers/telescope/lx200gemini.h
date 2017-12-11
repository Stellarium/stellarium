/*
    Losmandy Gemini INDI driver

    Copyright (C) 2017 Jasem Mutlaq

    Difference from LX200 Generic:

    1. Added Side of Pier
    2. Reimplemented isSlewComplete to use :Gv# since it is more reliable

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

class LX200Gemini : public LX200Generic
{
  public:
    LX200Gemini();
    ~LX200Gemini() {}

    virtual void ISGetProperties(const char *dev) override;
    virtual bool ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n) override;
    virtual bool ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n) override;

  protected:
    virtual const char *getDefaultName() override;

    virtual bool Connect() override;

    virtual bool initProperties() override ;
    virtual bool updateProperties() override;

    virtual bool isSlewComplete() override;
    virtual bool ReadScopeStatus() override;

    virtual bool Park()override ;
    virtual bool UnPark() override;

    virtual bool SetTrackMode(uint8_t mode) override;
    virtual bool SetTrackEnabled(bool enabled) override;

    virtual bool checkConnection() override;

    virtual bool saveConfigItems(FILE *fp) override;

  private:
    void syncSideOfPier();
    bool sleepMount();
    bool wakeupMount();

    bool getGeminiProperty(uint8_t propertyNumber, char* value);
    bool setGeminiProperty(uint8_t propertyNumber, char* value);

    // Checksum for private commands
    uint8_t calculateChecksum(char *cmd);

    INumber ManualSlewingSpeedN[1];
    INumberVectorProperty ManualSlewingSpeedNP;

    INumber GotoSlewingSpeedN[1];
    INumberVectorProperty GotoSlewingSpeedNP;

    INumber MoveSpeedN[1];
    INumberVectorProperty MoveSpeedNP;

    INumber GuidingSpeedN[1];
    INumberVectorProperty GuidingSpeedNP;

    INumber CenteringSpeedN[1];
    INumberVectorProperty CenteringSpeedNP;

    ISwitch ParkSettingsS[3];
    ISwitchVectorProperty ParkSettingsSP;
    enum
    {
        PARK_HOME,
        PARK_STARTUP,
        PARK_ZENITH
    };

    ISwitch StartupModeS[3];
    ISwitchVectorProperty StartupModeSP;
    enum
    {
        COLD_START,
        WARM_START,
        WARM_RESTART
    };

    enum
    {
        GEMINI_TRACK_SIDEREAL,
        GEMINI_TRACK_KING,
        GEMINI_TRACK_LUNAR,
        GEMINI_TRACK_SOLAR

    };

    enum MovementState
    {
        NO_MOVEMENT,
        TRACKING,
        GUIDING,
        CENTERING,
        SLEWING,
        STALLED
    };

    enum ParkingState
    {
        NOT_PARKED,
        PARKED,
        PARK_IN_PROGRESS
    };

    const uint8_t GEMINI_TIMEOUT = 3;

    void setTrackState(INDI::Telescope::TelescopeStatus state);
    void updateMovementState();
    MovementState getMovementState();
    ParkingState getParkingState();

    ParkingState priorParkingState = PARK_IN_PROGRESS;
};

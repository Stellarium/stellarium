/*******************************************************************************
 Copyright(c) 2017 Jasem Mutlaq. All rights reserved.

 Driver for using TheSky6 Pro Scripted operations for mounts via the TCP server.
 While this technically can operate any mount connected to the TheSky6 Pro, it is
 intended for Paramount mounts control.

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Library General Public
 License version 2 as published by the Free Software Foundation.
 .
 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Library General Public License for more details.
 .
 You should have received a copy of the GNU Library General Public License
 along with this library; see the file COPYING.LIB.  If not, write to
 the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 Boston, MA 02110-1301, USA.
*******************************************************************************/

#pragma once

#include "indiguiderinterface.h"
#include "inditelescope.h"

class Paramount : public INDI::Telescope, public INDI::GuiderInterface
{
  public:
    Paramount();
    virtual ~Paramount() = default;

    virtual const char *getDefaultName() override;
    virtual bool Handshake() override;
    virtual bool ReadScopeStatus() override;
    virtual bool initProperties() override;
    virtual bool updateProperties() override;

    virtual bool ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n) override;
    virtual bool ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n) override;

  protected:
    virtual bool MoveNS(INDI_DIR_NS dir, TelescopeMotionCommand command) override;
    virtual bool MoveWE(INDI_DIR_WE dir, TelescopeMotionCommand command) override;
    virtual bool Abort() override;

    virtual bool updateLocation(double latitude, double longitude, double elevation) override;
    virtual bool updateTime(ln_date *utc, double utc_offset) override;

    virtual bool SetParkPosition(double Axis1Value, double Axis2Value) override;
    virtual bool Goto(double, double) override;
    virtual bool Park() override;
    virtual bool UnPark() override;
    virtual bool Sync(double ra, double dec) override;

    // Tracking
    virtual bool SetTrackMode(uint8_t mode) override;
    virtual bool SetTrackRate(double raRate, double deRate) override;
    virtual bool SetTrackEnabled(bool enabled) override;

    // Parking
    virtual bool SetCurrentPark() override;
    virtual bool SetDefaultPark() override;

    // Guiding
    virtual IPState GuideNorth(float ms) override;
    virtual IPState GuideSouth(float ms) override;
    virtual IPState GuideEast(float ms) override;
    virtual IPState GuideWest(float ms) override;

  private:
    void mountSim();
    bool getMountRADE();
    bool isSlewComplete();

    bool sendTheSkyOKCommand(const char *command, const char *errorMessage);
    bool isTheSkyParked();
    bool isTheSkyTracking();
    bool startOpenLoopMotion(uint8_t motion, uint16_t rate);
    bool stopOpenLoopMotion();
    bool setTheSkyTracking(bool enable, bool isSidereal, double raRate, double deRate);

    double currentRA { 0 };
    double currentDEC { 90 };
    double targetRA { 0 };
    double targetDEC { 0 };

    ln_lnlat_posn lnobserver { 0, 0 };
    ln_hrz_posn lnaltaz { 0, 0 };
    unsigned int DBG_SCOPE { 0 };

    // Jog Rate
    INumber JogRateN[2];
    INumberVectorProperty JogRateNP;

    // Guide Rate
    INumber GuideRateN[2];
    INumberVectorProperty GuideRateNP;

    // Tracking Mode
    ISwitch TrackModeS[4];
    ISwitchVectorProperty TrackModeSP;

    // Tracking Rate
//    INumber TrackRateN[2];
//    INumberVectorProperty TrackRateNP;
};

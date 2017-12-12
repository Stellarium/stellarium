/*
    Celestron GPS
    Copyright (C) 2003-2017 Jasem Mutlaq (mutlaqja@ikarustech.com)

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

/*
    Version with experimental pulse guide support. GC 04.12.2015
*/

#pragma once

#include "celestrondriver.h"

#include "indiguiderinterface.h"
#include "inditelescope.h"

#define POLLMS 1000 /* poll period, ms */

//GUIDE: guider parent
class CelestronGPS : public INDI::Telescope, public INDI::GuiderInterface
{
  public:
    CelestronGPS();
    virtual ~CelestronGPS() {}

    virtual const char *getDefaultName() override;
    virtual bool Handshake() override;
    virtual bool ReadScopeStatus() override;
    virtual void ISGetProperties(const char *dev) override;
    virtual bool ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n) override;
    virtual bool ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n) override;
    virtual bool initProperties() override;
    virtual bool updateProperties() override;

    //GUIDE guideTimeout() funcion
    void guideTimeout(CELESTRON_DIRECTION calldir);

  protected:
    // Goto, Sync, and Motion
    virtual bool Goto(double ra, double dec) override;
    //bool GotoAzAlt(double az, double alt);
    virtual bool Sync(double ra, double dec) override;
    virtual bool MoveNS(INDI_DIR_NS dir, TelescopeMotionCommand command) override;
    virtual bool MoveWE(INDI_DIR_WE dir, TelescopeMotionCommand command) override;
    virtual bool Abort() override;

    // Time and Location
    virtual bool updateLocation(double latitude, double longitude, double elevation) override;
    virtual bool updateTime(ln_date *utc, double utc_offset) override;

    //GUIDE: guiding functions
    virtual IPState GuideNorth(float ms) override;
    virtual IPState GuideSouth(float ms) override;
    virtual IPState GuideEast(float ms) override;
    virtual IPState GuideWest(float ms) override;

    //GUIDE guideTimeoutHelper() function
    static void guideTimeoutHelperN(void *p);
    static void guideTimeoutHelperS(void *p);
    static void guideTimeoutHelperW(void *p);
    static void guideTimeoutHelperE(void *p);

    // Tracking
    virtual bool SetTrackMode(uint8_t mode) override;
    virtual bool SetTrackEnabled(bool enabled) override;

    // Parking
    virtual bool Park() override;
    virtual bool UnPark() override;
    virtual bool SetCurrentPark() override;
    virtual bool SetDefaultPark() override;

    virtual bool saveConfigItems(FILE *fp) override;

    virtual void simulationTriggered(bool enable) override;

    void mountSim();

    //GUIDE variables.
    int GuideNSTID;
    int GuideWETID;
    CELESTRON_DIRECTION guide_direction;

    /* Firmware */
    IText FirmwareT[5];
    ITextVectorProperty FirmwareTP;

    //INumberVectorProperty HorizontalCoordsNP;
    //INumber HorizontalCoordsN[2];

    //ISwitch TrackS[4];
    //ISwitchVectorProperty TrackSP;

    //GUIDE Pulse guide switch
    ISwitchVectorProperty UsePulseCmdSP;
    ISwitch UsePulseCmdS[2];

    ISwitchVectorProperty UseHibernateSP;
    ISwitch UseHibernateS[2];

  private:
    bool setTrackMode(CELESTRON_TRACK_MODE mode);

    double currentRA, currentDEC, currentAZ, currentALT;
    double targetRA, targetDEC, targetAZ, targetALT;

    FirmwareInfo fwInfo;
};

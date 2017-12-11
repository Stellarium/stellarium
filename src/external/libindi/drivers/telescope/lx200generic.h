/*
    LX200 Generic
    Copyright (C) 2003-2015 Jasem Mutlaq (mutlaqja@ikarustech.com)

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

#include "indiguiderinterface.h"
#include "inditelescope.h"

class LX200Generic : public INDI::Telescope, public INDI::GuiderInterface
{
  public:
    LX200Generic();
    virtual ~LX200Generic() = default;

    /**
     * \struct LX200Capability
     * \brief Holds properties of LX200 Generic that might be used by child classes
     */
    enum
    {
        LX200_HAS_FOCUS             = 1 << 0, /** Define focus properties */
        LX200_HAS_TRACKING_FREQ     = 1 << 1, /** Define Tracking Frequency */
        LX200_HAS_ALIGNMENT_TYPE    = 1 << 2, /** Define Alignment Type */
        LX200_HAS_SITES             = 1 << 3, /** Define Sites */
        LX200_HAS_PULSE_GUIDING     = 1 << 4, /** Define Pulse Guiding */
    } LX200Capability;

    uint32_t getLX200Capability() const { return genericCapability; }
    void setLX200Capability(uint32_t cap) { genericCapability = cap; }

    virtual const char *getDefaultName() override;
    virtual const char *getDriverName() override;
    virtual bool Handshake() override;
    virtual bool ReadScopeStatus() override;
    virtual void ISGetProperties(const char *dev) override;
    virtual bool initProperties() override;
    virtual bool updateProperties() override;
    virtual bool ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n) override;
    virtual bool ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n) override;
    virtual bool ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n) override;

    void updateFocusTimer();
    void guideTimeout();

  protected:
    // Slew Rate
    virtual bool SetSlewRate(int index) override;
    // Track Mode (Sidereal, Solar..etc)
    virtual bool SetTrackMode(uint8_t mode) override;

    // NSWE Motion Commands
    virtual bool MoveNS(INDI_DIR_NS dir, TelescopeMotionCommand command) override;
    virtual bool MoveWE(INDI_DIR_WE dir, TelescopeMotionCommand command) override;

    // Abort ALL motion
    virtual bool Abort() override;

    // Time and Location
    virtual bool updateTime(ln_date *utc, double utc_offset) override;
    virtual bool updateLocation(double latitude, double longitude, double elevation) override;

    // Guide Commands
    virtual IPState GuideNorth(float ms) override;
    virtual IPState GuideSouth(float ms) override;
    virtual IPState GuideEast(float ms) override;
    virtual IPState GuideWest(float ms) override;

    // Guide Pulse Commands
    virtual int SendPulseCmd(int direction, int duration_msec);

    // Goto
    virtual bool Goto(double ra, double dec) override;

    // Is slew over?
    virtual bool isSlewComplete();

    // Park Mount
    virtual bool Park() override;

    // Sync coordinates
    virtual bool Sync(double ra, double dec) override;

    // Check if mount is responsive
    virtual bool checkConnection();

    // Save properties in config file
    virtual bool saveConfigItems(FILE *fp) override;

    // Action to perform when Debug is turned on or off
    virtual void debugTriggered(bool enable) override;

    // Initial function to get data after connection is successful
    virtual void getBasicData();

    // Get local calender date (NOT UTC) from mount. Expected format is YYYY-MM-DD
    virtual bool getLocalDate(char *dateString);
    virtual bool setLocalDate(uint8_t days, uint8_t months, uint8_t years);

    // Get Local time in 24 hour format from mount. Expected format is HH:MM:SS
    virtual bool getLocalTime(char *timeString);
    virtual bool setLocalTime24(uint8_t hour, uint8_t minute, uint8_t second);

    // Return UTC Offset from mount in hours.
    virtual bool setUTCOffset(double offset);
    virtual bool getUTFOffset(double * offset);

    // Send slew error message to client
    void slewError(int slewCode);

    // Get mount alignment type (AltAz..etc)
    void getAlignment();

    // Send Mount time and location settings to client
    bool sendScopeTime();
    bool sendScopeLocation();

    // Simulate Mount in simulation mode
    void mountSim();

    static void updateFocusHelper(void *p);
    static void guideTimeoutHelper(void *p);

    int GuideNSTID;
    int GuideWETID;

    int timeFormat=-1;
    int currentSiteNum;
    int trackingMode;
    long guide_direction;
    bool sendTimeOnStartup=true, sendLocationOnStartup=true;

    unsigned int DBG_SCOPE;

    double JD;
    double targetRA, targetDEC;
    double currentRA, currentDEC;
    int MaxReticleFlashRate;

    /* Telescope Alignment Mode */
    ISwitchVectorProperty AlignmentSP;
    ISwitch AlignmentS[3];

    /* Tracking Frequency */
    INumberVectorProperty TrackingFreqNP;
    INumber TrackFreqN[1];

    /* Use pulse-guide commands */
    ISwitchVectorProperty UsePulseCmdSP;
    ISwitch UsePulseCmdS[2];

    /* Site Management */
    ISwitchVectorProperty SiteSP;
    ISwitch SiteS[4];

    /* Site Name */
    ITextVectorProperty SiteNameTP;
    IText SiteNameT[1];

    /* Focus motion */
    ISwitchVectorProperty FocusMotionSP;
    ISwitch FocusMotionS[2];

    /* Focus Timer */
    INumberVectorProperty FocusTimerNP;
    INumber FocusTimerN[1];

    /* Focus Mode */
    ISwitchVectorProperty FocusModeSP;
    ISwitch FocusModeS[3];

    uint32_t genericCapability;

};

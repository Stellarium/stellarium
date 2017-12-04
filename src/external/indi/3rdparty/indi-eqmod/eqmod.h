/* Copyright 2012 Geehalel (geehalel AT gmail DOT com) */
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

#include "config.h"
#include "skywatcher.h"
#ifdef WITH_ALIGN_GEEHALEL
#include "align/align.h"
#endif
#include "simulator/simulator.h"
#ifdef WITH_SCOPE_LIMITS
#include "scope-limits/scope-limits.h"
#endif

#include <inditelescope.h>
#include <indiguiderinterface.h>

#include <libnova/ln_types.h>

typedef struct SyncData
{
    double lst, jd;
    double targetRA, targetDEC;
    double telescopeRA, telescopeDEC;
    double deltaRA, deltaDEC;
    unsigned long targetRAEncoder, targetDECEncoder;
    unsigned long telescopeRAEncoder, telescopeDECEncoder;
    long deltaRAEncoder, deltaDECEncoder;
} SyncData;
#ifdef WITH_ALIGN

#include <alignment/AlignmentSubsystemForDrivers.h>

class EQMod : public INDI::Telescope,
              public INDI::GuiderInterface,
              INDI::AlignmentSubsystem::AlignmentSubsystemForDrivers
#else
class EQMod : public INDI::Telescope, public INDI::GuiderInterface
#endif
{
  protected:
  private:
    Skywatcher *mount;

    unsigned long currentRAEncoder, zeroRAEncoder, totalRAEncoder;
    unsigned long currentDEEncoder, zeroDEEncoder, totalDEEncoder;

    unsigned long homeRAEncoder, parkRAEncoder;
    unsigned long homeDEEncoder, parkDEEncoder;

    double currentRA, currentHA;
    double currentDEC;
    double alignedRA, alignedDEC;
    double ghalignedRA, ghalignedDEC;
    double targetRA;
    double targetDEC;

#ifdef WITH_ALIGN_GEEHALEL
    Align *align;
#endif

    int last_motion_ns;
    int last_motion_ew;

    /* for use with libnova */
    struct ln_equ_posn lnradec;
    struct ln_lnlat_posn lnobserver;
    struct ln_hrz_posn lnaltaz;

    /* Time variables */
    struct tm utc;
    struct ln_date lndate;
    struct timeval lasttimeupdate;
    struct timespec lastclockupdate;
    double juliandate;

    int GuideTimerNS;

    int GuideTimerWE;

    INumber *GuideRateN                        = NULL;
    INumberVectorProperty *GuideRateNP         = NULL;
    ITextVectorProperty *MountInformationTP    = NULL;
    INumberVectorProperty *SteppersNP          = NULL;
    INumberVectorProperty *CurrentSteppersNP   = NULL;
    INumberVectorProperty *PeriodsNP           = NULL;
    INumberVectorProperty *JulianNP            = NULL;
    INumberVectorProperty *TimeLSTNP           = NULL;
    ILightVectorProperty *RAStatusLP           = NULL;
    ILightVectorProperty *DEStatusLP           = NULL;
    INumberVectorProperty *SlewSpeedsNP        = NULL;
    ISwitchVectorProperty *HemisphereSP        = NULL;    
    ISwitchVectorProperty *TrackDefaultSP      = NULL;
    INumberVectorProperty *HorizontalCoordNP   = NULL;
    INumberVectorProperty *StandardSyncNP      = NULL;
    INumberVectorProperty *StandardSyncPointNP = NULL;
    INumberVectorProperty *SyncPolarAlignNP    = NULL;
    ISwitchVectorProperty *SyncManageSP        = NULL;
    ISwitchVectorProperty *ReverseDECSP        = NULL;
    INumberVectorProperty *BacklashNP          = NULL;
    ISwitchVectorProperty *UseBacklashSP       = NULL;
#if defined WITH_ALIGN && defined WITH_ALIGN_GEEHALEL
    ISwitch AlignMethodS[2];
    ISwitchVectorProperty AlignMethodSP;
#endif
#if defined WITH_ALIGN || defined WITH_ALIGN_GEEHALEL
    ISwitchVectorProperty *AlignSyncModeSP = NULL;
#endif
    ISwitchVectorProperty *AutoHomeSP   = NULL;
    ISwitchVectorProperty *AuxEncoderSP = NULL;
    INumberVectorProperty *AuxEncoderNP = NULL;

    ISwitchVectorProperty *ST4GuideRateNSSP = NULL;
    ISwitchVectorProperty *ST4GuideRateWESP = NULL;

    ISwitchVectorProperty *RAPPECTrainingSP = NULL;
    ISwitchVectorProperty *DEPPECTrainingSP = NULL;
    ISwitchVectorProperty *RAPPECSP         = NULL;
    ISwitchVectorProperty *DEPPECSP         = NULL;

    enum Hemisphere
    {
        NORTH = 0,
        SOUTH = 1
    };

    typedef struct GotoParams
    {
        double ratarget, detarget, racurrent, decurrent;
        unsigned long ratargetencoder, detargetencoder, racurrentencoder, decurrentencoder;
        unsigned long limiteast, limitwest;
        unsigned int iterative_count;
        bool forcecwup, checklimits, outsidelimits, completed;
    } GotoParams;

    Hemisphere Hemisphere;
    bool RAInverted, DEInverted;
    GotoParams gotoparams;
    SyncData syncdata, syncdata2;

    double tpa_alt, tpa_az;

    void EncodersToRADec(unsigned long rastep, unsigned long destep, double lst, double *ra, double *de, double *ha);
    double EncoderToHours(unsigned long destep, unsigned long initdestep, unsigned long totalrastep, enum Hemisphere h);
    double EncoderToDegrees(unsigned long destep, unsigned long initdestep, unsigned long totalrastep,
                            enum Hemisphere h);
    double EncoderFromHour(double hour, unsigned long initstep, unsigned long totalstep, enum Hemisphere h);
    double EncoderFromRA(double ratarget, double detarget, double lst, unsigned long initstep, unsigned long totalstep,
                         enum Hemisphere h);
    double EncoderFromDegree(double degree, TelescopePierSide p, unsigned long initstep, unsigned long totalstep,
                             enum Hemisphere h);
    double EncoderFromDec(double detarget, TelescopePierSide p, unsigned long initstep, unsigned long totalstep,
                          enum Hemisphere h);
    void EncoderTarget(GotoParams *g);
    void SetSouthernHemisphere(bool southern);
    double GetRATrackRate();
    double GetDETrackRate();
    double GetDefaultRATrackRate();
    double GetDefaultDETrackRate();
    static void timedguideNSCallback(void *userpointer);
    static void timedguideWECallback(void *userpointer);
    double GetRASlew();
    double GetDESlew();
    bool gotoInProgress();

    bool loadProperties();

    void setStepperSimulation(bool enable);

    void computePolarAlign(SyncData s1, SyncData s2, double lat, double *tpaalt, double *tpaaz);
    void starPolarAlign(double lst, double ra, double dec, double theta, double gamma, double *tra, double *tdec);
#if defined WITH_ALIGN || defined WITH_ALIGN_GEEHALEL
    bool isStandardSync();
#endif
    // Autohoming for EQ8
    int ah_confirm_timeout;
    bool ah_bSlewingUp_RA, ah_bSlewingUp_DE;
    unsigned long ah_iPosition_RA, ah_iPosition_DE;
    int ah_iChanges;
    bool ah_bIndexChanged_RA, ah_bIndexChanged_DE;
    unsigned long ah_sHomeIndexPosition_RA, ah_sHomeIndexPosition_DE;
    int ah_waitRA, ah_waitDE;

    // save PPEC status when guiding
    bool restartguideRAPPEC;
    bool restartguideDEPPEC;

  public:
    EQMod();
    virtual ~EQMod();

    virtual const char *getDefaultName();
    virtual bool Handshake();
    virtual bool Disconnect();
    virtual void TimerHit();
    virtual bool ReadScopeStatus();
    virtual bool initProperties();
    virtual bool updateProperties();
    virtual void ISGetProperties(const char *dev);
    virtual bool ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n);
    virtual bool ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n);
    virtual bool ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n);
#ifdef WITH_ALIGN
    virtual bool ISNewBLOB(const char *dev, const char *name, int sizes[], int blobsizes[], char *blobs[],
                           char *formats[], char *names[], int n);
#endif
    virtual bool MoveNS(INDI_DIR_NS dir, TelescopeMotionCommand command);
    virtual bool MoveWE(INDI_DIR_WE dir, TelescopeMotionCommand command);
    virtual bool Abort();

    virtual IPState GuideNorth(float ms);
    virtual IPState GuideSouth(float ms);
    virtual IPState GuideEast(float ms);
    virtual IPState GuideWest(float ms);

    bool Goto(double ra, double dec);
    bool Park();
    bool UnPark();
    bool SetCurrentPark();
    bool SetDefaultPark();
    bool Sync(double ra, double dec);

    // Tracking
    bool SetTrackMode(uint8_t mode);
    bool SetTrackRate(double raRate, double deRate);
    bool SetTrackEnabled(bool enabled);

    virtual bool saveConfigItems(FILE *fp);

    bool updateTime(ln_date *lndate_utc, double utc_offset);
    bool updateLocation(double latitude, double longitude, double elevation);

    double getLongitude();
    double getLatitude();
    double getJulianDate();
    double getLst(double jd, double lng);

    EQModSimulator *simulator;

#ifdef WITH_SCOPE_LIMITS
    HorizonLimits *horizon;
#endif
    // AutoHoming for EQ8
    static const TelescopeStatus SCOPE_AUTOHOMING = static_cast<TelescopeStatus>(SCOPE_PARKED + 1);
    enum AutoHomeStatus
    {
        AUTO_HOME_IDLE,
        AUTO_HOME_CONFIRM,
        AUTO_HOME_WAIT_PHASE1,
        AUTO_HOME_WAIT_PHASE2,
        AUTO_HOME_WAIT_PHASE3,
        AUTO_HOME_WAIT_PHASE4,
        AUTO_HOME_WAIT_PHASE5,
        AUTO_HOME_WAIT_PHASE6
    };
    AutoHomeStatus AutohomeState;
};

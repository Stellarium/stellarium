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

#include "eqmoderror.h"

#include <inditelescope.h>

#include <lilxml.h>

#include <time.h>
#include <sys/time.h>

class EQMod; // TODO

#include "simulator/simulator.h"

#define SKYWATCHER_MAX_CMD      16
#define SKYWATCHER_MAX_TRIES    3
#define SKYWATCHER_ERROR_BUFFER 1024

#define SKYWATCHER_SIDEREAL_DAY   86164.09053083288
#define SKYWATCHER_SIDEREAL_SPEED 15.04106864
#define SKYWATCHER_STELLAR_DAY    86164.098903691
#define SKYWATCHER_STELLAR_SPEED  15.041067179

#define SKYWATCHER_LOWSPEED_RATE 128
#define SKYWATCHER_MAXREFRESH    0.5

#define SKYWATCHER_BACKLASH_SPEED_RA 64
#define SKYWATCHER_BACKLASH_SPEED_DE 64

#define HEX(c) (((c) < 'A') ? ((c) - '0') : ((c) - 'A') + 10)

class Skywatcher
{
  public:
    Skywatcher(EQMod *t);
    ~Skywatcher();

    bool Handshake();
    bool Disconnect();
    void setDebug(bool enable);
    const char *getDeviceName();

    bool HasHomeIndexers();
    bool HasAuxEncoders();
    bool HasPPEC();

    unsigned long GetRAEncoder();
    unsigned long GetDEEncoder();
    unsigned long GetRAEncoderZero();
    unsigned long GetRAEncoderTotal();
    unsigned long GetRAEncoderHome();
    unsigned long GetDEEncoderZero();
    unsigned long GetDEEncoderTotal();
    unsigned long GetDEEncoderHome();
    unsigned long GetRAPeriod();
    unsigned long GetDEPeriod();
    void GetRAMotorStatus(ILightVectorProperty *motorLP);
    void GetDEMotorStatus(ILightVectorProperty *motorLP);
    void InquireBoardVersion(ITextVectorProperty *boardTP);
    void InquireFeatures();
    void InquireRAEncoderInfo(INumberVectorProperty *encoderNP);
    void InquireDEEncoderInfo(INumberVectorProperty *encoderNP);
    void Init();
    void SlewRA(double rate);
    void SlewDE(double rate);
    void StopRA();
    void StopDE();
    void SetRARate(double rate);
    void SetDERate(double rate);
    void SlewTo(long deltaraencoder, long deltadeencoder);
    void AbsSlewTo(unsigned long raencoder, unsigned long deencoder, bool raup, bool deup);
    void StartRATracking(double trackspeed);
    void StartDETracking(double trackspeed);
    bool IsRARunning();
    bool IsDERunning();
    // For AstroEQ (needs an explicit :G command at the end of gotos)
    void ResetMotions();
    void setSimulation(bool);
    bool isSimulation();
    bool simulation;

    // Backlash
    void SetBacklashRA(unsigned long backlash);
    void SetBacklashUseRA(bool usebacklash);
    void SetBacklashDE(unsigned long backlash);
    void SetBacklashUseDE(bool usebacklash);

    unsigned long GetlastreadRAIndexer();
    unsigned long GetlastreadDEIndexer();
    unsigned long GetRAAuxEncoder();
    unsigned long GetDEAuxEncoder();
    void TurnRAEncoder(bool on);
    void TurnDEEncoder(bool on);
    void TurnRAPPECTraining(bool on);
    void TurnDEPPECTraining(bool on);
    void TurnRAPPEC(bool on);
    void TurnDEPPEC(bool on);
    void GetRAPPECStatus(bool *intraining, bool *inppec);
    void GetDEPPECStatus(bool *intraining, bool *inppec);
    void ResetRAIndexer();
    void ResetDEIndexer();
    void GetRAIndexer();
    void GetDEIndexer();
    void SetRAAxisPosition(unsigned long step);
    void SetDEAxisPosition(unsigned long step);
    void SetST4RAGuideRate(unsigned char r);
    void SetST4DEGuideRate(unsigned char r);

    void setPortFD(int value);

  private:
    // Official Skywatcher Protocol
    // See http://code.google.com/p/skywatcher/wiki/SkyWatcherProtocol
    // Constants
    static const char SkywatcherLeadingChar  = ':';
    static const char SkywatcherTrailingChar = 0x0d;
    static constexpr double MIN_RATE         = 0.05;
    static constexpr double MAX_RATE         = 800.0;
    unsigned long minperiods[2];

    // Types
    enum SkywatcherCommand
    {
        Initialize                = 'F',
        InquireMotorBoardVersion  = 'e',
        InquireGridPerRevolution  = 'a',
        InquireTimerInterruptFreq = 'b',
        InquireHighSpeedRatio     = 'g',
        InquirePECPeriod          = 's',
        InstantAxisStop           = 'L',
        NotInstantAxisStop        = 'K',
        SetAxisPositionCmd        = 'E',
        GetAxisPosition           = 'j',
        GetAxisStatus             = 'f',
        SetSwitch                 = 'O',
        SetMotionMode             = 'G',
        SetGotoTargetIncrement    = 'H',
        SetBreakPointIncrement    = 'M',
        SetGotoTarget             = 'S',
        SetBreakStep              = 'U',
        SetStepPeriod             = 'I',
        StartMotion               = 'J',
        GetStepPeriod             = 'D', // See Merlin protocol http://www.papywizard.org/wiki/DevelopGuide
        ActivateMotor             = 'B', // See eq6direct implementation http://pierre.nerzic.free.fr/INDI/
        SetST4GuideRateCmd        = 'P',
        GetHomePosition           = 'd', // Get Home position encoder count (default at startup)
        SetFeatureCmd             = 'W', // EQ8/AZEQ6/AZEQ5 only
        GetFeatureCmd             = 'q', // EQ8/AZEQ6/AZEQ5 only
        InquireAuxEncoder         = 'd', // EQ8/AZEQ6/AZEQ5 only
        NUMBER_OF_SkywatcherCommand
    };

    enum SkywatcherAxis
    {
        Axis1 = 0, // RA/AZ
        Axis2 = 1, // DE/ALT
        NUMBER_OF_SKYWATCHERAXIS
    };
    char AxisCmd[2] {'1', '2'};

    enum SkywatcherDirection
    {
        BACKWARD = 0,
        FORWARD  = 1
    };
    enum SkywatcherSlewMode
    {
        SLEW = 0,
        GOTO = 1
    };
    enum SkywatcherSpeedMode
    {
        LOWSPEED  = 0,
        HIGHSPEED = 1
    };

    typedef struct SkyWatcherFeatures
    {
        bool inPPECTraining = false;
        bool inPPEC = false;
        bool hasEncoder = false;
        bool hasPPEC = false;
        bool hasHomeIndexer = false;
        bool isAZEQ = false;
        bool hasPolarLed = false;
        bool hasCommonSlewStart = false; // supports :J3
        bool hasHalfCurrentTracking = false;
        bool hasWifi = false;
    } SkyWatcherFeatures;

    enum SkywatcherGetFeatureCmd
    {
        GET_INDEXER_CMD  = 0x00,
        GET_FEATURES_CMD = 0x01
    };
    enum SkywatcherSetFeatureCmd
    {
        START_PPEC_TRAINING_CMD            = 0x00,
        STOP_PPEC_TRAINING_CMD             = 0x01,
        TURN_PPEC_ON_CMD                   = 0x02,
        TURN_PPEC_OFF_CMD                  = 0X03,
        ENCODER_ON_CMD                     = 0x04,
        ENCODER_OFF_CMD                    = 0x05,
        DISABLE_FULL_CURRENT_LOW_SPEED_CMD = 0x0006,
        ENABLE_FULL_CURRENT_LOW_SPEED_CMD  = 0x0106,
        RESET_HOME_INDEXER_CMD             = 0x08
    };

    typedef struct SkywatcherAxisStatus
    {
        SkywatcherDirection direction;
        SkywatcherSlewMode slewmode;
        SkywatcherSpeedMode speedmode;
    } SkywatcherAxisStatus;
    enum SkywatcherError
    {
        NO_ERROR,
        ER_1,
        ER_2,
        ER_3
    };

    struct timeval lastreadmotorstatus[NUMBER_OF_SKYWATCHERAXIS];
    struct timeval lastreadmotorposition[NUMBER_OF_SKYWATCHERAXIS];

    // Functions
    void CheckMotorStatus(SkywatcherAxis axis);
    void ReadMotorStatus(SkywatcherAxis axis);
    void SetMotion(SkywatcherAxis axis, SkywatcherAxisStatus newstatus);
    void SetSpeed(SkywatcherAxis axis, unsigned long period);
    void SetTarget(SkywatcherAxis axis, unsigned long increment);
    void SetTargetBreaks(SkywatcherAxis axis, unsigned long increment);
    void SetAbsTarget(SkywatcherAxis axis, unsigned long target);
    void SetAbsTargetBreaks(SkywatcherAxis axis, unsigned long breakstep);
    void StartMotor(SkywatcherAxis axis);
    void StopMotor(SkywatcherAxis axis);
    void InstantStopMotor(SkywatcherAxis axis);
    void StopWaitMotor(SkywatcherAxis axis);
    void SetFeature(SkywatcherAxis axis, unsigned long command);
    void GetFeature(SkywatcherAxis axis, unsigned long command);
    void TurnEncoder(SkywatcherAxis axis, bool on);
    unsigned long ReadEncoder(SkywatcherAxis axis);
    void ResetIndexer(SkywatcherAxis axis);
    void GetIndexer(SkywatcherAxis axis);
    void SetST4GuideRate(SkywatcherAxis axis, unsigned char r);
    void SetAxisPosition(SkywatcherAxis axis, unsigned long step);
    void TurnPPECTraining(SkywatcherAxis axis, bool on);
    void TurnPPEC(SkywatcherAxis axis, bool on);
    void GetPPECStatus(SkywatcherAxis axis, bool *intraining, bool *inppec);

    bool read_eqmod();
    bool dispatch_command(SkywatcherCommand cmd, SkywatcherAxis axis, char *arg);

    unsigned long Revu24str2long(char *);
    unsigned long Highstr2long(char *);
    void long2Revu24str(unsigned long, char *);

    double get_min_rate();
    double get_max_rate();
    bool isDebug();

    // Variables
    //string default_port;
    // See Skywatcher protocol
    unsigned long MCVersion; // Motor Controller Version
    unsigned long MountCode; //

    unsigned long RASteps360;
    unsigned long DESteps360;
    unsigned long RAStepsWorm;
    unsigned long DEStepsWorm;
    unsigned long RAHighspeedRatio; // Motor controller multiplies speed values by this ratio when in low speed mode
    unsigned long
        DEHighspeedRatio; // This is a reflect of either using a timer interrupt with an interrupt count greater than 1 for low speed
                          // or of using microstepping only for low speeds and half/full stepping for high speeds
    unsigned long RAStep;     // Current RA encoder position in step
    unsigned long DEStep;     // Current DE encoder position in step
    unsigned long RAStepInit; // Initial RA position in step
    unsigned long DEStepInit; // Initial DE position in step
    unsigned long RAStepHome; // Home RA position in step
    unsigned long DEStepHome; // Home DE position in step
    unsigned long RAPeriod;   // Current RA worm period
    unsigned long DEPeriod;   // Current DE worm period

    bool RAInitialized, DEInitialized, RARunning, DERunning;
    bool wasinitialized;
    SkywatcherAxisStatus RAStatus, DEStatus;
    SkyWatcherFeatures AxisFeatures[NUMBER_OF_SKYWATCHERAXIS];

    int PortFD = -1;
    char command[SKYWATCHER_MAX_CMD];
    char response[SKYWATCHER_MAX_CMD];

    bool debug;
    bool debugnextread;
    EQMod *telescope;
    bool reconnect;

    // Backlash
    unsigned long Backlash[NUMBER_OF_SKYWATCHERAXIS];
    bool UseBacklash[NUMBER_OF_SKYWATCHERAXIS];
    unsigned long Target[NUMBER_OF_SKYWATCHERAXIS];
    unsigned long TargetBreaks[NUMBER_OF_SKYWATCHERAXIS];
    SkywatcherAxisStatus LastRunningStatus[NUMBER_OF_SKYWATCHERAXIS];
    SkywatcherAxisStatus NewStatus[NUMBER_OF_SKYWATCHERAXIS];
    unsigned long backlashperiod[NUMBER_OF_SKYWATCHERAXIS];

    unsigned long lastreadIndexer[NUMBER_OF_SKYWATCHERAXIS];
};

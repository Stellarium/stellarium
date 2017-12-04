
#pragma once

#include "indiguiderinterface.h"
#include "inditelescope.h"
#include "alignment/AlignmentSubsystemForDrivers.h"

class ScopeSim : public INDI::Telescope, public INDI::AlignmentSubsystem::AlignmentSubsystemForDrivers
{
  public:
    ScopeSim();

private:
    virtual bool Abort() override;
    bool canSync();
    virtual bool Connect() override;
    virtual bool Disconnect() override;
    virtual const char *getDefaultName() override;
    virtual bool Goto(double ra, double dec) override;
    virtual bool initProperties() override;
    friend void ISNewBLOB(const char *dev, const char *name, int sizes[], int blobsizes[], char *blobs[],
                          char *formats[], char *names[], int n);
    virtual bool ISNewBLOB(const char *dev, const char *name, int sizes[], int blobsizes[], char *blobs[],
                           char *formats[], char *names[], int n) override;
    friend void ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n);
    virtual bool ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n) override;
    friend void ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n);
    virtual bool ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n) override;
    friend void ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n);
    virtual bool ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n) override;
    virtual bool MoveNS(INDI_DIR_NS dir, TelescopeMotionCommand command) override;
    virtual bool MoveWE(INDI_DIR_WE dir, TelescopeMotionCommand command) override;
    virtual bool ReadScopeStatus() override;
    virtual bool Sync(double ra, double dec) override;
    virtual void TimerHit() override;
    virtual bool updateLocation(double latitude, double longitude, double elevation) override;

    static constexpr long MICROSTEPS_PER_REVOLUTION { 1000000 };
    static constexpr double MICROSTEPS_PER_DEGREE { MICROSTEPS_PER_REVOLUTION / 360.0 };
    static constexpr double DEFAULT_SLEW_RATE { MICROSTEPS_PER_DEGREE * 2.0 };
    static constexpr long MAX_DEC { (long)(90.0 * MICROSTEPS_PER_DEGREE) };
    static constexpr long MIN_DEC { (long)(-90.0 * MICROSTEPS_PER_DEGREE) };

    enum AxisStatus
    {
        STOPPED,
        SLEWING,
        SLEWING_TO
    };
    enum AxisDirection
    {
        FORWARD,
        REVERSE
    };

    AxisStatus AxisStatusDEC { STOPPED };
    AxisDirection AxisDirectionDEC { FORWARD };
    double AxisSlewRateDEC { DEFAULT_SLEW_RATE };
    long CurrentEncoderMicrostepsDEC { 0 };
    long GotoTargetMicrostepsDEC { 0 };

    AxisStatus AxisStatusRA { STOPPED };
    AxisDirection AxisDirectionRA { FORWARD };
    double AxisSlewRateRA { DEFAULT_SLEW_RATE };
    long CurrentEncoderMicrostepsRA { 0 };
    long GotoTargetMicrostepsRA { 0 };

    // Tracking
    ln_equ_posn CurrentTrackingTarget { 0, 0 };

    // Tracing in timer tick
    int TraceThisTickCount { 0 };
    bool TraceThisTick { false };

    unsigned int DBG_SIMULATOR { 0 };
};

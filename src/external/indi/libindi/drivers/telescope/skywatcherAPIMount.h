/*!
 * \file skywatcherAPIMount.h
 *
 * \author Roger James
 * \author Gerry Rozema
 * \author Jean-Luc Geehalel
 * \date 13th November 2013
 *
 * This file contains the definitions for a C++ implementatiom of a INDI telescope driver using the Skywatcher API.
 * It is based on work from three sources.
 * A C++ implementation of the API by Roger James.
 * The indi_eqmod driver by Jean-Luc Geehalel.
 * The synscanmount driver by Gerry Rozema.
 */

#pragma once

#include "indiguiderinterface.h"
#include "skywatcherAPI.h"

#include "alignment/AlignmentSubsystemForDrivers.h"

typedef enum { PARK_COUNTERCLOCKWISE = 0, PARK_CLOCKWISE } ParkDirection_t;
typedef enum { PARK_NORTH = 0, PARK_EAST, PARK_SOUTH, PARK_WEST } ParkPosition_t;

struct GuidingPulse
{
    double DeltaAlt { 0 };
    double DeltaAz { 0 };
    int Duration { 0 };
    int OriginalDuration { 0 };
};


class SkywatcherAPIMount : public SkywatcherAPI,
                           public INDI::Telescope,
                           public INDI::GuiderInterface,
                           public INDI::AlignmentSubsystem::AlignmentSubsystemForDrivers
{
  public:
    SkywatcherAPIMount();
    virtual ~SkywatcherAPIMount() = default;

    //  overrides of base class virtual functions
    virtual bool Abort() override;
    virtual bool Handshake() override;
    virtual const char *getDefaultName() override;
    virtual bool Goto(double ra, double dec) override;
    virtual bool initProperties() override;
    virtual void ISGetProperties(const char *dev) override;
    virtual bool ISNewBLOB(const char *dev, const char *name, int sizes[], int blobsizes[], char *blobs[],
                           char *formats[], char *names[], int n) override;
    virtual bool ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n) override;
    virtual bool ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n) override;
    virtual bool ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n) override;
    double GetSlewRate();
    virtual bool MoveNS(INDI_DIR_NS dir, TelescopeMotionCommand command) override;
    virtual bool MoveWE(INDI_DIR_WE dir, TelescopeMotionCommand command) override;
    double GetParkDeltaAz(ParkDirection_t target_direction, ParkPosition_t target_position);
    virtual bool Park() override;
    virtual bool UnPark() override;
    virtual bool ReadScopeStatus() override;
    virtual bool saveConfigItems(FILE *fp) override;
    virtual bool Sync(double ra, double dec) override;
    virtual void TimerHit() override;
    virtual bool updateLocation(double latitude, double longitude, double elevation) override;
    virtual bool updateProperties() override;
    virtual IPState GuideNorth(float ms) override;
    virtual IPState GuideSouth(float ms) override;
    virtual IPState GuideEast(float ms) override;
    virtual IPState GuideWest(float ms) override;

private:
    void CalculateGuidePulses();
    void ResetGuidePulses();
    void ConvertGuideCorrection(double delta_ra, double delta_dec, double &delta_alt, double &delta_az);
    void UpdateScopeConfigSwitch();
    // Overrides for the pure virtual functions in SkyWatcherAPI
    virtual int skywatcher_tty_read(int fd, char *buf, int nbytes, int timeout, int *nbytes_read) override;
    virtual int skywatcher_tty_write(int fd, const char *buffer, int nbytes, int *nbytes_written) override;

    void SkywatcherMicrostepsFromTelescopeDirectionVector(
        const INDI::AlignmentSubsystem::TelescopeDirectionVector TelescopeDirectionVector, long &Axis1Microsteps,
        long &Axis2Microsteps);
    const INDI::AlignmentSubsystem::TelescopeDirectionVector
    TelescopeDirectionVectorFromSkywatcherMicrosteps(long Axis1Microsteps, long Axis2Microsteps);

    void UpdateDetailedMountInformation(bool InformClient);

    // Properties

    static constexpr const char *DetailedMountInfoPage { "Detailed Mount Information" };
    enum
    {
        MOTOR_CONTROL_FIRMWARE_VERSION,
        MOUNT_CODE,
        MOUNT_NAME,
        IS_DC_MOTOR
    };
    IText BasicMountInfo[4];
    ITextVectorProperty BasicMountInfoV;

    enum
    {
        MICROSTEPS_PER_REVOLUTION,
        STEPPER_CLOCK_FREQUENCY,
        HIGH_SPEED_RATIO,
        MICROSTEPS_PER_WORM_REVOLUTION
    };
    INumber AxisOneInfo[4];
    INumberVectorProperty AxisOneInfoV;
    INumber AxisTwoInfo[4];
    INumberVectorProperty AxisTwoInfoV;
    enum
    {
        FULL_STOP,
        SLEWING,
        SLEWING_TO,
        SLEWING_FORWARD,
        HIGH_SPEED,
        NOT_INITIALISED
    };
    ISwitch AxisOneState[6];
    ISwitchVectorProperty AxisOneStateV;
    ISwitch AxisTwoState[6];
    ISwitchVectorProperty AxisTwoStateV;
    enum
    {
        RAW_MICROSTEPS,
        MICROSTEPS_PER_ARCSEC,
        OFFSET_FROM_INITIAL,
        DEGREES_FROM_INITIAL
    };
    INumber AxisOneEncoderValues[4];
    INumberVectorProperty AxisOneEncoderValuesV;
    INumber AxisTwoEncoderValues[4];
    INumberVectorProperty AxisTwoEncoderValuesV;

    // A switch for silent/highspeed slewing modes
    enum
    {
        SLEW_SILENT,
        SLEW_NORMAL
    };
    ISwitch SlewModes[2];
    ISwitchVectorProperty SlewModesSP;

    // A switch for SoftPEC modes
    enum
    {
        SOFTPEC_ENABLED,
        SOFTPEC_DISABLED
    };
    ISwitch SoftPECModes[2];
    ISwitchVectorProperty SoftPECModesSP;

    // SoftPEC value for tracking mode
    INumber SoftPecN;
    INumberVectorProperty SoftPecNP;

    // Guiding rates (RA/Dec)
    INumber GuidingRatesN[2];
    INumberVectorProperty GuidingRatesNP;

    // A switch for park movement directions (clockwise/counterclockwise)
    ISwitch ParkMovementDirection[2];
    ISwitchVectorProperty ParkMovementDirectionSP;

    // A switch for park positions
    ISwitch ParkPosition[4];
    ISwitchVectorProperty ParkPositionSP;

    // A switch for unpark positions
    ISwitch UnparkPosition[4];
    ISwitchVectorProperty UnparkPositionSP;

    // Tracking
    ln_equ_posn CurrentTrackingTarget { 0, 0 };
    long OldTrackingTarget[2] { 0, 0 };
    struct ln_hrz_posn CurrentAltAz { 0, 0 };
    struct ln_hrz_posn TrackedAltAz { 0, 0 };
    bool ResetTrackingSeconds { false };
    int TrackingMsecs { 0 };
    double GuideDeltaAlt { 0 };
    double GuideDeltaAz { 0 };
    int TimeoutDuration { 500 };

    /// Save the serial port name
    std::string SerialPortName;
    /// Recover after disconnection
    bool RecoverAfterReconnection { false };

    GuidingPulse NorthPulse;
    GuidingPulse WestPulse;
    std::vector<GuidingPulse> GuidingPulses;

#ifdef USE_INITIAL_JULIAN_DATE
    double InitialJulianDate { 0 };
#endif
};

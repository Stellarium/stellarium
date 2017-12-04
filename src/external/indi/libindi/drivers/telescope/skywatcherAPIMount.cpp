/*!
 * \file skywatcherAPIMount.cpp
 *
 * \author Roger James
 * \author Gerry Rozema
 * \author Jean-Luc Geehalel
 * \date 13th November 2013
 *
 * This file contains the implementation in C++ of a INDI telescope driver using the Skywatcher API.
 * It is based on work from three sources.
 * A C++ implementation of the API by Roger James.
 * The indi_eqmod driver by Jean-Luc Geehalel.
 * The synscanmount driver by Gerry Rozema.
 */

#include "skywatcherAPIMount.h"

#include "indicom.h"
#include "alignment/DriverCommon.h"
#include "connectionplugins/connectionserial.h"

#include <chrono>
#include <thread>

using namespace INDI::AlignmentSubsystem;

// We declare an auto pointer to SkywatcherAPIMount.
std::unique_ptr<SkywatcherAPIMount> SkywatcherAPIMountPtr(new SkywatcherAPIMount());

/* Preset Slew Speeds */
#define SLEWMODES 9
double SlewSpeeds[SLEWMODES] = { 1.0, 2.0, 4.0, 8.0, 16.0, 32.0, 64.0, 128.0, 600.0 };

void ISPoll(void *p);

namespace
{
bool FileExists(const std::string &name)
{
    std::ifstream File(name.c_str());

    return File.good();
}
} // namespace

void ISGetProperties(const char *dev)
{
    SkywatcherAPIMountPtr->ISGetProperties(dev);
}

void ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    SkywatcherAPIMountPtr->ISNewSwitch(dev, name, states, names, n);
}

void ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n)
{
    SkywatcherAPIMountPtr->ISNewText(dev, name, texts, names, n);
}

void ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    SkywatcherAPIMountPtr->ISNewNumber(dev, name, values, names, n);
}

void ISNewBLOB(const char *dev, const char *name, int sizes[], int blobsizes[], char *blobs[], char *formats[],
               char *names[], int n)
{
    SkywatcherAPIMountPtr->ISNewBLOB(dev, name, sizes, blobsizes, blobs, formats, names, n);
}

void ISSnoopDevice(XMLEle *root)
{
    INDI_UNUSED(root);
}

SkywatcherAPIMount::SkywatcherAPIMount()
{
    // Set up the logging pointer in SkyWatcherAPI
    pChildTelescope  = this;
#ifdef USE_INITIAL_JULIAN_DATE
    InitialJulianDate = ln_get_julian_from_sys();
#endif

    SetTelescopeCapability(TELESCOPE_CAN_PARK | TELESCOPE_CAN_SYNC | TELESCOPE_CAN_GOTO | TELESCOPE_CAN_ABORT |
                               TELESCOPE_HAS_TIME | TELESCOPE_HAS_LOCATION,
                           SLEWMODES);
}

bool SkywatcherAPIMount::Abort()
{
    DEBUG(DBG_SCOPE, "SkywatcherAPIMount::Abort");
    SlowStop(AXIS1);
    SlowStop(AXIS2);
    TrackState = SCOPE_IDLE;

    if (GuideNSNP.s == IPS_BUSY || GuideWENP.s == IPS_BUSY)
    {
        GuideNSNP.s = GuideWENP.s = IPS_IDLE;
        GuideNSN[0].value = GuideNSN[1].value = 0.0;
        GuideWEN[0].value = GuideWEN[1].value = 0.0;

        IDMessage(getDeviceName(), "Guide aborted.");
        IDSetNumber(&GuideNSNP, nullptr);
        IDSetNumber(&GuideWENP, nullptr);

        return true;
    }

    return true;
}

bool SkywatcherAPIMount::Handshake()
{
    DEBUG(DBG_SCOPE, "SkywatcherAPIMount::Handshake");
    SetSerialPort(PortFD);

    bool Result = InitMount(RecoverAfterReconnection);

    if (getActiveConnection() == serialConnection)
    {
        SerialPortName = serialConnection->port();
    }
    else
    {
        SerialPortName = "";
    }

    // The default slew mode is silent on Virtuoso mounts.
    if (Result && !RecoverAfterReconnection && IsVirtuosoMount() &&
        IUFindSwitch(&SlewModesSP, "SLEW_SILENT") != nullptr &&
        IUFindSwitch(&SlewModesSP, "SLEW_NORMAL") != nullptr)
    {
        IUFindSwitch(&SlewModesSP, "SLEW_SILENT")->s = ISS_ON;
        IUFindSwitch(&SlewModesSP, "SLEW_NORMAL")->s = ISS_OFF;
    }
    // The SoftPEC is enabled on Virtuoso mounts by default.
    if (Result && !RecoverAfterReconnection && IsVirtuosoMount() &&
        IUFindSwitch(&SoftPECModesSP, "SOFTPEC_ENABLED") != nullptr &&
        IUFindSwitch(&SoftPECModesSP, "SOFTPEC_DISABLED") != nullptr)
    {
        IUFindSwitch(&SoftPECModesSP, "SOFTPEC_ENABLED")->s  = ISS_ON;
        IUFindSwitch(&SoftPECModesSP, "SOFTPEC_DISABLED")->s = ISS_OFF;
    }
    // The default position is parking on Virtuoso mounts (the telescope is oriented to polar).
    if (Result && !RecoverAfterReconnection && IsVirtuosoMount())
    {
        SetParked(true);
    }
    // The default mode is Slew out of Track/Slew/Sync
    if (!RecoverAfterReconnection && IUFindSwitch(&CoordSP, "TRACK") != nullptr &&
        IUFindSwitch(&CoordSP, "SLEW") != nullptr && IUFindSwitch(&CoordSP, "SYNC") != nullptr)
    {
        IUFindSwitch(&CoordSP, "TRACK")->s = ISS_OFF;
        IUFindSwitch(&CoordSP, "SLEW")->s  = ISS_ON;
        IUFindSwitch(&CoordSP, "SYNC")->s  = ISS_OFF;
    }
    RecoverAfterReconnection = false;
    DEBUGF(DBG_SCOPE, "SkywatcherAPIMount::Handshake - Result: %d", Result);
    return Result;
}

const char *SkywatcherAPIMount::getDefaultName()
{
    //DEBUG(DBG_SCOPE, "SkywatcherAPIMount::getDefaultName\n");
    return "Skywatcher Alt-Az Mount";
}

bool SkywatcherAPIMount::Goto(double ra, double dec)
{
    DEBUG(INDI::AlignmentSubsystem::DBG_ALIGNMENT, "SkywatcherAPIMount::Goto");

    if (TrackState != SCOPE_IDLE)
        Abort();

    DEBUGF(INDI::AlignmentSubsystem::DBG_ALIGNMENT, "RA %lf DEC %lf", ra, dec);

    if (IUFindSwitch(&CoordSP, "TRACK")->s == ISS_ON || IUFindSwitch(&CoordSP, "SLEW")->s == ISS_ON)
    {
        char RAStr[32], DecStr[32];
        fs_sexa(RAStr, ra, 2, 3600);
        fs_sexa(DecStr, dec, 2, 3600);
        CurrentTrackingTarget.ra  = ra;
        CurrentTrackingTarget.dec = dec;
        DEBUGF(INDI::Logger::DBG_SESSION, "New Tracking target RA %s DEC %s", RAStr, DecStr);
    }

    ln_hrz_posn AltAz { 0, 0 };
    TelescopeDirectionVector TDV;

    if (TransformCelestialToTelescope(ra, dec, 0.0, TDV))
    {
        DEBUGF(INDI::AlignmentSubsystem::DBG_ALIGNMENT, "TDV x %lf y %lf z %lf", TDV.x, TDV.y, TDV.z);
        AltitudeAzimuthFromTelescopeDirectionVector(TDV, AltAz);
        DEBUG(INDI::AlignmentSubsystem::DBG_ALIGNMENT, "Conversion OK");
    }
    else
    {
        // Try a conversion with the stored observatory position if any
        bool HavePosition = false;
        ln_lnlat_posn Position { 0, 0 };

        if ((nullptr != IUFindNumber(&LocationNP, "LAT")) && (0 != IUFindNumber(&LocationNP, "LAT")->value) &&
            (nullptr != IUFindNumber(&LocationNP, "LONG")) && (0 != IUFindNumber(&LocationNP, "LONG")->value))
        {
            // I assume that being on the equator and exactly on the prime meridian is unlikely
            Position.lat = IUFindNumber(&LocationNP, "LAT")->value;
            Position.lng = IUFindNumber(&LocationNP, "LONG")->value;
            HavePosition = true;
        }
        ln_equ_posn EquatorialCoordinates { 0, 0 };

        // libnova works in decimal degrees
        EquatorialCoordinates.ra  = ra * 360.0 / 24.0;
        EquatorialCoordinates.dec = dec;
        if (HavePosition)
        {
#ifdef USE_INITIAL_JULIAN_DATE
            ln_get_hrz_from_equ(&EquatorialCoordinates, &Position, InitialJulianDate, &AltAz);
#else
            ln_get_hrz_from_equ(&EquatorialCoordinates, &Position, ln_get_julian_from_sys(), &AltAz);
#endif
            TDV = TelescopeDirectionVectorFromAltitudeAzimuth(AltAz);
            switch (GetApproximateMountAlignment())
            {
                case ZENITH:
                    break;

                case NORTH_CELESTIAL_POLE:
                    // Rotate the TDV coordinate system clockwise (negative) around the y axis by 90 minus
                    // the (positive)observatory latitude. The vector itself is rotated anticlockwise
                    TDV.RotateAroundY(Position.lat - 90.0);
                    break;

                case SOUTH_CELESTIAL_POLE:
                    // Rotate the TDV coordinate system anticlockwise (positive) around the y axis by 90 plus
                    // the (negative)observatory latitude. The vector itself is rotated clockwise
                    TDV.RotateAroundY(Position.lat + 90.0);
                    break;
            }
            AltitudeAzimuthFromTelescopeDirectionVector(TDV, AltAz);
        }
        else
        {
            // The best I can do is just do a direct conversion to Alt/Az
            TDV = TelescopeDirectionVectorFromEquatorialCoordinates(EquatorialCoordinates);
            AltitudeAzimuthFromTelescopeDirectionVector(TDV, AltAz);
        }
        DEBUGF(INDI::AlignmentSubsystem::DBG_ALIGNMENT, "Conversion Failed - HavePosition %d", HavePosition);
    }
    if (IsVirtuosoMount())
    {
        // The initial position of the Virtuoso mount is polar aligned when switched on.
        // The altitude is corrected by the latitude.
        if (IUFindNumber(&LocationNP, "LAT") != nullptr)
            AltAz.alt = AltAz.alt - IUFindNumber(&LocationNP, "LAT")->value;

        AltAz.az = 180 + AltAz.az;
    }

    DEBUGF(INDI::AlignmentSubsystem::DBG_ALIGNMENT,
           "New Altitude %lf degrees %ld microsteps Azimuth %lf degrees %ld microsteps", AltAz.alt,
           DegreesToMicrosteps(AXIS2, AltAz.alt), AltAz.az, DegreesToMicrosteps(AXIS1, AltAz.az));

    // Update the current encoder positions
    GetEncoder(AXIS1);
    GetEncoder(AXIS2);

    long AltitudeOffsetMicrosteps =
        DegreesToMicrosteps(AXIS2, AltAz.alt) + ZeroPositionEncoders[AXIS2] - CurrentEncoders[AXIS2];
    long AzimuthOffsetMicrosteps =
        DegreesToMicrosteps(AXIS1, AltAz.az) + ZeroPositionEncoders[AXIS1] - CurrentEncoders[AXIS1];

    // Do I need to take out any complete revolutions before I do this test?
    if (AltitudeOffsetMicrosteps > MicrostepsPerRevolution[AXIS2] / 2)
    {
        // Going the long way round - send it the other way
        AltitudeOffsetMicrosteps -= MicrostepsPerRevolution[AXIS2];
    }

    if (AzimuthOffsetMicrosteps > MicrostepsPerRevolution[AXIS1] / 2)
    {
        // Going the long way round - send it the other way
        AzimuthOffsetMicrosteps -= MicrostepsPerRevolution[AXIS1];
    }
    DEBUGF(INDI::AlignmentSubsystem::DBG_ALIGNMENT, "Initial Axis2 %ld microsteps Axis1 %ld microsteps",
           ZeroPositionEncoders[AXIS2], ZeroPositionEncoders[AXIS1]);
    DEBUGF(INDI::AlignmentSubsystem::DBG_ALIGNMENT, "Current Axis2 %ld microsteps Axis1 %ld microsteps",
           CurrentEncoders[AXIS2], CurrentEncoders[AXIS1]);
    DEBUGF(INDI::AlignmentSubsystem::DBG_ALIGNMENT, "Altitude offset %ld microsteps Azimuth offset %ld microsteps",
           AltitudeOffsetMicrosteps, AzimuthOffsetMicrosteps);

    if (IUFindSwitch(&SlewModesSP, "SLEW_SILENT") != nullptr && IUFindSwitch(&SlewModesSP, "SLEW_SILENT")->s == ISS_ON)
    {
        SilentSlewMode = true;
    }
    else
    {
        SilentSlewMode = false;
    }
    SlewTo(AXIS1, AzimuthOffsetMicrosteps);
    SlewTo(AXIS2, AltitudeOffsetMicrosteps);

    TrackState = SCOPE_SLEWING;

    EqNP.s = IPS_BUSY;

    return true;
}

bool SkywatcherAPIMount::initProperties()
{
    IDLog("SkywatcherAPIMount::initProperties\n");

    // Allow the base class to initialise its visible before connection properties
    INDI::Telescope::initProperties();

    for (int i = 0; i < SlewRateSP.nsp; ++i)
    {
        sprintf(SlewRateSP.sp[i].label, "%.fx", SlewSpeeds[i]);
        SlewRateSP.sp[i].aux = (void *)&SlewSpeeds[i];
    }
    strncpy(SlewRateSP.sp[SlewRateSP.nsp - 1].name, "SLEW_MAX", MAXINDINAME);

    // Add default properties
    addDebugControl();
    addConfigurationControl();

    // Add alignment properties
    InitAlignmentProperties(this);

    // Force the alignment system to always be on
    getSwitch("ALIGNMENT_SUBSYSTEM_ACTIVE")->sp[0].s = ISS_ON;

    // Set up property variables
    IUFillText(&BasicMountInfo[MOTOR_CONTROL_FIRMWARE_VERSION], "MOTOR_CONTROL_FIRMWARE_VERSION",
               "Motor control firmware version", "-");
    IUFillText(&BasicMountInfo[MOUNT_CODE], "MOUNT_CODE", "Mount code", "-");
    IUFillText(&BasicMountInfo[MOUNT_NAME], "MOUNT_NAME", "Mount name", "-");
    IUFillText(&BasicMountInfo[IS_DC_MOTOR], "IS_DC_MOTOR", "Is DC motor", "-");
    IUFillTextVector(&BasicMountInfoV, BasicMountInfo, 4, getDeviceName(), "BASIC_MOUNT_INFO",
                     "Basic mount information", DetailedMountInfoPage, IP_RO, 60, IPS_IDLE);

    IUFillNumber(&AxisOneInfo[MICROSTEPS_PER_REVOLUTION], "MICROSTEPS_PER_REVOLUTION", "Microsteps per revolution",
                 "%.0f", 0, 0xFFFFFF, 1, 0);
    IUFillNumber(&AxisOneInfo[STEPPER_CLOCK_FREQUENCY], "STEPPER_CLOCK_FREQUENCY", "Stepper clock frequency", "%.0f", 0,
                 0xFFFFFF, 1, 0);
    IUFillNumber(&AxisOneInfo[HIGH_SPEED_RATIO], "HIGH_SPEED_RATIO", "High speed ratio", "%.0f", 0, 0xFFFFFF, 1, 0);
    IUFillNumber(&AxisOneInfo[MICROSTEPS_PER_WORM_REVOLUTION], "MICROSTEPS_PER_WORM_REVOLUTION",
                 "Microsteps per worm revolution", "%.0f", 0, 0xFFFFFF, 1, 0);

    IUFillNumberVector(&AxisOneInfoV, AxisOneInfo, 4, getDeviceName(), "AXIS_ONE_INFO", "Axis one information",
                       DetailedMountInfoPage, IP_RO, 60, IPS_IDLE);

    IUFillSwitch(&AxisOneState[FULL_STOP], "FULL_STOP", "FULL_STOP", ISS_OFF);
    IUFillSwitch(&AxisOneState[SLEWING], "SLEWING", "SLEWING", ISS_OFF);
    IUFillSwitch(&AxisOneState[SLEWING_TO], "SLEWING_TO", "SLEWING_TO", ISS_OFF);
    IUFillSwitch(&AxisOneState[SLEWING_FORWARD], "SLEWING_FORWARD", "SLEWING_FORWARD", ISS_OFF);
    IUFillSwitch(&AxisOneState[HIGH_SPEED], "HIGH_SPEED", "HIGH_SPEED", ISS_OFF);
    IUFillSwitch(&AxisOneState[NOT_INITIALISED], "NOT_INITIALISED", "NOT_INITIALISED", ISS_ON);
    IUFillSwitchVector(&AxisOneStateV, AxisOneState, 6, getDeviceName(), "AXIS_ONE_STATE", "Axis one state",
                       DetailedMountInfoPage, IP_RO, ISR_NOFMANY, 60, IPS_IDLE);

    IUFillNumber(&AxisTwoInfo[MICROSTEPS_PER_REVOLUTION], "MICROSTEPS_PER_REVOLUTION", "Microsteps per revolution",
                 "%.0f", 0, 0xFFFFFF, 1, 0);
    IUFillNumber(&AxisTwoInfo[STEPPER_CLOCK_FREQUENCY], "STEPPER_CLOCK_FREQUENCY", "Step timer frequency", "%.0f", 0,
                 0xFFFFFF, 1, 0);
    IUFillNumber(&AxisTwoInfo[HIGH_SPEED_RATIO], "HIGH_SPEED_RATIO", "High speed ratio", "%.0f", 0, 0xFFFFFF, 1, 0);
    IUFillNumber(&AxisTwoInfo[MICROSTEPS_PER_WORM_REVOLUTION], "MICROSTEPS_PER_WORM_REVOLUTION",
                 "Mictosteps per worm revolution", "%.0f", 0, 0xFFFFFF, 1, 0);

    IUFillNumberVector(&AxisTwoInfoV, AxisTwoInfo, 4, getDeviceName(), "AXIS_TWO_INFO", "Axis two information",
                       DetailedMountInfoPage, IP_RO, 60, IPS_IDLE);

    IUFillSwitch(&AxisTwoState[FULL_STOP], "FULL_STOP", "FULL_STOP", ISS_OFF);
    IUFillSwitch(&AxisTwoState[SLEWING], "SLEWING", "SLEWING", ISS_OFF);
    IUFillSwitch(&AxisTwoState[SLEWING_TO], "SLEWING_TO", "SLEWING_TO", ISS_OFF);
    IUFillSwitch(&AxisTwoState[SLEWING_FORWARD], "SLEWING_FORWARD", "SLEWING_FORWARD", ISS_OFF);
    IUFillSwitch(&AxisTwoState[HIGH_SPEED], "HIGH_SPEED", "HIGH_SPEED", ISS_OFF);
    IUFillSwitch(&AxisTwoState[NOT_INITIALISED], "NOT_INITIALISED", "NOT_INITIALISED", ISS_ON);
    IUFillSwitchVector(&AxisTwoStateV, AxisTwoState, 6, getDeviceName(), "AXIS_TWO_STATE", "Axis two state",
                       DetailedMountInfoPage, IP_RO, ISR_NOFMANY, 60, IPS_IDLE);

    IUFillNumber(&AxisOneEncoderValues[RAW_MICROSTEPS], "RAW_MICROSTEPS", "Raw Microsteps", "%.0f", 0, 0xFFFFFF, 1, 0);
    IUFillNumber(&AxisOneEncoderValues[MICROSTEPS_PER_ARCSEC], "MICROSTEPS_PER_ARCSEC", "Microsteps/arcsecond",
                 "%.4f", 0, 0xFFFFFF, 1, 0);
    IUFillNumber(&AxisOneEncoderValues[OFFSET_FROM_INITIAL], "OFFSET_FROM_INITIAL", "Offset from initial", "%.0f", 0,
                 0xFFFFFF, 1, 0);
    IUFillNumber(&AxisOneEncoderValues[DEGREES_FROM_INITIAL], "DEGREES_FROM_INITIAL", "Degrees from initial", "%.2f",
                 -1000.0, 1000.0, 1, 0);

    IUFillNumberVector(&AxisOneEncoderValuesV, AxisOneEncoderValues, 4, getDeviceName(), "AXIS1_ENCODER_VALUES",
                       "Axis 1 Encoder values", DetailedMountInfoPage, IP_RO, 60, IPS_IDLE);

    IUFillNumber(&AxisTwoEncoderValues[RAW_MICROSTEPS], "RAW_MICROSTEPS", "Raw Microsteps", "%.0f", 0, 0xFFFFFF, 1, 0);
    IUFillNumber(&AxisTwoEncoderValues[MICROSTEPS_PER_ARCSEC], "MICROSTEPS_PER_ARCSEC", "Microsteps/arcsecond",
                 "%.4f", 0, 0xFFFFFF, 1, 0);
    IUFillNumber(&AxisTwoEncoderValues[OFFSET_FROM_INITIAL], "OFFSET_FROM_INITIAL", "Offset from initial", "%.0f", 0,
                 0xFFFFFF, 1, 0);
    IUFillNumber(&AxisTwoEncoderValues[DEGREES_FROM_INITIAL], "DEGREES_FROM_INITIAL", "Degrees from initial", "%.2f",
                 -1000.0, 1000.0, 1, 0);

    IUFillNumberVector(&AxisTwoEncoderValuesV, AxisTwoEncoderValues, 4, getDeviceName(), "AXIS2_ENCODER_VALUES",
                       "Axis 2 Encoder values", DetailedMountInfoPage, IP_RO, 60, IPS_IDLE);
    // Register any visible before connection properties

    // Slew modes
    IUFillSwitch(&SlewModes[SLEW_SILENT], "SLEW_SILENT", "Silent", ISS_OFF);
    IUFillSwitch(&SlewModes[SLEW_NORMAL], "SLEW_NORMAL", "Normal", ISS_ON);
    IUFillSwitchVector(&SlewModesSP, SlewModes, 2, getDeviceName(), "TELESCOPE_MOTION_SLEWMODE", "Slew Mode",
                       MOTION_TAB, IP_RW, ISR_ATMOST1, 60, IPS_IDLE);

    // SoftPEC modes
    IUFillSwitch(&SoftPECModes[SOFTPEC_ENABLED], "SOFTPEC_ENABLED", "Enable for tracking", ISS_OFF);
    IUFillSwitch(&SoftPECModes[SOFTPEC_DISABLED], "SOFTPEC_DISABLED", "Disabled", ISS_ON);
    IUFillSwitchVector(&SoftPECModesSP, SoftPECModes, 2, getDeviceName(), "TELESCOPE_MOTION_SOFTPECMODE",
                       "SoftPEC Mode", MOTION_TAB, IP_RW, ISR_ATMOST1, 60, IPS_IDLE);

    // SoftPEC value for tracking mode
    IUFillNumber(&SoftPecN, "SOFTPEC_VALUE", "degree/minute (Alt)", "%1.3f", 0.001, 1.0, 0.001, 0.009);
    IUFillNumberVector(&SoftPecNP, &SoftPecN, 1, getDeviceName(), "SOFTPEC", "SoftPEC Value", MOTION_TAB, IP_RW, 60,
                       IPS_IDLE);

    // Guiding rates for RA/DEC axes
    IUFillNumber(&GuidingRatesN[0], "GUIDERA_RATE", "arcsec/seconds (RA)", "%1.3f", 1.0, 6000.0, 1.0, 120.0);
    IUFillNumber(&GuidingRatesN[1], "GUIDEDEC_RATE", "arcsec/seconds (Dec)", "%1.3f", 1.0, 6000.0, 1.0, 120.0);
    IUFillNumberVector(&GuidingRatesNP, GuidingRatesN, 2, getDeviceName(), "GUIDE_RATES", "Guide Rates", MOTION_TAB,
                       IP_RW, 60, IPS_IDLE);

    // Park movement directions
    IUFillSwitch(&ParkMovementDirection[PARK_COUNTERCLOCKWISE], "PMD_COUNTERCLOCKWISE", "Counterclockwise", ISS_ON);
    IUFillSwitch(&ParkMovementDirection[PARK_CLOCKWISE], "PMD_CLOCKWISE", "Clockwise", ISS_OFF);
    IUFillSwitchVector(&ParkMovementDirectionSP, ParkMovementDirection, 2, getDeviceName(), "PARK_DIRECTION",
                       "Park Direction", MOTION_TAB, IP_RW, ISR_ATMOST1, 60, IPS_IDLE);

    // Park positions
    IUFillSwitch(&ParkPosition[PARK_NORTH], "PARK_NORTH", "North", ISS_ON);
    IUFillSwitch(&ParkPosition[PARK_EAST], "PARK_EAST", "East", ISS_OFF);
    IUFillSwitch(&ParkPosition[PARK_SOUTH], "PARK_SOUTH", "South", ISS_OFF);
    IUFillSwitch(&ParkPosition[PARK_WEST], "PARK_WEST", "West", ISS_OFF);
    IUFillSwitchVector(&ParkPositionSP, ParkPosition, 4, getDeviceName(), "PARK_POSITION", "Park Position", MOTION_TAB,
                       IP_RW, ISR_ATMOST1, 60, IPS_IDLE);

    // Unpark positions
    IUFillSwitch(&UnparkPosition[PARK_NORTH], "UNPARK_NORTH", "North", ISS_OFF);
    IUFillSwitch(&UnparkPosition[PARK_EAST], "UNPARK_EAST", "East", ISS_OFF);
    IUFillSwitch(&UnparkPosition[PARK_SOUTH], "UNPARK_SOUTH", "South", ISS_ON);
    IUFillSwitch(&UnparkPosition[PARK_WEST], "UNPARK_WEST", "West", ISS_OFF);
    IUFillSwitchVector(&UnparkPositionSP, UnparkPosition, 4, getDeviceName(), "UNPARK_POSITION", "Unpark Position",
                       MOTION_TAB, IP_RW, ISR_ATMOST1, 60, IPS_IDLE);

    // Guiding support
    // TODO: Hide the auto-guide support now because it is not production-ready
//    initGuiderProperties(getDeviceName(), GUIDE_TAB);

//    setDriverInterface(getDriverInterface() | GUIDER_INTERFACE);

    return true;
}

void SkywatcherAPIMount::ISGetProperties(const char *dev)
{
    IDLog("SkywatcherAPIMount::ISGetProperties\n");
    INDI::Telescope::ISGetProperties(dev);

    if (isConnected())
    {
        // Fill in any real values now available MCInit should have been called already
        UpdateDetailedMountInformation(false);

        // Define our connected only properties to the base driver
        // e.g. defineNumber(MyNumberVectorPointer);
        // This will register our properties and send a IDDefXXXX mewssage to any connected clients
        defineText(&BasicMountInfoV);
        defineNumber(&AxisOneInfoV);
        defineSwitch(&AxisOneStateV);
        defineNumber(&AxisTwoInfoV);
        defineSwitch(&AxisTwoStateV);
        defineNumber(&AxisOneEncoderValuesV);
        defineNumber(&AxisTwoEncoderValuesV);
        defineSwitch(&SlewModesSP);
        defineSwitch(&SoftPECModesSP);
        defineNumber(&SoftPecNP);
        defineNumber(&GuidingRatesNP);
        defineSwitch(&ParkMovementDirectionSP);
        defineSwitch(&ParkPositionSP);
        defineSwitch(&UnparkPositionSP);
        defineNumber(&GuideNSNP);
        defineNumber(&GuideWENP);
    }
}

bool SkywatcherAPIMount::ISNewBLOB(const char *dev, const char *name, int sizes[], int blobsizes[], char *blobs[],
                                   char *formats[], char *names[], int n)
{
    if (dev != nullptr && strcmp(dev, getDeviceName()) == 0)
    {
        // It is for us
        ProcessAlignmentBLOBProperties(this, name, sizes, blobsizes, blobs, formats, names, n);
    }
    // Pass it up the chain
    return INDI::Telescope::ISNewBLOB(dev, name, sizes, blobsizes, blobs, formats, names, n);
}

bool SkywatcherAPIMount::ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    if (dev != nullptr && strcmp(dev, getDeviceName()) == 0)
    {
        ProcessAlignmentNumberProperties(this, name, values, names, n);

        if (strcmp(name, "SOFTPEC") == 0)
        {
            SoftPecNP.s = IPS_OK;
            IUUpdateNumber(&SoftPecNP, values, names, n);
            IDSetNumber(&SoftPecNP, nullptr);
            return true;
        }

        if (strcmp(name, "GUIDE_RATES") == 0)
        {
            ResetGuidePulses();
            GuidingRatesNP.s = IPS_OK;
            IUUpdateNumber(&GuidingRatesNP, values, names, n);
            IDSetNumber(&GuidingRatesNP, nullptr);
            return true;
        }

        // Let our driver do sync operation in park position
        if (strcmp(name, "EQUATORIAL_EOD_COORD") == 0)
        {
            double ra  = -1;
            double dec = -100;

            for (int x = 0; x < n; x++)
            {
                INumber *eqp = IUFindNumber(&EqNP, names[x]);
                if (eqp == &EqN[AXIS_RA])
                {
                    ra = values[x];
                }
                else if (eqp == &EqN[AXIS_DE])
                {
                    dec = values[x];
                }
            }
            if ((ra >= 0) && (ra <= 24) && (dec >= -90) && (dec <= 90))
            {
                ISwitch *sw = IUFindSwitch(&CoordSP, "SYNC");

                if (sw != nullptr && sw->s == ISS_ON && isParked())
                {
                    return Sync(ra, dec);
                }
            }
        }

        processGuiderProperties(name, values, names, n);
    }
    // Pass it up the chain
    return INDI::Telescope::ISNewNumber(dev, name, values, names, n);
}

bool SkywatcherAPIMount::ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    IUUpdateSwitch(getSwitch(name), states, names, n);
    if (dev != nullptr && strcmp(dev, getDeviceName()) == 0)
    {
        // It is for us
        ProcessAlignmentSwitchProperties(this, name, states, names, n);
    }
    // Pass it up the chain
    return INDI::Telescope::ISNewSwitch(dev, name, states, names, n);
}

bool SkywatcherAPIMount::ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n)
{
    if (dev != nullptr && strcmp(dev, getDeviceName()) == 0)
    {
        ProcessAlignmentTextProperties(this, name, texts, names, n);
    }
    // Pass it up the chain
    bool Ret =  INDI::Telescope::ISNewText(dev, name, texts, names, n);

    // The scope config switch must be updated after the config is saved to disk
    if (dev != nullptr && strcmp(dev, getDeviceName()) == 0)
    {
        if (name && std::string(name) == "SCOPE_CONFIG_NAME")
        {
            UpdateScopeConfigSwitch();
        }
    }
    return Ret;
}

void SkywatcherAPIMount::UpdateScopeConfigSwitch()
{
    if (!CheckFile(ScopeConfigFileName, false))
    {
        DEBUGF(INDI::Logger::DBG_SESSION, "Can't open XML file (%s) for read", ScopeConfigFileName.c_str());
        return;
    }
    LilXML *XmlHandle      = newLilXML();
    FILE *FilePtr          = fopen(ScopeConfigFileName.c_str(), "r");
    XMLEle *RootXmlNode    = nullptr;
    XMLEle *CurrentXmlNode = nullptr;
    XMLAtt *Ap             = nullptr;
    bool DeviceFound       = false;
    char ErrMsg[512];

    RootXmlNode = readXMLFile(FilePtr, XmlHandle, ErrMsg);
    delLilXML(XmlHandle);
    XmlHandle = nullptr;
    if (!RootXmlNode)
    {
        DEBUGF(INDI::Logger::DBG_SESSION, "Failed to parse XML file (%s): %s", ScopeConfigFileName.c_str(), ErrMsg);
        return;
    }
    if (std::string(tagXMLEle(RootXmlNode)) != ScopeConfigRootXmlNode)
    {
        DEBUGF(INDI::Logger::DBG_SESSION, "Not a scope config XML file (%s)", ScopeConfigFileName.c_str());
        delXMLEle(RootXmlNode);
        return;
    }
    CurrentXmlNode = nextXMLEle(RootXmlNode, 1);
    // Find the current telescope in the config file
    while (CurrentXmlNode)
    {
        if (std::string(tagXMLEle(CurrentXmlNode)) != ScopeConfigDeviceXmlNode)
        {
            CurrentXmlNode = nextXMLEle(RootXmlNode, 0);
            continue;
        }
        Ap = findXMLAtt(CurrentXmlNode, ScopeConfigNameXmlNode.c_str());
        if (Ap && !strcmp(valuXMLAtt(Ap), getDeviceName()))
        {
            DeviceFound = true;
            break;
        }
        CurrentXmlNode = nextXMLEle(RootXmlNode, 0);
    }
    if (!DeviceFound)
    {
        DEBUGF(INDI::Logger::DBG_SESSION, "No a scope config found for %s in the XML file (%s)", getDeviceName(),
               ScopeConfigFileName.c_str());
        delXMLEle(RootXmlNode);
        return;
    }
    // Read the values
    XMLEle *XmlNode       = nullptr;
    XMLEle *DeviceXmlNode = CurrentXmlNode;
    std::string ConfigName;

    for (int i = 1; i < 7; ++i)
    {
        bool Found = true;

        CurrentXmlNode = findXMLEle(DeviceXmlNode, ("config"+std::to_string(i)).c_str());
        if (CurrentXmlNode)
        {
            XmlNode = findXMLEle(CurrentXmlNode, ScopeConfigLabelApXmlNode.c_str());
            if (XmlNode)
            {
                ConfigName = pcdataXMLEle(XmlNode);
            }
        } else {
            Found = false;
        }
        // Change the switch label
        ISwitch *configSwitch = IUFindSwitch(&ScopeConfigsSP, ("SCOPE_CONFIG"+std::to_string(i)).c_str());

        if (configSwitch != nullptr)
        {
            // The config is not used yet
            if (!Found)
            {
                strncpy(configSwitch->label, ("Config #"+std::to_string(i)+" - Not used").c_str(), MAXINDILABEL);
                continue;
            }
            // Empty switch label
            if (ConfigName.empty())
            {
                strncpy(configSwitch->label, ("Config #"+std::to_string(i)+" - Untitled").c_str(), MAXINDILABEL);
                continue;
            }
            strncpy(configSwitch->label, ("Config #"+std::to_string(i)+" - "+ConfigName).c_str(), MAXINDILABEL);
        }
    }
    delXMLEle(RootXmlNode);
    // Delete the joystick control to get the telescope config switch to the bottom of the page
    deleteProperty("USEJOYSTICK");
    // Recreate the switch control
    deleteProperty(ScopeConfigsSP.name);
    defineSwitch(&ScopeConfigsSP);
}

double SkywatcherAPIMount::GetSlewRate()
{
    ISwitch *Switch = IUFindOnSwitch(&SlewRateSP);
    double Rate     = *((double *)Switch->aux);

    return Rate;
}

bool SkywatcherAPIMount::MoveNS(INDI_DIR_NS dir, TelescopeMotionCommand command)
{
    DEBUG(DBG_SCOPE, "SkywatcherAPIMount::MoveNS");

    double speed =
        (dir == DIRECTION_NORTH) ? GetSlewRate() * LOW_SPEED_MARGIN / 2 : -GetSlewRate() * LOW_SPEED_MARGIN / 2;
    const char *dirStr = (dir == DIRECTION_NORTH) ? "North" : "South";

    switch (command)
    {
        case MOTION_START:
            DEBUGF(DBG_SCOPE, "Starting Slew %s", dirStr);
            // Ignore the silent mode because MoveNS() is called by the manual motion UI controls.
            Slew(AXIS2, speed, true);
            break;

        case MOTION_STOP:
            DEBUGF(DBG_SCOPE, "Stopping Slew %s", dirStr);
            SlowStop(AXIS2);
            break;
    }

    return true;
}

bool SkywatcherAPIMount::MoveWE(INDI_DIR_WE dir, TelescopeMotionCommand command)
{
    DEBUG(DBG_SCOPE, "SkywatcherAPIMount::MoveWE");

    double speed =
        (dir == DIRECTION_WEST) ? GetSlewRate() * LOW_SPEED_MARGIN / 2 : -GetSlewRate() * LOW_SPEED_MARGIN / 2;
    const char *dirStr = (dir == DIRECTION_WEST) ? "West" : "East";

    if (IsVirtuosoMount())
        speed = -speed;

    switch (command)
    {
        case MOTION_START:
            DEBUGF(DBG_SCOPE, "Starting Slew %s", dirStr);
            // Ignore the silent mode because MoveNS() is called by the manual motion UI controls.
            Slew(AXIS1, speed, true);
            break;

        case MOTION_STOP:
            DEBUGF(DBG_SCOPE, "Stopping Slew %s", dirStr);
            SlowStop(AXIS1);
            break;
    }

    return true;
}

double SkywatcherAPIMount::GetParkDeltaAz(ParkDirection_t target_direction, ParkPosition_t target_position)
{
    double Result = 0;

    DEBUGF(INDI::AlignmentSubsystem::DBG_ALIGNMENT, "GetParkDeltaAz: direction %d - position: %d",
           (int)target_direction, (int)target_position);
    // Calculate delta degrees (target: NORTH)
    if (target_position == PARK_NORTH)
    {
        if (target_direction == PARK_COUNTERCLOCKWISE)
        {
            Result = -CurrentAltAz.az;
        }
        else
        {
            Result = 360 - CurrentAltAz.az;
        }
    }
    // Calculate delta degrees (target: EAST)
    if (target_position == PARK_EAST)
    {
        if (target_direction == PARK_COUNTERCLOCKWISE)
        {
            if (CurrentAltAz.az > 0 && CurrentAltAz.az < 90)
                Result = -270 - CurrentAltAz.az;
            else
                Result = -CurrentAltAz.az + 90;
        }
        else
        {
            if (CurrentAltAz.az > 0 && CurrentAltAz.az < 90)
                Result = 90 - CurrentAltAz.az;
            else
                Result = 360 - CurrentAltAz.az + 90;
        }
    }
    // Calculate delta degrees (target: SOUTH)
    if (target_position == PARK_SOUTH)
    {
        if (target_direction == PARK_COUNTERCLOCKWISE)
        {
            if (CurrentAltAz.az > 0 && CurrentAltAz.az < 180)
                Result = -180 - CurrentAltAz.az;
            else
                Result = -CurrentAltAz.az + 180;
        }
        else
        {
            if (CurrentAltAz.az > 0 && CurrentAltAz.az < 180)
                Result = 180 - CurrentAltAz.az;
            else
                Result = 360 - CurrentAltAz.az + 180;
        }
    }
    // Calculate delta degrees (target: WEST)
    if (target_position == PARK_WEST)
    {
        if (target_direction == PARK_COUNTERCLOCKWISE)
        {
            if (CurrentAltAz.az > 0 && CurrentAltAz.az < 270)
                Result = -90 - CurrentAltAz.az;
            else
                Result = -CurrentAltAz.az + 270;
        }
        else
        {
            if (CurrentAltAz.az > 0 && CurrentAltAz.az < 270)
                Result = 270 - CurrentAltAz.az;
            else
                Result = 360 - CurrentAltAz.az + 270;
        }
    }
    if (Result >= 360)
    {
        Result -= 360;
    }
    if (Result <= -360)
    {
        Result += 360;
    }
    return Result;
}

bool SkywatcherAPIMount::Park()
{
    DEBUG(DBG_SCOPE, "SkywatcherAPIMount::Park");
    ParkPosition_t TargetPosition   = PARK_NORTH;
    ParkDirection_t TargetDirection = PARK_COUNTERCLOCKWISE;
    double DeltaAlt                 = 0;
    double DeltaAz                  = 0;

    // Determinate the target position and direction
    if (IUFindSwitch(&ParkPositionSP, "PARK_NORTH") != nullptr &&
        IUFindSwitch(&ParkPositionSP, "PARK_NORTH")->s == ISS_ON)
    {
        TargetPosition = PARK_NORTH;
    }
    if (IUFindSwitch(&ParkPositionSP, "PARK_EAST") != nullptr &&
        IUFindSwitch(&ParkPositionSP, "PARK_EAST")->s == ISS_ON)
    {
        TargetPosition = PARK_EAST;
    }
    if (IUFindSwitch(&ParkPositionSP, "PARK_SOUTH") != nullptr &&
        IUFindSwitch(&ParkPositionSP, "PARK_SOUTH")->s == ISS_ON)
    {
        TargetPosition = PARK_SOUTH;
    }
    if (IUFindSwitch(&ParkPositionSP, "PARK_WEST") != nullptr &&
        IUFindSwitch(&ParkPositionSP, "PARK_WEST")->s == ISS_ON)
    {
        TargetPosition = PARK_WEST;
    }

    if (IUFindSwitch(&ParkMovementDirectionSP, "PMD_COUNTERCLOCKWISE") != nullptr &&
        IUFindSwitch(&ParkMovementDirectionSP, "PMD_COUNTERCLOCKWISE")->s == ISS_ON)
    {
        TargetDirection = PARK_COUNTERCLOCKWISE;
    }
    if (IUFindSwitch(&ParkMovementDirectionSP, "PMD_CLOCKWISE") != nullptr &&
        IUFindSwitch(&ParkMovementDirectionSP, "PMD_CLOCKWISE")->s == ISS_ON)
    {
        TargetDirection = PARK_CLOCKWISE;
    }
    DeltaAz = GetParkDeltaAz(TargetDirection, TargetPosition);
    // Altitude 3440 points the telescope downwards
    DeltaAlt = CurrentAltAz.alt - 3440;

    // Move the telescope to the desired position
    long AltitudeOffsetMicrosteps = DegreesToMicrosteps(AXIS2, DeltaAlt);
    long AzimuthOffsetMicrosteps  = DegreesToMicrosteps(AXIS1, DeltaAz);

    DEBUGF(INDI::AlignmentSubsystem::DBG_ALIGNMENT, "Parking: Delta altitude %1.2f - delta azimuth %1.2f", DeltaAlt,
           DeltaAz);
    DEBUGF(INDI::AlignmentSubsystem::DBG_ALIGNMENT,
           "Parking: Altitude offset %ld microsteps Azimuth offset %ld microsteps", AltitudeOffsetMicrosteps,
           AzimuthOffsetMicrosteps);

    if (IUFindSwitch(&SlewModesSP, "SLEW_SILENT") != nullptr && IUFindSwitch(&SlewModesSP, "SLEW_SILENT")->s == ISS_ON)
    {
        SilentSlewMode = true;
    }
    else
    {
        SilentSlewMode = false;
    }
    SlewTo(AXIS1, AzimuthOffsetMicrosteps);
    SlewTo(AXIS2, AltitudeOffsetMicrosteps);

    TrackState = SCOPE_PARKING;
    return true;
}

bool SkywatcherAPIMount::UnPark()
{
    DEBUG(DBG_SCOPE, "SkywatcherAPIMount::UnPark");

    ParkPosition_t TargetPosition   = PARK_NORTH;
    ParkDirection_t TargetDirection = PARK_COUNTERCLOCKWISE;
    double DeltaAlt                 = 0;
    double DeltaAz                  = 0;

    // Determinate the target position and direction
    if (IUFindSwitch(&UnparkPositionSP, "UNPARK_NORTH") != nullptr &&
        IUFindSwitch(&UnparkPositionSP, "UNPARK_NORTH")->s == ISS_ON)
    {
        TargetPosition = PARK_NORTH;
    }
    if (IUFindSwitch(&UnparkPositionSP, "UNPARK_EAST") != nullptr &&
        IUFindSwitch(&UnparkPositionSP, "UNPARK_EAST")->s == ISS_ON)
    {
        TargetPosition = PARK_EAST;
    }
    if (IUFindSwitch(&UnparkPositionSP, "UNPARK_SOUTH") != nullptr &&
        IUFindSwitch(&UnparkPositionSP, "UNPARK_SOUTH")->s == ISS_ON)
    {
        TargetPosition = PARK_SOUTH;
    }
    if (IUFindSwitch(&UnparkPositionSP, "UNPARK_WEST") != nullptr &&
        IUFindSwitch(&UnparkPositionSP, "UNPARK_WEST")->s == ISS_ON)
    {
        TargetPosition = PARK_WEST;
    }

    // Note: The reverse direction is used for unparking.
    if (IUFindSwitch(&ParkMovementDirectionSP, "PMD_COUNTERCLOCKWISE") != nullptr &&
        IUFindSwitch(&ParkMovementDirectionSP, "PMD_COUNTERCLOCKWISE")->s == ISS_ON)
    {
        TargetDirection = PARK_CLOCKWISE;
    }
    if (IUFindSwitch(&ParkMovementDirectionSP, "PMD_CLOCKWISE") != nullptr &&
        IUFindSwitch(&ParkMovementDirectionSP, "PMD_CLOCKWISE")->s == ISS_ON)
    {
        TargetDirection = PARK_COUNTERCLOCKWISE;
    }
    DeltaAz = GetParkDeltaAz(TargetDirection, TargetPosition);
    // Altitude 3360 points the telescope upwards
    DeltaAlt = CurrentAltAz.alt - 3360;

    // Move the telescope to the desired position
    long AltitudeOffsetMicrosteps = DegreesToMicrosteps(AXIS2, DeltaAlt);
    long AzimuthOffsetMicrosteps  = DegreesToMicrosteps(AXIS1, DeltaAz);

    DEBUGF(INDI::AlignmentSubsystem::DBG_ALIGNMENT, "Unparking: Delta altitude %1.2f - delta azimuth %1.2f", DeltaAlt,
           DeltaAz);
    DEBUGF(INDI::AlignmentSubsystem::DBG_ALIGNMENT,
           "Unparking: Altitude offset %ld microsteps Azimuth offset %ld microsteps", AltitudeOffsetMicrosteps,
           AzimuthOffsetMicrosteps);

    if (IUFindSwitch(&SlewModesSP, "SLEW_SILENT") != nullptr &&
        IUFindSwitch(&SlewModesSP, "SLEW_SILENT")->s == ISS_ON)
    {
        SilentSlewMode = true;
    }
    else
    {
        SilentSlewMode = false;
    }
    SlewTo(AXIS1, AzimuthOffsetMicrosteps);
    SlewTo(AXIS2, AltitudeOffsetMicrosteps);

    SetParked(false);
    TrackState = SCOPE_SLEWING;
    return true;
}

bool SkywatcherAPIMount::ReadScopeStatus()
{
    DEBUG(INDI::AlignmentSubsystem::DBG_ALIGNMENT, "SkywatcherAPIMount::ReadScopeStatus");

    // leave the following stuff in for the time being it is mostly harmless

    // Quick check of the mount
    if (!GetMotorBoardVersion(AXIS1))
        return false;

    if (!GetStatus(AXIS1))
        return false;

    if (!GetStatus(AXIS2))
        return false;

    // Update Axis Position
    if (!GetEncoder(AXIS1))
        return false;
    if (!GetEncoder(AXIS2))
        return false;

    UpdateDetailedMountInformation(true);

    if (TrackState == SCOPE_PARKING)
    {
        if (!IsInMotion(AXIS1) && !IsInMotion(AXIS2))
        {
            SetParked(true);
        }
    }

    // Calculate new RA DEC
    ln_hrz_posn AltAz { 0, 0 };

    AltAz.alt = MicrostepsToDegrees(AXIS2, CurrentEncoders[AXIS2] - ZeroPositionEncoders[AXIS2]);
    if (IsVirtuosoMount())
    {
        double MountDegree = AltAz.alt;

        // The initial position of the Virtuoso mount is polar aligned when switched on.
        // The altitude is corrected by the latitude.
        if (IUFindNumber(&LocationNP, "LAT") != nullptr)
            MountDegree += IUFindNumber(&LocationNP, "LAT")->value;

        // The altitude degrees in the Virtuoso Alt-Az mount are inverted.
        AltAz.alt = 3420 - MountDegree;
        // Drift compensation for tracking mode (SoftPEC)
        if (IUFindSwitch(&SoftPECModesSP, "SOFTPEC_ENABLED") != nullptr &&
            IUFindSwitch(&SoftPECModesSP, "SOFTPEC_ENABLED")->s == ISS_ON &&
            IUFindNumber(&SoftPecNP, "SOFTPEC_VALUE") != nullptr)
        {
            AltAz.alt += (IUFindNumber(&SoftPecNP, "SOFTPEC_VALUE")->value / 60) * TrackingMsecs / 1000;
        }
    }
    DEBUGF(INDI::AlignmentSubsystem::DBG_ALIGNMENT, "Axis2 encoder %ld initial %ld alt(degrees) %lf",
           CurrentEncoders[AXIS2], ZeroPositionEncoders[AXIS2], AltAz.alt);
    AltAz.az = MicrostepsToDegrees(AXIS1, CurrentEncoders[AXIS1] - ZeroPositionEncoders[AXIS1]);
    if (IsVirtuosoMount())
    {
        if (AltAz.az < 0)
            AltAz.az += 360;
    }
    CurrentAltAz = AltAz;
    DEBUGF(INDI::AlignmentSubsystem::DBG_ALIGNMENT, "Axis1 encoder %ld initial %ld az(degrees) %lf",
           CurrentEncoders[AXIS1], ZeroPositionEncoders[AXIS1], AltAz.az);
    TelescopeDirectionVector TDV = TelescopeDirectionVectorFromAltitudeAzimuth(AltAz);
    DEBUGF(INDI::AlignmentSubsystem::DBG_ALIGNMENT, "TDV x %lf y %lf z %lf", TDV.x, TDV.y, TDV.z);

    double RightAscension, Declination;
    if (TransformTelescopeToCelestial(TDV, RightAscension, Declination))
        DEBUG(INDI::AlignmentSubsystem::DBG_ALIGNMENT, "Conversion OK");
    else
    {
        bool HavePosition = false;
        ln_lnlat_posn Position { 0, 0 };

        if ((nullptr != IUFindNumber(&LocationNP, "LAT")) && (0 != IUFindNumber(&LocationNP, "LAT")->value) &&
            (nullptr != IUFindNumber(&LocationNP, "LONG")) && (0 != IUFindNumber(&LocationNP, "LONG")->value))
        {
            // I assume that being on the equator and exactly on the prime meridian is unlikely
            Position.lat = IUFindNumber(&LocationNP, "LAT")->value;
            Position.lng = IUFindNumber(&LocationNP, "LONG")->value;
            HavePosition = true;
        }
        ln_equ_posn EquatorialCoordinates { 0, 0 };

        if (HavePosition)
        {
            TelescopeDirectionVector RotatedTDV(TDV);
            switch (GetApproximateMountAlignment())
            {
                case ZENITH:
                    break;

                case NORTH_CELESTIAL_POLE:
                    // Rotate the TDV coordinate system anticlockwise (positive) around the y axis by 90 minus
                    // the (positive)observatory latitude. The vector itself is rotated clockwise
                    RotatedTDV.RotateAroundY(90.0 - Position.lat);
                    AltitudeAzimuthFromTelescopeDirectionVector(RotatedTDV, AltAz);
                    break;

                case SOUTH_CELESTIAL_POLE:
                    // Rotate the TDV coordinate system clockwise (negative) around the y axis by 90 plus
                    // the (negative)observatory latitude. The vector itself is rotated anticlockwise
                    RotatedTDV.RotateAroundY(-90.0 - Position.lat);
                    AltitudeAzimuthFromTelescopeDirectionVector(RotatedTDV, AltAz);
                    break;
            }
#ifdef USE_INITIAL_JULIAN_DATE
            ln_get_equ_from_hrz(&AltAz, &Position, InitialJulianDate, &EquatorialCoordinates);
#else
            ln_get_equ_from_hrz(&AltAz, &Position, ln_get_julian_from_sys(), &EquatorialCoordinates);
#endif
        }
        else
            // The best I can do is just do a direct conversion to RA/DEC
            EquatorialCoordinatesFromTelescopeDirectionVector(TDV, EquatorialCoordinates);
        // libnova works in decimal degrees
        RightAscension = EquatorialCoordinates.ra * 24.0 / 360.0;
        Declination    = EquatorialCoordinates.dec;
        DEBUGF(INDI::AlignmentSubsystem::DBG_ALIGNMENT,
               "Conversion Failed - HavePosition %d RA (degrees) %lf DEC (degrees) %lf", HavePosition,
               EquatorialCoordinates.ra, EquatorialCoordinates.dec);
    }

    DEBUGF(INDI::AlignmentSubsystem::DBG_ALIGNMENT, "New RA %lf (hours) DEC %lf (degrees)", RightAscension,
           Declination);
    NewRaDec(RightAscension, Declination);

    return true;
}

bool SkywatcherAPIMount::saveConfigItems(FILE *fp)
{
    SaveAlignmentConfigProperties(fp);

    return INDI::Telescope::saveConfigItems(fp);
}

bool SkywatcherAPIMount::Sync(double ra, double dec)
{
    DEBUG(INDI::AlignmentSubsystem::DBG_ALIGNMENT, "SkywatcherAPIMount::Sync");

    // Compute a telescope direction vector from the current encoders
    if (!GetEncoder(AXIS1))
        return false;
    if (!GetEncoder(AXIS2))
        return false;

    // Syncing is treated specially when the telescope position is known in park position to spare
    // "a huge-jump point" in the alignment model.
    if (isParked())
    {
        ln_hrz_posn AltAz { 0, 0 };
        TelescopeDirectionVector TDV;
        double OrigAlt = 0;

        if (TransformCelestialToTelescope(ra, dec, 0.0, TDV))
        {
            AltitudeAzimuthFromTelescopeDirectionVector(TDV, AltAz);
            OrigAlt = AltAz.alt;
            if (IsVirtuosoMount())
            {
                // The initial position of the Virtuoso mount is polar aligned when switched on.
                // The altitude is corrected by the latitude.
                if (IUFindNumber(&LocationNP, "LAT") != nullptr)
                    AltAz.alt = AltAz.alt - IUFindNumber(&LocationNP, "LAT")->value;

                AltAz.az = 180 + AltAz.az;
            }
            ZeroPositionEncoders[AXIS1] = PolarisPositionEncoders[AXIS1]-DegreesToMicrosteps(AXIS1, AltAz.az);
            ZeroPositionEncoders[AXIS2] = PolarisPositionEncoders[AXIS2]-DegreesToMicrosteps(AXIS2, AltAz.alt);
            MYDEBUGF(INDI::Logger::DBG_SESSION, "Sync (Alt: %lf Az: %lf) in park position", OrigAlt, AltAz.az);
            GetAlignmentDatabase().clear();
            return true;
        }
    }

    // The tracking seconds should be reset to restart the drift compensation
    ResetTrackingSeconds = true;
    // Might as well do this
    UpdateDetailedMountInformation(true);

    ln_hrz_posn AltAz { 0, 0 };

    AltAz.alt = MicrostepsToDegrees(AXIS2, CurrentEncoders[AXIS2] - ZeroPositionEncoders[AXIS2]);
    if (IsVirtuosoMount())
    {
        double MountDegree = AltAz.alt;

        // The initial position of the Virtuoso mount is polar aligned when switched on.
        // The altitude is corrected by the latitude.
        if (IUFindNumber(&LocationNP, "LAT") != nullptr)
            MountDegree += IUFindNumber(&LocationNP, "LAT")->value;

        // The altitude degrees in the Virtuoso Alt-Az mount are inverted.
        AltAz.alt = 3420 - MountDegree;
    }
    DEBUGF(INDI::AlignmentSubsystem::DBG_ALIGNMENT, "Axis2 encoder %ld initial %ld alt(degrees) %lf",
           CurrentEncoders[AXIS2], ZeroPositionEncoders[AXIS2], AltAz.alt);
    AltAz.az = MicrostepsToDegrees(AXIS1, CurrentEncoders[AXIS1] - ZeroPositionEncoders[AXIS1]);
    DEBUGF(INDI::AlignmentSubsystem::DBG_ALIGNMENT, "Axis1 encoder %ld initial %ld az(degrees) %lf",
           CurrentEncoders[AXIS1], ZeroPositionEncoders[AXIS1], AltAz.az);

    AlignmentDatabaseEntry NewEntry;
#ifdef USE_INITIAL_JULIAN_DATE
    NewEntry.ObservationJulianDate = InitialJulianDate;
#else
    NewEntry.ObservationJulianDate = ln_get_julian_from_sys();
#endif
    NewEntry.RightAscension     = ra;
    NewEntry.Declination        = dec;
    NewEntry.TelescopeDirection = TelescopeDirectionVectorFromAltitudeAzimuth(AltAz);
    NewEntry.PrivateDataSize    = 0;

    DEBUGF(INDI::AlignmentSubsystem::DBG_ALIGNMENT, "New sync point Date %lf RA %lf DEC %lf TDV(x %lf y %lf z %lf)",
           NewEntry.ObservationJulianDate, NewEntry.RightAscension, NewEntry.Declination, NewEntry.TelescopeDirection.x,
           NewEntry.TelescopeDirection.y, NewEntry.TelescopeDirection.z);

    if (!CheckForDuplicateSyncPoint(NewEntry))
    {
        GetAlignmentDatabase().push_back(NewEntry);

        // Tell the client about size change
        UpdateSize();

        // Tell the math plugin to reinitialise
        Initialise(this);

        return true;
    }
    return false;
}

void SkywatcherAPIMount::TimerHit()
{
    static bool Slewing  = false;
    static bool Tracking = false;

    // By default this method is called every POLLMS milliseconds

    // Call the base class handler
    // This normally just calls ReadScopeStatus
    INDI::Telescope::TimerHit();

    // Do my own timer stuff assuming ReadScopeStatus has just been called
    SetTimer(TimeoutDuration);

    switch (TrackState)
    {
        case SCOPE_SLEWING:
            if (!Slewing)
            {
                DEBUG(INDI::Logger::DBG_SESSION, "Slewing started");
            }
            TrackingMsecs   = 0;
            GuideDeltaAlt   = 0;
            GuideDeltaAz    = 0;
            ResetGuidePulses();
            TimeoutDuration = 500;
            Tracking        = false;
            Slewing         = true;
            GuidingPulses.clear();
            if ((AxesStatus[AXIS1].FullStop) && (AxesStatus[AXIS2].FullStop))
            {
                if (ISS_ON == IUFindSwitch(&CoordSP, "TRACK")->s)
                {
                    // Goto has finished start tracking
                    TrackState = SCOPE_TRACKING;
                    // Fall through to tracking case
                }
                else
                {
                    TrackState = SCOPE_IDLE;
                    break;
                }
            }
            break;

        case SCOPE_TRACKING:
        {
            if (!Tracking)
            {
                DEBUG(INDI::Logger::DBG_SESSION, "Tracking started");
                TrackingMsecs = 0;
                GuideDeltaAlt = 0;
                GuideDeltaAz  = 0;
                ResetGuidePulses();
                TrackedAltAz  = CurrentAltAz;
            }

            // Restart the drift compensation after syncing
            if (ResetTrackingSeconds)
            {
                ResetTrackingSeconds = false;
                TrackingMsecs        = 0;
                GuideDeltaAlt        = 0;
                GuideDeltaAz         = 0;
                ResetGuidePulses();
                TrackedAltAz         = CurrentAltAz;
            }
            double trackingDeltaAlt = std::abs(CurrentAltAz.alt-TrackedAltAz.alt);
            double trackingDeltaAz = std::abs(CurrentAltAz.az-TrackedAltAz.az);

            if (trackingDeltaAlt+trackingDeltaAz > 50.0)
            {
                IDMessage(nullptr, "Abort tracking after too much margin (%1.4f > 10)",
                          trackingDeltaAlt+trackingDeltaAz);
                Abort();
            }
            TrackingMsecs += TimeoutDuration;
            if (TrackingMsecs % 60000 == 0)
            {
                DEBUGF(INDI::Logger::DBG_SESSION, "Tracking in progress (%d seconds elapsed)", TrackingMsecs / 1000);
            }
            Tracking = true;
            Slewing  = false;
            // Continue or start tracking
            // Calculate where the mount needs to be in POLLMS time
            // POLLMS is hardcoded to be one second
            double JulianOffset =
                1.0 / (24.0 * 60 * 60); // TODO may need to make this longer to get a meaningful result
            TelescopeDirectionVector TDV;
            ln_hrz_posn AltAz { 0, 0 };

            if (TransformCelestialToTelescope(CurrentTrackingTarget.ra, CurrentTrackingTarget.dec,
#ifdef USE_INITIAL_JULIAN_DATE
                                              0, TDV))
#else
                                              JulianOffset, TDV))
#endif
            {
                DEBUGF(INDI::AlignmentSubsystem::DBG_ALIGNMENT, "TDV x %lf y %lf z %lf", TDV.x, TDV.y, TDV.z);
                AltitudeAzimuthFromTelescopeDirectionVector(TDV, AltAz);
            }
            else
            {
                // Try a conversion with the stored observatory position if any
                bool HavePosition = false;
                ln_lnlat_posn Position { 0, 0 };

                if ((nullptr != IUFindNumber(&LocationNP, "LAT")) && (0 != IUFindNumber(&LocationNP, "LAT")->value) &&
                    (nullptr != IUFindNumber(&LocationNP, "LONG")) && (0 != IUFindNumber(&LocationNP, "LONG")->value))
                {
                    // I assume that being on the equator and exactly on the prime meridian is unlikely
                    Position.lat = IUFindNumber(&LocationNP, "LAT")->value;
                    Position.lng = IUFindNumber(&LocationNP, "LONG")->value;
                    HavePosition = true;
                }
                ln_equ_posn EquatorialCoordinates { 0, 0 };

                // libnova works in decimal degrees
                EquatorialCoordinates.ra  = CurrentTrackingTarget.ra * 360.0 / 24.0;
                EquatorialCoordinates.dec = CurrentTrackingTarget.dec;
                if (HavePosition)
                    ln_get_hrz_from_equ(&EquatorialCoordinates, &Position,
#ifdef USE_INITIAL_JULIAN_DATE
                                        InitialJulianDate, &AltAz);
#else
                                        ln_get_julian_from_sys() + JulianOffset, &AltAz);
#endif
                else
                {
                    // No sense in tracking in this case
                    TrackState = SCOPE_IDLE;
                    break;
                }
            }
            if (IsVirtuosoMount())
            {
                // The initial position of the Virtuoso mount is polar aligned when switched on.
                // The altitude is corrected by the latitude.
                if (IUFindNumber(&LocationNP, "LAT") != nullptr)
                {
                    AltAz.alt = AltAz.alt - IUFindNumber(&LocationNP, "LAT")->value;
                }
                // Drift compensation for tracking mode (SoftPEC)
                if (IUFindSwitch(&SoftPECModesSP, "SOFTPEC_ENABLED") != nullptr &&
                    IUFindSwitch(&SoftPECModesSP, "SOFTPEC_ENABLED")->s == ISS_ON &&
                    IUFindNumber(&SoftPecNP, "SOFTPEC_VALUE") != nullptr)
                {
                    AltAz.alt += (IUFindNumber(&SoftPecNP, "SOFTPEC_VALUE")->value / 60) * TrackingMsecs / 1000;
                }
                AltAz.az = 180 + AltAz.az;
            }
            DEBUGF(DBG_SCOPE,
                   "Tracking AXIS1 CurrentEncoder %ld OldTrackingTarget %ld AXIS2 CurrentEncoder %ld OldTrackingTarget "
                   "%ld",
                   CurrentEncoders[AXIS1], OldTrackingTarget[AXIS1], CurrentEncoders[AXIS2], OldTrackingTarget[AXIS2]);
            DEBUGF(DBG_SCOPE,
                   "New Tracking Target Altitude %lf degrees %ld microsteps Azimuth %lf degrees %ld microsteps",
                   AltAz.alt, DegreesToMicrosteps(AXIS2, AltAz.alt), AltAz.az, DegreesToMicrosteps(AXIS1, AltAz.az));

            // Calculate the auto-guiding delta degrees
            double DeltaAlt = 0;
            double DeltaAz = 0;

            for (auto Iter = GuidingPulses.begin(); Iter != GuidingPulses.end(); )
            {
                // We treat the guide calibration specially
                if (Iter->OriginalDuration == 1000)
                {
                    DeltaAlt += Iter->DeltaAlt;
                    DeltaAz += Iter->DeltaAz;
                } else {
                    DeltaAlt += Iter->DeltaAlt / 2;
                    DeltaAz += Iter->DeltaAz / 2;
                }
                Iter->Duration -= TimeoutDuration;

                if (Iter->Duration < TimeoutDuration)
                {
                    Iter = GuidingPulses.erase(Iter);
                    if (Iter == GuidingPulses.end())
                    {
                        break;
                    }
                    continue;
                }
                ++Iter;
            }
            GuideDeltaAlt += DeltaAlt;
            GuideDeltaAz += DeltaAz;

            long AltitudeOffsetMicrosteps =
                DegreesToMicrosteps(AXIS2, AltAz.alt+GuideDeltaAlt) + ZeroPositionEncoders[AXIS2] - CurrentEncoders[AXIS2];
            long AzimuthOffsetMicrosteps =
                DegreesToMicrosteps(AXIS1, AltAz.az+GuideDeltaAz) + ZeroPositionEncoders[AXIS1] - CurrentEncoders[AXIS1];

            DEBUGF(DBG_SCOPE, "New Tracking Target AltitudeOffset %ld microsteps AzimuthOffset %ld microsteps",
                   AltitudeOffsetMicrosteps, AzimuthOffsetMicrosteps);

            if (AzimuthOffsetMicrosteps > MicrostepsPerRevolution[AXIS1] / 2)
            {
                DEBUG(DBG_SCOPE, "Tracking AXIS1 going long way round");
                // Going the long way round - send it the other way
                AzimuthOffsetMicrosteps -= MicrostepsPerRevolution[AXIS1];
            }
            if (0 != AzimuthOffsetMicrosteps)
            {
                // Calculate the slewing rates needed to reach that position
                // at the correct time.
                long AzimuthRate = StepperClockFrequency[AXIS1] / AzimuthOffsetMicrosteps;
                if (!AxesStatus[AXIS1].FullStop && ((AxesStatus[AXIS1].SlewingForward && (AzimuthRate < 0)) ||
                                                    (!AxesStatus[AXIS1].SlewingForward && (AzimuthRate > 0))))
                {
                    // Direction change whilst axis running
                    // Abandon tracking for this clock tick
                    DEBUG(DBG_SCOPE, "Tracking - AXIS1 direction change");
                    SlowStop(AXIS1);
                }
                else
                {
                    char Direction = AzimuthRate > 0 ? '0' : '1';
                    AzimuthRate    = std::abs(AzimuthRate);
                    SetClockTicksPerMicrostep(AXIS1, AzimuthRate < 1 ? 1 : AzimuthRate);
                    if (AxesStatus[AXIS1].FullStop)
                    {
                        DEBUG(DBG_SCOPE, "Tracking - AXIS1 restart");
                        SetMotionMode(AXIS1, '1', Direction);
                        StartMotion(AXIS1);
                    }
                    DEBUGF(DBG_SCOPE, "Tracking - AXIS1 offset %ld microsteps rate %ld direction %c",
                           AzimuthOffsetMicrosteps, AzimuthRate, Direction);
                }
            }
            else
            {
                // Nothing to do - stop the axis
                DEBUG(DBG_SCOPE, "Tracking - AXIS1 zero offset");
                SlowStop(AXIS1);
            }

            // Do I need to take out any complete revolutions before I do this test?
            if (AltitudeOffsetMicrosteps > MicrostepsPerRevolution[AXIS2] / 2)
            {
                DEBUG(DBG_SCOPE, "Tracking AXIS2 going long way round");
                // Going the long way round - send it the other way
                AltitudeOffsetMicrosteps -= MicrostepsPerRevolution[AXIS2];
            }
            if (0 != AltitudeOffsetMicrosteps)
            {
                // Calculate the slewing rates needed to reach that position
                // at the correct time.
                long AltitudeRate = StepperClockFrequency[AXIS2] / AltitudeOffsetMicrosteps;

                if (!AxesStatus[AXIS2].FullStop && ((AxesStatus[AXIS2].SlewingForward && (AltitudeRate < 0)) ||
                                                    (!AxesStatus[AXIS2].SlewingForward && (AltitudeRate > 0))))
                {
                    // Direction change whilst axis running
                    // Abandon tracking for this clock tick
                    DEBUG(DBG_SCOPE, "Tracking - AXIS2 direction change");
                    SlowStop(AXIS2);
                }
                else
                {
                    char Direction = AltitudeRate > 0 ? '0' : '1';
                    AltitudeRate   = std::abs(AltitudeRate);
                    SetClockTicksPerMicrostep(AXIS2, AltitudeRate < 1 ? 1 : AltitudeRate);
                    if (AxesStatus[AXIS2].FullStop)
                    {
                        DEBUG(DBG_SCOPE, "Tracking - AXIS2 restart");
                        SetMotionMode(AXIS2, '1', Direction);
                        StartMotion(AXIS2);
                    }
                    DEBUGF(DBG_SCOPE, "Tracking - AXIS2 offset %ld microsteps rate %ld direction %c",
                           AltitudeOffsetMicrosteps, AltitudeRate, Direction);
                }
            }
            else
            {
                // Nothing to do - stop the axis
                DEBUG(DBG_SCOPE, "Tracking - AXIS2 zero offset");
                SlowStop(AXIS2);
            }

            DEBUGF(DBG_SCOPE, "Tracking - AXIS1 error %d AXIS2 error %d",
                   OldTrackingTarget[AXIS1] - CurrentEncoders[AXIS1],
                   OldTrackingTarget[AXIS2] - CurrentEncoders[AXIS2]);

            OldTrackingTarget[AXIS1] = AzimuthOffsetMicrosteps + CurrentEncoders[AXIS1];
            OldTrackingTarget[AXIS2] = AltitudeOffsetMicrosteps + CurrentEncoders[AXIS2];
            break;
        }
        break;

        default:
            if (Slewing)
            {
                DEBUG(INDI::Logger::DBG_SESSION, "Slewing stopped");
            }
            if (Tracking)
            {
                DEBUG(INDI::Logger::DBG_SESSION, "Tracking stopped");
            }
            TrackingMsecs   = 0;
            GuideDeltaAlt   = 0;
            GuideDeltaAz    = 0;
            ResetGuidePulses();
            TimeoutDuration = 500;
            Tracking        = false;
            Slewing         = false;
            GuidingPulses.clear();
            break;
    }
}

bool SkywatcherAPIMount::updateLocation(double latitude, double longitude, double elevation)
{
    DEBUG(DBG_SCOPE, "SkywatcherAPIMount::updateLocation");
    UpdateLocation(latitude, longitude, elevation);
    return true;
}

bool SkywatcherAPIMount::updateProperties()
{
    INDI::Telescope::updateProperties();

    if (isConnected())
    {
        // Fill in any real values now available MCInit should have been called already
        UpdateDetailedMountInformation(false);

        // Define our connected only properties to the base driver
        // e.g. defineNumber(MyNumberVectorPointer);
        // This will register our properties and send a IDDefXXXX message to any connected clients
        // I have now idea why I have to do this here as well as in ISGetProperties. It makes me
        // concerned there is a design or implementation flaw somewhere.
        defineText(&BasicMountInfoV);
        defineNumber(&AxisOneInfoV);
        defineSwitch(&AxisOneStateV);
        defineNumber(&AxisTwoInfoV);
        defineSwitch(&AxisTwoStateV);
        defineNumber(&AxisOneEncoderValuesV);
        defineNumber(&AxisTwoEncoderValuesV);
        defineSwitch(&SlewModesSP);
        defineSwitch(&SoftPECModesSP);
        defineNumber(&SoftPecNP);
        defineNumber(&GuidingRatesNP);
        defineSwitch(&ParkMovementDirectionSP);
        defineSwitch(&ParkPositionSP);
        defineSwitch(&UnparkPositionSP);

        defineNumber(&GuideNSNP);
        defineNumber(&GuideWENP);

        // Start the timer if we need one
        // SetTimer(POLLMS);
        return true;
    }
    else
    {
        // Delete any connected only properties from the base driver's list
        // e.g. deleteProperty(MyNumberVector.name);
        deleteProperty(BasicMountInfoV.name);
        deleteProperty(AxisOneInfoV.name);
        deleteProperty(AxisOneStateV.name);
        deleteProperty(AxisTwoInfoV.name);
        deleteProperty(AxisTwoStateV.name);
        deleteProperty(AxisOneEncoderValuesV.name);
        deleteProperty(AxisTwoEncoderValuesV.name);
        deleteProperty(SlewModesSP.name);
        deleteProperty(SoftPECModesSP.name);
        deleteProperty(SoftPecNP.name);
        deleteProperty(GuidingRatesNP.name);
        deleteProperty(ParkMovementDirectionSP.name);
        deleteProperty(ParkPositionSP.name);
        deleteProperty(UnparkPositionSP.name);

        deleteProperty(GuideNSNP.name);
        deleteProperty(GuideWENP.name);

        return true;
    }
}

IPState SkywatcherAPIMount::GuideNorth(float ms)
{
    GuidingPulse Pulse;

    TimeoutDuration = 250;
    CalculateGuidePulses();
    Pulse.DeltaAz = NorthPulse.DeltaAz;
    Pulse.DeltaAlt = NorthPulse.DeltaAlt;
    Pulse.Duration = (int)ms;
    Pulse.OriginalDuration = (int)ms;
    GuidingPulses.push_back(Pulse);
//    IDMessage(nullptr, "GUIDE NORTH: %1.2f msec - deltaalt: %1.6f deltaaz: %1.6f", ms,
//              Pulse.DeltaAlt, Pulse.DeltaAz);
    return IPS_OK;
}

IPState SkywatcherAPIMount::GuideSouth(float ms)
{
    GuidingPulse Pulse;

    TimeoutDuration = 250;
    CalculateGuidePulses();
    Pulse.DeltaAz = -NorthPulse.DeltaAz;
    Pulse.DeltaAlt = -NorthPulse.DeltaAlt;
    Pulse.Duration = (int)ms;
    Pulse.OriginalDuration = (int)ms;
    GuidingPulses.push_back(Pulse);
//    IDMessage(nullptr, "GUIDE SOUTH: %1.2f msec - deltaalt: %1.6f deltaaz: %1.6f", ms,
//              Pulse.DeltaAlt, Pulse.DeltaAz);
    return IPS_OK;
}

IPState SkywatcherAPIMount::GuideWest(float ms)
{
    GuidingPulse Pulse;

    TimeoutDuration = 250;
    CalculateGuidePulses();
    Pulse.DeltaAz = WestPulse.DeltaAz;
    Pulse.DeltaAlt = WestPulse.DeltaAlt;
    Pulse.Duration = (int)ms;
    Pulse.OriginalDuration = (int)ms;
    GuidingPulses.push_back(Pulse);
//    IDMessage(nullptr, "GUIDE WEST: %1.2f msec - deltaalt: %1.6f deltaaz: %1.6f", ms,
//              Pulse.DeltaAlt, Pulse.DeltaAz);
    return IPS_OK;
}

IPState SkywatcherAPIMount::GuideEast(float ms)
{
    GuidingPulse Pulse;

    TimeoutDuration = 250;
    CalculateGuidePulses();
    Pulse.DeltaAz = -WestPulse.DeltaAz;
    Pulse.DeltaAlt = -WestPulse.DeltaAlt;
    Pulse.Duration = (int)ms;
    Pulse.OriginalDuration = (int)ms;
    GuidingPulses.push_back(Pulse);
//    IDMessage(nullptr, "GUIDE EAST: %1.2f msec - deltaalt: %1.6f deltaaz: %1.6f", ms,
//              Pulse.DeltaAlt, Pulse.DeltaAz);
    return IPS_OK;
}

// Private methods

void SkywatcherAPIMount::CalculateGuidePulses()
{
    if (NorthPulse.Duration != 0 || WestPulse.Duration != 0)
        return;

    // Calculate the west reference delta
    // Note: The RA is multiplied by 3.75 (90/24) to be more comparable with DEC values.
    const double WestRate = IUFindNumber(&GuidingRatesNP, "GUIDERA_RATE")->value / 10*-(double)1 / 60 / 60*3.75 / 100;

    ConvertGuideCorrection(WestRate, 0, WestPulse.DeltaAlt, WestPulse.DeltaAz);
    WestPulse.Duration = 1;

    // Calculate the north reference delta
    // Note: By some reason, it must be multiplied by 100 to match with the RA values.
    const double NorthRate = IUFindNumber(&GuidingRatesNP, "GUIDEDEC_RATE")->value / 10*(double)1 / 60 / 60*100 / 100;

    ConvertGuideCorrection(0, NorthRate, NorthPulse.DeltaAlt, NorthPulse.DeltaAz);
    NorthPulse.Duration = 1;
}

void SkywatcherAPIMount::ResetGuidePulses()
{
    NorthPulse.Duration = 0;
    WestPulse.Duration = 0;
}

void SkywatcherAPIMount::ConvertGuideCorrection(double delta_ra, double delta_dec, double &delta_alt, double &delta_az)
{
    ln_hrz_posn OldAltAz { 0, 0 };
    ln_hrz_posn NewAltAz { 0, 0 };
    TelescopeDirectionVector OldTDV;
    TelescopeDirectionVector NewTDV;

    TransformCelestialToTelescope(CurrentTrackingTarget.ra, CurrentTrackingTarget.dec, 0.0, OldTDV);
    AltitudeAzimuthFromTelescopeDirectionVector(OldTDV, OldAltAz);
    TransformCelestialToTelescope(CurrentTrackingTarget.ra+delta_ra, CurrentTrackingTarget.dec+delta_dec, 0.0, NewTDV);
    AltitudeAzimuthFromTelescopeDirectionVector(NewTDV, NewAltAz);
    delta_alt = NewAltAz.alt-OldAltAz.alt;
    delta_az = NewAltAz.az-OldAltAz.az;
}

int SkywatcherAPIMount::skywatcher_tty_read(int fd, char *buf, int nbytes, int timeout, int *nbytes_read)
{
    if (!RecoverAfterReconnection && !SerialPortName.empty() && !FileExists(SerialPortName))
    {
        RecoverAfterReconnection = true;
        serialConnection->Disconnect();
        serialConnection->Refresh();
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        if (!serialConnection->Connect())
        {
            RecoverAfterReconnection = true;
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            if (!serialConnection->Connect())
            {
                RecoverAfterReconnection = false;
                return 0;
            }
        }
        SetSerialPort(serialConnection->getPortFD());
        SerialPortName           = serialConnection->port();
        RecoverAfterReconnection = false;
    }
    return tty_read(fd, buf, nbytes, timeout, nbytes_read);
}

int SkywatcherAPIMount::skywatcher_tty_write(int fd, const char *buffer, int nbytes, int *nbytes_written)
{
    if (!RecoverAfterReconnection && !SerialPortName.empty() && !FileExists(SerialPortName))
    {
        RecoverAfterReconnection = true;
        serialConnection->Disconnect();
        serialConnection->Refresh();
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        if (!serialConnection->Connect())
        {
            RecoverAfterReconnection = true;
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            if (!serialConnection->Connect())
            {
                RecoverAfterReconnection = false;
                return 0;
            }
        }
        SetSerialPort(serialConnection->getPortFD());
        SerialPortName           = serialConnection->port();
        RecoverAfterReconnection = false;
    }
    return tty_write(fd, buffer, nbytes, nbytes_written);
}

void SkywatcherAPIMount::SkywatcherMicrostepsFromTelescopeDirectionVector(
    const TelescopeDirectionVector TelescopeDirectionVector, long &Axis1Microsteps, long &Axis2Microsteps)
{
    // For the time being I assume that all skywathcer mounts share the same encoder conventions
    double Axis1Angle = 0;
    double Axis2Angle = 0;
    SphericalCoordinateFromTelescopeDirectionVector(TelescopeDirectionVector, Axis1Angle,
                                                    TelescopeDirectionVectorSupportFunctions::CLOCKWISE, Axis1Angle,
                                                    FROM_AZIMUTHAL_PLANE);

    Axis1Microsteps = RadiansToMicrosteps(AXIS1, Axis1Angle);
    Axis2Microsteps = RadiansToMicrosteps(AXIS2, Axis2Angle);
}

const TelescopeDirectionVector
SkywatcherAPIMount::TelescopeDirectionVectorFromSkywatcherMicrosteps(long Axis1Microsteps, long Axis2Microsteps)
{
    // For the time being I assume that all skywathcer mounts share the same encoder conventions
    double Axis1Angle = MicrostepsToRadians(AXIS1, Axis1Microsteps);
    double Axis2Angle = MicrostepsToRadians(AXIS2, Axis2Microsteps);
    return TelescopeDirectionVectorFromSphericalCoordinate(
        Axis1Angle, TelescopeDirectionVectorSupportFunctions::CLOCKWISE, Axis2Angle, FROM_AZIMUTHAL_PLANE);
}

void SkywatcherAPIMount::UpdateDetailedMountInformation(bool InformClient)
{
    bool BasicMountInfoHasChanged = false;

    if (std::string(BasicMountInfo[MOTOR_CONTROL_FIRMWARE_VERSION].text) != std::to_string(MCVersion))
    {
        IUSaveText(&BasicMountInfo[MOTOR_CONTROL_FIRMWARE_VERSION], std::to_string(MCVersion).c_str());
        BasicMountInfoHasChanged = true;
    }
    if (std::string(BasicMountInfo[MOUNT_CODE].text) != std::to_string(MountCode))
    {
        IUSaveText(&BasicMountInfo[MOUNT_CODE], std::to_string(MountCode).c_str());
        SetApproximateMountAlignmentFromMountType(ALTAZ);
        BasicMountInfoHasChanged = true;
    }
    if (std::string(BasicMountInfo[IS_DC_MOTOR].text) != std::to_string(IsDCMotor))
    {
        IUSaveText(&BasicMountInfo[IS_DC_MOTOR], std::to_string(IsDCMotor).c_str());
        BasicMountInfoHasChanged = true;
    }
    if (BasicMountInfoHasChanged && InformClient)
        IDSetText(&BasicMountInfoV, nullptr);

    if (MountCode >= 128 && MountCode <= 143)
        IUSaveText(&BasicMountInfo[MOUNT_NAME], "Az Goto");
    if (MountCode >= 144 && MountCode <= 159)
        IUSaveText(&BasicMountInfo[MOUNT_NAME], "Dob Goto");
    if (MountCode >= 160)
        IUSaveText(&BasicMountInfo[MOUNT_NAME], "AllView Goto");

    bool AxisOneInfoHasChanged = false;

    if (AxisOneInfo[MICROSTEPS_PER_REVOLUTION].value != MicrostepsPerRevolution[0])
    {
        AxisOneInfo[MICROSTEPS_PER_REVOLUTION].value = MicrostepsPerRevolution[0];
        AxisOneInfoHasChanged                        = true;
    }
    if (AxisOneInfo[STEPPER_CLOCK_FREQUENCY].value != StepperClockFrequency[0])
    {
        AxisOneInfo[STEPPER_CLOCK_FREQUENCY].value = StepperClockFrequency[0];
        AxisOneInfoHasChanged                      = true;
    }
    if (AxisOneInfo[HIGH_SPEED_RATIO].value != HighSpeedRatio[0])
    {
        AxisOneInfo[HIGH_SPEED_RATIO].value = HighSpeedRatio[0];
        AxisOneInfoHasChanged               = true;
    }
    if (AxisOneInfo[MICROSTEPS_PER_WORM_REVOLUTION].value != MicrostepsPerWormRevolution[0])
    {
        AxisOneInfo[MICROSTEPS_PER_WORM_REVOLUTION].value = MicrostepsPerWormRevolution[0];
        AxisOneInfoHasChanged                             = true;
    }
    if (AxisOneInfoHasChanged && InformClient)
        IDSetNumber(&AxisOneInfoV, nullptr);

    bool AxisOneStateHasChanged = false;
    if (AxisOneState[FULL_STOP].s != (AxesStatus[0].FullStop ? ISS_ON : ISS_OFF))
    {
        AxisOneState[FULL_STOP].s = AxesStatus[0].FullStop ? ISS_ON : ISS_OFF;
        AxisOneStateHasChanged    = true;
    }
    if (AxisOneState[SLEWING].s != (AxesStatus[0].Slewing ? ISS_ON : ISS_OFF))
    {
        AxisOneState[SLEWING].s = AxesStatus[0].Slewing ? ISS_ON : ISS_OFF;
        AxisOneStateHasChanged  = true;
    }
    if (AxisOneState[SLEWING_TO].s != (AxesStatus[0].SlewingTo ? ISS_ON : ISS_OFF))
    {
        AxisOneState[SLEWING_TO].s = AxesStatus[0].SlewingTo ? ISS_ON : ISS_OFF;
        AxisOneStateHasChanged     = true;
    }
    if (AxisOneState[SLEWING_FORWARD].s != (AxesStatus[0].SlewingForward ? ISS_ON : ISS_OFF))
    {
        AxisOneState[SLEWING_FORWARD].s = AxesStatus[0].SlewingForward ? ISS_ON : ISS_OFF;
        AxisOneStateHasChanged          = true;
    }
    if (AxisOneState[HIGH_SPEED].s != (AxesStatus[0].HighSpeed ? ISS_ON : ISS_OFF))
    {
        AxisOneState[HIGH_SPEED].s = AxesStatus[0].HighSpeed ? ISS_ON : ISS_OFF;
        AxisOneStateHasChanged     = true;
    }
    if (AxisOneState[NOT_INITIALISED].s != (AxesStatus[0].NotInitialized ? ISS_ON : ISS_OFF))
    {
        AxisOneState[NOT_INITIALISED].s = AxesStatus[0].NotInitialized ? ISS_ON : ISS_OFF;
        AxisOneStateHasChanged          = true;
    }
    if (AxisOneStateHasChanged && InformClient)
        IDSetSwitch(&AxisOneStateV, nullptr);

    bool AxisTwoInfoHasChanged = false;
    if (AxisTwoInfo[MICROSTEPS_PER_REVOLUTION].value != MicrostepsPerRevolution[1])
    {
        AxisTwoInfo[MICROSTEPS_PER_REVOLUTION].value = MicrostepsPerRevolution[1];
        AxisTwoInfoHasChanged                        = true;
    }
    if (AxisTwoInfo[STEPPER_CLOCK_FREQUENCY].value != StepperClockFrequency[1])
    {
        AxisTwoInfo[STEPPER_CLOCK_FREQUENCY].value = StepperClockFrequency[1];
        AxisTwoInfoHasChanged                      = true;
    }
    if (AxisTwoInfo[HIGH_SPEED_RATIO].value != HighSpeedRatio[1])
    {
        AxisTwoInfo[HIGH_SPEED_RATIO].value = HighSpeedRatio[1];
        AxisTwoInfoHasChanged               = true;
    }
    if (AxisTwoInfo[MICROSTEPS_PER_WORM_REVOLUTION].value != MicrostepsPerWormRevolution[1])
    {
        AxisTwoInfo[MICROSTEPS_PER_WORM_REVOLUTION].value = MicrostepsPerWormRevolution[1];
        AxisTwoInfoHasChanged                             = true;
    }
    if (AxisTwoInfoHasChanged && InformClient)
        IDSetNumber(&AxisTwoInfoV, nullptr);

    bool AxisTwoStateHasChanged = false;
    if (AxisTwoState[FULL_STOP].s != (AxesStatus[1].FullStop ? ISS_ON : ISS_OFF))
    {
        AxisTwoState[FULL_STOP].s = AxesStatus[1].FullStop ? ISS_ON : ISS_OFF;
        AxisTwoStateHasChanged    = true;
    }
    if (AxisTwoState[SLEWING].s != (AxesStatus[1].Slewing ? ISS_ON : ISS_OFF))
    {
        AxisTwoState[SLEWING].s = AxesStatus[1].Slewing ? ISS_ON : ISS_OFF;
        AxisTwoStateHasChanged  = true;
    }
    if (AxisTwoState[SLEWING_TO].s != (AxesStatus[1].SlewingTo ? ISS_ON : ISS_OFF))
    {
        AxisTwoState[SLEWING_TO].s = AxesStatus[1].SlewingTo ? ISS_ON : ISS_OFF;
        AxisTwoStateHasChanged     = true;
    }
    if (AxisTwoState[SLEWING_FORWARD].s != (AxesStatus[1].SlewingForward ? ISS_ON : ISS_OFF))
    {
        AxisTwoState[SLEWING_FORWARD].s = AxesStatus[1].SlewingForward ? ISS_ON : ISS_OFF;
        AxisTwoStateHasChanged          = true;
    }
    if (AxisTwoState[HIGH_SPEED].s != (AxesStatus[1].HighSpeed ? ISS_ON : ISS_OFF))
    {
        AxisTwoState[HIGH_SPEED].s = AxesStatus[1].HighSpeed ? ISS_ON : ISS_OFF;
        AxisTwoStateHasChanged     = true;
    }
    if (AxisTwoState[NOT_INITIALISED].s != (AxesStatus[1].NotInitialized ? ISS_ON : ISS_OFF))
    {
        AxisTwoState[NOT_INITIALISED].s = AxesStatus[1].NotInitialized ? ISS_ON : ISS_OFF;
        AxisTwoStateHasChanged          = true;
    }
    if (AxisTwoStateHasChanged && InformClient)
        IDSetSwitch(&AxisTwoStateV, nullptr);

    bool AxisOneEncoderValuesHasChanged = false;
    if ((AxisOneEncoderValues[RAW_MICROSTEPS].value != CurrentEncoders[AXIS1]) ||
        (AxisOneEncoderValues[OFFSET_FROM_INITIAL].value != CurrentEncoders[AXIS1] - ZeroPositionEncoders[AXIS1]))
    {
        AxisOneEncoderValues[RAW_MICROSTEPS].value = CurrentEncoders[AXIS1];
        AxisOneEncoderValues[MICROSTEPS_PER_ARCSEC].value = MicrostepsPerDegree[AXIS1] / 3600.0;
        AxisOneEncoderValues[OFFSET_FROM_INITIAL].value = CurrentEncoders[AXIS1] - ZeroPositionEncoders[AXIS1];
        AxisOneEncoderValues[DEGREES_FROM_INITIAL].value =
            MicrostepsToDegrees(AXIS1, CurrentEncoders[AXIS1] - ZeroPositionEncoders[AXIS1]);
        AxisOneEncoderValuesHasChanged = true;
    }
    if (AxisOneEncoderValuesHasChanged && InformClient)
        IDSetNumber(&AxisOneEncoderValuesV, nullptr);

    bool AxisTwoEncoderValuesHasChanged = false;
    if ((AxisTwoEncoderValues[RAW_MICROSTEPS].value != CurrentEncoders[AXIS2]) ||
        (AxisTwoEncoderValues[OFFSET_FROM_INITIAL].value != CurrentEncoders[AXIS2] - ZeroPositionEncoders[AXIS2]))
    {
        AxisTwoEncoderValues[RAW_MICROSTEPS].value      = CurrentEncoders[AXIS2];
        AxisTwoEncoderValues[MICROSTEPS_PER_ARCSEC].value = MicrostepsPerDegree[AXIS2] / 3600.0;
        AxisTwoEncoderValues[OFFSET_FROM_INITIAL].value = CurrentEncoders[AXIS2] - ZeroPositionEncoders[AXIS2];
        AxisTwoEncoderValues[DEGREES_FROM_INITIAL].value =
            MicrostepsToDegrees(AXIS2, CurrentEncoders[AXIS2] - ZeroPositionEncoders[AXIS2]);
        AxisTwoEncoderValuesHasChanged = true;
    }
    if (AxisTwoEncoderValuesHasChanged && InformClient)
        IDSetNumber(&AxisTwoEncoderValuesV, nullptr);
}

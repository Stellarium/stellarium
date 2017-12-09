
#include "nexstarevo.h"

#include "config.h"

#include <indicom.h>
#include <connectionplugins/connectionserial.h>

using namespace INDI::AlignmentSubsystem;

#define POLLMS              1000 // Default timer tick
#define MAX_SLEW_RATE       9
#define FIND_SLEW_RATE      7
#define CENTERING_SLEW_RATE 3
#define GUIDE_SLEW_RATE     2

// We declare an auto pointer to NexStarEvo.
std::unique_ptr<NexStarEvo> telescope_nse(new NexStarEvo());

void ISGetProperties(const char *dev)
{
    telescope_nse->ISGetProperties(dev);
}

void ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int num)
{
    telescope_nse->ISNewSwitch(dev, name, states, names, num);
}

void ISNewText(const char *dev, const char *name, char *texts[], char *names[], int num)
{
    telescope_nse->ISNewText(dev, name, texts, names, num);
}

void ISNewNumber(const char *dev, const char *name, double values[], char *names[], int num)
{
    telescope_nse->ISNewNumber(dev, name, values, names, num);
}

void ISNewBLOB(const char *dev, const char *name, int sizes[], int blobsizes[], char *blobs[], char *formats[],
               char *names[], int n)
{
    telescope_nse->ISNewBLOB(dev, name, sizes, blobsizes, blobs, formats, names, n);
}

void ISSnoopDevice(XMLEle *root)
{
    telescope_nse->ISSnoopDevice(root);
}

// One definition rule (ODR) constants
// AUX commands use 24bit integer as a representation of angle in units of
// fractional revolutions. Thus 2^24 steps makes full revolution.
const long NexStarEvo::STEPS_PER_REVOLUTION = 16777216;
const double NexStarEvo::STEPS_PER_DEGREE   = STEPS_PER_REVOLUTION / 360.0;
const double NexStarEvo::DEFAULT_SLEW_RATE  = STEPS_PER_DEGREE * 2.0;
const long NexStarEvo::MAX_ALT              = 90.0 * STEPS_PER_DEGREE;
const long NexStarEvo::MIN_ALT              = -90.0 * STEPS_PER_DEGREE;

// The guide rate is probably (???) measured in 1000 arcmin/min
// This is based on experimentation and guesswork.
// The rate is calculated in steps/min - thus conversion is required.
// The best experimental value was 1.315 which is quite close
// to 60000/STEPS_PER_DEGREE = 1.2874603271484375.
const double NexStarEvo::TRACK_SCALE = 60000 / STEPS_PER_DEGREE;

NexStarEvo::NexStarEvo()
    : ScopeStatus(IDLE), AxisStatusALT(STOPPED), AxisDirectionALT(FORWARD), AxisStatusAZ(STOPPED),
      AxisDirectionAZ(FORWARD), TraceThisTickCount(0), TraceThisTick(false),
      DBG_NSEVO(INDI::Logger::DBG_SESSION),
      DBG_MOUNT(INDI::Logger::getInstance().addDebugLevel("NexStar Evo Verbose", "NSEVO"))
{
    setVersion(NSEVO_VERSION_MAJOR, NSEVO_VERSION_MINOR);
    SetTelescopeCapability(TELESCOPE_CAN_PARK | TELESCOPE_CAN_SYNC | TELESCOPE_CAN_GOTO | TELESCOPE_CAN_ABORT |
                               TELESCOPE_HAS_TIME | TELESCOPE_HAS_LOCATION,
                           4);
    // Approach from no further then degs away
    Approach = 1.0;

    // Max ticks before we reissue the goto to update position
    maxSlewTicks = 15;
}

NexStarEvo::~NexStarEvo()
{
}

// Private methods

bool NexStarEvo::Abort()
{
    if (MovementNSSP.s == IPS_BUSY)
    {
        IUResetSwitch(&MovementNSSP);
        MovementNSSP.s = IPS_IDLE;
        IDSetSwitch(&MovementNSSP, NULL);
    }

    if (MovementWESP.s == IPS_BUSY)
    {
        MovementWESP.s = IPS_IDLE;
        IUResetSwitch(&MovementWESP);
        IDSetSwitch(&MovementWESP, NULL);
    }

    if (EqNP.s == IPS_BUSY)
    {
        EqNP.s = IPS_IDLE;
        IDSetNumber(&EqNP, NULL);
    }

    TrackState = SCOPE_IDLE;

    AxisStatusAZ = AxisStatusALT = STOPPED;
    ScopeStatus                  = IDLE;
    scope.Abort();
    AbortSP.s = IPS_OK;
    IUResetSwitch(&AbortSP);
    IDSetSwitch(&AbortSP, NULL);
    DEBUG(INDI::Logger::DBG_SESSION, "Telescope aborted.");

    return true;
}

#if 0
bool NexStarEvo::Connect()
{
    SetTimer(POLLMS);
    // Check if TCP Address exists and not empty.
    // We call the TCP function in case the connection mode is set explicitly to TCP **OR** if the address is not empty (for CONNECTION_BOTH) then
    // TCP connection has higher priority than serial port.
    // For now only autodetected TCP connection is supported
    TelescopeConnection mode = getConnectionMode();
    if (mode == CONNECTION_TCP && (AddressT[0].text && AddressT[0].text[0] && AddressT[1].text && AddressT[1].text[0])) {
        // We have the proposed IP:port let us pass that

    } else if (mode == CONNECTION_TCP ) {
        // TCP mode but no IP fields - use detection/default IP:port
        if (scope == NULL)
            scope = new NexStarAUXScope();
    }
    if (scope != NULL) {
        scope.Connect();
    }
    return true;
}
#endif

bool NexStarEvo::Handshake()
{
    //scope.initScope(tcpConnection->host(), tcpConnection->port());
    return scope.Connect(PortFD);
}

bool NexStarEvo::Disconnect()
{
    scope.Disconnect();
    return INDI::Telescope::Disconnect();
}

const char *NexStarEvo::getDefaultName()
{
    return (char *)"NexStar Evolution";
}

bool NexStarEvo::Park()
{
    // Park at the northern horizon
    // This is a designated by celestron parking position
    Abort();
    scope.Park();
    TrackState = SCOPE_PARKING;
    ParkSP.s   = IPS_BUSY;
    IDSetSwitch(&ParkSP, NULL);
    DEBUG(DBG_NSEVO, "Telescope park in progress...");

    return true;
}

bool NexStarEvo::UnPark()
{
    SetParked(false);
    return true;
}

ln_hrz_posn NexStarEvo::AltAzFromRaDec(double ra, double dec, double ts)
{
    // Call the alignment subsystem to translate the celestial reference frame coordinate
    // into a telescope reference frame coordinate
    TelescopeDirectionVector TDV;
    ln_hrz_posn AltAz;

    if (TransformCelestialToTelescope(ra, dec, ts, TDV))
    {
        // The alignment subsystem has successfully transformed my coordinate
        AltitudeAzimuthFromTelescopeDirectionVector(TDV, AltAz);
    }
    else
    {
        // The alignment subsystem cannot transform the coordinate.
        // Try some simple rotations using the stored observatory position if any
        bool HavePosition = false;
        ln_lnlat_posn Position;
        if ((NULL != IUFindNumber(&LocationNP, "LAT")) && (0 != IUFindNumber(&LocationNP, "LAT")->value) &&
            (NULL != IUFindNumber(&LocationNP, "LONG")) && (0 != IUFindNumber(&LocationNP, "LONG")->value))
        {
            // I assume that being on the equator and exactly on the prime meridian is unlikely
            Position.lat = IUFindNumber(&LocationNP, "LAT")->value;
            Position.lng = IUFindNumber(&LocationNP, "LONG")->value;
            HavePosition = true;
        }
        struct ln_equ_posn EquatorialCoordinates;
        // libnova works in decimal degrees
        EquatorialCoordinates.ra  = ra * 360.0 / 24.0;
        EquatorialCoordinates.dec = dec;
        if (HavePosition)
        {
            ln_get_hrz_from_equ(&EquatorialCoordinates, &Position, ln_get_julian_from_sys() + ts, &AltAz);
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
    }
    return AltAz;
}

double anglediff(double a, double b)
{
    // Signed angle difference
    double d;
    a = fmod(a, 360.0);
    b = fmod(b, 360.0);
    d = fmod(a - b + 360.0, 360.0);
    if (d > 180)
        d = 360.0 - d;
    return std::abs(d) * ((a - b >= 0 && a - b <= 180) || (a - b <= -180 && a - b >= -360) ? 1 : -1);
}

// TODO: Make adjustment for the approx time it takes to slew to the given pos.
bool NexStarEvo::Goto(double ra, double dec)
{
    DEBUGF(DBG_NSEVO, "Goto - Celestial reference frame target RA:%lf(%lf h) Dec:%lf", ra * 360.0 / 24.0, ra, dec);
    if (ISS_ON == IUFindSwitch(&CoordSP, "TRACK")->s)
    {
        char RAStr[32], DecStr[32];
        fs_sexa(RAStr, ra, 2, 3600);
        fs_sexa(DecStr, dec, 2, 3600);
        CurrentTrackingTarget.ra  = ra;
        CurrentTrackingTarget.dec = dec;
        NewTrackingTarget         = CurrentTrackingTarget;
        DEBUG(DBG_NSEVO, "Goto - tracking requested");
    }

    GoToTarget.ra  = ra;
    GoToTarget.dec = dec;

    double timeshift = 0.0;
    if (ScopeStatus != APPROACH)
    {
        // The scope is not in slow approach mode - target should be modified
        // for precission approach. We go to the position from some time ago,
        // to keep the motors going in the same direction as in tracking
        timeshift = 3.0 / (24.0 * 60.0); // Three minutes worth of tracking
    }

    // Call the alignment subsystem to translate the celestial reference frame coordinate
    // into a telescope reference frame coordinate
    TelescopeDirectionVector TDV;
    ln_hrz_posn AltAz;

    AltAz = AltAzFromRaDec(ra, dec, -timeshift);

    // For high Alt azimuth may change very fast.
    // Let us limit azimuth approach to maxApproach degrees
    if (ScopeStatus != APPROACH)
    {
        ln_hrz_posn trgAltAz = AltAzFromRaDec(ra, dec, 0);
        double d;

        d = anglediff(AltAz.az, trgAltAz.az);
        DEBUGF(DBG_NSEVO, "Azimuth approach:  %lf (%lf)", d, Approach);
        AltAz.az = trgAltAz.az + ((d > 0) ? Approach : -Approach);

        d = anglediff(AltAz.alt, trgAltAz.alt);
        DEBUGF(DBG_NSEVO, "Altitude approach:  %lf (%lf)", d, Approach);
        AltAz.alt = trgAltAz.alt + ((d > 0) ? Approach : -Approach);
    }

    // Fold Azimuth into 0-360
    if (AltAz.az < 0)
        AltAz.az += 360.0;
    if (AltAz.az > 360.0)
        AltAz.az -= 360.0;
    // AltAz.az = fmod(AltAz.az, 360.0);

    // Altitude encoder runs -90 to +90 there is no point going outside.
    if (AltAz.alt > 90.0)
        AltAz.alt = 90.0;
    if (AltAz.alt < -90.0)
        AltAz.alt = -90.0;

    DEBUGF(DBG_NSEVO, "Goto: Scope reference frame target altitude %lf azimuth %lf", AltAz.alt, AltAz.az);

    TrackState = SCOPE_SLEWING;
    if (ScopeStatus == APPROACH)
    {
        // We need to make a slow slew to approach the final position
        ScopeStatus = SLEWING_SLOW;
        scope.GoToSlow(long(AltAz.alt * STEPS_PER_DEGREE), long(AltAz.az * STEPS_PER_DEGREE),
                       ISS_ON == IUFindSwitch(&CoordSP, "TRACK")->s);
    }
    else
    {
        // Just make a standard fast slew
        slewTicks   = 0;
        ScopeStatus = SLEWING_FAST;
        scope.GoToFast(long(AltAz.alt * STEPS_PER_DEGREE), long(AltAz.az * STEPS_PER_DEGREE),
                       ISS_ON == IUFindSwitch(&CoordSP, "TRACK")->s);
    }

    EqNP.s = IPS_BUSY;

    return true;
}

bool NexStarEvo::initProperties()
{
    /* Make sure to init parent properties first */
    INDI::Telescope::initProperties();

    IUFillSwitch(&SlewRateS[SLEW_GUIDE], "SLEW_GUIDE", "Guide", ISS_OFF);
    IUFillSwitch(&SlewRateS[SLEW_CENTERING], "SLEW_CENTERING", "Centering", ISS_OFF);
    IUFillSwitch(&SlewRateS[SLEW_FIND], "SLEW_FIND", "Find", ISS_OFF);
    IUFillSwitch(&SlewRateS[SLEW_MAX], "SLEW_MAX", "Max", ISS_ON);
    // Switch to slew rate switch name as defined in telescope_simulator
    // IUFillSwitchVector(&SlewRateSP, SlewRateS, 4, getDeviceName(), "SLEWMODE", "Slew Rate", MOTION_TAB, IP_RW, ISR_1OFMANY, 0, IPS_IDLE);
    IUFillSwitchVector(&SlewRateSP, SlewRateS, 4, getDeviceName(), "TELESCOPE_SLEW_RATE", "Slew Rate", MOTION_TAB,
                       IP_RW, ISR_1OFMANY, 0, IPS_IDLE);
    TrackState = SCOPE_IDLE;

    // We don't want serial port for now
    unRegisterConnection(serialConnection);

    /* Add debug controls so we may debug driver if necessary */
    addDebugControl();

    // Add alignment properties
    InitAlignmentProperties(this);

    return true;
}

bool NexStarEvo::saveConfigItems(FILE *fp)
{
    INDI::Telescope::saveConfigItems(fp);
    SaveAlignmentConfigProperties(fp);
    return true;
}

void NexStarEvo::ISGetProperties(const char *dev)
{
    /* First we let our parent populate */
    INDI::Telescope::ISGetProperties(dev);

    if (isConnected())
    {
    }

    return;
}

bool NexStarEvo::ISNewBLOB(const char *dev, const char *name, int sizes[], int blobsizes[], char *blobs[],
                           char *formats[], char *names[], int n)
{
    if (strcmp(dev, getDeviceName()) == 0)
    {
        // Process alignment properties
        ProcessAlignmentBLOBProperties(this, name, sizes, blobsizes, blobs, formats, names, n);
    }
    // Pass it up the chain
    return INDI::Telescope::ISNewBLOB(dev, name, sizes, blobsizes, blobs, formats, names, n);
}

bool NexStarEvo::ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    //  first check if it's for our device

    if (strcmp(dev, getDeviceName()) == 0)
    {
        // Process alignment properties
        ProcessAlignmentNumberProperties(this, name, values, names, n);
    }

    //  if we didn't process it, continue up the chain, let somebody else
    //  give it a shot
    return INDI::Telescope::ISNewNumber(dev, name, values, names, n);
}

bool NexStarEvo::ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    if (strcmp(dev, getDeviceName()) == 0)
    {
        // Slew mode
        if (!strcmp(name, SlewRateSP.name))
        {
            if (IUUpdateSwitch(&SlewRateSP, states, names, n) < 0)
                return false;

            SlewRateSP.s = IPS_OK;
            IDSetSwitch(&SlewRateSP, NULL);
            return true;
        }

        // Process alignment properties
        ProcessAlignmentSwitchProperties(this, name, states, names, n);
    }

    //  Nobody has claimed this, so, ignore it
    return INDI::Telescope::ISNewSwitch(dev, name, states, names, n);
}

bool NexStarEvo::ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n)
{
    if (strcmp(dev, getDeviceName()) == 0)
    {
        // Process alignment properties
        ProcessAlignmentTextProperties(this, name, texts, names, n);
    }
    // Pass it up the chain
    return INDI::Telescope::ISNewText(dev, name, texts, names, n);
}

bool NexStarEvo::MoveNS(INDI_DIR_NS dir, TelescopeMotionCommand command)
{
    int rate = IUFindOnSwitchIndex(&SlewRateSP);
    DEBUGF(DBG_NSEVO, "MoveNS dir:%d, cmd:%d, rate:%d", dir, command, rate);
    AxisDirectionALT = (dir == DIRECTION_NORTH) ? FORWARD : REVERSE;
    AxisStatusALT    = (command == MOTION_START) ? SLEWING : STOPPED;
    ScopeStatus      = SLEWING_MANUAL;
    TrackState       = SCOPE_SLEWING;
    if (command == MOTION_START)
    {
        switch (rate)
        {
            case SLEW_GUIDE:
                rate = GUIDE_SLEW_RATE;
                break;

            case SLEW_CENTERING:
                rate = CENTERING_SLEW_RATE;
                break;

            case SLEW_FIND:
                rate = FIND_SLEW_RATE;
                break;

            default:
                rate = MAX_SLEW_RATE;
                break;
        }
        return scope.SlewALT(((AxisDirectionALT == FORWARD) ? 1 : -1) * rate);
    }
    else
        return scope.SlewALT(0);
}

bool NexStarEvo::MoveWE(INDI_DIR_WE dir, TelescopeMotionCommand command)
{
    int rate = IUFindOnSwitchIndex(&SlewRateSP);
    DEBUGF(DBG_NSEVO, "MoveWE dir:%d, cmd:%d, rate:%d", dir, command, rate);
    AxisDirectionAZ = (dir == DIRECTION_WEST) ? FORWARD : REVERSE;
    AxisStatusAZ    = (command == MOTION_START) ? SLEWING : STOPPED;
    ScopeStatus     = SLEWING_MANUAL;
    TrackState      = SCOPE_SLEWING;
    if (command == MOTION_START)
    {
        switch (rate)
        {
            case SLEW_GUIDE:
                rate = GUIDE_SLEW_RATE;
                break;

            case SLEW_CENTERING:
                rate = CENTERING_SLEW_RATE;
                break;

            case SLEW_FIND:
                rate = FIND_SLEW_RATE;
                break;

            default:
                rate = MAX_SLEW_RATE;
                break;
        }
        return scope.SlewAZ(((AxisDirectionAZ == FORWARD) ? -1 : 1) * rate);
    }
    else
        return scope.SlewAZ(0);
}

bool NexStarEvo::trackingRequested()
{
    return (ISS_ON == IUFindSwitch(&CoordSP, "TRACK")->s);
}

bool NexStarEvo::ReadScopeStatus()
{
    struct ln_hrz_posn AltAz;
    double RightAscension, Declination;

    AltAz.alt = double(scope.GetALT()) / STEPS_PER_DEGREE;
    // libnova indexes Az from south while Celestron controllers index from north
    // Never mix two controllers/drivers they will never agree perfectly.
    // Furthermore the celestron hand controler resets the position encoders
    // on alignment and this will mess-up all orientation in the driver.
    // Here we are not attempting to make the driver agree with the hand
    // controller (That would involve adding 180deg here to the azimuth -
    // this way the celestron nexstar driver and this would agree in some
    // situations but not in other - better not to attepmpt impossible!).
    AltAz.az                     = double(scope.GetAZ()) / STEPS_PER_DEGREE;
    TelescopeDirectionVector TDV = TelescopeDirectionVectorFromAltitudeAzimuth(AltAz);

    if (TraceThisTick)
        DEBUGF(DBG_NSEVO, "ReadScopeStatus - Alt %lf deg ; Az %lf deg", AltAz.alt, AltAz.az);

    if (!TransformTelescopeToCelestial(TDV, RightAscension, Declination))
    {
        if (TraceThisTick)
            DEBUG(DBG_NSEVO, "ReadScopeStatus - TransformTelescopeToCelestial failed");

        bool HavePosition = false;
        ln_lnlat_posn Position;
        if ((NULL != IUFindNumber(&LocationNP, "LAT")) && (0 != IUFindNumber(&LocationNP, "LAT")->value) &&
            (NULL != IUFindNumber(&LocationNP, "LONG")) && (0 != IUFindNumber(&LocationNP, "LONG")->value))
        {
            // I assume that being on the equator and exactly on the prime meridian is unlikely
            Position.lat = IUFindNumber(&LocationNP, "LAT")->value;
            Position.lng = IUFindNumber(&LocationNP, "LONG")->value;
            HavePosition = true;
        }
        struct ln_equ_posn EquatorialCoordinates;
        if (HavePosition)
        {
            if (TraceThisTick)
                DEBUG(DBG_NSEVO, "ReadScopeStatus - HavePosition true");
            TelescopeDirectionVector RotatedTDV(TDV);
            switch (GetApproximateMountAlignment())
            {
                case ZENITH:
                    if (TraceThisTick)
                        DEBUG(DBG_NSEVO, "ReadScopeStatus - ApproximateMountAlignment ZENITH");
                    break;

                case NORTH_CELESTIAL_POLE:
                    if (TraceThisTick)
                        DEBUG(DBG_NSEVO, "ReadScopeStatus - ApproximateMountAlignment NORTH_CELESTIAL_POLE");
                    // Rotate the TDV coordinate system anticlockwise (positive) around the y axis by 90 minus
                    // the (positive)observatory latitude. The vector itself is rotated clockwise
                    RotatedTDV.RotateAroundY(90.0 - Position.lat);
                    AltitudeAzimuthFromTelescopeDirectionVector(RotatedTDV, AltAz);
                    break;

                case SOUTH_CELESTIAL_POLE:
                    if (TraceThisTick)
                        DEBUG(DBG_NSEVO, "ReadScopeStatus - ApproximateMountAlignment SOUTH_CELESTIAL_POLE");
                    // Rotate the TDV coordinate system clockwise (negative) around the y axis by 90 plus
                    // the (negative)observatory latitude. The vector itself is rotated anticlockwise
                    RotatedTDV.RotateAroundY(-90.0 - Position.lat);
                    AltitudeAzimuthFromTelescopeDirectionVector(RotatedTDV, AltAz);
                    break;
            }
            if (TraceThisTick)
                DEBUGF(DBG_NSEVO, "After rotations: Alt %lf deg ; Az %lf deg", AltAz.alt, AltAz.az);

            ln_get_equ_from_hrz(&AltAz, &Position, ln_get_julian_from_sys(), &EquatorialCoordinates);
        }
        else
        {
            if (TraceThisTick)
                DEBUG(DBG_NSEVO, "ReadScopeStatus - HavePosition false");

            // The best I can do is just do a direct conversion to RA/DEC
            EquatorialCoordinatesFromTelescopeDirectionVector(TDV, EquatorialCoordinates);
        }
        // libnova works in decimal degrees
        RightAscension = EquatorialCoordinates.ra * 24.0 / 360.0;
        Declination    = EquatorialCoordinates.dec;
    }

    if (TraceThisTick)
        DEBUGF(DBG_NSEVO, "ReadScopeStatus - RA %lf hours DEC %lf degrees", RightAscension, Declination);

    // In case we are slewing while tracking update the potential target
    NewTrackingTarget.ra  = RightAscension;
    NewTrackingTarget.dec = Declination;
    NewRaDec(RightAscension, Declination);

    return true;
}

bool NexStarEvo::Sync(double ra, double dec)
{
    struct ln_hrz_posn AltAz;
    AltAz.alt = double(scope.GetALT()) / STEPS_PER_DEGREE;
    AltAz.az  = double(scope.GetAZ()) / STEPS_PER_DEGREE;

    AlignmentDatabaseEntry NewEntry;
    NewEntry.ObservationJulianDate = ln_get_julian_from_sys();
    NewEntry.RightAscension        = ra;
    NewEntry.Declination           = dec;
    NewEntry.TelescopeDirection    = TelescopeDirectionVectorFromAltitudeAzimuth(AltAz);
    NewEntry.PrivateDataSize       = 0;

    DEBUGF(DBG_NSEVO, "Sync - Celestial reference frame target right ascension %lf(%lf) declination %lf",
           ra * 360.0 / 24.0, ra, dec);

    if (!CheckForDuplicateSyncPoint(NewEntry))
    {
        GetAlignmentDatabase().push_back(NewEntry);

        // Tell the client about size change
        UpdateSize();

        // Tell the math plugin to reinitialise
        Initialise(this);
        DEBUGF(DBG_NSEVO, "Sync - new entry added RA: %lf(%lf) DEC: %lf", ra * 360.0 / 24.0, ra, dec);
        ReadScopeStatus();
        return true;
    }
    DEBUGF(DBG_NSEVO, "Sync - duplicate entry RA: %lf(%lf) DEC: %lf", ra * 360.0 / 24.0, ra, dec);
    return false;
}

void NexStarEvo::TimerHit()
{
    TraceThisTickCount++;
    if (60 == TraceThisTickCount)
    {
        TraceThisTick      = true;
        TraceThisTickCount = 0;
    }
    // Simulate mount movement

    static struct timeval ltv; // previous system time
    struct timeval tv;         // new system time
    double dt;                 // Elapsed time in seconds since last tick

    gettimeofday(&tv, NULL);

    if (ltv.tv_sec == 0 && ltv.tv_usec == 0)
        ltv = tv;

    dt  = tv.tv_sec - ltv.tv_sec + (tv.tv_usec - ltv.tv_usec) / 1e6;
    ltv = tv;

    scope.TimerTick(dt);

    INDI::Telescope::TimerHit(); // This will call ReadScopeStatus

    // OK I have updated the celestial reference frame RA/DEC in ReadScopeStatus
    // Now handle the tracking state
    switch (TrackState)
    {
        case SCOPE_PARKING:
            if (!scope.slewing())
            {
                SetParked(true);
                DEBUG(DBG_NSEVO, "Telescope parked.");
            }
            break;

        case SCOPE_SLEWING:
            if (scope.slewing())
            {
                // The scope is still slewing
                slewTicks++;
                if ((ScopeStatus == SLEWING_FAST) && (slewTicks > maxSlewTicks))
                {
                    // Slewing too long, reissue GoTo to update target position
                    Goto(GoToTarget.ra, GoToTarget.dec);
                    slewTicks = 0;
                }
                break;
            }
            else
            {
                // The slew has finished check if that was a coarse slew
                // or precission approach
                if (ScopeStatus == SLEWING_FAST)
                {
                    // This was coarse slew. Execute precise approach.
                    ScopeStatus = APPROACH;
                    Goto(GoToTarget.ra, GoToTarget.dec);
                    break;
                }
                else if (trackingRequested())
                {
                    // Precise Goto or manual slew has finished.
                    // Start tracking if requested.
                    if (ScopeStatus == SLEWING_MANUAL)
                    {
                        // We have been slewing manually.
                        // Update the tracking target.
                        CurrentTrackingTarget = NewTrackingTarget;
                    }
                    DEBUGF(DBG_NSEVO, "Goto finished start tracking TargetRA: %f TargetDEC: %f",
                           CurrentTrackingTarget.ra, CurrentTrackingTarget.dec);
                    TrackState = SCOPE_TRACKING;
                    // Fall through to tracking case
                }
                else
                {
                    DEBUG(DBG_NSEVO, "Goto finished. No tracking requested");
                    // Precise goto or manual slew finished.
                    // No tracking requested -> go idle.
                    TrackState = SCOPE_IDLE;
                    break;
                }
            }
            break;

        case SCOPE_TRACKING:
        {
            // Continue or start tracking
            // Calculate where the mount needs to be in a minute
            double JulianOffset = 60.0 / (24.0 * 60 * 60);
            TelescopeDirectionVector TDV;
            ln_hrz_posn AltAz, AAzero;

            AltAz  = AltAzFromRaDec(CurrentTrackingTarget.ra, CurrentTrackingTarget.dec, JulianOffset);
            AAzero = AltAzFromRaDec(CurrentTrackingTarget.ra, CurrentTrackingTarget.dec, 0);
            if (TraceThisTick)
                DEBUGF(DBG_NSEVO, "Tracking - Calculated Alt %lf deg ; Az %lf deg", AltAz.alt, AltAz.az);
            /* 
            TODO 
            The tracking should take into account movement of the scope
            by the hand controller (we can hear the commands)
            and the movements made by the joystick.
            Right now when we move the scope by HC it returns to the
            designated target by corrective tracking.
            */

            // Fold Azimuth into 0-360
            if (AltAz.az < 0)
                AltAz.az += 360.0;
            if (AltAz.az > 360.0)
                AltAz.az -= 360.0;
            //AltAz.az = fmod(AltAz.az, 360.0);

            {
                long altRate, azRate;

                // This is in steps per minute
                altRate = long(AltAz.alt * STEPS_PER_DEGREE - scope.GetALT());
                azRate  = long(AltAz.az * STEPS_PER_DEGREE - scope.GetAZ());

                if (TraceThisTick)
                    DEBUGF(DBG_NSEVO, "Target (AltAz): %f  %f  Scope  (AltAz)  %f  %f", AltAz.alt, AltAz.az,
                           scope.GetALT() / STEPS_PER_DEGREE, scope.GetAZ() / STEPS_PER_DEGREE);

                if (std::abs(azRate) > STEPS_PER_REVOLUTION / 2)
                {
                    // Crossing the meridian. AZ skips from 350+ to 0+
                    // Correct for wrap-around
                    azRate += STEPS_PER_REVOLUTION;
                    if (azRate > STEPS_PER_REVOLUTION)
                        azRate %= STEPS_PER_REVOLUTION;
                }

                // Track function needs rates in 1000*arcmin/minute
                // Rates here are in steps/minute
                // conv. factor: TRACK_SCALE = 60000/STEPS_PER_DEGREE
                altRate = long(TRACK_SCALE * altRate);
                azRate  = long(TRACK_SCALE * azRate);
                scope.Track(altRate, azRate);

                if (TraceThisTick)
                    DEBUGF(DBG_NSEVO, "TimerHit - Tracking AltRate %d AzRate %d ; Pos diff (deg): Alt: %f Az: %f",
                           altRate, azRate, AltAz.alt - AAzero.alt, AltAz.az - AAzero.az);
            }
            break;
        }

        default:
            break;
    }

    TraceThisTick = false;
}

bool NexStarEvo::updateLocation(double latitude, double longitude, double elevation)
{
    UpdateLocation(latitude, longitude, elevation);
    scope.UpdateLocation(latitude, longitude, elevation);
    return true;
}

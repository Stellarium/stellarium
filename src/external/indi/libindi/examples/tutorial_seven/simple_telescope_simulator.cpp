/*
   INDI Developers Manual
   Tutorial #7

   "Simple telescope simulator"

   We construct a most basic (and useless) device driver to illustrate INDI.

   Refer to README, which contains instruction on how to build this driver, and use it
   with an INDI-compatible client.

*/

#include "simple_telescope_simulator.h"

#include "indicom.h"

#include <memory>

using namespace INDI::AlignmentSubsystem;

#define POLLMS 1000 // Default timer tick

// We declare an auto pointer to ScopeSim.
std::unique_ptr<ScopeSim> telescope_sim(new ScopeSim());

void ISGetProperties(const char *dev)
{
    telescope_sim->ISGetProperties(dev);
}

void ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    telescope_sim->ISNewSwitch(dev, name, states, names, n);
}

void ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n)
{
    telescope_sim->ISNewText(dev, name, texts, names, n);
}

void ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    telescope_sim->ISNewNumber(dev, name, values, names, n);
}

void ISNewBLOB(const char *dev, const char *name, int sizes[], int blobsizes[], char *blobs[], char *formats[],
               char *names[], int n)
{
    telescope_sim->ISNewBLOB(dev, name, sizes, blobsizes, blobs, formats, names, n);
}

void ISSnoopDevice(XMLEle *root)
{
    INDI_UNUSED(root);
}

ScopeSim::ScopeSim() :
    DBG_SIMULATOR(INDI::Logger::getInstance().addDebugLevel("Simulator Verbose", "SIMULATOR"))
{
}

bool ScopeSim::Abort()
{
    if (MovementNSSP.s == IPS_BUSY)
    {
        IUResetSwitch(&MovementNSSP);
        MovementNSSP.s = IPS_IDLE;
        IDSetSwitch(&MovementNSSP, nullptr);
    }

    if (MovementWESP.s == IPS_BUSY)
    {
        MovementWESP.s = IPS_IDLE;
        IUResetSwitch(&MovementWESP);
        IDSetSwitch(&MovementWESP, nullptr);
    }

    if (EqNP.s == IPS_BUSY)
    {
        EqNP.s = IPS_IDLE;
        IDSetNumber(&EqNP, nullptr);
    }

    TrackState = SCOPE_IDLE;

    AxisStatusRA = AxisStatusDEC = STOPPED; // This marvelous inertia free scope can be stopped instantly!

    AbortSP.s = IPS_OK;
    IUResetSwitch(&AbortSP);
    IDSetSwitch(&AbortSP, nullptr);
    DEBUG(INDI::Logger::DBG_SESSION, "Telescope aborted.");

    return true;
}

bool ScopeSim::canSync()
{
    return true;
}

bool ScopeSim::Connect()
{
    SetTimer(POLLMS);
    return true;
}

bool ScopeSim::Disconnect()
{
    return true;
}

const char *ScopeSim::getDefaultName()
{
    return (const char *)"Simple Telescope Simulator";
}

bool ScopeSim::Goto(double ra, double dec)
{
    DEBUGF(DBG_SIMULATOR, "Goto - Celestial reference frame target right ascension %lf(%lf) declination %lf",
           ra * 360.0 / 24.0, ra, dec);

    if (ISS_ON == IUFindSwitch(&CoordSP, "TRACK")->s)
    {
        char RAStr[32], DecStr[32];
        fs_sexa(RAStr, ra, 2, 3600);
        fs_sexa(DecStr, dec, 2, 3600);
        CurrentTrackingTarget.ra  = ra;
        CurrentTrackingTarget.dec = dec;
        DEBUG(DBG_SIMULATOR, "Goto - tracking requested");
    }

    // Call the alignment subsystem to translate the celestial reference frame coordinate
    // into a telescope reference frame coordinate
    TelescopeDirectionVector TDV;
    ln_hrz_posn AltAz { 0, 0 };

    if (TransformCelestialToTelescope(ra, dec, 0.0, TDV))
    {
        // The alignment subsystem has successfully transformed my coordinate
        AltitudeAzimuthFromTelescopeDirectionVector(TDV, AltAz);
    }
    else
    {
        // The alignment subsystem cannot transform the coordinate.
        // Try some simple rotations using the stored observatory position if any
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
            ln_get_hrz_from_equ(&EquatorialCoordinates, &Position, ln_get_julian_from_sys(), &AltAz);
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

    // My altitude encoder runs -90 to +90
    if ((AltAz.alt > 90.0) || (AltAz.alt < -90.0))
    {
        DEBUG(DBG_SIMULATOR, "Goto - Altitude out of range");
        // This should not happen
        return false;
    }

    // My polar encoder runs 0 to +360
    if ((AltAz.az > 360.0) || (AltAz.az < -360.0))
    {
        DEBUG(DBG_SIMULATOR, "Goto - Azimuth out of range");
        // This should not happen
        return false;
    }

    if (AltAz.az < 0.0)
    {
        DEBUG(DBG_SIMULATOR, "Goto - Azimuth negative");
        AltAz.az = 360.0 + AltAz.az;
    }

    DEBUGF(DBG_SIMULATOR, "Goto - Scope reference frame target altitude %lf azimuth %lf", AltAz.alt, AltAz.az);

    GotoTargetMicrostepsDEC = int(AltAz.alt * MICROSTEPS_PER_DEGREE);
    if (GotoTargetMicrostepsDEC == CurrentEncoderMicrostepsDEC)
        AxisStatusDEC = STOPPED;
    else
    {
        if (GotoTargetMicrostepsDEC > CurrentEncoderMicrostepsDEC)
            AxisDirectionDEC = FORWARD;
        else
            AxisDirectionDEC = REVERSE;
        AxisStatusDEC = SLEWING_TO;
    }
    GotoTargetMicrostepsRA = int(AltAz.az * MICROSTEPS_PER_DEGREE);
    if (GotoTargetMicrostepsRA == CurrentEncoderMicrostepsRA)
        AxisStatusRA = STOPPED;
    else
    {
        if (GotoTargetMicrostepsRA > CurrentEncoderMicrostepsRA)
            AxisDirectionRA = (GotoTargetMicrostepsRA - CurrentEncoderMicrostepsRA) < MICROSTEPS_PER_REVOLUTION / 2.0 ?
                                  FORWARD :
                                  REVERSE;
        else
            AxisDirectionRA = (CurrentEncoderMicrostepsRA - GotoTargetMicrostepsRA) < MICROSTEPS_PER_REVOLUTION / 2.0 ?
                                  REVERSE :
                                  FORWARD;
        AxisStatusRA = SLEWING_TO;
    }

    TrackState = SCOPE_SLEWING;

    EqNP.s = IPS_BUSY;

    return true;
}

bool ScopeSim::initProperties()
{
    /* Make sure to init parent properties first */
    INDI::Telescope::initProperties();

    // Let's simulate it to be an F/10 8" telescope
    ScopeParametersN[0].value = 203;
    ScopeParametersN[1].value = 2000;
    ScopeParametersN[2].value = 203;
    ScopeParametersN[3].value = 2000;

    TrackState = SCOPE_IDLE;

    /* Add debug controls so we may debug driver if necessary */
    addDebugControl();

    // Add alignment properties
    InitAlignmentProperties(this);

    return true;
}

bool ScopeSim::ISNewBLOB(const char *dev, const char *name, int sizes[], int blobsizes[], char *blobs[],
                         char *formats[], char *names[], int n)
{
    if (dev != nullptr && strcmp(dev, getDeviceName()) == 0)
    {
        // Process alignment properties
        ProcessAlignmentBLOBProperties(this, name, sizes, blobsizes, blobs, formats, names, n);
    }
    // Pass it up the chain
    return INDI::Telescope::ISNewBLOB(dev, name, sizes, blobsizes, blobs, formats, names, n);
}

bool ScopeSim::ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    //  first check if it's for our device

    if (dev != nullptr && strcmp(dev, getDeviceName()) == 0)
    {
        // Process alignment properties
        ProcessAlignmentNumberProperties(this, name, values, names, n);
    }

    //  if we didn't process it, continue up the chain, let somebody else
    //  give it a shot
    return INDI::Telescope::ISNewNumber(dev, name, values, names, n);
}

bool ScopeSim::ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    if (dev != nullptr && strcmp(dev, getDeviceName()) == 0)
    {
        // Process alignment properties
        ProcessAlignmentSwitchProperties(this, name, states, names, n);
    }

    //  Nobody has claimed this, so, ignore it
    return INDI::Telescope::ISNewSwitch(dev, name, states, names, n);
}

bool ScopeSim::ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n)
{
    if (dev != nullptr && strcmp(dev, getDeviceName()) == 0)
    {
        // Process alignment properties
        ProcessAlignmentTextProperties(this, name, texts, names, n);
    }
    // Pass it up the chain
    return INDI::Telescope::ISNewText(dev, name, texts, names, n);
}

bool ScopeSim::MoveNS(INDI_DIR_NS dir, TelescopeMotionCommand command)
{
    AxisDirection axisDir = (dir == DIRECTION_NORTH) ? FORWARD : REVERSE;
    AxisStatus axisStat   = (command == MOTION_START) ? SLEWING : STOPPED;

    AxisSlewRateDEC  = DEFAULT_SLEW_RATE;
    AxisDirectionDEC = axisDir;
    AxisStatusDEC    = axisStat;

    return true;
}

bool ScopeSim::MoveWE(INDI_DIR_WE dir, TelescopeMotionCommand command)
{
    AxisDirection axisDir = (dir == DIRECTION_WEST) ? FORWARD : REVERSE;
    AxisStatus axisStat   = (command == MOTION_START) ? SLEWING : STOPPED;

    AxisSlewRateRA  = DEFAULT_SLEW_RATE;
    AxisDirectionRA = axisDir;
    AxisStatusRA    = axisStat;

    return true;
}

bool ScopeSim::ReadScopeStatus()
{
    ln_hrz_posn AltAz { 0, 0 };

    AltAz.alt                    = double(CurrentEncoderMicrostepsDEC) / MICROSTEPS_PER_DEGREE;
    AltAz.az                     = double(CurrentEncoderMicrostepsRA) / MICROSTEPS_PER_DEGREE;

    TelescopeDirectionVector TDV = TelescopeDirectionVectorFromAltitudeAzimuth(AltAz);
    double RightAscension, Declination;

    if (!TransformTelescopeToCelestial(TDV, RightAscension, Declination))
    {
        if (TraceThisTick)
            DEBUG(DBG_SIMULATOR, "ReadScopeStatus - TransformTelescopeToCelestial failed");

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
            if (TraceThisTick)
                DEBUG(DBG_SIMULATOR, "ReadScopeStatus - HavePosition true");

            TelescopeDirectionVector RotatedTDV(TDV);

            switch (GetApproximateMountAlignment())
            {
                case ZENITH:
                    if (TraceThisTick)
                        DEBUG(DBG_SIMULATOR, "ReadScopeStatus - ApproximateMountAlignment ZENITH");
                    break;

                case NORTH_CELESTIAL_POLE:
                    if (TraceThisTick)
                        DEBUG(DBG_SIMULATOR, "ReadScopeStatus - ApproximateMountAlignment NORTH_CELESTIAL_POLE");
                    // Rotate the TDV coordinate system anticlockwise (positive) around the y axis by 90 minus
                    // the (positive)observatory latitude. The vector itself is rotated clockwise
                    RotatedTDV.RotateAroundY(90.0 - Position.lat);
                    AltitudeAzimuthFromTelescopeDirectionVector(RotatedTDV, AltAz);
                    break;

                case SOUTH_CELESTIAL_POLE:
                    if (TraceThisTick)
                        DEBUG(DBG_SIMULATOR, "ReadScopeStatus - ApproximateMountAlignment SOUTH_CELESTIAL_POLE");
                    // Rotate the TDV coordinate system clockwise (negative) around the y axis by 90 plus
                    // the (negative)observatory latitude. The vector itself is rotated anticlockwise
                    RotatedTDV.RotateAroundY(-90.0 - Position.lat);
                    AltitudeAzimuthFromTelescopeDirectionVector(RotatedTDV, AltAz);
                    break;
            }
            ln_get_equ_from_hrz(&AltAz, &Position, ln_get_julian_from_sys(), &EquatorialCoordinates);
        }
        else
        {
            if (TraceThisTick)
                DEBUG(DBG_SIMULATOR, "ReadScopeStatus - HavePosition false");

            // The best I can do is just do a direct conversion to RA/DEC
            EquatorialCoordinatesFromTelescopeDirectionVector(TDV, EquatorialCoordinates);
        }
        // libnova works in decimal degrees
        RightAscension = EquatorialCoordinates.ra * 24.0 / 360.0;
        Declination    = EquatorialCoordinates.dec;
    }

    if (TraceThisTick)
        DEBUGF(DBG_SIMULATOR, "ReadScopeStatus - RA %lf hours DEC %lf degrees", RightAscension, Declination);

    NewRaDec(RightAscension, Declination);

    return true;
}

bool ScopeSim::Sync(double ra, double dec)
{
    ln_hrz_posn AltAz { 0, 0 };
    AlignmentDatabaseEntry NewEntry;

    AltAz.alt = double(CurrentEncoderMicrostepsDEC) / MICROSTEPS_PER_DEGREE;
    AltAz.az  = double(CurrentEncoderMicrostepsRA) / MICROSTEPS_PER_DEGREE;

    NewEntry.ObservationJulianDate = ln_get_julian_from_sys();
    NewEntry.RightAscension        = ra;
    NewEntry.Declination           = dec;
    NewEntry.TelescopeDirection    = TelescopeDirectionVectorFromAltitudeAzimuth(AltAz);
    NewEntry.PrivateDataSize       = 0;

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

void ScopeSim::TimerHit()
{
    TraceThisTickCount++;
    if (60 == TraceThisTickCount)
    {
        TraceThisTick      = true;
        TraceThisTickCount = 0;
    }
    // Simulate mount movement

    static struct timeval ltv { 0, 0 }; // previous system time
    struct timeval tv { 0, 0 };         // new system time
    double dt;                 // Elapsed time in seconds since last tick

    gettimeofday(&tv, nullptr);

    if (ltv.tv_sec == 0 && ltv.tv_usec == 0)
        ltv = tv;

    dt  = tv.tv_sec - ltv.tv_sec + (tv.tv_usec - ltv.tv_usec) / 1e6;
    ltv = tv;

    // RA axis
    long SlewSteps          = dt * AxisSlewRateRA;
    bool CompleteRevolution = SlewSteps >= MICROSTEPS_PER_REVOLUTION;
    SlewSteps               = SlewSteps % MICROSTEPS_PER_REVOLUTION; // Just in case ;-)

    switch (AxisStatusRA)
    {
        case STOPPED:
            // Do nothing
            break;

        case SLEWING:
        {
            DEBUGF(DBG_SIMULATOR,
                   "TimerHit Slewing - RA Current Encoder %ld SlewSteps %ld Direction %d Target %ld Status %d",
                   CurrentEncoderMicrostepsRA, SlewSteps, AxisDirectionRA, GotoTargetMicrostepsRA, AxisStatusRA);

            // Update the encoder
            if (FORWARD == AxisDirectionRA)
                CurrentEncoderMicrostepsRA += SlewSteps;
            else
                CurrentEncoderMicrostepsRA -= SlewSteps;
            if (CurrentEncoderMicrostepsRA < 0)
                CurrentEncoderMicrostepsRA += MICROSTEPS_PER_REVOLUTION;
            else if (CurrentEncoderMicrostepsRA >= MICROSTEPS_PER_REVOLUTION)
                CurrentEncoderMicrostepsRA -= MICROSTEPS_PER_REVOLUTION;

            DEBUGF(DBG_SIMULATOR, "TimerHit Slewing - RA New Encoder %d New Status %d", CurrentEncoderMicrostepsRA,
                   AxisStatusRA);
            break;
        }

        case SLEWING_TO:
        {
            DEBUGF(DBG_SIMULATOR,
                   "TimerHit SlewingTo - RA Current Encoder %ld SlewSteps %ld Direction %d Target %ld Status %d",
                   CurrentEncoderMicrostepsRA, SlewSteps, AxisDirectionRA, GotoTargetMicrostepsRA, AxisStatusRA);

            long OldEncoder = CurrentEncoderMicrostepsRA;
            // Update the encoder
            if (FORWARD == AxisDirectionRA)
                CurrentEncoderMicrostepsRA += SlewSteps;
            else
                CurrentEncoderMicrostepsRA -= SlewSteps;
            if (CurrentEncoderMicrostepsRA < 0)
                CurrentEncoderMicrostepsRA += MICROSTEPS_PER_REVOLUTION;
            else if (CurrentEncoderMicrostepsRA >= MICROSTEPS_PER_REVOLUTION)
                CurrentEncoderMicrostepsRA -= MICROSTEPS_PER_REVOLUTION;

            if (CompleteRevolution)
            {
                // Must have found the target
                AxisStatusRA               = STOPPED;
                CurrentEncoderMicrostepsRA = GotoTargetMicrostepsRA;
            }
            else
            {
                bool FoundTarget = false;
                if (FORWARD == AxisDirectionRA)
                {
                    if (CurrentEncoderMicrostepsRA < OldEncoder)
                    {
                        // Two ranges to search
                        if ((GotoTargetMicrostepsRA >= OldEncoder) &&
                            (GotoTargetMicrostepsRA <= MICROSTEPS_PER_REVOLUTION))
                            FoundTarget = true;
                        else if ((GotoTargetMicrostepsRA >= 0) &&
                                 (GotoTargetMicrostepsRA <= CurrentEncoderMicrostepsRA))
                            FoundTarget = true;
                    }
                    else if ((GotoTargetMicrostepsRA >= OldEncoder) &&
                             (GotoTargetMicrostepsRA <= CurrentEncoderMicrostepsRA))
                        FoundTarget = true;
                }
                else
                {
                    if (CurrentEncoderMicrostepsRA > OldEncoder)
                    {
                        // Two ranges to search
                        if ((GotoTargetMicrostepsRA >= 0) && (GotoTargetMicrostepsRA <= OldEncoder))
                            FoundTarget = true;
                        else if ((GotoTargetMicrostepsRA >= CurrentEncoderMicrostepsRA) &&
                                 (GotoTargetMicrostepsRA <= MICROSTEPS_PER_REVOLUTION))
                            FoundTarget = true;
                    }
                    else if ((GotoTargetMicrostepsRA >= CurrentEncoderMicrostepsRA) &&
                             (GotoTargetMicrostepsRA <= OldEncoder))
                        FoundTarget = true;
                }
                if (FoundTarget)
                {
                    AxisStatusRA               = STOPPED;
                    CurrentEncoderMicrostepsRA = GotoTargetMicrostepsRA;
                }
            }
            DEBUGF(DBG_SIMULATOR, "TimerHit SlewingTo - RA New Encoder %d New Status %d", CurrentEncoderMicrostepsRA,
                   AxisStatusRA);
            break;
        }
    }

    // DEC axis
    SlewSteps = dt * AxisSlewRateDEC;

    switch (AxisStatusDEC)
    {
        case STOPPED:
            // Do nothing
            break;

        case SLEWING:
        {
            DEBUGF(DBG_SIMULATOR,
                   "TimerHit Slewing - DEC Current Encoder %ld SlewSteps %d Direction %ld Target %ld Status %d",
                   CurrentEncoderMicrostepsDEC, SlewSteps, AxisDirectionDEC, GotoTargetMicrostepsDEC, AxisStatusDEC);

            // Update the encoder
            SlewSteps = SlewSteps % MICROSTEPS_PER_REVOLUTION; // Just in case ;-)
            if (FORWARD == AxisDirectionDEC)
                CurrentEncoderMicrostepsDEC += SlewSteps;
            else
                CurrentEncoderMicrostepsDEC -= SlewSteps;
            if (CurrentEncoderMicrostepsDEC > MAX_DEC)
            {
                CurrentEncoderMicrostepsDEC = MAX_DEC;
                AxisStatusDEC               = STOPPED; // Hit the buffers
                DEBUG(DBG_SIMULATOR, "TimerHit - DEC axis hit the buffers at MAX_DEC");
            }
            else if (CurrentEncoderMicrostepsDEC < MIN_DEC)
            {
                CurrentEncoderMicrostepsDEC = MIN_DEC;
                AxisStatusDEC               = STOPPED; // Hit the buffers
                DEBUG(DBG_SIMULATOR, "TimerHit - DEC axis hit the buffers at MIN_DEC");
            }

            DEBUGF(DBG_SIMULATOR, "TimerHit Slewing - DEC New Encoder %d New Status %d", CurrentEncoderMicrostepsDEC,
                   AxisStatusDEC);
            break;
        }

        case SLEWING_TO:
        {
            DEBUGF(DBG_SIMULATOR,
                   "TimerHit SlewingTo - DEC Current Encoder %ld SlewSteps %d Direction %ld Target %ld Status %d",
                   CurrentEncoderMicrostepsDEC, SlewSteps, AxisDirectionDEC, GotoTargetMicrostepsDEC, AxisStatusDEC);

            // Calculate steps to target
            int StepsToTarget;
            if (FORWARD == AxisDirectionDEC)
            {
                if (CurrentEncoderMicrostepsDEC <= GotoTargetMicrostepsDEC)
                    StepsToTarget = GotoTargetMicrostepsDEC - CurrentEncoderMicrostepsDEC;
                else
                    StepsToTarget = CurrentEncoderMicrostepsDEC - GotoTargetMicrostepsDEC;
            }
            else
            {
                // Axis in reverse
                if (CurrentEncoderMicrostepsDEC >= GotoTargetMicrostepsDEC)
                    StepsToTarget = CurrentEncoderMicrostepsDEC - GotoTargetMicrostepsDEC;
                else
                    StepsToTarget = GotoTargetMicrostepsDEC - CurrentEncoderMicrostepsDEC;
            }
            if (StepsToTarget <= SlewSteps)
            {
                // Target was hit this tick
                AxisStatusDEC               = STOPPED;
                CurrentEncoderMicrostepsDEC = GotoTargetMicrostepsDEC;
            }
            else
            {
                if (FORWARD == AxisDirectionDEC)
                    CurrentEncoderMicrostepsDEC += SlewSteps;
                else
                    CurrentEncoderMicrostepsDEC -= SlewSteps;
                if (CurrentEncoderMicrostepsDEC < 0)
                    CurrentEncoderMicrostepsDEC += MICROSTEPS_PER_REVOLUTION;
                else if (CurrentEncoderMicrostepsDEC >= MICROSTEPS_PER_REVOLUTION)
                    CurrentEncoderMicrostepsDEC -= MICROSTEPS_PER_REVOLUTION;
            }

            DEBUGF(DBG_SIMULATOR, "TimerHit SlewingTo - DEC New Encoder %d New Status %d", CurrentEncoderMicrostepsDEC,
                   AxisStatusDEC);
            break;
        }
    }

    INDI::Telescope::TimerHit(); // This will call ReadScopeStatus

    // OK I have updated the celestial reference frame RA/DEC in ReadScopeStatus
    // Now handle the tracking state
    switch (TrackState)
    {
        case SCOPE_SLEWING:
            if ((STOPPED == AxisStatusRA) && (STOPPED == AxisStatusDEC))
            {
                if (ISS_ON == IUFindSwitch(&CoordSP, "TRACK")->s)
                {
                    // Goto has finished start tracking
                    DEBUG(DBG_SIMULATOR, "TimerHit - Goto finished start tracking");
                    TrackState = SCOPE_TRACKING;
                    // Fall through to tracking case
                }
                else
                {
                    TrackState = SCOPE_IDLE;
                    break;
                }
            }
            else
                break;

        case SCOPE_TRACKING:
        {
            // Continue or start tracking
            // Calculate where the mount needs to be in POLLMS time
            // POLLMS is hardcoded to be one second
            // TODO may need to make this longer to get a meaningful result
            double JulianOffset = 1.0 / (24.0 * 60 * 60);
            TelescopeDirectionVector TDV;
            ln_hrz_posn AltAz { 0, 0 };

            if (TransformCelestialToTelescope(CurrentTrackingTarget.ra, CurrentTrackingTarget.dec, JulianOffset, TDV))
                AltitudeAzimuthFromTelescopeDirectionVector(TDV, AltAz);
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
                    ln_get_hrz_from_equ(&EquatorialCoordinates, &Position, ln_get_julian_from_sys() + JulianOffset,
                                        &AltAz);
                else
                {
                    // No sense in tracking in this case
                    TrackState = SCOPE_IDLE;
                    break;
                }
            }

            // My altitude encoder runs -90 to +90
            if ((AltAz.alt > 90.0) || (AltAz.alt < -90.0))
            {
                DEBUG(DBG_SIMULATOR, "TimerHit tracking - Altitude out of range");
                // This should not happen
                return;
            }

            // My polar encoder runs 0 to +360
            if ((AltAz.az > 360.0) || (AltAz.az < -360.0))
            {
                DEBUG(DBG_SIMULATOR, "TimerHit tracking - Azimuth out of range");
                // This should not happen
                return;
            }

            if (AltAz.az < 0.0)
            {
                DEBUG(DBG_SIMULATOR, "TimerHit tracking - Azimuth negative");
                AltAz.az = 360.0 + AltAz.az;
            }

            long AltitudeOffsetMicrosteps = int(AltAz.alt * MICROSTEPS_PER_DEGREE - CurrentEncoderMicrostepsDEC);
            long AzimuthOffsetMicrosteps  = int(AltAz.az * MICROSTEPS_PER_DEGREE - CurrentEncoderMicrostepsRA);

            DEBUGF(DBG_SIMULATOR, "TimerHit - Tracking AltitudeOffsetMicrosteps %d AzimuthOffsetMicrosteps %d",
                   AltitudeOffsetMicrosteps, AzimuthOffsetMicrosteps);

            if (0 != AzimuthOffsetMicrosteps)
            {
                // Calculate the slewing rates needed to reach that position
                // at the correct time. This is simple as interval is one second
                if (AzimuthOffsetMicrosteps > 0)
                {
                    if (AzimuthOffsetMicrosteps < MICROSTEPS_PER_REVOLUTION / 2.0)
                    {
                        // Forward
                        AxisDirectionRA = FORWARD;
                        AxisSlewRateRA  = AzimuthOffsetMicrosteps;
                    }
                    else
                    {
                        // Reverse
                        AxisDirectionRA = REVERSE;
                        AxisSlewRateRA  = MICROSTEPS_PER_REVOLUTION - AzimuthOffsetMicrosteps;
                    }
                }
                else
                {
                    AzimuthOffsetMicrosteps = std::abs(AzimuthOffsetMicrosteps);
                    if (AzimuthOffsetMicrosteps < MICROSTEPS_PER_REVOLUTION / 2.0)
                    {
                        // Forward
                        AxisDirectionRA = REVERSE;
                        AxisSlewRateRA  = AzimuthOffsetMicrosteps;
                    }
                    else
                    {
                        // Reverse
                        AxisDirectionRA = FORWARD;
                        AxisSlewRateRA  = MICROSTEPS_PER_REVOLUTION - AzimuthOffsetMicrosteps;
                    }
                }
                AxisSlewRateRA  = std::abs(AzimuthOffsetMicrosteps);
                AxisDirectionRA = AzimuthOffsetMicrosteps > 0 ? FORWARD : REVERSE; // !!!! BEWARE INERTIA FREE MOUNT
                AxisStatusRA    = SLEWING;
                DEBUGF(DBG_SIMULATOR, "TimerHit - Tracking AxisSlewRateRA %lf AxisDirectionRA %d", AxisSlewRateRA,
                       AxisDirectionRA);
            }
            else
            {
                // Nothing to do - stop the axis
                AxisStatusRA = STOPPED; // !!!! BEWARE INERTIA FREE MOUNT
                DEBUG(DBG_SIMULATOR, "TimerHit - Tracking nothing to do stopping RA axis");
            }

            if (0 != AltitudeOffsetMicrosteps)
            {
                // Calculate the slewing rates needed to reach that position
                // at the correct time.
                AxisSlewRateDEC  = std::abs(AltitudeOffsetMicrosteps);
                AxisDirectionDEC = AltitudeOffsetMicrosteps > 0 ? FORWARD : REVERSE; // !!!! BEWARE INERTIA FREE MOUNT
                AxisStatusDEC    = SLEWING;
                DEBUGF(DBG_SIMULATOR, "TimerHit - Tracking AxisSlewRateDEC %lf AxisDirectionDEC %d", AxisSlewRateDEC,
                       AxisDirectionDEC);
            }
            else
            {
                // Nothing to do - stop the axis
                AxisStatusDEC = STOPPED; // !!!! BEWARE INERTIA FREE MOUNT
                DEBUG(DBG_SIMULATOR, "TimerHit - Tracking nothing to do stopping DEC axis");
            }

            break;
        }

        default:
            break;
    }

    TraceThisTick = false;
}

bool ScopeSim::updateLocation(double latitude, double longitude, double elevation)
{
    UpdateLocation(latitude, longitude, elevation);
    return true;
}

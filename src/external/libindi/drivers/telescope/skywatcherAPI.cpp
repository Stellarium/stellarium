/*!
 * \file skywatcherAPI.cpp
 *
 * \author Roger James
 * \author Gerry Rozema
 * \author Jean-Luc Geehalel
 * \date 13th November 2013
 *
 * This file contains an implementation in C++ of the Skywatcher API.
 * It is based on work from four sources.
 * A C++ implementation of the API by Roger James.
 * The indi_eqmod driver by Jean-Luc Geehalel.
 * The synscanmount driver by Gerry Rozema.
 * The C# implementation published by Skywatcher/Synta
 */

#include "skywatcherAPI.h"

#include <cmath>
#include <iomanip>
#include <memory>
#include <thread>

void AXISSTATUS::SetFullStop()
{
    FullStop  = true;
    SlewingTo = Slewing = false;
}

void AXISSTATUS::SetSlewing(bool forward, bool highspeed)
{
    FullStop = SlewingTo = false;
    Slewing              = true;

    SlewingForward = forward;
    HighSpeed      = highspeed;
}

void AXISSTATUS::SetSlewingTo(bool forward, bool highspeed)
{
    FullStop = Slewing = false;
    SlewingTo          = true;

    SlewingForward = forward;
    HighSpeed      = highspeed;
}

SkywatcherAPI::SkywatcherAPI()
{
    // I add an additional debug level so I can log verbose scope status
    DBG_SCOPE = INDI::Logger::getInstance().addDebugLevel("Scope Verbose", "SCOPE");

    RadiansPerMicrostep[AXIS1] = RadiansPerMicrostep[AXIS2] = 0;
    MicrostepsPerRadian[AXIS1] = MicrostepsPerRadian[AXIS2] = 0;
    DegreesPerMicrostep[AXIS1] = DegreesPerMicrostep[AXIS2] = 0;
    MicrostepsPerDegree[AXIS1] = MicrostepsPerDegree[AXIS2] = 0;
    CurrentEncoders[AXIS1] = CurrentEncoders[AXIS2] = 0;
    PolarisPositionEncoders[AXIS1] = PolarisPositionEncoders[AXIS2] = 0;
    ZeroPositionEncoders[AXIS1] = ZeroPositionEncoders[AXIS2] = 0;
    SlewingSpeed[AXIS1] = SlewingSpeed[AXIS2] = 0;
}

unsigned long SkywatcherAPI::BCDstr2long(std::string &String)
{
    if (String.size() != 6)
    {
        return 0;
    }
    unsigned long value = 0;

#define HEX(c) (((c) < 'A') ? ((c) - '0') : ((c) - 'A') + 10)

    value = HEX(String[4]);
    value <<= 4;
    value |= HEX(String[5]);
    value <<= 4;
    value |= HEX(String[2]);
    value <<= 4;
    value |= HEX(String[3]);
    value <<= 4;
    value |= HEX(String[0]);
    value <<= 4;
    value |= HEX(String[1]);

#undef HEX

    return value;
}

unsigned long SkywatcherAPI::Highstr2long(std::string &String)
{
    if (String.size() < 2)
    {
        return 0;
    }
    unsigned long res = 0;

#define HEX(c) (((c) < 'A') ? ((c) - '0') : ((c) - 'A') + 10)

    res = HEX(String[0]);
    res <<= 4;
    res |= HEX(String[1]);

#undef HEX

    return res;
}

bool SkywatcherAPI::CheckIfDCMotor()
{
    MYDEBUG(DBG_SCOPE, "CheckIfDCMotor");
    // Flush the tty read buffer
    char input[20];
    int rc;
    int nbytes;

    while (true)
    {
        rc = skywatcher_tty_read(MyPortFD, input, 20, 5, &nbytes);
        if (TTY_TIME_OUT == rc)
            break;
        if (TTY_OK != rc)
            return false;
    }

    if (TTY_OK != skywatcher_tty_write(MyPortFD, ":", 1, &nbytes))
        return false;

    rc = skywatcher_tty_read(MyPortFD, input, 1, 5, &nbytes);

    if ((TTY_OK == rc) && (1 == nbytes) && (':' == input[0]))
    {
        IsDCMotor = true;
        return true;
    }
    if (TTY_TIME_OUT == rc)
    {
        IsDCMotor = false;
        return true;
    }

    return false;
}

bool SkywatcherAPI::IsVirtuosoMount() const
{
    return MountCode >= 0x90;
}

long SkywatcherAPI::DegreesPerSecondToClocksTicksPerMicrostep(AXISID Axis, double DegreesPerSecond)
{
    double MicrostepsPerSecond = DegreesPerSecond * MicrostepsPerDegree[Axis];

    return long((double(StepperClockFrequency[Axis]) / MicrostepsPerSecond));
}

long SkywatcherAPI::DegreesToMicrosteps(AXISID Axis, double AngleInDegrees)
{
    return (long)(AngleInDegrees * MicrostepsPerDegree[(int)Axis]);
}

bool SkywatcherAPI::GetEncoder(AXISID Axis)
{
    MYDEBUG(DBG_SCOPE, "GetEncoder");
    std::string Parameters, Response;
    if (!TalkWithAxis(Axis, 'j', Parameters, Response))
        return false;

    long Microsteps            = BCDstr2long(Response);
    CurrentEncoders[(int)Axis] = Microsteps;

    return true;
}

bool SkywatcherAPI::GetHighSpeedRatio(AXISID Axis)
{
    MYDEBUG(DBG_SCOPE, "GetHighSpeedRatio");
    std::string Parameters, Response;

    if (!TalkWithAxis(Axis, 'g', Parameters, Response))
        return false;

    unsigned long highSpeedRatio = Highstr2long(Response);
    HighSpeedRatio[(int)Axis]    = highSpeedRatio;

    return true;
}

bool SkywatcherAPI::GetMicrostepsPerRevolution(AXISID Axis)
{
    MYDEBUG(DBG_SCOPE, "GetMicrostepsPerRevolution");
    std::string Parameters, Response;

    if (!TalkWithAxis(Axis, 'a', Parameters, Response))
        return false;

    long tmpMicrostepsPerRevolution = BCDstr2long(Response);

    // There is a bug in the earlier version firmware(Before 2.00) of motor controller MC001.
    // Overwrite the MicrostepsPerRevolution reported by the MC for 80GT mount and 114GT mount.
    if (MountCode == GT)
        tmpMicrostepsPerRevolution = 0x162B97; // for 80GT mount
    if (MountCode == _114GT)
        tmpMicrostepsPerRevolution = 0x205318; // for 114GT mount

    MicrostepsPerRevolution[(int)Axis] = tmpMicrostepsPerRevolution;

    MicrostepsPerRadian[(int)Axis] = tmpMicrostepsPerRevolution / (2 * M_PI);
    RadiansPerMicrostep[(int)Axis] = 2 * M_PI / tmpMicrostepsPerRevolution;
    MicrostepsPerDegree[(int)Axis] = tmpMicrostepsPerRevolution / 360.0;
    DegreesPerMicrostep[(int)Axis] = 360.0 / tmpMicrostepsPerRevolution;

    MYDEBUGF(INDI::Logger::DBG_SESSION, "Axis %d: %lf microsteps/degree, %lf microsteps/arcsec", Axis,
             (double)tmpMicrostepsPerRevolution / 360.0, (double)tmpMicrostepsPerRevolution / 360.0 / 60 / 60);

    return true;
}

bool SkywatcherAPI::GetMicrostepsPerWormRevolution(AXISID Axis)
{
    MYDEBUG(DBG_SCOPE, "GetMicrostepsPerWormRevolution");
    std::string Parameters, Response;

    if (!TalkWithAxis(Axis, 's', Parameters, Response))
        return false;

    MicrostepsPerWormRevolution[(int)Axis] = BCDstr2long(Response);

    return true;
}

bool SkywatcherAPI::GetMotorBoardVersion(AXISID Axis)
{
    MYDEBUG(DBG_SCOPE, "GetMotorBoardVersion");
    std::string Parameters, Response;

    if (!TalkWithAxis(Axis, 'e', Parameters, Response))
        return false;

    unsigned long tmpMCVersion = BCDstr2long(Response);

    MCVersion = ((tmpMCVersion & 0xFF) << 16) | ((tmpMCVersion & 0xFF00)) | ((tmpMCVersion & 0xFF0000) >> 16);
    return true;
}

SkywatcherAPI::PositiveRotationSense_t SkywatcherAPI::GetPositiveRotationDirection(AXISID Axis)
{
    INDI_UNUSED(Axis);
    if (MountCode == _114GT)
        return CLOCKWISE;

    return ANTICLOCKWISE;
}

bool SkywatcherAPI::GetStepperClockFrequency(AXISID Axis)
{
    MYDEBUG(DBG_SCOPE, "GetStepperClockFrequency");
    std::string Parameters, Response;

    if (!TalkWithAxis(Axis, 'b', Parameters, Response))
        return false;

    StepperClockFrequency[(int)Axis] = BCDstr2long(Response);

    return true;
}

bool SkywatcherAPI::GetStatus(AXISID Axis)
{
    MYDEBUG(DBG_SCOPE, "GetStatus");
    std::string Parameters, Response;

    if (!TalkWithAxis(Axis, 'f', Parameters, Response))
        return false;

    if ((Response[1] & 0x01) != 0)
    {
        // Axis is running
        AxesStatus[(int)Axis].FullStop = false;
        if ((Response[0] & 0x01) != 0)
        {
            AxesStatus[(int)Axis].Slewing   = true; // Axis in slewing(AstroMisc speed) mode.
            AxesStatus[(int)Axis].SlewingTo = false;
        }
        else
        {
            AxesStatus[(int)Axis].SlewingTo = true; // Axis in SlewingTo mode.
            AxesStatus[(int)Axis].Slewing   = false;
        }
    }
    else
    {
        // SlewTo Debugging
        if (AxesStatus[(int)Axis].SlewingTo)
        {
            // If the mount was doing a slew to
            GetEncoder(Axis);
            MYDEBUGF(INDI::Logger::DBG_SESSION,
                     "Axis %s SlewTo complete - offset to target %ld microsteps %lf arc seconds "
                     "LastSlewToTarget %ld CurrentEncoder %ld",
                     Axis == AXIS1 ? "AXIS1" : "AXIS2", LastSlewToTarget[Axis] - CurrentEncoders[Axis],
                     MicrostepsToDegrees(Axis, LastSlewToTarget[Axis] - CurrentEncoders[Axis]) * 3600,
                     LastSlewToTarget[Axis], CurrentEncoders[Axis]);
        }

        AxesStatus[(int)Axis].FullStop  = true; // FullStop = 1;	// Axis is fully stop.
        AxesStatus[(int)Axis].Slewing   = false;
        AxesStatus[(int)Axis].SlewingTo = false;
    }

    if ((Response[0] & 0x02) == 0)
        AxesStatus[(int)Axis].SlewingForward = true; // Angle increase = 1;
    else
        AxesStatus[(int)Axis].SlewingForward = false;

    if ((Response[0] & 0x04) != 0)
        AxesStatus[(int)Axis].HighSpeed = true; // HighSpeed running mode = 1;
    else
        AxesStatus[(int)Axis].HighSpeed = false;

    if ((Response[2] & 1) == 0)
        AxesStatus[(int)Axis].NotInitialized = true; // MC is not initialized.
    else
        AxesStatus[(int)Axis].NotInitialized = false;

    return true;
}

// Set initialization done ":F3", where '3'= Both CH1 and CH2.
bool SkywatcherAPI::InitializeMC()
{
    MYDEBUG(DBG_SCOPE, "InitializeMC");
    std::string Parameters, Response;

    if (!TalkWithAxis(AXIS1, 'F', Parameters, Response))
        return false;
    if (!TalkWithAxis(AXIS2, 'F', Parameters, Response))
        return false;
    return true;
}

bool SkywatcherAPI::InitMount(bool recover)
{
    MYDEBUG(DBG_SCOPE, "InitMount");

    if (!CheckIfDCMotor())
        return false;

    if (!GetMotorBoardVersion(AXIS1))
        return false;

    MountCode = MCVersion & 0xFF;

    // Disable EQ mounts
    if (MountCode < 0x80)
        return false;

    //// NOTE: Simulator settings, Mount dependent Settings

    // Inquire Gear Rate
    if (!GetMicrostepsPerRevolution(AXIS1))
        return false;
    if (!GetMicrostepsPerRevolution(AXIS2))
        return false;

    // Get stepper clock frequency
    if (!GetStepperClockFrequency(AXIS1))
        return false;
    if (!GetStepperClockFrequency(AXIS2))
        return false;

    // Inquire motor high speed ratio
    if (!GetHighSpeedRatio(AXIS1))
        return false;
    if (!GetHighSpeedRatio(AXIS2))
        return false;

    // Inquire PEC period
    // DC motor controller does not support PEC
    if (!IsDCMotor)
    {
        GetMicrostepsPerWormRevolution(AXIS1);
        GetMicrostepsPerWormRevolution(AXIS2);
    }

    // Inquire Axis Position
    if (!GetEncoder(AXIS1))
        return false;
    if (!GetEncoder(AXIS2))
        return false;
    MYDEBUGF(DBG_SCOPE, "Encoders before init Axis1 %ld Axis2 %ld", CurrentEncoders[AXIS1], CurrentEncoders[AXIS2]);

    // Set initial axis positions
    // These are used to define the arbitrary zero position vector for the axis
    if (!recover)
    {
        PolarisPositionEncoders[AXIS1] = CurrentEncoders[AXIS1];
        PolarisPositionEncoders[AXIS2] = CurrentEncoders[AXIS2];
        ZeroPositionEncoders[AXIS1] = PolarisPositionEncoders[AXIS1];
        ZeroPositionEncoders[AXIS2] = PolarisPositionEncoders[AXIS2];
    }

    if (!InitializeMC())
        return false;

    if (!GetEncoder(AXIS1))
        return false;
    if (!GetEncoder(AXIS2))
        return false;
    MYDEBUGF(DBG_SCOPE, "Encoders after init Axis1 %ld Axis2 %ld", CurrentEncoders[AXIS1], CurrentEncoders[AXIS2]);

    // These two LowSpeedGotoMargin are calculate from slewing for 5 seconds in 128x sidereal rate
    LowSpeedGotoMargin[(int)AXIS1] = (long)(640 * SIDEREALRATE * MicrostepsPerRadian[(int)AXIS1]);
    LowSpeedGotoMargin[(int)AXIS2] = (long)(640 * SIDEREALRATE * MicrostepsPerRadian[(int)AXIS2]);

    return true;
}

bool SkywatcherAPI::InstantStop(AXISID Axis)
{
    // Request a slow stop
    MYDEBUG(DBG_SCOPE, "InstantStop");
    std::string Parameters, Response;
    if (!TalkWithAxis(Axis, 'L', Parameters, Response))
        return false;
    AxesStatus[(int)Axis].SetFullStop();
    return true;
}

void SkywatcherAPI::Long2BCDstr(long Number, std::string &String)
{
    std::stringstream Temp;
    Temp << std::hex << std::setfill('0') << std::uppercase << std::setw(2) << (Number & 0xff) << std::setw(2)
         << ((Number & 0xff00) >> 8) << std::setw(2) << ((Number & 0xff0000) >> 16);
    String = Temp.str();
}

double SkywatcherAPI::MicrostepsToDegrees(AXISID Axis, long Microsteps)
{
    return Microsteps * DegreesPerMicrostep[(int)Axis];
}

double SkywatcherAPI::MicrostepsToRadians(AXISID Axis, long Microsteps)
{
    return Microsteps * RadiansPerMicrostep[(int)Axis];
}

void SkywatcherAPI::PrepareForSlewing(AXISID Axis, double Speed)
{
    // Update the axis status
    if (!GetStatus(Axis))
        return;

    if (!AxesStatus[Axis].FullStop)
    {
        // Axis is running
        if ((AxesStatus[Axis].SlewingTo)                            // slew to (GOTO) in progress
            || (AxesStatus[Axis].HighSpeed)                         // currently high speed slewing
            || (std::abs(Speed) >= LOW_SPEED_MARGIN)                // I am about to request high speed
            || ((AxesStatus[Axis].SlewingForward) && (Speed < 0))   // Direction change
            || (!(AxesStatus[Axis].SlewingForward) && (Speed > 0))) // Direction change
        {
            // I need to stop the axis first
            SlowStop(Axis);
        }
        else
            return; // NO need change motion mode

        // Horrible bit A POLLING LOOP !!!!!!!!!!
        while (true)
        {
            // Update status
            GetStatus(Axis);

            if (AxesStatus[Axis].FullStop)
                break;

            std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Sleep for 1/10 second
        }
    }

    char Direction;
    if (Speed > 0.0)
        Direction = '0';
    else
    {
        Direction = '1';
        Speed     = -Speed;
    }

    if (Speed > LOW_SPEED_MARGIN)
        SetMotionMode(Axis, '3', Direction);
    else
        SetMotionMode(Axis, '1', Direction);
}

long SkywatcherAPI::RadiansPerSecondToClocksTicksPerMicrostep(AXISID Axis, double RadiansPerSecond)
{
    double MicrostepsPerSecond = RadiansPerSecond * MicrostepsPerRadian[Axis];

    return long((double(StepperClockFrequency[Axis]) / MicrostepsPerSecond));
}

long SkywatcherAPI::RadiansToMicrosteps(AXISID Axis, double AngleInRadians)
{
    return (long)(AngleInRadians * MicrostepsPerRadian[(int)Axis]);
}

bool SkywatcherAPI::SetEncoder(AXISID Axis, long Microsteps)
{
    MYDEBUG(DBG_SCOPE, "SetEncoder");
    std::string Parameters, Response;

    Long2BCDstr(Microsteps, Parameters);

    return TalkWithAxis(Axis, 'L', Parameters, Response);
}

bool SkywatcherAPI::SetGotoTargetOffset(AXISID Axis, long OffsetInMicrosteps)
{
    MYDEBUG(DBG_SCOPE, "SetGotoTargetOffset");
    std::string Parameters, Response;

    Long2BCDstr(OffsetInMicrosteps, Parameters);

    return TalkWithAxis(Axis, 'H', Parameters, Response);
}

/// Func - 0 High speed slew to mode (goto)
/// Func - 1 Low speed slew mode
/// Func - 2 Low speed slew to mode (goto)
/// Func - 3 High speed slew mode
bool SkywatcherAPI::SetMotionMode(AXISID Axis, char Func, char Direction)
{
    MYDEBUG(DBG_SCOPE, "SetMotionMode");
    std::string Parameters, Response;

    Parameters.push_back(Func);
    Parameters.push_back(Direction);

    return TalkWithAxis(Axis, 'G', Parameters, Response);
}

bool SkywatcherAPI::SetClockTicksPerMicrostep(AXISID Axis, long ClockTicksPerMicrostep)
{
    MYDEBUG(DBG_SCOPE, "SetClockTicksPerMicrostep");
    std::string Parameters, Response;

    Long2BCDstr(ClockTicksPerMicrostep, Parameters);

    return TalkWithAxis(Axis, 'I', Parameters, Response);
}

bool SkywatcherAPI::SetSlewModeDeccelerationRampLength(AXISID Axis, long Microsteps)
{
    MYDEBUG(DBG_SCOPE, "SetSlewModeDeccelerationRampLength");
    std::string Parameters, Response;

    Long2BCDstr(Microsteps, Parameters);

    return TalkWithAxis(Axis, 'U', Parameters, Response);
}

bool SkywatcherAPI::SetSlewToModeDeccelerationRampLength(AXISID Axis, long Microsteps)
{
    MYDEBUG(DBG_SCOPE, "SetSlewToModeDeccelerationRampLength");
    std::string Parameters, Response;

    Long2BCDstr(Microsteps, Parameters);

    return TalkWithAxis(Axis, 'M', Parameters, Response);
}

bool SkywatcherAPI::SetSwitch(bool OnOff)
{
    MYDEBUG(DBG_SCOPE, "SetSwitch");
    std::string Parameters, Response;

    if (OnOff)
        Parameters = "1";
    else
        Parameters = "0";

    return TalkWithAxis(AXIS1, 'O', Parameters, Response);
}

void SkywatcherAPI::Slew(AXISID Axis, double SpeedInRadiansPerSecond, bool IgnoreSilentMode)
{
    MYDEBUGF(DBG_SCOPE, "Slew axis: %d speed: %1.6f", (int)Axis, SpeedInRadiansPerSecond);
    // Clamp to MAX_SPEED
    if (SpeedInRadiansPerSecond > MAX_SPEED)
        SpeedInRadiansPerSecond = MAX_SPEED;
    else if (SpeedInRadiansPerSecond < -MAX_SPEED)
        SpeedInRadiansPerSecond = -MAX_SPEED;

    double InternalSpeed = SpeedInRadiansPerSecond;

    if (std::abs(InternalSpeed) <= SIDEREALRATE / 1000.0)
    {
        SlowStop(Axis);
        return;
    }

    // Stop motor and set motion mode if necessary
    PrepareForSlewing(Axis, InternalSpeed);

    bool Forward;
    if (InternalSpeed > 0.0)
        Forward = true;
    else
    {
        InternalSpeed = -InternalSpeed;
        Forward       = false;
    }

    bool HighSpeed = false;

    if (InternalSpeed > LOW_SPEED_MARGIN && (IgnoreSilentMode || !SilentSlewMode))
    {
        InternalSpeed = InternalSpeed / (double)HighSpeedRatio[Axis];
        HighSpeed     = true;
    }
    long SpeedInt = RadiansPerSecondToClocksTicksPerMicrostep(Axis, InternalSpeed);
    if ((MCVersion == 0x010600) || (MCVersion == 0x0010601)) // Cribbed from Mount_Skywatcher.cs
        SpeedInt -= 3;
    if (SpeedInt < 6)
        SpeedInt = 6;
    SetClockTicksPerMicrostep(Axis, SpeedInt);

    StartMotion(Axis);

    AxesStatus[Axis].SetSlewing(Forward, HighSpeed);
    SlewingSpeed[Axis] = SpeedInRadiansPerSecond;
}

void SkywatcherAPI::SlewTo(AXISID Axis, long OffsetInMicrosteps)
{
    MYDEBUGF(INDI::Logger::DBG_SESSION, "SlewTo axis: %d offset: %ld", (int)Axis, OffsetInMicrosteps);
    if (0 == OffsetInMicrosteps)
        // Nothing to do
        return;

    // Debugging
    LastSlewToTarget[Axis] = CurrentEncoders[Axis] + OffsetInMicrosteps;
    MYDEBUGF(INDI::Logger::DBG_SESSION, "SlewTo axis %d Offset %ld CurrentEncoder %ld SlewToTarget %ld", Axis,
             OffsetInMicrosteps, CurrentEncoders[Axis], LastSlewToTarget[Axis]);

    char Direction;
    bool Forward;

    if (OffsetInMicrosteps > 0)
    {
        Forward   = true;
        Direction = '0';
    }
    else
    {
        Forward            = false;
        Direction          = '1';
        OffsetInMicrosteps = -OffsetInMicrosteps;
    }

    bool HighSpeed = false;

    if (OffsetInMicrosteps > LowSpeedGotoMargin[Axis] && !SilentSlewMode)
        HighSpeed = true;

    if (!GetStatus(Axis))
        return;

    if (!AxesStatus[Axis].FullStop)
    {
        // Axis is running
        if ((AxesStatus[Axis].SlewingTo)                        // slew to (GOTO) in progress
            || (AxesStatus[Axis].HighSpeed)                     // currently high speed slewing
            || HighSpeed                                        // I am about to request high speed
            || ((AxesStatus[Axis].SlewingForward) && !Forward)  // Direction change
            || (!(AxesStatus[Axis].SlewingForward) && Forward)) // Direction change
        {
            // I need to stop the axis first
            SlowStop(Axis);
            // Horrible bit A POLLING LOOP !!!!!!!!!!
            while (true)
            {
                // Update status
                GetStatus(Axis);

                if (AxesStatus[Axis].FullStop)
                    break;

                std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Sleep for 1/10 second
            }
        }
    }

    if (HighSpeed)
        SetMotionMode(Axis, '0', Direction);
    else
        SetMotionMode(Axis, '2', Direction);

    SetGotoTargetOffset(Axis, OffsetInMicrosteps);

    if (HighSpeed)
        SetSlewToModeDeccelerationRampLength(Axis, OffsetInMicrosteps > 3200 ? 3200 : OffsetInMicrosteps);
    else
        SetSlewToModeDeccelerationRampLength(Axis, OffsetInMicrosteps > 200 ? 200 : OffsetInMicrosteps);
    StartMotion(Axis);

    AxesStatus[Axis].SetSlewingTo(Forward, HighSpeed);
}

bool SkywatcherAPI::SlowStop(AXISID Axis)
{
    // Request a slow stop
    MYDEBUG(DBG_SCOPE, "SlowStop");
    std::string Parameters, Response;

    return TalkWithAxis(Axis, 'K', Parameters, Response);
}

bool SkywatcherAPI::StartMotion(AXISID Axis)
{
    MYDEBUG(DBG_SCOPE, "StartMotion");
    std::string Parameters, Response;

    return TalkWithAxis(Axis, 'J', Parameters, Response);
}

bool SkywatcherAPI::TalkWithAxis(AXISID Axis, char Command, std::string &cmdDataStr, std::string &responseStr)
{
    MYDEBUGF(DBG_SCOPE, "TalkWithAxis Axis %s Command %c Data (%s)", Axis == AXIS1 ? "AXIS1" : "AXIS2", Command,
             cmdDataStr.c_str());

    std::string SendBuffer;
    int bytesWritten;
    int bytesRead;
    bool StartReading   = false;
    bool EndReading     = false;
    bool mount_response = false;

    SendBuffer.push_back(':');
    SendBuffer.push_back(Command);
    SendBuffer.push_back(Axis == AXIS1 ? '1' : '2');
    SendBuffer.append(cmdDataStr);
    SendBuffer.push_back('\r');
    skywatcher_tty_write(MyPortFD, SendBuffer.c_str(), SendBuffer.size(), &bytesWritten);

    while (!EndReading)
    {
        char c;

        int rc = skywatcher_tty_read(MyPortFD, &c, 1, 10, &bytesRead);
        if ((rc != TTY_OK) || (bytesRead != 1))
            return false;

        if ((c == '=') || (c == '!'))
        {
            mount_response = (c == '=');
            StartReading = true;
            continue;
        }

        if ((c == '\r') && StartReading)
        {
            EndReading = true;
            continue;
        }

        if (StartReading)
            responseStr.push_back(c);
    }
    MYDEBUGF(DBG_SCOPE, "TalkWithAxis - %s Response (%s)", mount_response ? "Good" : "Bad", responseStr.c_str());
    return true;
}

bool SkywatcherAPI::IsInMotion(AXISID Axis)
{
    MYDEBUG(DBG_SCOPE, "IsInMotion");

    return AxesStatus[(int)Axis].Slewing || AxesStatus[(int)Axis].SlewingTo;
}

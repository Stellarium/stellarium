/*******************************************************************************
 Copyright(c) 2015 Camiel Severijns. All rights reserved.

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Library General Public
 License version 2 as published by the Free Software Foundation.
 .
 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Library General Public License for more details.
 .
 You should have received a copy of the GNU Library General Public License
 along with this library; see the file COPYING.LIB.  If not, write to
 the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 Boston, MA 02110-1301, USA.
*******************************************************************************/

#include "smartfocus.h"

#include "indicom.h"

#include <cerrno>
#include <fcntl.h>
#include <memory>
#include <cstring>
#include <termios.h>
#include <unistd.h>

namespace
{
static constexpr const SmartFocus::Position PositionInvalid { static_cast<SmartFocus::Position>(0xFFFF) };
// Interval to check the focuser state (in milliseconds)
static constexpr const int TimerInterval { 500 };
// in seconds
static constexpr const int ReadTimeOut { 1 };

// SmartFocus command and response characters
static constexpr const char goto_position { 'g' };
static constexpr const char stop_focuser { 's' };
static constexpr const char read_id_register { 'b' };
static constexpr const char read_id_respons { 'j' };
static constexpr const char read_position { 'p' };
static constexpr const char read_flags { 't' };
static constexpr const char motion_complete { 'c' };
static constexpr const char motion_error { 'r' };
static constexpr const char motion_stopped { 's' };
} // namespace

std::unique_ptr<SmartFocus> smartFocus(new SmartFocus());

void ISGetProperties(const char *dev)
{
    smartFocus->ISGetProperties(dev);
}

void ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    smartFocus->ISNewSwitch(dev, name, states, names, n);
}

void ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n)
{
    smartFocus->ISNewText(dev, name, texts, names, n);
}

void ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    smartFocus->ISNewNumber(dev, name, values, names, n);
}

void ISNewBLOB(const char *dev, const char *name, int sizes[], int blobsizes[], char *blobs[], char *formats[],
               char *names[], int n)
{
    INDI_UNUSED(dev);
    INDI_UNUSED(name);
    INDI_UNUSED(sizes);
    INDI_UNUSED(blobsizes);
    INDI_UNUSED(blobs);
    INDI_UNUSED(formats);
    INDI_UNUSED(names);
    INDI_UNUSED(n);
}

void ISSnoopDevice(XMLEle *root)
{
    smartFocus->ISSnoopDevice(root);
}

SmartFocus::SmartFocus()
{
    SetFocuserCapability(FOCUSER_CAN_ABS_MOVE | FOCUSER_CAN_REL_MOVE | FOCUSER_CAN_ABORT);
}

bool SmartFocus::initProperties()
{
    INDI::Focuser::initProperties();

    // No speed for SmartFocus
    FocusSpeedN[0].min = FocusSpeedN[0].max = FocusSpeedN[0].value = 1;
    IUUpdateMinMax(&FocusSpeedNP);

    IUFillLight(&FlagsL[STATUS_SERIAL_FRAMING_ERROR], "SERIAL_FRAMING_ERROR", "Serial framing error", IPS_OK);
    IUFillLight(&FlagsL[STATUS_SERIAL_OVERRUN_ERROR], "SERIAL_OVERRUN_ERROR", "Serial overrun error", IPS_OK);
    IUFillLight(&FlagsL[STATUS_MOTOR_ENCODE_ERROR], "MOTOR_ENCODER_ERROR", "Motor/encoder error", IPS_OK);
    IUFillLight(&FlagsL[STATUS_AT_ZERO_POSITION], "AT_ZERO_POSITION", "At zero position", IPS_OK);
    IUFillLight(&FlagsL[STATUS_AT_MAX_POSITION], "AT_MAX_POSITION", "At max. position", IPS_OK);
    IUFillLightVector(&FlagsLP, FlagsL, STATUS_NUM_FLAGS, getDeviceName(), "FLAGS", "Status Flags", MAIN_CONTROL_TAB,
                      IPS_IDLE);

    IUFillNumber(&MaxPositionN[0], "MAXPOSITION", "Maximum position", "%6.0f", 1., 100000., 0., 1833);
    IUFillNumberVector(&MaxPositionNP, MaxPositionN, 1, getDeviceName(), "FOCUS_MAXPOSITION", "Max. position",
                       OPTIONS_TAB, IP_RW, 0, IPS_IDLE);

    FocusRelPosN[0].min   = 0.;
    FocusRelPosN[0].max   = MaxPositionN[0].value;
    FocusRelPosN[0].value = 10;
    FocusRelPosN[0].step  = 1;

    FocusAbsPosN[0].min   = 0.;
    FocusAbsPosN[0].max   = MaxPositionN[0].value;
    FocusAbsPosN[0].value = 0;
    FocusAbsPosN[0].step  = 1;

    updatePeriodMS = TimerInterval;

    return true;
}

bool SmartFocus::updateProperties()
{
    INDI::Focuser::updateProperties();

    if (isConnected())
    {
        defineLight(&FlagsLP);
        defineNumber(&MaxPositionNP);
        SFgetState();
        IDMessage(getDeviceName(), "SmartFocus focuser ready for use.");
    }
    else
    {
        deleteProperty(FlagsLP.name);
        deleteProperty(MaxPositionNP.name);
    }
    return true;
}

bool SmartFocus::Handshake()
{
    if (isSimulation())
        return true;

    if (!SFacknowledge())
    {
        DEBUG(INDI::Logger::DBG_DEBUG, "SmartFocus is not communicating.");
        return false;
    }

    DEBUG(INDI::Logger::DBG_DEBUG, "SmartFocus is communicating.");
    return true;
}

const char *SmartFocus::getDefaultName()
{
    return "SmartFocus";
}

bool SmartFocus::ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    if (dev != nullptr && strcmp(dev, getDeviceName()) == 0)
    {
        if (strcmp(name, MaxPositionNP.name) == 0)
        {
            if (values[0] > 0)
            {
                IUUpdateNumber(&MaxPositionNP, values, names, n);
                FocusAbsPosN[0].min = 0;
                FocusAbsPosN[0].max = MaxPositionN[0].value;
                IUUpdateMinMax(&FocusAbsPosNP);
                MaxPositionNP.s = IPS_OK;
                IDSetNumber(&MaxPositionNP, nullptr);
                return true;
            }
            else
                return false;
        }
    }
    return INDI::Focuser::ISNewNumber(dev, name, values, names, n);
}

//bool SmartFocus::ISNewSwitch (const char *dev, const char *name, ISState *states, char *names[], int n) {
//  return INDI::Focuser::ISNewSwitch(dev,name,states,names,n);
//}

bool SmartFocus::AbortFocuser()
{
    bool result = true;
    if (!isSimulation() && SFisMoving())
    {
        DEBUG(INDI::Logger::DBG_DEBUG, "AbortFocuser: stopping motion");
        result = send(&stop_focuser, sizeof(stop_focuser), "AbortFocuser");
        // N.B.: The respons to this stop command will be captured in the TimerHit method!
    }
    return result;
}

IPState SmartFocus::MoveAbsFocuser(uint32_t targetPosition)
{
    const Position destination = static_cast<Position>(targetPosition);
    IPState result             = IPS_ALERT;
    if (isSimulation())
    {
        position = destination;
        state    = Idle;
        result   = IPS_OK;
    }
    else
    {
        char command[3];
        command[0] = goto_position;
        command[1] = ((destination >> 8) & 0xFF);
        command[2] = (destination & 0xFF);
        DEBUGF(INDI::Logger::DBG_DEBUG, "MoveAbsFocuser: destination= %d", destination);
        tcflush(PortFD, TCIOFLUSH);
        if (send(command, sizeof(command), "MoveAbsFocuser"))
        {
            char respons;
            if (recv(&respons, sizeof(respons), "MoveAbsFocuser"))
            {
                DEBUGF(INDI::Logger::DBG_DEBUG, "MoveAbsFocuser received echo: %c", respons);
                if (respons != goto_position)
                    DEBUGF(INDI::Logger::DBG_ERROR, "MoveAbsFocuser received unexpected respons: %c (0x02x)", respons,
                           respons);
                else
                {
                    state  = MovingTo;
                    result = IPS_BUSY;
                }
            }
        }
    }
    return result;
}

IPState SmartFocus::MoveRelFocuser(FocusDirection dir, uint32_t ticks)
{
    return MoveAbsFocuser(position + (dir == FOCUS_INWARD ? -ticks : ticks));
}

class NonBlockingIO
{
  public:
    NonBlockingIO(const char *_device, const int _fd) : device(_device), fd(_fd), flags(fcntl(_fd, F_GETFL, 0))
    {
        if (flags == -1)
            DEBUGFDEVICE(device, INDI::Logger::DBG_ERROR, "NonBlockingIO::NonBlockingIO() fcntl get error: errno=%d",
                         errno);
        else if (fcntl(fd, F_SETFL, (flags | O_NONBLOCK)) == -1)
            DEBUGFDEVICE(device, INDI::Logger::DBG_ERROR, "NonBlockingIO::NonBlockingIO() fcntl set error: errno=%d",
                         errno);
    }

    ~NonBlockingIO()
    {
        if (flags != -1 && fcntl(fd, F_SETFL, flags) == -1)
            DEBUGFDEVICE(device, INDI::Logger::DBG_ERROR,
                         "NonBlockinIO::~NonBlockingIO() fcntl set error: errno=%d", errno);
    }

  private:
    const char *device;
    const int fd;
    const int flags;
};

void SmartFocus::TimerHit()
{
    // Wait for the end-of-motion character (c,r, or s) when the focuser is moving
    // due to a request from this driver. Otherwise, retrieve the current position
    // and state flags of the SmartFocus unit to keep the driver up-to-date with
    // motion commands issued manually.
    // TODO: What happens when the smartFocus unit is switched off?
    if (!isConnected())
        return;

    if (!isSimulation() && SFisMoving())
    {
        NonBlockingIO non_blocking(getDeviceName(), PortFD); // Automatically controls blocking IO by its scope
        char respons;
        if (read(PortFD, &respons, sizeof(respons)) == sizeof(respons))
        {
            DEBUGF(INDI::Logger::DBG_DEBUG, "TimerHit() received character: %c (0x%02x)", respons, respons);
            if (respons != motion_complete && respons != motion_error && respons != motion_stopped)
                DEBUGF(INDI::Logger::DBG_ERROR, "TimerHit() received unexpected character: %c (0x%02x)", respons,
                       respons);
            state = Idle;
        }
    }
    if (SFisIdle())
        SFgetState();
    timer_id = SetTimer(TimerInterval);
}

bool SmartFocus::saveConfigItems(FILE *fp)
{
    IUSaveConfigNumber(fp, &MaxPositionNP);
    return true;
}

bool SmartFocus::SFacknowledge()
{
    bool success = false;
    if (isSimulation())
        success = true;
    else
    {
        tcflush(PortFD, TCIOFLUSH);
        if (send(&read_id_register, sizeof(read_id_register), "SFacknowledge"))
        {
            char respons[2];
            if (recv(respons, sizeof(respons), "SFacknowledge", false))
            {
                DEBUGF(INDI::Logger::DBG_DEBUG, "SFacknowledge received: %c%c", respons[0], respons[1]);
                success = (respons[0] == read_id_register && respons[1] == read_id_respons);
                if (!success)
                    DEBUGF(INDI::Logger::DBG_ERROR, "SFacknowledge received unexpected respons: %c%c (0x02 0x02x)",
                           respons[0], respons[1], respons[0], respons[1]);
            }
        }
    }
    return success;
}

SmartFocus::Position SmartFocus::SFgetPosition()
{
    Position result = PositionInvalid;
    if (isSimulation())
        result = position;
    else
    {
        tcflush(PortFD, TCIOFLUSH);
        if (send(&read_position, sizeof(read_position), "SFgetPosition"))
        {
            char respons[3];
            if (recv(respons, sizeof(respons), "SFgetPosition"))
            {
                if (respons[0] == read_position)
                {
                    result = (((static_cast<Position>(respons[1]) << 8) & 0xFF00) |
                              (static_cast<Position>(respons[2]) & 0x00FF));
                    DEBUGF(INDI::Logger::DBG_DEBUG, "SFgetPosition: position=%d", result);
                }
                else
                    DEBUGF(INDI::Logger::DBG_ERROR, "SFgetPosition received unexpected respons: %c (0x02x)", respons[0],
                           respons[0]);
            }
        }
    }
    return result;
}

SmartFocus::Flags SmartFocus::SFgetFlags()
{
    Flags result = 0x00;
    if (!isSimulation())
    {
        tcflush(PortFD, TCIOFLUSH);
        if (send(&read_flags, sizeof(read_flags), "SFgetFlags"))
        {
            char respons[2];
            if (recv(respons, sizeof(respons), "SFgetFlags"))
            {
                if (respons[0] == read_flags)
                {
                    result = static_cast<Flags>(respons[1]);
                    DEBUGF(INDI::Logger::DBG_DEBUG, "SFgetFlags: flags=0x%02x", result);
                }
                else
                    DEBUGF(INDI::Logger::DBG_ERROR, "SFgetFlags received unexpected respons: %c (0x02x)", respons[0],
                           respons[0]);
            }
        }
    }
    return result;
}

void SmartFocus::SFgetState()
{
    const Flags flags = SFgetFlags();

    FlagsL[STATUS_SERIAL_FRAMING_ERROR].s = (flags & SerFramingError ? IPS_ALERT : IPS_OK);
    FlagsL[STATUS_SERIAL_OVERRUN_ERROR].s = (flags & SerOverrunError ? IPS_ALERT : IPS_OK);
    FlagsL[STATUS_MOTOR_ENCODE_ERROR].s   = (flags & MotorEncoderError ? IPS_ALERT : IPS_OK);
    FlagsL[STATUS_AT_ZERO_POSITION].s     = (flags & AtZeroPosition ? IPS_ALERT : IPS_OK);
    FlagsL[STATUS_AT_MAX_POSITION].s      = (flags & AtMaxPosition ? IPS_ALERT : IPS_OK);
    IDSetLight(&FlagsLP, nullptr);

    if ((position = SFgetPosition()) == PositionInvalid)
    {
        FocusAbsPosNP.s = IPS_ALERT;
        IDSetNumber(&FocusAbsPosNP, "Error while reading SmartFocus position");
    }
    else
    {
        FocusAbsPosN[0].value = position;
        FocusAbsPosNP.s       = IPS_OK;
        IDSetNumber(&FocusAbsPosNP, nullptr);
    }
}

bool SmartFocus::send(const char *command, const size_t nbytes, const char *from, const bool log_error)
{
    int nbytes_written = 0;
    const int rc       = tty_write(PortFD, command, nbytes, &nbytes_written);
    const bool success = (rc == TTY_OK && nbytes_written == (int)nbytes);

    if (!success && log_error)
    {
        char errstr[MAXRBUF];
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s: %s (%d of %d bytes written).", from, errstr, nbytes_written, nbytes);
    }
    return success;
}

bool SmartFocus::recv(char *respons, const size_t nbytes, const char *from, const bool log_error)
{
    int nbytes_read    = 0;
    const int rc       = tty_read(PortFD, respons, nbytes, ReadTimeOut, &nbytes_read);
    const bool success = (rc == TTY_OK && nbytes_read == (int)nbytes);

    if (!success && log_error)
    {
        char errstr[MAXRBUF];
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s: %s (%d of %d bytes read).", errstr, from, nbytes_read, nbytes);
    }
    return success;
}

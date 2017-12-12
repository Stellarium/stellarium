/*
    Pulsar2 INDI driver

    Copyright (C) 2016, 2017 Jasem Mutlaq and Camiel Severijns

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

#include "lx200pulsar2.h"

#include "indicom.h"
#include "lx200driver.h"

#include <cmath>
#include <cerrno>
#include <cstring>
#include <termios.h>
#include <unistd.h>

extern char lx200Name[MAXINDIDEVICE];
extern unsigned int DBG_SCOPE;

namespace Pulsar2Commands
{
// Reimplement LX200 commands to solve intermittent problems with tcflush() calls on the input stream.
// This implementation parses all input received from the Pulsar controller.

static constexpr int TimeOut     = 1;
static constexpr int BufferSize  = 32;
static constexpr int MaxAttempts = 3;

static constexpr char Null        = '\0';
static constexpr char Acknowledge = '\006';
static constexpr char Termination = '#';

static bool resynchronize_needed =
    false; // Indicates whether the input and output on the port needs to be resynchronized due to a timeout error.

int ACK(const int fd)
{
    DEBUGFDEVICE(lx200Name, DBG_SCOPE, "CMD <%02X>", Acknowledge);
    if (write(fd, &Acknowledge, sizeof(Acknowledge)) < 0)
    {
        DEBUGFDEVICE(lx200Name, DBG_SCOPE, "Error sending ACK: %s", strerror(errno));
        return -1;
    }
    char MountAlign[2];
    int nbytes_read      = 0;
    const int error_type = tty_read(fd, MountAlign, 1, TimeOut, &nbytes_read);
    if (error_type != TTY_OK)
        DEBUGFDEVICE(lx200Name, DBG_SCOPE, "Error receiving ACK: %s", strerror(errno));
    else
        DEBUGFDEVICE(lx200Name, DBG_SCOPE, "RES <%c>", MountAlign[0]);
    return (nbytes_read == 1 ? MountAlign[0] : error_type);
}

void resynchronize(const int fd)
{
    class ACKChecker
    {
      public:
        ACKChecker() : previous(Null) {}
        bool operator()(const int c)
        {
            // We need two successful acknowledges
            bool result = false;
            if (previous == Null)
            {
                if (c == 'P' || c == 'A' || c == 'L')
                    previous = c; // Remember first acknowledge response
            }
            else
            {
                result   = (c == previous); // Second acknowledge response must equal to previous one
                previous = Null;            // Both on success or failure, reset the previous character
            }
            return result;
        }

      private:
        int previous;
    };
    DEBUGDEVICE(lx200Name, DBG_SCOPE, "RESYNC");
    ACKChecker valid;
    for (int c = ACK(fd); !valid(c); c = ACK(fd))
        tcflush(fd, TCIFLUSH);
    resynchronize_needed = false;
}

// Send a command string without waiting for any response from the Pulsar controller
bool send(const int fd, const char *cmd)
{
    if (resynchronize_needed)
        resynchronize(fd);
    DEBUGFDEVICE(lx200Name, DBG_SCOPE, "CMD <%s>", cmd);
    const int nbytes   = strlen(cmd);
    int nbytes_written = 0;
    do
    {
        const int errcode = tty_write(fd, &cmd[nbytes_written], nbytes - nbytes_written, &nbytes_written);
        if (errcode != TTY_OK)
        {
            char errmsg[MAXRBUF];
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGFDEVICE(lx200Name, DBG_SCOPE, "Error: %s (%s)", errmsg, strerror(errno));
            return false;
        }
    } while (nbytes_written < nbytes); // Ensure that all characters have been sent
    return true;
}

// Send a command string and wait for a single character response indicating success or failure
// Ignore leading # characters
bool confirmed(const int fd, const char *cmd, char &response)
{
    response = Termination;
    if (send(fd, cmd))
    {
        for (int attempt = 0; response == Termination; ++attempt)
        {
            int nbytes_read   = 0;
            const int errcode = tty_read(fd, &response, sizeof(response), TimeOut, &nbytes_read);
            if (errcode != TTY_OK)
            {
                char errmsg[MAXRBUF];
                tty_error_msg(errcode, errmsg, MAXRBUF);
                DEBUGFDEVICE(lx200Name, DBG_SCOPE, "Error: %s (%s, attempt %d)", errmsg, strerror(errno), attempt);
                if (attempt == MaxAttempts - 1)
                {
                    resynchronize_needed = true;
                    return false;
                }
            }
            else // tty_read was successful and nbytes_read is garantueed to be 1
                DEBUGFDEVICE(lx200Name, DBG_SCOPE, "RES <%c> (attempt %d)", response, attempt);
        }
    }
    return true;
}

// Receive a terminated response string
bool receive(const int fd, char response[])
{
    response[0]           = Null;
    bool done             = false;
    int nbytes_read_total = 0;
    int attempt;
    for (attempt = 0; !done; ++attempt)
    {
        int nbytes_read   = 0;
        const int errcode = tty_read_section(fd, response + nbytes_read_total, Termination, TimeOut, &nbytes_read);
        if (errcode != TTY_OK)
        {
            char errmsg[MAXRBUF];
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGFDEVICE(lx200Name, DBG_SCOPE, "Error: %s (%s, attempt %d)", errmsg, strerror(errno), attempt);
            nbytes_read_total +=
                nbytes_read; // Keep track of how many characters have been read successfully despite the error
            if (attempt == MaxAttempts - 1)
            {
                resynchronize_needed        = (errcode == TTY_TIME_OUT);
                response[nbytes_read_total] = Null;
                return false;
            }
        }
        else
        {
            // Skip response strings consisting of a single termination character
            if (nbytes_read_total == 0 && response[0] == Termination)
                response[0] = Null;
            else
                done = true;
        }
    }
    response[nbytes_read_total - 1] = Null; // Remove the termination character
    DEBUGFDEVICE(lx200Name, DBG_SCOPE, "RES <%s> (attempt %d)", response, attempt);
    return true;
}

inline bool getString(const int fd, const char *cmd, char response[])
{
    return (send(fd, cmd) && receive(fd, response));
}

inline bool getVersion(const int fd, char response[])
{
    return getString(fd, ":YV#", response);
}

enum PECorrection
{
    PECorrectionOff = 0,
    PECorrectionOn  = 1
};

bool getPECorrection(const int fd, PECorrection *PECra, PECorrection *PECdec)
{
    char response[8];
    bool success = getString(fd, "#:YGP#", response);
    if (success)
    {
        success = (sscanf(response, "%1d,%1d", reinterpret_cast<int *>(PECra), reinterpret_cast<int *>(PECdec)) == 2);
    }
    return success;
}

enum RCorrection
{
    RCorrectionOff = 0,
    RCorrectionOn  = 1
};

bool getRCorrection(const int fd, RCorrection *Rra, RCorrection *Rdec)
{
    char response[8];
    bool success = getString(fd, "#:YGR#", response);
    if (success)
    {
        success = (sscanf(response, "%1d,%1d", reinterpret_cast<int *>(Rra), reinterpret_cast<int *>(Rdec)) == 2);
    }
    return success;
}

bool getInt(const int fd, const char *cmd, int *value)
{
    char response[16];
    bool success = getString(fd, cmd, response);
    if (success)
    {
        success = (sscanf(response, "%d", value) == 1);
        if (success)
            DEBUGFDEVICE(lx200Name, DBG_SCOPE, "VAL [%d]", *value);
        else
            DEBUGDEVICE(lx200Name, DBG_SCOPE, "Unable to parse response");
    }
    return success;
}

enum SideOfPier
{
    EastOfPier = 0,
    WestOfPier = 1
};

inline bool getSideOfPier(const int fd, SideOfPier *side_of_pier)
{
    return getInt(fd, "#:YGN#", reinterpret_cast<int *>(side_of_pier));
}

enum PoleCrossing
{
    PoleCrossingOff = 0,
    PoleCrossingOn  = 1
};

inline bool getPoleCrossing(const int fd, PoleCrossing *pole_crossing)
{
    return getInt(fd, "#:YGQ#", reinterpret_cast<int *>(pole_crossing));
}

bool getSexa(const int fd, const char *cmd, double *value)
{
    char response[16];
    bool success = getString(fd, cmd, response);
    if (success)
    {
        success = (f_scansexa(response, value) == 0);
        if (success)
            DEBUGFDEVICE(lx200Name, DBG_SCOPE, "VAL [%g]", *value);
        else
            DEBUGDEVICE(lx200Name, DBG_SCOPE, "Unable to parse response");
    }
    return success;
}

inline bool getObjectRADec(const int fd, double *ra, double *dec)
{
    return (getSexa(fd, "#:GR#", ra) && getSexa(fd, "#:GD#", dec));
}

bool getDegreesMinutes(const int fd, const char *cmd, int *d, int *m)
{
    *d = *m = 0;
    char response[16];
    bool success = getString(fd, cmd, response);
    if (success)
    {
        success = (sscanf(response, "%d%*c%d", d, m) == 2);
        if (success)
            DEBUGFDEVICE(lx200Name, DBG_SCOPE, "VAL [%+03d:%02d]", *d, *m);
        else
            DEBUGDEVICE(lx200Name, DBG_SCOPE, "Unable to parse response");
    }
    return success;
}

inline bool getSiteLatitude(const int fd, int *d, int *m)
{
    return getDegreesMinutes(fd, "#:Gt#", d, m);
}

inline bool getSiteLongitude(const int fd, int *d, int *m)
{
    return getDegreesMinutes(fd, "#:Gg#", d, m);
}

bool getUTCDate(const int fd, int *m, int *d, int *y)
{
    char response[12];
    bool success = getString(fd, "#:GC#", response);
    if (success)
    {
        success = (sscanf(response, "%2d%*c%2d%*c%2d", m, d, y) == 3);
        if (success)
        {
            *y += (*y < 50 ? 2000 : 1900);
            DEBUGFDEVICE(lx200Name, DBG_SCOPE, "VAL [%02d/%02d/%04d]", *m, *d, *y);
        }
        else
            DEBUGDEVICE(lx200Name, DBG_SCOPE, "Unable to parse date string");
    }
    return success;
}

bool getUTCTime(const int fd, int *h, int *m, int *s)
{
    char response[12];
    bool success = getString(fd, "#:GL#", response);
    if (success)
    {
        success = (sscanf(response, "%2d%*c%2d%*c%2d", h, m, s) == 3);
        if (success)
            DEBUGFDEVICE(lx200Name, DBG_SCOPE, "VAL [%02d:%02d:%02d]", *h, *m, *s);
        else
            DEBUGDEVICE(lx200Name, DBG_SCOPE, "Unable to parse time string");
    }
    return success;
}

bool setDegreesMinutes(const int fd, const char *cmd, const double value)
{
    int degrees, minutes, seconds;
    getSexComponents(value, &degrees, &minutes, &seconds);
    char full_cmd[32];
    snprintf(full_cmd, sizeof(full_cmd), "#:%s %03d:%02d#", cmd, degrees, minutes);
    char response;
    return (confirmed(fd, full_cmd, response) && response == '1');
}

inline bool setSite(const int fd, const double longitude, const double latitude)
{
    return (setDegreesMinutes(fd, "Sl", 360.0 - longitude) && setDegreesMinutes(fd, "St", latitude));
}

enum SlewMode
{
    SlewMax = 0,
    SlewFind,
    SlewCenter,
    SlewGuide,
    NumSlewRates
};

bool setSlewMode(const int fd, const SlewMode slewMode)
{
    static const char *commands[NumSlewRates]{ "#:RS#", "#:RM#", "#:RC#", "#:RG#" };
    return send(fd, commands[slewMode]);
}

enum Direction
{
    North = 0,
    East,
    South,
    West,
    NumDirections
};
static const char *DirectionName[NumDirections] = { "North", "East", "South", "West" };

bool moveTo(const int fd, const Direction direction)
{
    static const char *commands[NumDirections] = { "#:Mn#", "#:Me#", "#:Ms#", "#:Mw#" };
    return send(fd, commands[direction]);
}

bool haltMovement(const int fd, const Direction direction)
{
    static const char *commands[NumDirections] = { "#:Qn#", "#:Qe#", "#:Qs#", "#:Qw#" };
    return send(fd, commands[direction]);
}

inline bool startSlew(const int fd)
{
    char response[4];
    const bool success = (getString(fd, "#:MS#", response) && response[0] == '0');
    return success;
}

inline bool abortSlew(const int fd)
{
    return send(fd, "#:Q#");
}

// Pulse guide commands are only supported by the Pulsar2 controller and not the older Pulsar controller
bool pulseGuide(const int fd, const Direction direction, const float ms)
{
    static const char code[NumDirections] = { 'n', 'e', 's', 'w' };
    const int pulse                       = std::min(std::max(1, static_cast<int>(1000.0 * (ms + 0.5))), 999);
    char full_cmd[16];
    snprintf(full_cmd, sizeof(full_cmd), "#:MG%c%03d3#", code[direction], pulse);
    return send(fd, full_cmd);
}

bool setTime(const int fd, const int h, const int m, const int s)
{
    char full_cmd[32];
    snprintf(full_cmd, sizeof(full_cmd), "#:SL %02d:%02d:%02d#", h, m, s);
    char response;
    return (confirmed(fd, full_cmd, response) && response == '1');
}

bool setDate(const int fd, const int dd, const int mm, const int yy)
{
    char cmd[64];
    snprintf(cmd, sizeof(cmd), ":SC %02d/%02d/%02d#", mm, dd, (yy % 100));
    char response;
    const bool success = (confirmed(fd, cmd, response) && response == '1');
    if (success)
    {
        // Read dumped data
        char dumpPlanetaryUpdateString[64];
        int nbytes_read = 0;

        (void)tty_read_section(fd, dumpPlanetaryUpdateString, Termination, 1, &nbytes_read);
        (void)tty_read_section(fd, dumpPlanetaryUpdateString, Termination, 1, &nbytes_read);
    }
    return success;
}

bool ensureLongFormat(const int fd)
{
    char response[16] = { 0 };
    bool success      = getString(fd, "#:GR#", response);
    if (response[5] == '.')
    {
        // In case of short format, set long format
        success = (confirmed(fd, "#:U#", response[0]) && response[0] == '1');
    }
    return success;
}

bool setObjectRA(const int fd, const double ra)
{
    int h, m, s;
    getSexComponents(ra, &h, &m, &s);
    char full_cmd[32];
    snprintf(full_cmd, sizeof(full_cmd), "#:Sr %02d:%02d:%02d#", h, m, s);
    char response;
    return (confirmed(fd, full_cmd, response) && response == '1');
}

bool setObjectDEC(const int fd, const double dec)
{
    int d, m, s;
    getSexComponents(dec, &d, &m, &s);
    char full_cmd[32];
    snprintf(full_cmd, sizeof(full_cmd), "#:Sd %+03d:%02d:%02d#", d, m, s);
    char response;
    return (confirmed(fd, full_cmd, response) && response == '1');
}

inline bool setObjectRADec(const int fd, const double ra, const double dec)
{
    return (setObjectRA(fd, ra) && setObjectDEC(fd, dec));
}

inline bool park(const int fd)
{
    int success = 0;
    return (getInt(fd, "#:YH#", &success) && success == 1);
}

inline bool unpark(const int fd)
{
    int success = 0;
    return (getInt(fd, "#:YL#", &success) && success == 1);
}

inline bool sync(const int fd)
{
    return send(fd, "#:CM#");
}

static const char OnOff[2] = { '0', '1' };

bool setSideOfPier(const int fd, const SideOfPier side_of_pier)
{
    static char cmd[] = "#:YSN_#";
    cmd[5]            = OnOff[side_of_pier];
    char response;
    return (confirmed(fd, cmd, response) && response == '1');
}

bool setPECorrection(const int fd, const PECorrection pec_ra, const PECorrection pec_dec)
{
    static char cmd[] = "#:YSP_,_#";
    cmd[5]            = OnOff[pec_ra];
    cmd[7]            = OnOff[pec_dec];
    char response;
    return (confirmed(fd, cmd, response) && response == '1');
}

bool setPoleCrossing(const int fd, const PoleCrossing pole_crossing)
{
    static char cmd[] = "#:YSQ_#";
    cmd[5]            = OnOff[pole_crossing];
    char response;
    return (confirmed(fd, cmd, response) && response == '1');
}

bool setRCorrection(const int fd, const RCorrection rc_ra, const RCorrection rc_dec)
{
    static char cmd[] = "#:YSR_,_#";
    cmd[5]            = OnOff[rc_ra];
    cmd[7]            = OnOff[rc_dec];
    char response;
    return (confirmed(fd, cmd, response) && response == '1');
}

inline bool isHomeSet(const int fd)
{
    int is_home_set = -1;
    return (getInt(fd, "#:YGh#", &is_home_set) && is_home_set == 1);
}

inline bool isParked(const int fd)
{
    int is_parked = -1;
    return (getInt(fd, "#:YGk#", &is_parked) && is_parked == 1);
}

inline bool isParking(const int fd)
{
    int is_parking = -1;
    return (getInt(fd, "#:YGj#", &is_parking) && is_parking == 1);
}
};

LX200Pulsar2::LX200Pulsar2() : LX200Generic(), just_started_slewing(false)
{
    setVersion(1, 1);
    setLX200Capability(0);

    SetTelescopeCapability(TELESCOPE_CAN_SYNC | TELESCOPE_CAN_GOTO | TELESCOPE_CAN_PARK | TELESCOPE_CAN_ABORT |
                           TELESCOPE_HAS_TIME | TELESCOPE_HAS_LOCATION | TELESCOPE_HAS_PIER_SIDE, 4);
}

const char *LX200Pulsar2::getDefaultName()
{
    return static_cast<const char *>("Pulsar2");
}

bool LX200Pulsar2::Connect()
{
    const bool success = INDI::Telescope::Connect();
    if (success)
    {
        if (isParked())
        {
            DEBUGF(INDI::Logger::DBG_DEBUG, "%s", "Trying to wake up the mount.");
            UnPark();
        }
        else
            DEBUGF(INDI::Logger::DBG_DEBUG, "%s", "The mount is already tracking.");
    }
    return success;
}

bool LX200Pulsar2::Handshake()
{
    // Anything needs to be done besides this? INDI::Telescope would call ReadScopeStatus but
    // maybe we need to UnPark() before ReadScopeStatus() can return valid results?
    return true;
}

bool LX200Pulsar2::ReadScopeStatus()
{
    bool success = isConnected();
    if (success)
    {
        success = isSimulation();
        if (success)
            mountSim();
        else
        {
            switch (TrackState)
            {
                case SCOPE_SLEWING:
                    // Check if LX200 is done slewing
                    if (isSlewComplete())
                    {
                        // Set slew mode to "Centering"
                        IUResetSwitch(&SlewRateSP);
                        SlewRateS[SLEW_CENTERING].s = ISS_ON;
                        IDSetSwitch(&SlewRateSP, nullptr);
                        TrackState = SCOPE_TRACKING;
                        IDMessage(getDeviceName(), "Slew is complete. Tracking...");
                    }
                    break;

                case SCOPE_PARKING:
                    if (isSlewComplete())
                        SetParked(true);
                    break;

                default:
                    break;
            }
            success = Pulsar2Commands::getObjectRADec(PortFD, &currentRA, &currentDEC);
            if (success)
                NewRaDec(currentRA, currentDEC);
            else
            {
                EqNP.s = IPS_ALERT;
                IDSetNumber(&EqNP, "Error reading RA/DEC.");
            }
        }
    }

    Pulsar2Commands::SideOfPier side_of_pier = Pulsar2Commands::EastOfPier;
    if (Pulsar2Commands::getSideOfPier(PortFD, &side_of_pier))
    {
        //PierSideS[side_of_pier].s = ISS_ON;
        //IDSetSwitch(&PierSideSP, nullptr);
        setPierSide((side_of_pier == Pulsar2Commands::EastOfPier) ? PIER_EAST : PIER_WEST);
    }
    else
    {
        PierSideSP.s = IPS_ALERT;
        IDSetSwitch(&PierSideSP, "Can't check at which side of the pier the telescope is.");
    }

    return success;
}

void LX200Pulsar2::ISGetProperties(const char *dev)
{
    if (dev != nullptr && strcmp(dev, getDeviceName()) != 0)
        return;
    LX200Generic::ISGetProperties(dev);
    if (isConnected())
    {
        defineSwitch(&PeriodicErrorCorrectionSP);
        defineSwitch(&PoleCrossingSP);
        defineSwitch(&RefractionCorrectionSP);
    }
}

bool LX200Pulsar2::initProperties()
{
    const bool result = LX200Generic::initProperties();
    if (result)
    {
        IUFillSwitch(&PeriodicErrorCorrectionS[0], "PEC_OFF", "Off", ISS_OFF);
        IUFillSwitch(&PeriodicErrorCorrectionS[1], "PEC_ON", "On", ISS_ON);
        IUFillSwitchVector(&PeriodicErrorCorrectionSP, PeriodicErrorCorrectionS, 2, getDeviceName(), "PE_CORRECTION",
                           "P.E. Correction", MAIN_CONTROL_TAB, IP_RW, ISR_1OFMANY, 0, IPS_IDLE);
        IUFillSwitch(&PoleCrossingS[0], "POLE_CROSS_OFF", "Off", ISS_OFF);
        IUFillSwitch(&PoleCrossingS[1], "POLE_CROSS_ON", "On", ISS_ON);
        IUFillSwitchVector(&PoleCrossingSP, PoleCrossingS, 2, getDeviceName(), "POLE_CROSSING", "Pole Crossing",
                           MAIN_CONTROL_TAB, IP_RW, ISR_1OFMANY, 0, IPS_IDLE);
        IUFillSwitch(&RefractionCorrectionS[0], "REFR_CORR_OFF", "Off", ISS_OFF);
        IUFillSwitch(&RefractionCorrectionS[1], "REFR_CORR_ON", "On", ISS_ON);
        IUFillSwitchVector(&RefractionCorrectionSP, RefractionCorrectionS, 2, getDeviceName(), "REFR_CORRECTION",
                           "Refraction Corr.", MAIN_CONTROL_TAB, IP_RW, ISR_1OFMANY, 0, IPS_IDLE);

        // PierSide property is RW here so we override
        PierSideSP.p = IP_RW;
    }
    return result;
}

bool LX200Pulsar2::updateProperties()
{
    LX200Generic::updateProperties();

    if (isConnected())
    {
        defineSwitch(&PeriodicErrorCorrectionSP);
        defineSwitch(&PoleCrossingSP);
        defineSwitch(&RefractionCorrectionSP);
        getBasicData();
    }
    else
    {
        deleteProperty(PeriodicErrorCorrectionSP.name);
        deleteProperty(PoleCrossingSP.name);
        deleteProperty(RefractionCorrectionSP.name);
    }

    return true;
}

bool LX200Pulsar2::ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    if (dev != nullptr && strcmp(dev, getDeviceName()) == 0)
    {
        if (strcmp(name, PierSideSP.name) == 0)
        {
            if (IUUpdateSwitch(&PierSideSP, states, names, n) < 0)
                return false;

            if (!isSimulation())
            {
                // Define which side of the pier the telescope is.
                // Required for the sync command. This is *not* related to a meridian flip.
                const bool success = Pulsar2Commands::setSideOfPier(
                    PortFD, (PierSideS[1].s == ISS_ON ? Pulsar2Commands::WestOfPier : Pulsar2Commands::EastOfPier));
                if (success)
                {
                    PierSideSP.s = IPS_OK;
                    IDSetSwitch(&PierSideSP, nullptr);
                }
                else
                {
                    PierSideSP.s = IPS_ALERT;
                    IDSetSwitch(&PierSideSP, "Could not set side of mount");
                }
                return success;
            }
        }

        if (strcmp(name, PeriodicErrorCorrectionSP.name) == 0)
        {
            if (IUUpdateSwitch(&PeriodicErrorCorrectionSP, states, names, n) < 0)
                return false;

            if (!isSimulation())
            {
                // Only control PEC in RA. PEC in decl. doesn't seem usefull.
                const bool success = Pulsar2Commands::setPECorrection(PortFD,
                                                                      (PeriodicErrorCorrectionS[1].s == ISS_ON ?
                                                                           Pulsar2Commands::PECorrectionOn :
                                                                           Pulsar2Commands::PECorrectionOff),
                                                                      Pulsar2Commands::PECorrectionOff);
                if (success)
                {
                    PeriodicErrorCorrectionSP.s = IPS_OK;
                    IDSetSwitch(&PeriodicErrorCorrectionSP, nullptr);
                }
                else
                {
                    PeriodicErrorCorrectionSP.s = IPS_ALERT;
                    IDSetSwitch(&PeriodicErrorCorrectionSP, "Could not change the periodic error correction");
                }
                return success;
            }
        }

        if (strcmp(name, PoleCrossingSP.name) == 0)
        {
            if (IUUpdateSwitch(&PoleCrossingSP, states, names, n) < 0)
                return false;

            if (!isSimulation())
            {
                const bool success = Pulsar2Commands::setPoleCrossing(PortFD, (PoleCrossingS[1].s == ISS_ON ?
                                                                                   Pulsar2Commands::PoleCrossingOn :
                                                                                   Pulsar2Commands::PoleCrossingOff));
                if (success)
                {
                    PoleCrossingSP.s = IPS_OK;
                    IDSetSwitch(&PoleCrossingSP, nullptr);
                }
                else
                {
                    PoleCrossingSP.s = IPS_ALERT;
                    IDSetSwitch(&PoleCrossingSP, "Could not change the pole crossing");
                }
                return success;
            }
        }

        if (strcmp(name, RefractionCorrectionSP.name) == 0)
        {
            if (IUUpdateSwitch(&RefractionCorrectionSP, states, names, n) < 0)
                return false;

            if (!isSimulation())
            {
                // Control refraction correction in both RA and decl.
                const Pulsar2Commands::RCorrection rc =
                    (RefractionCorrectionS[1].s == ISS_ON ? Pulsar2Commands::RCorrectionOn :
                                                            Pulsar2Commands::RCorrectionOff);
                const bool success = Pulsar2Commands::setRCorrection(PortFD, rc, rc);
                if (success)
                {
                    RefractionCorrectionSP.s = IPS_OK;
                    IDSetSwitch(&RefractionCorrectionSP, nullptr);
                }
                else
                {
                    RefractionCorrectionSP.s = IPS_ALERT;
                    IDSetSwitch(&RefractionCorrectionSP, "Could not change the refraction correction");
                }
                return success;
            }
        }
    }
    //  Nobody has claimed this, so pass it to the parent
    return LX200Generic::ISNewSwitch(dev, name, states, names, n);
}

bool LX200Pulsar2::ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n)
{
    if (dev != nullptr && strcmp(dev, getDeviceName()) == 0)
    {
        // Nothing to do yet
    }
    return LX200Generic::ISNewText(dev, name, texts, names, n);
}

bool LX200Pulsar2::SetSlewRate(int index)
{
    // Convert index to Meade format
    index = 3 - index;
    const bool success =
        (isSimulation() || Pulsar2Commands::setSlewMode(PortFD, static_cast<Pulsar2Commands::SlewMode>(index)));
    if (success)
    {
        SlewRateSP.s = IPS_OK;
        IDSetSwitch(&SlewRateSP, nullptr);
    }
    else
    {
        SlewRateSP.s = IPS_ALERT;
        IDSetSwitch(&SlewRateSP, "Error setting slew mode.");
    }
    return success;
}

bool LX200Pulsar2::MoveNS(INDI_DIR_NS dir, TelescopeMotionCommand command)
{
    const Pulsar2Commands::Direction current_move =
        (dir == DIRECTION_NORTH ? Pulsar2Commands::North : Pulsar2Commands::South);
    bool success = true;
    switch (command)
    {
        case MOTION_START:
            success = (isSimulation() || Pulsar2Commands::moveTo(PortFD, current_move));
            if (success)
                DEBUGF(INDI::Logger::DBG_SESSION, "Moving toward %s.", Pulsar2Commands::DirectionName[current_move]);
            else
                DEBUG(INDI::Logger::DBG_ERROR, "Error starting N/S motion.");
            break;
        case MOTION_STOP:
            success = (isSimulation() || Pulsar2Commands::haltMovement(PortFD, current_move));
            if (success)
                DEBUGF(INDI::Logger::DBG_SESSION, "Movement toward %s halted.",
                       Pulsar2Commands::DirectionName[current_move]);
            else
                DEBUG(INDI::Logger::DBG_ERROR, "Error stopping N/S motion.");
            break;
    }
    return success;
}

bool LX200Pulsar2::MoveWE(INDI_DIR_WE dir, TelescopeMotionCommand command)
{
    const Pulsar2Commands::Direction current_move =
        (dir == DIRECTION_WEST ? Pulsar2Commands::West : Pulsar2Commands::East);
    bool success = true;
    switch (command)
    {
        case MOTION_START:
            success = (isSimulation() || Pulsar2Commands::moveTo(PortFD, current_move));
            if (success)
                DEBUGF(INDI::Logger::DBG_SESSION, "Moving toward %s.", Pulsar2Commands::DirectionName[current_move]);
            else
                DEBUG(INDI::Logger::DBG_ERROR, "Error starting W/E motion.");
            break;
        case MOTION_STOP:
            success = (isSimulation() || Pulsar2Commands::haltMovement(PortFD, current_move));
            if (success)
                DEBUGF(INDI::Logger::DBG_SESSION, "Movement toward %s halted.",
                       Pulsar2Commands::DirectionName[current_move]);
            else
                DEBUG(INDI::Logger::DBG_ERROR, "Error stopping W/E motion.");
            break;
    }
    return success;
}

bool LX200Pulsar2::Abort()
{
    const bool success = (isSimulation() || Pulsar2Commands::abortSlew(PortFD));
    if (success)
    {
        if (GuideNSNP.s == IPS_BUSY || GuideWENP.s == IPS_BUSY)
        {
            GuideNSNP.s = GuideWENP.s = IPS_IDLE;
            GuideNSN[0].value = GuideNSN[1].value = 0.0;
            GuideWEN[0].value = GuideWEN[1].value = 0.0;
            if (GuideNSTID)
            {
                IERmTimer(GuideNSTID);
                GuideNSTID = 0;
            }
            if (GuideWETID)
            {
                IERmTimer(GuideWETID);
                GuideNSTID = 0;
            }
            IDMessage(getDeviceName(), "Guide aborted.");
            IDSetNumber(&GuideNSNP, nullptr);
            IDSetNumber(&GuideWENP, nullptr);
        }
    }
    else
        DEBUG(INDI::Logger::DBG_ERROR, "Failed to abort slew.");
    return success;
}

IPState LX200Pulsar2::GuideNorth(float ms)
{
    const int use_pulse_cmd = IUFindOnSwitchIndex(&UsePulseCmdSP);
    if (!use_pulse_cmd && (MovementNSSP.s == IPS_BUSY || MovementWESP.s == IPS_BUSY))
    {
        DEBUG(INDI::Logger::DBG_ERROR, "Cannot guide while moving.");
        return IPS_ALERT;
    }
    // If already moving (no pulse command), then stop movement
    if (MovementNSSP.s == IPS_BUSY)
    {
        const int dir = IUFindOnSwitchIndex(&MovementNSSP);
        MoveNS(dir == 0 ? DIRECTION_NORTH : DIRECTION_SOUTH, MOTION_STOP);
    }
    if (GuideNSTID)
    {
        IERmTimer(GuideNSTID);
        GuideNSTID = 0;
    }
    if (use_pulse_cmd)
        (void)Pulsar2Commands::pulseGuide(PortFD, Pulsar2Commands::North, ms);
    else
    {
        if (!Pulsar2Commands::setSlewMode(PortFD, Pulsar2Commands::SlewGuide))
        {
            SlewRateSP.s = IPS_ALERT;
            IDSetSwitch(&SlewRateSP, "Error setting slew mode.");
            return IPS_ALERT;
        }
        MovementNSS[0].s = ISS_ON;
        MoveNS(DIRECTION_NORTH, MOTION_START);
    }
    // Set slew to guiding
    IUResetSwitch(&SlewRateSP);
    SlewRateS[SLEW_GUIDE].s = ISS_ON;
    IDSetSwitch(&SlewRateSP, nullptr);
    guide_direction = LX200_NORTH;
    GuideNSTID      = IEAddTimer(ms, guideTimeoutHelper, this);
    return IPS_BUSY;
}

IPState LX200Pulsar2::GuideSouth(float ms)
{
    const int use_pulse_cmd = IUFindOnSwitchIndex(&UsePulseCmdSP);
    if (!use_pulse_cmd && (MovementNSSP.s == IPS_BUSY || MovementWESP.s == IPS_BUSY))
    {
        DEBUG(INDI::Logger::DBG_ERROR, "Cannot guide while moving.");
        return IPS_ALERT;
    }
    // If already moving (no pulse command), then stop movement
    if (MovementNSSP.s == IPS_BUSY)
    {
        const int dir = IUFindOnSwitchIndex(&MovementNSSP);
        MoveNS(dir == 0 ? DIRECTION_NORTH : DIRECTION_SOUTH, MOTION_STOP);
    }
    if (GuideNSTID)
    {
        IERmTimer(GuideNSTID);
        GuideNSTID = 0;
    }
    if (use_pulse_cmd)
        (void)Pulsar2Commands::pulseGuide(PortFD, Pulsar2Commands::South, ms);
    else
    {
        if (!Pulsar2Commands::setSlewMode(PortFD, Pulsar2Commands::SlewGuide))
        {
            SlewRateSP.s = IPS_ALERT;
            IDSetSwitch(&SlewRateSP, "Error setting slew mode.");
            return IPS_ALERT;
        }
        MovementNSS[1].s = ISS_ON;
        MoveNS(DIRECTION_SOUTH, MOTION_START);
    }
    // Set slew to guiding
    IUResetSwitch(&SlewRateSP);
    SlewRateS[SLEW_GUIDE].s = ISS_ON;
    IDSetSwitch(&SlewRateSP, nullptr);
    guide_direction = LX200_SOUTH;
    GuideNSTID      = IEAddTimer(ms, guideTimeoutHelper, this);
    return IPS_BUSY;
}

IPState LX200Pulsar2::GuideEast(float ms)
{
    const int use_pulse_cmd = IUFindOnSwitchIndex(&UsePulseCmdSP);
    if (!use_pulse_cmd && (MovementNSSP.s == IPS_BUSY || MovementWESP.s == IPS_BUSY))
    {
        DEBUG(INDI::Logger::DBG_ERROR, "Cannot guide while moving.");
        return IPS_ALERT;
    }
    // If already moving (no pulse command), then stop movement
    if (MovementWESP.s == IPS_BUSY)
    {
        const int dir = IUFindOnSwitchIndex(&MovementWESP);
        MoveWE(dir == 0 ? DIRECTION_WEST : DIRECTION_EAST, MOTION_STOP);
    }
    if (GuideWETID)
    {
        IERmTimer(GuideWETID);
        GuideWETID = 0;
    }
    if (use_pulse_cmd)
        (void)Pulsar2Commands::pulseGuide(PortFD, Pulsar2Commands::East, ms);
    else
    {
        if (!Pulsar2Commands::setSlewMode(PortFD, Pulsar2Commands::SlewGuide))
        {
            SlewRateSP.s = IPS_ALERT;
            IDSetSwitch(&SlewRateSP, "Error setting slew mode.");
            return IPS_ALERT;
        }
        MovementWES[1].s = ISS_ON;
        MoveWE(DIRECTION_EAST, MOTION_START);
    }
    // Set slew to guiding
    IUResetSwitch(&SlewRateSP);
    SlewRateS[SLEW_GUIDE].s = ISS_ON;
    IDSetSwitch(&SlewRateSP, nullptr);
    guide_direction = LX200_EAST;
    GuideWETID      = IEAddTimer(ms, guideTimeoutHelper, this);
    return IPS_BUSY;
}

IPState LX200Pulsar2::GuideWest(float ms)
{
    const int use_pulse_cmd = IUFindOnSwitchIndex(&UsePulseCmdSP);
    if (!use_pulse_cmd && (MovementNSSP.s == IPS_BUSY || MovementWESP.s == IPS_BUSY))
    {
        DEBUG(INDI::Logger::DBG_ERROR, "Cannot guide while moving.");
        return IPS_ALERT;
    }
    // If already moving (no pulse command), then stop movement
    if (MovementWESP.s == IPS_BUSY)
    {
        const int dir = IUFindOnSwitchIndex(&MovementWESP);
        MoveWE(dir == 0 ? DIRECTION_WEST : DIRECTION_EAST, MOTION_STOP);
    }
    if (GuideWETID)
    {
        IERmTimer(GuideWETID);
        GuideWETID = 0;
    }
    if (use_pulse_cmd)
        (void)Pulsar2Commands::pulseGuide(PortFD, Pulsar2Commands::West, ms);
    else
    {
        if (!Pulsar2Commands::setSlewMode(PortFD, Pulsar2Commands::SlewGuide))
        {
            SlewRateSP.s = IPS_ALERT;
            IDSetSwitch(&SlewRateSP, "Error setting slew mode.");
            return IPS_ALERT;
        }
        MovementWES[0].s = ISS_ON;
        MoveWE(DIRECTION_WEST, MOTION_START);
    }
    // Set slew to guiding
    IUResetSwitch(&SlewRateSP);
    SlewRateS[SLEW_GUIDE].s = ISS_ON;
    IDSetSwitch(&SlewRateSP, nullptr);
    guide_direction = LX200_WEST;
    GuideWETID      = IEAddTimer(ms, guideTimeoutHelper, this);
    return IPS_BUSY;
}

bool LX200Pulsar2::updateTime(ln_date *utc, double utc_offset)
{
    INDI_UNUSED(utc_offset);
    bool success = true;
    if (!isSimulation())
    {
        struct ln_zonedate ltm;
        ln_date_to_zonedate(utc, &ltm, 0.0); // One should use UTC only with Pulsar!
        JD = ln_get_julian_day(utc);
        DEBUGF(INDI::Logger::DBG_DEBUG, "New JD is %f", static_cast<float>(JD));
        success = Pulsar2Commands::setTime(PortFD, ltm.hours, ltm.minutes, ltm.seconds);
        if (success)
        {
            success = Pulsar2Commands::setDate(PortFD, ltm.days, ltm.months, ltm.years);
            if (success)
                DEBUG(INDI::Logger::DBG_SESSION, "Time updated, updating planetary data...");
            else
                DEBUG(INDI::Logger::DBG_ERROR, "Error setting UTC date.");
        }
        else
            DEBUG(INDI::Logger::DBG_ERROR, "Error setting UTC time.");
        // Pulsar cannot set UTC offset (?)
    }
    return success;
}

bool LX200Pulsar2::updateLocation(double latitude, double longitude, double elevation)
{
    INDI_UNUSED(elevation);
    bool success = true;
    if (!isSimulation())
    {
        success = Pulsar2Commands::setSite(PortFD, longitude, latitude);
        if (success)
        {
            char l[32], L[32];
            fs_sexa(l, latitude, 3, 3600);
            fs_sexa(L, longitude, 4, 3600);
            IDMessage(getDeviceName(), "Site location updated to Lat %.32s - Long %.32s", l, L);
        }
        else
            DEBUG(INDI::Logger::DBG_ERROR, "Error setting site coordinates");
    }
    return success;
}

bool LX200Pulsar2::Goto(double r, double d)
{
    char RAStr[64], DecStr[64];
    fs_sexa(RAStr, targetRA = r, 2, 3600);
    fs_sexa(DecStr, targetDEC = d, 2, 3600);

    // If moving, let's stop it first.
    if (EqNP.s == IPS_BUSY)
    {
        if (!isSimulation() && !Pulsar2Commands::abortSlew(PortFD))
        {
            AbortSP.s = IPS_ALERT;
            IDSetSwitch(&AbortSP, "Abort slew failed.");
            return false;
        }

        AbortSP.s = IPS_OK;
        EqNP.s    = IPS_IDLE;
        IDSetSwitch(&AbortSP, "Slew aborted.");
        IDSetNumber(&EqNP, nullptr);

        if (MovementNSSP.s == IPS_BUSY || MovementWESP.s == IPS_BUSY)
        {
            MovementNSSP.s = MovementWESP.s = IPS_IDLE;
            EqNP.s                          = IPS_IDLE;
            IUResetSwitch(&MovementNSSP);
            IUResetSwitch(&MovementWESP);
            IDSetSwitch(&MovementNSSP, nullptr);
            IDSetSwitch(&MovementWESP, nullptr);
        }
        usleep(100000); // sleep for 100 mseconds
    }

    if (!isSimulation())
    {
        if (!Pulsar2Commands::setObjectRADec(PortFD, targetRA, targetDEC))
        {
            EqNP.s = IPS_ALERT;
            IDSetNumber(&EqNP, "Error setting RA/DEC.");
            return false;
        }
        if (!Pulsar2Commands::startSlew(PortFD))
        {
            EqNP.s = IPS_ALERT;
            IDSetNumber(&EqNP, "Error Slewing to JNow RA %s - DEC %s\n", RAStr, DecStr);
            slewError(3);
            return false;
        }
        just_started_slewing = true;
    }

    TrackState = SCOPE_SLEWING;
    EqNP.s     = IPS_BUSY;
    DEBUGF(INDI::Logger::DBG_SESSION, "Slewing to RA: %s - DEC: %s", RAStr, DecStr);
    return true;
}

bool LX200Pulsar2::Park()
{
    if (!isSimulation())
    {
        if (!Pulsar2Commands::isHomeSet(PortFD))
        {
            ParkSP.s = IPS_ALERT;
            IDSetSwitch(&ParkSP, "No parking position defined.");
            return false;
        }
        if (Pulsar2Commands::isParked(PortFD))
        {
            ParkSP.s = IPS_ALERT;
            IDSetSwitch(&ParkSP, "Scope has already been parked.");
            return false;
        }
    }

    // If scope is moving, let's stop it first.
    if (EqNP.s == IPS_BUSY)
    {
        if (!isSimulation() && !Pulsar2Commands::abortSlew(PortFD))
        {
            AbortSP.s = IPS_ALERT;
            IDSetSwitch(&AbortSP, "Abort slew failed.");
            return false;
        }

        AbortSP.s = IPS_OK;
        EqNP.s    = IPS_IDLE;
        IDSetSwitch(&AbortSP, "Slew aborted.");
        IDSetNumber(&EqNP, nullptr);

        if (MovementNSSP.s == IPS_BUSY || MovementWESP.s == IPS_BUSY)
        {
            MovementNSSP.s = MovementWESP.s = IPS_IDLE;
            EqNP.s                          = IPS_IDLE;
            IUResetSwitch(&MovementNSSP);
            IUResetSwitch(&MovementWESP);

            IDSetSwitch(&MovementNSSP, nullptr);
            IDSetSwitch(&MovementWESP, nullptr);
        }
        usleep(100000); // sleep for 100 msec
    }

    if (!isSimulation() && !Pulsar2Commands::park(PortFD))
    {
        ParkSP.s = IPS_ALERT;
        IDSetSwitch(&ParkSP, "Parking Failed.");
        return false;
    }
    ParkSP.s   = IPS_BUSY;
    TrackState = SCOPE_PARKING;
    IDMessage(getDeviceName(), "Parking telescope in progress...");
    return true;
}

bool LX200Pulsar2::Sync(double ra, double dec)
{
    bool result = true;
    if (!isSimulation())
    {
        result = Pulsar2Commands::setObjectRADec(PortFD, ra, dec);
        if (!result)
        {
            EqNP.s = IPS_ALERT;
            IDSetNumber(&EqNP, "Error setting RA/DEC. Unable to Sync.");
        }
        else
        {
            usleep(300000L); // This seems to be necessary
            result = Pulsar2Commands::sync(PortFD);
            if (result)
            {
                DEBUG(INDI::Logger::DBG_SESSION, "Reading sync response");
                // Pulsar sends coordinates separated by # characters (<RA>#<Dec>#)
                char RAresponse[Pulsar2Commands::BufferSize];
                result = Pulsar2Commands::receive(PortFD, RAresponse);
                if (result)
                {
                    DEBUGF(INDI::Logger::DBG_DEBUG, "First synchronization string: '%s'.", RAresponse);
                    char DECresponse[Pulsar2Commands::BufferSize];
                    result = Pulsar2Commands::receive(PortFD, DECresponse);
                    if (result)
                        DEBUGF(INDI::Logger::DBG_DEBUG, "Second synchronization string: '%s'.", DECresponse);
                }
                //TODO: Check that the received coordinates match the original coordinates
                if (!result)
                {
                    EqNP.s = IPS_ALERT;
                    IDSetNumber(&EqNP, "Synchronization failed.");
                }
            }
        }
    }
    if (result)
    {
        currentRA  = ra;
        currentDEC = dec;
        DEBUG(INDI::Logger::DBG_SESSION, "Synchronization successful.");
        EqNP.s     = IPS_OK;
        NewRaDec(currentRA, currentDEC);
    }
    return result;
}

bool LX200Pulsar2::UnPark()
{
    if (!isSimulation())
    {
        if (!Pulsar2Commands::isParked(PortFD))
        {
            ParkSP.s = IPS_ALERT;
            IDSetSwitch(&ParkSP, "Mount is not parked.");
            return false;
        }
        if (!Pulsar2Commands::unpark(PortFD))
        {
            ParkSP.s = IPS_ALERT;
            IDSetSwitch(&ParkSP, "Unparking failed.");
            return false;
        }
    }
    ParkSP.s   = IPS_OK;
    TrackState = SCOPE_IDLE;
    SetParked(false);
    IDMessage(getDeviceName(), "Telescope has been unparked.");
    return true;
}

bool LX200Pulsar2::isSlewComplete()
{
    bool result = false;
    switch (TrackState)
    {
        case SCOPE_SLEWING:
            result = !isSlewing();
            break;
        case SCOPE_PARKING:
            result = !Pulsar2Commands::isParking(PortFD);
            break;
        default:
            break;
    }
    return result;
}

bool LX200Pulsar2::checkConnection()
{
    if (isSimulation())
        return true;

    if (LX200Generic::checkConnection())
    {
        DEBUG(INDI::Logger::DBG_DEBUG, "Checking Pulsar version ...");
        for (int i = 0; i < 2; ++i)
        {
            char response[Pulsar2Commands::BufferSize];
            if (Pulsar2Commands::getVersion(PortFD, response))
            {
                // Determine which Pulsar version this is. Expected response similar to: 'PULSAR V2.66aR  ,2008.12.10.     #'
                char version[16];
                int year, month, day;
                (void)sscanf(response, "PULSAR V%8s ,%4d.%2d.%2d. ", version, &year, &month, &day);
                DEBUGF(INDI::Logger::DBG_SESSION, "%s version %s dated %04d.%02d.%02d",
                       (version[0] > '2' ? "Pulsar2" : "Pulsar"), version, year, month, day);
                return true;
            }
            usleep(50000);
        }
    }
    return false;
}

void LX200Pulsar2::getBasicData()
{
    if (!isSimulation())
    {
        if (!Pulsar2Commands::ensureLongFormat(PortFD))
        {
            DEBUG(INDI::Logger::DBG_DEBUG, "Failed to ensure that long format coordinates are used.");
        }
        if (!Pulsar2Commands::getObjectRADec(PortFD, &currentRA, &currentDEC))
        {
            EqNP.s = IPS_ALERT;
            IDSetNumber(&EqNP, "Error reading RA/DEC.");
            return;
        }
        NewRaDec(currentRA, currentDEC);

        Pulsar2Commands::SideOfPier side_of_pier = Pulsar2Commands::EastOfPier;
        if (Pulsar2Commands::getSideOfPier(PortFD, &side_of_pier))
        {
            //PierSideS[side_of_pier].s = ISS_ON;
            //IDSetSwitch(&PierSideSP, nullptr);
            setPierSide((side_of_pier == Pulsar2Commands::EastOfPier) ? PIER_EAST : PIER_WEST);
        }
        else
        {
            PierSideSP.s = IPS_ALERT;
            IDSetSwitch(&PierSideSP, "Can't check at which side of the pier the telescope is.");
        }

        // There are separate values for RA and DEC but we only use the RA value for now
        Pulsar2Commands::PECorrection pec_ra  = Pulsar2Commands::PECorrectionOff,
                                      pec_dec = Pulsar2Commands::PECorrectionOff;
        if (Pulsar2Commands::getPECorrection(PortFD, &pec_ra, &pec_dec))
        {
            PeriodicErrorCorrectionS[pec_ra].s = ISS_ON;
            IDSetSwitch(&PeriodicErrorCorrectionSP, nullptr);
        }
        else
        {
            PeriodicErrorCorrectionSP.s = IPS_ALERT;
            IDSetSwitch(&PeriodicErrorCorrectionSP, "Can't check whether PEC is enabled.");
        }

        Pulsar2Commands::PoleCrossing pole_crossing = Pulsar2Commands::PoleCrossingOff;
        if (Pulsar2Commands::getPoleCrossing(PortFD, &pole_crossing))
        {
            PoleCrossingS[pole_crossing].s = ISS_ON;
            IDSetSwitch(&PoleCrossingSP, nullptr);
        }
        else
        {
            PoleCrossingSP.s = IPS_ALERT;
            IDSetSwitch(&PoleCrossingSP, "Can't check whether pole crossing is enabled.");
        }

        // There are separate values for RA and DEC but we only use the RA value for now
        Pulsar2Commands::RCorrection rc_ra = Pulsar2Commands::RCorrectionOff, rc_dec = Pulsar2Commands::RCorrectionOn;
        if (Pulsar2Commands::getRCorrection(PortFD, &rc_ra, &rc_dec))
        {
            RefractionCorrectionS[rc_ra].s = ISS_ON;
            IDSetSwitch(&RefractionCorrectionSP, nullptr);
        }
        else
        {
            RefractionCorrectionSP.s = IPS_ALERT;
            IDSetSwitch(&RefractionCorrectionSP, "Can't check whether refraction correction is enabled.");
        }
    }
    sendScopeLocation();
    sendScopeTime();
}

void LX200Pulsar2::sendScopeLocation()
{
    LocationNP.s = IPS_OK;
    int dd = 29, mm = 30;
    if (isSimulation() || Pulsar2Commands::getSiteLatitude(PortFD, &dd, &mm))
    {
        LocationNP.np[0].value = (dd < 0 ? -1 : 1) * (abs(dd) + mm / 60.0);
        if (isDebug())
        {
            IDLog("Pulsar latitude: %d:%d\n", dd, mm);
            IDLog("INDI Latitude: %g\n", LocationNP.np[0].value);
        }
    }
    else
    {
        IDMessage(getDeviceName(), "Failed to get site latitude from Pulsar controller.");
        LocationNP.s = IPS_ALERT;
    }
    dd = 48, mm = 0;
    if (isSimulation() || Pulsar2Commands::getSiteLongitude(PortFD, &dd, &mm))
    {
        LocationNP.np[1].value = (dd > 0 ? 360.0 - (dd + mm / 60.0) : -(dd - mm / 60.0));
        if (isDebug())
        {
            IDLog("Pulsar longitude: %d:%d\n", dd, mm);
            IDLog("INDI Longitude: %g\n", LocationNP.np[1].value);
        }
    }
    else
    {
        IDMessage(getDeviceName(), "Failed to get site longitude from Pulsar controller.");
        LocationNP.s = IPS_ALERT;
    }
    IDSetNumber(&LocationNP, nullptr);
}

void LX200Pulsar2::sendScopeTime()
{
    struct tm ltm;
    if (isSimulation())
    {
        const time_t t = time(nullptr);
        if (gmtime_r(&t, &ltm) == nullptr)
            return;
    }
    else
    {
        if (!Pulsar2Commands::getUTCTime(PortFD, &ltm.tm_hour, &ltm.tm_min, &ltm.tm_sec) ||
            !Pulsar2Commands::getUTCDate(PortFD, &ltm.tm_mon, &ltm.tm_mday, &ltm.tm_year))
            return;
        ltm.tm_mon -= 1;
        ltm.tm_year -= 1900;
    }

    // Get time epoch and convert to TimeT
    const time_t time_epoch = mktime(&ltm);
    struct tm utm;
    localtime_r(&time_epoch, &utm);

    // Format it into ISO 8601
    char cdate[32];
    strftime(cdate, sizeof(cdate), "%Y-%m-%dT%H:%M:%S", &utm);

    IUSaveText(&TimeT[0], cdate);
    IUSaveText(&TimeT[1], "0"); // Pulsar maintains time in UTC only
    if (isDebug())
    {
        IDLog("Telescope Local Time: %02d:%02d:%02d\n", ltm.tm_hour, ltm.tm_min, ltm.tm_sec);
        IDLog("Telescope TimeT Offset: %s\n", TimeT[1].text);
        IDLog("Telescope UTC Time: %s\n", TimeT[0].text);
    }
    // Let's send everything to the client
    IDSetText(&TimeTP, nullptr);
}

void LX200Pulsar2::guideTimeoutHelper(void *p)
{
    static_cast<LX200Pulsar2 *>(p)->guideTimeout();
}

void LX200Pulsar2::guideTimeout()
{
    const int use_pulse_cmd = IUFindOnSwitchIndex(&UsePulseCmdSP);
    if (guide_direction == -1)
    {
        Pulsar2Commands::haltMovement(PortFD, Pulsar2Commands::North);
        Pulsar2Commands::haltMovement(PortFD, Pulsar2Commands::South);
        Pulsar2Commands::haltMovement(PortFD, Pulsar2Commands::East);
        Pulsar2Commands::haltMovement(PortFD, Pulsar2Commands::West);
        MovementNSSP.s = IPS_IDLE;
        MovementWESP.s = IPS_IDLE;
        IUResetSwitch(&MovementNSSP);
        IUResetSwitch(&MovementWESP);
        IDSetSwitch(&MovementNSSP, nullptr);
        IDSetSwitch(&MovementWESP, nullptr);
        IERmTimer(GuideNSTID);
        IERmTimer(GuideWETID);
    }
    else if (!use_pulse_cmd)
    {
        switch (guide_direction)
        {
            case LX200_NORTH:
            case LX200_SOUTH:
                MoveNS(guide_direction == LX200_NORTH ? DIRECTION_NORTH : DIRECTION_SOUTH, MOTION_STOP);
                GuideNSNP.np[(guide_direction == LX200_NORTH ? 0 : 1)].value = 0;
                GuideNSNP.s                                                  = IPS_IDLE;
                IDSetNumber(&GuideNSNP, nullptr);
                MovementNSSP.s = IPS_IDLE;
                IUResetSwitch(&MovementNSSP);
                IDSetSwitch(&MovementNSSP, nullptr);
                break;
            case LX200_WEST:
            case LX200_EAST:
                MoveWE(guide_direction == LX200_WEST ? DIRECTION_WEST : DIRECTION_EAST, MOTION_STOP);
                GuideWENP.np[(guide_direction == LX200_WEST ? 0 : 1)].value = 0;
                GuideWENP.s                                                 = IPS_IDLE;
                IDSetNumber(&GuideWENP, nullptr);
                MovementWESP.s = IPS_IDLE;
                IUResetSwitch(&MovementWESP);
                IDSetSwitch(&MovementWESP, nullptr);
                break;
        }
    }
    if (guide_direction == LX200_NORTH || guide_direction == LX200_SOUTH || guide_direction == -1)
    {
        GuideNSNP.np[0].value = 0;
        GuideNSNP.np[1].value = 0;
        GuideNSNP.s           = IPS_IDLE;
        GuideNSTID            = 0;
        IDSetNumber(&GuideNSNP, nullptr);
    }
    if (guide_direction == LX200_WEST || guide_direction == LX200_EAST || guide_direction == -1)
    {
        GuideWENP.np[0].value = 0;
        GuideWENP.np[1].value = 0;
        GuideWENP.s           = IPS_IDLE;
        GuideWETID            = 0;
        IDSetNumber(&GuideWENP, nullptr);
    }
}

bool LX200Pulsar2::isSlewing()
{
    // A problem with the Pulsar controller is that the :YGi# command starts
    // returning the value 1 only a few seconds after a slew has been started.
    // This also means that a (short) slew can end before this happens.
    auto mount_is_off_target = [this](void) {
        return (fabs(currentRA - targetRA) > 1.0 / 3600.0 || fabs(currentDEC - targetDEC) > 5.0 / 3600.0);
    };
    // Detect the end of a short slew
    bool result = (just_started_slewing ? mount_is_off_target() : true);
    if (result)
    {
        int is_slewing = -1;
        if (Pulsar2Commands::getInt(PortFD, "#:YGi#", &is_slewing))
        {
            if (is_slewing ==
                1) // When the Pulsar controller indicates that it is slewing, we can rely on it from now on
                result = true, just_started_slewing = false;
            else // ... otherwise we have to rely on the value of the attribute just_started_slewing
                result = just_started_slewing;
        }
        else // Fallback in case of error
            result = mount_is_off_target();
    }
    // Make sure that just_started_slewing is reset at the end of a slew
    if (!result)
        just_started_slewing = false;
    return result;
}

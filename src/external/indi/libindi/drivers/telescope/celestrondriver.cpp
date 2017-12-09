/*
    Celestron driver

    Copyright (C) 2015 Jasem Mutlaq

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

/*
    Version with experimental pulse guide support. GC 04.12.2015
*/

#include "celestrondriver.h"

#include "indicom.h"
#include "indilogger.h"

#include <libnova/julian_day.h>

#include <map>

#include <cstring>
#include <cmath>
#include <termios.h>
#include <unistd.h>

#define CELESTRON_TIMEOUT 5 /* FD timeout in seconds */
std::map<int, std::string> celestronModels;

bool celestron_debug                 = false;
bool celestron_simulation            = false;
char celestron_device[MAXINDIDEVICE] = "Celestron GPS";
double currentRA, currentDEC, currentSlewRate;

struct
{
    double ra;
    double dec;
    double az;
    double alt;
    CELESTRON_GPS_STATUS gpsStatus;
    CELESTRON_SLEW_RATE slewRate;
    CELESTRON_TRACK_MODE trackMode;
    bool isSlewing;
} simData;

void set_celestron_debug(bool enable)
{
    celestron_debug = enable;
}

void set_celestron_simulation(bool enable)
{
    celestron_simulation = enable;
}

void set_celestron_device(const char *name)
{
    strncpy(celestron_device, name, MAXINDIDEVICE);
}

void set_sim_gps_status(CELESTRON_GPS_STATUS value)
{
    simData.gpsStatus = value;
}

void set_sim_slew_rate(CELESTRON_SLEW_RATE value)
{
    simData.slewRate = value;
}

void set_sim_track_mode(CELESTRON_TRACK_MODE value)
{
    simData.trackMode = value;
}

void set_sim_slewing(bool isSlewing)
{
    simData.isSlewing = isSlewing;
}

void set_sim_ra(double ra)
{
    simData.ra = ra;
}

double get_sim_ra()
{
    return simData.ra;
}

void set_sim_dec(double dec)
{
    simData.dec = dec;
}

double get_sim_dec()
{
    return simData.dec;
}

void set_sim_az(double az)
{
    simData.az = az;
}

void set_sim_alt(double alt)
{
    simData.alt = alt;
}

bool check_celestron_connection(int fd)
{
    char initCMD[] = "Kx";
    int errcode    = 0;
    char errmsg[MAXRBUF];
    char response[8];
    int nbytes_read    = 0;
    int nbytes_written = 0;

    if (celestronModels.empty())
    {
        celestronModels[1]  = "GPS Series";
        celestronModels[3]  = "i-Series";
        celestronModels[4]  = "i-Series SE";
        celestronModels[5]  = "CGE";
        celestronModels[6]  = "Advanced GT";
        celestronModels[7]  = "SLT";
        celestronModels[9]  = "CPC";
        celestronModels[10] = "GT";
        celestronModels[11] = "4/5 SE";
        celestronModels[12] = "6/8 SE";
        celestronModels[13] = "CGE Pro";
        celestronModels[14] = "CGEM DX";
        celestronModels[20] = "AVX";
    }

    DEBUGDEVICE(celestron_device, INDI::Logger::DBG_DEBUG, "Initializing Celestron using Kx CMD...");

    for (int i = 0; i < 2; i++)
    {
        if (celestron_simulation)
        {
            strcpy(response, "x#");
            nbytes_read = strlen(response);
        }
        else
        {
            tcflush(fd, TCIOFLUSH);

            if ((errcode = tty_write(fd, initCMD, 2, &nbytes_written)) != TTY_OK)
            {
                tty_error_msg(errcode, errmsg, MAXRBUF);
                DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_ERROR, "%s", errmsg);
                usleep(50000);
                continue;
            }

            if ((errcode = tty_read_section(fd, response, '#', CELESTRON_TIMEOUT, &nbytes_read)))
            {
                tty_error_msg(errcode, errmsg, MAXRBUF);
                DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_ERROR, "%s", errmsg);
                usleep(50000);
                continue;
            }
        }

        if (nbytes_read > 0)
        {
            response[nbytes_read] = '\0';
            DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_DEBUG, "RES <%s>", response);

            if (!strcmp(response, "x#"))
                return true;
        }

        usleep(50000);
    }

    return false;
}

bool get_celestron_firmware(int fd, FirmwareInfo *info)
{
    bool rc = false;

    DEBUGDEVICE(celestron_device, INDI::Logger::DBG_DEBUG, "Getting controller version...");
    rc = get_celestron_version(fd, info);

    if (!rc)
        return false;

    DEBUGDEVICE(celestron_device, INDI::Logger::DBG_DEBUG, "Getting controller variant...");
    get_celestron_variant(fd, info);

    if (((info->controllerVariant == ISSTARSENSE) &&
          info->controllerVersion >= MINSTSENSVER) ||
        (info->controllerVersion >= 2.2))
    {
        DEBUGDEVICE(celestron_device, INDI::Logger::DBG_DEBUG, "Getting controller model...");
        rc = get_celestron_model(fd, info);
        if (!rc)
            return rc;
    }
    else
        info->Model = "Unknown";

    DEBUGDEVICE(celestron_device, INDI::Logger::DBG_DEBUG, "Getting GPS firmware version...");
    rc = get_celestron_gps_firmware(fd, info);

    if (!rc)
        return rc;

    DEBUGDEVICE(celestron_device, INDI::Logger::DBG_DEBUG, "Getting RA firmware version...");
    rc = get_celestron_ra_firmware(fd, info);

    if (!rc)
        return rc;

    DEBUGDEVICE(celestron_device, INDI::Logger::DBG_DEBUG, "Getting DE firmware version...");
    rc = get_celestron_dec_firmware(fd, info);

    return rc;
}

bool get_celestron_version(int fd, FirmwareInfo *info)
{
    const char *cmd  = "V";
    int errcode = 0;
    char errmsg[MAXRBUF];
    char response[16];
    int nbytes_read    = 0;
    int nbytes_written = 0;

    DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_DEBUG, "CMD <%s>", cmd);

    if (celestron_simulation)
    {
        char major = 4;
        char minor = 4;
        snprintf(response, 16, "%c%c#", major, minor);
        nbytes_read = strlen(response);
    }
    else
    {
        tcflush(fd, TCIOFLUSH);

        if ((errcode = tty_write(fd, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }

        if ((errcode = tty_read_section(fd, response, '#', CELESTRON_TIMEOUT, &nbytes_read)))
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }
    }

    if (nbytes_read > 0)
    {
        response[nbytes_read] = '\0';
        DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_DEBUG, "RES (%#02X %#02X %#02X)", response[0], response[1],
                     response[2]);

        if (nbytes_read == 3)
        {
            int major = response[0];
            int minor = response[1];

            char versionStr[8];
            snprintf(versionStr, 8, "%01d.%01d", major, minor);

            info->controllerVersion = atof(versionStr);
            info->Version           = versionStr;

            DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_SESSION, "Controller version: %s", versionStr);

            return true;
        }
    }

    DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_ERROR, "Received #%d bytes, expected 3.", nbytes_read);
    return false;
}

bool get_celestron_variant (int fd, FirmwareInfo * info)
{
    const char *cmd = "v";
    int errcode = 0;
    char errmsg[MAXRBUF];
    char response[16];
    int nbytes_read = 0;
    int nbytes_written = 0;

    DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_DEBUG, "CMD <%s>", cmd);

    if (celestron_simulation)
    {
        char res = 0x11;
        snprintf(response, 16, "%c#", res);
        nbytes_read = strlen(response);
    }
    else
    {
        tcflush(fd, TCIOFLUSH);

        // No critical errors for this
        if ( (errcode = tty_write(fd, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_DEBUG, "%s", errmsg);
            return false;
        }

        // No critical errors for this
        if ( (errcode = tty_read_section(fd, response, '#', 1, &nbytes_read)))
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_DEBUG, "%s", errmsg);
            return false;
        }
    }

    if (nbytes_read > 0)
    {
        response[nbytes_read] = '\0';
        DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_DEBUG, "RES <%#02X %#02X>", response[0], response[1]);

        if (nbytes_read == 2)
        {
            info->controllerVariant = response[0];
            return true;
        }
    }

    DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_DEBUG, "Received #%d bytes, expected 2.", nbytes_read);
    return false;

}

bool get_celestron_model(int fd, FirmwareInfo *info)
{
    const char *cmd  = "m";
    int errcode = 0;
    char errmsg[MAXRBUF];
    char response[16];
    int nbytes_read    = 0;
    int nbytes_written = 0;

    DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_DEBUG, "CMD <%s>", cmd);

    if (celestron_simulation)
    {
        char device = 6;
        snprintf(response, 16, "%c#", device);
        nbytes_read = strlen(response);
    }
    else
    {
        tcflush(fd, TCIOFLUSH);

        if ((errcode = tty_write(fd, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }

        if ((errcode = tty_read_section(fd, response, '#', CELESTRON_TIMEOUT, &nbytes_read)))
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }
    }

    if (nbytes_read > 0)
    {
        response[nbytes_read] = '\0';
        DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_DEBUG, "RES <%#02X %#02X>", response[0], response[1]);

        if (nbytes_read == 2)
        {
            response[1] = '\0';
            int model   = response[0];

            if (celestronModels.find(model) != celestronModels.end())
                info->Model = celestronModels[model];
            else
            {
                DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_WARNING, "Unrecognized model (%d).", model);
                info->Model = "Unknown";
            }

            DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_SESSION, "Mount model: %s", info->Model.c_str());

            return true;
        }
    }

    DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_ERROR, "Received #%d bytes, expected 2.", nbytes_read);
    return false;
}

bool get_celestron_ra_firmware(int fd, FirmwareInfo *info)
{
    unsigned char cmd[] = { 0x50, 0x01, 0x10, 0xFE, 0x0, 0x0, 0x0, 0x02 };
    int errcode         = 0;
    char errmsg[MAXRBUF];
    char response[16];
    int nbytes_read    = 0;
    int nbytes_written = 0;

    DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_DEBUG, "CMD (%#02X %#02X %#02X %#02X %#02X %#02X %#02X %#02X)", cmd[0],
                 cmd[1], cmd[2], cmd[3], cmd[4], cmd[5], cmd[6], cmd[7]);

    if (celestron_simulation)
    {
        char major = 1;
        char minor = 9;
        snprintf(response, 16, "%c%c#", major, minor);
        nbytes_read = strlen(response);
    }
    else
    {
        tcflush(fd, TCIOFLUSH);

        if ((errcode = tty_write(fd, (char *)cmd, 8, &nbytes_written)) != TTY_OK)
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }

        if ((errcode = tty_read_section(fd, response, '#', CELESTRON_TIMEOUT, &nbytes_read)))
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }
    }

    if (nbytes_read > 0)
    {
        response[nbytes_read] = '\0';
        DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_DEBUG, "RES <%#02X %#02X %#02X>", response[0], response[1],
                     response[2]);

        if (nbytes_read == 3)
        {
            int major = response[0];
            int minor = response[1];

            char versionStr[8];
            snprintf(versionStr, 8, "%01d.%01d", major, minor);

            info->RAFirmware = versionStr;

            return true;
        }
    }

    DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_ERROR, "Received #%d bytes, expected 3.", nbytes_read);
    return false;
}

bool get_celestron_dec_firmware(int fd, FirmwareInfo *info)
{
    unsigned char cmd[] = { 0x50, 0x01, 0x11, 0xFE, 0x0, 0x0, 0x0, 0x02 };
    int errcode         = 0;
    char errmsg[MAXRBUF];
    char response[16];
    int nbytes_read    = 0;
    int nbytes_written = 0;

    DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_DEBUG, "CMD <%#02X %#02X %#02X %#02X %#02X %#02X %#02X %#02X>", cmd[0],
                 cmd[1], cmd[2], cmd[3], cmd[4], cmd[5], cmd[6], cmd[7]);

    if (celestron_simulation)
    {
        char major = 1;
        char minor = 6;
        snprintf(response, 16, "%c%c#", major, minor);
        nbytes_read = strlen(response);
    }
    else
    {
        tcflush(fd, TCIOFLUSH);

        if ((errcode = tty_write(fd, (char *)cmd, 8, &nbytes_written)) != TTY_OK)
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }

        if ((errcode = tty_read_section(fd, response, '#', CELESTRON_TIMEOUT, &nbytes_read)))
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }
    }

    if (nbytes_read > 0)
    {
        response[nbytes_read] = '\0';
        DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_DEBUG, "RES <%#02X %#02X %#02X>", response[0], response[1],
                     response[2]);

        if (nbytes_read == 3)
        {
            int major = response[0];
            int minor = response[1];

            char versionStr[8];
            snprintf(versionStr, 8, "%01d.%01d", major, minor);

            info->DEFirmware = versionStr;

            return true;
        }
    }

    DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_ERROR, "Received #%d bytes, expected 3.", nbytes_read);
    return false;
}

bool get_celestron_gps_firmware(int fd, FirmwareInfo *info)
{
    unsigned char cmd[] = { 0x50, 0x01, 0x10, 0xFE, 0x0, 0x0, 0x0, 0x02 };
    int errcode         = 0;
    char errmsg[MAXRBUF];
    char response[16] = {0};
    int nbytes_read    = 0;
    int nbytes_written = 0;

    DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_DEBUG, "CMD <%#02X %#02X %#02X %#02X %#02X %#02X %#02X %#02X>", cmd[0],
                 cmd[1], cmd[2], cmd[3], cmd[4], cmd[5], cmd[6], cmd[7]);

    if (celestron_simulation)
    {
        char major = 1;
        char minor = 6;
        snprintf(response, 16, "%c%c#", major, minor);
        nbytes_read = strlen(response);
    }
    else
    {
        tcflush(fd, TCIOFLUSH);

        if ((errcode = tty_write(fd, (char *)cmd, 8, &nbytes_written)) != TTY_OK)
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }

        if ((errcode = tty_read_section(fd, response, '#', CELESTRON_TIMEOUT, &nbytes_read)))
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }
    }

    if (nbytes_read > 0)
    {
        response[nbytes_read] = '\0';
        DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_DEBUG, "RES <%#02X %#02X %#02X>", response[0], response[1],
                     response[2]);

        if (nbytes_read == 3)
        {
            int major = response[0];
            int minor = response[1];

            char versionStr[8];
            snprintf(versionStr, 8, "%01d.%01d", major, minor);

            info->GPSFirmware = versionStr;
            return true;
        }
        // Some models return only 2 bytes
        else if (nbytes_read == 2)
        {
            int major = response[0];
            int minor = 0;

            char versionStr[8];
            snprintf(versionStr, 8, "%01d.%01d", major, minor);

            info->GPSFirmware = versionStr;
            return true;
        }
    }

    DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_ERROR, "Received #%d bytes, expected 3.", nbytes_read);
    return false;
}

/*****************************************************************
 PulseGuide commands, experimental:                              *
                                                                 *
     SendPulseCmd( int fd,                                       *
                   CELESTRON_DIRECTION direction,                *
                   signed char rate,                             *
                   unsigned char duration_csec );                *
                                                                 *
       Send a guiding pulse to the  mount connected to "fd",     *
       in direction "direction". "rate" should be a * signed     *
       8-bit integer in the range (-100,100) that represents     *
       the pulse  velocity in % of sidereal. "duration_csec"     *
       is an unsigned  8-bit integer (0,255) with  the pulse     *
       duration in centiseconds (i.e. 1/100 s  =  10ms). The     *
       max pulse duration is 2550 ms.                            *
                                                                 *
                                                                 *
     SendPulseStatusCmd( int fd,                                 *
                         CELESTRON_DIRECTION direction,          *
                         bool & pulse_state );                   *
                                                                 *
       Send the guiding pulse status check command to device     *
       "fd" for the motor responsible for "direction". If  a     *
       pulse is being executed, "pulse_state" is  set  to 1,     *
       whereas if the pulse motion has been  completed it is     *
       set to 0. Return "false" if the status command fails,     *
       otherwise return "true".                                  *
******************************************************************/

int SendPulseCmd(int fd, CELESTRON_DIRECTION direction, signed char rate, unsigned char duration_csec)
{
    DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_WARNING,
                 " PULSE REQUEST: (FD:%#02X, DIR:%02i, RATE:%02i, CSEC:%02i)", fd, direction, rate, duration_csec);

    char cmd[]  = { 0x50, 0x04, 0x11, 0x26, 0x00, 0x00, 0x00, 0x00 };
    int errcode = 0;
    char errmsg[MAXRBUF];
    int nbytes_written = 0, nbytes_read = 0;
    char response[8];

    switch (direction)
    {
        case CELESTRON_N:
            cmd[2] = 0x11;
            cmd[4] = rate;
            cmd[5] = duration_csec;
            break;

        case CELESTRON_S:
            cmd[2] = 0x11;
            cmd[4] = -rate;
            cmd[5] = duration_csec;
            break;

        case CELESTRON_W:
            cmd[2] = 0x10;
            cmd[4] = rate;
            cmd[5] = duration_csec;
            break;

        case CELESTRON_E:
            cmd[2] = 0x10;
            cmd[4] = -rate;
            cmd[5] = duration_csec;
            break;
    }

#if 0
    DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_WARNING, "CMD <%#02X %#02X %#02X %#02X %#02X %#02X %#02X %#02X>",
                 cmd[0], cmd[1], cmd[2], cmd[3], (unsigned char)cmd[4], (unsigned char)cmd[5], cmd[6], cmd[7]);
#endif

    if (celestron_simulation)
    {
        strcpy(response, "#");
        nbytes_read = strlen(response);
        //DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_WARNING, "SIMULATION: NBYTES = %02i, RESPONSE = %s",
        //             nbytes_read, response);
    }
    else
    {
        tcflush(fd, TCIOFLUSH);

        // Send command and check success.
        //DEBUGDEVICE(celestron_device, INDI::Logger::DBG_DEBUG, "   ISSUING COMMAND");
        if ((errcode = tty_write(fd, cmd, 8, &nbytes_written)) != TTY_OK)
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }

        //DEBUGDEVICE(celestron_device, INDI::Logger::DBG_DEBUG, "   WAITING FOR REPLY");
        // Receive response and check success.
        if ((errcode = tty_read(fd, response, 1, CELESTRON_TIMEOUT, &nbytes_read)))
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }
    }

    if (nbytes_read == 1)
    {
        response[nbytes_read] = '\0';
        //DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_WARNING, "    NBYTES = %i, RESPONSE = %#02X", nbytes_read,
        //             response[0]);

        return true;
    }

    DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_ERROR, "RECEIVED %d BYTES, expected 1", nbytes_read);
    return false;
}

int SendPulseStatusCmd(int fd, CELESTRON_DIRECTION direction, bool &pulse_state)
{
    //DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_WARNING, " PULSE STATUS REQUEST: (FD:%#02X, DIR:%02i)", fd,
    //             direction);

    char cmd[]  = { 0x50, 0x03, 0x11, 0x27, 0x00, 0x00, 0x00, 0x01 };
    int errcode = 0;
    char errmsg[MAXRBUF];
    int nbytes_written = 0, nbytes_read = 0;
    char response[8];

    switch (direction)
    {
        case CELESTRON_N:
        case CELESTRON_S:
            cmd[2] = 0x11;
            break;
        case CELESTRON_W:
        case CELESTRON_E:
            cmd[2] = 0x10;
            break;
    }

    //DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_WARNING, "   COMMAND (%#02X %#02X %#02X %#02X %#02X %#02X %#02X %#02X)",
    //             cmd[0], cmd[1], cmd[2], cmd[3], cmd[4], cmd[5], cmd[6], cmd[7]);

    if (celestron_simulation)
    {
        strcpy(response, "0#");
        nbytes_read = strlen(response);
        DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_DEBUG, "SIMULATION: NBYTES = %02i, RESPONSE = %s",
                     nbytes_read, response);
    }
    else
    {
        tcflush(fd, TCIOFLUSH);

        //DEBUGDEVICE(celestron_device, INDI::Logger::DBG_DEBUG, "   ISSUING COMMAND");
        if ((errcode = tty_write(fd, cmd, 8, &nbytes_written)) != TTY_OK)
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }
        //DEBUGDEVICE(celestron_device, INDI::Logger::DBG_DEBUG, "   WAITING FOR REPLY");
        if ((errcode = tty_read_section(fd, response, '#', CELESTRON_TIMEOUT, &nbytes_read)))
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }
    }

    if (nbytes_read == 2)
    {
        response[nbytes_read] = '\0';
        //DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_WARNING, "    NBYTES = %i, RESPONSE = %#02X %#02X", nbytes_read,
        //             (unsigned char)response[0], (unsigned char)response[1]);

        if (response[0] == 0 && response[1] == '#')
        {
            pulse_state = 0;
        }
        else if (response[0] == 1 && response[1] == '#')
        {
            pulse_state = 1;
        }
        else
        {
            DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_WARNING, "UNEXPECTED RESPONSE: %#02X %#02X", (unsigned char)response[0], (unsigned char)response[1]);
            return false;
        }

        return true;
    }

    DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_ERROR, "RECEIVED %d BYTES, expected 2", nbytes_read);
    return false;
}

bool start_celestron_motion(int fd, CELESTRON_DIRECTION dir, CELESTRON_SLEW_RATE rate)
{
    char cmd[]  = { 0x50, 0x02, 0x11, 0x24, 0x09, 0x00, 0x00, 0x00 };
    int errcode = 0;
    char errmsg[MAXRBUF];
    int nbytes_written = 0, nbytes_read = 0;
    char response[8];

    switch (dir)
    {
        case CELESTRON_N:
            cmd[2] = 0x11;
            cmd[3] = 0x24;
            cmd[4] = rate + 1;
            break;

        case CELESTRON_S:
            cmd[2] = 0x11;
            cmd[3] = 0x25;
            cmd[4] = rate + 1;
            break;

        case CELESTRON_W:
            cmd[2] = 0x10;
            cmd[3] = 0x24;
            cmd[4] = rate + 1;
            break;

        case CELESTRON_E:
            cmd[2] = 0x10;
            cmd[3] = 0x25;
            cmd[4] = rate + 1;
            break;
    }

    DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_DEBUG, "CMD (%#02X %#02X %#02X %#02X %#02X %#02X %#02X %#02X)", cmd[0],
                 cmd[1], cmd[2], cmd[3], cmd[4], cmd[5], cmd[6], cmd[7]);

    if (celestron_simulation)
    {
        strcpy(response, "#");
        nbytes_read = strlen(response);
    }
    else
    {
        tcflush(fd, TCIOFLUSH);

        if ((errcode = tty_write(fd, cmd, 8, &nbytes_written)) != TTY_OK)
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }

        if ((errcode = tty_read(fd, response, 1, CELESTRON_TIMEOUT, &nbytes_read)))
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }
    }

    if (nbytes_read > 0)
    {
        response[nbytes_read] = '\0';
        DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_DEBUG, "RES <%s>", response);

        return true;
    }

    DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_ERROR, "Received #%d bytes, expected 1.", nbytes_read);
    return false;
}

bool stop_celestron_motion(int fd, CELESTRON_DIRECTION dir)
{
    char cmd[]  = { 0x50, 0x02, 0x11, 0x24, 0x00, 0x00, 0x00, 0x00 };
    int errcode = 0;
    char errmsg[MAXRBUF];
    char response[8];
    int nbytes_read    = 0;
    int nbytes_written = 0;

    switch (dir)
    {
        case CELESTRON_N:
        case CELESTRON_S:
            cmd[2] = 0x11;
            cmd[3] = 0x24;
            break;

        case CELESTRON_W:
        case CELESTRON_E:
            cmd[2] = 0x10;
            cmd[3] = 0x24;
            break;
    }

    DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_DEBUG, "CMD (%#02X %#02X %#02X %#02X %#02X %#02X %#02X %#02X)", cmd[0],
                 cmd[1], cmd[2], cmd[3], cmd[4], cmd[5], cmd[6], cmd[7]);

    if (celestron_simulation)
    {
        strcpy(response, "#");
        nbytes_read = strlen(response);
    }
    else
    {
        tcflush(fd, TCIOFLUSH);

        if ((errcode = tty_write(fd, cmd, 8, &nbytes_written)) != TTY_OK)
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }

        if ((errcode = tty_read(fd, response, 1, CELESTRON_TIMEOUT, &nbytes_read)))
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }
    }

    if (nbytes_read > 0)
    {
        response[nbytes_read] = '\0';
        DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_DEBUG, "RES <%s>", response);
        return true;
    }

    DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_ERROR, "Received #%d bytes, expected 1.", nbytes_read);
    return false;
}

bool abort_celestron(int fd)
{
    const char *cmd  = "M";
    int errcode = 0;
    char errmsg[MAXRBUF];
    char response[8];
    int nbytes_read    = 0;
    int nbytes_written = 0;

    DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_DEBUG, "CMD <%s>", cmd);

    if (celestron_simulation)
    {
        strcpy(response, "#");
        nbytes_read = strlen(response);
    }
    else
    {
        tcflush(fd, TCIOFLUSH);

        if ((errcode = tty_write(fd, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }

        if ((errcode = tty_read(fd, response, 1, CELESTRON_TIMEOUT, &nbytes_read)))
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }
    }

    if (nbytes_read > 0)
    {
        response[nbytes_read] = '\0';
        DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_DEBUG, "RES <%s>", response);
        return true;
    }

    DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_ERROR, "Only received #%d bytes, expected 1.", nbytes_read);
    return false;
}

bool slew_celestron(int fd, double ra, double dec)
{
    char cmd[16];
    int errcode = 0;
    char errmsg[MAXRBUF];
    char response[8];
    int nbytes_read    = 0;
    int nbytes_written = 0;

    int ra_int, de_int;

    ra_int = get_ra_fraction(ra);
    de_int = get_de_fraction(dec);

    char RAStr[16], DecStr[16];
    fs_sexa(RAStr, ra, 2, 3600);
    fs_sexa(DecStr, dec, 2, 3600);

    DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_DEBUG, "Goto (%s,%s)", RAStr, DecStr);

    snprintf(cmd, 16, "R%04X,%04X", ra_int, de_int);

    DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_DEBUG, "CMD <%s>", cmd);

    if (celestron_simulation)
    {
        strcpy(response, "#");
        nbytes_read = strlen(response);
        set_sim_slewing(true);
    }
    else
    {
        tcflush(fd, TCIOFLUSH);

        if ((errcode = tty_write(fd, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }

        if ((errcode = tty_read(fd, response, 1, CELESTRON_TIMEOUT, &nbytes_read)))
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }
    }

    if (nbytes_read > 0)
    {
        response[nbytes_read] = '\0';
        DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_DEBUG, "RES <%s>", response);

        if (!strcmp(response, "#"))
        {
            return true;
        }
        else
        {
            DEBUGDEVICE(celestron_device, INDI::Logger::DBG_ERROR, "Requested object is below horizon.");
            return false;
        }
    }

    DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_ERROR, "Received #%d bytes, expected 1.", nbytes_read);
    return false;
}

bool slew_celestron_azalt(int fd, double latitude, double az, double alt)
{
    char cmd[16] = {0};
    int errcode = 0;
    char errmsg[MAXRBUF];
    char response[8];
    int nbytes_read    = 0;
    int nbytes_written = 0;

    uint16_t az_int, alt_int;

    az_int  = get_az_fraction(az);
    alt_int = get_alt_fraction(latitude, alt, az);

    char AzStr[16], AltStr[16];
    fs_sexa(AzStr, az, 3, 3600);
    fs_sexa(AltStr, alt, 2, 3600);

    DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_DEBUG, "Goto AZM-ALT (%s,%s)", AzStr, AltStr);

    snprintf(cmd, 16, "B%04X,%04X", az_int, alt_int);

    DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_DEBUG, "CMD <%s>", cmd);

    if (celestron_simulation)
    {
        strcpy(response, "#");
        nbytes_read = strlen(response);
        set_sim_slewing(true);
    }
    else
    {
        tcflush(fd, TCIOFLUSH);

        if ((errcode = tty_write(fd, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }

        if ((errcode = tty_read(fd, response, 1, CELESTRON_TIMEOUT, &nbytes_read)))
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }
    }

    if (nbytes_read > 0)
    {
        response[nbytes_read] = '\0';
        DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_DEBUG, "RES <%s>", response);

        if (!strcmp(response, "#"))
        {
            return true;
        }
        else
        {
            DEBUGDEVICE(celestron_device, INDI::Logger::DBG_ERROR, "Requested object is below horizon.");
            return false;
        }
    }

    DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_ERROR, "Received #%d bytes, expected 1.", nbytes_read);
    return false;
}

bool sync_celestron(int fd, double ra, double dec)
{
    char cmd[16] = {0};
    int errcode = 0;
    char errmsg[MAXRBUF];
    char response[8];
    int nbytes_read    = 0;
    int nbytes_written = 0;

    int ra_int, de_int;

    char RAStr[16], DecStr[16];
    fs_sexa(RAStr, ra, 2, 3600);
    fs_sexa(DecStr, dec, 2, 3600);
    DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_DEBUG, "Sync (%s,%s)", RAStr, DecStr);

    ra_int = get_ra_fraction(ra);
    de_int = get_de_fraction(dec);

    snprintf(cmd, 16, "S%04X,%04X", ra_int, de_int);

    DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_DEBUG, "CMD <%s>", cmd);

    if (celestron_simulation)
    {
        simData.ra  = ra;
        simData.dec = dec;
        strcpy(response, "#");
        nbytes_read = strlen(response);
    }
    else
    {
        tcflush(fd, TCIOFLUSH);

        if ((errcode = tty_write(fd, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }

        if ((errcode = tty_read(fd, response, 1, CELESTRON_TIMEOUT, &nbytes_read)))
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }
    }

    if (nbytes_read > 0)
    {
        response[nbytes_read] = '\0';
        DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_DEBUG, "RES <%s>", response);

        if (!strcmp(response, "#"))
        {
            return true;
        }
        else
        {
            DEBUGDEVICE(celestron_device, INDI::Logger::DBG_ERROR, "Requested object is below horizon.");
            return false;
        }
    }

    DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_ERROR, "Received #%d bytes, expected 1.", nbytes_read);
    return false;
}

bool get_celestron_coords(int fd, double *ra, double *dec)
{
    const char *cmd  = "E";
    int errcode = 0;
    char errmsg[MAXRBUF];
    char response[16];
    int nbytes_read    = 0;
    int nbytes_written = 0;

    DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_EXTRA_1, "CMD <%s>", cmd);

    if (celestron_simulation)
    {
        int ra_int = get_ra_fraction(simData.ra);
        int de_int = get_de_fraction(simData.dec);
        snprintf(response, 16, "%04X,%04X#", ra_int, de_int);
        nbytes_read = strlen(response);
    }
    else
    {
        tcflush(fd, TCIOFLUSH);

        if ((errcode = tty_write(fd, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }

        if ((errcode = tty_read_section(fd, response, '#', CELESTRON_TIMEOUT, &nbytes_read)))
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }
    }

    if (nbytes_read > 0)
    {
        response[nbytes_read] = '\0';

        char ra_str[16], de_str[16];
        memset(ra_str, 0, 16);
        memset(de_str, 0, 16);

        strncpy(ra_str, response, 4);
        strncpy(de_str, response + 5, 4);

        int CELESTRONRA = strtol(ra_str, nullptr, 16);
        int CELESTRONDE = strtol(de_str, nullptr, 16);

        *ra  = (CELESTRONRA / 65536.0) * (360.0 / 15.0);
        *dec = (CELESTRONDE / 65536.0) * 360.0;

        /* Account for the quadrant in declination
         * Author:  John Kielkopf (kielkopf@louisville.edu)  */
        /* 90 to 180 */
        if ((*dec > 90.) && (*dec <= 180.))
            *dec = 180. - *dec;
        /* 180 to 270 */
        if ((*dec > 180.) && (*dec <= 270.))
            *dec = *dec - 270.;
        /* 270 to 360 */
        if ((*dec > 270.) && (*dec <= 360.))
            *dec = *dec - 360.;

        char RAStr[16], DecStr[16];
        fs_sexa(RAStr, *ra, 2, 3600);
        fs_sexa(DecStr, *dec, 2, 3600);
        DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_EXTRA_1, "RES <%s> ==> RA-DEC (%s,%s)", response, RAStr,
                     DecStr);

        return true;
    }

    DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_ERROR, "Received #%d bytes, expected 10.", nbytes_read);
    return false;
}

bool get_celestron_coords_azalt(int fd, double latitude, double *az, double *alt)
{
    const char *cmd  = "Z";
    int errcode = 0;
    char errmsg[MAXRBUF];
    char response[16];
    int nbytes_read    = 0;
    int nbytes_written = 0;

    DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_EXTRA_1, "CMD <%c>", cmd[0]);

    if (celestron_simulation)
    {
        int az_int  = get_az_fraction(simData.az);
        int alt_int = get_alt_fraction(latitude, simData.alt, simData.az);
        snprintf(response, 16, "%04X,%04X#", az_int, alt_int);
        nbytes_read = strlen(response);
    }
    else
    {
        tcflush(fd, TCIOFLUSH);

        if ((errcode = tty_write(fd, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }

        if ((errcode = tty_read_section(fd, response, '#', CELESTRON_TIMEOUT, &nbytes_read)))
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }
    }

    if (nbytes_read > 0)
    {
        response[nbytes_read] = '\0';

        uint16_t CELESTRONAZ = 0, CELESTRONALT = 0;

        sscanf(response, "%hx,%hx#", &CELESTRONAZ, &CELESTRONALT);

        *az  = ((CELESTRONAZ - 0x4000) / 65536.0) * 360.0;
        *alt = ((CELESTRONALT - 0x4000) / 65536.0) * 360.0 + latitude;

        *az = range360(*az);

        if (*alt > 90 && *alt <= 270)
            *alt = 180 - *alt;
        //else if (*alt > 180 && *alt < 270)
        //  *alt = 180 - *alt;
        else if (*alt > 270)
            *alt -= 360;

        char AzStr[16], AltStr[16];
        fs_sexa(AzStr, *az, 3, 3600);
        fs_sexa(AltStr, *alt, 2, 3600);
        DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_EXTRA_1, "RES <%s> ==> AZM-ALT (%s,%s)", response, AzStr,
                     AltStr);

        return true;
    }

    DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_ERROR, "Received #%d bytes, expected 10.", nbytes_read);
    return false;
}

bool set_celestron_location(int fd, double longitude, double latitude)
{
    char cmd[16] = {0};
    int errcode = 0;
    char errmsg[MAXRBUF];
    char response[8];
    int nbytes_read    = 0;
    int nbytes_written = 0;

    int lat_d, lat_m, lat_s;
    int long_d, long_m, long_s;

    // Convert from INDI standard to regular east/west -180 to 180
    if (longitude > 180)
        longitude -= 360;

    getSexComponents(latitude, &lat_d, &lat_m, &lat_s);
    getSexComponents(longitude, &long_d, &long_m, &long_s);

    cmd[0] = 'W';
    cmd[1] = abs(lat_d);
    cmd[2] = lat_m;
    cmd[3] = lat_s;
    cmd[4] = lat_d > 0 ? 0 : 1;
    cmd[5] = abs(long_d);
    cmd[6] = long_m;
    cmd[7] = long_s;
    cmd[8] = long_d > 0 ? 0 : 1;

    DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_DEBUG, "CMD (%#02X %#02X %#02X %#02X %#02X %#02X %#02X %#02X %#02X)",
                 cmd[0], cmd[1], cmd[2], cmd[3], cmd[4], cmd[5], cmd[6], cmd[7], cmd[8]);

    if (celestron_simulation)
    {
        strcpy(response, "#");
        nbytes_read = strlen(response);
    }
    else
    {
        tcflush(fd, TCIOFLUSH);

        if ((errcode = tty_write(fd, cmd, 9, &nbytes_written)) != TTY_OK)
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }

        if ((errcode = tty_read(fd, response, 1, CELESTRON_TIMEOUT, &nbytes_read)))
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }
    }

    if (nbytes_read > 0)
    {
        response[nbytes_read] = '\0';
        DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_DEBUG, "RES <%s>", response);

        return true;
    }

    DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_ERROR, "Received #%d bytes, expected 1.", nbytes_read);
    return false;
}

bool set_celestron_datetime(int fd, struct ln_date *utc, double utc_offset)
{
    char cmd[16] = {0};
    int errcode = 0;
    char errmsg[MAXRBUF];
    char response[8];
    int nbytes_read    = 0;
    int nbytes_written = 0;

    struct ln_zonedate local_date;

    // Celestron takes local time
    ln_date_to_zonedate(utc, &local_date, utc_offset * 3600);

    cmd[0] = 'H';
    cmd[1] = local_date.hours;
    cmd[2] = local_date.minutes;
    cmd[3] = local_date.seconds;
    cmd[4] = local_date.months;
    cmd[5] = local_date.days;
    cmd[6] = local_date.years - 2000;

    if (utc_offset < 0)
        cmd[7] = 256 - ((uint16_t)fabs(utc_offset));
    else
        cmd[7] = ((uint16_t)fabs(utc_offset));

    // Always assume standard time
    cmd[8] = 0;

    DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_DEBUG, "CMD (%#02X %#02X %#02X %#02X %#02X %#02X %#02X %#02X %#02X)",
                 cmd[0], cmd[1], cmd[2], cmd[3], cmd[4], cmd[5], cmd[6], cmd[7], cmd[8]);

    if (celestron_simulation)
    {
        strcpy(response, "#");
        nbytes_read = strlen(response);
    }
    else
    {
        tcflush(fd, TCIOFLUSH);

        if ((errcode = tty_write(fd, cmd, 9, &nbytes_written)) != TTY_OK)
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }

        if ((errcode = tty_read(fd, response, 1, CELESTRON_TIMEOUT, &nbytes_read)))
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }
    }

    if (nbytes_read > 0)
    {
        response[nbytes_read] = '\0';
        DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_DEBUG, "RES <%s>", response);

        return true;
    }

    DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_ERROR, "Only received #%d bytes, expected 1.", nbytes_read);
    return false;
}

bool get_celestron_utc_date_time(int fd, double *utc_hours, int *yy, int *mm, int *dd, int *hh, int *minute, int *ss)
{
    const char *cmd  = "h";
    int errcode = 0;
    char errmsg[MAXRBUF];
    char response[32];
    int nbytes_read    = 0;
    int nbytes_written = 0;

    DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_DEBUG, "CMD <%c>", cmd[0]);

    if (celestron_simulation)
    {
        // HH MM SS MONTH DAY YEAR OFFSET DAYLIGHT
        response[0] = 17;
        response[1] = 30;
        response[2] = 10;
        response[3] = 4;
        response[4] = 1;
        response[5] = 15;
        response[6] = 3;
        response[7] = 0;
        response[8] = '#';
        nbytes_read = strlen(response);
    }
    else
    {
        tcflush(fd, TCIOFLUSH);

        if ((errcode = tty_write(fd, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }

        if ((errcode = tty_read_section(fd, response, '#', CELESTRON_TIMEOUT, &nbytes_read)))
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }
    }

    if (nbytes_read > 0)
    {
        response[nbytes_read] = '\0';
        unsigned char *res    = (unsigned char *)response;
        DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_DEBUG, "RES (%#02X %#02X %#02X %#02X %#02X %#02X %#02X %#02X)", res[0],
                     res[1], res[2], res[3], res[4], res[5], res[6], res[7]);

        // HH MM SS MONTH DAY YEAR OFFSET DAYLIGHT
        *hh        = res[0];
        *minute    = res[1];
        *ss        = res[2];
        *mm        = res[3];
        *dd        = res[4];
        *yy        = res[5] + 2000;
        *utc_hours = res[6];

        if (*utc_hours > 12)
            *utc_hours -= 256;

        ln_zonedate localTime;
        ln_date utcTime;

        localTime.years   = *yy;
        localTime.months  = *mm;
        localTime.days    = *dd;
        localTime.hours   = *hh;
        localTime.minutes = *minute;
        localTime.seconds = *ss;
        localTime.gmtoff  = *utc_hours * 3600;

        ln_zonedate_to_date(&localTime, &utcTime);

        *yy     = utcTime.years;
        *mm     = utcTime.months;
        *dd     = utcTime.days;
        *hh     = utcTime.hours;
        *minute = utcTime.minutes;
        *ss     = utcTime.seconds;

        return true;
    }

    DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_ERROR, "Only received #%d bytes, expected 1.", nbytes_read);
    return false;
}

bool is_scope_slewing(int fd)
{
    const char* cmd  = "L";
    int errcode = 0;
    char errmsg[MAXRBUF];
    char response[8];
    int nbytes_read    = 0;
    int nbytes_written = 0;

    DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_DEBUG, "CMD <%c>", cmd[0]);

    if (celestron_simulation)
    {
        if (simData.isSlewing)
            strcpy(response, "1#");
        else
            strcpy(response, "0#");

        nbytes_read = strlen(response);
    }
    else
    {
        tcflush(fd, TCIOFLUSH);

        if ((errcode = tty_write(fd, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }

        if ((errcode = tty_read(fd, response, 2, CELESTRON_TIMEOUT, &nbytes_read)))
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }
    }

    if (nbytes_read > 0)
    {
        response[nbytes_read] = '\0';
        DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_DEBUG, "RES <%s>", response);

        tcflush(fd, TCIFLUSH);

        if (response[0] == '0')
            return false;
        else
            return true;
    }

    DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_ERROR, "Received #%d bytes, expected 1.", nbytes_read);
    return false;
}

bool get_celestron_track_mode(int fd, CELESTRON_TRACK_MODE *mode)
{
    const char *cmd  = "t";
    int errcode = 0;
    char errmsg[MAXRBUF];
    char response[8];
    int nbytes_read    = 0;
    int nbytes_written = 0;

    DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_DEBUG, "CMD <%c>", cmd[0]);

    if (celestron_simulation)
    {
        response[0] = simData.trackMode;
        response[1] = '#';
        nbytes_read = 2;
    }
    else
    {
        tcflush(fd, TCIOFLUSH);

        if ((errcode = tty_write(fd, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }

        if ((errcode = tty_read(fd, response, 2, CELESTRON_TIMEOUT, &nbytes_read)))
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }
    }

    if (nbytes_read > 0)
    {
        response[nbytes_read] = '\0';
        DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_DEBUG, "RES (%#02X %#02X)", response[0], response[1]);

        tcflush(fd, TCIFLUSH);

        *mode = ((CELESTRON_TRACK_MODE)response[0]);

        return true;
    }

    DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_ERROR, "Received #%d bytes, expected 1.", nbytes_read);
    return false;
}

bool set_celestron_track_mode(int fd, CELESTRON_TRACK_MODE mode)
{
    char cmd[2];
    int errcode = 0;
    char errmsg[MAXRBUF];
    char response[8];
    int nbytes_read    = 0;
    int nbytes_written = 0;

    cmd[0] = 'T';
    cmd[1] = mode;

    DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_DEBUG, "CMD <%#02X %#02X>", cmd[0], cmd[1]);

    if (celestron_simulation)
    {
        simData.trackMode = mode;
        strcpy(response, "#");
        nbytes_read = strlen(response);
    }
    else
    {
        tcflush(fd, TCIOFLUSH);

        if ((errcode = tty_write(fd, cmd, 2, &nbytes_written)) != TTY_OK)
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }

        if ((errcode = tty_read(fd, response, 1, CELESTRON_TIMEOUT, &nbytes_read)))
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }
    }

    if (nbytes_read > 0)
    {
        response[nbytes_read] = '\0';
        DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_DEBUG, "RES <%s>", response);

        if (!strcmp(response, "#"))
        {
            tcflush(fd, TCIFLUSH);
            return true;
        }
        else
        {
            DEBUGDEVICE(celestron_device, INDI::Logger::DBG_ERROR, "Error setting tracking mode.");
            tcflush(fd, TCIFLUSH);
            return false;
        }
    }

    DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_ERROR, "Received #%d bytes, expected 1.", nbytes_read);
    return false;
}

bool hibernate(int fd)
{
    const char *cmd  = "x#";
    int errcode = 0;
    char errmsg[MAXRBUF];
    int nbytes_written = 0;

    DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_DEBUG, "CMD <%s>", cmd);

    if (celestron_simulation)
        return true;

    tcflush(fd, TCIOFLUSH);

    if ((errcode = tty_write(fd, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
    {
        tty_error_msg(errcode, errmsg, MAXRBUF);
        DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_ERROR, "%s", errmsg);
        return false;
    }

    return true;
}

bool wakeup(int fd)
{
    const char *cmd  = "y#";
    int errcode = 0;
    char errmsg[MAXRBUF];
    int nbytes_written = 0;
    int nbytes_read    = 0;
    char response[2];

    DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_DEBUG, "CMD <%s>", cmd);

    if (celestron_simulation)
        return true;

    tcflush(fd, TCIOFLUSH);

    if ((errcode = tty_write(fd, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
    {
        tty_error_msg(errcode, errmsg, MAXRBUF);
        DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_ERROR, "%s", errmsg);
        return false;
    }

    if ((errcode = tty_read(fd, response, 1, CELESTRON_TIMEOUT, &nbytes_read)))
    {
        tty_error_msg(errcode, errmsg, MAXRBUF);
        DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_ERROR, "%s", errmsg);
        return false;
    }

    if (nbytes_read > 0)
    {
        response[nbytes_read] = '\0';
        DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_DEBUG, "RES <%s>", response);

        if (!strcmp(response, "#"))
        {
            tcflush(fd, TCIFLUSH);
            return true;
        }
        else
        {
            DEBUGDEVICE(celestron_device, INDI::Logger::DBG_ERROR, "Error waking up the mount.");
            tcflush(fd, TCIFLUSH);
            return false;
        }
    }

    DEBUGFDEVICE(celestron_device, INDI::Logger::DBG_ERROR, "Received #%d bytes, expected 1.", nbytes_read);
    return false;
}

uint16_t get_angle_fraction(double angle)
{
    if (angle >= 0)
        return ((uint16_t)(angle * 65536 / 360.0));
    else
        return ((uint16_t)((angle + 360) * 65536 / 360.0));
}

uint16_t get_ra_fraction(double ra)
{
    return ((uint16_t)(ra * 15.0 * 65536 / 360.0));
}

uint16_t get_de_fraction(double de)
{
    uint16_t de_int = 0;

    if (de >= 0)
        de_int = (uint16_t)(de * 65536 / 360.0);
    else
        de_int = (uint16_t)((de + 360.0) * 65536 / 360.0);

    return de_int;
}

uint16_t get_az_fraction(double az)
{
    return (uint16_t)(az * 65536 / 360.0) + 0x4000;
}

uint16_t get_alt_fraction(double lat, double alt, double az)
{
    uint16_t alt_int = 0;

    if (alt >= 0)
    {
        // North
        if (az >= 270 || az <= 90)
            alt_int = (uint16_t)((alt - lat) * 65536 / 360.0) + 0x4000;
        else
            alt_int = (uint16_t)((180 - (lat + alt)) * 65536 / 360.0) + 0x4000;
    }
    else
    {
        if (az >= 270 || az <= 90)
            alt_int = (uint16_t)((360 - lat + alt) * 65536 / 360.0) + 0x4000;
        else
            alt_int = (uint16_t)((180 - (lat + alt)) * 65536 / 360.0) + 0x4000;
    }

    return alt_int;
}

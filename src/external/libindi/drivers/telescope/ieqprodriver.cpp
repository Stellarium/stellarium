/*
    IEQ Pro driver

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

#include "ieqprodriver.h"

#include "indicom.h"
#include "indilogger.h"

#include <libnova/julian_day.h>

#include <cmath>
#include <cstring>
#include <termios.h>
#include <unistd.h>

#define IEQPRO_TIMEOUT 5 /* FD timeout in seconds */

bool ieqpro_debug                 = false;
bool ieqpro_simulation            = false;
char ieqpro_device[MAXINDIDEVICE] = "iEQ";
IEQInfo simInfo;

struct
{
    double ra;
    double dec;
    double guide_rate;
} simData;

void set_ieqpro_debug(bool enable)
{
    ieqpro_debug = enable;
}

void set_ieqpro_simulation(bool enable)
{
    ieqpro_simulation = enable;
    if (enable)
        simData.guide_rate = 0.5;
}

void set_ieqpro_device(const char *name)
{
    strncpy(ieqpro_device, name, MAXINDIDEVICE);
}

void set_sim_gps_status(IEQ_GPS_STATUS value)
{
    simInfo.gpsStatus = value;
}

void set_sim_system_status(IEQ_SYSTEM_STATUS value)
{
    simInfo.systemStatus = value;
}

void set_sim_track_rate(IEQ_TRACK_RATE value)
{
    simInfo.trackRate = value;
}

void set_sim_slew_rate(IEQ_SLEW_RATE value)
{
    simInfo.slewRate = value;
}

void set_sim_time_source(IEQ_TIME_SOURCE value)
{
    simInfo.timeSource = value;
}

void set_sim_hemisphere(IEQ_HEMISPHERE value)
{
    simInfo.hemisphere = value;
}

void set_sim_ra(double ra)
{
    simData.ra = ra;
}

void set_sim_dec(double dec)
{
    simData.dec = dec;
}

void set_sim_guide_rate(double rate)
{
    simData.guide_rate = rate;
}

bool check_ieqpro_connection(int fd)
{
    char initCMD[] = ":V#";
    int errcode    = 0;
    char errmsg[MAXRBUF];
    char response[8];
    int nbytes_read    = 0;
    int nbytes_written = 0;

    DEBUGDEVICE(ieqpro_device, INDI::Logger::DBG_DEBUG, "Initializing IOptron using :V# CMD...");

    for (int i = 0; i < 2; i++)
    {
        if (ieqpro_simulation)
        {
            strcpy(response, "V1.00#");
            nbytes_read = strlen(response);
        }
        else
        {
            tcflush(fd, TCIFLUSH);

            if ((errcode = tty_write(fd, initCMD, 3, &nbytes_written)) != TTY_OK)
            {
                tty_error_msg(errcode, errmsg, MAXRBUF);
                DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_ERROR, "%s", errmsg);
                usleep(50000);
                continue;
            }

            if ((errcode = tty_read_section(fd, response, '#', IEQPRO_TIMEOUT, &nbytes_read)))
            {
                tty_error_msg(errcode, errmsg, MAXRBUF);
                DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_ERROR, "%s", errmsg);
                usleep(50000);
                continue;
            }
        }

        if (nbytes_read > 0)
        {
            response[nbytes_read] = '\0';
            DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_DEBUG, "RES (%s)", response);

            if (!strcmp(response, "V1.00#"))
                return true;
        }

        usleep(50000);
    }

    return false;
}

bool get_ieqpro_status(int fd, IEQInfo *info)
{
    char cmd[]  = ":GAS#";
    int errcode = 0;
    char errmsg[MAXRBUF];
    char response[8];
    int nbytes_read    = 0;
    int nbytes_written = 0;

    DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_EXTRA_1, "CMD (%s)", cmd);

    if (ieqpro_simulation)
    {
        snprintf(response, 8, "%d%d%d%d%d%d#", simInfo.gpsStatus, simInfo.systemStatus, simInfo.trackRate,
                 simInfo.slewRate + 1, simInfo.timeSource, simInfo.hemisphere);
        nbytes_read = strlen(response);
    }
    else
    {
        tcflush(fd, TCIFLUSH);

        if ((errcode = tty_write(fd, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }

        if ((errcode = tty_read_section(fd, response, '#', IEQPRO_TIMEOUT, &nbytes_read)))
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }
    }

    if (nbytes_read > 0)
    {
        response[nbytes_read] = '\0';
        DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_EXTRA_1, "RES (%s)", response);

        if (nbytes_read == 7)
        {
            info->gpsStatus    = (IEQ_GPS_STATUS)(response[0] - '0');
            info->systemStatus = (IEQ_SYSTEM_STATUS)(response[1] - '0');
            info->trackRate    = (IEQ_TRACK_RATE)(response[2] - '0');
            info->slewRate     = (IEQ_SLEW_RATE)(response[3] - '0' - 1);
            info->timeSource   = (IEQ_TIME_SOURCE)(response[4] - '0');
            info->hemisphere   = (IEQ_HEMISPHERE)(response[5] - '0');

            tcflush(fd, TCIFLUSH);

            return true;
        }
    }

    DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_ERROR, "Only received #%d bytes, expected 7.", nbytes_read);
    return false;
}

bool get_ieqpro_firmware(int fd, FirmwareInfo *info)
{
    bool rc = false;

    rc = get_ieqpro_model(fd, info);

    if (!rc)
        return rc;

    rc = get_ieqpro_main_firmware(fd, info);

    if (!rc)
        return rc;

    rc = get_ieqpro_radec_firmware(fd, info);

    return rc;
}

bool get_ieqpro_model(int fd, FirmwareInfo *info)
{
    char cmd[]  = ":MountInfo#";
    int errcode = 0;
    char errmsg[MAXRBUF];
    char response[16];
    int nbytes_read    = 0;
    int nbytes_written = 0;

    DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_DEBUG, "CMD (%s)", cmd);

    if (ieqpro_simulation)
    {
        strcpy(response, "0045");
        nbytes_read = strlen(response);
    }
    else
    {
        tcflush(fd, TCIFLUSH);

        if ((errcode = tty_write(fd, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }

        if ((errcode = tty_read(fd, response, 4, IEQPRO_TIMEOUT, &nbytes_read)))
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }
    }

    if (nbytes_read > 0)
    {
        response[nbytes_read] = '\0';
        DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_DEBUG, "RES (%s)", response);

        if (nbytes_read == 4)
        {
            if (!strcmp(response, "0060"))
                info->Model = "CEM60";
            else if (!strcmp(response, "0061"))
                info->Model = "CEM60-EC";
            else if (!strcmp(response, "0045"))
                info->Model = "iEQ45 Pro";
            else if (!strcmp(response, "0046"))
                info->Model = "iEQ45 Pro AA";
            else if (!strcmp(response, "0025"))
                info->Model = "CEM25";
            else
                info->Model = "Unknown";

            tcflush(fd, TCIFLUSH);

            return true;
        }
    }

    DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_ERROR, "Only received #%d bytes, expected 4.", nbytes_read);
    return false;
}

bool get_ieqpro_main_firmware(int fd, FirmwareInfo *info)
{
    char cmd[]  = ":FW1#";
    int errcode = 0;
    char errmsg[MAXRBUF];
    char response[16];
    int nbytes_read    = 0;
    int nbytes_written = 0;

    DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_DEBUG, "CMD (%s)", cmd);

    if (ieqpro_simulation)
    {
        strcpy(response, "150324150101#");
        nbytes_read = strlen(response);
    }
    else
    {
        tcflush(fd, TCIFLUSH);

        if ((errcode = tty_write(fd, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }

        if ((errcode = tty_read_section(fd, response, '#', IEQPRO_TIMEOUT, &nbytes_read)))
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }
    }

    if (nbytes_read > 0)
    {
        response[nbytes_read] = '\0';
        DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_DEBUG, "RES (%s)", response);

        if (nbytes_read == 13)
        {
            char board[8] = {0}, controller[8] = {0};

            strncpy(board, response, 6);
            strncpy(controller, response + 6, 6);

            info->MainBoardFirmware.assign(board, 6);
            info->ControllerFirmware.assign(controller, 6);

            tcflush(fd, TCIFLUSH);

            return true;
        }
    }

    DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_ERROR, "Only received #%d bytes, expected 13.", nbytes_read);
    return false;
}

bool get_ieqpro_radec_firmware(int fd, FirmwareInfo *info)
{
    char cmd[]  = ":FW2#";
    int errcode = 0;
    char errmsg[MAXRBUF];
    char response[16];
    int nbytes_read    = 0;
    int nbytes_written = 0;

    DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_DEBUG, "CMD (%s)", cmd);

    if (ieqpro_simulation)
    {
        strcpy(response, "140324140101#");
        nbytes_read = strlen(response);
    }
    else
    {
        tcflush(fd, TCIFLUSH);

        if ((errcode = tty_write(fd, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }

        if ((errcode = tty_read_section(fd, response, '#', IEQPRO_TIMEOUT, &nbytes_read)))
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }
    }

    if (nbytes_read > 0)
    {
        response[nbytes_read] = '\0';
        DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_DEBUG, "RES (%s)", response);

        if (nbytes_read == 13)
        {
            char ra[8] = {0}, dec[8] = {0};

            strncpy(ra, response, 6);
            strncpy(dec, response + 6, 6);

            info->RAFirmware.assign(ra, 6);
            info->DEFirmware.assign(dec, 6);

            tcflush(fd, TCIFLUSH);

            return true;
        }
    }

    DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_ERROR, "Only received #%d bytes, expected 13.", nbytes_read);
    return false;
}

bool start_ieqpro_motion(int fd, IEQ_DIRECTION dir)
{
    char cmd[16];
    int errcode = 0;
    char errmsg[MAXRBUF];
    int nbytes_written = 0;

    switch (dir)
    {
        case IEQ_N:
            strcpy(cmd, ":mn#");
            break;
        case IEQ_S:
            strcpy(cmd, ":ms#");
            break;
        case IEQ_W:
            strcpy(cmd, ":mw#");
            break;
        case IEQ_E:
            strcpy(cmd, ":me#");
            break;
    }

    DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_DEBUG, "CMD (%s)", cmd);

    if (ieqpro_simulation)
        return true;

    tcflush(fd, TCIFLUSH);

    if ((errcode = tty_write(fd, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
    {
        tty_error_msg(errcode, errmsg, MAXRBUF);
        DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_ERROR, "%s", errmsg);
        return false;
    }

    tcflush(fd, TCIFLUSH);
    return true;
}

bool stop_ieqpro_motion(int fd, IEQ_DIRECTION dir)
{
    char cmd[16];
    int errcode = 0;
    char errmsg[MAXRBUF];
    char response[8];
    int nbytes_read    = 0;
    int nbytes_written = 0;

    switch (dir)
    {
        case IEQ_N:
        case IEQ_S:
            strcpy(cmd, ":qD#");
            break;

        case IEQ_W:
        case IEQ_E:
            strcpy(cmd, ":qR#");
            break;
    }

    DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_DEBUG, "CMD (%s)", cmd);

    if (ieqpro_simulation)
    {
        strcpy(response, "1");
        nbytes_read = strlen(response);
    }
    else
    {
        tcflush(fd, TCIFLUSH);

        if ((errcode = tty_write(fd, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }

        if ((errcode = tty_read(fd, response, 1, IEQPRO_TIMEOUT, &nbytes_read)))
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }
    }

    if (nbytes_read > 0)
    {
        response[nbytes_read] = '\0';
        DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_DEBUG, "RES (%s)", response);

        tcflush(fd, TCIFLUSH);
        return true;
    }

    DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_ERROR, "Only received #%d bytes, expected 1.", nbytes_read);
    return false;
}

bool find_ieqpro_home(int fd)
{
    char cmd[]  = ":MSH#";
    int errcode = 0;
    char errmsg[MAXRBUF];
    char response[8];
    int nbytes_read    = 0;
    int nbytes_written = 0;

    DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_DEBUG, "CMD (%s)", cmd);

    if (ieqpro_simulation)
    {
        strcpy(response, "1");
        nbytes_read = strlen(response);
    }
    else
    {
        tcflush(fd, TCIFLUSH);

        if ((errcode = tty_write(fd, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }

        if ((errcode = tty_read(fd, response, 1, IEQPRO_TIMEOUT, &nbytes_read)))
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }
    }

    if (nbytes_read > 0)
    {
        response[nbytes_read] = '\0';
        DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_DEBUG, "RES (%s)", response);

        tcflush(fd, TCIFLUSH);
        return true;
    }

    DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_ERROR, "Only received #%d bytes, expected 1.", nbytes_read);
    return false;
}

bool goto_ieqpro_home(int fd)
{
    char cmd[]  = ":MH#";
    int errcode = 0;
    char errmsg[MAXRBUF];
    char response[8];
    int nbytes_read    = 0;
    int nbytes_written = 0;

    DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_DEBUG, "CMD (%s)", cmd);

    if (ieqpro_simulation)
    {
        strcpy(response, "1");
        nbytes_read = strlen(response);
    }
    else
    {
        tcflush(fd, TCIFLUSH);

        if ((errcode = tty_write(fd, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }

        if ((errcode = tty_read(fd, response, 1, IEQPRO_TIMEOUT, &nbytes_read)))
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }
    }

    if (nbytes_read > 0)
    {
        response[nbytes_read] = '\0';
        DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_DEBUG, "RES (%s)", response);

        tcflush(fd, TCIFLUSH);
        return true;
    }

    DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_ERROR, "Only received #%d bytes, expected 1.", nbytes_read);
    return false;
}

bool set_ieqpro_current_home(int fd)
{
    char cmd[]  = ":SZP#";
    int errcode = 0;
    char errmsg[MAXRBUF];
    char response[8];
    int nbytes_read    = 0;
    int nbytes_written = 0;

    DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_DEBUG, "CMD (%s)", cmd);

    if (ieqpro_simulation)
    {
        strcpy(response, "1");
        nbytes_read = strlen(response);
    }
    else
    {
        tcflush(fd, TCIFLUSH);

        if ((errcode = tty_write(fd, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }

        if ((errcode = tty_read(fd, response, 1, IEQPRO_TIMEOUT, &nbytes_read)))
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }
    }

    if (nbytes_read > 0)
    {
        response[nbytes_read] = '\0';
        DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_DEBUG, "RES (%s)", response);

        tcflush(fd, TCIFLUSH);
        return true;
    }

    DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_ERROR, "Only received #%d bytes, expected 1.", nbytes_read);
    return false;
}

bool set_ieqpro_slew_rate(int fd, IEQ_SLEW_RATE rate)
{
    char cmd[16];
    int errcode = 0;
    char errmsg[MAXRBUF];
    char response[8];
    int nbytes_read    = 0;
    int nbytes_written = 0;

    snprintf(cmd, 16, ":SR%d#", ((int)rate) + 1);

    DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_DEBUG, "CMD (%s)", cmd);

    if (ieqpro_simulation)
    {
        simInfo.slewRate = rate;
        strcpy(response, "1");
        nbytes_read = strlen(response);
    }
    else
    {
        tcflush(fd, TCIFLUSH);

        if ((errcode = tty_write(fd, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }

        if ((errcode = tty_read(fd, response, 1, IEQPRO_TIMEOUT, &nbytes_read)))
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }
    }

    if (nbytes_read > 0)
    {
        response[nbytes_read] = '\0';
        DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_DEBUG, "RES (%s)", response);

        tcflush(fd, TCIFLUSH);
        return true;
    }

    DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_ERROR, "Only received #%d bytes, expected 1.", nbytes_read);
    return false;
}

bool set_ieqpro_track_mode(int fd, IEQ_TRACK_RATE rate)
{
    char cmd[16];
    int errcode = 0;
    char errmsg[MAXRBUF];
    char response[8];
    int nbytes_read    = 0;
    int nbytes_written = 0;

    switch (rate)
    {
        case TR_SIDEREAL:
            strcpy(cmd, ":RT0#");
            break;
        case TR_LUNAR:
            strcpy(cmd, ":RT1#");
            break;
        case TR_SOLAR:
            strcpy(cmd, ":RT2#");
            break;
        case TR_KING:
            strcpy(cmd, ":RT3#");
            break;
        case TR_CUSTOM:
            strcpy(cmd, ":RT4#");
            break;
    }

    DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_DEBUG, "CMD (%s)", cmd);

    if (ieqpro_simulation)
    {
        simInfo.trackRate = rate;
        strcpy(response, "1");
        nbytes_read = strlen(response);
    }
    else
    {
        tcflush(fd, TCIFLUSH);

        if ((errcode = tty_write(fd, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }

        if ((errcode = tty_read(fd, response, 1, IEQPRO_TIMEOUT, &nbytes_read)))
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }
    }

    if (nbytes_read > 0)
    {
        response[nbytes_read] = '\0';
        DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_DEBUG, "RES (%s)", response);

        tcflush(fd, TCIFLUSH);
        return true;
    }

    DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_ERROR, "Only received #%d bytes, expected 1.", nbytes_read);
    return false;
}

bool set_ieqpro_custom_ra_track_rate(int fd, double rate)
{
    char cmd[16];
    char sign;
    int errcode = 0;
    char errmsg[MAXRBUF];
    char response[8];
    int nbytes_read    = 0;
    int nbytes_written = 0;

    if (rate < 0)
        sign = '-';
    else
        sign = '+';

    snprintf(cmd, 16, ":RR%c%07.4f#", sign, fabs(rate));

    DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_DEBUG, "CMD (%s)", cmd);

    if (ieqpro_simulation)
    {
        strcpy(response, "1");
        nbytes_read = strlen(response);
    }
    else
    {
        tcflush(fd, TCIFLUSH);

        if ((errcode = tty_write(fd, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }

        if ((errcode = tty_read(fd, response, 1, IEQPRO_TIMEOUT, &nbytes_read)))
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }
    }

    if (nbytes_read > 0)
    {
        response[nbytes_read] = '\0';
        DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_DEBUG, "RES (%s)", response);

        tcflush(fd, TCIFLUSH);
        return true;
    }

    DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_ERROR, "Only received #%d bytes, expected 1.", nbytes_read);
    return false;
}

bool set_ieqpro_custom_de_track_rate(int fd, double rate)
{
    char cmd[16];
    char sign;
    int errcode = 0;
    char errmsg[MAXRBUF];
    char response[8];
    int nbytes_read    = 0;
    int nbytes_written = 0;

    if (rate < 0)
        sign = '-';
    else
        sign = '+';

    snprintf(cmd, 16, ":RD%c%07.4f#", sign, fabs(rate));

    DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_DEBUG, "CMD (%s)", cmd);

    if (ieqpro_simulation)
    {
        strcpy(response, "1");
        nbytes_read = strlen(response);
    }
    else
    {
        tcflush(fd, TCIFLUSH);

        if ((errcode = tty_write(fd, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }

        if ((errcode = tty_read(fd, response, 1, IEQPRO_TIMEOUT, &nbytes_read)))
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }
    }

    if (nbytes_read > 0)
    {
        response[nbytes_read] = '\0';
        DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_DEBUG, "RES (%s)", response);

        tcflush(fd, TCIFLUSH);
        return true;
    }

    DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_ERROR, "Only received #%d bytes, expected 1.", nbytes_read);
    return false;
}

bool set_ieqpro_guide_rate(int fd, double rate)
{
    char cmd[16];
    int errcode = 0;
    char errmsg[MAXRBUF];
    char response[8];
    int nbytes_read    = 0;
    int nbytes_written = 0;

    int num = rate * 100;
    snprintf(cmd, 16, ":RG%03d#", num);

    DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_DEBUG, "CMD (%s)", cmd);

    if (ieqpro_simulation)
    {
        simData.guide_rate = rate;
        strcpy(response, "1");
        nbytes_read = strlen(response);
    }
    else
    {
        tcflush(fd, TCIFLUSH);

        if ((errcode = tty_write(fd, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }

        if ((errcode = tty_read(fd, response, 1, IEQPRO_TIMEOUT, &nbytes_read)))
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }
    }

    if (nbytes_read > 0)
    {
        response[nbytes_read] = '\0';
        DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_DEBUG, "RES (%s)", response);

        tcflush(fd, TCIFLUSH);
        return true;
    }

    DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_ERROR, "Only received #%d bytes, expected 1.", nbytes_read);
    return false;
}

bool get_ieqpro_guide_rate(int fd, double *rate)
{
    char cmd[]  = ":AG#";
    int errcode = 0;
    char errmsg[MAXRBUF];
    char response[8];
    int nbytes_read    = 0;
    int nbytes_written = 0;

    DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_DEBUG, "CMD (%s)", cmd);

    if (ieqpro_simulation)
    {
        snprintf(response, 8, "%3d#", (int)(simData.guide_rate * 100));
        nbytes_read = strlen(response);
    }
    else
    {
        tcflush(fd, TCIFLUSH);

        if ((errcode = tty_write(fd, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }

        if ((errcode = tty_read(fd, response, 4, IEQPRO_TIMEOUT, &nbytes_read)))
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }
    }

    if (nbytes_read > 0)
    {
        response[nbytes_read] = '\0';
        DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_DEBUG, "RES (%s)", response);

        int rate_num;

        if (sscanf(response, "%d#", &rate_num) > 0)
        {
            *rate = rate_num / 100.0;
            tcflush(fd, TCIFLUSH);
            return true;
        }
        else
        {
            DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_ERROR, "Error: Malformed result (%s).", response);
            return false;
        }
    }

    DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_ERROR, "Only received #%d bytes, expected 1.", nbytes_read);
    return false;
}

bool start_ieqpro_guide(int fd, IEQ_DIRECTION dir, int ms)
{
    char cmd[16];
    int errcode = 0;
    char errmsg[MAXRBUF];
    int nbytes_written = 0;

    char dir_c = 0;

    switch (dir)
    {
        case IEQ_N:
            dir_c = 'n';
            break;

        case IEQ_S:
            dir_c = 's';
            break;

        case IEQ_W:
            dir_c = 'w';
            break;

        case IEQ_E:
            dir_c = 'e';
            break;
    }

    snprintf(cmd, 16, ":M%c%05d#", dir_c, ms);

    DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_DEBUG, "CMD (%s)", cmd);

    if (ieqpro_simulation)
        return true;
    else
    {
        tcflush(fd, TCIFLUSH);

        if ((errcode = tty_write(fd, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }
    }

    tcflush(fd, TCIFLUSH);
    return true;
}

bool park_ieqpro(int fd)
{
    char cmd[]  = ":MP1#";
    int errcode = 0;
    char errmsg[MAXRBUF];
    char response[8];
    int nbytes_read    = 0;
    int nbytes_written = 0;

    DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_DEBUG, "CMD (%s)", cmd);

    if (ieqpro_simulation)
    {
        simInfo.rememberSystemStatus = simInfo.systemStatus;
        set_sim_system_status(ST_SLEWING);
        strcpy(response, "1");
        nbytes_read = strlen(response);
    }
    else
    {
        tcflush(fd, TCIFLUSH);

        if ((errcode = tty_write(fd, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }

        if ((errcode = tty_read(fd, response, 1, IEQPRO_TIMEOUT, &nbytes_read)))
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }
    }

    if (nbytes_read > 0)
    {
        response[nbytes_read] = '\0';
        DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_DEBUG, "RES (%s)", response);

        if (!strcmp(response, "1"))
        {
            tcflush(fd, TCIFLUSH);
            return true;
        }
        else
        {
            DEBUGDEVICE(ieqpro_device, INDI::Logger::DBG_ERROR, "Error: Requested parking position is below horizon.");
            return false;
        }
    }

    DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_ERROR, "Only received #%d bytes, expected 1.", nbytes_read);
    return false;
}

bool unpark_ieqpro(int fd)
{
    char cmd[]  = ":MP0#";
    int errcode = 0;
    char errmsg[MAXRBUF];
    char response[8];
    int nbytes_read    = 0;
    int nbytes_written = 0;

    DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_DEBUG, "CMD (%s)", cmd);

    if (ieqpro_simulation)
    {
        set_sim_system_status(ST_STOPPED);
        strcpy(response, "1");
        nbytes_read = strlen(response);
    }
    else
    {
        tcflush(fd, TCIFLUSH);

        if ((errcode = tty_write(fd, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }

        if ((errcode = tty_read(fd, response, 1, IEQPRO_TIMEOUT, &nbytes_read)))
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }
    }

    if (nbytes_read > 0)
    {
        response[nbytes_read] = '\0';
        DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_DEBUG, "RES (%s)", response);

        tcflush(fd, TCIFLUSH);
        return true;
    }

    DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_ERROR, "Only received #%d bytes, expected 1.", nbytes_read);
    return false;
}

bool abort_ieqpro(int fd)
{
    char cmd[]  = ":Q#";
    int errcode = 0;
    char errmsg[MAXRBUF];
    char response[8];
    int nbytes_read    = 0;
    int nbytes_written = 0;

    DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_DEBUG, "CMD (%s)", cmd);

    if (ieqpro_simulation)
    {
        if (simInfo.systemStatus == ST_SLEWING)
            simInfo.systemStatus =  simInfo.rememberSystemStatus;
        strcpy(response, "1");
        nbytes_read = strlen(response);
    }
    else
    {
        tcflush(fd, TCIFLUSH);

        if ((errcode = tty_write(fd, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }

        if ((errcode = tty_read(fd, response, 1, IEQPRO_TIMEOUT, &nbytes_read)))
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }
    }

    if (nbytes_read > 0)
    {
        response[nbytes_read] = '\0';
        DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_DEBUG, "RES (%s)", response);

        tcflush(fd, TCIFLUSH);
        return true;
    }

    DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_ERROR, "Only received #%d bytes, expected 1.", nbytes_read);
    return false;
}

bool slew_ieqpro(int fd)
{
    char cmd[]  = ":MS#";
    int errcode = 0;
    char errmsg[MAXRBUF];
    char response[8];
    int nbytes_read    = 0;
    int nbytes_written = 0;

    DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_DEBUG, "CMD (%s)", cmd);

    if (ieqpro_simulation)
    {
        simInfo.rememberSystemStatus = simInfo.systemStatus;
        simInfo.systemStatus = ST_SLEWING;
        strcpy(response, "1");
        nbytes_read = strlen(response);
    }
    else
    {
        tcflush(fd, TCIFLUSH);

        if ((errcode = tty_write(fd, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }

        if ((errcode = tty_read(fd, response, 1, IEQPRO_TIMEOUT, &nbytes_read)))
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }
    }

    if (nbytes_read > 0)
    {
        response[nbytes_read] = '\0';
        DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_DEBUG, "RES (%s)", response);

        if (!strcmp(response, "1"))
        {
            tcflush(fd, TCIFLUSH);
            return true;
        }
        else
        {
            DEBUGDEVICE(ieqpro_device, INDI::Logger::DBG_ERROR, "Requested object is below horizon.");
            tcflush(fd, TCIFLUSH);
            return false;
        }
    }

    DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_ERROR, "Only received #%d bytes, expected 1.", nbytes_read);
    return false;
}

bool sync_ieqpro(int fd)
{
    char cmd[]  = ":CM#";
    int errcode = 0;
    char errmsg[MAXRBUF];
    char response[8];
    int nbytes_read    = 0;
    int nbytes_written = 0;

    DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_DEBUG, "CMD (%s)", cmd);

    if (ieqpro_simulation)
    {
        strcpy(response, "1");
        nbytes_read = strlen(response);
    }
    else
    {
        tcflush(fd, TCIFLUSH);

        if ((errcode = tty_write(fd, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }

        if ((errcode = tty_read(fd, response, 1, IEQPRO_TIMEOUT, &nbytes_read)))
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }
    }

    if (nbytes_read > 0)
    {
        response[nbytes_read] = '\0';
        DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_DEBUG, "RES (%s)", response);

        tcflush(fd, TCIFLUSH);
        return true;
    }

    DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_ERROR, "Only received #%d bytes, expected 1.", nbytes_read);
    return false;
}

bool set_ieqpro_track_enabled(int fd, bool enabled)
{
    char cmd[32];
    int errcode = 0;
    char errmsg[MAXRBUF];
    char response[8];
    int nbytes_read    = 0;
    int nbytes_written = 0;

    snprintf(cmd, 32, ":ST%d#", enabled ? 1 : 0);

    DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_DEBUG, "CMD (%s)", cmd);

    if (ieqpro_simulation)
    {
        simInfo.systemStatus = enabled ? ST_TRACKING_PEC_ON : ST_STOPPED;
        strcpy(response, "1");
        nbytes_read = strlen(response);
    }
    else
    {
        tcflush(fd, TCIFLUSH);

        if ((errcode = tty_write(fd, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }

        if ((errcode = tty_read(fd, response, 1, IEQPRO_TIMEOUT, &nbytes_read)))
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }
    }

    if (nbytes_read > 0)
    {
        response[nbytes_read] = '\0';
        DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_DEBUG, "RES (%s)", response);

        tcflush(fd, TCIFLUSH);
        return true;
    }

    DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_ERROR, "Only received #%d bytes, expected 1.", nbytes_read);
    return false;
}

bool set_ieqpro_ra(int fd, double ra)
{
    char cmd[32];
    int errcode = 0;
    char errmsg[MAXRBUF];
    char response[8];
    int nbytes_read    = 0;
    int nbytes_written = 0;

    // Send as milliseconds resolution
    int ieqValue = ra * 60 * 60 * 1000;

    snprintf(cmd, 32, ":Sr%08d#", ieqValue);

    DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_DEBUG, "CMD (%s)", cmd);

    if (ieqpro_simulation)
    {
        simData.ra = ra;
        strcpy(response, "1");
        nbytes_read = strlen(response);
    }
    else
    {
        tcflush(fd, TCIFLUSH);

        if ((errcode = tty_write(fd, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }

        if ((errcode = tty_read(fd, response, 1, IEQPRO_TIMEOUT, &nbytes_read)))
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }
    }

    if (nbytes_read > 0)
    {
        response[nbytes_read] = '\0';
        DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_DEBUG, "RES (%s)", response);

        tcflush(fd, TCIFLUSH);
        return true;
    }

    DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_ERROR, "Only received #%d bytes, expected 1.", nbytes_read);
    return false;
}

bool set_ieqpro_dec(int fd, double dec)
{
    char cmd[32];
    char sign;
    int errcode = 0;
    char errmsg[MAXRBUF];
    char response[8];
    int nbytes_read    = 0;
    int nbytes_written = 0;

    if (dec >= 0)
        sign = '+';
    else
        sign = '-';

    // Send as 0.01 arcseconds resolution
    int ieqValue = fabs(dec) * 60 * 60 * 100;

    snprintf(cmd, 32, ":Sd%c%08d#", sign, ieqValue);

    DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_DEBUG, "CMD (%s)", cmd);

    if (ieqpro_simulation)
    {
        simData.dec = dec;
        strcpy(response, "1");
        nbytes_read = strlen(response);
    }
    else
    {
        tcflush(fd, TCIFLUSH);

        if ((errcode = tty_write(fd, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }

        if ((errcode = tty_read(fd, response, 1, IEQPRO_TIMEOUT, &nbytes_read)))
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }
    }

    if (nbytes_read > 0)
    {
        response[nbytes_read] = '\0';
        DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_DEBUG, "RES (%s)", response);

        tcflush(fd, TCIFLUSH);
        return true;
    }

    DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_ERROR, "Only received #%d bytes, expected 1.", nbytes_read);
    return false;
}

bool set_ieqpro_longitude(int fd, double longitude)
{
    char cmd[16];
    char sign;
    int errcode = 0;
    char errmsg[MAXRBUF];
    char response[8];
    int nbytes_read    = 0;
    int nbytes_written = 0;

    if (longitude >= 0)
        sign = '+';
    else
        sign = '-';

    int longitude_arcsecs = fabs(longitude) * 60 * 60;
    snprintf(cmd, 16, ":Sg%c%06d#", sign, longitude_arcsecs);

    DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_DEBUG, "CMD (%s)", cmd);

    if (ieqpro_simulation)
    {
        strcpy(response, "1");
        nbytes_read = strlen(response);
    }
    else
    {
        tcflush(fd, TCIFLUSH);

        if ((errcode = tty_write(fd, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }

        if ((errcode = tty_read(fd, response, 1, IEQPRO_TIMEOUT, &nbytes_read)))
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }
    }

    if (nbytes_read > 0)
    {
        response[nbytes_read] = '\0';
        DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_DEBUG, "RES (%s)", response);

        tcflush(fd, TCIFLUSH);
        return true;
    }

    DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_ERROR, "Only received #%d bytes, expected 1.", nbytes_read);
    return false;
}

bool set_ieqpro_latitude(int fd, double latitude)
{
    char cmd[16];
    char sign;
    int errcode = 0;
    char errmsg[MAXRBUF];
    char response[8];
    int nbytes_read    = 0;
    int nbytes_written = 0;

    if (latitude >= 0)
        sign = '+';
    else
        sign = '-';

    int latitude_arcsecs = fabs(latitude) * 60 * 60;
    snprintf(cmd, 16, ":St%c%06d#", sign, latitude_arcsecs);

    DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_DEBUG, "CMD (%s)", cmd);

    if (ieqpro_simulation)
    {
        strcpy(response, "1");
        nbytes_read = strlen(response);
    }
    else
    {
        tcflush(fd, TCIFLUSH);

        if ((errcode = tty_write(fd, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }

        if ((errcode = tty_read(fd, response, 1, IEQPRO_TIMEOUT, &nbytes_read)))
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }
    }

    if (nbytes_read > 0)
    {
        response[nbytes_read] = '\0';
        DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_DEBUG, "RES (%s)", response);

        tcflush(fd, TCIFLUSH);
        return true;
    }

    DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_ERROR, "Only received #%d bytes, expected 1.", nbytes_read);
    return false;
}

bool get_ieqpro_longitude(int fd, double *longitude)
{
    char cmd[16];
    int errcode = 0;
    char errmsg[MAXRBUF];
    char response[8];
    int nbytes_read    = 0;
    int nbytes_written = 0;

    strcpy(cmd, ":Gg#");

    DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_DEBUG, "CMD (%s)", cmd);

    if (ieqpro_simulation)
    {
        strcpy(response, "+172800");
        nbytes_read = strlen(response);
    }
    else
    {
        tcflush(fd, TCIFLUSH);

        if ((errcode = tty_write(fd, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }
        if ((errcode = tty_read_section(fd, response, '#', IEQPRO_TIMEOUT, &nbytes_read)))
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }
    }

    if (nbytes_read > 0)
    {
        response[nbytes_read] = '\0';
        DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_DEBUG, "RES (%s)", response);

        tcflush(fd, TCIFLUSH);

        int longitude_arcsecs = 0;

        if (sscanf(response, "%d#", &longitude_arcsecs) > 0)
        {
            *longitude = longitude_arcsecs / 3600.0;
            return true;
        }

        DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_ERROR, "Error: Malformed result (%s).", response);
        return false;
    }

    DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_ERROR, "Only received #%d bytes, expected 8.", nbytes_read);
    return false;
}

bool get_ieqpro_latitude(int fd, double *latitude)
{
    char cmd[16];
    int errcode = 0;
    char errmsg[MAXRBUF];
    char response[8];
    int nbytes_read    = 0;
    int nbytes_written = 0;

    strcpy(cmd, ":Gt#");

    DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_DEBUG, "CMD (%s)", cmd);

    if (ieqpro_simulation)
    {
        strcpy(response, "+106200");
        nbytes_read = strlen(response);
    }
    else
    {
        tcflush(fd, TCIFLUSH);

        if ((errcode = tty_write(fd, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }
        if ((errcode = tty_read_section(fd, response, '#', IEQPRO_TIMEOUT, &nbytes_read)))
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }
    }

    if (nbytes_read > 0)
    {
        response[nbytes_read] = '\0';
        DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_DEBUG, "RES (%s)", response);

        tcflush(fd, TCIFLUSH);

        int latitude_arcsecs = 0;

        if (sscanf(response, "%d#", &latitude_arcsecs) > 0)
        {
            *latitude = latitude_arcsecs / 3600.0;
            return true;
        }

        DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_ERROR, "Error: Malformed result (%s).", response);
        return false;
    }

    DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_ERROR, "Only received #%d bytes, expected 8.", nbytes_read);
    return false;
}

bool set_ieqpro_local_date(int fd, int yy, int mm, int dd)
{
    char cmd[16];
    int errcode = 0;
    char errmsg[MAXRBUF];
    char response[8];
    int nbytes_read    = 0;
    int nbytes_written = 0;

    snprintf(cmd, 16, ":SC%02d%02d%02d#", yy, mm, dd);

    DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_DEBUG, "CMD (%s)", cmd);

    if (ieqpro_simulation)
    {
        strcpy(response, "1");
        nbytes_read = strlen(response);
    }
    else
    {
        tcflush(fd, TCIFLUSH);

        if ((errcode = tty_write(fd, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }

        if ((errcode = tty_read(fd, response, 1, IEQPRO_TIMEOUT, &nbytes_read)))
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }
    }

    if (nbytes_read > 0)
    {
        response[nbytes_read] = '\0';
        DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_DEBUG, "RES (%s)", response);

        tcflush(fd, TCIFLUSH);
        return true;
    }

    DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_ERROR, "Only received #%d bytes, expected 1.", nbytes_read);
    return false;
}

bool set_ieqpro_local_time(int fd, int hh, int mm, int ss)
{
    char cmd[16];
    int errcode = 0;
    char errmsg[MAXRBUF];
    char response[8];
    int nbytes_read    = 0;
    int nbytes_written = 0;

    snprintf(cmd, 16, ":SL%02d%02d%02d#", hh, mm, ss);

    DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_DEBUG, "CMD (%s)", cmd);

    if (ieqpro_simulation)
    {
        strcpy(response, "1");
        nbytes_read = strlen(response);
    }
    else
    {
        tcflush(fd, TCIFLUSH);

        if ((errcode = tty_write(fd, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }

        if ((errcode = tty_read(fd, response, 1, IEQPRO_TIMEOUT, &nbytes_read)))
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }
    }

    if (nbytes_read > 0)
    {
        response[nbytes_read] = '\0';
        DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_DEBUG, "RES (%s)", response);

        tcflush(fd, TCIFLUSH);
        return true;
    }

    DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_ERROR, "Only received #%d bytes, expected 1.", nbytes_read);
    return false;
}

bool set_ieqpro_daylight_saving(int fd, bool enabled)
{
    char cmd[16];
    int errcode = 0;
    char errmsg[MAXRBUF];
    char response[8];
    int nbytes_read    = 0;
    int nbytes_written = 0;

    if (enabled)
        strcpy(cmd, ":SDS1#");
    else
        strcpy(cmd, ":SDS0#");

    DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_DEBUG, "CMD (%s)", cmd);

    if (ieqpro_simulation)
    {
        strcpy(response, "1");
        nbytes_read = strlen(response);
    }
    else
    {
        tcflush(fd, TCIFLUSH);

        if ((errcode = tty_write(fd, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }

        if ((errcode = tty_read(fd, response, 1, IEQPRO_TIMEOUT, &nbytes_read)))
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }
    }

    if (nbytes_read > 0)
    {
        response[nbytes_read] = '\0';
        DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_DEBUG, "RES (%s)", response);

        tcflush(fd, TCIFLUSH);
        return true;
    }

    DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_ERROR, "Only received #%d bytes, expected 1.", nbytes_read);
    return false;
}

bool set_ieqpro_utc_offset(int fd, double offset)
{
    char cmd[16];
    char sign;
    int errcode = 0;
    char errmsg[MAXRBUF];
    char response[8];
    int nbytes_read    = 0;
    int nbytes_written = 0;

    if (offset >= 0)
        sign = '+';
    else
        sign = '-';

    int offset_minutes = fabs(offset) * 60.0;

    snprintf(cmd, 16, ":SG%c%03d#", sign, offset_minutes);

    DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_DEBUG, "CMD (%s)", cmd);

    if (ieqpro_simulation)
    {
        strcpy(response, "1");
        nbytes_read = strlen(response);
    }
    else
    {
        tcflush(fd, TCIFLUSH);

        if ((errcode = tty_write(fd, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }

        if ((errcode = tty_read(fd, response, 1, IEQPRO_TIMEOUT, &nbytes_read)))
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }
    }

    if (nbytes_read > 0)
    {
        response[nbytes_read] = '\0';
        DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_DEBUG, "RES (%s)", response);

        tcflush(fd, TCIFLUSH);
        return true;
    }

    DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_ERROR, "Only received #%d bytes, expected 1.", nbytes_read);
    return false;
}

bool get_ieqpro_coords(int fd, double *ra, double *dec)
{
    char cmd[]  = ":GEC#";
    int errcode = 0;
    char errmsg[MAXRBUF];
    char response[32];
    int nbytes_read    = 0;
    int nbytes_written = 0;

    DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_EXTRA_1, "CMD (%s)", cmd);

    if (ieqpro_simulation)
    {
        char ra_str[16], dec_str[16];

        char sign;
        if (simData.dec >= 0)
            sign = '+';
        else
            sign = '-';

        int ieqDEC = fabs(simData.dec) * 60 * 60 * 100;

        snprintf(dec_str, 16, "%c%08d", sign, ieqDEC);

        int ieqRA = simData.ra * 60 * 60 * 1000;
        snprintf(ra_str, 16, "%08d", ieqRA);

        snprintf(response, 32, "%s%s#", dec_str, ra_str);
        nbytes_read = strlen(response);
    }
    else
    {
        tcflush(fd, TCIFLUSH);

        if ((errcode = tty_write(fd, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }

        if ((errcode = tty_read_section(fd, response, '#', IEQPRO_TIMEOUT, &nbytes_read)))
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }
    }

    if (nbytes_read > 0)
    {
        tcflush(fd, TCIFLUSH);
        response[nbytes_read] = '\0';
        DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_EXTRA_1, "RES (%s)", response);

        char ra_str[16]= {0}, dec_str[16] = {0};

        strncpy(dec_str, response, 9);
        strncpy(ra_str, response + 9, 8);

        int ieqDEC = atoi(dec_str);
        int ieqRA  = atoi(ra_str);

        *ra  = ieqRA / (60.0 * 60.0 * 1000.0);
        *dec = ieqDEC / (60.0 * 60.0 * 100.0);

        return true;
    }

    DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_ERROR, "Only received #%d bytes, expected 1.", nbytes_read);
    return false;
}

bool get_ieqpro_utc_date_time(int fd, double *utc_hours, int *yy, int *mm, int *dd, int *hh, int *minute, int *ss)
{
    char cmd[]  = ":GLT#";
    int errcode = 0;
    char errmsg[MAXRBUF];
    char response[32];
    int nbytes_read    = 0;
    int nbytes_written = 0;

    DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_DEBUG, "CMD (%s)", cmd);

    // Format according to Manual is sMMMYYMMDDHHMMSS#
    // However as pointed out by user Shepherd on INDI forums, actual format is
    // sMMMxYYMMDDHHMMSS#
    // Where x is either 0 or 1 denoting daying savings
    if (ieqpro_simulation)
    {
        strncpy(response, "+1800150331173000#", 32);
        nbytes_read = strlen(response);
    }
    else
    {
        tcflush(fd, TCIFLUSH);

        if ((errcode = tty_write(fd, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }

        if ((errcode = tty_read_section(fd, response, '#', IEQPRO_TIMEOUT, &nbytes_read)))
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_ERROR, "%s", errmsg);
            return false;
        }
    }

    if (nbytes_read > 0)
    {
        tcflush(fd, TCIFLUSH);
        response[nbytes_read] = '\0';
        DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_DEBUG, "RES (%s)", response);

        char utc_str[8]={0}, yy_str[8]={0}, mm_str[8]={0}, dd_str[8]={0}, hh_str[8]={0}, minute_str[8]={0}, ss_str[8]={0}, dst_str[8]={0};

        // UTC Offset
        strncpy(utc_str, response, 4);
        // Daylight savings
        strncpy(dst_str, response + 4, 1);
        // Year
        strncpy(yy_str, response + 5, 2);
        // Month
        strncpy(mm_str, response + 7, 2);
        // Day
        strncpy(dd_str, response + 9, 2);
        // Hour
        strncpy(hh_str, response + 11, 2);
        // Minute
        strncpy(minute_str, response + 13, 2);
        // Second
        strncpy(ss_str, response + 15, 2);

        *utc_hours = atoi(utc_str) / 60.0;
        *yy        = atoi(yy_str) + 2000;
        *mm        = atoi(mm_str) + 1;
        *dd        = atoi(dd_str);
        *hh        = atoi(hh_str);
        *minute    = atoi(minute_str);
        *ss        = atoi(ss_str);

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

    DEBUGFDEVICE(ieqpro_device, INDI::Logger::DBG_ERROR, "Only received #%d bytes, expected 1.", nbytes_read);
    return false;
}

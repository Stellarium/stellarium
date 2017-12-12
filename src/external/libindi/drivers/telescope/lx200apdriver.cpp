#if 0
LX200 Astro - Physics Driver
Copyright (C) 2007 Markus Wildi

This library is free software;
you can redistribute it and / or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation;
either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY;
without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library;
if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110 - 1301  USA

#endif

#include <cmath>
#include "lx200apdriver.h"

#include "indicom.h"
#include "indilogger.h"
#include "lx200driver.h"

#include <cstring>
#include <unistd.h>

#ifndef _WIN32
#include <termios.h>
#endif

#define LX200_TIMEOUT 5 /* FD timeout in seconds */

char lx200ap_name[MAXINDIDEVICE];
unsigned int AP_DBG_SCOPE;

void set_lx200ap_name(const char *deviceName, unsigned int debug_level)
{
    strncpy(lx200ap_name, deviceName, MAXINDIDEVICE);
    AP_DBG_SCOPE = debug_level;
}

int check_lx200ap_connection(int fd)
{
    int i = 0;
    char temp_string[64];
    int error_type;
    int nbytes_write = 0;
    int nbytes_read  = 0;

    DEBUGDEVICE(lx200ap_name, INDI::Logger::DBG_DEBUG, "Testing telescope's connection using #:GG#...");

    if (fd <= 0)
    {
        DEBUGDEVICE(lx200ap_name, INDI::Logger::DBG_ERROR,
                    "check_lx200ap_connection: not a valid file descriptor received");

        return -1;
    }
    for (i = 0; i < 2; i++)
    {
        if ((error_type = tty_write_string(fd, "#:GG#", &nbytes_write)) != TTY_OK)
        {
            DEBUGFDEVICE(lx200ap_name, INDI::Logger::DBG_ERROR,
                         "check_lx200ap_connection: unsuccessful write to telescope, %d", nbytes_write);

            return error_type;
        }
        tty_read_section(fd, temp_string, '#', LX200_TIMEOUT, &nbytes_read);
        tcflush(fd, TCIFLUSH);
        if (nbytes_read > 1)
        {
            temp_string[nbytes_read - 1] = '\0';

            DEBUGFDEVICE(lx200ap_name, INDI::Logger::DBG_ERROR, "check_lx200ap_connection: received bytes %d, [%s]",
                         nbytes_write, temp_string);

            return 0;
        }
        usleep(50000);
    }

    DEBUGDEVICE(lx200ap_name, INDI::Logger::DBG_ERROR, "check_lx200ap_connection: wrote, but nothing received.");

    return -1;
}
int getAPUTCOffset(int fd, double *value)
{
    int error_type;
    int nbytes_write = 0;
    int nbytes_read  = 0;

    char temp_string[16];

    DEBUGFDEVICE(lx200ap_name, AP_DBG_SCOPE, "CMD <%s>", "#:GG#");

    if ((error_type = tty_write_string(fd, "#:GG#", &nbytes_write)) != TTY_OK)
        return error_type;

    if ((error_type = tty_read_section(fd, temp_string, '#', LX200_TIMEOUT, &nbytes_read)) != TTY_OK)
    {
        DEBUGFDEVICE(lx200ap_name, INDI::Logger::DBG_ERROR, "getAPUTCOffset: saying good bye %d, %d", error_type,
                     nbytes_read);
        return error_type;
    }

    tcflush(fd, TCIFLUSH);

    DEBUGFDEVICE(lx200ap_name, AP_DBG_SCOPE, "RES <%s>", temp_string);

    /* Negative offsets, see AP keypad manual p. 77 */
    if ((temp_string[0] == 'A') || ((temp_string[0] == '0') && (temp_string[1] == '0')) || (temp_string[0] == '@'))
    {
        int i;
        for (i = nbytes_read; i > 0; i--)
        {
            temp_string[i] = temp_string[i - 1];
        }
        temp_string[0]               = '-';
        temp_string[nbytes_read + 1] = '\0';

        if (temp_string[1] == 'A')
        {
            temp_string[1] = '0';
            switch (temp_string[2])
            {
            case '5':

                temp_string[2] = '1';
                break;
            case '4':

                temp_string[2] = '2';
                break;
            case '3':

                temp_string[2] = '3';
                break;
            case '2':

                temp_string[2] = '4';
                break;
            case '1':

                temp_string[2] = '5';
                break;
            default:
                DEBUGFDEVICE(lx200ap_name, INDI::Logger::DBG_ERROR, "getAPUTCOffset: string not handled %s",
                             temp_string);
                return -1;
                break;
            }
        }
        else if (temp_string[1] == '0')
        {
            temp_string[1] = '0';
            temp_string[2] = '6';
        }
        else if (temp_string[1] == '@')
        {
            temp_string[1] = '0';
            switch (temp_string[2])
            {
            case '9':

                temp_string[2] = '7';
                break;
            case '8':

                temp_string[2] = '8';
                break;
            case '7':

                temp_string[2] = '9';
                break;
            case '6':

                temp_string[2] = '0';
                break;
            case '5':
                temp_string[1] = '1';
                temp_string[2] = '1';
                break;
            case '4':

                temp_string[1] = '1';
                temp_string[2] = '2';
                break;
            default:
                DEBUGFDEVICE(lx200ap_name, INDI::Logger::DBG_ERROR, "getAPUTCOffset: string not handled %s",
                             temp_string);
                return -1;
                break;
            }
        }
        else
        {
            DEBUGFDEVICE(lx200ap_name, INDI::Logger::DBG_ERROR, "getAPUTCOffset: string not handled %s", temp_string);
        }
    }
    else
    {
        temp_string[nbytes_read - 1] = '\0';
    }

    if (f_scansexa(temp_string, value))
    {
        DEBUGFDEVICE(lx200ap_name, INDI::Logger::DBG_ERROR, "getAPUTCOffset: unable to process %s", temp_string);
        return -1;
    }
    return 0;
}

int setAPObjectAZ(int fd, double az)
{
    int h, m, s;
    char temp_string[16];

    getSexComponents(az, &h, &m, &s);

    snprintf(temp_string, sizeof(temp_string), "#:Sz %03d*%02d:%02d#", h, m, s);

    DEBUGFDEVICE(lx200ap_name, AP_DBG_SCOPE, "CMD <%s>", temp_string);

    return (setStandardProcedure(fd, temp_string));
}

/* wildi Valid set Values are positive, add error condition */

int setAPObjectAlt(int fd, double alt)
{
    int d, m, s;
    char temp_string[16];

    getSexComponents(alt, &d, &m, &s);

    /* case with negative zero */
    if (!d && alt < 0)
    {
        snprintf(temp_string, sizeof(temp_string), "#:Sa -%02d*%02d:%02d#", d, m, s);
    }
    else
    {
        snprintf(temp_string, sizeof(temp_string), "#:Sa %+02d*%02d:%02d#", d, m, s);
    }

    DEBUGFDEVICE(lx200ap_name, AP_DBG_SCOPE, "CMD <%s>", temp_string);

    return (setStandardProcedure(fd, temp_string));
}
int setAPUTCOffset(int fd, double hours)
{
    int h, m, s;

    char temp_string[16];

    getSexComponents(hours, &h, &m, &s);

    snprintf(temp_string, sizeof(temp_string), "#:SG %+03d:%02d:%02d#", h, m, s);

    DEBUGFDEVICE(lx200ap_name, AP_DBG_SCOPE, "CMD <%s>", temp_string);

    return (setStandardProcedure(fd, temp_string));
}
int APSyncCM(int fd, char *matchedObject)
{
    int error_type;
    int nbytes_write = 0;
    int nbytes_read  = 0;

    DEBUGFDEVICE(lx200ap_name, AP_DBG_SCOPE, "CMD <%s>", "#:CM#");

    if ((error_type = tty_write_string(fd, "#:CM#", &nbytes_write)) != TTY_OK)
        return error_type;

    if ((error_type = tty_read_section(fd, matchedObject, '#', LX200_TIMEOUT, &nbytes_read)) != TTY_OK)
        return error_type;

    matchedObject[nbytes_read - 1] = '\0';

    DEBUGFDEVICE(lx200ap_name, AP_DBG_SCOPE, "RES <%s>", matchedObject);

    /* Sleep 10ms before flushing. This solves some issues with LX200 compatible devices. */
    usleep(10000);

    tcflush(fd, TCIFLUSH);

    return 0;
}

int APSyncCMR(int fd, char *matchedObject)
{
    int error_type;
    int nbytes_write = 0;
    int nbytes_read  = 0;

    DEBUGFDEVICE(lx200ap_name, AP_DBG_SCOPE, "CMD <%s>", "#:CMR#");

    if ((error_type = tty_write_string(fd, "#:CMR#", &nbytes_write)) != TTY_OK)
        return error_type;

    /* read_ret = portRead(matchedObject, -1, LX200_TIMEOUT); */
    if ((error_type = tty_read_section(fd, matchedObject, '#', LX200_TIMEOUT, &nbytes_read)) != TTY_OK)
        return error_type;

    matchedObject[nbytes_read - 1] = '\0';

    DEBUGFDEVICE(lx200ap_name, AP_DBG_SCOPE, "RES <%s>", matchedObject);

    /* Sleep 10ms before flushing. This solves some issues with LX200 compatible devices. */
    usleep(10000);

    tcflush(fd, TCIFLUSH);

    return 0;
}

int selectAPPECState(int fd, int pecstate)
{
    int error_type;
    int nbytes_write = 0;

    switch (pecstate)
    {
    // PEC OFF
    case 0:
        DEBUGDEVICE(lx200ap_name, INDI::Logger::DBG_DEBUG, "selectAPPECState: Setting PEC OFF");
        DEBUGFDEVICE(lx200ap_name, AP_DBG_SCOPE, "CMD <%s>", "#:p#");

        if ((error_type = tty_write_string(fd, "#:p#", &nbytes_write)) != TTY_OK)
            return error_type;

        break;

        // PEC ON
    case 1:
        DEBUGDEVICE(lx200ap_name, INDI::Logger::DBG_DEBUG, "selectAPPECState: Setting PEC ON");
        DEBUGFDEVICE(lx200ap_name, AP_DBG_SCOPE, "CMD <%s>", "#:pP#");

        if ((error_type = tty_write_string(fd, "#:pP#", &nbytes_write)) != TTY_OK)
            return error_type;

        break;

    default:
        return -1;
        break;
    }

    return 0;
}

int selectAPMoveToRate(int fd, int moveToRate)
{
    int error_type;
    int nbytes_write = 0;

    switch (moveToRate)
    {
    /* 12x*/
    case 0:
        DEBUGDEVICE(lx200ap_name, INDI::Logger::DBG_DEBUG, "selectAPMoveToRate: Setting move to rate to 12x");
        DEBUGFDEVICE(lx200ap_name, AP_DBG_SCOPE, "CMD <%s>", "#:RC0#");

        if ((error_type = tty_write_string(fd, "#:RC0#", &nbytes_write)) != TTY_OK)
            return error_type;
        break;

    /* 64x */
    case 1:
        DEBUGDEVICE(lx200ap_name, INDI::Logger::DBG_DEBUG, "selectAPMoveToRate: Setting move to rate to 64x");
        DEBUGFDEVICE(lx200ap_name, AP_DBG_SCOPE, "CMD <%s>", "#:RC1#");

        if ((error_type = tty_write_string(fd, "#:RC1#", &nbytes_write)) != TTY_OK)
            return error_type;
        break;

    /* 600x */
    case 2:
        DEBUGDEVICE(lx200ap_name, INDI::Logger::DBG_DEBUG, "selectAPMoveToRate: Setting move to rate to 600x");
        DEBUGFDEVICE(lx200ap_name, AP_DBG_SCOPE, "CMD <%s>", "#:RC2#");
        if ((error_type = tty_write_string(fd, "#:RC2#", &nbytes_write)) != TTY_OK)
            return error_type;
        break;

    /* 1200x */
    case 3:
        DEBUGDEVICE(lx200ap_name, INDI::Logger::DBG_DEBUG, "selectAPMoveToRate: Setting move to rate to 1200x");
        DEBUGFDEVICE(lx200ap_name, AP_DBG_SCOPE, "CMD <%s>", "#:RC3#");

        if ((error_type = tty_write_string(fd, "#:RC3#", &nbytes_write)) != TTY_OK)
            return error_type;
        break;

    default:
        return -1;
        break;
    }
    return 0;
}

int selectAPSlewRate(int fd, int slewRate)
{
    int error_type;
    int nbytes_write = 0;
    switch (slewRate)
    {
    /* 600x */
    case 0:

        DEBUGDEVICE(lx200ap_name, INDI::Logger::DBG_DEBUG, "selectAPSlewRate: Setting slew to rate to 600x");
        DEBUGFDEVICE(lx200ap_name, AP_DBG_SCOPE, "CMD <%s>", "#:RS0#");

        if ((error_type = tty_write_string(fd, "#:RS0#", &nbytes_write)) != TTY_OK)
            return error_type;
        break;

    /* 900x */
    case 1:

        DEBUGDEVICE(lx200ap_name, INDI::Logger::DBG_DEBUG, "selectAPSlewRate: Setting slew to rate to 900x");
        DEBUGFDEVICE(lx200ap_name, AP_DBG_SCOPE, "CMD <%s>", "#:RS1#");

        if ((error_type = tty_write_string(fd, "#:RS1#", &nbytes_write)) != TTY_OK)
            return error_type;
        break;


    /* 1200x */
    case 2:

        DEBUGDEVICE(lx200ap_name, INDI::Logger::DBG_DEBUG, "selectAPSlewRate: Setting slew to rate to 1200x");
        DEBUGFDEVICE(lx200ap_name, AP_DBG_SCOPE, "CMD <%s>", "#:RS2#");

        if ((error_type = tty_write_string(fd, "#:RS2#", &nbytes_write)) != TTY_OK)
            return error_type;
        break;

    default:
        return -1;
        break;
    }
    return 0;
}

int selectAPTrackingMode(int fd, int trackMode)
{
    int error_type;
    int nbytes_write = 0;

    switch (trackMode)
    {
    /* Sidereal */
    case AP_TRACKING_SIDEREAL:

        DEBUGDEVICE(lx200ap_name, INDI::Logger::DBG_DEBUG,
                    "selectAPTrackingMode: Setting tracking mode to sidereal.");
        DEBUGFDEVICE(lx200ap_name, AP_DBG_SCOPE, "CMD <%s>", "#:RT2#");

        if ((error_type = tty_write_string(fd, "#:RT2#", &nbytes_write)) != TTY_OK)
            return error_type;
        break;

        /* Solar */
    case AP_TRACKING_SOLAR:

        DEBUGDEVICE(lx200ap_name, INDI::Logger::DBG_DEBUG, "selectAPTrackingMode: Setting tracking mode to solar.");
        DEBUGFDEVICE(lx200ap_name, AP_DBG_SCOPE, "CMD <%s>", "#:RT1#");

        if ((error_type = tty_write_string(fd, "#:RT1#", &nbytes_write)) != TTY_OK)
            return error_type;
        break;

        /* Lunar */
    case AP_TRACKING_LUNAR:

        DEBUGDEVICE(lx200ap_name, INDI::Logger::DBG_DEBUG, "selectAPTrackingMode: Setting tracking mode to lunar.");
        DEBUGFDEVICE(lx200ap_name, AP_DBG_SCOPE, "CMD <%s>", "#:RT0#");

        if ((error_type = tty_write_string(fd, "#:RT0#", &nbytes_write)) != TTY_OK)
            return error_type;
        break;

    case AP_TRACKING_CUSTOM:
            DEBUGDEVICE(lx200ap_name, INDI::Logger::DBG_DEBUG, "selectAPTrackingMode: Setting tracking mode to Custom.");
            break;

        /* Zero */
    case AP_TRACKING_OFF:

        DEBUGDEVICE(lx200ap_name, INDI::Logger::DBG_DEBUG, "selectAPTrackingMode: Setting tracking mode to Zero.");
        DEBUGFDEVICE(lx200ap_name, AP_DBG_SCOPE, "CMD <%s>", "#:RT9#");

        if ((error_type = tty_write_string(fd, "#:RT9#", &nbytes_write)) != TTY_OK)
            return error_type;
        break;

    default:
        return -1;
        break;
    }
    return 0;
}

int selectAPGuideRate(int fd, int guideRate)
{
    int error_type;
    int nbytes_write = 0;
    switch (guideRate)
    {
    /* 0.25x */
    case 0:

        DEBUGDEVICE(lx200ap_name, INDI::Logger::DBG_DEBUG, "selectAPGuideRate: Setting guide to rate to 0.25x");
        DEBUGFDEVICE(lx200ap_name, AP_DBG_SCOPE, "CMD <%s>", "#:RG0#");

        if ((error_type = tty_write_string(fd, "#:RG0#", &nbytes_write)) != TTY_OK)
            return error_type;
        break;

    /* 0.50x */
    case 1:

        DEBUGDEVICE(lx200ap_name, INDI::Logger::DBG_DEBUG, "selectAPGuideRate: Setting guide to rate to 0.50x");
        DEBUGFDEVICE(lx200ap_name, AP_DBG_SCOPE, "CMD <%s>", "#:RG1#");

        if ((error_type = tty_write_string(fd, "#:RG1#", &nbytes_write)) != TTY_OK)
            return error_type;
        break;


    /* 1.00x */
    case 2:

        DEBUGDEVICE(lx200ap_name, INDI::Logger::DBG_DEBUG, "selectAPGuideRate: Setting guide to rate to 1.00x");
        DEBUGFDEVICE(lx200ap_name, AP_DBG_SCOPE, "CMD <%s>", "#:RG2#");

        if ((error_type = tty_write_string(fd, "#:RG2#", &nbytes_write)) != TTY_OK)
            return error_type;
        break;

    default:
        return -1;
        break;
    }
    return 0;
}

int swapAPButtons(int fd, int currentSwap)
{
    int error_type;
    int nbytes_write = 0;

    switch (currentSwap)
    {
    case 0:

        DEBUGFDEVICE(lx200ap_name, AP_DBG_SCOPE, "CMD <%s>", "#:NS#");
        if ((error_type = tty_write_string(fd, "#:NS#", &nbytes_write)) != TTY_OK)
            return error_type;
        break;

    case 1:
        DEBUGFDEVICE(lx200ap_name, AP_DBG_SCOPE, "CMD <%s>", "#:EW#");
        if ((error_type = tty_write_string(fd, "#:EW#", &nbytes_write)) != TTY_OK)
            return error_type;
        break;

    default:
        return -1;
        break;
    }
    return 0;
}

int setAPObjectRA(int fd, double ra)
{
    /*ToDo AP accepts "#:Sr %02d:%02d:%02d.%1d#"*/
    int h, m, s;
    char temp_string[16];

    getSexComponents(ra, &h, &m, &s);

    snprintf(temp_string, sizeof(temp_string), "#:Sr %02d:%02d:%02d#", h, m, s);

    DEBUGFDEVICE(lx200ap_name, AP_DBG_SCOPE, "CMD <%s>", temp_string);

    return (setStandardProcedure(fd, temp_string));
}

int setAPObjectDEC(int fd, double dec)
{
    int d, m, s;
    char temp_string[16];

    getSexComponents(dec, &d, &m, &s);
    /* case with negative zero */
    if (!d && dec < 0)
    {
        snprintf(temp_string, sizeof(temp_string), "#:Sd -%02d*%02d:%02d#", d, m, s);
    }
    else
    {
        snprintf(temp_string, sizeof(temp_string), "#:Sd %+03d*%02d:%02d#", d, m, s);
    }

    DEBUGFDEVICE(lx200ap_name, AP_DBG_SCOPE, "CMD <%s>", temp_string);

    return (setStandardProcedure(fd, temp_string));
}

int setAPSiteLongitude(int fd, double Long)
{
    int d, m, s;
    char temp_string[32];

    getSexComponents(Long, &d, &m, &s);
    snprintf(temp_string, sizeof(temp_string), "#:Sg %03d*%02d:%02d#", d, m, s);

    DEBUGFDEVICE(lx200ap_name, AP_DBG_SCOPE, "CMD <%s>", temp_string);

    return (setStandardProcedure(fd, temp_string));
}

int setAPSiteLatitude(int fd, double Lat)
{
    int d, m, s;
    char temp_string[32];

    getSexComponents(Lat, &d, &m, &s);
    snprintf(temp_string, sizeof(temp_string), "#:St %+03d*%02d:%02d#", d, m, s);

    DEBUGFDEVICE(lx200ap_name, AP_DBG_SCOPE, "CMD <%s>", temp_string);

    return (setStandardProcedure(fd, temp_string));
}

int setAPRATrackRate(int fd, double rate)
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

    snprintf(cmd, 16, ":RR%c%03.4f#", sign, fabs(rate));

    DEBUGFDEVICE(lx200ap_name, INDI::Logger::DBG_DEBUG, "CMD (%s)", cmd);

    tcflush(fd, TCIFLUSH);

    if ((errcode = tty_write(fd, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
    {
        tty_error_msg(errcode, errmsg, MAXRBUF);
        DEBUGFDEVICE(lx200ap_name, INDI::Logger::DBG_ERROR, "%s", errmsg);
        return errcode;
    }

    if ((errcode = tty_read(fd, response, 1, LX200_TIMEOUT, &nbytes_read)))
    {
        tty_error_msg(errcode, errmsg, MAXRBUF);
        DEBUGFDEVICE(lx200ap_name, INDI::Logger::DBG_ERROR, "%s", errmsg);
        return errcode;
    }

    if (nbytes_read > 0)
    {
        response[nbytes_read] = '\0';
        DEBUGFDEVICE(lx200ap_name, INDI::Logger::DBG_DEBUG, "RES (%s)", response);

        tcflush(fd, TCIFLUSH);
        return 0;
    }

    DEBUGFDEVICE(lx200ap_name, INDI::Logger::DBG_ERROR, "Only received #%d bytes, expected 1.", nbytes_read);
    return -1;
}

int setAPDETrackRate(int fd, double rate)
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

    snprintf(cmd, 16, ":RD%c%03.4f#", sign, fabs(rate));

    DEBUGFDEVICE(lx200ap_name, INDI::Logger::DBG_DEBUG, "CMD (%s)", cmd);


    tcflush(fd, TCIFLUSH);

    if ((errcode = tty_write(fd, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
    {
        tty_error_msg(errcode, errmsg, MAXRBUF);
        DEBUGFDEVICE(lx200ap_name, INDI::Logger::DBG_ERROR, "%s", errmsg);
        return errcode;
    }

    if ((errcode = tty_read(fd, response, 1, LX200_TIMEOUT, &nbytes_read)))
    {
        tty_error_msg(errcode, errmsg, MAXRBUF);
        DEBUGFDEVICE(lx200ap_name, INDI::Logger::DBG_ERROR, "%s", errmsg);
        return errcode;
    }

    if (nbytes_read > 0)
    {
        response[nbytes_read] = '\0';
        DEBUGFDEVICE(lx200ap_name, INDI::Logger::DBG_DEBUG, "RES (%s)", response);

        tcflush(fd, TCIFLUSH);
        return 0;
    }

    DEBUGFDEVICE(lx200ap_name, INDI::Logger::DBG_ERROR, "Only received #%d bytes, expected 1.", nbytes_read);
    return -1;
}

int APSendPulseCmd(int fd, int direction, int duration_msec)
{
    DEBUGFDEVICE(lx200ap_name, AP_DBG_SCOPE, "<%s>", __FUNCTION__);
    int nbytes_write = 0;
    char cmd[20];
    switch (direction)
    {
        case LX200_NORTH:
            sprintf(cmd, ":Mn%04d#", duration_msec);
            break;
        case LX200_SOUTH:
            sprintf(cmd, ":Ms%04d#", duration_msec);
	    break;
        case LX200_EAST:
            sprintf(cmd, ":Me%04d#", duration_msec);
	    break;
        case LX200_WEST:
            sprintf(cmd, ":Mw%04d#", duration_msec);
	    break;
        default:
            return 1;
    }

    DEBUGFDEVICE(lx200ap_name, AP_DBG_SCOPE, "CMD <%s>", cmd);

    tty_write_string(fd, cmd, &nbytes_write);

    tcflush(fd, TCIFLUSH);
    return 0;
}

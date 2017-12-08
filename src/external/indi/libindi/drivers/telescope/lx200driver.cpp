#if 0
LX200 Driver
Copyright (C) 2003 Jasem Mutlaq (mutlaqja@ikarustech.com)

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

#include "lx200driver.h"

#include "indicom.h"
#include "indilogger.h"

#include <cstring>
#include <unistd.h>

#ifndef _WIN32
#include <termios.h>
#endif

#define LX200_TIMEOUT 5 /* FD timeout in seconds */
#define RB_MAX_LEN    64

int controller_format;
char lx200Name[MAXINDIDEVICE];
unsigned int DBG_SCOPE;

void setLX200Debug(const char *deviceName, unsigned int debug_level)
{
    strncpy(lx200Name, deviceName, MAXINDIDEVICE);
    DBG_SCOPE = debug_level;
}

/**************************************************************************
 Diagnostics
 **************************************************************************/
char ACK(int fd);
int check_lx200_connection(int fd);

/**************************************************************************
 Get Commands: store data in the supplied buffer. Return 0 on success or -1 on failure
 **************************************************************************/

/* Get Double from Sexagisemal */
int getCommandSexa(int fd, double *value, const char *cmd);
/* Get String */
int getCommandString(int fd, char *data, const char *cmd);
/* Get Int */
int getCommandInt(int fd, int *value, const char *cmd);
/* Get tracking frequency */
int getTrackFreq(int fd, double *value);
/* Get site Latitude */
int getSiteLatitude(int fd, int *dd, int *mm);
/* Get site Longitude */
int getSiteLongitude(int fd, int *ddd, int *mm);
/* Get Calender data */
int getCalendarDate(int fd, char *date);
/* Get site Name */
int getSiteName(int fd, char *siteName, int siteNum);
/* Get Home Search Status */
int getHomeSearchStatus(int fd, int *status);
/* Get OTA Temperature */
int getOTATemp(int fd, double *value);
/* Get time format: 12 or 24 */
int getTimeFormat(int fd, int *format);

/**************************************************************************
 Set Commands
 **************************************************************************/

/* Set Int */
int setCommandInt(int fd, int data, const char *cmd);
/* Set Sexagesimal */
int setCommandXYZ(int fd, int x, int y, int z, const char *cmd);
/* Common routine for Set commands */
int setStandardProcedure(int fd, const char *writeData);
/* Set Slew Mode */
int setSlewMode(int fd, int slewMode);
/* Set Alignment mode */
int setAlignmentMode(int fd, unsigned int alignMode);
/* Set Object RA */
int setObjectRA(int fd, double ra);
/* set Object DEC */
int setObjectDEC(int fd, double dec);
/* Set Calendar date */
int setCalenderDate(int fd, int dd, int mm, int yy);
/* Set UTC offset */
int setUTCOffset(int fd, double hours);
/* Set Track Freq */
int setTrackFreq(int fd, double trackF);
/* Set current site longitude */
int setSiteLongitude(int fd, double Long);
/* Set current site latitude */
int setSiteLatitude(int fd, double Lat);
/* Set Object Azimuth */
int setObjAz(int fd, double az);
/* Set Object Altitude */
int setObjAlt(int fd, double alt);
/* Set site name */
int setSiteName(int fd, char *siteName, int siteNum);
/* Set maximum slew rate */
int setMaxSlewRate(int fd, int slewRate);
/* Set focuser motion */
int setFocuserMotion(int fd, int motionType);
/* Set focuser speed mode */
int setFocuserSpeedMode(int fd, int speedMode);
/* Set minimum elevation limit */
int setMinElevationLimit(int fd, int min);
/* Set maximum elevation limit */
int setMaxElevationLimit(int fd, int max);

/**************************************************************************
 Motion Commands
 **************************************************************************/
/* Slew to the selected coordinates */
int Slew(int fd);
/* Synchronize to the selected coordinates and return the matching object if any */
int Sync(int fd, char *matchedObject);
/* Abort slew in all axes */
int abortSlew(int fd);
/* Move into one direction, two valid directions can be stacked */
int MoveTo(int fd, int direction);
/* Half movement in a particular direction */
int HaltMovement(int fd, int direction);
/* Select the tracking mode */
int selectTrackingMode(int fd, int trackMode);
/* Send Pulse-Guide command (timed guide move), two valid directions can be stacked */
int SendPulseCmd(int fd, int direction, int duration_msec);

/**************************************************************************
 Other Commands
 **************************************************************************/
/* Determines LX200 RA/DEC format, tries to set to long if found short */
int checkLX200Format(int fd);
/* return the controller_format enum value */
int getLX200Format();
/* Select a site from the LX200 controller */
int selectSite(int fd, int siteNum);
/* Select a catalog object */
int selectCatalogObject(int fd, int catalog, int NNNN);
/* Select a sub catalog */
int selectSubCatalog(int fd, int catalog, int subCatalog);

int check_lx200_connection(int in_fd)
{
    int i       = 0;
    char ack[1] = { (char)0x06 };
    char MountAlign[64];
    int nbytes_read = 0;

    DEBUGDEVICE(lx200Name, INDI::Logger::DBG_DEBUG, "Testing telescope connection using ACK...");

    if (in_fd <= 0)
        return -1;

    for (i = 0; i < 2; i++)
    {
        if (write(in_fd, ack, 1) < 0)
            return -1;
        tty_read(in_fd, MountAlign, 1, LX200_TIMEOUT, &nbytes_read);
        if (nbytes_read == 1)
        {
            DEBUGDEVICE(lx200Name, INDI::Logger::DBG_DEBUG, "Testing successful!");
            return 0;
        }
        usleep(50000);
    }

    DEBUGDEVICE(lx200Name, INDI::Logger::DBG_DEBUG, "Failure. Telescope is not responding to ACK!");
    return -1;
}

/**********************************************************************
* GET
**********************************************************************/

char ACK(int fd)
{
    DEBUGFDEVICE(lx200Name, DBG_SCOPE, "<%s>", __FUNCTION__);

    char ack[1] = { (char)0x06 };
    char MountAlign[2];
    int nbytes_write = 0, nbytes_read = 0, error_type;

    DEBUGFDEVICE(lx200Name, DBG_SCOPE, "CMD <%#02X>", ack[0]);

    nbytes_write = write(fd, ack, 1);

    if (nbytes_write < 0)
        return -1;

    error_type = tty_read(fd, MountAlign, 1, LX200_TIMEOUT, &nbytes_read);

    DEBUGFDEVICE(lx200Name, DBG_SCOPE, "RES <%c>", MountAlign[0]);

    if (nbytes_read == 1)
        return MountAlign[0];
    else
        return error_type;
}

int getCommandSexa(int fd, double *value, const char *cmd)
{
    char read_buffer[RB_MAX_LEN]={0};
    int error_type;
    int nbytes_write = 0, nbytes_read = 0;

    tcflush(fd, TCIFLUSH);

    DEBUGFDEVICE(lx200Name, DBG_SCOPE, "CMD <%s>", cmd);

    if ((error_type = tty_write_string(fd, cmd, &nbytes_write)) != TTY_OK)
        return error_type;

    error_type = tty_read_section(fd, read_buffer, '#', LX200_TIMEOUT, &nbytes_read);
    tcflush(fd, TCIFLUSH);
    if (error_type != TTY_OK)
        return error_type;

    read_buffer[nbytes_read - 1] = '\0';

    DEBUGFDEVICE(lx200Name, DBG_SCOPE, "RES <%s>", read_buffer);

    if (f_scansexa(read_buffer, value))
    {
        DEBUGDEVICE(lx200Name, DBG_SCOPE, "Unable to parse response");
        return -1;
    }

    DEBUGFDEVICE(lx200Name, DBG_SCOPE, "VAL [%g]", *value);

    tcflush(fd, TCIFLUSH);
    return 0;
}

int getCommandInt(int fd, int *value, const char *cmd)
{
    char read_buffer[RB_MAX_LEN]={0};
    float temp_number;
    int error_type;
    int nbytes_write = 0, nbytes_read = 0;

    tcflush(fd, TCIFLUSH);

    DEBUGFDEVICE(lx200Name, DBG_SCOPE, "CMD <%s>", cmd);

    if ((error_type = tty_write_string(fd, cmd, &nbytes_write)) != TTY_OK)
        return error_type;

    error_type = tty_read_section(fd, read_buffer, '#', LX200_TIMEOUT, &nbytes_read);
    tcflush(fd, TCIFLUSH);
    if (error_type != TTY_OK)
        return error_type;

    read_buffer[nbytes_read - 1] = '\0';

    DEBUGFDEVICE(lx200Name, DBG_SCOPE, "RES <%s>", read_buffer);

    /* Float */
    if (strchr(read_buffer, '.'))
    {
        if (sscanf(read_buffer, "%f", &temp_number) != 1)
            return -1;

        *value = (int)temp_number;
    }
    /* Int */
    else if (sscanf(read_buffer, "%d", value) != 1)
        return -1;

    DEBUGFDEVICE(lx200Name, DBG_SCOPE, "VAL [%d]", *value);

    return 0;
}

int getCommandString(int fd, char *data, const char *cmd)
{
    char *term;
    int error_type;
    int nbytes_write = 0, nbytes_read = 0;

    DEBUGFDEVICE(lx200Name, DBG_SCOPE, "CMD <%s>", cmd);

    if ((error_type = tty_write_string(fd, cmd, &nbytes_write)) != TTY_OK)
        return error_type;

    error_type = tty_read_section(fd, data, '#', LX200_TIMEOUT, &nbytes_read);
    tcflush(fd, TCIFLUSH);

    if (error_type != TTY_OK)
        return error_type;

    term = strchr(data, '#');
    if (term)
        *term = '\0';

    DEBUGFDEVICE(lx200Name, DBG_SCOPE, "RES <%s>", data);

    return 0;
}

int isSlewComplete(int fd)
{
    DEBUGFDEVICE(lx200Name, DBG_SCOPE, "<%s>", __FUNCTION__);
    char data[8] = { 0 };
    int error_type;
    int nbytes_write = 0, nbytes_read = 0;
    const char *cmd = ":D#";

    DEBUGFDEVICE(lx200Name, DBG_SCOPE, "CMD <%s>", cmd);

    if ((error_type = tty_write_string(fd, cmd, &nbytes_write)) != TTY_OK)
        return error_type;

    error_type = tty_read_section(fd, data, '#', LX200_TIMEOUT, &nbytes_read);
    tcflush(fd, TCIOFLUSH);

    if (error_type != TTY_OK)
        return error_type;

    DEBUGFDEVICE(lx200Name, DBG_SCOPE, "RES <%s>", data);

    if (data[0] == '#')
        return 1;
    else
        return 0;
}

int getCalendarDate(int fd, char *date)
{
    DEBUGFDEVICE(lx200Name, DBG_SCOPE, "<%s>", __FUNCTION__);
    int dd, mm, yy, YYYY;
    int error_type;
    int nbytes_read = 0;
    char mell_prefix[3]={0};
    int len = 0;

    if ((error_type = getCommandString(fd, date, ":GC#")))
        return error_type;
    len = strnlen(date, 32);
    if (len == 10)
    {
        /* 10Micron Ultra Precision mode calendar date format is YYYY-MM-DD */
        nbytes_read = sscanf(date, "%4d-%2d-%2d", &YYYY, &mm, &dd);
        if (nbytes_read < 3)
            return -1;
        /* We're done, date is already in ISO format */
    }
    else
    {
        /* Meade format is MM/DD/YY */
        nbytes_read = sscanf(date, "%d%*c%d%*c%d", &mm, &dd, &yy);
        if (nbytes_read < 3)
            return -1;
        /* We consider years 50 or more to be in the last century, anything less in the 21st century.*/
        if (yy > 50)
            strncpy(mell_prefix, "19", 3);
        else
            strncpy(mell_prefix, "20", 3);
        /* We need to have it in YYYY-MM-DD ISO format */
        snprintf(date, 32, "%s%02d-%02d-%02d", mell_prefix, yy, mm, dd);
    }
    return (0);
}

int getTimeFormat(int fd, int *format)
{
    DEBUGFDEVICE(lx200Name, DBG_SCOPE, "<%s>", __FUNCTION__);
    char read_buffer[RB_MAX_LEN]={0};
    char formatString[6] = {0};
    int error_type;
    int nbytes_write = 0, nbytes_read = 0;
    int tMode;

    DEBUGFDEVICE(lx200Name, DBG_SCOPE, "CMD <%s>", ":Gc#");

    if ((error_type = tty_write_string(fd, ":Gc#", &nbytes_write)) != TTY_OK)
        return error_type;

    if ((error_type = tty_read_section(fd, read_buffer, '#', LX200_TIMEOUT, &nbytes_read)) != TTY_OK)
        return error_type;

    tcflush(fd, TCIFLUSH);

    if (nbytes_read < 1)
        return error_type;

    read_buffer[nbytes_read - 1] = '\0';

    DEBUGFDEVICE(lx200Name, DBG_SCOPE, "RES <%s>", read_buffer);

    // The Losmandy Gemini puts () around it's time format
    if (strstr(read_buffer, "("))
        strcpy(formatString, "(%d)");
    else
        strcpy(formatString, "%d");

    nbytes_read = sscanf(read_buffer, formatString, &tMode);

    if (nbytes_read < 1)
        return -1;
    else
        *format = tMode;

    return 0;
}

int getSiteName(int fd, char *siteName, int siteNum)
{
    DEBUGFDEVICE(lx200Name, DBG_SCOPE, "<%s>", __FUNCTION__);
    char *term;
    int error_type;
    int nbytes_write = 0, nbytes_read = 0;

    switch (siteNum)
    {
        case 1:
            DEBUGFDEVICE(lx200Name, DBG_SCOPE, "CMD <%s>", ":GM#");
            if ((error_type = tty_write_string(fd, ":GM#", &nbytes_write)) != TTY_OK)
                return error_type;
            break;
        case 2:
            DEBUGFDEVICE(lx200Name, DBG_SCOPE, "CMD <%s>", ":GN#");
            if ((error_type = tty_write_string(fd, ":GN#", &nbytes_write)) != TTY_OK)
                return error_type;
            break;
        case 3:
            DEBUGFDEVICE(lx200Name, DBG_SCOPE, "CMD <%s>", ":GO#");
            if ((error_type = tty_write_string(fd, ":GO#", &nbytes_write)) != TTY_OK)
                return error_type;
            break;
        case 4:
            DEBUGFDEVICE(lx200Name, DBG_SCOPE, "CMD <%s>", ":GP#");
            if ((error_type = tty_write_string(fd, ":GP#", &nbytes_write)) != TTY_OK)
                return error_type;
            break;
        default:
            return -1;
    }

    error_type = tty_read_section(fd, siteName, '#', LX200_TIMEOUT, &nbytes_read);
    tcflush(fd, TCIFLUSH);

    if (nbytes_read < 1)
        return error_type;

    siteName[nbytes_read - 1] = '\0';

    DEBUGFDEVICE(lx200Name, DBG_SCOPE, "RES <%s>", siteName);

    term = strchr(siteName, ' ');
    if (term)
        *term = '\0';

    term = strchr(siteName, '<');
    if (term)
        strcpy(siteName, "unused site");

    DEBUGFDEVICE(lx200Name, INDI::Logger::DBG_DEBUG, "Site Name <%s>", siteName);

    return 0;
}

int getSiteLatitude(int fd, int *dd, int *mm)
{
    DEBUGFDEVICE(lx200Name, DBG_SCOPE, "<%s>", __FUNCTION__);
    char read_buffer[RB_MAX_LEN]={0};
    int error_type;
    int nbytes_write = 0, nbytes_read = 0;

    DEBUGFDEVICE(lx200Name, DBG_SCOPE, "CMD <%s>", ":Gt#");

    tcflush(fd, TCIFLUSH);

    if ((error_type = tty_write_string(fd, ":Gt#", &nbytes_write)) != TTY_OK)
        return error_type;

    error_type = tty_read_section(fd, read_buffer, '#', LX200_TIMEOUT, &nbytes_read);

    tcflush(fd, TCIFLUSH);

    if (nbytes_read < 1)
        return error_type;

    read_buffer[nbytes_read - 1] = '\0';

    DEBUGFDEVICE(lx200Name, DBG_SCOPE, "RES <%s>", read_buffer);

    if (sscanf(read_buffer, "%d%*c%d", dd, mm) < 2)
    {
        DEBUGDEVICE(lx200Name, DBG_SCOPE, "Unable to parse response");
        return -1;
    }

    DEBUGFDEVICE(lx200Name, DBG_SCOPE, "VAL [%d,%d]", *dd, *mm);

    return 0;
}

int getSiteLongitude(int fd, int *ddd, int *mm)
{
    DEBUGFDEVICE(lx200Name, DBG_SCOPE, "<%s>", __FUNCTION__);
    char read_buffer[RB_MAX_LEN]={0};
    int error_type;
    int nbytes_write = 0, nbytes_read = 0;

    DEBUGFDEVICE(lx200Name, DBG_SCOPE, "CMD <%s>", ":Gg#");

    if ((error_type = tty_write_string(fd, ":Gg#", &nbytes_write)) != TTY_OK)
        return error_type;

    error_type = tty_read_section(fd, read_buffer, '#', LX200_TIMEOUT, &nbytes_read);

    tcflush(fd, TCIFLUSH);

    if (nbytes_read < 1)
        return error_type;

    read_buffer[nbytes_read - 1] = '\0';

    DEBUGFDEVICE(lx200Name, DBG_SCOPE, "RES <%s>", read_buffer);

    if (sscanf(read_buffer, "%d%*c%d", ddd, mm) < 2)
    {
        DEBUGDEVICE(lx200Name, DBG_SCOPE, "Unable to parse response");
        return -1;
    }

    DEBUGFDEVICE(lx200Name, DBG_SCOPE, "VAL [%d,%d]", *ddd, *mm);

    return 0;
}

int getTrackFreq(int fd, double *value)
{
    DEBUGFDEVICE(lx200Name, DBG_SCOPE, "<%s>", __FUNCTION__);
    float Freq;
    char read_buffer[RB_MAX_LEN]={0};
    int error_type;
    int nbytes_write = 0, nbytes_read = 0;

    DEBUGFDEVICE(lx200Name, DBG_SCOPE, "CMD <%s>", ":GT#");

    if ((error_type = tty_write_string(fd, ":GT#", &nbytes_write)) != TTY_OK)
        return error_type;

    error_type = tty_read_section(fd, read_buffer, '#', LX200_TIMEOUT, &nbytes_read);
    tcflush(fd, TCIFLUSH);

    if (nbytes_read < 1)
        return error_type;

    read_buffer[nbytes_read] = '\0';

    DEBUGFDEVICE(lx200Name, DBG_SCOPE, "RES <%s>", read_buffer);

    if (sscanf(read_buffer, "%f#", &Freq) < 1)
    {
        DEBUGDEVICE(lx200Name, DBG_SCOPE, "Unable to parse response");
        return -1;
    }

    *value = (double)Freq;

    DEBUGFDEVICE(lx200Name, DBG_SCOPE, "VAL [%g]", *value);

    return 0;
}

int getHomeSearchStatus(int fd, int *status)
{
    DEBUGFDEVICE(lx200Name, DBG_SCOPE, "<%s>", __FUNCTION__);
    char read_buffer[RB_MAX_LEN]={0};
    int error_type;
    int nbytes_write = 0, nbytes_read = 0;

    DEBUGFDEVICE(lx200Name, DBG_SCOPE, "CMD <%s>", ":h?#");

    if ((error_type = tty_write_string(fd, ":h?#", &nbytes_write)) != TTY_OK)
        return error_type;

    error_type = tty_read_section(fd, read_buffer, '#', LX200_TIMEOUT, &nbytes_read);
    tcflush(fd, TCIFLUSH);

    if (nbytes_read < 1)
        return error_type;

    read_buffer[1] = '\0';

    DEBUGFDEVICE(lx200Name, DBG_SCOPE, "RES <%s>", read_buffer);

    if (read_buffer[0] == '0')
        *status = 0;
    else if (read_buffer[0] == '1')
        *status = 1;
    else if (read_buffer[0] == '2')
        *status = 1;

    DEBUGFDEVICE(lx200Name, DBG_SCOPE, "VAL [%d]", *status);

    return 0;
}

int getOTATemp(int fd, double *value)
{
    DEBUGFDEVICE(lx200Name, DBG_SCOPE, "<%s>", __FUNCTION__);
    char read_buffer[RB_MAX_LEN]={0};
    int error_type;
    int nbytes_write = 0, nbytes_read = 0;
    float temp;

    DEBUGFDEVICE(lx200Name, DBG_SCOPE, "CMD <%s>", ":fT#");

    if ((error_type = tty_write_string(fd, ":fT#", &nbytes_write)) != TTY_OK)
        return error_type;

    error_type = tty_read_section(fd, read_buffer, '#', LX200_TIMEOUT, &nbytes_read);

    if (nbytes_read < 1)
        return error_type;

    read_buffer[nbytes_read - 1] = '\0';

    DEBUGFDEVICE(lx200Name, DBG_SCOPE, "RES <%s>", read_buffer);

    if (sscanf(read_buffer, "%f", &temp) < 1)
    {
        DEBUGDEVICE(lx200Name, DBG_SCOPE, "Unable to parse response");
        return -1;
    }

    *value = (double)temp;

    DEBUGFDEVICE(lx200Name, DBG_SCOPE, "VAL [%g]", *value);

    return 0;
}

/**********************************************************************
* SET
**********************************************************************/

int setStandardProcedure(int fd, const char *data)
{
    char bool_return[2];
    int error_type;
    int nbytes_write = 0, nbytes_read = 0;

    DEBUGFDEVICE(lx200Name, DBG_SCOPE, "CMD <%s>", data);

    tcflush(fd, TCIFLUSH);

    if ((error_type = tty_write_string(fd, data, &nbytes_write)) != TTY_OK)
        return error_type;

    error_type = tty_read(fd, bool_return, 1, LX200_TIMEOUT, &nbytes_read);

    tcflush(fd, TCIFLUSH);

    if (nbytes_read < 1)
        return error_type;

    if (bool_return[0] == '0')
    {
        DEBUGFDEVICE(lx200Name, DBG_SCOPE, "CMD <%s> failed.", data);
        return -1;
    }

    DEBUGFDEVICE(lx200Name, DBG_SCOPE, "CMD <%s> successful.", data);

    return 0;
}

int setCommandInt(int fd, int data, const char *cmd)
{
    char read_buffer[RB_MAX_LEN]={0};
    int error_type;
    int nbytes_write = 0;

    snprintf(read_buffer, sizeof(read_buffer), "%s%d#", cmd, data);

    DEBUGFDEVICE(lx200Name, DBG_SCOPE, "CMD <%s>", read_buffer);

    tcflush(fd, TCIFLUSH);

    if ((error_type = tty_write_string(fd, read_buffer, &nbytes_write)) != TTY_OK)
    {
        DEBUGFDEVICE(lx200Name, DBG_SCOPE, "CMD <%s> failed.", read_buffer);
        return error_type;
    }

    tcflush(fd, TCIFLUSH);

    DEBUGFDEVICE(lx200Name, DBG_SCOPE, "CMD <%s> successful.", read_buffer);

    return 0;
}

int setMinElevationLimit(int fd, int min)
{
    DEBUGFDEVICE(lx200Name, DBG_SCOPE, "<%s>", __FUNCTION__);
    char read_buffer[RB_MAX_LEN]={0};

    snprintf(read_buffer, sizeof(read_buffer), ":Sh%02d#", min);

    return (setStandardProcedure(fd, read_buffer));
}

int setMaxElevationLimit(int fd, int max)
{
    DEBUGFDEVICE(lx200Name, DBG_SCOPE, "<%s>", __FUNCTION__);

    char read_buffer[RB_MAX_LEN]={0};

    snprintf(read_buffer, sizeof(read_buffer), ":So%02d*#", max);

    return (setStandardProcedure(fd, read_buffer));
}

int setMaxSlewRate(int fd, int slewRate)
{
    DEBUGFDEVICE(lx200Name, DBG_SCOPE, "<%s>", __FUNCTION__);

    char read_buffer[RB_MAX_LEN]={0};

    if (slewRate < 2 || slewRate > 8)
        return -1;

    snprintf(read_buffer, sizeof(read_buffer), ":Sw%d#", slewRate);

    return (setStandardProcedure(fd, read_buffer));
}

int setObjectRA(int fd, double ra)
{
    DEBUGFDEVICE(lx200Name, DBG_SCOPE, "<%s>", __FUNCTION__);

    int h, m, s;
    char read_buffer[22];

    switch (controller_format)
    {
        case LX200_LONG_FORMAT:
            getSexComponents(ra, &h, &m, &s);
            snprintf(read_buffer, sizeof(read_buffer), ":Sr %02d:%02d:%02d#", h, m, s);
            break;
        case LX200_LONGER_FORMAT:
            double d_s;
            getSexComponentsIID(ra, &h, &m, &d_s);
            snprintf(read_buffer, sizeof(read_buffer), ":Sr %02d:%02d:%05.02f#", h, m, d_s);
            break;
        case LX200_SHORT_FORMAT:
            int frac_m;
            getSexComponents(ra, &h, &m, &s);
            frac_m = (s / 60.0) * 10.;
            snprintf(read_buffer, sizeof(read_buffer), ":Sr %02d:%02d.%01d#", h, m, frac_m);
            break;
        default:
            DEBUGFDEVICE(lx200Name, DBG_SCOPE, "Unknown controller_format <%d>", controller_format);
            return -1;
            break;
    }

    return (setStandardProcedure(fd, read_buffer));
}

int setObjectDEC(int fd, double dec)
{
    DEBUGFDEVICE(lx200Name, DBG_SCOPE, "<%s>", __FUNCTION__);

    int d, m, s;
    char read_buffer[22];

    switch (controller_format)
    {
        case LX200_LONG_FORMAT:
            getSexComponents(dec, &d, &m, &s);
            /* case with negative zero */
            if (!d && dec < 0)
                snprintf(read_buffer, sizeof(read_buffer), ":Sd -%02d:%02d:%02d#", d, m, s);
            else
                snprintf(read_buffer, sizeof(read_buffer), ":Sd %+03d:%02d:%02d#", d, m, s);
            break;
        case LX200_LONGER_FORMAT:
            double d_s;
            getSexComponentsIID(dec, &d, &m, &d_s);
            /* case with negative zero */
            if (!d && dec < 0)
                snprintf(read_buffer, sizeof(read_buffer), ":Sd -%02d:%02d:%05.02f#", d, m, d_s);
            else
                snprintf(read_buffer, sizeof(read_buffer), ":Sd %+03d:%02d:%05.02f#", d, m, d_s);
            break;
        case LX200_SHORT_FORMAT:
            getSexComponents(dec, &d, &m, &s);
            /* case with negative zero */
            if (!d && dec < 0)
                snprintf(read_buffer, sizeof(read_buffer), ":Sd -%02d*%02d#", d, m);
            else
                snprintf(read_buffer, sizeof(read_buffer), ":Sd %+03d*%02d#", d, m);
            break;
        default:
            DEBUGFDEVICE(lx200Name, DBG_SCOPE, "Unknown controller_format <%d>", controller_format);
            return -1;
            break;
    }

    return (setStandardProcedure(fd, read_buffer));
}

int setCommandXYZ(int fd, int x, int y, int z, const char *cmd)
{
    char read_buffer[RB_MAX_LEN]={0};

    snprintf(read_buffer, sizeof(read_buffer), "%s %02d:%02d:%02d#", cmd, x, y, z);

    return (setStandardProcedure(fd, read_buffer));
}

int setAlignmentMode(int fd, unsigned int alignMode)
{
    int error_type;
    int nbytes_write = 0;

    switch (alignMode)
    {
        case LX200_ALIGN_POLAR:
            DEBUGFDEVICE(lx200Name, DBG_SCOPE, "CMD <%s>", ":AP#");
            if ((error_type = tty_write_string(fd, ":AP#", &nbytes_write)) != TTY_OK)
                return error_type;
            break;
        case LX200_ALIGN_ALTAZ:
            DEBUGFDEVICE(lx200Name, DBG_SCOPE, "CMD <%s>", ":AA#");
            if ((error_type = tty_write_string(fd, ":AA#", &nbytes_write)) != TTY_OK)
                return error_type;
            break;
        case LX200_ALIGN_LAND:
            DEBUGFDEVICE(lx200Name, DBG_SCOPE, "CMD <%s>", ":AL#");
            if ((error_type = tty_write_string(fd, ":AL#", &nbytes_write)) != TTY_OK)
                return error_type;
            break;
    }

    tcflush(fd, TCIFLUSH);
    return 0;
}

int setCalenderDate(int fd, int dd, int mm, int yy)
{
    DEBUGFDEVICE(lx200Name, DBG_SCOPE, "<%s>", __FUNCTION__);
    char read_buffer[32];
    char dummy_buffer[32];
    int error_type;
    int nbytes_write = 0, nbytes_read = 0;
    yy = yy % 100;

    snprintf(read_buffer, sizeof(read_buffer), ":SC %02d/%02d/%02d#", mm, dd, yy);

    DEBUGFDEVICE(lx200Name, DBG_SCOPE, "CMD <%s>", read_buffer);

    tcflush(fd, TCIFLUSH);

    if ((error_type = tty_write_string(fd, read_buffer, &nbytes_write)) != TTY_OK)
        return error_type;

    error_type = tty_read_section(fd, read_buffer, '#', LX200_TIMEOUT, &nbytes_read);
    // Read the next section whih has 24 blanks and then a #
    // Can't just use the tcflush to clear the stream because it doesn't seem to work correctly on sockets
    tty_read_section(fd, dummy_buffer, '#', LX200_TIMEOUT, &nbytes_read);

    tcflush(fd, TCIFLUSH);

    if (nbytes_read < 1)
    {
        DEBUGDEVICE(lx200Name, DBG_SCOPE, "Unable to parse response");
        return error_type;
    }

    read_buffer[1] = '\0';

    DEBUGFDEVICE(lx200Name, DBG_SCOPE, "RES <%s>", read_buffer);

    if (read_buffer[0] == '0')
        return -1;

    /* Sleep 10ms before flushing. This solves some issues with LX200 compatible devices. */
    usleep(10000);
    tcflush(fd, TCIFLUSH);

    return 0;
}

int setUTCOffset(int fd, double hours)
{
    DEBUGFDEVICE(lx200Name, DBG_SCOPE, "<%s>", __FUNCTION__);
    char read_buffer[RB_MAX_LEN]={0};

    snprintf(read_buffer, sizeof(read_buffer), ":SG %+03d#", (int)hours);

    return (setStandardProcedure(fd, read_buffer));
}

int setSiteLongitude(int fd, double Long)
{
    DEBUGFDEVICE(lx200Name, DBG_SCOPE, "<%s>", __FUNCTION__);
    int d, m, s;
    char read_buffer[32];

    getSexComponents(Long, &d, &m, &s);

    snprintf(read_buffer, sizeof(read_buffer), ":Sg%03d:%02d#", d, m);

    return (setStandardProcedure(fd, read_buffer));
}

int setSiteLatitude(int fd, double Lat)
{
    DEBUGFDEVICE(lx200Name, DBG_SCOPE, "<%s>", __FUNCTION__);
    int d, m, s;
    char read_buffer[32];

    getSexComponents(Lat, &d, &m, &s);

    snprintf(read_buffer, sizeof(read_buffer), ":St%+03d:%02d:%02d#", d, m, s);

    return (setStandardProcedure(fd, read_buffer));
}

int setObjAz(int fd, double az)
{
    DEBUGFDEVICE(lx200Name, DBG_SCOPE, "<%s>", __FUNCTION__);
    int d, m, s;
    char read_buffer[RB_MAX_LEN]={0};

    getSexComponents(az, &d, &m, &s);

    snprintf(read_buffer, sizeof(read_buffer), ":Sz%03d:%02d#", d, m);

    return (setStandardProcedure(fd, read_buffer));
}

int setObjAlt(int fd, double alt)
{
    DEBUGFDEVICE(lx200Name, DBG_SCOPE, "<%s>", __FUNCTION__);
    int d, m, s;
    char read_buffer[RB_MAX_LEN]={0};

    getSexComponents(alt, &d, &m, &s);

    snprintf(read_buffer, sizeof(read_buffer), ":Sa%+02d*%02d#", d, m);

    return (setStandardProcedure(fd, read_buffer));
}

int setSiteName(int fd, char *siteName, int siteNum)
{
    DEBUGFDEVICE(lx200Name, DBG_SCOPE, "<%s>", __FUNCTION__);
    char read_buffer[RB_MAX_LEN]={0};

    switch (siteNum)
    {
        case 1:
            snprintf(read_buffer, sizeof(read_buffer), ":SM %s#", siteName);
            break;
        case 2:
            snprintf(read_buffer, sizeof(read_buffer), ":SN %s#", siteName);
            break;
        case 3:
            snprintf(read_buffer, sizeof(read_buffer), ":SO %s#", siteName);
            break;
        case 4:
            snprintf(read_buffer, sizeof(read_buffer), ":SP %s#", siteName);
            break;
        default:
            return -1;
    }

    return (setStandardProcedure(fd, read_buffer));
}

int setSlewMode(int fd, int slewMode)
{
    DEBUGFDEVICE(lx200Name, DBG_SCOPE, "<%s>", __FUNCTION__);
    int error_type;
    int nbytes_write = 0;

    switch (slewMode)
    {
        case LX200_SLEW_MAX:
            DEBUGFDEVICE(lx200Name, DBG_SCOPE, "CMD <%s>", ":RS#");
            if ((error_type = tty_write_string(fd, ":RS#", &nbytes_write)) != TTY_OK)
                return error_type;
            break;
        case LX200_SLEW_FIND:
            DEBUGFDEVICE(lx200Name, DBG_SCOPE, "CMD <%s>", ":RM#");
            if ((error_type = tty_write_string(fd, ":RM#", &nbytes_write)) != TTY_OK)
                return error_type;
            break;
        case LX200_SLEW_CENTER:
            DEBUGFDEVICE(lx200Name, DBG_SCOPE, "CMD <%s>", ":RC#");
            if ((error_type = tty_write_string(fd, ":RC#", &nbytes_write)) != TTY_OK)
                return error_type;
            break;
        case LX200_SLEW_GUIDE:
            DEBUGFDEVICE(lx200Name, DBG_SCOPE, "CMD <%s>", ":RG#");
            if ((error_type = tty_write_string(fd, ":RG#", &nbytes_write)) != TTY_OK)
                return error_type;
            break;
        default:
            break;
    }

    tcflush(fd, TCIFLUSH);
    return 0;
}

int setFocuserMotion(int fd, int motionType)
{
    DEBUGFDEVICE(lx200Name, DBG_SCOPE, "<%s>", __FUNCTION__);
    int error_type;
    int nbytes_write = 0;

    switch (motionType)
    {
        case LX200_FOCUSIN:
            DEBUGFDEVICE(lx200Name, DBG_SCOPE, "CMD <%s>", ":F+#");
            if ((error_type = tty_write_string(fd, ":F+#", &nbytes_write)) != TTY_OK)
                return error_type;
            break;
        case LX200_FOCUSOUT:
            DEBUGFDEVICE(lx200Name, DBG_SCOPE, "CMD <%s>", ":F-#");
            if ((error_type = tty_write_string(fd, ":F-#", &nbytes_write)) != TTY_OK)
                return error_type;
            break;
    }

    tcflush(fd, TCIFLUSH);
    return 0;
}

int setFocuserSpeedMode(int fd, int speedMode)
{
    DEBUGFDEVICE(lx200Name, DBG_SCOPE, "<%s>", __FUNCTION__);
    int error_type;
    int nbytes_write = 0;

    switch (speedMode)
    {
        case LX200_HALTFOCUS:
            DEBUGFDEVICE(lx200Name, DBG_SCOPE, "CMD <%s>", ":FQ#");
            if ((error_type = tty_write_string(fd, ":FQ#", &nbytes_write)) != TTY_OK)
                return error_type;
            break;
        case LX200_FOCUSSLOW:
            DEBUGFDEVICE(lx200Name, DBG_SCOPE, "CMD <%s>", ":FS#");
            if ((error_type = tty_write_string(fd, ":FS#", &nbytes_write)) != TTY_OK)
                return error_type;
            break;
        case LX200_FOCUSFAST:
            DEBUGFDEVICE(lx200Name, DBG_SCOPE, "CMD <%s>", ":FF#");
            if ((error_type = tty_write_string(fd, ":FF#", &nbytes_write)) != TTY_OK)
                return error_type;
            break;
    }

    tcflush(fd, TCIFLUSH);
    return 0;
}

int setGPSFocuserSpeed(int fd, int speed)
{
    DEBUGFDEVICE(lx200Name, DBG_SCOPE, "<%s>", __FUNCTION__);
    char speed_str[8];
    int error_type;
    int nbytes_write = 0;

    if (speed == 0)
    {
        DEBUGFDEVICE(lx200Name, DBG_SCOPE, "CMD <%s>", ":FQ#");
        if ((error_type = tty_write_string(fd, ":FQ#", &nbytes_write)) != TTY_OK)
            return error_type;

        tcflush(fd, TCIFLUSH);
        return 0;
    }

    snprintf(speed_str, 8, ":F%d#", speed);

    DEBUGFDEVICE(lx200Name, DBG_SCOPE, "CMD <%s>", speed_str);

    if ((error_type = tty_write_string(fd, speed_str, &nbytes_write)) != TTY_OK)
        return error_type;

    tcflush(fd, TCIFLUSH);
    return 0;
}

int setTrackFreq(int fd, double trackF)
{
    DEBUGFDEVICE(lx200Name, DBG_SCOPE, "<%s>", __FUNCTION__);
    char read_buffer[RB_MAX_LEN]={0};

    snprintf(read_buffer, sizeof(read_buffer), ":ST %04.1f#", trackF);

    return (setStandardProcedure(fd, read_buffer));
}

/**********************************************************************
* Misc
*********************************************************************/

int Slew(int fd)
{
    DEBUGFDEVICE(lx200Name, DBG_SCOPE, "<%s>", __FUNCTION__);
    char slewNum[2];
    int error_type;
    int nbytes_write = 0, nbytes_read = 0;

    DEBUGFDEVICE(lx200Name, DBG_SCOPE, "CMD <%s>", ":MS#");

    if ((error_type = tty_write_string(fd, ":MS#", &nbytes_write)) != TTY_OK)
        return error_type;

    error_type = tty_read(fd, slewNum, 1, LX200_TIMEOUT, &nbytes_read);

    if (nbytes_read < 1)
    {
        DEBUGFDEVICE(lx200Name, DBG_SCOPE, "RES ERROR <%d>", error_type);
        return error_type;
    }

    /* We don't need to read the string message, just return corresponding error code */
    tcflush(fd, TCIFLUSH);

    DEBUGFDEVICE(lx200Name, DBG_SCOPE, "RES <%c>", slewNum[0]);

    if (slewNum[0] == '0')
        return 0;
    else if (slewNum[0] == '1')
        return 1;
    else
        return 2;
}

int MoveTo(int fd, int direction)
{
    DEBUGFDEVICE(lx200Name, DBG_SCOPE, "<%s>", __FUNCTION__);
    int nbytes_write = 0;

    switch (direction)
    {
        case LX200_NORTH:
            DEBUGFDEVICE(lx200Name, DBG_SCOPE, "CMD <%s>", ":Mn#");
            tty_write_string(fd, ":Mn#", &nbytes_write);
            break;
        case LX200_WEST:
            DEBUGFDEVICE(lx200Name, DBG_SCOPE, "CMD <%s>", ":Mw#");
            tty_write_string(fd, ":Mw#", &nbytes_write);
            break;
        case LX200_EAST:
            DEBUGFDEVICE(lx200Name, DBG_SCOPE, "CMD <%s>", ":Me#");
            tty_write_string(fd, ":Me#", &nbytes_write);
            break;
        case LX200_SOUTH:
            DEBUGFDEVICE(lx200Name, DBG_SCOPE, "CMD <%s>", ":Ms#");
            tty_write_string(fd, ":Ms#", &nbytes_write);
            break;
        default:
            break;
    }

    tcflush(fd, TCIFLUSH);
    return 0;
}

int SendPulseCmd(int fd, int direction, int duration_msec)
{
    DEBUGFDEVICE(lx200Name, DBG_SCOPE, "<%s>", __FUNCTION__);
    int nbytes_write = 0;
    char cmd[20];
    switch (direction)
    {
        case LX200_NORTH:
            sprintf(cmd, ":Mgn%04d#", duration_msec);
            break;
        case LX200_SOUTH:
            sprintf(cmd, ":Mgs%04d#", duration_msec);
            break;
        case LX200_EAST:
            sprintf(cmd, ":Mge%04d#", duration_msec);
            break;
        case LX200_WEST:
            sprintf(cmd, ":Mgw%04d#", duration_msec);
            break;
        default:
            return 1;
    }

    DEBUGFDEVICE(lx200Name, DBG_SCOPE, "CMD <%s>", cmd);

    tty_write_string(fd, cmd, &nbytes_write);

    tcflush(fd, TCIFLUSH);
    return 0;
}

int HaltMovement(int fd, int direction)
{
    DEBUGFDEVICE(lx200Name, DBG_SCOPE, "<%s>", __FUNCTION__);
    int error_type;
    int nbytes_write = 0;

    switch (direction)
    {
        case LX200_NORTH:
            DEBUGFDEVICE(lx200Name, DBG_SCOPE, "CMD <%s>", ":Qn#");
            if ((error_type = tty_write_string(fd, ":Qn#", &nbytes_write)) != TTY_OK)
                return error_type;
            break;
        case LX200_WEST:
            DEBUGFDEVICE(lx200Name, DBG_SCOPE, "CMD <%s>", ":Qw#");
            if ((error_type = tty_write_string(fd, ":Qw#", &nbytes_write)) != TTY_OK)
                return error_type;
            break;
        case LX200_EAST:
            DEBUGFDEVICE(lx200Name, DBG_SCOPE, "CMD <%s>", ":Qe#");
            if ((error_type = tty_write_string(fd, ":Qe#", &nbytes_write)) != TTY_OK)
                return error_type;
            break;
        case LX200_SOUTH:
            DEBUGFDEVICE(lx200Name, DBG_SCOPE, "CMD <%s>", ":Qs#");
            if ((error_type = tty_write_string(fd, ":Qs#", &nbytes_write)) != TTY_OK)
                return error_type;
            break;
        case LX200_ALL:
            DEBUGFDEVICE(lx200Name, DBG_SCOPE, "CMD <%s>", ":Q#");
            if ((error_type = tty_write_string(fd, ":Q#", &nbytes_write)) != TTY_OK)
                return error_type;
            break;
        default:
            return -1;
            break;
    }

    tcflush(fd, TCIFLUSH);
    return 0;
}

int abortSlew(int fd)
{
    DEBUGFDEVICE(lx200Name, DBG_SCOPE, "<%s>", __FUNCTION__);
    int error_type;
    int nbytes_write = 0;

    if ((error_type = tty_write_string(fd, ":Q#", &nbytes_write)) != TTY_OK)
        return error_type;

    tcflush(fd, TCIFLUSH);
    return 0;
}

int Sync(int fd, char *matchedObject)
{
    DEBUGFDEVICE(lx200Name, DBG_SCOPE, "<%s>", __FUNCTION__);
    int error_type;
    int nbytes_write = 0, nbytes_read = 0;

    DEBUGFDEVICE(lx200Name, DBG_SCOPE, "CMD <%s>", ":CM#");

    if ((error_type = tty_write_string(fd, ":CM#", &nbytes_write)) != TTY_OK)
        return error_type;

    error_type = tty_read_section(fd, matchedObject, '#', LX200_TIMEOUT, &nbytes_read);

    if (nbytes_read < 1)
        return error_type;

    matchedObject[nbytes_read - 1] = '\0';

    DEBUGFDEVICE(lx200Name, DBG_SCOPE, "RES <%s>", matchedObject);

    /* Sleep 10ms before flushing. This solves some issues with LX200 compatible devices. */
    usleep(10000);
    tcflush(fd, TCIFLUSH);

    return 0;
}

int selectSite(int fd, int siteNum)
{
    DEBUGFDEVICE(lx200Name, DBG_SCOPE, "<%s>", __FUNCTION__);
    int error_type;
    int nbytes_write = 0;

    switch (siteNum)
    {
        case 1:
            DEBUGFDEVICE(lx200Name, DBG_SCOPE, "CMD <%s>", ":W1#");
            if ((error_type = tty_write_string(fd, ":W0#", &nbytes_write)) != TTY_OK) //azwing index starts at 0 not 1
                return error_type;
            break;
        case 2:
            DEBUGFDEVICE(lx200Name, DBG_SCOPE, "CMD <%s>", ":W2#");
            if ((error_type = tty_write_string(fd, ":W1#", &nbytes_write)) != TTY_OK) //azwing index starts at 0 not 1
                return error_type;
            break;
        case 3:
            DEBUGFDEVICE(lx200Name, DBG_SCOPE, "CMD <%s>", ":W3#");
            if ((error_type = tty_write_string(fd, ":W2#", &nbytes_write)) != TTY_OK) //azwing index starts at 0 not 1
                return error_type;
            break;
        case 4:
            DEBUGFDEVICE(lx200Name, DBG_SCOPE, "CMD <%s>", ":W4#");
            if ((error_type = tty_write_string(fd, ":W3#", &nbytes_write)) != TTY_OK) //azwing index starts at 0 not 1
                return error_type;
            break;
        default:
            return -1;
            break;
    }

    tcflush(fd, TCIFLUSH);
    return 0;
}

int selectCatalogObject(int fd, int catalog, int NNNN)
{
    DEBUGFDEVICE(lx200Name, DBG_SCOPE, "<%s>", __FUNCTION__);
    char read_buffer[RB_MAX_LEN]={0};
    int error_type;
    int nbytes_write = 0;

    switch (catalog)
    {
        case LX200_STAR_C:
            snprintf(read_buffer, sizeof(read_buffer), ":LS%d#", NNNN);
            break;
        case LX200_DEEPSKY_C:
            snprintf(read_buffer, sizeof(read_buffer), ":LC%d#", NNNN);
            break;
        case LX200_MESSIER_C:
            snprintf(read_buffer, sizeof(read_buffer), ":LM%d#", NNNN);
            break;
        default:
            return -1;
    }

    DEBUGFDEVICE(lx200Name, DBG_SCOPE, "CMD <%s>", read_buffer);

    if ((error_type = tty_write_string(fd, read_buffer, &nbytes_write)) != TTY_OK)
        return error_type;

    tcflush(fd, TCIFLUSH);
    return 0;
}

int selectSubCatalog(int fd, int catalog, int subCatalog)
{
    DEBUGFDEVICE(lx200Name, DBG_SCOPE, "<%s>", __FUNCTION__);
    char read_buffer[RB_MAX_LEN]={0};
    switch (catalog)
    {
        case LX200_STAR_C:
            snprintf(read_buffer, sizeof(read_buffer), ":LsD%d#", subCatalog);
            break;
        case LX200_DEEPSKY_C:
            snprintf(read_buffer, sizeof(read_buffer), ":LoD%d#", subCatalog);
            break;
        case LX200_MESSIER_C:
            return 1;
        default:
            return 0;
    }

    return (setStandardProcedure(fd, read_buffer));
}

int getLX200Format()
{
    return controller_format;
}

int checkLX200Format(int fd)
{
    char read_buffer[RB_MAX_LEN] = {0};
    controller_format = LX200_LONG_FORMAT;
    int error_type;
    int nbytes_write = 0, nbytes_read = 0;

    DEBUGFDEVICE(lx200Name, DBG_SCOPE, "CMD <%s>", ":GR#");

    tcflush(fd, TCIFLUSH);

    if ((error_type = tty_write_string(fd, ":GR#", &nbytes_write)) != TTY_OK)
        return error_type;

    error_type = tty_read_section(fd, read_buffer, '#', LX200_TIMEOUT, &nbytes_read);

    if (nbytes_read < 1)
    {
        DEBUGFDEVICE(lx200Name, DBG_SCOPE, "RES ERROR <%d>", error_type);
        return error_type;
    }

    read_buffer[nbytes_read - 1] = '\0';

    DEBUGFDEVICE(lx200Name, DBG_SCOPE, "RES <%s>", read_buffer);

    /* If it's short format, try to toggle to high precision format */
    if (read_buffer[5] == '.')
    {
        DEBUGDEVICE(lx200Name, DBG_SCOPE, "Detected low precision format, attempting to switch to high precision.");
        if ((error_type = tty_write_string(fd, ":U#", &nbytes_write)) != TTY_OK)
            return error_type;
    }
    else if (read_buffer[8] == '.')
    {
        controller_format = LX200_LONGER_FORMAT;
        DEBUGDEVICE(lx200Name, DBG_SCOPE, "Coordinate format is ultra high precision.");
        return 0;
    }
    else
    {
        controller_format = LX200_LONG_FORMAT;
        DEBUGDEVICE(lx200Name, DBG_SCOPE, "Coordinate format is high precision.");
        return 0;
    }

    DEBUGFDEVICE(lx200Name, DBG_SCOPE, "CMD <%s>", ":GR#");

    tcflush(fd, TCIFLUSH);

    if ((error_type = tty_write_string(fd, ":GR#", &nbytes_write)) != TTY_OK)
        return error_type;

    error_type = tty_read_section(fd, read_buffer, '#', LX200_TIMEOUT, &nbytes_read);

    if (nbytes_read < 1)
    {
        DEBUGFDEVICE(lx200Name, DBG_SCOPE, "RES ERROR <%d>", error_type);
        return error_type;
    }

    read_buffer[nbytes_read - 1] = '\0';

    DEBUGFDEVICE(lx200Name, DBG_SCOPE, "RES <%s>", read_buffer);

    if (read_buffer[5] == '.')
    {
        controller_format = LX200_SHORT_FORMAT;
        DEBUGDEVICE(lx200Name, DBG_SCOPE, "Coordinate format is low precision.");
    }
    else
    {
        controller_format = LX200_LONG_FORMAT;
        DEBUGDEVICE(lx200Name, DBG_SCOPE, "Coordinate format is high precision.");
    }

    tcflush(fd, TCIFLUSH);

    return 0;
}

int selectTrackingMode(int fd, int trackMode)
{
    DEBUGFDEVICE(lx200Name, DBG_SCOPE, "<%s>", __FUNCTION__);
    int error_type;
    int nbytes_write = 0;

    switch (trackMode)
    {
        case LX200_TRACK_SIDEREAL:
            DEBUGFDEVICE(lx200Name, DBG_SCOPE, "CMD <%s>", ":TQ#");
            if ((error_type = tty_write_string(fd, ":TQ#", &nbytes_write)) != TTY_OK)
                return error_type;
            break;
        case LX200_TRACK_SOLAR:
            DEBUGFDEVICE(lx200Name, DBG_SCOPE, "CMD <%s>", ":TS#");
            if ((error_type = tty_write_string(fd, ":TS#", &nbytes_write)) != TTY_OK)
                return error_type;
            break;
        case LX200_TRACK_LUNAR:
            DEBUGFDEVICE(lx200Name, DBG_SCOPE, "CMD <%s>", ":TL#");
            if ((error_type = tty_write_string(fd, ":TL#", &nbytes_write)) != TTY_OK)
                return error_type;
            break;
        case LX200_TRACK_MANUAL:
            DEBUGFDEVICE(lx200Name, DBG_SCOPE, "CMD <%s>", ":TM#");
            if ((error_type = tty_write_string(fd, ":TM#", &nbytes_write)) != TTY_OK)
                return error_type;
            break;
        default:
            return -1;
            break;
    }

    tcflush(fd, TCIFLUSH);
    return 0;
}

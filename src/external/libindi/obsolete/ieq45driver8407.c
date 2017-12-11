#if 0
    IEQ45 Driver
    Copyright (C) 2011 Nacho Mas (mas.ignacio@gmail.com). Only litle changes
    from lx200basic made it by Jasem Mutlaq (mutlaqja@ikarustech.com) 

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

    NOTES on 9407 vs 8406:
	.-Diferente init V#,
        .- Diferent response to :MS#
	.- Diff RT0,1, .. codification

#endif

#include "ieq45driver.h"

#include "indicom.h"
#include "indidevapi.h"

#ifndef _WIN32
#include <termios.h>
#endif

#define IEQ45_TIMEOUT 5 /* FD timeout in seconds */

int controller_format;
int is8407ver = 0;

/**************************************************************************
 Diagnostics
 **************************************************************************/
int check_IEQ45_connection(int fd);

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
/* Get Number of Bars */
int getNumberOfBars(int fd, int *value);
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
/* Set Sexigesimal */
int setCommandXYZ(int fd, int x, int y, int z, const char *cmd);
/* Common routine for Set commands */
int setStandardProcedure(int fd, char *writeData);
/* Set Slew Mode */
int setSlewMode(int fd, int slewMode);
/* Set Alignment mode */
int setAlignmentMode(int fd, unsigned int alignMode);
/* Set Object RA */
int setObjectRA(int fd, double ra);
/* set Object DEC */
int setObjectDEC(int fd, double dec);
/* Set Calender date */
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
/* Select Astro-Physics tracking mode */
int selectAPTrackingMode(int fd, int trackMode);
/* Send Pulse-Guide command (timed guide move), two valid directions can be stacked */
int SendPulseCmd(int fd, int direction, int duration_msec);

/**************************************************************************
 Other Commands
 **************************************************************************/
/* Ensures IEQ45 RA/DEC format is long */
int checkIEQ45Format(int fd);
/* Select a site from the IEQ45 controller */
int selectSite(int fd, int siteNum);
/* Select a catalog object */
int selectCatalogObject(int fd, int catalog, int NNNN);
/* Select a sub catalog */
int selectSubCatalog(int fd, int catalog, int subCatalog);

int check_IEQ45_connection(int in_fd)
{
    int i                  = 0;
    char firmwareVersion[] = ":V#";
    char MountInfo[]       = ":MountInfo#";
    char response[64];
    int nbytes_read = 0;

#ifdef INDI_DEBUG
    IDLog("Testing telescope's connection using :V# command...\n");
#endif

    if (in_fd <= 0)
        return -1;

    for (i = 0; i < 2; i++)
    {
        if (write(in_fd, firmwareVersion, sizeof(firmwareVersion)) < 0)
            return -1;
        tty_read(in_fd, response, 1, IEQ45_TIMEOUT, &nbytes_read);
        if (nbytes_read != 1)
            return -1;
        usleep(50000);
    }

#ifdef INDI_DEBUG
    IDLog("Initializating telescope's using :MountInfo# command...\n");
#endif

    for (i = 0; i < 2; i++)
    {
        if (write(in_fd, MountInfo, sizeof(MountInfo)) < 0)
            return -1;
        tty_read(in_fd, response, 1, IEQ45_TIMEOUT, &nbytes_read);
        if (nbytes_read == 1)
            return 0;
        usleep(50000);
    }

    return -1;
}

/**********************************************************************
* GET
**********************************************************************/
void remove_spaces(char *texto_recibe)
{
    char *texto_sin_espacio;
    for (texto_sin_espacio = texto_recibe; *texto_recibe; texto_recibe++)
    {
        if (isspace(*texto_recibe))
            continue;
        *texto_sin_espacio++ = *texto_recibe;
    }
    *texto_sin_espacio = '\0';
}

int getCommandSexa(int fd, double *value, const char *cmd)
{
    char temp_string[16];
    int error_type;
    int nbytes_write = 0, nbytes_read = 0;

    tcflush(fd, TCIFLUSH);
    if ((error_type = tty_write_string(fd, cmd, &nbytes_write)) != TTY_OK)
        return error_type;

    /*if ( (read_ret = portRead(temp_string, -1, IEQ45_TIMEOUT)) < 1)
     return read_ret;*/
    tty_read_section(fd, temp_string, '#', IEQ45_TIMEOUT, &nbytes_read);

    temp_string[nbytes_read - 1] = '\0';

    /*IDLog("getComandSexa: %s\n", temp_string);*/
    //IEQ45 sometimes send a malformed RA/DEC (intermediate spaces)
    //so I clean before:
    remove_spaces(temp_string);

    if (f_scansexa(temp_string, value))
    {
#ifdef INDI_DEBUG
        IDLog("unable to process [%s]\n", temp_string);
#endif
        return -1;
    }

    tcflush(fd, TCIFLUSH);
    return 0;
}

int getCommandInt(int fd, int *value, const char *cmd)
{
    char temp_string[16];
    float temp_number;
    int error_type;
    int nbytes_write = 0, nbytes_read = 0;

    tcflush(fd, TCIFLUSH);

    if ((error_type = tty_write_string(fd, cmd, &nbytes_write)) != TTY_OK)
        return error_type;

    tty_read_section(fd, temp_string, '#', IEQ45_TIMEOUT, &nbytes_read);

    temp_string[nbytes_read - 1] = '\0';

    /* Float */
    if (strchr(temp_string, '.'))
    {
        if (sscanf(temp_string, "%f", &temp_number) != 1)
            return -1;

        *value = (int)temp_number;
    }
    /* Int */
    else if (sscanf(temp_string, "%d", value) != 1)
        return -1;

    return 0;
}

int getCommandString(int fd, char *data, const char *cmd)
{
    char *term;
    int error_type;
    int nbytes_write = 0, nbytes_read = 0;

    /*if (portWrite(cmd) < 0)
      return -1;*/

    if ((error_type = tty_write_string(fd, cmd, &nbytes_write)) != TTY_OK)
        return error_type;

    /*read_ret = portRead(data, -1, IEQ45_TIMEOUT);*/
    error_type = tty_read_section(fd, data, '#', IEQ45_TIMEOUT, &nbytes_read);
    tcflush(fd, TCIFLUSH);

    if (error_type != TTY_OK)
        return error_type;

    term = strchr(data, '#');
    if (term)
        *term = '\0';

#ifdef INDI_DEBUG
/*IDLog("Requested data: %s\n", data);*/
#endif

    return 0;
}

int getCalendarDate(int fd, char *date)
{
    int dd, mm, yy;
    int error_type;
    int nbytes_read = 0;
    char mell_prefix[3];

    if ((error_type = getCommandString(fd, date, ":GC#")))
        return error_type;

    /* Meade format is MM/DD/YY */

    nbytes_read = sscanf(date, "%d%*c%d%*c%d", &mm, &dd, &yy);
    if (nbytes_read < 3)
        return -1;

    /* We consider years 50 or more to be in the last century, anything less in the 21st century.*/
    if (yy > 50)
        strncpy(mell_prefix, "19", 3);
    else
        strncpy(mell_prefix, "20", 3);

    /* We need to have in in YYYY/MM/DD format */
    snprintf(date, 16, "%s%02d/%02d/%02d", mell_prefix, yy, mm, dd);

    return (0);
}

int getTimeFormat(int fd, int *format)
{
    char temp_string[16];
    int error_type;
    int nbytes_write = 0, nbytes_read = 0;
    int tMode;

    /*if (portWrite(":Gc#") < 0)
    return -1;*/

    if ((error_type = tty_write_string(fd, ":Gc#", &nbytes_write)) != TTY_OK)
        return error_type;

    /*read_ret = portRead(temp_string, -1, IEQ45_TIMEOUT);*/
    if ((error_type = tty_read_section(fd, temp_string, '#', IEQ45_TIMEOUT, &nbytes_read)) != TTY_OK)
        return error_type;

    tcflush(fd, TCIFLUSH);

    if (nbytes_read < 1)
        return error_type;

    temp_string[nbytes_read - 1] = '\0';

    nbytes_read = sscanf(temp_string, "(%d)", &tMode);

    if (nbytes_read < 1)
        return -1;
    else
        *format = tMode;

    return 0;
}

int getSiteName(int fd, char *siteName, int siteNum)
{
    char *term;
    int error_type;
    int nbytes_write = 0, nbytes_read = 0;

    switch (siteNum)
    {
        case 1:
            /*if (portWrite(":GM#") < 0)
      return -1;*/
            if ((error_type = tty_write_string(fd, ":GM#", &nbytes_write)) != TTY_OK)
                return error_type;
            break;
        case 2:
            /*if (portWrite(":GN#") < 0)
      return -1;*/
            if ((error_type = tty_write_string(fd, ":GN#", &nbytes_write)) != TTY_OK)
                return error_type;
            break;
        case 3:
            /*if (portWrite(":GO#") < 0)
       return -1;*/
            if ((error_type = tty_write_string(fd, ":GO#", &nbytes_write)) != TTY_OK)
                return error_type;
            break;
        case 4:
            /*if (portWrite(":GP#") < 0)
      return -1;*/
            if ((error_type = tty_write_string(fd, ":GP#", &nbytes_write)) != TTY_OK)
                return error_type;
            break;
        default:
            return -1;
    }

    /*read_ret = portRead(siteName, -1, IEQ45_TIMEOUT);*/
    error_type = tty_read_section(fd, siteName, '#', IEQ45_TIMEOUT, &nbytes_read);
    tcflush(fd, TCIFLUSH);

    if (nbytes_read < 1)
        return error_type;

    siteName[nbytes_read - 1] = '\0';

    term = strchr(siteName, ' ');
    if (term)
        *term = '\0';

    term = strchr(siteName, '<');
    if (term)
        strcpy(siteName, "unused site");

#ifdef INDI_DEBUG
    IDLog("Requested site name: %s\n", siteName);
#endif

    return 0;
}

int getSiteLatitude(int fd, int *dd, int *mm)
{
    char temp_string[16];
    int error_type;
    int nbytes_write = 0, nbytes_read = 0;

    /*if (portWrite(":Gt#") < 0)
    return -1;*/
    if ((error_type = tty_write_string(fd, ":Gt#", &nbytes_write)) != TTY_OK)
        return error_type;

    /*read_ret = portRead(temp_string, -1, IEQ45_TIMEOUT);*/
    error_type = tty_read_section(fd, temp_string, '#', IEQ45_TIMEOUT, &nbytes_read);
    tcflush(fd, TCIFLUSH);

    if (nbytes_read < 1)
        return error_type;

    temp_string[nbytes_read - 1] = '\0';

    if (sscanf(temp_string, "%d%*c%d", dd, mm) < 2)
        return -1;

#ifdef INDI_DEBUG
    fprintf(stderr, "Requested site latitude in String %s\n", temp_string);
    fprintf(stderr, "Requested site latitude %d:%d\n", *dd, *mm);
#endif

    return 0;
}

int getSiteLongitude(int fd, int *ddd, int *mm)
{
    char temp_string[16];
    int error_type;
    int nbytes_write = 0, nbytes_read = 0;

    if ((error_type = tty_write_string(fd, ":Gg#", &nbytes_write)) != TTY_OK)
        return error_type;

    /*if (portWrite(":Gg#") < 0)
   return -1;*/
    error_type = tty_read_section(fd, temp_string, '#', IEQ45_TIMEOUT, &nbytes_read);
    /*read_ret = portRead(temp_string, -1, IEQ45_TIMEOUT);*/

    tcflush(fd, TCIFLUSH);

    if (nbytes_read < 1)
        return error_type;

    temp_string[nbytes_read - 1] = '\0';

    if (sscanf(temp_string, "%d%*c%d", ddd, mm) < 2)
        return -1;

#ifdef INDI_DEBUG
    fprintf(stderr, "Requested site longitude in String %s\n", temp_string);
    fprintf(stderr, "Requested site longitude %d:%d\n", *ddd, *mm);
#endif

    return 0;
}

int getTrackFreq(int fd, double *value)
{
    float Freq;
    char temp_string[16];
    int error_type;
    int nbytes_write = 0, nbytes_read = 0;

    if ((error_type = tty_write_string(fd, ":GT#", &nbytes_write)) != TTY_OK)
        return error_type;

    /*if (portWrite(":GT#") < 0)
      return -1;*/

    /*read_ret = portRead(temp_string, -1, IEQ45_TIMEOUT);*/
    error_type = tty_read_section(fd, temp_string, '#', IEQ45_TIMEOUT, &nbytes_read);
    tcflush(fd, TCIFLUSH);

    if (nbytes_read < 1)
        return error_type;

    temp_string[nbytes_read] = '\0';

    /*fprintf(stderr, "Telescope tracking freq str: %s\n", temp_string);*/

    if (sscanf(temp_string, "%f#", &Freq) < 1)
        return -1;

    *value = (double)Freq;

#ifdef INDI_DEBUG
    fprintf(stderr, "Tracking frequency value is %f\n", Freq);
#endif

    return 0;
}

int getNumberOfBars(int fd, int *value)
{
    char temp_string[128];
    int error_type;
    int nbytes_write = 0, nbytes_read = 0;

    if ((error_type = tty_write_string(fd, ":D#", &nbytes_write)) != TTY_OK)
        return error_type;
    /*if (portWrite(":D#") < 0)
     return -1;*/

    error_type = tty_read_section(fd, temp_string, '#', IEQ45_TIMEOUT, &nbytes_read);
    tcflush(fd, TCIFLUSH);

    if (nbytes_read < 0)
        return error_type;

    *value = nbytes_read - 1;

    return 0;
}

int getHomeSearchStatus(int fd, int *status)
{
    char temp_string[16];
    int error_type;
    int nbytes_write = 0, nbytes_read = 0;

    if ((error_type = tty_write_string(fd, ":h?#", &nbytes_write)) != TTY_OK)
        return error_type;
    /*if (portWrite(":h?#") < 0)
   return -1;*/

    /*read_ret = portRead(temp_string, 1, IEQ45_TIMEOUT);*/
    error_type = tty_read_section(fd, temp_string, '#', IEQ45_TIMEOUT, &nbytes_read);
    tcflush(fd, TCIFLUSH);

    if (nbytes_read < 1)
        return error_type;

    temp_string[1] = '\0';

    if (temp_string[0] == '0')
        *status = 0;
    else if (temp_string[0] == '1')
        *status = 1;
    else if (temp_string[0] == '2')
        *status = 1;

    return 0;
}

int getOTATemp(int fd, double *value)
{
    char temp_string[16];
    int error_type;
    int nbytes_write = 0, nbytes_read = 0;
    float temp;

    if ((error_type = tty_write_string(fd, ":fT#", &nbytes_write)) != TTY_OK)
        return error_type;

    error_type = tty_read_section(fd, temp_string, '#', IEQ45_TIMEOUT, &nbytes_read);

    if (nbytes_read < 1)
        return error_type;

    temp_string[nbytes_read - 1] = '\0';

    if (sscanf(temp_string, "%f", &temp) < 1)
        return -1;

    *value = (double)temp;

    return 0;
}

int updateSkyCommanderCoord(int fd, double *ra, double *dec)
{
    char coords[16];
    char CR[1] = { (char)0x0D };
    float RA = 0.0, DEC = 0.0;
    int error_type;
    int nbytes_read = 0;

    error_type = write(fd, CR, 1);

    error_type = tty_read(fd, coords, 16, IEQ45_TIMEOUT, &nbytes_read);
    /*read_ret = portRead(coords, 16, IEQ45_TIMEOUT);*/
    tcflush(fd, TCIFLUSH);

    nbytes_read = sscanf(coords, " %g %g", &RA, &DEC);

    if (nbytes_read < 2)
    {
#ifdef INDI_DEBUG
        IDLog("Error in Sky commander number format [%s], exiting.\n", coords);
#endif
        return error_type;
    }

    *ra  = RA;
    *dec = DEC;

    return 0;
}

int updateIntelliscopeCoord(int fd, double *ra, double *dec)
{
    char coords[16];
    char CR[1] = { (char)0x51 }; /* "Q" */
    float RA = 0.0, DEC = 0.0;
    int error_type;
    int nbytes_read = 0;

    /*IDLog ("Sending a Q\n");*/
    error_type = write(fd, CR, 1);
    /* We start at 14 bytes in case its a Sky Wizard, 
     but read one more later it if it's a intelliscope */
    /*read_ret = portRead (coords, 14, IEQ45_TIMEOUT);*/
    error_type = tty_read(fd, coords, 14, IEQ45_TIMEOUT, &nbytes_read);
    tcflush(fd, TCIFLUSH);
    /*IDLog ("portRead() = [%s]\n", coords);*/

    /* Remove the Q in the response from the Intelliscope  but not the Sky Wizard */
    if (coords[0] == 'Q')
    {
        coords[0] = ' ';
        /* Read one more byte if Intelliscope to get the "CR" */
        error_type = tty_read(fd, coords, 1, IEQ45_TIMEOUT, &nbytes_read);
        /*read_ret = portRead (coords, 1, IEQ45_TIMEOUT);*/
    }
    nbytes_read = sscanf(coords, " %g %g", &RA, &DEC);
    /*IDLog ("sscanf() RA = [%f]\n", RA * 0.0390625);*/
    /*IDLog ("sscanf() DEC = [%f]\n", DEC * 0.0390625);*/

    /*IDLog ("Intelliscope output [%s]", coords);*/
    if (nbytes_read < 2)
    {
#ifdef INDI_DEBUG
        IDLog("Error in Intelliscope number format [%s], exiting.\n", coords);
#endif
        return -1;
    }

    *ra  = RA * 0.0390625;
    *dec = DEC * 0.0390625;

    return 0;
}

/**********************************************************************
* SET
**********************************************************************/

int setStandardProcedure(int fd, char *data)
{
    char bool_return[2];
    int error_type;
    int nbytes_write = 0, nbytes_read = 0;

    if ((error_type = tty_write_string(fd, data, &nbytes_write)) != TTY_OK)
        return error_type;

    error_type = tty_read(fd, bool_return, 1, IEQ45_TIMEOUT, &nbytes_read);
    /*read_ret = portRead(boolRet, 1, IEQ45_TIMEOUT);*/
    tcflush(fd, TCIFLUSH);

    if (nbytes_read < 1)
        return error_type;

    if (bool_return[0] == '0')
    {
#ifdef INDI_DEBUG
        IDLog("%s Failed.\n", data);
#endif
        return -1;
    }

#ifdef INDI_DEBUG
    IDLog("%s Successful\n", data);
#endif

    return 0;
}

int setCommandInt(int fd, int data, const char *cmd)
{
    char temp_string[16];
    int error_type;
    int nbytes_write = 0;

    snprintf(temp_string, sizeof(temp_string), "%s%d#", cmd, data);

    if ((error_type = tty_write_string(fd, temp_string, &nbytes_write)) != TTY_OK)
        return error_type;

    /*  if (portWrite(temp_string) < 0)
    return -1;*/

    return 0;
}

int setMinElevationLimit(int fd, int min)
{
    char temp_string[16];

    snprintf(temp_string, sizeof(temp_string), ":Sh%02d#", min);

    return (setStandardProcedure(fd, temp_string));
}

int setMaxElevationLimit(int fd, int max)
{
    char temp_string[16];

    snprintf(temp_string, sizeof(temp_string), ":So%02d*#", max);

    return (setStandardProcedure(fd, temp_string));
}

int setMaxSlewRate(int fd, int slewRate)
{
    char temp_string[16];

    if (slewRate < 2 || slewRate > 8)
        return -1;

    snprintf(temp_string, sizeof(temp_string), ":Sw%d#", slewRate);

    return (setStandardProcedure(fd, temp_string));
}

int setObjectRA(int fd, double ra)
{
    int h, m, s, frac_m;
    char temp_string[16];

    getSexComponents(ra, &h, &m, &s);

    frac_m = (s / 60.0) * 10.;

    if (controller_format == IEQ45_LONG_FORMAT)
        snprintf(temp_string, sizeof(temp_string), ":Sr %02d:%02d:%02d#", h, m, s);
    else
        snprintf(temp_string, sizeof(temp_string), ":Sr %02d:%02d.%01d#", h, m, frac_m);

    /*IDLog("Set Object RA String %s\n", temp_string);*/
    return (setStandardProcedure(fd, temp_string));
}

int setObjectDEC(int fd, double dec)
{
    int d, m, s;
    char temp_string[16];

    getSexComponents(dec, &d, &m, &s);

    switch (controller_format)
    {
        case IEQ45_SHORT_FORMAT:
            /* case with negative zero */
            if (!d && dec < 0)
                snprintf(temp_string, sizeof(temp_string), ":Sd -%02d*%02d#", d, m);
            else
                snprintf(temp_string, sizeof(temp_string), ":Sd %+03d*%02d#", d, m);
            break;

        case IEQ45_LONG_FORMAT:
            /* case with negative zero */
            if (!d && dec < 0)
                snprintf(temp_string, sizeof(temp_string), ":Sd -%02d:%02d:%02d#", d, m, s);
            else
                snprintf(temp_string, sizeof(temp_string), ":Sd %+03d:%02d:%02d#", d, m, s);
            break;
    }

    /*IDLog("Set Object DEC String %s\n", temp_string);*/

    return (setStandardProcedure(fd, temp_string));
}

int setCommandXYZ(int fd, int x, int y, int z, const char *cmd)
{
    char temp_string[16];

    snprintf(temp_string, sizeof(temp_string), "%s %02d:%02d:%02d#", cmd, x, y, z);

    return (setStandardProcedure(fd, temp_string));
}

int setAlignmentMode(int fd, unsigned int alignMode)
{
    /*fprintf(stderr , "Set alignment mode %d\n", alignMode);*/
    int error_type;
    int nbytes_write = 0;

    switch (alignMode)
    {
        case IEQ45_ALIGN_POLAR:
            if ((error_type = tty_write_string(fd, ":AP#", &nbytes_write)) != TTY_OK)
                return error_type;
            /*if (portWrite(":AP#") < 0)
        return -1;*/
            break;
        case IEQ45_ALIGN_ALTAZ:
            if ((error_type = tty_write_string(fd, ":AA#", &nbytes_write)) != TTY_OK)
                return error_type;
            /*if (portWrite(":AA#") < 0)
        return -1;*/
            break;
        case IEQ45_ALIGN_LAND:
            if ((error_type = tty_write_string(fd, ":AL#", &nbytes_write)) != TTY_OK)
                return error_type;
            /*if (portWrite(":AL#") < 0)
        return -1;*/
            break;
    }

    tcflush(fd, TCIFLUSH);
    return 0;
}

int setCalenderDate(int fd, int dd, int mm, int yy)
{
    char temp_string[32];
    char dumpPlanetaryUpdateString[64];
    char bool_return[2];
    int error_type;
    int nbytes_write = 0, nbytes_read = 0;
    yy = yy % 100;

    snprintf(temp_string, sizeof(temp_string), ":SC %02d/%02d/%02d#", mm, dd, yy);

    if ((error_type = tty_write_string(fd, temp_string, &nbytes_write)) != TTY_OK)
        return error_type;

    /*if (portWrite(temp_string) < 0)
    return -1;*/

    /*read_ret = portRead(boolRet, 1, IEQ45_TIMEOUT);*/
    error_type = tty_read(fd, bool_return, 1, IEQ45_TIMEOUT, &nbytes_read);
    tcflush(fd, TCIFLUSH);

    if (nbytes_read < 1)
        return error_type;

    bool_return[1] = '\0';

    if (bool_return[0] == '0')
        return -1;

    /* Read dumped data */
    error_type = tty_read_section(fd, dumpPlanetaryUpdateString, '#', IEQ45_TIMEOUT, &nbytes_read);
    error_type = tty_read_section(fd, dumpPlanetaryUpdateString, '#', 5, &nbytes_read);

    return 0;
}

int setUTCOffset(int fd, double hours)
{
    char temp_string[16];

    snprintf(temp_string, sizeof(temp_string), ":SG %+03d#", (int)hours);

    /*IDLog("UTC string is %s\n", temp_string);*/

    return (setStandardProcedure(fd, temp_string));
}

int setSiteLongitude(int fd, double Long)
{
    int d, m, s;
    char temp_string[32];

    getSexComponents(Long, &d, &m, &s);

    snprintf(temp_string, sizeof(temp_string), ":Sg%03d:%02d#", d, m);

    return (setStandardProcedure(fd, temp_string));
}

int setSiteLatitude(int fd, double Lat)
{
    int d, m, s;
    char temp_string[32];

    getSexComponents(Lat, &d, &m, &s);

    snprintf(temp_string, sizeof(temp_string), ":St%+03d:%02d:%02d#", d, m, s);

    return (setStandardProcedure(fd, temp_string));
}

int setObjAz(int fd, double az)
{
    int d, m, s;
    char temp_string[16];

    getSexComponents(az, &d, &m, &s);

    snprintf(temp_string, sizeof(temp_string), ":Sz%03d:%02d#", d, m);

    return (setStandardProcedure(fd, temp_string));
}

int setObjAlt(int fd, double alt)
{
    int d, m, s;
    char temp_string[16];

    getSexComponents(alt, &d, &m, &s);

    snprintf(temp_string, sizeof(temp_string), ":Sa%+02d*%02d#", d, m);

    return (setStandardProcedure(fd, temp_string));
}

int setSiteName(int fd, char *siteName, int siteNum)
{
    char temp_string[16];

    switch (siteNum)
    {
        case 1:
            snprintf(temp_string, sizeof(temp_string), ":SM %s#", siteName);
            break;
        case 2:
            snprintf(temp_string, sizeof(temp_string), ":SN %s#", siteName);
            break;
        case 3:
            snprintf(temp_string, sizeof(temp_string), ":SO %s#", siteName);
            break;
        case 4:
            snprintf(temp_string, sizeof(temp_string), ":SP %s#", siteName);
            break;
        default:
            return -1;
    }

    return (setStandardProcedure(fd, temp_string));
}

int setSlewMode(int fd, int slewMode)
{
    int error_type;
    int nbytes_write = 0;

    switch (slewMode)
    {
        case IEQ45_SLEW_MAX:
            if ((error_type = tty_write_string(fd, ":RS#", &nbytes_write)) != TTY_OK)
                return error_type;
            /*if (portWrite(":RS#") < 0)
      return -1;*/
            break;
        case IEQ45_SLEW_FIND:
            if ((error_type = tty_write_string(fd, ":RM#", &nbytes_write)) != TTY_OK)
                return error_type;
            /*if (portWrite(":RM#") < 0)
      return -1;*/
            break;
        case IEQ45_SLEW_CENTER:
            if ((error_type = tty_write_string(fd, ":RC#", &nbytes_write)) != TTY_OK)
                return error_type;
            /*if (portWrite(":RC#") < 0)
      return -1;*/
            break;
        case IEQ45_SLEW_GUIDE:
            if ((error_type = tty_write_string(fd, ":RG#", &nbytes_write)) != TTY_OK)
                return error_type;
            /*if (portWrite(":RG#") < 0)
      return -1;*/
            break;
        default:
            break;
    }

    tcflush(fd, TCIFLUSH);
    return 0;
}

int setFocuserMotion(int fd, int motionType)
{
    int error_type;
    int nbytes_write = 0;

    switch (motionType)
    {
        case IEQ45_FOCUSIN:
            if ((error_type = tty_write_string(fd, ":F+#", &nbytes_write)) != TTY_OK)
                return error_type;
#ifdef INDI_DEBUG
/*IDLog("Focus IN Command\n");*/
#endif
            /*if (portWrite(":F+#") < 0)
      return -1;*/
            break;
        case IEQ45_FOCUSOUT:
            if ((error_type = tty_write_string(fd, ":F-#", &nbytes_write)) != TTY_OK)
                return error_type;
#ifdef INDI_DEBUG
/*IDLog("Focus OUT Command\n");*/
#endif
            /*if (portWrite(":F-#") < 0)
       return -1;*/
            break;
    }

    tcflush(fd, TCIFLUSH);
    return 0;
}

int setFocuserSpeedMode(int fd, int speedMode)
{
    int error_type;
    int nbytes_write = 0;

    switch (speedMode)
    {
        case IEQ45_HALTFOCUS:
            if ((error_type = tty_write_string(fd, ":FQ#", &nbytes_write)) != TTY_OK)
                return error_type;
#ifdef INDI_DEBUG
/*IDLog("Halt Focus Command\n");*/
#endif
            /* if (portWrite(":FQ#") < 0)
      return -1;*/
            break;
        case IEQ45_FOCUSSLOW:
            if ((error_type = tty_write_string(fd, ":FS#", &nbytes_write)) != TTY_OK)
                return error_type;
#ifdef INDI_DEBUG
/*IDLog("Focus Slow (FS) Command\n");*/
#endif
            /*if (portWrite(":FS#") < 0)
       return -1;*/
            break;
        case IEQ45_FOCUSFAST:
            if ((error_type = tty_write_string(fd, ":FF#", &nbytes_write)) != TTY_OK)
                return error_type;
#ifdef INDI_DEBUG
/*IDLog("Focus Fast (FF) Command\n");*/
#endif
            /*if (portWrite(":FF#") < 0)
      return -1;*/
            break;
    }

    tcflush(fd, TCIFLUSH);
    return 0;
}

int setGPSFocuserSpeed(int fd, int speed)
{
    char speed_str[8];
    int error_type;
    int nbytes_write = 0;

    if (speed == 0)
    {
        /*if (portWrite(":FQ#") < 0)
      return -1;*/
        if ((error_type = tty_write_string(fd, ":FQ#", &nbytes_write)) != TTY_OK)
            return error_type;
#ifdef INDI_DEBUG
/*IDLog("GPS Focus HALT Command (FQ) \n");*/
#endif

        return 0;
    }

    snprintf(speed_str, 8, ":F%d#", speed);

    if ((error_type = tty_write_string(fd, speed_str, &nbytes_write)) != TTY_OK)
        return error_type;

#ifdef INDI_DEBUG
/*IDLog("GPS Focus Speed command %s \n", speed_str);*/
#endif
    /*if (portWrite(speed_str) < 0)
       return -1;*/

    tcflush(fd, TCIFLUSH);
    return 0;
}

int setTrackFreq(int fd, double trackF)
{
    char temp_string[16];

    snprintf(temp_string, sizeof(temp_string), ":ST %04.1f#", trackF);

    return (setStandardProcedure(fd, temp_string));
}

/**********************************************************************
* Misc
*********************************************************************/

int Slew(int fd)
{
    char slewNum[2];
    int error_type;
    int nbytes_write = 0, nbytes_read = 0;

    if ((error_type = tty_write_string(fd, ":MS#", &nbytes_write)) != TTY_OK)
        return error_type;

    error_type = tty_read(fd, slewNum, 1, IEQ45_TIMEOUT, &nbytes_read);
#ifdef INDI_DEBUG
//IDLog("SLEW _MS# %s %u \n", slewNum, nbytes_read);
#endif

    if (nbytes_read < 1)
        return error_type;

    /* We don't need to read the string message, just return corresponding error code */
    tcflush(fd, TCIFLUSH);

    if (slewNum[0] == '1')
        return 0;
    else if (slewNum[0] == '0')
        return 1;
    else
        return 2;
}

int MoveTo(int fd, int direction)
{
    int nbytes_write = 0;

    switch (direction)
    {
        case IEQ45_NORTH:
            tty_write_string(fd, ":Mn#", &nbytes_write);
            /*portWrite(":Mn#");*/
            break;
        case IEQ45_WEST:
            tty_write_string(fd, ":Mw#", &nbytes_write);
            /*portWrite(":Mw#");*/
            break;
        case IEQ45_EAST:
            tty_write_string(fd, ":Me#", &nbytes_write);
            /*portWrite(":Me#");*/
            break;
        case IEQ45_SOUTH:
            tty_write_string(fd, ":Ms#", &nbytes_write);
            /*portWrite(":Ms#");*/
            break;
        default:
            break;
    }

    tcflush(fd, TCIFLUSH);
    return 0;
}

int SendPulseCmd(int fd, int direction, int duration_msec)
{
    int nbytes_write = 0;
    char cmd[20];
    switch (direction)
    {
        case IEQ45_NORTH:
            sprintf(cmd, ":Mgn%04d#", duration_msec);
            break;
        case IEQ45_SOUTH:
            sprintf(cmd, ":Mgs%04d#", duration_msec);
            break;
        case IEQ45_EAST:
            sprintf(cmd, ":Mge%04d#", duration_msec);
            break;
        case IEQ45_WEST:
            sprintf(cmd, ":Mgw%04d#", duration_msec);
            break;
        default:
            return 1;
    }

    tty_write_string(fd, cmd, &nbytes_write);

    tcflush(fd, TCIFLUSH);
    return 0;
}

int HaltMovement(int fd, int direction)
{
    int error_type;
    int nbytes_write = 0;

    switch (direction)
    {
        case IEQ45_NORTH:
            /*if (portWrite(":Qn#") < 0)
      return -1;*/
            if ((error_type = tty_write_string(fd, ":Qn#", &nbytes_write)) != TTY_OK)
                return error_type;
            break;
        case IEQ45_WEST:
            /*if (portWrite(":Qw#") < 0)
      return -1;*/
            if ((error_type = tty_write_string(fd, ":Qw#", &nbytes_write)) != TTY_OK)
                return error_type;
            break;
        case IEQ45_EAST:
            /*if (portWrite(":Qe#") < 0)
      return -1;*/
            if ((error_type = tty_write_string(fd, ":Qe#", &nbytes_write)) != TTY_OK)
                return error_type;
            break;
        case IEQ45_SOUTH:
            if ((error_type = tty_write_string(fd, ":Qs#", &nbytes_write)) != TTY_OK)
                return error_type;
            /*if (portWrite(":Qs#") < 0)
       return -1;*/
            break;
        case IEQ45_ALL:
            /*if (portWrite(":Q#") < 0)
       return -1;*/
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
    /*if (portWrite(":Q#") < 0)
  return -1;*/
    int error_type;
    int nbytes_write = 0;

    if ((error_type = tty_write_string(fd, ":Q#", &nbytes_write)) != TTY_OK)
        return error_type;

    tcflush(fd, TCIFLUSH);
    return 0;
}

int Sync(int fd, char *matchedObject)
{
    int error_type;
    int nbytes_write = 0, nbytes_read = 0;

    //  if ( (error_type = tty_write_string(fd, ":CM#", &nbytes_write)) != TTY_OK)
    if ((error_type = tty_write_string(fd, ":CMR#", &nbytes_write)) != TTY_OK)
        return error_type;

    /*portWrite(":CM#");*/

    /*read_ret = portRead(matchedObject, -1, IEQ45_TIMEOUT);*/
    error_type = tty_read_section(fd, matchedObject, '#', IEQ45_TIMEOUT, &nbytes_read);

    if (nbytes_read < 1)
        return error_type;

    matchedObject[nbytes_read - 1] = '\0';

    /*IDLog("Matched Object: %s\n", matchedObject);*/

    /* Sleep 10ms before flushing. This solves some issues with IEQ45 compatible devices. */
    usleep(10000);
    tcflush(fd, TCIFLUSH);

    return 0;
}

int selectSite(int fd, int siteNum)
{
    int error_type;
    int nbytes_write = 0;

    switch (siteNum)
    {
        case 1:
            if ((error_type = tty_write_string(fd, ":W1#", &nbytes_write)) != TTY_OK)
                return error_type;
            /*if (portWrite(":W1#") < 0)
       return -1;*/
            break;
        case 2:
            if ((error_type = tty_write_string(fd, ":W2#", &nbytes_write)) != TTY_OK)
                return error_type;
            /*if (portWrite(":W2#") < 0)
       return -1;*/
            break;
        case 3:
            if ((error_type = tty_write_string(fd, ":W3#", &nbytes_write)) != TTY_OK)
                return error_type;
            /*if (portWrite(":W3#") < 0)
       return -1;*/
            break;
        case 4:
            if ((error_type = tty_write_string(fd, ":W4#", &nbytes_write)) != TTY_OK)
                return error_type;
            /*if (portWrite(":W4#") < 0)
       return -1;*/
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
    char temp_string[16];
    int error_type;
    int nbytes_write = 0;

    switch (catalog)
    {
        case IEQ45_STAR_C:
            snprintf(temp_string, sizeof(temp_string), ":LS%d#", NNNN);
            break;
        case IEQ45_DEEPSKY_C:
            snprintf(temp_string, sizeof(temp_string), ":LC%d#", NNNN);
            break;
        case IEQ45_MESSIER_C:
            snprintf(temp_string, sizeof(temp_string), ":LM%d#", NNNN);
            break;
        default:
            return -1;
    }

    if ((error_type = tty_write_string(fd, temp_string, &nbytes_write)) != TTY_OK)
        return error_type;
    /*if (portWrite(temp_string) < 0)
   return -1;*/

    tcflush(fd, TCIFLUSH);
    return 0;
}

int selectSubCatalog(int fd, int catalog, int subCatalog)
{
    char temp_string[16];
    switch (catalog)
    {
        case IEQ45_STAR_C:
            snprintf(temp_string, sizeof(temp_string), ":LsD%d#", subCatalog);
            break;
        case IEQ45_DEEPSKY_C:
            snprintf(temp_string, sizeof(temp_string), ":LoD%d#", subCatalog);
            break;
        case IEQ45_MESSIER_C:
            return 1;
        default:
            return 0;
    }

    return (setStandardProcedure(fd, temp_string));
}

int checkIEQ45Format(int fd)
{
    char temp_string[16];
    controller_format = IEQ45_LONG_FORMAT;
    int error_type;
    int nbytes_write = 0, nbytes_read = 0;

    if ((error_type = tty_write_string(fd, ":GR#", &nbytes_write)) != TTY_OK)
        return error_type;

    /*if (portWrite(":GR#") < 0)
   return -1;*/

    /*read_ret = portRead(temp_string, -1, IEQ45_TIMEOUT);*/
    error_type = tty_read_section(fd, temp_string, '#', IEQ45_TIMEOUT, &nbytes_read);

    if (nbytes_read < 1)
        return error_type;

    temp_string[nbytes_read - 1] = '\0';

    /* Check whether it's short or long */
    if (temp_string[5] == '.')
    {
        controller_format = IEQ45_SHORT_FORMAT;
        return 0;
    }
    else
        return 0;
}

int selectTrackingMode(int fd, int trackMode)
{
    int error_type;
    int nbytes_write = 0;

    switch (trackMode)
    {
        case IEQ45_TRACK_SIDERAL:
#ifdef INDI_DEBUG
            IDLog("Setting tracking mode to sidereal.\n");
#endif
            if ((error_type = tty_write_string(fd, ":RT0#", &nbytes_write)) != TTY_OK)
                return error_type;
            /*if (portWrite(":TQ#") < 0)
       return -1;*/
            break;
        case IEQ45_TRACK_LUNAR:
#ifdef INDI_DEBUG
            IDLog("Setting tracking mode to LUNAR.\n");
#endif
            if ((error_type = tty_write_string(fd, ":RT1#", &nbytes_write)) != TTY_OK)
                return error_type;
            /*if (portWrite(":TL#") < 0)
       return -1;*/
            break;
        case IEQ45_TRACK_SOLAR:
#ifdef INDI_DEBUG
            IDLog("Setting tracking mode to SOLAR.\n");
#endif
            if ((error_type = tty_write_string(fd, ":RT2#", &nbytes_write)) != TTY_OK)
                return error_type;
            /*if (portWrite(":TM#") < 0)
      return -1;*/
            break;
        case IEQ45_TRACK_ZERO:
#ifdef INDI_DEBUG
            IDLog("Setting tracking mode to custom.\n");
#endif
            if ((error_type = tty_write_string(fd, ":RT4#", &nbytes_write)) != TTY_OK)
                return error_type;
            /*if (portWrite(":TM#") < 0)
      return -1;*/
            break;

        default:
            return -1;
            break;
    }

    tcflush(fd, TCIFLUSH);
    return 0;
}

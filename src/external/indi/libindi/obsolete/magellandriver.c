#if 0
    MAGELLAN Driver
    Copyright (C) 2011 Onno Hommes  (ohommes@alumni.cmu.edu)

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

#endif

#include "magellandriver.h"

#include "indicom.h"
#include "indidevapi.h"

#ifndef _WIN32
#include <termios.h>
#endif

/**************************************************************************
 Connection Diagnostics
 **************************************************************************/
char ACK(int fd);
int check_magellan_connection(int fd);

/**************************************************************************
 Get Commands: store data in the supplied buffer. 
 Return 0 on success or -1 on failure 
 **************************************************************************/

/* Get Double from Sexagisemal */
int getCommandSexa(int fd, double *value, const char *cmd);

/* Get String */
static int getCommandString(int fd, char *data, const char *cmd);

/* Get Calender data */
int getCalendarDate(int fd, char *date);

/**************************************************************************
 Driver Implementations
 **************************************************************************/

/* 
   Check the Magellan Connection using any set command
   Magellan 1 does not support any sey but still sends
   'OK' for it
*/
int check_magellan_connection(int fd)
{
    int i = 0;

    /* Magellan I ALways Response OK for :S?# */
    char ack[4] = ":S?#";

    char Response[64];
    int nbytes_read = 0;

#ifdef INDI_DEBUG
    IDLog("Testing telescope's connection...\n");
#endif

    if (fd <= 0)
        return MAGELLAN_ERROR;

    for (i = 0; i < CONNECTION_RETRIES; i++)
    {
        if (write(fd, ack, 4) < 0)
            return MAGELLAN_ERROR;

        tty_read(fd, Response, 2, MAGELLAN_TIMEOUT, &nbytes_read);

        if (nbytes_read == 2)
            return MAGELLAN_OK;

        usleep(50000);
    }

    tcflush(fd, TCIFLUSH);
    return MAGELLAN_ERROR;
}

/**********************************************************************
* GET FUNCTIONS
**********************************************************************/

char ACK(int fd)
{
    /* Magellan I ALways Response OK for :S?# (any set will do)*/
    char ack[4] = ":S?#";
    char Response[64];
    int nbytes_read = 0;
    int i           = 0;
    int result      = MAGELLAN_ERROR;

    /* Check for Comm handle */
    if (fd > 0)
    {
        for (i = 0; i < CONNECTION_RETRIES; i++)
        {
            /* Send ACK string to Magellan */
            if (write(fd, ack, 4) >= 0)
            {
                /* Read Response */
                tty_read(fd, Response, 2, MAGELLAN_TIMEOUT, &nbytes_read);

                /* If the two byte OK is returned we have an Ack */
                if (nbytes_read == 2)
                {
                    result = MAGELLAN_ACK;
                    break; /* Force quick success return */
                }
                else
                    usleep(50000);
            }
        }
    }

    tcflush(fd, TCIFLUSH);
    return result;
}

int getCommandSexa(int fd, double *value, const char *cmd)
{
    char temp_string[16];
    int result       = MAGELLAN_ERROR;
    int nbytes_write = 0, nbytes_read = 0;

    if ((tty_write_string(fd, cmd, &nbytes_write)) == TTY_OK)
    {
        if ((tty_read_section(fd, temp_string, '#', MAGELLAN_TIMEOUT, &nbytes_read)) == TTY_OK)
        {
            temp_string[nbytes_read - 1] = '\0';

            if (f_scansexa(temp_string, value))
            {
#ifdef INDI_DEBUG
                IDLog("unable to process [%s]\n", temp_string);
#endif
            }
            else
            {
                result = MAGELLAN_OK;
            }
        }
    }

    tcflush(fd, TCIFLUSH);
    return result;
}

static int getCommandString(int fd, char *data, const char *cmd)
{
    int nbytes_write = 0, nbytes_read = 0;
    int result = MAGELLAN_ERROR;

    if ((tty_write_string(fd, cmd, &nbytes_write)) == TTY_OK)
    {
        if ((tty_read_section(fd, data, '#', MAGELLAN_TIMEOUT, &nbytes_read)) == TTY_OK)
        {
            data[nbytes_read - 1] = '\0';
            result                = MAGELLAN_OK;
        }
    }

    tcflush(fd, TCIFLUSH);
    return result;
}

int getCalendarDate(int fd, char *date)
{
    int dd, mm, yy;
    char century[3];
    int result;

    if ((result = getCommandString(fd, date, "#:GC#")))
    {
        /* Magellan format is MM/DD/YY */
        if ((sscanf(date, "%d%*c%d%*c%d", &mm, &dd, &yy)) > 2)
        {
            /* Consider years 92 or more to be in the last century, anything less 
	in the 21st century.*/
            if (yy > CENTURY_THRESHOLD)
                strncpy(century, "19", 3);
            else
                strncpy(century, "20", 3);

            /* Format must be YYYY/MM/DD format */
            snprintf(date, 16, "%s%02d/%02d/%02d", century, yy, mm, dd);
        }
    }

    return result;
}

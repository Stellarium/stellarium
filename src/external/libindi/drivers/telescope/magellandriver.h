/*
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
    Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA  02110-1301  USA

*/

#pragma once

/* Just use Default tracking for what ever telescope is feeding Magellan I */
enum TFreq
{
    MAGELLAN_TRACK_DEFAULT,
    MAGELLAN_TRACK_LUNAR,
    MAGELLAN_TRACK_MANUAL
};

/* Time Format */
enum TTimeFormat
{
    MAGELLAN_24,
    MAGELLAN_AM,
    MAGELLAN_PM
};

#define MAGELLAN_TIMEOUT 5   /* FD timeout in seconds */
#define MAGELLAN_ERROR   -1  /* Default Error Code */
#define MAGELLAN_OK      0   /* Default Success Code */
#define MAGELLAN_ACK     'P' /* Default Success Code */

#define CENTURY_THRESHOLD  91 /* When to goto 21st Century */
#define CONNECTION_RETRIES 2  /* Retry Attempt cut-off */

/* GET formatted sexagisemal value from device, return as double */
#define getMAGELLANRA(fd, x)  getCommandSexa(fd, x, "#:GR#")
#define getMAGELLANDEC(fd, x) getCommandSexa(fd, x, "#:GD#")

#ifdef __cplusplus
extern "C" {
#endif

/**************************************************************************
 Diagnostics
 **************************************************************************/
char ACK(int fd);
int check_magellan_connection(int fd);

/**************************************************************************
 Get Commands: store data in the supplied buffer. Return 0 on success or -1
 on failure
 **************************************************************************/

/* Get Double from Sexagisemal */
int getCommandSexa(int fd, double *value, const char *cmd);

/* Get Calender data */
int getCalendarDate(int fd, char *date);

#ifdef __cplusplus
}
#endif

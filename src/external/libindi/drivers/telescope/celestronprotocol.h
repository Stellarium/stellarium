/*
* Header File for the Telescope Control protocols for the Meade LX200
* Author:  John Kielkopf (kielkopf@louisville.edu)
*
* This file contains header information used in common with xmtel.

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
*
* 15 May 2003 -- Version 2.00
*
*/

#pragma once

#include <libnova/julian_day.h>

/* These are user defined quantities that set the limits over which it */
/* is safe to operate the telescope.  */

/* LOWER is the number of degrees from the zenith that you will allow. */
/* Use 80, for example, to keep the eyepiece end out of the fork arm space */
/* of an LX200 telescope. */

#define LOWER 90.

/* HIGHER is the horizon.  0 is an unobstructed horizon in every direction. */
/* Use 10, for example, to limit sighting below 10 degrees above the horizon. */

#define HIGHER 0.

/* Set this if a slew to the north sends the telescope south. */

#define REVERSE_NS 0 /* 1 for reverse; 0 for normal. */

/* Set this for maximum slew rate allowed in degree/sec. */

#define MAXSLEWRATE 4 /* 2 for safety; 4 for 16-inch; 8 otherwise. */

/* The following parameters are used internally to set speed and direction. */
/* Do not change these values. */

#define SLEW   0
#define FIND   1
#define CENTER 2
#define GUIDE  3

#if REVERSE_NS > 0
#define NORTH 3
#define SOUTH 0
#else
#define NORTH 0
#define SOUTH 3
#endif

#define EAST 2
#define WEST 1

/* Slew speed defines */

#define SLEWRATE8 8 /* should be 8 degrees per second (not 16-inch) */
#define SLEWRATE4 4 /* should be 4 degrees per second */
#define SLEWRATE3 3 /* should be 3 degrees per second */
#define SLEWRATE2 2 /* should be 2 degrees per second */

/* Reticle defines */

#define BRIGHTER 16 /* increase */
#define DIMMER   8  /* decrease */
#define BLINK0   0  /* no blinking */
#define BLINK1   1  /* blink rate 1 */
#define BLINK2   2  /* blink rate 2 */
#define BLINK3   4  /* blink rate 3 */

/* Focus defines */

#define FOCUSOUT  8 /* positive voltage output */
#define FOCUSIN   4 /* negative voltage output */
#define FOCUSSTOP 0 /* no output */
#define FOCUSSLOW 1 /* half voltage */
#define FOCUSFAST 2 /* full voltage */

/* Rotator defines */

#define ROTATORON  1 /* image rotator on */
#define ROTATOROFF 0 /* image rotator off */

/* Fan defines */

#define FANON  1 /* cooling fan on */
#define FANOFF 0 /* cooling fan off */

#ifdef __cplusplus
extern "C" {
#endif

int ConnectTel(char *port);
void DisconnectTel();
/* 0 if connection is OK, -1 otherwise */
int CheckConnectTel();

void SetRate(int newRate);
void SetLimits(double limitLower, double limitHigher);
int StartSlew(int direction);
int StopSlew(int direction);
double GetRA();
double GetDec();
int SlewToCoords(double newRA, double newDec);
int SyncToCoords(double newRA, double newDec);
int CheckCoords(double desRA, double desDec, double tolRA, double tolDEC);
int isScopeSlewing();
int updateLocation(double lng, double lat);
int updateTime(struct ln_date *utc, double utc_offset);

void StopNSEW();
int SetSlewRate();

int SyncLST(double newTime);
int SyncLocalTime();

void Reticle(int reticle);
void Focus(int focus);
void Derotator(int rotate);
void Fan(int fan);

#ifdef __cplusplus
}
#endif

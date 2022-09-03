/* Copyright (C) 2018, Project Pluto

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
02110-1301, USA. */

/* -----------------------------------------------------------------------! */
/* Downloaded from ftp://ftp.imcce.fr/pub/ephem/satel/htc20/htc20.f         */
/* translated by f2c (version 20100827),  then heavily edited.              */
/* A test routine is provided at the end of this file.                      */
/*                                                                          */
/* Positions (in astronomical unit) and velocities (in astronomical unit    */
/* per day) of Helene, Telesto, and Calypso (Lagrangian satellites of Dione */
/* and Tethys in Saturn's system) referred to the center of Saturn and to   */
/* the mean ecliptic and mean equinox of J2000. From: "An upgraded theory   */
/* for Helene, Telesto, and Calypso",   Oberti P., Vienne A., 2002, A&A     */
/* XXX, XXX                                                                 */
/* Pascal.Oberti@obs-nice.fr / Alain.Vienne@imcce.fr                        */
/*                                                                          */
/*   WARNING:  Comparison to JPL's _Horizons_ ephemerides show errors of    */
/* several thousand km for all three objects.  I don't really know why.     */
/* I think it may just be due to the fact that modelling objects at         */
/* libration points could require more terms than are given below.  Also,   */
/* HTC20 does predate the arrival of Cassini at Jupiter,  and was based     */
/* entirely on twenty years of ground-based observations.                   */
/*                                                                          */
/* Input: jd,  a Saturnian time in Julian day (TT)                          */
/* sat_no: the satellite's number (0: Helene, 1: Telesto, 2: Calypso)       */
/* Output: xyz(3) (real*8 array); VXYZ(3) (real*8 array)                    */
/* xyz: the 3 Cartesian coordinates of the satellite's position (a.u.)      */
/* vxyz: the 3 Cartesian coordinates of the satellite's velocity (a.u./day) */
/*      (may be NULL to indicate no interest in the velocity)               */
/* Return value is non-zero if sat_no is invalid (i.e.,  not 0, 1, or 2)    */
/* -----------------------------------------------------------------------! */

/* Sample output from the test code below for 'htc20b 2451590':
Helen -351737.078  126965.254  -33017.275  -289321.532 -705652.257  396448.135
Teles  222550.256 -182154.616   71289.412   640383.150  632265.407 -391213.715
Calyp -278795.483  -66349.449   64265.081   292171.217 -836964.572  420403.505

*/

#include <math.h>
#include <stdint.h>

#define HTC2_HELENE           0
#define HTC2_TELESTO          1
#define HTC2_CALYPSO          2

int htc20( const double jd, const int sat_no, double *xyz, double *vxyz);


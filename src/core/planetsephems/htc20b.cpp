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

#include "htc20b.hpp"

int htc20( const double jd, const int sat_no, double *xyz, double *vxyz)
{
    /* Initialized data */

    static const double rnu1[3] = { 2.29427177,3.32489098,-3.32489617 };
    static const double rnu2[3] = { -.00802443,-.00948045,.00946761 };
    static const double rnu3[3] = { 2.29714724,3.33170385,-3.33170262 };
    static const double rlamda[3] = { 2.29571726,3.32830561,3.32830561 };
    static const double phi1[3] = { 3.27342548,6.2423359,5.4138476 };
    static const double phi2[3] = { 1.30770422,4.62624497,1.36874776 };
    static const double phi3[3] = { .77232982,.04769409,5.64157287 };
    static const double theta[3] = { 3.07410251,3.24465053,3.2507488 };

    static const int8_t ih[48]   /* was [4][12] */ = {
                0,  0,  0,  1,
                0,  0,  1,  0,
                0,  1,  0, -1,
                0,  1,  0,  1,
                0,  2,  0, -1,
                0,  2,  0,  1,
                0,  3,  0, -1,
                0,  3,  0,  1,
                1, -1,  0,  1,
                1,  0,  0, -1,
                1,  0,  0,  1,
                1,  1,  0,  1   };
    static const int16_t rh[144] = {
                -2396,  -399,  442, 1278, -4939,  2466,
                  557, -2152, 1074, 5500,   916, -1015,
                    0,     0,    0,    0,     5,    10,
                    0,     5,   10,    0,     0,     0,
                  -67,   264, -132, -677,  -110,   123,
                  294,    48,  -53, -154,   608,  -304,
                  -66,   265, -133, -676,  -107,   122,
                 -295,   -47,   53,  151,  -607,   303,
                   15,    16,  -10,  -44,    33,   -13,
                   19,   -14,    6,   35,    38,   -23,
                   15,    17,  -10,  -44,    33,   -13,
                  -19,    14,   -6,  -35,   -38,    23,
                    2,     0,    0,   -2,     4,    -2,
                    0,    -2,    0,    4,     2,     0,
                    2,     0,    0,   -2,     4,    -2,
                    0,     2,    0,   -4,    -2,     0,
                    0,     0,    0,    0,     3,     0,
                    0,     0,    0,   -3,     0,     0,
                   -2,     8,   -4,    0,     0,     0,
                    9,     0,   -2,    0,     0,     0,
                    0,     2,    0,  -13,    -2,     2,
                   -3,     0,    0,    3,   -11,     6,
                    0,     0,    0,    0,     3,     0,
                    0,     0,    0,   -3,     0,     0 };

    static const int8_t it[24]   /* was [4][6] */ = {
                         0,  0,  0,  1,
                         0,  0,  1,  0,
                         0,  1,  0, -1,
                         0,  1,  0,  1,
                         1,  0,  0, -1,
                         1,  0,  0,  1   };
    static const int16_t rt[72]   /* was [6][12] */ = {
                -1933,  -253, 320, 1237, -5767,  2904,
                  372, -1733, 873, 6432,   842, -1066,
                    0,     0,   0,    2,    10,    19,
                    2,    10,  19,    0,     0,     0,
                   -6,    29, -15, -108,   -14,    18,
                   32,     4,  -5,  -21,    97,   -49,
                   -6,    29, -15, -108,   -14,    18,
                  -33,    -4,   5,   20,   -97,    49,
                    0,     7,  -3,    0,     0,     0,
                    7,     0,   0,    0,     0,     0,
                    0,     2,   0,  -16,    -2,     3,
                   -2,     0,   0,    3,   -14,     7 };

    static const int8_t ic[48]   /* was [4][12] */ = {
                         0,  0,  0,  1,
                         0,  0,  1,  0,
                         0,  1, -1,  0,
                         0,  1,  0, -1,
                         0,  1,  0,  1,
                         0,  1,  1,  0,
                         0,  2,  0, -1,
                         0,  2,  0,  1,
                         1, -1,  0, -1,
                         1,  0,  0, -1,
                         1,  0,  0,  1,
                         1,  1,  0, -1 };
    static const int16_t rc[144]   /* was [6][24] */ = {
                  651, 1615, -910, -6145,  2170, -542,
                -1846,  652, -163, -2166, -5375, 3030,
                    0,    0,    0,     5,    27,   52,
                    5,   27,   52,     0,     0,    0,
                    0,    0,   -2,     0,     0,    0,
                    0,    0,    0,     0,     0,   -2,
                  -78,   27,   -7,   -89,  -225,  127,
                   27,   68,  -38,  -257,    89,  -22,
                  -77,   28,   -7,   -92,  -225,  127,
                  -28,  -67,   38,   257,   -92,   23,
                    0,    0,   -2,     0,     0,    0,
                    0,    0,    0,     0,     0,    2,
                   -2,    0,    0,     4,    -6,    3,
                    0,    2,    0,    -7,    -3,    2,
                   -2,    0,    0,     4,    -6,    3,
                    0,   -2,    0,     7,     3,   -2,
                    0,    0,    0,     2,     0,    0,
                    0,    0,    0,     0,    -2,    0,
                   -4,    0,    0,    -9,   -22,   12,
                    0,    3,   -2,   -25,     9,   -2,
                  -11,    4,    0,     0,     0,    0,
                   -4,  -10,    6,     0,     0,    0,
                    0,    0,    0,     2,     0,    0,
                    0,    0,    0,     0,    -2,    0 };

   int i, n_terms = (sat_no == HTC2_TELESTO ? 6 : 12);
   const int8_t *iptr;
   const int16_t *rptr;
   double arg1, arg2, arg3, arg4;
   const double t = jd - 2451545.;

   switch( sat_no)
      {
      case HTC2_HELENE:
         iptr = ih;
         rptr = rh;
         break;
      case HTC2_TELESTO:
         iptr = it;
         rptr = rt;
         break;
      case HTC2_CALYPSO:
         iptr = ic;
         rptr = rc;
         break;
      default:    /* Not a valid satellite */
         return( -1);
      }

   for (i = 0; i < 3; ++i)
      {
      xyz[i] = 0.;
      if( vxyz)
         vxyz[i] = 0.;
      }

   arg1 = rnu1[sat_no] * t + phi1[sat_no];
   arg2 = rnu2[sat_no] * t + phi2[sat_no];
   arg3 = rnu3[sat_no] * t + phi3[sat_no];
   arg4 = rlamda[sat_no] * t + theta[sat_no];

   while( n_terms--)
      {
      const double ang =
                     iptr[0] * arg1 + iptr[1] * arg2
                   + iptr[2] * arg3 + iptr[3] * arg4;
      const double cos_ang = cos( ang);
      const double sin_ang = sin( ang);

      for( i = 0; i < 3; ++i, rptr++)
         {
         xyz[i]     += (double)rptr[0] * cos_ang + (double)rptr[6] * sin_ang;
         if( vxyz)
            vxyz[i] += (double)rptr[3] * cos_ang + (double)rptr[9] * sin_ang;
         }
      iptr += 4;
      rptr += 9;
      }

   for( i = 0; i < 3; ++i)
      {
      *xyz++ *= 1.e-6;
      if( vxyz)
         *vxyz++ *= 1.e-6;
      }
   return 0;
}               /* htc20 */


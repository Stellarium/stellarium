/* jpleph.h: header for JPL ephemeris functions

Copyright (C) 2011, Project Pluto

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
02110-1301, USA.    */

#ifndef JPLEPH_H
#define JPLEPH_H

/***************************************************************************
*******                  JPLEPH.H                                  *********
****************************************************************************
**  This header file is used both by ASC2EPH and TESTEPH programs.        **
****************************************************************************
**  Written: May 28, 1997 by PAD   **  Last modified: June 23,1997 by PAD **
**  Modified further by Bill Gray,  Jun-Aug 2001                          **
****************************************************************************
**  PAD: dr. Piotr A. Dybczynski,          e-mail: dybol@phys.amu.edu.pl  **
**   Astronomical Observatory of the A.Mickiewicz Univ., Poznan, Poland   **
***************************************************************************/

/* By default,  in Windoze 32,  the JPL ephemeris functions are compiled
   into a DLL.  This is not really all that helpful at present,  but may
   be useful to people who want to use the functions from languages other
   than C. */

#ifdef _WIN32
#define DLL_FUNC __stdcall
#else
#define DLL_FUNC
#endif

#ifdef __WATCOMC__
   #include <stdbool.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

void * DLL_FUNC jpl_init_ephemeris( const char *ephemeris_filename,
                                             char nam[][6], double *val);
void DLL_FUNC jpl_close_ephemeris( void *ephem);
int DLL_FUNC jpl_state( void *ephem, const double et, const int list[14],
                          double pv[][6], double nut[4], const int bary);
int DLL_FUNC jpl_pleph( void *ephem, const double et, const int ntarg,
                      const int ncent, double rrd[], const int calc_velocity);
double DLL_FUNC jpl_get_double( const void *ephem, const int value);
long DLL_FUNC jpl_get_long( const void *ephem, const int value);
int DLL_FUNC make_sub_ephem( void *ephem, const char *sub_filename,
                              const double start_jd, const double end_jd);
double DLL_FUNC jpl_get_constant( const int idx, void *ephem, char *constant_name);

#ifdef __cplusplus
}
#endif

         /* Following are constants used in          */
         /* jpl_get_double( ) and jpl_get_long( ):   */

#define JPL_EPHEM_START_JD               0
#define JPL_EPHEM_END_JD                 8
#define JPL_EPHEM_STEP                  16
#define JPL_EPHEM_N_CONSTANTS           24
#define JPL_EPHEM_AU_IN_KM              28
#define JPL_EPHEM_EARTH_MOON_RATIO      36
#define JPL_EPHEM_IPT_ARRAY             44
#define JPL_EPHEM_EPHEMERIS_VERSION    224
#define JPL_EPHEM_KERNEL_SIZE          228
#define JPL_EPHEM_KERNEL_RECORD_SIZE   232
#define JPL_EPHEM_KERNEL_NCOEFF        236
#define JPL_EPHEM_KERNEL_SWAP_BYTES    240

         /* The following error codes may be returned by */
         /* jpl_state() and jpl_pleph():                 */
#define JPL_EPH_OUTSIDE_RANGE                (-1)
#define JPL_EPH_READ_ERROR                   (-2)
#define JPL_EPH_QUANTITY_NOT_IN_EPHEMERIS    (-3)
#define JPL_EPH_INVALID_INDEX                (-5)
#define JPL_EPH_FSEEK_ERROR                  (-6)

int DLL_FUNC jpl_init_error_code( void);

         /* The following error codes may be returned by       */
         /* jpl_init_error_code( ) after jpl_init_ephemeris( ) */
         /* has been called:                                   */

#define JPL_INIT_NO_ERROR                 0
#define JPL_INIT_FILE_NOT_FOUND          -1
#define JPL_INIT_FSEEK_FAILED            -2
#define JPL_INIT_FREAD_FAILED            -3
#define JPL_INIT_FREAD2_FAILED           -4
#define JPL_INIT_FREAD5_FAILED           -10
#define JPL_INIT_FILE_CORRUPT            -5
#define JPL_INIT_MEMORY_FAILURE          -6
#define JPL_INIT_FREAD3_FAILED           -7
#define JPL_INIT_FREAD4_FAILED           -8
#define JPL_INIT_NOT_CALLED              -9

#define jpl_get_pvsun( ephem) ((double *)((char *)ephem + 248))

/* addition for use in stellarium */
#define JPL_MAX_N_CONSTANTS 1018
#define CALC_VELOCITY       0
#define CENTRAL_PLANET_ID   11  //ID of sun in JPL enumeration

const char * jpl_init_error_message(void);

#endif // JPLEPH_H

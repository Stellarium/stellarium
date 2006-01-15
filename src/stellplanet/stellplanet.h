/*
Copyright (C) 2000 Liam Girdwood <liam@nova-ioe.org>
Copyright (C) 2003 Fabien Chéreau

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Library General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#ifndef _STELLPLANET_H_
#define _STELLPLANET_H_

#include "tass17gust86.h"
#include "esaphodei.h"
#include "elp82b.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Chapter 31 Pg 206-207 Equ 31.1 31.2 , 31.3 using VSOP 87
 * Calculate planets rectangular heliocentric ecliptical coordinates
 * for given julian day. Values are in UA.
 * params : Julian day, rect coords */
void get_mercury_helio_coords(double JD, double * X, double * Y, double * Z);
void get_venus_helio_coords(double JD, double * X, double * Y, double * Z);
void get_earth_helio_coords(double JD, double * X, double * Y, double * Z);
void get_mars_helio_coords(double JD, double * X, double * Y, double * Z);
void get_jupiter_helio_coords(double JD, double * X, double * Y, double * Z);
void get_saturn_helio_coords(double JD, double * X, double * Y, double * Z);
void get_uranus_helio_coords(double JD, double * X, double * Y, double * Z);
void get_neptune_helio_coords(double JD, double * X, double * Y, double * Z);
void get_pluto_helio_coords(double JD, double * X, double * Y, double * Z);

void get_europa_parent_coords(double JD, double * X, double * Y, double * Z);
void get_callisto_parent_coords(double JD, double * X, double * Y, double * Z);
void get_io_parent_coords(double JD, double * X, double * Y, double * Z);
void get_ganymede_parent_coords(double JD, double * X, double * Y, double * Z);

// Return 0 of course...
void get_sun_helio_coords(double JD, double * X, double * Y, double * Z) {*X=0.; *Y=0.; *Z=0.;};

/* Calculate the rectangular geocentric lunar coordinates to the inertial mean
 * ecliptic and equinox of J2000.
 * The geocentric coordinates returned are in units of UA.
 * This function is based upon the Lunar Solution ELP2000-82B by
 * Michelle Chapront-Touze and Jean Chapront of the Bureau des Longitudes,
 * Paris. ELP 2000-82B theory
 * param JD Julian day, rect pos */
void get_lunar_parent_coordsv(double JD, double* XYZ)
  {GetElp82bCoor(JD,XYZ);}

void get_mercury_helio_coordsv(double JD, double* XYZ) {get_mercury_helio_coords(JD, &XYZ[0], &XYZ[1], &XYZ[2]);}
void get_venus_helio_coordsv(double JD, double* XYZ) {get_venus_helio_coords(JD, &XYZ[0], &XYZ[1], &XYZ[2]);}
void get_earth_helio_coordsv(double JD, double* XYZ) {get_earth_helio_coords(JD, &XYZ[0], &XYZ[1], &XYZ[2]);}
void get_mars_helio_coordsv(double JD, double* XYZ) {get_mars_helio_coords(JD, &XYZ[0], &XYZ[1], &XYZ[2]);}
void get_jupiter_helio_coordsv(double JD, double* XYZ) {get_jupiter_helio_coords(JD, &XYZ[0], &XYZ[1], &XYZ[2]);}
void get_saturn_helio_coordsv(double JD, double* XYZ) {get_saturn_helio_coords(JD, &XYZ[0], &XYZ[1], &XYZ[2]);}
void get_uranus_helio_coordsv(double JD, double* XYZ) {get_uranus_helio_coords(JD, &XYZ[0], &XYZ[1], &XYZ[2]);}
void get_neptune_helio_coordsv(double JD, double* XYZ) {get_neptune_helio_coords(JD, &XYZ[0], &XYZ[1], &XYZ[2]);}
void get_pluto_helio_coordsv(double JD, double* XYZ) {get_pluto_helio_coords(JD, &XYZ[0], &XYZ[1], &XYZ[2]);}

void get_phobos_parent_coordsv(double JD, double* XYZ)
  {GetEsaPhoDeiCoor(JD,ESAPHODEI_PHOBOS,XYZ);}
void get_deimos_parent_coordsv(double JD, double* XYZ)
  {GetEsaPhoDeiCoor(JD,ESAPHODEI_DEIMOS,XYZ);}

void get_europa_parent_coordsv(double JD, double* XYZ) {get_europa_parent_coords(JD, &XYZ[0], &XYZ[1], &XYZ[2]);}
void get_callisto_parent_coordsv(double JD, double* XYZ) {get_callisto_parent_coords(JD, &XYZ[0], &XYZ[1], &XYZ[2]);}
void get_io_parent_coordsv(double JD, double* XYZ) {get_io_parent_coords(JD, &XYZ[0], &XYZ[1], &XYZ[2]);}
void get_ganymede_parent_coordsv(double JD, double* XYZ) {get_ganymede_parent_coords(JD, &XYZ[0], &XYZ[1], &XYZ[2]);}

void get_mimas_parent_coordsv(double JD, double* XYZ)
  {GetTass17Coor(JD,TASS17_MIMAS,XYZ);}
void get_enceladus_parent_coordsv(double JD, double* XYZ)
  {GetTass17Coor(JD,TASS17_ENCELADUS,XYZ);}
void get_tethys_parent_coordsv(double JD, double* XYZ)
  {GetTass17Coor(JD,TASS17_TETHYS,XYZ);}
void get_dione_parent_coordsv(double JD, double* XYZ)
  {GetTass17Coor(JD,TASS17_DIONE,XYZ);}
void get_rhea_parent_coordsv(double JD, double* XYZ)
  {GetTass17Coor(JD,TASS17_RHEA,XYZ);}
void get_titan_parent_coordsv(double JD, double* XYZ)
  {GetTass17Coor(JD,TASS17_TITAN,XYZ);}
void get_hyperion_parent_coordsv(double JD, double* XYZ)
  {GetTass17Coor(JD,TASS17_HYPERION,XYZ);}
void get_iapetus_parent_coordsv(double JD, double* XYZ)
  {GetTass17Coor(JD,TASS17_IAPETUS,XYZ);}

void get_miranda_parent_coordsv(double JD, double* XYZ)
  {GetGust86Coor(JD,GUST86_MIRANDA,XYZ);}
void get_ariel_parent_coordsv(double JD, double* XYZ)
  {GetGust86Coor(JD,GUST86_ARIEL,XYZ);}
void get_umbriel_parent_coordsv(double JD, double* XYZ)
  {GetGust86Coor(JD,GUST86_UMBRIEL,XYZ);}
void get_titania_parent_coordsv(double JD, double* XYZ)
  {GetGust86Coor(JD,GUST86_TITANIA,XYZ);}
void get_oberon_parent_coordsv(double JD, double* XYZ)
  {GetGust86Coor(JD,GUST86_OBERON,XYZ);}

void get_sun_helio_coordsv(double JD, double* XYZ) {XYZ[0]=0.; XYZ[1]=0.; XYZ[2]=0.;}

#ifdef __cplusplus
};
#endif


#endif /* _STELLPLANET_H_ */

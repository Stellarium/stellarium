#include "vsop87.h"
#include "elp82b.h"
#include "marssat.h"
#include "l1.h"
#include "tass17.h"
#include "gust86.h"

/* Chapter 31 Pg 206-207 Equ 31.1 31.2 , 31.3 using VSOP 87
 * Calculate planets rectangular heliocentric ecliptical coordinates
 * for given julian day. Values are in AU.
 * params : Julian day, rect coords */
void get_pluto_helio_coords(double jd, double * X, double * Y, double * Z);

void get_pluto_helio_coordsv(double jd,double xyz[3], void* unused)
  {get_pluto_helio_coords(jd, &xyz[0], &xyz[1], &xyz[2]);}


/* Return 0 of course... */
void get_sun_helio_coordsv(double jd,double xyz[3], void* unused)
  {xyz[0]=0.; xyz[1]=0.; xyz[2]=0.;}

void get_mercury_helio_coordsv(double jd,double xyz[3], void* unused)
  {GetVsop87Coor(jd,VSOP87_MERCURY,xyz);}
void get_venus_helio_coordsv(double jd,double xyz[3], void* unused)
  {GetVsop87Coor(jd,VSOP87_VENUS,xyz);}

void get_earth_helio_coordsv(const double jd,double xyz[3]) {
  double moon[3];
  GetVsop87Coor(jd,VSOP87_EMB,xyz);
  GetElp82bCoor(jd,moon);
    /* Earth != EMB:
       0.0121505677733761 = mu_m/(1+mu_m),
       mu_m = mass(moon)/mass(earth) = 0.01230002 */
  xyz[0] -= 0.0121505677733761 * moon[0];
  xyz[1] -= 0.0121505677733761 * moon[1];
  xyz[2] -= 0.0121505677733761 * moon[2];
}

void get_mars_helio_coordsv(double jd,double xyz[3], void* unused)
  {GetVsop87Coor(jd,VSOP87_MARS,xyz);}
void get_jupiter_helio_coordsv(double jd,double xyz[3], void* unused)
  {GetVsop87Coor(jd,VSOP87_JUPITER,xyz);}
void get_saturn_helio_coordsv(double jd,double xyz[3], void* unused)
  {GetVsop87Coor(jd,VSOP87_SATURN,xyz);}
void get_uranus_helio_coordsv(double jd,double xyz[3], void* unused)
  {GetVsop87Coor(jd,VSOP87_URANUS,xyz);}
void get_neptune_helio_coordsv(double jd,double xyz[3], void* unused)
  {GetVsop87Coor(jd,VSOP87_NEPTUNE,xyz);}

void get_mercury_helio_osculating_coords(double jd0,double jd,double xyz[3])
  {GetVsop87OsculatingCoor(jd0,jd,VSOP87_MERCURY,xyz);}
void get_venus_helio_osculating_coords(double jd0,double jd,double xyz[3])
  {GetVsop87OsculatingCoor(jd0,jd,VSOP87_VENUS,xyz);}
void get_earth_helio_osculating_coords(double jd0,double jd,double xyz[3])
  {GetVsop87OsculatingCoor(jd0,jd,VSOP87_EMB,xyz);}
void get_mars_helio_osculating_coords(double jd0,double jd,double xyz[3])
  {GetVsop87OsculatingCoor(jd0,jd,VSOP87_MARS,xyz);}
void get_jupiter_helio_osculating_coords(double jd0,double jd,double xyz[3])
  {GetVsop87OsculatingCoor(jd0,jd,VSOP87_JUPITER,xyz);}
void get_saturn_helio_osculating_coords(double jd0,double jd,double xyz[3])
  {GetVsop87OsculatingCoor(jd0,jd,VSOP87_SATURN,xyz);}
void get_uranus_helio_osculating_coords(double jd0,double jd,double xyz[3])
  {GetVsop87OsculatingCoor(jd0,jd,VSOP87_URANUS,xyz);}
void get_neptune_helio_osculating_coords(double jd0,double jd,double xyz[3])
  {GetVsop87OsculatingCoor(jd0,jd,VSOP87_NEPTUNE,xyz);}

/* Calculate the rectangular geocentric lunar coordinates to the inertial mean
 * ecliptic and equinox of J2000.
 * The geocentric coordinates returned are in units of AU.
 * This function is based upon the Lunar Solution ELP2000-82B by
 * Michelle Chapront-Touze and Jean Chapront of the Bureau des Longitudes,
 * Paris. ELP 2000-82B theory
 * param jd Julian day, rect pos */
void get_lunar_parent_coordsv(double jd,double xyz[3], void* unused)
  {GetElp82bCoor(jd,xyz);}

void get_phobos_parent_coordsv(double jd,double xyz[3], void* unused)
  {GetMarsSatCoor(jd,MARS_SAT_PHOBOS,xyz);}
void get_deimos_parent_coordsv(double jd,double xyz[3], void* unused)
  {GetMarsSatCoor(jd,MARS_SAT_DEIMOS,xyz);}

void get_io_parent_coordsv(double jd,double xyz[3], void* unused)
  {GetL1Coor(jd,L1_IO,xyz);}
void get_europa_parent_coordsv(double jd,double xyz[3], void* unused)
  {GetL1Coor(jd,L1_EUROPA,xyz);}
void get_ganymede_parent_coordsv(double jd,double xyz[3], void* unused)
  {GetL1Coor(jd,L1_GANYMEDE,xyz);}
void get_callisto_parent_coordsv(double jd,double xyz[3], void* unused)
  {GetL1Coor(jd,L1_CALLISTO,xyz);}

void get_mimas_parent_coordsv(double jd,double xyz[3], void* unused)
  {GetTass17Coor(jd,TASS17_MIMAS,xyz);}
void get_enceladus_parent_coordsv(double jd,double xyz[3], void* unused)
  {GetTass17Coor(jd,TASS17_ENCELADUS,xyz);}
void get_tethys_parent_coordsv(double jd,double xyz[3], void* unused)
  {GetTass17Coor(jd,TASS17_TETHYS,xyz);}
void get_dione_parent_coordsv(double jd,double xyz[3], void* unused)
  {GetTass17Coor(jd,TASS17_DIONE,xyz);}
void get_rhea_parent_coordsv(double jd,double xyz[3], void* unused)
  {GetTass17Coor(jd,TASS17_RHEA,xyz);}
void get_titan_parent_coordsv(double jd,double xyz[3], void* unused)
  {GetTass17Coor(jd,TASS17_TITAN,xyz);}
void get_hyperion_parent_coordsv(double jd,double xyz[3], void* unused)
  {GetTass17Coor(jd,TASS17_HYPERION,xyz);}
void get_iapetus_parent_coordsv(double jd,double xyz[3], void* unused)
  {GetTass17Coor(jd,TASS17_IAPETUS,xyz);}

void get_miranda_parent_coordsv(double jd,double xyz[3], void* unused)
  {GetGust86Coor(jd,GUST86_MIRANDA,xyz);}
void get_ariel_parent_coordsv(double jd,double xyz[3], void* unused)
  {GetGust86Coor(jd,GUST86_ARIEL,xyz);}
void get_umbriel_parent_coordsv(double jd,double xyz[3], void* unused)
  {GetGust86Coor(jd,GUST86_UMBRIEL,xyz);}
void get_titania_parent_coordsv(double jd,double xyz[3], void* unused)
  {GetGust86Coor(jd,GUST86_TITANIA,xyz);}
void get_oberon_parent_coordsv(double jd,double xyz[3], void* unused)
  {GetGust86Coor(jd,GUST86_OBERON,xyz);}


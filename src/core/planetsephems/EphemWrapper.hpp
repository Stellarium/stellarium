/*
Copyright (C) 2003 Fabien Chereau
Copyright (C) 2015 Holger Niessner
Copyright (C) 2015 Georg Zotti

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
Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
*/

/*
 * This class provides a wrapper to multiple methods to calculate ephemerides.
 * Depending on availability of extra data files, the class uses:
 * - VSOP87 and ELP82B
 * - DE430
 * - DE431
 *
 * Extending the old stellplanet-class, this updated version now
 * includes access to DE430 and DE431 for a more accurate, yet storage-space intensive solution.
 */

#ifndef EPHEMWRAPPER_HPP
#define EPHEMWRAPPER_HPP

#define DE430_FILENAME  "linux_p1550p2650.430"
#define DE431_FILENAME  "lnxm13000p17000.431"

class EphemWrapper{
public:
    static void init_de430(const char* filepath);
    static void init_de431(const char* filepath);
    static bool jd_fits_de430(const double jd);
    static bool jd_fits_de431(const double jd);
    static bool use_de430(const double jd);
    static bool use_de431(const double jd);
};

// These functions have an unused void pointer to be compatible to PosFuncType in SolarSystem and Planet classes.
void get_sun_helio_coordsv(double jd,double xyz[3], double xyzdot[3], void*);
void get_mercury_helio_coordsv(double jd,double xyz[3], double xyzdot[3], void*);
void get_venus_helio_coordsv(double jd,double xyz[3], double xyzdot[3], void*);
void get_earth_helio_coordsv(double jd,double xyz[3], double xyzdot[3], void*);
void get_mars_helio_coordsv(double jd,double xyz[3], double xyzdot[3], void*);
void get_jupiter_helio_coordsv(double jd,double xyz[3], double xyzdot[3], void*);
void get_saturn_helio_coordsv(double jd,double xyz[3], double xyzdot[3], void*);
void get_uranus_helio_coordsv(double jd,double xyz[3], double xyzdot[3], void*);
void get_neptune_helio_coordsv(double jd,double xyz[3], double xyzdot[3], void*);
void get_pluto_helio_coordsv(double jd,double xyz[3], double xyzdot[3], void*);

void get_mercury_helio_osculating_coords(double jd0,double jd,double xyz[3], double xyzdot[3]);
void get_venus_helio_osculating_coords(double jd0,double jd,double xyz[3], double xyzdot[3]);
void get_earth_helio_osculating_coords(double jd0,double jd,double xyz[3], double xyzdot[3]);
void get_mars_helio_osculating_coords(double jd0,double jd,double xyz[3], double xyzdot[3]);
void get_jupiter_helio_osculating_coords(double jd0,double jd,double xyz[3], double xyzdot[3]);
void get_saturn_helio_osculating_coords(double jd0,double jd,double xyz[3], double xyzdot[3]);
void get_uranus_helio_osculating_coords(double jd0,double jd,double xyz[3], double xyzdot[3]);
void get_neptune_helio_osculating_coords(double jd0,double jd,double xyz[3], double xyzdot[3]);
void get_pluto_helio_osculating_coords(double jd0,double jd,double xyz[3], double xyzdot[3]);

void get_lunar_parent_coordsv(double jde, double xyz[3], double xyzdot[3], void*);

void get_phobos_parent_coordsv(double jd,double xyz[3], double xyzdot[3], void*);
void get_deimos_parent_coordsv(double jd,double xyz[3], double xyzdot[3], void*);

void get_io_parent_coordsv(double jd,double xyz[3], double xyzdot[3], void*);
void get_europa_parent_coordsv(double jd,double xyz[3], double xyzdot[3], void*);
void get_ganymede_parent_coordsv(double jd,double xyz[3], double xyzdot[3], void*);
void get_callisto_parent_coordsv(double jd,double xyz[3], double xyzdot[3], void*);

void get_mimas_parent_coordsv(double jd,double xyz[3], double xyzdot[3], void*);
void get_enceladus_parent_coordsv(double jd,double xyz[3], double xyzdot[3], void*);
void get_tethys_parent_coordsv(double jd,double xyz[3], double xyzdot[3], void*);
void get_dione_parent_coordsv(double jd,double xyz[3], double xyzdot[3], void*);
void get_rhea_parent_coordsv(double jd,double xyz[3], double xyzdot[3], void*);
void get_titan_parent_coordsv(double jd,double xyz[3], double xyzdot[3], void*);
void get_hyperion_parent_coordsv(double jd,double xyz[3], double xyzdot[3], void*);
void get_iapetus_parent_coordsv(double jd,double xyz[3], double xyzdot[3], void*);

void get_miranda_parent_coordsv(double jd,double xyz[3], double xyzdot[3], void*);
void get_ariel_parent_coordsv(double jd,double xyz[3], double xyzdot[3], void*);
void get_umbriel_parent_coordsv(double jd,double xyz[3], double xyzdot[3], void*);
void get_titania_parent_coordsv(double jd,double xyz[3], double xyzdot[3], void*);
void get_oberon_parent_coordsv(double jd,double xyz[3], double xyzdot[3], void*);

#endif // _EPHEMWRAPPER_HPP_

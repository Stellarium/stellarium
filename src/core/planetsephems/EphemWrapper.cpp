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

#include "EphemWrapper.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "vsop87.h"
#include "elp82b.h"
#include "marssat.h"
#include "l12.h"
#include "tass17.h"
#include "gust86.h"
#include "de431.hpp"
#include "de430.hpp"
#include "pluto.h"

#define EPHEM_MERCURY_ID  0
#define EPHEM_VENUS_ID    1
#define EPHEM_EMB_ID      2
#define EPHEM_MARS_ID     3
#define EPHEM_JUPITER_ID  4
#define EPHEM_SATURN_ID   5
#define EPHEM_URANUS_ID   6
#define EPHEM_NEPTUNE_ID  7
#define EPHEM_PLUTO_ID    8

// GZ No idea what IMD stands for?
//#define EPHEM_IMD_EARTH_ID 2
#define EPHEM_JPL_EARTH_ID 3
#define EPHEM_JPL_PLUTO_ID 9
#define EPHEM_JPL_MOON_ID 10
#define EPHEM_JPL_SUN_ID  11

/**   JPL PLANET ID LIST
**            1 = mercury           8 = neptune                             **
**            2 = venus             9 = pluto                               **
**            3 = earth            10 = moon                                **
**            4 = mars             11 = sun                                 **
**            5 = jupiter          12 = solar-system barycenter             **
**            6 = saturn           13 = earth-moon barycenter               **
**            7 = uranus 
**/

void EphemWrapper::init_de430(const char* filepath)
{
	InitDE430(filepath);
}

void EphemWrapper::init_de431(const char* filepath)
{
	InitDE431(filepath);
}

bool EphemWrapper::jd_fits_de431(const double jd)
{
	//Correct limits found via jpl_get_double(). Limits hardcoded to avoid calls each time.
	//return !(jd < -3027215.5 || jd > 7930192.5);
	//This limits inside those where sun can jump between ecliptic of date and ecliptic2000.
	// We lose a month in -13000 and a few months in +17000, this should not matter.
	return ((jd > -3027188.25 ) && (jd < 7930056.87916));
}

bool EphemWrapper::jd_fits_de430(const double jd)
{
	return ((jd > 2287184.5) && (jd < 2688976.5));
}

bool EphemWrapper::use_de430(const double jd)
{
	return StelApp::getInstance().getCore()->de430IsActive() && EphemWrapper::jd_fits_de430(jd);
}

bool EphemWrapper::use_de431(const double jd)
{
	return StelApp::getInstance().getCore()->de431IsActive() && EphemWrapper::jd_fits_de431(jd);
}

// planet_id is ONLY one of the #defined values 0..8 above.
void get_planet_helio_coordsv(const double jd, double xyz[3], double xyzdot[3], const int planet_id)
{
	bool deOk=false;
	double xyz6[6];
	if(!std::isfinite(jd))
	{
		qDebug() << "get_planet_helio_coordsv(): SKIPPED CoordCalc, jd is infinite/nan: " << jd;
		return;
	}

	if(EphemWrapper::use_de430(jd))
	{
		deOk=GetDe430Coor(jd, planet_id + 1, xyz6);
	}
	else if(EphemWrapper::use_de431(jd))
	{
		deOk=GetDe431Coor(jd, planet_id + 1, xyz6);
	}
	if (!deOk) //VSOP87 as fallback
	{
		GetVsop87Coor(jd, planet_id, xyz6);
	}
	xyz[0]   =xyz6[0]; xyz[1]   =xyz6[1]; xyz[2]   =xyz6[2];
	xyzdot[0]=xyz6[3]; xyzdot[1]=xyz6[4]; xyzdot[2]=xyz6[5];
}

// Osculating positions for time JDE in elements for JDE0, if possible by the theory used (e.g. VSOP87).
// For ephemerides like DE4xx, JDE0 is irrelevant.
void get_planet_helio_osculating_coordsv(double jd0, double jd, double xyz[3], double xyzdot[3], int planet_id)
{
	bool deOk=false;
	double xyz6[6];
	if(!(std::isfinite(jd) && std::isfinite(jd0)))
	{
		qDebug() << "get_planet_helio_osculating_coordsv(): SKIPPED CoordCalc, jd0 or jd is infinite/nan. jd0:" << jd0 << "jd: "<< jd;
		return;
	}

	if(EphemWrapper::use_de430(jd))
	{
		deOk=GetDe430Coor(jd, planet_id + 1, xyz6);
	}
	else if(EphemWrapper::use_de431(jd))
	{
		deOk=GetDe431Coor(jd, planet_id + 1, xyz6);
	}
	if (!deOk) //VSOP87 as fallback
	{
		GetVsop87OsculatingCoor(jd0, jd, planet_id, xyz6);
	}
	xyz[0]   =xyz6[0]; xyz[1]   =xyz6[1]; xyz[2]   =xyz6[2];
	xyzdot[0]=xyz6[3]; xyzdot[1]=xyz6[4]; xyzdot[2]=xyz6[5];
}

/* Chapter 31 Pg 206-207 Equ 31.1 31.2, 31.3 using VSOP 87
 * Calculate Pluto's rectangular heliocentric ecliptical coordinates
 * for given Julian Day. Values are in AU.
 * params : Julian day, rect coords */

void get_pluto_helio_coordsv(double jd,double xyz[3], double xyzdot[3], void* unused)
{
	Q_UNUSED(unused);
	bool deOk=false;
	double xyz6[6];
	if(!std::isfinite(jd))
	{
		qDebug() << "get_pluto_helio_coordsv(): SKIPPED PlutoCoordCalc, jd is infinite/nan:" << jd;
		return;
	}

	if(EphemWrapper::use_de430(jd))
	{
		deOk=GetDe430Coor(jd, EPHEM_JPL_PLUTO_ID, xyz6);
	}
	else if(EphemWrapper::use_de431(jd))
	{
		deOk=GetDe431Coor(jd, EPHEM_JPL_PLUTO_ID, xyz6);
	}

	if (deOk)
	{
		xyz[0]   =xyz6[0]; xyz[1]   =xyz6[1]; xyz[2]   =xyz6[2];
		xyzdot[0]=xyz6[3]; xyzdot[1]=xyz6[4]; xyzdot[2]=xyz6[5];
	}
	else	// fallback to previous solution
	{
		get_pluto_helio_coords(jd, &xyz[0], &xyz[1], &xyz[2]);
		xyzdot[0]=xyzdot[1]=xyzdot[2]=0.0; // TODO: Some meaningful way to get speed?
	}
}

/* Return 0 for the sun */
void get_sun_helio_coordsv(double jd,double xyz[3], double xyzdot[3], void* unused)
{
	Q_UNUSED(jd);
	Q_UNUSED(unused);
	xyz[0]   =0.; xyz[1]   =0.; xyz[2]   =0.;
	xyzdot[0]=0.; xyzdot[1]=0.; xyzdot[2]=0.;
}

void get_mercury_helio_coordsv(double jd,double xyz[3], double xyzdot[3], void* unused)
{
	Q_UNUSED(unused);
	get_planet_helio_coordsv(jd, xyz, xyzdot, EPHEM_MERCURY_ID);
}
void get_venus_helio_coordsv(double jd,double xyz[3], double xyzdot[3], void* unused)
{
	Q_UNUSED(unused);
	get_planet_helio_coordsv(jd, xyz, xyzdot, EPHEM_VENUS_ID);
}

void get_earth_helio_coordsv(const double jd,double xyz[3], double xyzdot[3], void* unused)
{
	Q_UNUSED(unused);
	bool deOk=false;
	double xyz6[6];
	if(!std::isfinite(jd))
	{
		qDebug() << "get_earth_helio_coordsv(): SKIPPED EarthCoordCalc, jd is infinite/nan:" << jd;
		return;
	}

	if(EphemWrapper::use_de430(jd))
	{
		deOk=GetDe430Coor(jd, EPHEM_JPL_EARTH_ID, xyz6);
	}
	else if(EphemWrapper::use_de431(jd))
	{
		deOk=GetDe431Coor(jd, EPHEM_JPL_EARTH_ID, xyz6);
	}
	if (!deOk) //VSOP87 as fallback
	{
		double moon[3];
		GetVsop87Coor(jd,EPHEM_EMB_ID,xyz6);
		GetElp82bCoor(jd,moon);
		/* Earth != EMB:
	0.0121505677733761 = mu_m/(1+mu_m),
	mu_m = mass(moon)/mass(earth) = 0.01230002 */
		xyz6[0] -= 0.0121505677733761 * moon[0];
		xyz6[1] -= 0.0121505677733761 * moon[1];
		xyz6[2] -= 0.0121505677733761 * moon[2];
		// TODO: HOW TO FIX EARTH SPEED?
	}
	xyz[0]   =xyz6[0]; xyz[1]   =xyz6[1]; xyz[2]   =xyz6[2];
	xyzdot[0]=xyz6[3]; xyzdot[1]=xyz6[4]; xyzdot[2]=xyz6[5];
}

void get_mars_helio_coordsv(double jd,double xyz[3], double xyzdot[3], void* unused)
{
	Q_UNUSED(unused);
	get_planet_helio_coordsv(jd, xyz, xyzdot, EPHEM_MARS_ID);
}

void get_jupiter_helio_coordsv(double jd,double xyz[3], double xyzdot[3], void* unused)
{
	Q_UNUSED(unused);
	get_planet_helio_coordsv(jd, xyz, xyzdot, EPHEM_JUPITER_ID);
}

void get_saturn_helio_coordsv(double jd,double xyz[3], double xyzdot[3], void* unused)
{
	Q_UNUSED(unused);
	get_planet_helio_coordsv(jd, xyz, xyzdot, EPHEM_SATURN_ID);
}

void get_uranus_helio_coordsv(double jd,double xyz[3], double xyzdot[3], void* unused)
{
	Q_UNUSED(unused);
	get_planet_helio_coordsv(jd, xyz, xyzdot, EPHEM_URANUS_ID);
}

void get_neptune_helio_coordsv(double jd,double xyz[3], double xyzdot[3], void* unused)
{
	Q_UNUSED(unused);
	get_planet_helio_coordsv(jd, xyz, xyzdot, EPHEM_NEPTUNE_ID);
}

void get_mercury_helio_osculating_coords(double jd0,double jd,double xyz[3], double xyzdot[3])
{
	get_planet_helio_osculating_coordsv(jd0, jd, xyz, xyzdot, EPHEM_MERCURY_ID);
}

void get_venus_helio_osculating_coords(double jd0,double jd,double xyz[3], double xyzdot[3])
{
	get_planet_helio_osculating_coordsv(jd0, jd, xyz, xyzdot, EPHEM_VENUS_ID);
}

void get_earth_helio_osculating_coords(double jd0,double jd,double xyz[3], double xyzdot[3])
{
	get_planet_helio_osculating_coordsv(jd0, jd, xyz, xyzdot, EPHEM_EMB_ID);
}

void get_mars_helio_osculating_coords(double jd0,double jd,double xyz[3], double xyzdot[3])
{
	get_planet_helio_osculating_coordsv(jd0, jd, xyz, xyzdot, EPHEM_MARS_ID);
}

void get_jupiter_helio_osculating_coords(double jd0,double jd,double xyz[3], double xyzdot[3])
{
	get_planet_helio_osculating_coordsv(jd0, jd, xyz, xyzdot, EPHEM_JUPITER_ID);
}

void get_saturn_helio_osculating_coords(double jd0,double jd,double xyz[3], double xyzdot[3])
{
	get_planet_helio_osculating_coordsv(jd0, jd, xyz, xyzdot, EPHEM_SATURN_ID);
}

void get_uranus_helio_osculating_coords(double jd0,double jd,double xyz[3], double xyzdot[3])
{
	get_planet_helio_osculating_coordsv(jd0, jd, xyz, xyzdot, EPHEM_URANUS_ID);
}

void get_neptune_helio_osculating_coords(double jd0,double jd,double xyz[3], double xyzdot[3])
{
	get_planet_helio_osculating_coordsv(jd0, jd, xyz, xyzdot, EPHEM_NEPTUNE_ID);
}

/* Calculate the rectangular geocentric lunar coordinates to the inertial mean
 * ecliptic and equinox of J2000.
 * The geocentric coordinates returned are in units of AU.
 * This function is based upon the Lunar Solution ELP2000-82B by
 * Michelle Chapront-Touze and Jean Chapront of the Bureau des Longitudes,
 * Paris. ELP 2000-82B theory
 * param jd Julian day, rect pos */
void get_lunar_parent_coordsv(double jde, double xyz[3], double xyzdot[3], void* unused)
{
	Q_UNUSED(unused);
	double xyz6[6];
	bool deOk=false;
	if(EphemWrapper::use_de430(jde))
		deOk=GetDe430Coor(jde, EPHEM_JPL_MOON_ID, xyz6, EPHEM_JPL_EARTH_ID);
	else if(EphemWrapper::use_de431(jde))
		deOk=GetDe431Coor(jde, EPHEM_JPL_MOON_ID, xyz6, EPHEM_JPL_EARTH_ID);

	if (deOk)
	{
		xyz[0]   =xyz6[0]; xyz[1]   =xyz6[1]; xyz[2]   =xyz6[2];
		xyzdot[0]=xyz6[3]; xyzdot[1]=xyz6[4]; xyzdot[2]=xyz6[5];
	}
	else
	{  // fallback to DE-less solution.
		GetElp82bCoor(jde,xyz);
		xyzdot[0]=xyzdot[1]=xyzdot[2]=0.0; // TODO: Some meaningful way to get speed?
	}
}

void get_phobos_parent_coordsv(double jd, double xyz[3], double xyzdot[3], void* unused)
{
	Q_UNUSED(unused);
	GetMarsSatCoor(jd, MARS_SAT_PHOBOS, xyz, xyzdot);
}

void get_deimos_parent_coordsv(double jd, double xyz[3], double xyzdot[3], void* unused)
{
	Q_UNUSED(unused);
	GetMarsSatCoor(jd, MARS_SAT_DEIMOS, xyz, xyzdot);
}

void get_io_parent_coordsv(double jd, double xyz[3], double xyzdot[3], void* unused)
{
	Q_UNUSED(unused);
	GetL12Coor(jd, L12_IO, xyz, xyzdot);
}

void get_europa_parent_coordsv(double jd, double xyz[3], double xyzdot[3], void* unused)
{
	Q_UNUSED(unused);
	GetL12Coor(jd, L12_EUROPA, xyz, xyzdot);
}

void get_ganymede_parent_coordsv(double jd, double xyz[3], double xyzdot[3], void* unused)
{
	Q_UNUSED(unused);
	GetL12Coor(jd, L12_GANYMEDE, xyz, xyzdot);
}

void get_callisto_parent_coordsv(double jd, double xyz[3], double xyzdot[3], void* unused)
{
	Q_UNUSED(unused);
	GetL12Coor(jd, L12_CALLISTO, xyz, xyzdot);
}

void get_mimas_parent_coordsv(double jd, double xyz[3], double xyzdot[3], void* unused)
{ 
	Q_UNUSED(unused);
	GetTass17Coor(jd, TASS17_MIMAS, xyz, xyzdot);
}

void get_enceladus_parent_coordsv(double jd, double xyz[3], double xyzdot[3], void* unused)
{
	Q_UNUSED(unused);
	GetTass17Coor(jd, TASS17_ENCELADUS, xyz, xyzdot);
}

void get_tethys_parent_coordsv(double jd, double xyz[3], double xyzdot[3], void* unused)
{ 
	Q_UNUSED(unused);
	GetTass17Coor(jd, TASS17_TETHYS, xyz, xyzdot);
}

void get_dione_parent_coordsv(double jd, double xyz[3], double xyzdot[3], void* unused)
{ 
	Q_UNUSED(unused);
	GetTass17Coor(jd, TASS17_DIONE, xyz, xyzdot);
}

void get_rhea_parent_coordsv(double jd, double xyz[3], double xyzdot[3], void* unused)
{ 
	Q_UNUSED(unused);
	GetTass17Coor(jd, TASS17_RHEA, xyz, xyzdot);
}

void get_titan_parent_coordsv(double jd, double xyz[3], double xyzdot[3], void* unused)
{ 
	Q_UNUSED(unused);
	GetTass17Coor(jd, TASS17_TITAN, xyz, xyzdot);
}

void get_hyperion_parent_coordsv(double jd, double xyz[3], double xyzdot[3], void* unused)
{ 
	Q_UNUSED(unused);
	GetTass17Coor(jd, TASS17_HYPERION, xyz, xyzdot);
}

void get_iapetus_parent_coordsv(double jd, double xyz[3], double xyzdot[3], void* unused)
{ 
	Q_UNUSED(unused);
	GetTass17Coor(jd, TASS17_IAPETUS, xyz, xyzdot);
}

void get_miranda_parent_coordsv(double jd, double xyz[3], double xyzdot[3], void* unused)
{
	Q_UNUSED(unused);
	GetGust86Coor(jd, GUST86_MIRANDA, xyz, xyzdot);
}

void get_ariel_parent_coordsv(double jd, double xyz[3], double xyzdot[3], void* unused)
{
	Q_UNUSED(unused);
	GetGust86Coor(jd, GUST86_ARIEL, xyz, xyzdot);
}

void get_umbriel_parent_coordsv(double jd, double xyz[3], double xyzdot[3], void* unused)
{
	Q_UNUSED(unused);
	GetGust86Coor(jd, GUST86_UMBRIEL, xyz, xyzdot);
}

void get_titania_parent_coordsv(double jd, double xyz[3], double xyzdot[3], void* unused)
{
	Q_UNUSED(unused);
	GetGust86Coor(jd, GUST86_TITANIA, xyz, xyzdot);
}

void get_oberon_parent_coordsv(double jd, double xyz[3], double xyzdot[3], void* unused)
{
	Q_UNUSED(unused);
	GetGust86Coor(jd, GUST86_OBERON, xyz, xyzdot);
}

#if 0

/* GZ: The functions provide non-generic axis rotations from
 * B.A. Archinal et al.: Report of the IAU Working Group on Cartographic Coordinates and Rotational Elements: 2009
 * DOI 10.1007/s10569-010-9320-4
 * Many objects just have a simple standard model, ra0+offset*T etc.
 * ALL these functions are JDE-->raDeg, decDeg, rotDeg, i.e. degree values for ICRF pole target and rotational angle.
 */
// From Table 1:
void get_mercury_axis_rot(const double jde, double *raDeg, double *decDeg, double *rotDeg)
{
	const double d=(jde-2451545.0);
	const double T=d * (1/36525.0);
	// The constants from the paper should be made into radians by the compiler!
	const double M1=(174.791086*M_PI/180.0) + ( 4.092335*M_PI/180.0)*d;
	const double M2=(349.582171*M_PI/180.0) + ( 8.184670*M_PI/180.0)*d;
	const double M3=(164.373257*M_PI/180.0) + (12.277005*M_PI/180.0)*d;
	const double M4=(339.164343*M_PI/180.0) + (16.369340*M_PI/180.0)*d;
	const double M5=(153.955429*M_PI/180.0) + (20.461675*M_PI/180.0)*d;
	*raDeg=281.0097 - 0.0328*T;
	*decDeg=61.4143 - 0.0049*T;
	*rotDeg= -0.00000535*sin(M5)
		-0.00002364*sin(M4)
		-0.00010280*sin(M3)
		-0.00104581*sin(M2)
		-0.00993822*sin(M1)
		+6.1385025*d + 329.5469;
}

void get_jupiter_axis_rot(const double jde, double *raDeg, double *decDeg, double *rotDeg1, double *rotDeg2, double *rotDeg3)
{
	const double d=(jde-2451545.0);
	const double T=d * (1/36525.0);
	// The constants from the paper should be made into radians by the compiler!
	const double Ja = ( 99.360714*M_PI/180.0) + (4850.4046*M_PI/180.0)*T;
	const double Jb = (175.895369*M_PI/180.0) + (1191.9605*M_PI/180.0)*T;
	const double Jc = (300.323162*M_PI/180.0) + ( 262.5475*M_PI/180.0)*T;
	const double Jd = (114.012305*M_PI/180.0) + (6070.2476*M_PI/180.0)*T;
	const double Je = ( 49.511251*M_PI/180.0) + (  64.3000*M_PI/180.0)*T;
	*raDeg = 268.056595 - 0.006499*T + 0.000117*sin(Ja) + 0.000938*sin(Jb)+0.001432*sin(Jc) + 0.000030*sin(Jd) + 0.002150*sin(Je);
	*decDeg = 64.495303 + 0.002413*T + 0.000050*cos(Ja) + 0.000404*cos(Jb)+0.000617*cos(Jc) - 0.000013*cos(Jd) + 0.000926*cos(Je);
	*rotDeg1 =  67.1  + 877.900*d;
	*rotDeg3 =  43.3  + 870.270*d;
	*rotDeg3 = 284.95 + 870.5360000*d;
}

void get_neptune_axis_rot(const double jde, double *raDeg, double *decDeg, double *rotDeg)
{
	const double d=(jde-2451545.0);
	const double T=d * (1/36525.0);
	const double N=357.85*M_PI/180.0+(52.316*M_PI/180.0)*T;
	*raDeg = 299.36 + 0.70*sin(N);
	*decDeg = 43.46 - 0.51*cos(N);
	*rotDeg = 253.18 + 536.3128492*d - 0.48*sin(N);
}

// Table 2:
void get_moon_axis_rot(const double jde, double *raDeg, double *decDeg, double *rotDeg)
{
	const double d=(jde-2451545.0);
	const double T=d * (1/36525.0);
	// The constants from the paper should be made into radians by the compiler!
	const double E1 = (125.045*M_PI/180.0) - ( 0.0529921*M_PI/180.0)*d;
	const double E2 = (250.089*M_PI/180.0) - ( 0.1059842*M_PI/180.0)*d;
	const double E3 = (260.008*M_PI/180.0) + (13.0120009*M_PI/180.0)*d;
	const double E4 = (176.625*M_PI/180.0) + (13.3407154*M_PI/180.0)*d;
	const double E5 = (357.529*M_PI/180.0) + ( 0.9856003*M_PI/180.0)*d;
	const double E6 = (311.589*M_PI/180.0) + (26.4057084*M_PI/180.0)*d;
	const double E7 = (134.963*M_PI/180.0) + (13.0649930*M_PI/180.0)*d;
	const double E8 = (276.617*M_PI/180.0) + ( 0.3287146*M_PI/180.0)*d;
	const double E9 = ( 34.226*M_PI/180.0) + ( 1.7484877*M_PI/180.0)*d;
	const double E10= ( 15.134*M_PI/180.0) - ( 0.1589763*M_PI/180.0)*d;
	const double E11= (119.743*M_PI/180.0) + ( 0.0036096*M_PI/180.0)*d;
	const double E12= (239.961*M_PI/180.0) + ( 0.1643573*M_PI/180.0)*d;
	const double E13= ( 25.053*M_PI/180.0) + (12.9590088*M_PI/180.0)*d;
	*raDeg = 269.9949 + 0.0031*T - 3.8787*sin(E1) - 0.1204*sin(E2)
		+0.0700*sin(E3) - 0.0172*sin(E4) + 0.0072*sin(E6)-0.0052*sin(E10) + 0.0043*sin(E13);
	*decDeg = 66.5392 + 0.0130*T + 1.5419*cos(E1) + 0.0239*cos(E2)-0.0278*cos(E3) + 0.0068*cos(E4)
			 - 0.0029*cos(E6) + 0.0009*cos(E7) + 0.0008*cos(E10) - 0.0009*cos(E13);
	*rotDeg = 38.3213 + 13.17635815*d - 1.4e-12 *d*d + 3.5610*sin(E1)
		+0.1208*sin(E2)  - 0.0642*sin(E3)  + 0.0158*sin(E4)
		+0.0252*sin(E5)  - 0.0066*sin(E6)  - 0.0047*sin(E7)
		-0.0046*sin(E8)  + 0.0028*sin(E9)  + 0.0052*sin(E10)
		+0.0040*sin(E11) + 0.0019*sin(E12) - 0.0044*sin(E13);
}

void get_deimos_axis_rot(const double jde, double *raDeg, double *decDeg, double *rotDeg)
{
	const double d=(jde-2451545.0);
	const double T=d * (1/36525.0);
	const double M1 = (169.51*M_PI/180.0) - (   0.4357640*M_PI/180.0)*d;
	const double M2 = (192.93*M_PI/180.0) + (1128.4096700*M_PI/180.0)*d + (8.864*M_PI/180.0)*T*T;
	const double M3 = ( 53.47*M_PI/180.0) - (   0.0181510*M_PI/180.0)*d;
	*raDeg = 316.65 -   0.108*T + 2.98*sin(M3);
	*decDeg = 53.52 -   0.061*T - 1.78*cos(M3);
	*rotDeg = 79.41 + 285.1618970*d - 0.520*T*T - 2.58*sin(M3) + 0.19*cos(M3);
}

void get_phobos_axis_rot(const double jde, double *raDeg, double *decDeg, double *rotDeg)
{
	const double d=(jde-2451545.0);
	const double T=d * (1/36525.0);
	const double M1 = (169.51*M_PI/180.0) - (   0.4357640*M_PI/180.0)*d;
	const double M2 = (192.93*M_PI/180.0) + (1128.4096700*M_PI/180.0)*d + (8.864*M_PI/180.0)*T*T;
	const double M3 = ( 53.47*M_PI/180.0) - (   0.0181510*M_PI/180.0)*d;
	*raDeg = 317.68 - 0.108*T + 1.79*sin(M1);
	*decDeg = 52.90 - 0.061*T - 1.08*cos(M1);
	*rotDeg = 35.06 + 1128.8445850*d + 8.864*T*T - 1.42*sin(M1) - 0.78*sin(M2);
}

static double Jrot[9]; // auxiliary values for Jupiter. Actually element0 keeps T of computation.
static void computeJupRotTerms(const double T)
{
	if (abs(T-Jrot[0])> 0.001)
	{
		Jrot[0] = T;
		Jrot[1] =  (73.32*M_PI/180.0) + (91472.9*M_PI/180.0)*T;
		Jrot[2] =  (24.62*M_PI/180.0) + (45137.2*M_PI/180.0)*T;
		Jrot[3] = (283.90*M_PI/180.0) + ( 4850.7*M_PI/180.0)*T;
		Jrot[4] = (355.80*M_PI/180.0) + ( 1191.3*M_PI/180.0)*T;
		Jrot[5] = (119.90*M_PI/180.0) + (  262.1*M_PI/180.0)*T;
		Jrot[6] = (229.80*M_PI/180.0) + (   64.3*M_PI/180.0)*T;
		Jrot[7] = (352.25*M_PI/180.0) + ( 2382.6*M_PI/180.0)*T;
		Jrot[8] = (113.35*M_PI/180.0) + ( 6070.0*M_PI/180.0)*T;
	}
}

void get_amalthea_axis_rot(const double jde, double *raDeg, double *decDeg, double *rotDeg)
{
	const double d=(jde-2451545.0);
	const double T=d * (1/36525.0);
	computeJupRotTerms(T);
	*raDeg = 268.05 - 0.009*T - 0.84*sin(Jrot[1]) + 0.01*sin(2*Jrot[1]);
	*decDeg = 64.49 + 0.003*T - 0.36*cos(Jrot[1]);
	*rotDeg = 231.67 + 722.6314560*d + 0.76*sin(Jrot[1]) - 0.01*sin(2*Jrot[1]);
}

void get_thebe_axis_rot(const double jde, double *raDeg, double *decDeg, double *rotDeg)
{
	const double d=(jde-2451545.0);
	const double T=d * (1/36525.0);
	computeJupRotTerms(T);
	*raDeg = 268.05 - 0.009*T - 2.11*sin(Jrot[2]) + 0.04*sin(2*Jrot[2]);
	*decDeg = 64.49 + 0.003*T - 0.91*cos(Jrot[2]) + 0.01*cos(2*Jrot[2]);
	*rotDeg =  8.56 + 533.7004100*d + 1.91*sin(Jrot[2]) - 0.04*sin(2*Jrot[2]);
}

void get_io_axis_rot(const double jde, double *raDeg, double *decDeg, double *rotDeg)
{
	const double d=(jde-2451545.0);
	const double T=d * (1/36525.0);
	computeJupRotTerms(T);
	*raDeg =  268.05 - 0.009*T + 0.094*sin(Jrot[3]) + 0.024*sin(Jrot[4]);
	*decDeg =  64.50 + 0.003*T + 0.040*cos(Jrot[3]) + 0.011*cos(Jrot[4]);
	*rotDeg = 200.39 + 203.4889538*d - 0.085*sin(Jrot[3]) - 0.022*sin(Jrot[4]);
}

void get_europa_axis_rot(const double jde, double *raDeg, double *decDeg, double *rotDeg)
{
	const double d=(jde-2451545.0);
	const double T=d * (1/36525.0);
	computeJupRotTerms(T);
	*raDeg = 268.08  -   0.009*T + 1.086*sin(Jrot[4]) + 0.060*sin(Jrot[5]) + 0.015*sin(Jrot[6]) + 0.009*sin(Jrot[7]);
	*decDeg = 64.51  +   0.003*T + 0.468*cos(Jrot[4]) + 0.026*cos(Jrot[5]) + 0.007*cos(Jrot[6]) + 0.002*cos(Jrot[7]);
	*rotDeg = 36.022 + 101.3747235*d - 0.980*sin(Jrot[4]) - 0.054*sin(Jrot[5]) - 0.014*sin(Jrot[6]) - 0.008*sin(Jrot[7]);
}

void get_ganymede_axis_rot(const double jde, double *raDeg, double *decDeg, double *rotDeg)
{
	const double d=(jde-2451545.0);
	const double T=d * (1/36525.0);
	computeJupRotTerms(T);
	*raDeg = 268.20  - 0.009*T - 0.037*sin(Jrot[4]) + 0.431*sin(Jrot[5]) + 0.091*sin(Jrot[6]);
	*decDeg = 64.57  + 0.003*T - 0.016*cos(Jrot[4]) + 0.186*cos(Jrot[5]) + 0.039*cos(Jrot[6]);
	*rotDeg = 44.064 + 50.3176081*d + 0.033*sin(Jrot[4]) - 0.389*sin(Jrot[5]) - 0.082*sin(Jrot[6]);
}

void get_callisto_axis_rot(const double jde, double *raDeg, double *decDeg, double *rotDeg)
{
	const double d=(jde-2451545.0);
	const double T=d * (1/36525.0);
	computeJupRotTerms(T);
	*raDeg = 268.72 - 0.009*T - 0.068*sin(Jrot[5]) + 0.590*sin(Jrot[6]) + 0.010*sin(Jrot[8]);
	*decDeg = 64.83 + 0.003*T - 0.029*cos(Jrot[5]) + 0.254*cos(Jrot[6]) - 0.004*cos(Jrot[8]);
	*rotDeg = 259.51 + 21.5710715*d + 0.061*sin(Jrot[5]) - 0.533*sin(Jrot[6]) - 0.009*sin(Jrot[8]);
}

static double Srot[7]; // auxiliary values for Saturn. Actually element0 keeps T of last computation.
static void computeSatRotTerms(const double T)
{
	if (abs(T-Srot[0])> 0.001)
	{
		Srot[0] = T;
		Srot[1] = (353.32*M_PI/180.0) + (75706.7*M_PI/180.0)*T;
		Srot[2] = ( 28.72*M_PI/180.0) + (75706.7*M_PI/180.0)*T;
		Srot[3] = (177.40*M_PI/180.0) - (36505.5*M_PI/180.0)*T;
		Srot[4] = (300.00*M_PI/180.0) - ( 7225.9*M_PI/180.0)*T;
		Srot[5] = (316.45*M_PI/180.0) + (  506.2*M_PI/180.0)*T;
		Srot[6] = (345.20*M_PI/180.0) - ( 1016.3*M_PI/180.0)*T;
	}
}

void get_epimetheus_axis_rot(const double jde, double *raDeg, double *decDeg, double *rotDeg)
{
	const double d=(jde-2451545.0);
	const double T=d * (1/36525.0);
	computeSatRotTerms(T);
	*raDeg  =  40.58 - 0.036*T - 3.153*sin(Srot[1]) + 0.086*sin(2*Srot[1]);
	*decDeg =  83.52 - 0.004*T - 0.356*cos(Srot[1]) + 0.005*cos(2*Srot[1]);
	*rotDeg = 293.87 + 518.4907239*d + 3.133*sin(Srot[1]) - 0.086*sin(2.0*Srot[1]);
}

void get_janus_axis_rot(const double jde, double *raDeg, double *decDeg, double *rotDeg)
{
	const double d=(jde-2451545.0);
	const double T=d * (1/36525.0);
	computeSatRotTerms(T);
	*raDeg = 40.58 - 0.036*T - 1.623*sin(Srot[2]) + 0.023*sin(2.0*Srot[2]);
	*decDeg = 83.52 - 0.004*T - 0.183*cos(Srot[2]) + 0.001*cos(2-0*Srot[2]);
	*rotDeg = 58.83 + 518.2359876*d + 1.613*sin(Srot[2]) - 0.023*sin(2.0*Srot[2]);
}

void get_mimas_axis_rot(const double jde, double *raDeg, double *decDeg, double *rotDeg)
{
	const double d=(jde-2451545.0);
	const double T=d * (1/36525.0);
	computeSatRotTerms(T);
	*raDeg  =  40.66 - 0.036*T + 13.56*sin(Srot[3]);
	*decDeg =  83.52 - 0.004*T - 1.53*cos(Srot[3]);
	*rotDeg = 333.46 + 381.9945550*d - 13.48*sin(Srot[3]) - 44.85*sin(Srot[5]);
}

void get_tethys_axis_rot(const double jde, double *raDeg, double *decDeg, double *rotDeg)
{
	const double d=(jde-2451545.0);
	const double T=d * (1/36525.0);
	computeSatRotTerms(T);
	*raDeg  = 40.66 - 0.036*T + 9.66*sin(Srot[4]);
	*decDeg = 83.52 - 0.004*T - 1.09*cos(Srot[4]);
	*rotDeg =  8.95 + 190.6979085*d - 9.60*sin(Srot[4]) + 2.23*sin(Srot[5]);
}

void get_rhea_axis_rot(const double jde, double *raDeg, double *decDeg, double *rotDeg)
{
	const double d=(jde-2451545.0);
	const double T=d * (1/36525.0);
	computeSatRotTerms(T);
	*raDeg  =  40.38 - 0.036*T + 3.10*sin(Srot[6]);
	*decDeg =  83.55 - 0.004*T - 0.35*cos(Srot[6]);
	*rotDeg = 235.16 + 79.6900478*d - 3.08*sin(Srot[6]);
}

static double Urot[17]; // auxiliary values for Uranus. Actually element0 keeps T of last computation.
static void computeUraRotTerms(const double T)
{
	if (abs(T-Urot[0])> 0.001)
	{
		Urot[0] = T;
		Urot[1]  = (115.75*M_PI/180.0) + (54991.87*M_PI/180.0)*T;
		Urot[2]  = (141.69*M_PI/180.0) + (41887.66*M_PI/180.0)*T;
		Urot[3]  = (135.03*M_PI/180.0) + (29927.35*M_PI/180.0)*T;
		Urot[4]  = ( 61.77*M_PI/180.0) + (25733.59*M_PI/180.0)*T;
		Urot[5]  = (249.32*M_PI/180.0) + (24471.46*M_PI/180.0)*T;
		Urot[6]  = ( 43.86*M_PI/180.0) + (22278.41*M_PI/180.0)*T;
		Urot[7]  = ( 77.66*M_PI/180.0) + (20289.42*M_PI/180.0)*T;
		Urot[8]  = (157.36*M_PI/180.0) + (16652.76*M_PI/180.0)*T;
		Urot[9]  = (101.81*M_PI/180.0) + (12872.63*M_PI/180.0)*T;
		Urot[10] = (138.64*M_PI/180.0) + ( 8061.81*M_PI/180.0)*T;
		Urot[11] = (102.23*M_PI/180.0) - ( 2024.22*M_PI/180.0)*T;
		Urot[12] = (316.41*M_PI/180.0) + ( 2863.96*M_PI/180.0)*T;
		Urot[13] = (304.01*M_PI/180.0) - (   51.94*M_PI/180.0)*T;
		Urot[14] = (308.71*M_PI/180.0) - (   93.17*M_PI/180.0)*T;
		Urot[15] = (340.82*M_PI/180.0) - (   75.32*M_PI/180.0)*T;
		Urot[16] = (259.14*M_PI/180.0) - (  504.81*M_PI/180.0)*T;
	}
}

void get_cordelia_axis_rot(const double jde, double *raDeg, double *decDeg, double *rotDeg)
{
	const double d=(jde-2451545.0);
	const double T=d * (1/36525.0);
	computeUraRotTerms(T);
	*raDeg  = 257.31 - 0.15*sin(Urot[1]);
	*decDeg = -15.18 + 0.14*cos(Urot[1]);
	*rotDeg = 127.69 - 1074.5205730*d - 0.04*sin(Urot[1]);
}

void get_ophelia_axis_rot(const double jde, double *raDeg, double *decDeg, double *rotDeg)
{
	const double d=(jde-2451545.0);
	const double T=d * (1/36525.0);
	computeUraRotTerms(T);
	*raDeg  = 257.31 - 0.09*sin(Urot[2]);
	*decDeg = -15.18 + 0.09*cos(Urot[2]);
	*rotDeg = 130.35 - 956.4068150*d - 0.03*sin(Urot[2]);
}
void get_bianca_axis_rot(const double jde, double *raDeg, double *decDeg, double *rotDeg)
{
	const double d=(jde-2451545.0);
	const double T=d * (1/36525.0);
	computeUraRotTerms(T);
	*raDeg  = 257.31 - 0.16*sin(Urot[3]);
	*decDeg = -15.18 + 0.16*cos(Urot[3]);
	*rotDeg = 105.46 - 828.3914760*d - 0.04*sin(Urot[3]);
}
void get_cressida_axis_rot(const double jde, double *raDeg, double *decDeg, double *rotDeg)
{
	const double d=(jde-2451545.0);
	const double T=d * (1/36525.0);
	computeUraRotTerms(T);
	*raDeg  = 257.31 - 0.04*sin(Urot[4]);
	*decDeg = -15.18 + 0.04*cos(Urot[4]);
	*rotDeg =  59.16 - 776.5816320*d - 0.01*sin(Urot[4]);
}
void get_desdemona_axis_rot(const double jde, double *raDeg, double *decDeg, double *rotDeg)
{
	const double d=(jde-2451545.0);
	const double T=d * (1/36525.0);
	computeUraRotTerms(T);
	*raDeg  = 257.31 - 0.17*sin(Urot[5]);
	*decDeg = -15.18 + 0.16*cos(Urot[5]);
	*rotDeg =  95.08 - 760.0531690*d - 0.04*sin(Urot[5]);
}
void get_juliet_axis_rot(const double jde, double *raDeg, double *decDeg, double *rotDeg)
{
	const double d=(jde-2451545.0);
	const double T=d * (1/36525.0);
	computeUraRotTerms(T);
	*raDeg  = 257.31 - 0.06*sin(Urot[6]);
	*decDeg = -15.18 + 0.06*cos(Urot[6]);
	*rotDeg = 302.56 - 730.1253660*d - 0.02*sin(Urot[6]);
}
void get_portia_axis_rot(const double jde, double *raDeg, double *decDeg, double *rotDeg)
{
	const double d=(jde-2451545.0);
	const double T=d * (1/36525.0);
	computeUraRotTerms(T);
	*raDeg  = 257.31 - 0.09*sin(Urot[7]);
	*decDeg = -15.18 + 0.09*cos(Urot[7]);
	*rotDeg =  25.03 - 701.4865870*d - 0.02*sin(Urot[7]);
}
void get_rosalind_axis_rot(const double jde, double *raDeg, double *decDeg, double *rotDeg)
{
	const double d=(jde-2451545.0);
	const double T=d * (1/36525.0);
	computeUraRotTerms(T);
	*raDeg  = 257.31 - 0.29*sin(Urot[8]);
	*decDeg = -15.18 + 0.28*cos(Urot[8]);
	*rotDeg = 314.90 - 644.6311260*d - 0.08*sin(Urot[8]);
}
void get_belinda_axis_rot(const double jde, double *raDeg, double *decDeg, double *rotDeg)
{
	const double d=(jde-2451545.0);
	const double T=d * (1/36525.0);
	computeUraRotTerms(T);
	*raDeg  = 257.31 - 0.03*sin(Urot[9]);
	*decDeg = -15.18 + 0.03*cos(Urot[9]);
	*rotDeg = 297.46 - 577.3628170*d - 0.01*sin(Urot[9]);
}
void get_puck_axis_rot(const double jde, double *raDeg, double *decDeg, double *rotDeg)
{
	const double d=(jde-2451545.0);
	const double T=d * (1/36525.0);
	computeUraRotTerms(T);
	*raDeg  = 257.31 - 0.33*sin(Urot[10]);
	*decDeg = -15.18 + 0.31*cos(Urot[10]);
	*rotDeg =  91.24 - 472.5450690*d - 0.09*sin(Urot[10]);
}
void get_miranda_axis_rot(const double jde, double *raDeg, double *decDeg, double *rotDeg)
{
	const double d=(jde-2451545.0);
	const double T=d * (1/36525.0);
	computeUraRotTerms(T);
	*raDeg  = 257.43 + 4.41*sin(Urot[11]) - 0.04*sin(2.0*Urot[11]);
	*decDeg = -15.08 + 4.25*cos(Urot[11]) - 0.02*cos(2.0*Urot[11]);
	*rotDeg =  30.70 - 254.6906892*d - 1.27*sin(Urot[12]) + 0.15*sin(2.0*Urot[12]) + 1.15*sin(Urot[11]) - 0.09*sin(2.*Urot[11]);
}
void get_ariel_axis_rot(const double jde, double *raDeg, double *decDeg, double *rotDeg)
{
	const double d=(jde-2451545.0);
	const double T=d * (1/36525.0);
	computeUraRotTerms(T);
	*raDeg  = 257.43 + 0.29*sin(Urot[13]);
	*decDeg = -15.10 + 0.28*cos(Urot[13]);
	*rotDeg = 156.22 - 142.8356681*d + 0.05*sin(Urot[12]) + 0.08*sin(Urot[13]);
}
void get_umbriel_axis_rot(const double jde, double *raDeg, double *decDeg, double *rotDeg)
{
	const double d=(jde-2451545.0);
	const double T=d * (1/36525.0);
	computeUraRotTerms(T);
	*raDeg  = 257.43 + 0.21*sin(Urot[14]);
	*decDeg = -15.10 + 0.20*cos(Urot[14]);
	*rotDeg = 108.05 - 86.8688923*d - 0.09*sin(Urot[12]) + 0.06*sin(Urot[14]);
}
void get_titania_axis_rot(const double jde, double *raDeg, double *decDeg, double *rotDeg)
{
	const double d=(jde-2451545.0);
	const double T=d * (1/36525.0);
	computeUraRotTerms(T);
	*raDeg  = 257.43 + 0.29*sin(Urot[15]);
	*decDeg = -15.10 + 0.28*cos(Urot[15]);
	*rotDeg =  77.74 - 41.3514316*d + 0.08*sin(Urot[15]);
}
void get_oberon_axis_rot(const double jde, double *raDeg, double *decDeg, double *rotDeg)
{
	const double d=(jde-2451545.0);
	const double T=d * (1/36525.0);
	computeUraRotTerms(T);
	*raDeg  = 257.43 + 0.16*sin(Urot[16]);
	*decDeg = -15.10 + 0.16*cos(Urot[16]);
	*rotDeg =   6.77 - 26.7394932*d + 0.04*sin(Urot[16]);
}

static double Nrot[9]; // auxiliary values for Neptune. Actually element0 keeps T of last computation. The element "N" is stored in Nrot[8]
static void computeNepRotTerms(const double T)
{
	if (abs(T-Nrot[0])> 0.001)
	{
		Nrot[0] = T;
		Nrot[1] = (323.92*M_PI/180.0) + (62606.6  *M_PI/180.0)*T;
		Nrot[2] = (220.51*M_PI/180.0) + (55064.2  *M_PI/180.0)*T;
		Nrot[3] = (354.27*M_PI/180.0) + (46564.5  *M_PI/180.0)*T;
		Nrot[4] = ( 75.31*M_PI/180.0) + (26109.4  *M_PI/180.0)*T;
		Nrot[5] = ( 35.36*M_PI/180.0) + (14325.4  *M_PI/180.0)*T;
		Nrot[6] = (142.61*M_PI/180.0) + ( 2824.6  *M_PI/180.0)*T;
		Nrot[7] = (177.85*M_PI/180.0) + (   52.316*M_PI/180.0)*T;
		Nrot[8] = (357.85*M_PI/180.0) + (   52.316*M_PI/180.0)*T; // called N in the paper.
	}
}

void get_naiad_axis_rot(const double jde, double *raDeg, double *decDeg, double *rotDeg)
{
	const double d=(jde-2451545.0);
	const double T=d * (1/36525.0);
	computeNepRotTerms(T);
	*raDeg = 299.36 + 0.70*sin(Nrot[8]) - 6.49*sin(Nrot[1]) + 0.25*sin(2.0*Nrot[1]);
	*decDeg = 43.36 - 0.51*cos(Nrot[8]) - 4.75*cos(Nrot[1]) + 0.09*cos(2.0*Nrot[1]);
	*rotDeg = 254.06 + 1222.8441209*d - 0.48*sin(Nrot[8]) + 4.40*sin(Nrot[1]) - 0.27*sin(2.0*Nrot[1]);
}

void get_thalassa_axis_rot(const double jde, double *raDeg, double *decDeg, double *rotDeg)
{
	const double d=(jde-2451545.0);
	const double T=d * (1/36525.0);
	computeNepRotTerms(T);
	*raDeg  = 299.36 + 0.70*sin(Nrot[8]) - 0.28*sin(Nrot[2]);
	*decDeg =  43.45 - 0.51*cos(Nrot[8]) - 0.21*cos(Nrot[2]);
	*rotDeg = 102.06 + 1155.7555612*d - 0.48*sin(Nrot[8]) + 0.19*sin(Nrot[2]);
}

void get_despina_axis_rot(const double jde, double *raDeg, double *decDeg, double *rotDeg)
{
	const double d=(jde-2451545.0);
	const double T=d * (1/36525.0);
	computeNepRotTerms(T);
	*raDeg  = 299.36 + 0.70*sin(Nrot[8]) - 0.09*sin(Nrot[3]);
	*decDeg =  43.45 - 0.51*cos(Nrot[8]) - 0.07*cos(Nrot[3]);
	*rotDeg = 306.51 + 1075.7341562*d - 0.49*sin(Nrot[8]) + 0.06*sin(Nrot[3]);
}

void get_galatea_axis_rot(const double jde, double *raDeg, double *decDeg, double *rotDeg)
{
	const double d=(jde-2451545.0);
	const double T=d * (1/36525.0);
	computeNepRotTerms(T);
	*raDeg  = 299.36 + 0.70*sin(Nrot[8]) - 0.07*sin(Nrot[4]);
	*decDeg =  43.43 - 0.51*cos(Nrot[8]) - 0.05*cos(Nrot[4]);
	*rotDeg = 258.09 + 839.6597686*d - 0.48*sin(Nrot[8]) + 0.05*sin(Nrot[4]);
}

void get_larissa_axis_rot(const double jde, double *raDeg, double *decDeg, double *rotDeg)
{
	const double d=(jde-2451545.0);
	const double T=d * (1/36525.0);
	computeNepRotTerms(T);
	*raDeg  = 299.36 + 0.70*sin(Nrot[8]) - 0.27*sin(Nrot[5]);
	*decDeg =  43.41 - 0.51*cos(Nrot[8]) - 0.20*cos(Nrot[5]);
	*rotDeg = 179.41 + 649.0534470*d - 0.48*sin(Nrot[8]) + 0.19*sin(Nrot[5]);
}

void get_proteus_axis_rot(const double jde, double *raDeg, double *decDeg, double *rotDeg)
{
	const double d=(jde-2451545.0);
	const double T=d * (1/36525.0);
	computeNepRotTerms(T);
	*raDeg  = 299.27 + 0.70*sin(Nrot[8]) - 0.05*sin(Nrot[6]);
	*decDeg =  42.91 - 0.51*cos(Nrot[8]) - 0.04*cos(Nrot[6]);
	*rotDeg =  93.38 + 320.7654228*d - 0.48*sin(Nrot[8]) + 0.04*sin(Nrot[6]);
}

void get_triton_axis_rot(const double jde, double *raDeg, double *decDeg, double *rotDeg)
{
	const double d=(jde-2451545.0);
	const double T=d * (1/36525.0);
	computeNepRotTerms(T);
	*raDeg = 299.36 - 32.35*sin(Nrot[7]) - 6.28*sin(2.0*Nrot[7]) - 2.08*sin(3.0*Nrot[7])
		- 0.74*sin(4.0*Nrot[7]) - 0.28*sin(5.0*Nrot[7]) - 0.11*sin(6.0*Nrot[7])
		- 0.07*sin(7.0*Nrot[7]) - 0.02*sin(8.0*Nrot[7]) - 0.01*sin(9.0*Nrot[7]);
	*decDeg = 41.17 + 22.55*cos(Nrot[7]) + 2.10*cos(2.0*Nrot[7]) + 0.55*cos(3.0*Nrot[7])
		+ 0.16*cos(4.0*Nrot[7]) + 0.05*cos(5.0*Nrot[7]) + 0.02*cos(6.0*Nrot[7]) + 0.01*cos(7.0*Nrot[7]);
	*rotDeg = 296.53 - 61.2572637*d + 22.25*sin(Nrot[7]) + 6.73*sin(2.0*Nrot[7])
		+ 2.05*sin(3.0*Nrot[7]) + 0.74*sin(4.0*Nrot[7]) + 0.28*sin(5.0*Nrot[7])
		+ 0.11*sin(6.0*Nrot[7]) + 0.05*sin(7.0*Nrot[7]) + 0.02*sin(8.0*Nrot[7])
		+ 0.01*sin(9.0*Nrot[7]);
}
#endif


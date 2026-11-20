/*
 * Stellarium
 * Copyright (C) 2026 Sylvain Simard
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */

#ifndef HORIZONSSITE_HPP
#define HORIZONSSITE_HPP

#include "HorizonsZones.hpp"

//! (SS) 2026-11-09 Position and velocity of an observer on the Earth surface (the "site") as JPL Horizons computes them, and the
//! sidereal times of the site. This is STEP 2b and STEP 7 of the standalone programs (sun_r1.cpp, mercury_r6.cpp, moon_r3.cpp):
//!
//!   ITRS (WGS84 geodetic -> geocentric)  ->  polar motion Rpom(xp, yp, s')  ->  TIRS  ->  Rz(-GAST)  ->  true equator of date
//!   ->  transpose of rnpSite  ->  J2000 (ICRS).
//!
//! GAST = GMST 1982 (with UT1 = UTC + UT1-UTC) + equation of the equinoxes made with dPsi of the IAU 1980 nutation + EOP dPsi, but
//! without the empirical JPL offsets (see HorizonsPrecNut::Result::dpsiST). The velocity is omega x r in the true equator of date,
//! omega = 7.2921151467064e-5 (1 - LOD/86400) rad/s, to be added to the Earth velocity for the diurnal aberration.
//!
//! The TIME ZONE rules are those of the standalone program: in zones 1 and 2 (before 1962-01-20) there are no EOP, JD(UT) is UT1 and
//! polar motion is the identity, whatever the EOP values of the parameters are. In zones 3 to 5 the caller gives the EOP of the date:
//! interpolated in zone 3, the last values in zones 4 and 5 (the same rule as HorizonsPrecNut for dPsi and dEps).
//!
//! All lengths are in metres and velocities in m/s: the caller converts them to AU with the AU of JPL Horizons.
//! This class does not use StelApp, StelCore or any other Stellarium singleton (unit testable).
class HorizonsSite
{
public:
	struct Params
	{
		// time and precession-nutation: same meaning as HorizonsPrecNut::Params
		double jdUT = 0.;        //!< JD(UT1) before 1962-01-20, JD(UTC) after (StelCore::getJD() with the JPL Horizons Delta-T)
		double jdTDB = 0.;
		double jdTT = 0.;
		double zone4Start = 0.;
		bool useNutation = true; //!< false: precession only, sidereal time without equation of the equinoxes (mean sidereal time)
		double eopDpsi = 0.;     //!< [arcsec] see HorizonsPrecNut::Params
		double eopDeps = 0.;
		bool forMoon = false;
		// Earth orientation (zones 3 to 5 only, ignored before)
		double ut1utc = 0.;      //!< UT1-UTC [s]
		double xp = 0., yp = 0.; //!< polar motion [arcsec]
		double lod = 0.;         //!< length of day excess [s]
		// site
		double lonRad = 0.;      //!< geodetic longitude, east positive [rad]
		double latRad = 0.;      //!< geodetic latitude [rad]
		double heightM = 0.;     //!< height above the WGS84 ellipsoid [m]
	};

	struct Result
	{
		bool ok = false;            //!< false outside of the range of Horizons or when the site is not valid
		HorizonsZones::Zone zone = HorizonsZones::Zone::None;
		double ut1utc = 0., xp = 0., yp = 0., lod = 0.;   //!< Earth orientation actually used (0 in zones 1 and 2)
		double gmst = 0.;           //!< Greenwich mean sidereal time 1982 [rad, 0..2pi]
		double gast = 0.;           //!< Greenwich apparent sidereal time [rad]
		double last = 0.;           //!< local apparent sidereal time, no polar motion: GAST + longitude [rad]
		double lastPM = 0.;         //!< local apparent sidereal time of the site with polar motion [rad] (the one for hour angles)
		double rItrs[3] = {0,0,0};  //!< site in ITRS, geocentric [m]
		double rTod[3] = {0,0,0};   //!< site in the true equator and equinox of date (with polar motion) [m]
		double rJ2000[3] = {0,0,0}; //!< site in J2000 [m]
		double vTod[3] = {0,0,0};   //!< site velocity in the true equator of date [m/s]
		double vJ2000[3] = {0,0,0}; //!< site velocity in J2000 [m/s]
		double rpom[3][3] = {{1,0,0},{0,1,0},{0,0,1}};      //!< polar motion matrix (ERFA convention)
		double rSite[3][3] = {{1,0,0},{0,1,0},{0,0,1}};     //!< rnpSite, J2000 -> true equator of date (ERFA convention)
	};

	static Result compute(const Params& p);

	//! Apparent local hour angle (west positive, as Horizons) and airless azimuth and elevation of a direction p3 given in the true
	//! equator and equinox of date (STEP 7 of the standalone programs). @param lastPM Result::lastPM [rad], @param latRad geodetic latitude.
	//! Azimuth from North through East, all angles in radians. ha in -pi..pi.
	static void hourAngleAzEl(double lastPM, const double p3[3], double latRad, double& ha, double& az, double& el);

	//! Equation of the equinoxes of the standalone program: dpsi cos(epsMean) + the 1994 complementary terms, with the given dPsi
	//! (eraEqeq94 only knows the model dPsi). tCy in Julian centuries from J2000. [rad]
	static double equationOfEquinoxes(double tCy, double dpsi, double epsMean);
};

#endif // HORIZONSSITE_HPP

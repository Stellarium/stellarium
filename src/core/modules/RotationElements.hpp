/*
 * Stellarium
 * Copyright (C) 2002 Fabien Chereau
 * Copyright (C) 2021 Georg Zotti
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

#ifndef ROTATIONELEMENTS_HPP
#define ROTATIONELEMENTS_HPP

//! Class used to store rotational elements, i.e. axis orientation for the planetary body.
//! Data are read from ssystem_(major|minor).ini in SolarSystem.cpp::loadPlanets().
//!
//! Notes of early 2021: The original planet axis model in Stellarium seems to be an outdated model untraceable in the literature.
//! Stellarium used to have elements w.r.t. VSOP87 (ecliptical) frame for planets, and elements w.r.t. the host planet's equator for planet moons:
//! ascendingNode
//! obliquity
//! offset
//! period
//! We cannot remove these old data as long as some Moon orbits for the outer planets depend on them.
//! The problem is that without a reference we have no idea whether any of these elements and algorithms are correct.
//!
//! IAU standards like the Report of the IAU Working Group on Cartographic Coordinates and Rotational elements 2009 (DOI:10.1007/s10569-010-9320-4)
//! has axes given w.r.t. J2000 ICRF.
//!
//! Between V0.15 and 0.20.*, if the planet elements in ssystem.ini had elements "rot_pole_ra" and "rot_pole_de" given, the poles were transformed to
//! ecliptically-based directions (VSOP87) obliquity/ascendingNode. But these were only ra/de_J2000 poles. Some axes have precession, which need to be modelled/updated.
//! The new way (0.21+): We still allow using the previous 4 elements in Planet::computeTransMatrix(.,.) and Planet::getSiderealTime(), but only if WGCCRE elements are not available.
//!
//! ra=ra0+T*ra1
//! de=de0+T*de1         ( --> obliquity, ascendingNode)
//! rot_rotation_offset [degrees]     =W0
//! rot_periode  [hours] =  computed from rot_pole_W1[deg/day] if that exists.  360 [deg] / _rot_ [hours] --> 360 * _rot_ / 24 [deg/hours]
//!
//! If rot_pole... values are given, then they are ICRF and transformed on the fly to VSOP87, stored in here.
//!
//! Since 0.21 we use the WGCCRE elements and orientations directly, so that axes behave properly.
//! In addition, the objects with more complicated element behaviour are updated with two special functions.
//! New keys in ssystem_*.ini, their storage in RotationElements, and their equivalents in the IAU report:
//! rot_pole_ra  [degrees]     re.ra0 [rad]         constant term for alpha_0
//! rot_pole_de  [degrees]     re.ra1 [rad/ct]      constant term for delta_0
//! rot_pole_ra1 [degrees/cy]  re.de0 [rad]         T factor for alpha_0
//! rot_pole_de1 [degrees/cy]  re.de1 [rad/ct]      T factor for delta_0
//! rot_pole_W0  [degrees]     re.W0  [degrees]     constant term fo W.
//! rot_pole_W1  [degrees/day] re.W1  [degrees/day] d factor for W

#include <QString>
#include "VecMath.hpp"

// epoch J2000: 12 UT on 1 Jan 2000
#define J2000 2451545.0

class RotationElements
{
public:
	enum ComputationMethod {
		Traditional, // Stellarium prior to 0.21, This is unfortunately not documented. Orbits and axes given w.r.t. parent axes.
		WGCCRE       // Orientation as described by the IAU Working Group on Cartographic Coordinates and Rotational Elements reports of 2009 or later
	};
	enum PlanetCorrection		// To be used as named argument for correction calculations regarding orientations of planetary axes.
	{				// See updatePlanetCorrections()
		EarthMoon=3,
		Mars=4,
		Jupiter=5,
		Saturn=6,
		Uranus=7,
		Neptune=8
	};

	RotationElements(void) : period(1.), offset(0.), epoch(J2000), obliquity(0.), ascendingNode(0.),
		method(Traditional), ra0(0.), ra1(0.), de0(0.), de1(0.), W0(0.), W1(0.),
		currentAxisRA(0.), currentAxisDE(0.), currentAxisW(0.) {}
	double period;          // [deprecated] (sidereal) rotation period [earth days]
	double offset;          // [deprecated] rotation at epoch  [degrees]
	double epoch;           // JDE (JD TT) of epoch for these elements
	double obliquity;       // [deprecated] tilt of rotation axis w.r.t. ecliptic [radians]
	double ascendingNode;   // [deprecated] long. of ascending node of equator on the ecliptic [radians]
	// new elements for 0.21+: The 6/9 new entries after the switch are enough for many objects. More corrections can be applied where required.
	ComputationMethod method; // The reference system in use for the respective object. WGCCRE is preferred, but we don't have all data.
	double ra0;            // [rad] RA_0 right ascension of north pole. ssystem.ini: rot_pole_ra    /180*M_PI
	double ra1;            // [rad/century] rate of change in axis ra   ssystem.ini: rot_pole_ra1   /180*M_PI
	double de0;            // [rad] DE_0 declination of north pole      ssystem.ini: rot_pole_de    /180*M_PI
	double de1;            // [rad/century] rate of change in axis de   ssystem.ini: rot_pole_de1   /180*M_PI
	// These values are only in the modern algorithms. invalid if ra0=0.
	double W0;             // [deg] mean longitude of prime meridian along equator measured from intersection with ICRS plane at epoch.
	double W1;             // [deg/d] mean longitude motion. W=W0+d*W1.
	double currentAxisRA;  // [rad] Mostly for infostring: RA=RA0+d*RA1(+corrections)
	double currentAxisDE;  // [rad] Mostly for infostring: DE=DE0+d*DE1(+corrections)
	double currentAxisW;   // [deg] Mostly for infostring: W =W0 +d*W1 (+corrections)

	//! 0.21+: Axes of planets and moons require terms depending on T=(jde-J2000)/36525, described in Explanatory Supplement 2013, Tables 10.1 and 10.10-14,
	//! updated in WGCCRE reports 2009 and 2015.
	//! Others require frequent updates, depending on jde-J2000. (Moon etc.)
	//! These should be updated as frequently as needed, optimally with the planet. Light time correction should be applied when needed.
	//! best place to call update is the SolarSystem::computePlanets()
	struct PlanetCorrections {
		double JDE_E; // keep record of when these values are valid: Earth
		double JDE_M; // keep record of when these values are valid: Mars
		double JDE_J; // keep record of when these values are valid: Jupiter
		double JDE_S; // keep record of when these values are valid: Saturn
		double JDE_U; // keep record of when these values are valid: Uranus
		double JDE_N; // keep record of when these values are valid: Neptune

		double E1; // Earth corrections. These are from WGCCRE2009.
		double E2;
		double E3;
		double E4;
		double E5;
		double E6;
		double E7;
		double E8;
		double E9;
		double E10;
		double E11;
		double E12;
		double E13;
		double M1;  // Mars corrections. These are from WGCCRE2015.
		double M2;
		double M3;
		double M4;
		double M5;
		double M6;
		double M7;
		double M8;
		double M9;
		double M10;
		double Ja; // Jupiter axis terms, Table 10.1
		double Jb;
		double Jc;
		double Jd;
		double Je;
		double Na; // Neptune axix term
		double J1; // corrective terms for Jupiter' moons, Table 10.10
		double J2;
		double J3;
		double J4;
		double J5;
		double J6;
		double J7;
		double J8;
		double S1; // corrective terms for Saturn's moons, Table 10.12
		double S2;
		double S3;
		double S4;
		double S5;
		double S6;
		double U1; // for Cordelia // corrective terms for Uranus's moons, Table 10.14.
		double U2; // for Ophelia
		double U3; // for Bianca   (not in 0.20)
		double U4; // for Cressida
		double U5; // for Desdemona
		double U6; // for Juliet
		double U7; // for Portia   (not in 0.20)
		double U8; // for Rosalind (not in 0.20)
		double U9; // for Belinda  (not in 0.20)
		double U10;// for Puck
		double U11;
		double U12;
		double U13;
		double U14;
		double U15;
		double U16;
		double N1; // corrective terms for Neptune's moons, Table 10.15 (N=Na!)
		double N2;
		double N3;
		double N4;
		double N5;
		double N6;
		double N7;
	};
	static PlanetCorrections planetCorrections;
	//! Update the planet corrections. planet=3(Moon), 4 (Mars), 5(Jupiter), 6(Saturn), 7(Uranus), 8(Neptune).
	//! The values are immediately converted to radians.
	static void updatePlanetCorrections(const double JDE, const PlanetCorrection planet);

	//! Finetuning correction functions. These functions are attached to the planets in Planet::setRotationElements()
	//! and called at the right places. This avoids ugly sequences of name query ifelses.
	//! arguments are d=JDE-J2000, T=d/36525, J2000NPoleRA, J2000NPoleDE
	typedef double (*axisRotFuncType)(const double, const double);
	typedef void   (*axisOriFuncType)(const double, const double, double*, double*);
	axisRotFuncType corrW;
	axisOriFuncType corrOri;
	static const QMap<QString, axisRotFuncType>axisRotCorrFuncMap;
	static const QMap<QString, axisOriFuncType>axisOriCorrFuncMap;

	//! These corr* functions can be used as corrW.
	static double corrWnil(const double d, const double T){Q_UNUSED(d) Q_UNUSED(T) return 0;}
	static double corrWMoon(const double d, const double T);
	static double corrWMercury(const double d, const double T);
	static double corrWMars(const double d, const double T);
	//! The default W delivers SystemII longitude.
	//! We have to shift by GRS position and texture position.
	//! The final value will no longer be W but the rotation value required to show the GRS.
	static double corrWJupiter(const double d, const double T);
	static double corrWNeptune(const double d, const double T);
	static double corrWPhobos(const double d, const double T);
	static double corrWDeimos(const double d, const double T);
	static double corrWIo(const double d, const double T);
	static double corrWEuropa(const double d, const double T);
	static double corrWGanymede(const double d, const double T);
	static double corrWCallisto(const double d, const double T);
	static double corrWAmalthea(const double d, const double T);
	static double corrWThebe(const double d, const double T);
	static double corrWMimas(const double d, const double T);
	static double corrWTethys(const double d, const double T);
	static double corrWRhea(const double d, const double T);
	static double corrWJanus(const double d, const double T);
	static double corrWEpimetheus(const double d, const double T);
	static double corrWCordelia(const double d, const double T);
	static double corrWOphelia(const double d, const double T);
	static double corrWBianca(const double d, const double T);
	static double corrWCressida(const double d, const double T);
	static double corrWDesdemona(const double d, const double T);
	static double corrWJuliet(const double d, const double T);
	static double corrWPortia(const double d, const double T);
	static double corrWRosalind(const double d, const double T);
	static double corrWBelinda(const double d, const double T);
	static double corrWPuck(const double d, const double T);
	static double corrWAriel(const double d, const double T);
	static double corrWUmbriel(const double d, const double T);
	static double corrWTitania(const double d, const double T);
	static double corrWOberon(const double d, const double T);
	static double corrWMiranda(const double d, const double T);
	static double corrWTriton(const double d, const double T);
	static double corrWNaiad(const double d, const double T);
	static double corrWThalassa(const double d, const double T);
	static double corrWDespina(const double d, const double T);
	static double corrWGalatea(const double d, const double T);
	static double corrWLarissa(const double d, const double T);
	static double corrWProteus(const double d, const double T);

	//! These functions can be used as corrOri.
	static void corrOriNil(const double, const double, double*, double*){} // Do nothing.
	static void corrOriMoon(const double d, const double T, double* J2000NPoleRA, double* J2000NPoleDE);
	static void corrOriMars(const double d, const double T, double* J2000NPoleRA, double* J2000NPoleDE);
	static void corrOriJupiter(const double d, const double T, double* J2000NPoleRA, double* J2000NPoleDE);
	static void corrOriNeptune(const double d, const double T, double* J2000NPoleRA, double* J2000NPoleDE);
	static void corrOriPhobos(const double d, const double T, double* J2000NPoleRA, double* J2000NPoleDE);
	static void corrOriDeimos(const double d, const double T, double* J2000NPoleRA, double* J2000NPoleDE);
	static void corrOriIo(const double d, const double T, double* J2000NPoleRA, double* J2000NPoleDE);
	static void corrOriEuropa(const double d, const double T, double* J2000NPoleRA, double* J2000NPoleDE);
	static void corrOriGanymede(const double d, const double T, double* J2000NPoleRA, double* J2000NPoleDE);
	static void corrOriCallisto(const double d, const double T, double* J2000NPoleRA, double* J2000NPoleDE);
	static void corrOriAmalthea(const double d, const double T, double* J2000NPoleRA, double* J2000NPoleDE);
	static void corrOriThebe(const double d, const double T, double* J2000NPoleRA, double* J2000NPoleDE);
	static void corrOriMimas(const double d, const double T, double* J2000NPoleRA, double* J2000NPoleDE);
	static void corrOriTethys(const double d, const double T, double* J2000NPoleRA, double* J2000NPoleDE);
	static void corrOriRhea(const double d, const double T, double* J2000NPoleRA, double* J2000NPoleDE);
	static void corrOriJanus(const double d, const double T, double* J2000NPoleRA, double* J2000NPoleDE);
	static void corrOriEpimetheus(const double d, const double T, double* J2000NPoleRA, double* J2000NPoleDE);
	static void corrOriAriel(const double d, const double T, double* J2000NPoleRA, double* J2000NPoleDE);
	static void corrOriUmbriel(const double d, const double T, double* J2000NPoleRA, double* J2000NPoleDE);
	static void corrOriTitania(const double d, const double T, double* J2000NPoleRA, double* J2000NPoleDE);
	static void corrOriOberon(const double d, const double T, double* J2000NPoleRA, double* J2000NPoleDE);
	static void corrOriMiranda(const double d, const double T, double* J2000NPoleRA, double* J2000NPoleDE);
	static void corrOriCordelia(const double d, const double T, double* J2000NPoleRA, double* J2000NPoleDE);
	static void corrOriOphelia(const double d, const double T, double* J2000NPoleRA, double* J2000NPoleDE);
	static void corrOriBianca(const double d, const double T, double* J2000NPoleRA, double* J2000NPoleDE);
	static void corrOriCressida(const double d, const double T, double* J2000NPoleRA, double* J2000NPoleDE);
	static void corrOriDesdemona(const double d, const double T, double* J2000NPoleRA, double* J2000NPoleDE);
	static void corrOriJuliet(const double d, const double T, double* J2000NPoleRA, double* J2000NPoleDE);
	static void corrOriPortia(const double d, const double T, double* J2000NPoleRA, double* J2000NPoleDE);
	static void corrOriRosalind(const double d, const double T, double* J2000NPoleRA, double* J2000NPoleDE);
	static void corrOriBelinda(const double d, const double T, double* J2000NPoleRA, double* J2000NPoleDE);
	static void corrOriPuck(const double d, const double T, double* J2000NPoleRA, double* J2000NPoleDE);
	static void corrOriTriton(const double d, const double T, double* J2000NPoleRA, double* J2000NPoleDE);
	static void corrOriNaiad(const double d, const double T, double* J2000NPoleRA, double* J2000NPoleDE);
	static void corrOriThalassa(const double d, const double T, double* J2000NPoleRA, double* J2000NPoleDE);
	static void corrOriDespina(const double d, const double T, double* J2000NPoleRA, double* J2000NPoleDE);
	static void corrOriGalatea(const double d, const double T, double* J2000NPoleRA, double* J2000NPoleDE);
	static void corrOriLarissa(const double d, const double T, double* J2000NPoleRA, double* J2000NPoleDE);
	static void corrOriProteus(const double d, const double T, double* J2000NPoleRA, double* J2000NPoleDE);

	static bool flagCustomGrsSettings;	//!< Use custom settings for calculation of position of Great Red Spot?
	static double customGrsJD;		//!< Initial JD (epoch) for calculation of position of Great Red Spot
	static double customGrsLongitude;	//!< Longitude of Great Red Spot at customGrsJD (System II, degrees)
	static double customGrsDrift;		//!< Annual drift of Great Red Spot position (degrees)

	//! Retrieve magnitude variation depending on angle Ls [radians].
	//! Source: A. Mallama: The magnitude and albedo of Mars. Icarus 192(2007) 404-416.
	//! @arg albedo true  to return longitudinal albedo correction, Ls=the average of sub-earth and sub-solar planetographic longitudes
	//!             false for the Orbital Longitude correction. Ls here is the apparent solar longitude along Mars' "ecliptic". (0=Martian Vernal Equinox)
	static double getMarsMagLs(const double Ls, const bool albedo);
private:
	static const QList<Vec3d> marsMagLs;
};

#endif // ROTATIONELEMENTS_HPP

// orbit.cpp
//
// Initial structure of orbit computation; EllipticalOrbit: Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
// CometOrbit, InitHyp,InitPar,InitEll,Init3D: Copyright (C) 1995,2007,2008 Johannes Gajdosik
// InitHyp,InitPar,InitEll,Init3D checked, improved, extended, annotated 2013 Georg Zotti (GZ).
// Combination to KeplerOrbit, GimbalOrbit (c) 2020 Georg Zotti
// Algorithms identified and extended from:
//    Meeus: Astronomical Algorithms 1998
//    Heafner: Fundamental Ephemeris Computations 1999
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#include "Solve.hpp"
#include "Orbit.hpp"
#include "StelUtils.hpp"

#include <functional>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <QDebug>

using namespace std;

#define EPSILON 1e-10
//#define EPSILON 1e-4
// Gaussian gravitation constant k, also used by Heafner, 5.3.12.
// From the definition (see https://de.wikipedia.org/wiki/Vis-Viva-Gleichung):
// Gaussian Grav. Constant k=sqrt(GM/1AU)=2pi AU/year ~ 0.0172020 AU/d ~ 29.8km/s.
// Here M is solar mass=1.
// For effects around moon orbits around planets, we must reduce the central body's mass to m/M=centralMass/1.
#define GAUSS_GRAV_k 0.01720209895
#define GAUSS_GRAV_k_SQ (GAUSS_GRAV_k*GAUSS_GRAV_k)

#if defined(_MSC_VER) && (_MSC_VER<1900)
// cuberoot is missing in older VC++ !?
#define cbrt(x) pow((x),1./3.)
#endif

//! Solve true anomaly nu for hyperbolic "orbit" (better: trajectory) around the sun.
//! @param dt: days from perihel
//! @param rCosNu: r*cos(nu)
//! @param rSinNu: r*sin(nu)
void KeplerOrbit::InitHyp(const double dt, double &rCosNu, double &rSinNu)
{
//	qDebug() << "InitHyp";
	Q_ASSERT(e>1.0);
	const double a = q/(e-1.0);
	Q_ASSERT(a>0.0);
	const double M = n * dt;
//	Heafner, ch.5.4
	double E=StelUtils::sign(M)*log(2.0*fabs(M)/e + 1.85);
//	qDebug() << "InitHyp: E=" << E << " M=" << M ;
	for (;;)
	{
		const double Ep=E;
		const double f2=e*sinh(E);
		const double f=f2-E-M;
		const double f1=e*cosh(E)-1.0;
		E+= (-5.0*f)/(f1+StelUtils::sign(f1)*std::sqrt(fabs(16.0*f1*f1-20.0*f*f2)));
		if (fabs(E-Ep) < EPSILON) break;
	}
	rCosNu = a*(e-cosh(E));
	rSinNu = a*std::sqrt(e*e-1.0)*sinh(E);
}

//! Solve true anomaly nu for parabolic orbit around the sun.
//! @param dt: days from perihel
//! @param rCosNu: r*cos(nu)
//! @param rSinNu: r*sin(nu)
void KeplerOrbit::InitPar(const double dt, double &rCosNu, double &rSinNu)
{
//	qDebug() << "InitPar";
	Q_ASSERT(e==1.0);
//	const double M=dt*sqrt(GAUSS_GRAV_CONST/(2.0*q*q*q));
//	const double W=1.5*M;
	const double W=dt*n;
	const double Y=cbrt(W+std::sqrt(W*W+1.));
	const double tanNu2=Y-1.0/Y; // Heafner (5.5.8) has an error here, writes (Y-1)/Y.
	rCosNu=q*(1.0-tanNu2*tanNu2);
	rSinNu=2.0*q*tanNu2;
}


//! Solve true anomaly nu for elliptical orbit with Laguerre-Conway's method. (May have high e)
//! @param dt: days from perihel
//! @param rCosNu: r*cos(nu)
//! @param rSinNu: r*sin(nu)
void KeplerOrbit::InitEll(const double dt, double &rCosNu, double &rSinNu)
{
//	qDebug() << "InitEll";
	Q_ASSERT(e<1.0);
	const double a = q/(1.0-e); // semimajor axis
	double M = fmod(n*dt,2*M_PI);  // Mean Anomaly
	if (M < 0.0) M += 2.0*M_PI;
//	GZ: Comet orbits are quite often near-parabolic, where this may still only converge slowly.
//	Better always use Laguerre-Conway. See Heafner, Ch. 5.3
//	Ouch! https://bugs.launchpad.net/stellarium/+bug/1465112 ==>It seems we still need an escape counter!
//      Debug line in test case fabs(E-Ep) indicates it usually takes 2-3, occasionally up to 6 cycles.
//	It seems safe to assume 10 should not be exceeded. N.B.: A GPU fixed-loopcount implementation could go for 8 passes.
	double E=M+0.85*e*StelUtils::sign(sin(M));
	int escape=0;
	for (;;)
	{
		const double Ep=E;
		const double f2=e*sin(E);
		const double f=E-f2-M;
		const double f1=1.0-e*cos(E);
		E+= (-5.0*f)/(f1+StelUtils::sign(f1)*std::sqrt(fabs(16.0*f1*f1-20.0*f*f2)));
		if (fabs(E-Ep) < EPSILON)
		{
			//qDebug() << "Ell. orbit with eccentricity " << e << "Escaping after" << escape << "loops at E-Ep=" << E-Ep;
			break;
		}
		if (++escape>10)
		{
			qDebug() << "Ell. orbit with eccentricity " << e << "would have caused endless loop. Escaping after 10 runs at E-Ep=" << E-Ep;
			break;
		}
	}
//	Note: q=a*(1-e)
	const double h1 = q*std::sqrt((1.0+e)/(1.0-e));  // elsewhere: a sqrt(1-e²)     ... q / (1-e) sqrt( (1+e)(1-e)) = q sqrt((1+e)/(1-e))
	rCosNu = a*(cos(E)-e);
	rSinNu = h1*sin(E);
}

KeplerOrbit::KeplerOrbit(double pericenterDistance,
			double eccentricity,
			double inclination,
			double ascendingNode,
			double argOfPerhelion,
			double timeAtPerihelion,
			double orbitGoodDays,
			double meanMotion,              // GZ: for parabolics, this is W/dt in Heafner's lettering
			double parentRotObliquity,
			double parentRotAscendingnode,
			double parentRotJ2000Longitude,
			double centralMass)
	: q(pericenterDistance),
	  e(eccentricity),
	  i(inclination),
	  Om(ascendingNode),
	  w(argOfPerhelion),
	  t0(timeAtPerihelion),
	  n(meanMotion),
	  centralMass(centralMass),
	  orbitGood(orbitGoodDays),
	  rdot(0.0),
	  updateTails(true)
{
	// For Comets and Minor planets, this just builds a unity matrix. For moons, it rotates into the equatorial system of the parent planet
	setParentOrientation(parentRotObliquity, parentRotAscendingnode, parentRotJ2000Longitude);
}

//! For planet moons which have orbits given in relation to their parent planet's equator.
//! This is called by the constructor, and must be updated for parent planets when their axis changes over time.
void Orbit::setParentOrientation(const double parentRotObliquity, const double parentRotAscendingNode, const double parentRotJ2000Longitude)
{
	// GZ MAKE SURE THIS IS ALWAYS 0/0/0. ==> OK for all sun-parented objects: Minor Planets and Comets.
	// qDebug() << "parentRotObliquity" << parentRotObliquity*M_180_PI << "parentRotAscendingnode" << parentRotAscendingNode*M_180_PI << "parentRotJ2000Longitude" << parentRotJ2000Longitude*M_180_PI;
	// For Moons on Kepler orbits, these elements describe the orientation of its orbit, apparently in epoch J2000.
	// This indicates that in Stellarium, all those moon orbits are given with relation to the Planet equator.
	// The problem is that on a planet with changing axis, this rotation matrix should be recomputed e.g. once per year or so.
	// Another problem is that probably moons exist with elements given in other reference frames. ExplanSup2013 even has many orbits w.r.t. B1950 ecliptic.
#ifdef _GNU_SOURCE
	double c_obl, s_obl, c_nod, s_nod, cj, sj;
	sincos(parentRotObliquity, &s_obl, &c_obl);
	sincos(parentRotAscendingNode, &s_nod, &c_nod);
	sincos(parentRotJ2000Longitude, &sj, &cj);
#else
	const double c_obl = cos(parentRotObliquity);
	const double s_obl = sin(parentRotObliquity);
	const double c_nod = cos(parentRotAscendingNode);
	const double s_nod = sin(parentRotAscendingNode);
	const double cj = cos(parentRotJ2000Longitude);
	const double sj = sin(parentRotJ2000Longitude);
#endif
	rotateToVsop87[0] =  c_nod*cj-s_nod*c_obl*sj;
	rotateToVsop87[1] = -c_nod*sj-s_nod*c_obl*cj;
	rotateToVsop87[2] =           s_nod*s_obl;
	rotateToVsop87[3] =  s_nod*cj+c_nod*c_obl*sj;
	rotateToVsop87[4] = -s_nod*sj+c_nod*c_obl*cj;
	rotateToVsop87[5] =          -c_nod*s_obl;
	rotateToVsop87[6] =                 s_obl*sj;
	rotateToVsop87[7] =                 s_obl*cj;
	rotateToVsop87[8] =                 c_obl;
}


void KeplerOrbit::positionAtTimevInVSOP87Coordinates(double JDE, double *v)
{
	JDE -= t0;
	double rCosNu,rSinNu;
	if (e < 1.0) InitEll(JDE,rCosNu,rSinNu); // Laguerre-Conway seems stable enough to go for <1.0.
	else if (e > 1.0)
	{
		// qDebug() << "Hyperbolic orbit for ecc=" << e << ", i=" << i << ", w=" << w << ", Mean Motion n=" << n;
		InitHyp(JDE,rCosNu,rSinNu);
	}
	else InitPar(JDE,rCosNu,rSinNu);

	// Compute position vector and speed vector from orbital elements and true anomaly components. See e.g. Heafner, Fund.Eph.Comp.1999
#ifdef _GNU_SOURCE
	double cw, sw, cOm, sOm, ci, si;
	sincos(w, &sw, &cw);
	sincos(Om, &sOm, &cOm);
	sincos(i, &si, &ci);
#else
	const double cw = cos(w);
	const double sw = sin(w);
	const double cOm = cos(Om);
	const double sOm = sin(Om);
	const double ci = cos(i);
	const double si = sin(i);
#endif
	const double Px=-sw*sOm*ci+cw*cOm; // Heafner, 5.3.1 Px
	const double Qx=-cw*sOm*ci-sw*cOm; // Heafner, 5.3.4 Qx
	const double Py= sw*cOm*ci+cw*sOm; // Heafner, 5.3.2 Py
	const double Qy= cw*cOm*ci-sw*sOm; // Heafner, 5.3.5 Qy
	const double Pz= sw*si;            // Heafner, 5.3.3 Pz
	const double Qz= cw*si;            // Heafner, 5.3.6 Qz
	const double p0 = Px*rCosNu+Qx*rSinNu; // rx: x component of position vector, AU Heafner, 5.3.18 r
	const double p1 = Py*rCosNu+Qy*rSinNu; // ry: y component of position vector, AU
	const double p2 = Pz*rCosNu+Qz*rSinNu; // rz: z component of position vector, AU
	const double r=std::sqrt(rSinNu*rSinNu+rCosNu*rCosNu);
	const double sinNu=rSinNu/r;
	const double cosNu=rCosNu/r;
	const double p=q*(1.0+e); // Heafner: semilatus rectum
	const double sqrtMuP=std::sqrt(GAUSS_GRAV_k_SQ*centralMass/p);
	const double s0=sqrtMuP*((e+cosNu)*Qx - sinNu*Px); // rdotx: x component of velocity vector, AU/d Heafner, 5.3.19 r'
	const double s1=sqrtMuP*((e+cosNu)*Qy - sinNu*Py); // rdoty: y component of velocity vector, AU/d
	const double s2=sqrtMuP*((e+cosNu)*Qz - sinNu*Pz); // rdotz: z component of velocity vector, AU/d

	v[0] = rotateToVsop87[0]*p0 + rotateToVsop87[1]*p1 + rotateToVsop87[2]*p2;
	v[1] = rotateToVsop87[3]*p0 + rotateToVsop87[4]*p1 + rotateToVsop87[5]*p2;
	v[2] = rotateToVsop87[6]*p0 + rotateToVsop87[7]*p1 + rotateToVsop87[8]*p2;

	//rdot.set(s0, s1, s2); // FIXME: The speed also needs to be rotated. Correct?

	rdot[0] = rotateToVsop87[0]*s0 + rotateToVsop87[1]*s1 + rotateToVsop87[2]*s2;
	rdot[1] = rotateToVsop87[3]*s0 + rotateToVsop87[4]*s1 + rotateToVsop87[5]*s2;
	rdot[2] = rotateToVsop87[6]*s0 + rotateToVsop87[7]*s1 + rotateToVsop87[8]*s2;

	updateTails=true;
}

// Calculate sidereal period in days from semi-major axis.
// Source: Heafner, Fundamental Eph. Comp. p.71.
double KeplerOrbit::calculateSiderealPeriod(const double semiMajorAxis, const double centralMass)
{
	// Solution for non-Solar central mass (Moons:) we need to take central mass (in Solar units) into account. Tested with comparison of preconfigured Moon data.
	return (semiMajorAxis <=0 ? 0. : (2.*M_PI/GAUSS_GRAV_k)*sqrt(semiMajorAxis*semiMajorAxis*semiMajorAxis/centralMass));
}

double KeplerOrbit::calculateSiderealPeriod() const
{
	if (e>=1.)
		return 0.;
	const double a = q/(1.0-e); // semimajor axis
	return calculateSiderealPeriod(a, centralMass);
}

GimbalOrbit::GimbalOrbit(double distance, double longitude, double latitude):
	distance(distance),
	longitude(longitude),
	latitude(latitude)
{
	setParentOrientation(0., 0., 0.);
};
//! Compute the position (JDE is just a placeholder) and return a "stellarium compliant" position
void GimbalOrbit::positionAtTimevInVSOP87Coordinates(double JDE, double* v)
{
	Q_UNUSED(JDE)
	Vec3d pos;
	StelUtils::spheToRect(longitude, latitude, pos);
	v[0] = rotateToVsop87[0]*pos[0] + rotateToVsop87[1]*pos[1] + rotateToVsop87[2]*pos[2];
	v[1] = rotateToVsop87[3]*pos[0] + rotateToVsop87[4]*pos[1] + rotateToVsop87[5]*pos[2];
	v[2] = rotateToVsop87[6]*pos[0] + rotateToVsop87[7]*pos[1] + rotateToVsop87[8]*pos[2];
	v[0] *= distance;
	v[1] *= distance;
	v[2] *= distance;
}

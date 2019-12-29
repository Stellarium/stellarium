// orbit.cpp
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// CometOrbit, InitHyp,InitPar,InitEll,Init3D: Copyright (C) 1995,2007,2008 Johannes Gajdosik
// InitHyp,InitPar,InitEll,Init3D checked, improved, extended, annotated 2013 Georg Zotti (GZ).
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
// line from vsop87.c, but also used by Heafner, 5.3.12. This is mu, we ignore the comet's mass i.r.t. the Sun's.
#define GAUSS_GRAV_CONST_SQ (0.01720209895*0.01720209895)

#if defined(_MSC_VER) && (_MSC_VER<1900)
// cuberoot is missing in older VC++ !?
#define cbrt(x) pow((x),1./3.)
#endif

//! Solve true anomaly nu for hyperbolic orbit.
//! @param q: perihel distance
//! @param n: mean motion
//! @param e: excentricity
//! @param dt: days from perihel
//! @param rCosNu: r*cos(nu)
//! @param rSinNu: r*sin(nu)
static void InitHyp(const double q, const double n, const double e, const double dt, double &rCosNu, double &rSinNu)
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

//! Solve true anomaly nu for parabolic orbit.
//! @param q: perihel distance
//! @param n: mean motion equivalent related to W (n=W/dt) in Heafner, ch5.5
//! @param dt: days from perihel
//! @param rCosNu: r*cos(nu)
//! @param rSinNu: r*sin(nu)
static void InitPar(const double q, const double n, const double dt, double &rCosNu, double &rSinNu)
{
//	qDebug() << "InitPar";
//	const double M=dt*sqrt(GAUSS_GRAV_CONST/(2.0*q*q*q));
//	const double W=1.5*M;
	const double W=dt*n;
	const double Y=cbrt(W+std::sqrt(W*W+1.));
	const double tanNu2=Y-1.0/Y; // Heafner (5.5.8) has an error here, writes (Y-1)/Y.
	rCosNu=q*(1.0-tanNu2*tanNu2);
	rSinNu=2.0*q*tanNu2;
}


//! Solve true anomaly nu for elliptical orbit with Laguerre-Conway's method. (May have high e)
//! @param q: perihel distance
//! @param n: mean motion
//! @param e: excentricity
//! @param dt: days from perihel
//! @param rCosNu: r*cos(nu)
//! @param rSinNu: r*sin(nu)
static void InitEll(const double q, const double n, const double e, const double dt, double &rCosNu, double &rSinNu)
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
		       double parentRotJ2000Longitude)
	: q(pericenterDistance),
	  e(eccentricity),
	  i(inclination),
	  Om(ascendingNode),
	  w(argOfPerhelion),
	  t0(timeAtPerihelion),
	  n(meanMotion),
	  updateTails(true),
	  orbitGood(orbitGoodDays)
{
	// GZ MAKE SURE THIS IS ALWAYS 0/0/0. ==> OK.
	//qDebug() << "parentRotObliquity" << parentRotObliquity << "parentRotAscendingnode" << parentRotAscendingnode << "parentRotJ2000Longitude" << parentRotJ2000Longitude;
	rdot.set(0.0, 0.0, 0.0);
	const double c_obl = cos(parentRotObliquity);
	const double s_obl = sin(parentRotObliquity);
	const double c_nod = cos(parentRotAscendingnode);
	const double s_nod = sin(parentRotAscendingnode);
	const double cj = cos(parentRotJ2000Longitude);
	const double sj = sin(parentRotJ2000Longitude);

	rotateToVsop87[0] =  c_nod*cj-s_nod*c_obl*sj;
	rotateToVsop87[1] = -c_nod*sj-s_nod*c_obl*cj;
	rotateToVsop87[2] =           s_nod*s_obl;
	rotateToVsop87[3] =  s_nod*cj+c_nod*c_obl*sj;
	rotateToVsop87[4] = -s_nod*sj+c_nod*c_obl*cj;
	rotateToVsop87[5] =          -c_nod*s_obl;
	rotateToVsop87[6] =                 s_obl*sj;
	rotateToVsop87[7] =                 s_obl*cj;
	rotateToVsop87[8] =                 c_obl;
	//  qDebug() << "CometOrbit::()...done";
}

void KeplerOrbit::positionAtTimevInVSOP87Coordinates(double JDE, double *v)
{
	JDE -= t0;
	double rCosNu,rSinNu;
	if (e < 1.0) InitEll(q,n,e,JDE,rCosNu,rSinNu); // Laguerre-Conway seems stable enough to go for <1.0.
	else if (e > 1.0)
	{
		// qDebug() << "Hyperbolic orbit for ecc=" << e << ", i=" << i << ", w=" << w << ", Mean Motion n=" << n;
		InitHyp(q,n,e,JDE,rCosNu,rSinNu);
	}
	else InitPar(q,n,JDE,rCosNu,rSinNu);

	// Compute position vector and speed vector from orbital elements and true anomaly components. See e.g. Heafner, Fund.Eph.Comp.1999
	const double cw = cos(w);
	const double sw = sin(w);
	const double cOm = cos(Om);
	const double sOm = sin(Om);
	const double ci = cos(i);
	const double si = sin(i);
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
	const double p=q*(1.0+e);
	const double sqrtMuP=std::sqrt(GAUSS_GRAV_CONST_SQ/p);
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
double KeplerOrbit::calculateSiderealPeriod(const double semiMajorAxis)
{
	return (semiMajorAxis >=0 ? (2.*M_PI/0.01720209895)*sqrt(semiMajorAxis*semiMajorAxis*semiMajorAxis) : std::numeric_limits<double>::max() );
}



/*

EllipticalOrbit::EllipticalOrbit(double pericenterDistance,
                                 double eccentricity,
                                 double inclination,
                                 double ascendingNode,
                                 double argOfPeriapsis,
                                 double meanAnomalyAtEpoch,
                                 double period,
                                 double epoch,
                                 double parentRotObliquity,
				 double parentRotAscendingnode,
				 double parentRotJ2000Longitude)
	: pericenterDistance(pericenterDistance),
	  eccentricity(eccentricity),
	  inclination(inclination),
	  ascendingNode(ascendingNode),
	  argOfPeriapsis(argOfPeriapsis),
	  meanAnomalyAtEpoch(meanAnomalyAtEpoch),
	  period(period),
	  epoch(epoch)
{
	Q_ASSERT(0); // WE WANT TO GET RID OF THIS CLASS!
	const double c_obl = cos(parentRotObliquity);
	const double s_obl = sin(parentRotObliquity);
	const double c_nod = cos(parentRotAscendingnode);
	const double s_nod = sin(parentRotAscendingnode);
	const double cj = cos(parentRotJ2000Longitude);
	const double sj = sin(parentRotJ2000Longitude);

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

// Standard iteration for solving Kepler's Equation
struct SolveKeplerFunc1 : public unary_function<double, double>
{
	double ecc;
	double M;

	SolveKeplerFunc1(double _ecc, double _M) : ecc(_ecc), M(_M) {}

	double operator()(double x) const
	{
		return M + ecc * sin(x);
	}
};


// Faster converging iteration for Kepler's Equation; more efficient
// than above for orbits with eccentricities greater than 0.3.  This
// is from Jean Meeus's _Astronomical Algorithms_ (2nd ed), p. 199
struct SolveKeplerFunc2 : public unary_function<double, double>
{
	double ecc;
	double M;

	SolveKeplerFunc2(double _ecc, double _M) : ecc(_ecc), M(_M) {}

	double operator()(double x) const
	{
		return x + (M + ecc * sin(x) - x) / (1 - ecc * cos(x));
	}
};

struct SolveKeplerLaguerreConway : public unary_function<double, double>
{
	double ecc;
	double M;

	SolveKeplerLaguerreConway(double _ecc, double _M) : ecc(_ecc), M(_M) {}
	// cf Heafner, Fundamental Ephemeris Computations p.73
	// GZ: note&add Heafner's initial guess for E!
	double operator()(double E) const
	{
		double s = ecc * sin(E);
		double c = ecc * cos(E);
		double f = E - s - M;
		double f1 = 1 - c;
		double f2 = s;
		E += -5 * f / (f1 + StelUtils::sign(f1) * std::sqrt(abs(16 * f1 * f1 - 20 * f * f2)));

		return E;
	}
};

struct SolveKeplerLaguerreConwayHyp : public unary_function<double, double>
{
	double ecc;
	double M;

	SolveKeplerLaguerreConwayHyp(double _ecc, double _M) : ecc(_ecc), M(_M) {}
	// cf Heafner, Fundamental Ephemeris Computations p.73
	double operator()(double x) const
	{
		double s = ecc * sinh(x);
		double c = ecc * cosh(x);
		double f = s - x - M;
		double f1 = c - 1;
		double f2 = s;
		x += -5 * f / (f1 + StelUtils::sign(f1) * std::sqrt(abs(16 * f1 * f1 - 20 * f * f2)));

		return x;
	}
};

typedef pair<double, double> Solution;

double EllipticalOrbit::eccentricAnomaly(const double M) const
{
	if (eccentricity == 0.0)
	{
		// Circular orbit
		return M;
	}
	else if (eccentricity < 0.2)
	{
		// Low eccentricity, so use the standard iteration technique
		Solution sol = solveIteration_fixed(SolveKeplerFunc1(eccentricity, M), M, 5);
		return sol.first;
	}
	else if (eccentricity < 0.9)
	{
		// Higher eccentricity elliptical orbit; use a more complex but
		// much faster converging iteration.
		Solution sol = solveIteration_fixed(SolveKeplerFunc2(eccentricity, M), M, 6);
		// Debugging
		// qDebug("ecc: %f, error: %f mas\n",
		//        eccentricity, radToDeg(sol.second) * 3600000);
		return sol.first;
	}
	else if (eccentricity < 1.0)
	{
		// Extremely stable Laguerre-Conway method for solving Kepler's
		// equation.  Only use this for high-eccentricity orbits, as it
		// requires more calculation.
		double E = M + 0.85 * eccentricity * StelUtils::sign(sin(M));
		Solution sol = solveIteration_fixed(SolveKeplerLaguerreConway(eccentricity, M), E, 8);
		return sol.first;
	}
	else if (eccentricity == 1.0)
	{
		// parabolic orbit; very common for comets
		// TODO: handle this.
		// Problem: E does not make sense here. True anomaly quantities (rSinNu, rCosNu) computed directly.
		// Anyhow, Comets use CometOrbit class.
		return M;
	}
	else
	{
		// Laguerre-Conway method for hyperbolic (ecc > 1) orbits.
		double E = log(2 * M / eccentricity + 1.85);
		Solution sol = solveIteration_fixed(SolveKeplerLaguerreConwayHyp(eccentricity, M), E, 30);
		return sol.first;
	}
}


Vec3d EllipticalOrbit::positionAtE(const double E) const
{
	double x, z;

	if (eccentricity < 1.0)
	{
		double a = pericenterDistance / (1.0 - eccentricity);
		x = a * (cos(E) - eccentricity);
		z = a * std::sqrt(1 - eccentricity * eccentricity) * -sin(E);
	}
	else if (eccentricity > 1.0) // N.B. This is odd at least: elliptical must have ecc<1!
	{
		Q_ASSERT(0); // We should never come here!
		double a = pericenterDistance / (1.0 - eccentricity);
		x = -a * (eccentricity - cosh(E));
		z = -a * std::sqrt(eccentricity * eccentricity - 1) * -sinh(E);
	}
	else
	{
		// TODO: Handle parabolic orbits
		Q_ASSERT(0); // We should never come here!
		x = 0.0;
		z = 0.0;
	}

	Mat4d R = (Mat4d::zrotation(ascendingNode) *
		   Mat4d::xrotation(inclination) *
		   Mat4d::zrotation(argOfPeriapsis));

	return R * Vec3d(x, -z, 0);
}

// Return the offset from the center.
Vec3d EllipticalOrbit::positionAtTime(const double JDE) const
{
	double meanMotion = 2.0 * M_PI / period;
	double meanAnomaly = meanAnomalyAtEpoch + (JDE-epoch) * meanMotion;
	double E = eccentricAnomaly(meanAnomaly);

	return positionAtE(E);
}

void EllipticalOrbit::positionAtTimevInVSOP87Coordinates(const double JDE, double* v) const
{
	Vec3d pos = positionAtTime(JDE);
	v[0] = rotateToVsop87[0]*pos[0] + rotateToVsop87[1]*pos[1] + rotateToVsop87[2]*pos[2];
	v[1] = rotateToVsop87[3]*pos[0] + rotateToVsop87[4]*pos[1] + rotateToVsop87[5]*pos[2];
	v[2] = rotateToVsop87[6]*pos[0] + rotateToVsop87[7]*pos[1] + rotateToVsop87[8]*pos[2];
}
*/

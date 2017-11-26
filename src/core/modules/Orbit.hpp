// orbit.h
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// CometOrbit: Copyright (C) 2007,2008 Johannes Gajdosik
//             Amendments (c) 2013 Georg Zotti
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef _ORBIT_HPP_
#define _ORBIT_HPP_

#include "VecMath.hpp"

class OrbitSampleProc;

//! @internal
//! Orbit computations used for comet and asteroids
class Orbit
{
public:
    Orbit(void) {}
    virtual ~Orbit(void) {}
private:
    Orbit(const Orbit&);
    const Orbit &operator=(const Orbit&);
};


class EllipticalOrbit : public Orbit
{
public:
	EllipticalOrbit(double pericenterDistance,
			double eccentricity,
			double inclination,
			double ascendingNode,
			double argOfPeriapsis,
			double meanAnomalyAtEpoch,
			double period,
			double epoch,			// = 2451545.0,
			double parentRotObliquity,	// = 0.0,
			double parentRotAscendingnode,	// = 0.0
			double parentRotJ2000Longitude	// = 0.0
			);

	// Compute position for a specified Julian date and return coordinates
	// given in "dynamical equinox and ecliptic J2000"
	// which is the reference frame for VSOP87
	// In order to rotate to VSOP87
	// parentRotObliquity and parentRotAscendingnode must be supplied.
	void positionAtTimevInVSOP87Coordinates(const double JDE, double* v) const;

	// Original one
	Vec3d positionAtTime(const double JDE) const;
	double getPeriod() const;
	// double getBoundingRadius() const; // Return apoapsis distance. UNUSED!
	// virtual void sample(double, double, int, OrbitSampleProc&) const; //UNDOCUMENTED & UNUSED

private:
	//! returns eccentric anomaly E for Mean anomaly M
	double eccentricAnomaly(const double M) const;
	Vec3d positionAtE(const double E) const;

	double pericenterDistance;
	double eccentricity;
	double inclination;
	double ascendingNode;
	double argOfPeriapsis;
	double meanAnomalyAtEpoch;
	double period;
	double epoch;
	double rotateToVsop87[9];
};


class CometOrbit : public Orbit {
public:
	CometOrbit(double pericenterDistance,
		   double eccentricity,
		   double inclination,
		   double ascendingNode,
		   double argOfPerhelion,
		   double timeAtPerihelion,
		   double orbitGoodDays,
		   double meanMotion,			// GZ: for parabolics, this is W/dt in Heafner's lettering
		   double parentRotObliquity,		// Comets only have parent==sun, no need for these? Oh yes, VSOP/J2000 eq frames!
		   double parentRotAscendingnode,
		   double parentRotJ2000Longitude
		   );
	// Compute the orbit for a specified Julian day and return a "stellarium compliant" function
	// GZ: new optional variable: updateVelocityVector, true required for dust tail orientation!
	void positionAtTimevInVSOP87Coordinates(double JDE, double* v, bool updateVelocityVector=true);
	// updating the tails is a bit expensive. try not to overdo it.
	bool getUpdateTails() const { return updateTails; }
	void setUpdateTails(const bool update){ updateTails=update; }
	//! return speed value [AU/d] last computed by positionAtTimevInVSOP87Coordinates(JDE, v, true)
	Vec3d getVelocity() const { return rdot; }
	void getVelocity(double *vel) const { vel[0]=rdot[0]; vel[1]=rdot[1]; vel[2]=rdot[2];}
	double getSemimajorAxis() const { return (e==1. ? 0. : q / (1.-e)); }
	double getEccentricity() const { return e; }
	bool objectDateValid(const double JDE) const { return (fabs(t0-JDE)<orbitGood); }
private:
	const double q;  //! perihel distance
	const double e;  //! eccentricity
	const double i;  //! inclination
	const double Om; //! longitude of ascending node
	const double w;  //! argument of perihel
	const double t0; //! time of perihel, JDE
	const double n;  //! mean motion (for parabolic orbits: W/dt in Heafner's presentation)
	Vec3d rdot;      //! GZ: velocity vector. Caches velocity from last position computation, [AU/d]
	double rotateToVsop87[9]; //! Rotation matrix
	bool updateTails; //! flag to signal that tails must be recomputed.
	const double orbitGood; //! orb. elements are only valid for this time from perihel [days]. Don't draw the object outside.
};


class OrbitSampleProc
{
 public:
	virtual ~OrbitSampleProc() {;}
	virtual void sample(const Vec3d&) = 0;
};

/*
 * Stuff found unused and deactivated pre-0.15

// Custom orbit classes should be derived from CachingOrbit.  The custom
// orbits can be expensive to compute, with more than 50 periodic terms.
// Celestia may need require position of a Planet more than once per frame; in
// order to avoid redundant calculation, the CachingOrbit class saves the
// result of the last calculation and uses it if the time matches the cached
// time.
class CachingOrbit : public Orbit
{
public:
	CachingOrbit() : lastTime(1.0e-30) {} //;

	virtual Vec3d computePosition(double JDE) const = 0;
	virtual double getPeriod() const = 0;
	virtual double getBoundingRadius() const = 0;

	Vec3d positionAtTime(double JDE) const;

	virtual void sample(double, double, int, OrbitSampleProc& proc) const;

private:
	mutable Vec3d lastPosition;
	mutable double lastTime;
};

*/

#endif // _ORBIT_HPP_

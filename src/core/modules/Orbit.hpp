// orbit.h
//
// Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
//
// CometOrbit: Copyright (C) 2007,2008 Johannes Gajdosik
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
                    double epoch, // = 2451545.0,
                    double parentRotObliquity, // = 0.0,
                    double parentRotAscendingnode, // = 0.0
					double parentRotJ2000Longitude  // = 0.0
                    );

	// Compute position for a specified Julian date and return coordinates
	// given in "dynamical equinox and ecliptic J2000"
	// which is the reference frame for VSOP87
	// In order to rotate to VSOP87
	// parentRotObliquity and parentRotAscendingnode must be supplied.
    void positionAtTimevInVSOP87Coordinates(double JD, double* v) const;

	// Original one
    Vec3d positionAtTime(double) const;
    double getPeriod() const;
    double getBoundingRadius() const;
    virtual void sample(double, double, int, OrbitSampleProc&) const;

private:
    double eccentricAnomaly(double) const;
    Vec3d positionAtE(double) const;

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
             double meanMotion,
             double parentRotObliquity,
             double parentRotAscendingnode,
             double parentRotJ2000Longitude);

    // Compute the orbit for a specified Julian date and return a "stellarium compliant" function
  void positionAtTimevInVSOP87Coordinates(double JD, double* v) const;
private:
  const double q;
  const double e;
  const double i;
  const double Om;
  const double o;
  const double t0;
  const double n;
  double rotateToVsop87[9];
};


class OrbitSampleProc
{
 public:
	virtual ~OrbitSampleProc() {;}
    virtual void sample(const Vec3d&) = 0;
};



// Custom orbit classes should be derived from CachingOrbit.  The custom
// orbits can be expensive to compute, with more than 50 periodic terms.
// Celestia may need require position of a Planet more than once per frame; in
// order to avoid redundant calculation, the CachingOrbit class saves the
// result of the last calculation and uses it if the time matches the cached
// time.
class CachingOrbit : public Orbit
{
public:
    CachingOrbit() : lastTime(1.0e-30) {};

    virtual Vec3d computePosition(double jd) const = 0;
    virtual double getPeriod() const = 0;
    virtual double getBoundingRadius() const = 0;

    Vec3d positionAtTime(double jd) const;

    virtual void sample(double, double, int, OrbitSampleProc& proc) const;

private:
    mutable Vec3d lastPosition;
    mutable double lastTime;
};



#endif // _ORBIT_HPP_

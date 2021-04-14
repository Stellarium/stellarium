// orbit.h
//
// Initial structure of orbit computation; EllipticalOrbit: Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
// CometOrbit: Copyright (c) 2007,2008 Johannes Gajdosik
//             Amendments (c) 2013 Georg Zotti (GZ).
// Combination to KeplerOrbit, GimbalOrbit (c) 2020 Georg Zotti
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

#ifndef ORBIT_HPP
#define ORBIT_HPP

#define USE_GIMBAL_ORBIT 1

#include "VecMath.hpp"
#include "StelUtils.hpp"

//! @internal
//! Orbit computations used for comets, minor planets and "simple" moons
class Orbit
{
public:
    Orbit(void) {}
    virtual ~Orbit(void) {}
    //! Return a position (XYZ in AU).
    //! @param JDE Julian Ephemeris Date
    //! @param v double array of at least 3 elements. The first three should be filled by the X/Y/Z data.
    virtual void positionAtTimevInVSOP87Coordinates(double JDE, double* v){Q_UNUSED(JDE) Q_UNUSED(v)}
    //! return speed value [AU/d]. (zero in the base class)
    virtual Vec3d getVelocity() const { return Vec3d(0.); }
    //! write speed value [AU/d] into first 3 elements of vel. (zero in the base class)
    virtual void getVelocity(double *vel) const { vel[0]=0.; vel[1]=0.; vel[2]=0.;}
    //! return semimajor axis. (zero in the base class)
    virtual double getSemimajorAxis() const { return 0.; }
    //! return orbit eccentricity. (zero in the base class)
    virtual double getEccentricity() const { return 0; }
    //! For planet moons which have orbits given in relation to their parent planet's equator.
    //! This is called by the constructor, and must be updated for parent planets when their axis changes over time.
    void setParentOrientation(const double parentRotObliquity, const double parentRotAscendingNode, const double parentRotJ2000Longitude);
private:
    Orbit(const Orbit&);
    const Orbit &operator=(const Orbit&);
protected:
    double rotateToVsop87[9]; //! Rotation matrix.
};

// This class was called CometOrbit, but was now recombined with the former EllipticalOrbit class. They did almost the same.
class KeplerOrbit : public Orbit {
public:
	KeplerOrbit(double pericenterDistance,
		    double eccentricity,
		    double inclination,
		    double ascendingNode,
		    double argOfPerhelion,
		    double timeAtPerihelion,
		    double orbitGoodDays,
		    double meanMotion,			// GZ: for parabolics, this is W/dt in Heafner's lettering
		    double parentRotObliquity,		// Comets/Minor Planets only have parent==sun, no need for these? Oh yes, VSOP/J2000 eq frames!
		    double parentRotAscendingnode,
		    double parentRotJ2000Longitude,
		    double centralMass = 1.0            // central mass in Solar masses. Velocity value depends on this!
		   );
	//! Compute the object position for a specified Julian day.
	//! @param JDE Julian Ephemeris Day
	//! @param v double vector of at least 3 elements. The first three will receive X/Y/Z values in AU.
	virtual void positionAtTimevInVSOP87Coordinates(double JDE, double* v) Q_DECL_OVERRIDE;
	//! updating comet tails is a bit expensive. try not to overdo it.
	bool getUpdateTails() const { return updateTails; }
	void setUpdateTails(const bool update){ updateTails=update; }
	//! return speed value [AU/d] last computed by positionAtTimevInVSOP87Coordinates(JDE, v)
	virtual Vec3d getVelocity() const Q_DECL_OVERRIDE { return rdot; }
	virtual void getVelocity(double *vel) const Q_DECL_OVERRIDE { vel[0]=rdot[0]; vel[1]=rdot[1]; vel[2]=rdot[2];}
	//! Returns semimajor axis [AU] for elliptic orbit, 0 for a parabolic orbit, and a negative value [AU] for hyperbolic orbit.
	virtual double getSemimajorAxis() const Q_DECL_OVERRIDE { return (e==1. ? 0. : q / (1.-e)); }
	virtual double getEccentricity() const Q_DECL_OVERRIDE { return e; }
	bool objectDateValid(const double JDE) const { return ((orbitGood<=0) || (fabs(t0-JDE)<orbitGood)); }
	//! Calculate sidereal period in days from semi-major axis and central mass. If SMA<=0 (hyperbolic orbit), return 0.
	double calculateSiderealPeriod() const;
	//! @param semiMajorAxis in AU. If SMA<=0 (hyperbolic orbit), return 0.
	//! @param centralMass in units of Solar masses
	static double calculateSiderealPeriod(const double semiMajorAxis, const double centralMass);

private:
	const double q;  //!< pericenter distance [AU]
	const double e;  //!< eccentricity
	const double i;  //!< inclination [radians]
	const double Om; //!< longitude of ascending node [radians]
	const double w;  //!< argument of perihel [radians]
	const double t0; //!< time of perihel, JDE
	const double n;  //!< mean motion (for parabolic orbits: W/dt in Heafner's presentation, ch5.5) [radians/day]
	const double centralMass; //!< Mass in Solar masses. Velocity depends on this.
	const double orbitGood; //!< orb. elements are only valid for this time from perihel [days]. Don't draw the object outside. Values <=0 mean "always good" (objects on undisturbed elliptic orbit)
	Vec3d rdot;       //!< velocity vector. Caches velocity from last position computation, [AU/d]
	bool updateTails; //!< flag to signal that comet tails must be recomputed.
	void InitEll(const double dt, double &rCosNu, double &rSinNu);
	void InitPar(const double dt, double &rCosNu, double &rSinNu);
	void InitHyp(const double dt, double &rCosNu, double &rSinNu);
};

//! A pseudo-orbit for "observers" linked to a planet's sphere. It allows setting distance and longitude/latitude in the VSOP87 frame.
//! This class ic currently in an experimental state. rotateToVsop87 may need to be set up correctly, and view frame currently cannot be controlled properly.
class GimbalOrbit : public Orbit {
public:
	GimbalOrbit(double distance,
		   double longitude,
		   double latitude
		   );
	//! Compute position for a (unused) Julian day.
	virtual void positionAtTimevInVSOP87Coordinates(double JDE, double* v) Q_DECL_OVERRIDE;
	//! Returns (pseudo) semimajor axis [AU] of a circular orbit.
	double getSemimajorAxis() const Q_DECL_OVERRIDE { return distance; }

	double getLongitude() const { return longitude*M_180_PI;}
	double getLatitude()  const { return latitude*M_180_PI;}
	double getDistance()  const { return distance;}
	void setLongitude(const double lng){ longitude=lng*M_PI_180;}
	void setLatitude(const double lat) { latitude=lat*M_PI_180;}
	void setDistance(const double dist){ distance=dist;}
	void addToLongitude(const double dlong){ longitude+=dlong*M_PI_180; }
	void addToLatitude(const double dlat)  { latitude=qBound(-M_PI_2, latitude+dlat*M_PI_180, M_PI_2);}
	void addToDistance(const double ddist) { distance=qBound(0.01, distance+ddist, 50.);}

private:
	double distance;   //! distance to parent planet center, AU
	double longitude;  //! longitude [radians]
	double latitude;   //! latitude [radians]
};
#endif // ORBIT_HPP

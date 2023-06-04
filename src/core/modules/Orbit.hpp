// orbit.h
//
// Initial structure of orbit computation; EllipticalOrbit: Copyright (C) 2001, Chris Laurel <claurel@shatters.net>
// CometOrbit: Copyright (c) 2007,2008 Johannes Gajdosik
//             Amendments (c) 2013 Georg Zotti
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

//! KeplerOrbit describes an undisturbed orbit in a two-body system.
//! This is used for minor bodies orbiting the sun, but also for planet moons.
//! Orbital elements are considered valid for a relatively short time span (orbitGood)
//! around epoch only and should be updated periodically,
//! because the other planets perturbate the orbiting bodies.
//! To avoid using outdated elements, the KeplerOrbit object can be queried
//! using objectDateValid(JDE) whether it makes sense to assume the retrieved positions
//! are close enough to reality to find the object in a telescope. Another test is
//! objectDateGoodEnoughForOrbits(JDE), which test a bit more relaxed, for the sake
//! of retrieving positions for graphics.
//! @note This class was called CometOrbit previously, but was now recombined
//! with the former EllipticalOrbit class. They did almost the same.
//! @note Algorithms from:
//!   - Meeus: Astronomical Algorithms 1998
//!   - Heafner: Fundamental Ephemeris Computations 1999
//! @todo Add state vector equations from Heafner to create orbits from position+velocity,
//! or change orbits from velocity changes.
class KeplerOrbit : public Orbit {
public:
    //! Constructor.
    //! @param epochJDE                JDE epoch of orbital elements.
    //! @param pericenterDistance      [AU] pericenter distance
    //! @param eccentricity            0..>1 (>>1 for Interstellar objects)
    //! @param inclination             [radians]
    //! @param ascendingNode           [radians]
    //! @param argOfPerhelion          [radians]
    //! @param timeAtPerihelion        JDE
    //! @param orbitGoodDays           [earth days] can be used to exclude computation for dates too far outside epoch.
    //!                                 0: always good (use that for planet moons. Not really correct, but most users won't care)
    //!                                -1: signal "auto-compute to 1/2 the orbital period or 1000 days if there is no period [e>=1]")
    //! @param meanMotion              [radians/day] for parabolics, this is W/dt in Heafner's lettering
    //! @param parentRotObliquity      [radians] Comets/Minor Planets only have parent==sun, no need for these? --> Oh yes, these relate VSOP/J2000 eq frames!
    //! @param parentRotAscendingnode  [radians]
    //! @param parentRotJ2000Longitude [radians]
    //! @param centralMass central mass in Solar masses. Velocity value depends on this!
	KeplerOrbit(double epochJDE,
		    double pericenterDistance,
		    double eccentricity,
		    double inclination,
		    double ascendingNode,
		    double argOfPerhelion,
		    double timeAtPerihelion,
		    double orbitGoodDays,
		    double meanMotion,
		    double parentRotObliquity,
		    double parentRotAscendingnode,
		    double parentRotJ2000Longitude,
		    double centralMass = 1.0
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
	//! return whether a position returned for JDE can be regarded accurate enough for telescope use.
	//! This is limited to dates within 1 year or epoch, or within orbitGood around epoch, whichever is smaller.
	//! If orbitGood is zero, this is always true.
	//! @note This will still return false positives after close encounters with major masses which change orbital parameters.
	//! However, it should catch the usual case of outdated orbital elements which should be updated at least yearly.
	bool objectDateValid(const double JDE) const { return ((orbitGood==0.) || (fabs(epochJDE-JDE)<qMin(orbitGood, 365.0))); }
	//! return whether a position returned for JDE would be good enough for at least plotting the orbit.
	//! This is true for dates within orbitGood around epoch.
	//! If orbitGood is zero, this is always true.
	//! @note This relieves conditions of objectDateValid(JDE) somewhat, for the sake of illustratory completeness.
	bool objectDateGoodEnoughForOrbits(const double JDE) const { return ((orbitGood==0.) || (fabs(epochJDE-JDE)<orbitGood)); }
	//! Return minimal and maximal JDE values where this orbit should be used.
	//! @returns the limits where objectDateValid returns true
	Vec2d objectDateValidRange(const bool strict) const;
	//! Calculate sidereal period in days from semi-major axis and central mass. If SMA<=0 (hyperbolic orbit), return 0.
	double calculateSiderealPeriod() const;
	//! @param semiMajorAxis in AU. If SMA<=0 (hyperbolic orbit), return 0.
	//! @param centralMass in units of Solar masses
	static double calculateSiderealPeriod(const double semiMajorAxis, const double centralMass);
	double getEpochJDE() const { return epochJDE; }

private:
	const double epochJDE; //!< epoch (date of validity) of the elements.
	const double q;  //!< pericenter distance [AU]
	const double e;  //!< eccentricity
	const double i;  //!< inclination [radians]
	const double Om; //!< longitude of ascending node [radians]
	const double w;  //!< argument of perihel [radians]
	const double t0; //!< time of perihel, JDE
	const double n;  //!< mean motion (for parabolic orbits: W/dt in Heafner's presentation, ch5.5) [radians/day]
	const double centralMass; //!< Mass in Solar masses. Velocity depends on this.
	double orbitGood; //!< orb. elements are only valid for this time from perihel [days]. Don't draw the object outside. Values <=0 mean "always good" (objects on undisturbed elliptic orbit)
	Vec3d rdot;       //!< velocity vector. Caches velocity from last position computation, [AU/d]
	bool updateTails; //!< flag to signal that comet tails must be recomputed.
	void InitEll(const double dt, double &rCosNu, double &rSinNu);
	void InitPar(const double dt, double &rCosNu, double &rSinNu);
	void InitHyp(const double dt, double &rCosNu, double &rSinNu);
};

//! A pseudo-orbit for "observers" linked to a planet's sphere. It allows setting distance and longitude/latitude in the VSOP87 frame.
//! This class is currently in an experimental state. rotateToVsop87 may need to be set up correctly.
//! The view frame for an observer is correctly oriented when the observer is located on the pseudo-planet's North pole.
//! Positional changes are currently performed with keyboard interaction (see @class StelMovementMgr)
class GimbalOrbit : public Orbit {
public:
	//! Constructor. @param distance in AU, @param longitude in radians, @param latitude in radians.
	GimbalOrbit(double distance, double longitude, double latitude);
	//! Compute position for a (unused) Julian day.
	virtual void positionAtTimevInVSOP87Coordinates(double JDE, double* v) Q_DECL_OVERRIDE;
	//! Returns (pseudo) semimajor axis [AU] of a circular "orbit", i.e., distance.
	double getSemimajorAxis() const Q_DECL_OVERRIDE { return distance; }

	//! Set minimum distance for observers (may depend on central object)
	void setMinDistance(double dist) {minDistance=dist; distance=qMax(distance, minDistance);}

	//! Retrieve observer's longitude in degrees
	double getLongitude() const { return longitude*M_180_PI;}
	//! Retrieve observer's latitude in degrees
	double getLatitude()  const { return latitude*M_180_PI;}
	//! Retrieve observer's distance in AU
	double getDistance()  const { return distance;}
	//! Set observer's longitude in degrees
	void setLongitude(const double lng){ longitude=lng*M_PI_180;}
	//! Set observer's latitude in degrees
	void setLatitude(const double lat) { latitude=lat*M_PI_180;}
	//! Set observer's distance in AU
	void setDistance(const double dist){ distance=dist;}
	//! Incrementally change longitude by @param dlong degrees
	void addToLongitude(const double dlong){ longitude+=dlong*M_PI_180; }
	//! Incrementally change latitude by @param dlat degrees. Clamped to |lat|<(90-delta) with a small delta at the poles.
	void addToLatitude(const double dlat);
	//! Incrementally change distance by @param ddist AU. Clamped to minDistance...50 AU.
	void addToDistance(const double ddist) { distance=qBound(minDistance, distance+ddist, 50.);}

private:
	double distance;   //! distance to parent planet center, AU
	double longitude;  //! longitude [radians]
	double latitude;   //! latitude [radians]
	double minDistance; //! minimum distance. May depend on size of observed object
};
#endif // ORBIT_HPP

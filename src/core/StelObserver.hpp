/*
 * Stellarium
 * Copyright (C) 2002 Fabien Chereau
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

#ifndef STELOBSERVER_HPP
#define STELOBSERVER_HPP

#include "Planet.hpp"
#include "StelLocation.hpp"
#include "VecMath.hpp"

#include <QObject>
#include <QString>
#include <QSharedPointer>


class ArtificialPlanet;

//! @class StelObserver
//! Should be renamed as PlanetBasedObserver and derive from a more generical StelObserver class
class StelObserver : public QObject
{
	Q_OBJECT

public:
	//! Create a new StelObserver instance which is at a fixed Location
	StelObserver(const StelLocation& loc);
	~StelObserver();

	//! Update StelObserver info if needed. Default implementation does nothing.
	//! returns whether we actually changed the position.
	virtual bool update(double) {return false;}

	//! Get the position of the home planet center in the heliocentric VSOP87 frame in AU
	Vec3d getCenterVsop87Pos(void) const;
	//! Get the distance between observer and home planet center in AU.
	//! This is distance &rho; from Meeus, Astron. Algorithms, 2nd edition 1998, ch.11, p.81f.
	//! &rho; is also delivered from getTopographicOffsetFromCenter().v[3];
	double getDistanceFromCenter(void) const;
	//! Get the geocentric rectangular coordinates of the observer in AU, plus geocentric latitude &phi;'.
	//! This is vector &rho; from Meeus, Astron. Algorithms, 2nd edition 1998, ch.11, p.81f.
	//! The first component is &rho; cos &phi;' [AU],
	//! the second component is &rho; sin &phi&' [AU],
	//! the third is &phi;' [radians]
	//! the fourth is &rho; [AU]
	Vec4d getTopographicOffsetFromCenter(void) const;

	//! returns rotation matrix for conversion of alt-azimuthal to equatorial coordinates
	//! For Earth we need JD(UT), for other planets JDE! To be general, just have both in here!
	Mat4d getRotAltAzToEquatorial(double JD, double JDE) const;
	Mat4d getRotEquatorialToVsop87(void) const;

	virtual const QSharedPointer<Planet> getHomePlanet(void) const;

	//! Get the informations on the current location
	virtual const StelLocation& getCurrentLocation() const {return currentLocation;}

	//! Get whether the life of this observer is over, and therefore that it should be changed to the next one
	//! provided by the getNextObserver() method
	virtual bool isObserverLifeOver() const {return false;}

	//! Get whether the location is a moving one.
	virtual bool isTraveling() const {return false;}

	//! Get the next observer to use once the life of this one is over
	virtual StelObserver* getNextObserver() const {return new StelObserver(currentLocation);}

protected:
	StelLocation currentLocation;
	QSharedPointer<Planet> planet;
};

//! @class SpaceShipObserver
//! An observer which moves from from one position to another one and/or from one planet to another one
class SpaceShipObserver : public StelObserver
{
	Q_OBJECT
public:
	SpaceShipObserver(const StelLocation& startLoc, const StelLocation& target, double transitSeconds=1., double timeToGo=-1.0);
	~SpaceShipObserver();

	//! Update StelObserver info if needed. Default implementation does nothing.
	virtual bool update(double deltaTime);
	virtual const QSharedPointer<Planet> getHomePlanet() const;
	virtual bool isObserverLifeOver() const {return timeToGo <= 0.;}
	virtual bool isTraveling() const {return !isObserverLifeOver();}
	virtual StelObserver* getNextObserver() const {return new StelObserver(moveTargetLocation);}

	//! Returns the target location
	StelLocation getTargetLocation() const { return moveTargetLocation; }
	//! Returns the remaining movement time
	double getRemainingTime() const { return timeToGo; }
	//! Returns the total movement time
	double getTransitTime() const { return transitSeconds; }

private:
	StelLocation moveStartLocation;
	StelLocation moveTargetLocation;
	QSharedPointer<Planet> artificialPlanet;
	double timeToGo;
	double transitSeconds;
};

#endif // STELOBSERVER_HPP


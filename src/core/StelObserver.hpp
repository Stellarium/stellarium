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

#ifndef _STELOBSERVER_HPP_
#define _STELOBSERVER_HPP_

#include "StelLocation.hpp"
#include <QObject>
#include <QString>
#include <QSharedPointer>
#include "VecMath.hpp"
#include "Planet.hpp"

class ArtificialPlanet;
class StelObserver;

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
	virtual void update(double) {;}

	//! Get the position of the home planet center in the heliocentric VSOP87 frame in AU
	Vec3d getCenterVsop87Pos(void) const;
	//! Get the distance between observer and home planet center in AU
	double getDistanceFromCenter(void) const;
	Mat4d getRotAltAzToEquatorial(double jd) const;
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
public:
	SpaceShipObserver(const StelLocation& startLoc, const StelLocation& target, double transitSeconds=1.f);
	~SpaceShipObserver();

	//! Update StelObserver info if needed. Default implementation does nothing.
	virtual void update(double deltaTime);
	virtual const QSharedPointer<Planet> getHomePlanet() const;
	virtual bool isObserverLifeOver() const {return timeToGo <= 0.;}
	virtual bool isTraveling() const {return !isObserverLifeOver();}
	virtual StelObserver* getNextObserver() const {return new StelObserver(moveTargetLocation);}

private:
	StelLocation moveStartLocation;
	StelLocation moveTargetLocation;
	QSharedPointer<Planet> artificialPlanet;
	double timeToGo;
	double transitSeconds;
};

#endif // _STELOBSERVER_HPP_


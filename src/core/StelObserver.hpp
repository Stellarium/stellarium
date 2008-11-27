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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef _OBSERVER_HPP_
#define _OBSERVER_HPP_

#include "StelLocation.hpp"
#include <QObject>
#include <QString>
#include "vecmath.h"

class Planet;
class ArtificialPlanet;
class StelObserver;

//! @class StelObserver
//! Should be renamed as PlanetBasedObserver and derive from a more generical StelObserver class
class StelObserver : public QObject
{
	Q_OBJECT;
	
public:
	//! Create a new StelObserver instance which is at a fixed Location
	StelObserver(const StelLocation& loc);
	~StelObserver();

	//! Update StelObserver info if needed. Default implementation does nothing.
	virtual void update(double deltaTime) {;}
	
	Vec3d getCenterVsop87Pos(void) const;
	double getDistanceFromCenter(void) const;
	Mat4d getRotAltAzToEquatorial(double jd) const;
	Mat4d getRotEquatorialToVsop87(void) const;
	
	virtual const Planet* getHomePlanet(void) const {return planet;}
	
	//! Get the informations on the current location
	const StelLocation& getCurrentLocation() const {return currentLocation;}
	
	//! Get whether the life of this observer is over, and therefore that it should be changed to the next one
	//! provided by the getNextObserver() method
	virtual bool isObserverLifeOver() const {return false;}
	
	//! Get the next observer to use once the life of this one is over
	virtual StelObserver* getNextObserver() const {return new StelObserver(currentLocation);}
	
protected:
	StelLocation currentLocation;
	const Planet* planet;    
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
	virtual const Planet* getHomePlanet(void) const {return (isObserverLifeOver() || artificialPlanet==NULL)  ? planet : (Planet*)artificialPlanet;}
	virtual bool isObserverLifeOver() const {return timeToGo <= 0.;}
	virtual StelObserver* getNextObserver() const {return new StelObserver(moveTargetLocation);}
	
private:
	StelLocation moveStartLocation;
	StelLocation moveTargetLocation;
	ArtificialPlanet* artificialPlanet;
	double timeToGo;
	double transitSeconds;
};

#endif // _OBSERVER_HPP_

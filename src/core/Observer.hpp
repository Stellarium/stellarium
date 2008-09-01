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

#include "PlanetLocation.hpp"
#include <QObject>
#include <QString>
#include "vecmath.h"

class SolarSystem;
class Planet;
class ArtificialPlanet;
class QSettings;

//! @TODO Should be renamed as PlanetBasedObserver and derive from a more generical Observer class
class Observer : public QObject
{
	Q_OBJECT;

public:
	Observer(const class SolarSystem &ssystem);
	~Observer();

	void update(int deltaTime);  // for moving observing position 
	
	void init();
	
	//! @param duration in s
	void moveTo(const PlanetLocation& target, double duration=1);
	
	Vec3d getCenterVsop87Pos(void) const;
	double getDistanceFromCenter(void) const;
	Mat4d getRotLocalToEquatorial(double jd) const;
	Mat4d getRotEquatorialToVsop87(void) const;
	
	//! Get the sideral time shifted by the observer longitude
	//! @param jd the Julian Day
	//! @return the locale sideral time in radian
	double getLocalSideralTime(double jd) const;
	
	const Planet *getHomePlanet(void) const;
	
public slots:
	///////////////////////////////////////////////////////////////////////////
	// Method callable from script and GUI
	//! Get the informations on the current location
	const PlanetLocation& getCurrentLocation() const {return currentLocation;}
	//! Set the new current location
	void setPlanetLocation(const PlanetLocation& loc);
	
private:
	//! Set the home planet from its english name
	bool setHomePlanet(const QString& englishName);
	
	void setHomePlanet(const Planet *p,float transitSeconds=2.f);
	
    const SolarSystem &ssystem;

	const Planet* planet;
    ArtificialPlanet *artificialPlanet;
    int timeToGo;
	PlanetLocation currentLocation;

	// for changing position
	bool flagMoveTo;
	float moveToCoef, moveToMult;
	PlanetLocation moveStartLocation;
	PlanetLocation moveTargetLocation;
};

#endif // _OBSERVER_HPP_

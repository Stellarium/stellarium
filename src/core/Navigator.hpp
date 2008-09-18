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

#ifndef _NAVIGATOR_HPP_
#define _NAVIGATOR_HPP_

#include "vecmath.h"

#include "fixx11h.h"
#include <QObject>
#include <QString>
#include <QTime>

// Conversion in standar Julian time format
#define JD_SECOND 0.000011574074074074074074
#define JD_MINUTE 0.00069444444444444444444
#define JD_HOUR   0.041666666666666666666
#define JD_DAY    1.

extern const Mat4d matJ2000ToVsop87;
extern const Mat4d matVsop87ToJ2000;

class Observer;
class StelObject;
class LoadingBar;
class Planet;
class Location;

//! @class Navigator
//! Manages a navigation context.  This includes:
//! - date/time
//! - viewing direction/fov
//! - observer position
//! - coordinate changes
class Navigator : public QObject
{
	Q_OBJECT;
	
public:

	enum ViewingModeType
	{
		ViewHorizon,
		ViewEquator
	};
	
	//! Possible mount modes
	enum MountMode { MountAltAzimuthal, MountEquatorial };
	
	// Create and initialise to default a navigation context
	Navigator();
	~Navigator();

	void init();

	void updateTime(double deltaTime);
	void updateTransformMatrices(void);
	
	//! Set current mount type
	void setMountMode(MountMode m) {setViewingMode((m==MountAltAzimuthal) ? Navigator::ViewHorizon : Navigator::ViewEquator);}
	//! Get current mount type
	MountMode getMountMode(void) {return ((getViewingMode()==Navigator::ViewHorizon) ? MountAltAzimuthal : MountEquatorial);}


	//! Get vision direction
	const Vec3d& getEquVision(void) const {return equVision;}
	const Vec3d& getJ2000EquVision(void) const {return J2000EquVision;}
	const Vec3d& getLocalVision(void) const {return localVision;}
	void setLocalVision(const Vec3d& _pos);
	void setEquVision(const Vec3d& _pos);
	void setJ2000EquVision(const Vec3d& _pos);
	
	//! Get the informations on the current location
	const Location& getCurrentLocation() const;
	//! Smoothly move the observer to the given location
	//! @param duration direction of view move duration in s
	//! @param durationIfPlanetChange direction of view + planet travel move duration in s.
	//! This is used only if the destination planet is different from the starting one.
	void moveObserverTo(const Location& target, double duration=1., double durationIfPlanetChange=1.);
	
	//! Get the sideral time shifted by the observer longitude
	//! @return the locale sideral time in radian
	double getLocalSideralTime() const;
	
	const Planet *getHomePlanet(void) const;

	//! Return the observer heliocentric position
	Vec3d getObserverHelioPos(void) const;

	//! Transform vector from local coordinate to equatorial
	Vec3d localToEarthEqu(const Vec3d& v) const { return matLocalToEarthEqu*v; }

	//! Transform vector from equatorial coordinate to local
	Vec3d earthEquToLocal(const Vec3d& v) const { return matEarthEquToLocal*v; }

	Vec3d earthEquToJ2000(const Vec3d& v) const { return matEarthEquToJ2000*v; }
	Vec3d j2000ToEarthEqu(const Vec3d& v) const { return matJ2000ToEarthEqu*v; }
	Vec3d j2000ToLocal(const Vec3d& v) const { return matJ2000ToLocal*v; }
	
	//! Transform vector from heliocentric coordinate to local
	Vec3d helioToLocal(const Vec3d& v) const { return matHelioToLocal*v; }

	//! Transform vector from heliocentric coordinate to earth equatorial,
	//! only needed in meteor.cpp
	Vec3d helioToEarthEqu(const Vec3d& v) const { return matHelioToEarthEqu*v; }

	//! Transform vector from heliocentric coordinate to false equatorial : equatorial
	//! coordinate but centered on the observer position (usefull for objects close to earth)
	Vec3d helioToEarthPosEqu(const Vec3d& v) const { return matLocalToEarthEqu*matHelioToLocal*v; }


	//! Return the modelview matrix for some coordinate systems
	const Mat4d& getHelioToEyeMat(void) const {return matHelioToEye;}
	const Mat4d& getEarthEquToEyeMat(void) const {return matEarthEquToEye;}
	const Mat4d& getLocalToEyeMat(void) const {return matLocalToEye;}
	const Mat4d& getJ2000ToEyeMat(void) const {return matJ2000ToEye;}

	void setViewingMode(ViewingModeType viewMode);
	ViewingModeType getViewingMode(void) const {return viewingMode;}

	const Vec3d& getinitViewPos() {return initViewPos;}
	
	//! Return the preset sky time in JD
	double getPresetSkyTime() const {return presetSkyTime;}
	void setPresetSkyTime(double d) {presetSkyTime=d;}
	
	//! Return the startup mode, can be preset|Preset or anything else
	QString getStartupTimeMode() {return startupTimeMode;}
	void setStartupTimeMode(const QString& s);
	
	//! Update the modelview matrices
	void updateModelViewMat(void);
	
public slots:
	//! Toggle current mount mode between equatorial and altazimutal
	void toggleMountMode() {if (getMountMode()==MountAltAzimuthal) setMountMode(MountEquatorial); else setMountMode(MountAltAzimuthal);}
	//! Define whether we should use equatorial mount or altazimutal
	void setEquatorialMount(bool b) {setMountMode(b ? MountEquatorial : MountAltAzimuthal);}
	
	//! Set the current date in Julian Day
	void setJDay(double JD) {JDay=JD;}
	//! Get the current date in Julian Day
	double getJDay() const {return JDay;}
	
	//! Set time speed in JDay/sec
	void setTimeSpeed(double ts) {timeSpeed=ts;}
	//! Get time speed in JDay/sec
	double getTimeSpeed() const {return timeSpeed;}
	
	//! Increase the time speed
	void increaseTimeSpeed();
	//! Decrease the time speed
	void decreaseTimeSpeed();
	
	//! Set real time speed, i.e. 1 sec/sec
	void setRealTimeSpeed() {setTimeSpeed(JD_SECOND);}
	//! Get whether it is real time speed, i.e. 1 sec/sec
	bool getRealTimeSpeed() const {return (fabs(timeSpeed-JD_SECOND)<0.0000001);}
	
	//! Set stellarium time to current real world time
	void setTimeNow();
	//! Set the time to some value, leaving the day the same.
	void setTodayTime(const QTime& target);
	//! Get wether the current stellarium time is the real world time
	bool getIsTimeNow() const;

	//! get the initial "today time" from the config file
	QTime getInitTodayTime(void);
	//! set the initial "today time" in the config file for "today" initial time type
	void setInitTodayTime(const QTime& t);
	//! get the initial date and time from the config file for "preset" initial time type
	QDateTime getInitDateTime(void);
	//! set the initial date and time from the config file for "preset" initial time type
	void setInitDateTime(const QDateTime& t);

	//! Add one [Earth, solar] hour to the current simulation time.
	void addHour() {addSolarDays(0.04166666666666666667);}
	//! Add one [Earth, solar] day to the current simulation time.
	void addDay() {addSolarDays(1.0);}
	//! Add one [Earth, solar] week to the current simulation time.
	void addWeek() {addSolarDays(7.0);}

	//! Add one sidereal day to the simulation time. The length of time depends
	//! on the current planetary body on which the observer is located.
	void addSiderealDay() {addSiderealDays(1.0);}
	//! Add one sidereal week to the simulation time. The length of time depends
	//! on the current planetary body on which the observer is located.
	void addSiderealWeek() {addSiderealDays(7.0);}

	//! Subtract one [Earth, solar] hour to the current simulation time.
	void subtractHour() {addSolarDays(-0.04166666666666666667);}
	//! Subtract one [Earth, solar] day to the current simulation time.
	void subtractDay() {addSolarDays(-1.0);}
	//! Subtract one [Earth, solar] week to the current simulation time.
	void subtractWeek() {addSolarDays(-7.0);}

	//! Subtract one sidereal week to the simulation time. The length of time depends
	//! on the current planetary body on which the observer is located.
	void subtractSiderealDay() {addSiderealDays(-1.0);}
	//! Subtract one sidereal week to the simulation time. The length of time depends
	//! on the current planetary body on which the observer is located.
	void subtractSiderealWeek() {addSiderealDays(-7.0);}

	//! Add a number of Earth Solar days to the current simulation time
	//! @param d the decimal number of days to add (use negative values to subtract)
	void addSolarDays(double d);
	//! Add a number of sidereal days to the current simulation time, 
	//! based on the observer body's rotational period.
	//! @param d the decimal number of sidereal days to add (use negative values to subtract)
	void addSiderealDays(double d);
	
	//! Move the observer to the selected object. This will only do something if
	//! the selected object is of the correct type - i.e. a planet.
	void moveObserverToSelected(void);
	
	//! Get the location used by default at startup
	QString getDefaultLocationID() const {return defaultLocationID;}
	//! Set the location to use by default at startup
	void setDefaultLocationID(const QString& id);
	
	//! Sets the initial direction of view to the current altitude and azimuth.
	//! Note: Updates the configuration file.
	void setInitViewDirectionToCurrent(void);
	
private:
	// Matrices used for every coordinate transfo
	Mat4d matHelioToLocal;    // Transform from Heliocentric to Observer local coordinate
	Mat4d matLocalToHelio;    // Transform from Observer local coordinate to Heliocentric
	Mat4d matLocalToEarthEqu; // Transform from Observer local coordinate to Earth Equatorial
	Mat4d matEarthEquToLocal; // Transform from Observer local coordinate to Earth Equatorial
	Mat4d matHelioToEarthEqu; // Transform from Heliocentric to earth equatorial coordinate
	Mat4d matEarthEquToJ2000;
	Mat4d matJ2000ToEarthEqu;
	Mat4d matJ2000ToLocal;
	
	Mat4d matLocalToEye;      // Modelview matrix for observer local drawing
	Mat4d matEarthEquToEye;   // Modelview matrix for geocentric equatorial drawing
	Mat4d matJ2000ToEye;      // precessed version
	Mat4d matHelioToEye;      // Modelview matrix for heliocentric equatorial drawing

	// Vision variables
	// Viewing direction in local and equatorial coordinates
	Vec3d localVision, equVision, J2000EquVision;

	// Time variable
	double timeSpeed;        // Positive : forward, Negative : Backward, 1 = 1sec/sec
	double JDay;              // Curent time in Julian day

	double presetSkyTime;
	QString startupTimeMode;

	// The ID of the default startup location
	QString defaultLocationID;
			
	// Position variables
	Observer* position;

	Vec3d initViewPos;        // Default viewing direction

	// defines if view corrects for horizon, or uses equatorial coordinates
	ViewingModeType viewingMode;
};

#endif // _NAVIGATOR_HPP_

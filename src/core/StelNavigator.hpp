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
#include <QDateTime>

// Conversion in standar Julian time format
#define JD_SECOND 0.000011574074074074074074
#define JD_MINUTE 0.00069444444444444444444
#define JD_HOUR   0.041666666666666666666
#define JD_DAY    1.

class StelObserver;
class StelObject;
class StelLoadingBar;
class Planet;
class StelLocation;

//! @class StelNavigator
//! Manages a navigation context.  This includes:
//! - date/time
//! - viewing direction/fov
//! - observer position
//! - coordinate changes
class StelNavigator : public QObject
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
	StelNavigator();
	~StelNavigator();

	void init();

	void updateTime(double deltaTime);
	void updateTransformMatrices(void);
	
	//! Set current mount type
	void setMountMode(MountMode m) {setViewingMode((m==MountAltAzimuthal) ? StelNavigator::ViewHorizon : StelNavigator::ViewEquator);}
	//! Get current mount type
	MountMode getMountMode(void) {return ((getViewingMode()==StelNavigator::ViewHorizon) ? MountAltAzimuthal : MountEquatorial);}

	//! Get vision direction
	const Vec3d& getEquinoxEquVisionDirection(void) const {return earthEquVisionDirection;}
	const Vec3d& getJ2000EquVisionDirection(void) const {return J2000EquVisionDirection;}
	const Vec3d& getAltAzVisionDirection(void) const {return altAzVisionDirection;}
	void setAltAzVisionDirection(const Vec3d& _pos);
	void setEquinoxEquVisionDirection(const Vec3d& _pos);
	void setJ2000EquVisionDirection(const Vec3d& _pos);
	
	//! Get the informations on the current location
	const StelLocation& getCurrentLocation() const;
	//! Smoothly move the observer to the given location
	//! @param duration direction of view move duration in s
	//! @param durationIfPlanetChange direction of view + planet travel move duration in s.
	//! This is used only if the destination planet is different from the starting one.
	void moveObserverTo(const StelLocation& target, double duration=1., double durationIfPlanetChange=1.);
	
	//! Get the sideral time shifted by the observer longitude
	//! @return the locale sideral time in radian
	double getLocalSideralTime() const;
	
	const Planet *getHomePlanet(void) const;

	//! Return the observer heliocentric ecliptic position
	Vec3d getObserverHeliocentricEclipticPos(void) const;

	//! Transform vector from altazimuthal coordinate to equatorial
	Vec3d altAzToEquinoxEqu(const Vec3d& v) const { return matAltAzToEquinoxEqu*v; }

	//! Transform vector from equatorial coordinate to altazimuthal
	Vec3d equinoxEquToAltAz(const Vec3d& v) const { return matEquinoxEquToAltAz*v; }
	Vec3d equinoxEquToJ2000(const Vec3d& v) const { return matEquinoxEquToJ2000*v; }
	Vec3d j2000ToEquinoxEqu(const Vec3d& v) const { return matJ2000ToEquinoxEqu*v; }
	Vec3d j2000ToAltAz(const Vec3d& v) const { return matJ2000ToAltAz*v; }
	
	//! Transform vector from heliocentric ecliptic coordinate to altazimuthal
	Vec3d heliocentricEclipticToAltAz(const Vec3d& v) const { return matHeliocentricEclipticToAltAz*v; }

	//! Transform from heliocentric coordinate to equatorial at current equinox (for the planet where the observer stands)
	Vec3d heliocentricEclipticToEquinoxEqu(const Vec3d& v) const { return matHeliocentricEclipticToEquinoxEqu*v; }

	//! Transform vector from heliocentric coordinate to false equatorial : equatorial
	//! coordinate but centered on the observer position (usefull for objects close to earth)
	Vec3d heliocentricEclipticToEarthPosEquinoxEqu(const Vec3d& v) const { return matAltAzToEquinoxEqu*matHeliocentricEclipticToAltAz*v; }

	//! Return the modelview matrix for some coordinate systems
	const Mat4d& getHeliocentricEclipticModelViewMat(void) const {return matHeliocentricEclipticModelView;}
	const Mat4d& getEquinoxEquModelViewMat(void) const {return matEquinoxEquModelView;}
	const Mat4d& getAltAzModelViewMat(void) const {return matAltAzModelView;}
	const Mat4d& getJ2000ModelViewMat(void) const {return matJ2000ModelView;}

	void setViewingMode(ViewingModeType viewMode);
	ViewingModeType getViewingMode(void) const {return viewingMode;}

	//! Return the inital viewing direction in altazimuthal coordinates
	const Vec3d& getInitViewingDirection() {return initViewPos;}
	
	//! Return the preset sky time in JD
	double getPresetSkyTime() const {return presetSkyTime;}
	//! Set the preset sky time from a JD
	void setPresetSkyTime(double d) {presetSkyTime=d;}
	
	//! Return the startup mode, can be preset|Preset or anything else
	QString getStartupTimeMode() {return startupTimeMode;}
	void setStartupTimeMode(const QString& s);
	
	//! Rotation matrix from equatorial J2000 to ecliptic (Vsop87)
	static const Mat4d matJ2000ToVsop87;
	//! Rotation matrix from ecliptic (Vsop87) to equatorial J2000
	static const Mat4d matVsop87ToJ2000;
	
public slots:
	//! Toggle current mount mode between equatorial and altazimuthal
	void toggleMountMode() {if (getMountMode()==MountAltAzimuthal) setMountMode(MountEquatorial); else setMountMode(MountAltAzimuthal);}
	//! Define whether we should use equatorial mount or altazimuthal
	void setEquatorialMount(bool b) {setMountMode(b ? MountEquatorial : MountAltAzimuthal);}
	
	//! Set the current date in Julian Day
	void setJDay(double JD) {JDay=JD;}
	//! Get the current date in Julian Day
	double getJDay() const {return JDay;}
	
	//! Set time speed in JDay/sec
	void setTimeRate(double ts) {timeSpeed=ts;}
	//! Get time speed in JDay/sec
	double getTimeRate() const {return timeSpeed;}
	
	//! Increase the time speed
	void increaseTimeSpeed();
	//! Decrease the time speed
	void decreaseTimeSpeed();
	
	//! Set time speed to 0, i.e. freeze the passage of simulation time
	void setZeroTimeSpeed() {setTimeRate(0);}
	//! Set real time speed, i.e. 1 sec/sec
	void setRealTimeSpeed() {setTimeRate(JD_SECOND);}
	//! Get whether it is real time speed, i.e. 1 sec/sec
	bool getRealTimeSpeed() const {return (fabs(timeSpeed-JD_SECOND)<0.0000001);}
	
	//! Set stellarium time to current real world time
	void setTimeNow();
	//! Set the time to some value, leaving the day the same.
	void setTodayTime(const QTime& target);
	//! Get wether the current stellarium time is the real world time
	bool getIsTimeNow() const;

	//! get the initial "today time" from the config file
	QTime getInitTodayTime(void) {return initTodayTime;}
	//! set the initial "today time" from the config file
	void setInitTodayTime(const QTime& t) {initTodayTime=t;}
	//! Set the preset sky time from a QDateTime
	void setPresetSkyTime(QDateTime dt);

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
	//! Update the modelview matrices
	void updateModelViewMat(void);
	
	// Matrices used for every coordinate transfo
	Mat4d matHeliocentricEclipticToAltAz;	// Transform from heliocentric ecliptic (Vsop87) to observer-centric altazimuthal coordinate
	Mat4d matAltAzToHeliocentricEcliptic;	// Transform from observer-centric altazimuthal coordinate to heliocentric ecliptic (Vsop87)
	Mat4d matAltAzToEquinoxEqu;				// Transform from observer-centric altazimuthal coordinate to Earth Equatorial
	Mat4d matEquinoxEquToAltAz;				// Transform from observer-centric altazimuthal coordinate to Earth Equatorial
	Mat4d matHeliocentricEclipticToEquinoxEqu;// Transform from heliocentric ecliptic (Vsop87) to earth equatorial coordinate
	Mat4d matEquinoxEquToJ2000;
	Mat4d matJ2000ToEquinoxEqu;
	Mat4d matJ2000ToAltAz;
	
	Mat4d matAltAzModelView;					// Modelview matrix for observer-centric altazimuthal drawing
	Mat4d matEquinoxEquModelView;					// Modelview matrix for observer-centric equatorial drawing
	Mat4d matJ2000ModelView;					// Modelview matrix for observer-centric J2000 equatorial drawing
	Mat4d matHeliocentricEclipticModelView;		// Modelview matrix for heliocentric ecliptic (Vsop87) drawing

	// Vision variables
	// Viewing direction in altazimuthal and equatorial coordinates
	Vec3d altAzVisionDirection, earthEquVisionDirection, J2000EquVisionDirection;

	// Time variable
	double timeSpeed;        // Positive : forward, Negative : Backward, 1 = 1sec/sec
	double JDay;              // Curent time in Julian day

	double presetSkyTime;
	QTime initTodayTime;
	QString startupTimeMode;

	// The ID of the default startup location
	QString defaultLocationID;
			
	// Position variables
	StelObserver* position;

	Vec3d initViewPos;        // Default viewing direction

	// defines if view corrects for horizon, or uses equatorial coordinates
	ViewingModeType viewingMode;
};

#endif // _NAVIGATOR_HPP_

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

#ifndef _STELNAVIGATOR_HPP_
#define _STELNAVIGATOR_HPP_

#include "VecMath.hpp"
#include "StelLocation.hpp"

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

//! @class StelNavigator
//! Manages a navigation context.  This includes:
//! - date/time
//! - viewing direction/fov
//! - observer position
//! - coordinate changes
class StelNavigator : public QObject
{
	Q_OBJECT

public:
	// Create and initialise to default a navigation context
	StelNavigator();
	~StelNavigator();

	void init();

	void updateTime(double deltaTime);
	void updateTransformMatrices(void);

	//! Get vision direction
	void lookAtJ2000(const Vec3d& pos, const Vec3d& up);

	//! Get the informations on the current location
	const StelLocation& getCurrentLocation() const;

	//! Smoothly move the observer to the given location
	//! @param target the target location
	//! @param duration direction of view move duration in s
	//! @param durationIfPlanetChange direction of view + planet travel move duration in s.
	//! This is used only if the destination planet is different from the starting one.
	void moveObserverTo(const StelLocation& target, double duration=1., double durationIfPlanetChange=1.);

	//! Get the sideral time shifted by the observer longitude
	//! @return the locale sideral time in radian
	double getLocalSideralTime() const;

	//! Get the duration of a sideral day for the current observer in day.
	double getLocalSideralDayLength() const;

	//! Return the observer heliocentric ecliptic position
	Vec3d getObserverHeliocentricEclipticPos() const;


	Vec3d altAzToEquinoxEqu(const Vec3d& v) const {return matAltAzToEquinoxEqu*v;}
	Vec3d equinoxEquToAltAz(const Vec3d& v) const {return matEquinoxEquToAltAz*v;}
	Vec3d equinoxEquToJ2000(const Vec3d& v) const {return matEquinoxEquToJ2000*v;}
	Vec3d altAzToJ2000(const Vec3d& v) const {return matEquinoxEquToJ2000*matAltAzToEquinoxEqu*v;}
	Vec3d galacticToJ2000(const Vec3d& v) const {return matGalacticToJ2000*v;}
	Vec3d j2000ToEquinoxEqu(const Vec3d& v) const {return matJ2000ToEquinoxEqu*v;}
	Vec3d j2000ToAltAz(const Vec3d& v) const {return matJ2000ToAltAz*v;}
	Vec3d j2000ToGalactic(const Vec3d& v) const {return matJ2000ToGalactic*v;}
	//! Transform vector from heliocentric ecliptic coordinate to altazimuthal
	Vec3d heliocentricEclipticToAltAz(const Vec3d& v) const {return matHeliocentricEclipticToAltAz*v;}
	//! Transform from heliocentric coordinate to equatorial at current equinox (for the planet where the observer stands)
	Vec3d heliocentricEclipticToEquinoxEqu(const Vec3d& v) const {return matHeliocentricEclipticToEquinoxEqu*v;}
	//! Transform vector from heliocentric coordinate to false equatorial : equatorial
	//! coordinate but centered on the observer position (usefull for objects close to earth)
	Vec3d heliocentricEclipticToEarthPosEquinoxEqu(const Vec3d& v) const {return matAltAzToEquinoxEqu*matHeliocentricEclipticToAltAz*v;}

	//! Get the modelview matrix for heliocentric ecliptic (Vsop87) drawing
	const Mat4d getHeliocentricEclipticModelViewMat() const {return matAltAzModelView*matHeliocentricEclipticToAltAz;}
	//! Get the modelview matrix for observer-centric ecliptic (Vsop87) drawing
	const Mat4d getObservercentricEclipticModelViewMat() const {return matAltAzModelView*matJ2000ToAltAz*matVsop87ToJ2000;}
	//! Get the modelview matrix for observer-centric equatorial at equinox drawing
	const Mat4d getEquinoxEquModelViewMat() const {return matAltAzModelView*matEquinoxEquToAltAz;}
	//! Get the modelview matrix for observer-centric altazimuthal drawing
	const Mat4d& getAltAzModelViewMat() const {return matAltAzModelView;}
	//! Get the modelview matrix for observer-centric J2000 equatorial drawing
	const Mat4d getJ2000ModelViewMat() const {return matAltAzModelView*matEquinoxEquToAltAz*matJ2000ToEquinoxEqu;}
	//! Get the modelview matrix for observer-centric Galactic equatorial drawing
	const Mat4d getGalacticModelViewMat() const {return getJ2000ModelViewMat()*matGalacticToJ2000;}

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
	//! Rotation matrix from J2000 to Galactic reference frame, using FITS convention.
	static const Mat4d matJ2000ToGalactic;
	//! Rotation matrix from J2000 to Galactic reference frame, using FITS convention.
	static const Mat4d matGalacticToJ2000;

public slots:
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
	//! Increase the time speed, but not as much as with increaseTimeSpeed()
	void increaseTimeSpeedLess();
	//! Decrease the time speed but not as much as with decreaseTimeSpeed()
	void decreaseTimeSpeedLess();

	//! Set time speed to 0, i.e. freeze the passage of simulation time
	void setZeroTimeSpeed() {setTimeRate(0);}
	//! Set real time speed, i.e. 1 sec/sec
	void setRealTimeSpeed() {setTimeRate(JD_SECOND);}
	//! Set real time speed or pause simulation if we are already in realtime speed.
	void toggleRealTimeSpeed() {(!getRealTimeSpeed()) ? setRealTimeSpeed() : setZeroTimeSpeed();}
	//! Get whether it is real time speed, i.e. 1 sec/sec
	bool getRealTimeSpeed() const {return (fabs(timeSpeed-JD_SECOND)<0.0000001);}

	//! Set stellarium time to current real world time
	void setTimeNow();
	//! Set the time to some value, leaving the day the same.
	void setTodayTime(const QTime& target);
	//! Get whether the current stellarium time is the real world time
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
	void moveObserverToSelected();

	//! Get the location used by default at startup
	QString getDefaultLocationID() const {return defaultLocationID;}
	//! Set the location to use by default at startup
	void setDefaultLocationID(const QString& id);

signals:
	//! This signal is emitted when the observer location has changed.
	void locationChanged(StelLocation);

private:
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
};

#endif // _STELNAVIGATOR_HPP_

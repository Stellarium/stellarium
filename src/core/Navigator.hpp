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

#ifndef _NAVIGATOR_H_
#define _NAVIGATOR_H_

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

extern const Mat4d mat_j2000_to_vsop87;
extern const Mat4d mat_vsop87_to_j2000;

class Observer;
class StelObject;
class LoadingBar;
class Planet;

// Class which manages a navigation context
// Manage date/time, viewing direction/fov, observer position, and coordinate changes
class Navigator : public QObject
{
	Q_OBJECT;
	
public:

	enum ViewingModeType
	{
		VIEW_HORIZON,
		VIEW_EQUATOR
	};
	
	//! Possible mount modes
	enum MOUNT_MODE { MOUNT_ALTAZIMUTAL, MOUNT_EQUATORIAL };
	
	// Create and initialise to default a navigation context
	Navigator(Observer* obs);
	~Navigator();

	void init();

	void updateTime(double deltaTime);
	void updateTransformMatrices(void);
	
	//! Set current mount type
	void setMountMode(MOUNT_MODE m) {setViewingMode((m==MOUNT_ALTAZIMUTAL) ? Navigator::VIEW_HORIZON : Navigator::VIEW_EQUATOR);}
	//! Get current mount type
	MOUNT_MODE getMountMode(void) {return ((getViewingMode()==Navigator::VIEW_HORIZON) ? MOUNT_ALTAZIMUTAL : MOUNT_EQUATORIAL);}


	// Get vision direction
	const Vec3d& getEquVision(void) const {return equ_vision;}
	const Vec3d& getPrecEquVision(void) const {return prec_equ_vision;}
	const Vec3d& getLocalVision(void) const {return local_vision;}
	void setLocalVision(const Vec3d& _pos);
	void setEquVision(const Vec3d& _pos);
	void setPrecEquVision(const Vec3d& _pos);
	
	const Planet *getHomePlanet(void) const;

	// Return the observer heliocentric position
	Vec3d getObserverHelioPos(void) const;

	// Transform vector from local coordinate to equatorial
	Vec3d local_to_earth_equ(const Vec3d& v) const { return mat_local_to_earth_equ*v; }

	// Transform vector from equatorial coordinate to local
	Vec3d earth_equ_to_local(const Vec3d& v) const { return mat_earth_equ_to_local*v; }

	Vec3d earth_equ_to_j2000(const Vec3d& v) const { return mat_earth_equ_to_j2000*v; }
	Vec3d j2000_to_earth_equ(const Vec3d& v) const { return mat_j2000_to_earth_equ*v; }

	// Transform vector from heliocentric coordinate to local
	Vec3d helio_to_local(const Vec3d& v) const { return mat_helio_to_local*v; }

	// Transform vector from heliocentric coordinate to earth equatorial,
	// only needed in meteor.cpp
	Vec3d helio_to_earth_equ(const Vec3d& v) const { return mat_helio_to_earth_equ*v; }

	// Transform vector from heliocentric coordinate to false equatorial : equatorial
	// coordinate but centered on the observer position (usefull for objects close to earth)
	Vec3d helio_to_earth_pos_equ(const Vec3d& v) const { return mat_local_to_earth_equ*mat_helio_to_local*v; }


	// Return the modelview matrix for some coordinate systems
	const Mat4d& get_helio_to_eye_mat(void) const {return mat_helio_to_eye;}
	const Mat4d& get_earth_equ_to_eye_mat(void) const {return mat_earth_equ_to_eye;}
	const Mat4d& get_local_to_eye_mat(void) const {return mat_local_to_eye;}
	const Mat4d& get_j2000_to_eye_mat(void) const {return mat_j2000_to_eye;}

	void setViewingMode(ViewingModeType view_mode);
	ViewingModeType getViewingMode(void) const {return viewing_mode;}

	const Vec3d& getinitViewPos() {return initViewPos;}
	
	//! Return the preset sky time in JD
	double getPresetSkyTime() const {return PresetSkyTime;}
	void setPresetSkyTime(double d) {PresetSkyTime=d;}
	
	//! Return the startup mode, can be preset|Preset or anything else
	QString getStartupTimeMode() {return StartupTimeMode;}
	void setStartupTimeMode(const QString& s);
	
	// Update the modelview matrices
	void updateModelViewMat(void);
	
public slots:
	//! Toggle current mount mode between equatorial and altazimutal
	void toggleMountMode() {if (getMountMode()==MOUNT_ALTAZIMUTAL) setMountMode(MOUNT_EQUATORIAL); else setMountMode(MOUNT_ALTAZIMUTAL);}
	//! Define whether we should use equatorial mount or altazimutal
	void setEquatorialMount(bool b) {setMountMode(b ? MOUNT_EQUATORIAL : MOUNT_ALTAZIMUTAL);}
	
	//! Set the current date in Julian Day
	void setJDay(double JD) {JDay=JD;}
	//! Get the current date in Julian Day
	double getJDay() const {return JDay;}
	
	//! Set time speed in JDay/sec
	void setTimeSpeed(double ts) {time_speed=ts;}
	//! Get time speed in JDay/sec
	double getTimeSpeed() const {return time_speed;}
	
	//! Increase the time speed
	void increaseTimeSpeed();
	//! Decrease the time speed
	void decreaseTimeSpeed();
	
	//! Set real time speed, i.e. 1 sec/sec
	void setRealTimeSpeed() {setTimeSpeed(JD_SECOND);}
	//! Get whether it is real time speed, i.e. 1 sec/sec
	bool getRealTimeSpeed() const {return (fabs(time_speed-JD_SECOND)<0.0000001);}
	
	//! Set stellarium time to current real world time
	void setTimeNow();
	//! Set the time to some value, leaving the day the same.
	void setTodayTime(const QTime& target);
	//! Get wether the current stellarium time is the real world time
	bool getIsTimeNow() const;

	//! get the initial "today time" from the config file
	QTime getInitTodayTime(void);
	//! set the initial "today time" in the config file
	void setInitTodayTime(const QTime& t);

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
	//! Move the observer to the selected objejct. This will only do something if
	//! the selected object is of the correct type - i.e. a planet.
	void moveObserverToSelected(void);
	//! Sets the initial direction of view to the current altitude and azimuth.
	//! Note: Updates the configuration file.
	void setInitViewDirectionToCurrent(void);
	
private:
	// Matrices used for every coordinate transfo
	Mat4d mat_helio_to_local;		// Transform from Heliocentric to Observer local coordinate
	Mat4d mat_local_to_helio;		// Transform from Observer local coordinate to Heliocentric
	Mat4d mat_local_to_earth_equ;	// Transform from Observer local coordinate to Earth Equatorial
	Mat4d mat_earth_equ_to_local;	// Transform from Observer local coordinate to Earth Equatorial
	Mat4d mat_helio_to_earth_equ;	// Transform from Heliocentric to earth equatorial coordinate
	Mat4d mat_earth_equ_to_j2000;
	Mat4d mat_j2000_to_earth_equ;

	Mat4d mat_local_to_eye;			// Modelview matrix for observer local drawing
	Mat4d mat_earth_equ_to_eye;		// Modelview matrix for geocentric equatorial drawing
	Mat4d mat_j2000_to_eye;	// precessed version
	Mat4d mat_helio_to_eye;			// Modelview matrix for heliocentric equatorial drawing

	// Vision variables
	Vec3d local_vision, equ_vision, prec_equ_vision;	// Viewing direction in local and equatorial coordinates

	// Time variable
	double time_speed;			// Positive : forward, Negative : Backward, 1 = 1sec/sec
	double JDay;        			// Curent time in Julian day

	double PresetSkyTime;
	QString StartupTimeMode;

	// Position variables
	Observer* position;

	Vec3d initViewPos;				// Default viewing direction

	ViewingModeType viewing_mode;   // defines if view corrects for horizon, or uses equatorial coordinates
};

#endif //_NAVIGATOR_H_

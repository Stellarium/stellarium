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

#include "SDL_opengl.h"

#include "stellarium.h"
#include "Observer.hpp"
#include "vecmath.h"

// Conversion in standar Julian time format
#define JD_SECOND 0.000011574074074074074074
#define JD_MINUTE 0.00069444444444444444444
#define JD_HOUR   0.041666666666666666666
#define JD_DAY    1.

extern const Mat4d mat_j2000_to_vsop87;
extern const Mat4d mat_vsop87_to_j2000;

class StelObject;
class LoadingBar;

// Class which manages a navigation context
// Manage date/time, viewing direction/fov, observer position, and coordinate changes
class Navigator
{
public:

	enum VIEWING_MODE_TYPE
	{
		VIEW_HORIZON,
		VIEW_EQUATOR
	};
	// Create and initialise to default a navigation context
	Navigator(Observer* obs);
    virtual ~Navigator();

	virtual void init(const InitParser& conf, LoadingBar& lb);

	void updateTime(int delta_time);
	void updateTransformMatrices(void);
	void updateVisionVector(int delta_time,const StelObject &selected);

	// Move to the given position in equatorial or local coordinate depending on _local_pos value
	void moveTo(const Vec3d& _aim, float move_duration = 1., bool _local_pos = false, int zooming = 0);

	// Time controls
	//! Set the current date in Julian Day
	void setJDay(double JD) {JDay=JD;}
	//! Get the current date in Julian Day
	double getJDay(void) const {return JDay;}
	
	//! Set time speed in JDay/sec
	void setTimeSpeed(double ts) {time_speed=ts;}
	//! Get time speed in JDay/sec
	double getTimeSpeed(void) const {return time_speed;}

	// Flags controls
	void setFlagTraking(int v) {flag_traking=v;}
	int getFlagTraking(void) const {return flag_traking;}
	void setFlagLockEquPos(int v) {flag_lock_equ_pos=v;}
	int getFlagLockEquPos(void) const {return flag_lock_equ_pos;}

	// Get vision direction
	const Vec3d& getPrecEquVision(void) const {return prec_equ_vision;}
	const Vec3d& getLocalVision(void) const {return local_vision;}
	void setLocalVision(const Vec3d& _pos);

	const Planet *getHomePlanet(void) const {return position->getHomePlanet();}

    // Return the observer heliocentric position
	Vec3d getObserverHelioPos(void) const;

	// Place openGL in earth equatorial coordinates
	void switchToEarthEquatorial(void) const { glLoadMatrixd(mat_earth_equ_to_eye); }

	// Place openGL in heliocentric ecliptical coordinates
	void switchToHeliocentric(void) const { glLoadMatrixd(mat_helio_to_eye); }

	// Place openGL in local viewer coordinates (Usually somewhere on earth viewing in a specific direction)
	void switchToLocal(void) const { glLoadMatrixd(mat_local_to_eye); }

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

	void updateMove(double deltaAz, double deltaAlt);

	void setViewingMode(VIEWING_MODE_TYPE view_mode);
	VIEWING_MODE_TYPE getViewingMode(void) const {return viewing_mode;}

	const Vec3d& getinitViewPos() {return initViewPos;}
	
	//! Set stellarium time to current real world time
	void setTimeNow();
	//! Get wether the current stellarium time is the real world time
	bool getIsTimeNow(void) const;
	
	//! Return the preset sky time in JD
	double getPresetSkyTime() const {return PresetSkyTime;}
	void setPresetSkyTime(double d) {PresetSkyTime=d;}
	
	//! Return the startupe mode, can be preset|Preset or anything else
	string getStartupTimeMode() {return StartupTimeMode;}
	void setStartupTimeMode(const string& s) {StartupTimeMode = s;}
	
private:
	// Update the modelview matrices
	void updateModelViewMat(void);

	// Struct used to store data for auto mov
	typedef struct
	{
		Vec3d start;
	    Vec3d aim;
	    float speed;
	    float coef;
		bool local_pos;				// Define if the position are in equatorial or altazimutal
	}auto_move;


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
	int flag_traking;				// Define if the selected object is followed
	int flag_lock_equ_pos;			// Define if the equatorial position is locked

	// Automove
	auto_move move;					// Current auto movement
    int flag_auto_move;				// Define if automove is on or off
	int zooming_mode;				// 0 : undefined, 1 zooming, -1 unzooming

	// Time variable
    double time_speed;				// Positive : forward, Negative : Backward, 1 = 1sec/sec
	double JDay;        			// Curent time in Julian day

	double PresetSkyTime;
	string StartupTimeMode;

	// Position variables
	Observer* position;

	Vec3d initViewPos;				// Default viewing direction

	VIEWING_MODE_TYPE viewing_mode;   // defines if view corrects for horizon, or uses equatorial coordinates
};

#endif //_NAVIGATOR_H_

/*
 * Stellarium
 * Copyright (C) 2002 Fabien Chéreau
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

#include "stellarium.h"
#include "vecmath.h"
#include "stel_object.h"

// Conversion in standar Julian time format
#define JD_SECOND 0.000011574074074074074074
#define JD_MINUTE 0.00069444444444444444444
#define JD_HOUR   0.041666666666666666666
#define JD_DAY    1.

class stel_object;

typedef struct          // Struct used to store data on the auto mov
{
	Vec3d start;
    Vec3d aim;
    float speed;
    float coef;
}auto_move;


class observator_pos
{
public:
	observator_pos();
	~observator_pos();
	void save(FILE *);
	void load(FILE *);
	char * name;
	unsigned int planet;// Planet number : 0 floating, 1 Mercure - 9 pluton
    double longitude;	// Longitude in degree
	double latitude;	// Latitude in degree
	int time_zone;		// Time zone
    int altitude;		// Altitude in meter
};

// Class which manages a navigation context
// Manage date/time, viewing direction/fov, observer position, and coordinate changes

class navigator
{
public:
	// Create and initialise to default a navigation context
	navigator();
    virtual ~navigator();

	// Init the viewing matrix, setting the field of view, the clipping planes, and screen size
	void init_project_matrix(int w, int h, double near, double far);

	void update_time(int delta_time);
	void update_transform_matrices(Vec3d earth_ecliptic_pos);
	void update_vision_vector(int delta_time, stel_object* selected);

	// Home made gluLookAt(0., 0., 0.,local_vision[0],local_vision[1],local_vision[2],0.,0.,1.);
	// to keep a better precision to prevent the shaking bug..
	void update_local_to_eye(void);

	void move_to(const Vec3d& _aim);

	// Loads
	void load_position(const char *);		// Load the position info in the file name given
	void save_position(const char *);		// Save the position info in the file name given

	// Time controls
	void set_JDay(double JD) {JDay=JD;}
	double get_JDay(void) const {return JDay;}
	void set_time_speed(double ts) {time_speed=ts;}

	// Flags controls
	void set_flag_traking(int v) {flag_traking=v;}
	void set_flag_lock_equ_pos(int v) {flag_lock_equ_pos=v;}
	int get_flag_lock_equ_pos() const {return flag_lock_equ_pos;}

	// Get vision direction
	const Vec3d& get_equ_vision(void) const {return equ_vision;}
	const Vec3d& get_local_vision(void) const {return local_vision;}
	void set_local_vision(const Vec3d& _pos);

	// Observer position
	void set_time_zone(int t) {position.time_zone=t;}
	int get_time_zone(void) const {return position.time_zone;}
	void set_latitude(double l) {position.latitude=l;}
	double get_latitude(void) const {return position.latitude;}
	void set_longitude(double l) {position.longitude=l;}
	double get_longitude(void) const {return position.longitude;}
	void set_altitude(int a) {position.altitude=a;}
	int get_altitude(void) const {return position.altitude;}

	// Return the observer heliocentric position
	Vec3d get_observer_helio_pos(void) const;


	// Place openGL in earth equatorial coordinates
	void switch_to_earth_equatorial(void) { glLoadMatrixd(mat_earth_equ_to_eye); }

	// Place openGL in heliocentric ecliptical coordinates
	void switch_to_heliocentric(void) { glLoadMatrixd(mat_helio_to_eye); }

	// Place openGL in local viewer coordinates (Usually somewhere on earth viewing in a specific direction)
	void switch_to_local(void) { glLoadMatrixd(mat_local_to_eye); }


	// Transform vector from local coordinate to equatorial
	Vec3d local_to_earth_equ(const Vec3d& v) const { return mat_local_to_earth_equ*v; }

	// Transform vector from equatorial coordinate to local
	Vec3d earth_equ_to_local(const Vec3d& v) const { return mat_earth_equ_to_local*v; }

	// Transform vector from heliocentric coordinate to local
	Vec3d helio_to_local(const Vec3d& v) const { return mat_helio_to_local*v; }

	// Transform vector from heliocentric coordinate to earth equatorial
	Vec3d helio_to_earth_equ(const Vec3d& v) const { return mat_helio_to_earth_equ*v; }

	// Transform vector from heliocentric coordinate to false equatorial : equatorial
	// coordinate but centered on the observer position (usefull for objects close to earth)
	Vec3d helio_to_earth_pos_equ(const Vec3d& v) const { return mat_local_to_earth_equ*mat_helio_to_local*v; }


	// Return the modelview matrix for some coordinate systems
	const Mat4d& get_helio_to_eye_mat(void) const {return mat_helio_to_eye;}
	const Mat4d& get_earth_equ_to_eye_mat(void) const {return mat_earth_equ_to_eye;}
	const Mat4d& get_local_to_eye_mat(void) const {return mat_local_to_eye;}

	void update_move(double deltaAz, double deltaAlt);

private:

	// Matrices used for every coordinate transfo
	Mat4d mat_helio_to_local;		// Transform from Heliocentric to Observator local coordinate
	Mat4d mat_local_to_helio;		// Transform from Observator local coordinate to Heliocentric
	Mat4d mat_local_to_earth_equ;	// Transform from Observator local coordinate to Earth Equatorial
	Mat4d mat_earth_equ_to_local;	// Transform from Observator local coordinate to Earth Equatorial
	Mat4d mat_helio_to_earth_equ;	// Transform from Heliocentric to earth equatorial coordinate

	Mat4d mat_local_to_eye;			// Modelview matrix for observer local drawing
	Mat4d mat_earth_equ_to_eye;		// Modelview matrix for geocentric equatorial drawing
	Mat4d mat_helio_to_eye;			// Modelview matrix for heliocentric equatorial drawing

	// Vision variables
    Vec3d local_vision, equ_vision;		// Viewing direction in local and equatorial coordinates
	int flag_traking;					// Define if the selected object is followed
	int flag_lock_equ_pos;				// Define if the equatorial position is locked

	// Automove
	auto_move move;				// Current auto movement
    int flag_auto_move;			// Define if automove is on or off

	// Time variable
    double time_speed;			// Positive : forward, Negative : Backward, 1 = 1sec/sec
	double JDay;        		// Curent time in Julian day

	// Position variables
	observator_pos position;
};

#endif //_NAVIGATOR_H_

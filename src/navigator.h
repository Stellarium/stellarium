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

#ifndef _NAVIGATION_H_
#define _NAVIGATION_H_

#include "vecmath.h"

typedef struct          // Struct used to store data on the auto mov
{
	Vec3d start;
    Vec3d aim;
    float speed;
    float coef;
}auto_move;


// Class which manages a navigation context
// Manage date/time, viewing direction/fov, observer position, and coordinate changes

class navigator
{
public:
	// Create and initialise to default a navigation context
	navigator();

    ~navigator();

	// Init the viewing matrix, setting the field of view, the clipping planes, and screen size
	void init_project_matrix(int w, int h, double near, double far);

	void update_time(int delta_time);
	void update_transform_matrices(void);
	void update_vision_vector(int delta_time);

	// Place openGL in earth equatorial coordinates
	void switch_to_equatorial(void);
	// Place openGL in heliocentric coordinates
	void switch_to_heliocentric(void);
	// Place openGL in local viewer coordinates (Usually somewhere on earth viewing in a specific direction)
	void switch_to_local(void);

	Vec3d local_to_equ(Vec3d*);		// Transform vector from local coordinate to equatorial
	Vec3d equ_to_local(Vec3d*);		// Transform vector from equatorial coordinate to local
	Vec3d helio_to_local(Vec3d*);	// Transform vector from heliocentric coordinate to local

	// Viewing direction function : 1 move, 0 stop.
	void move_to(Vec3d _aim);
	void turn_right(int);
	void turn_left(int);
	void turn_up(int);
	void turn_down(int);
	void zoom_in(int);
	void zoom_out(int);

	// Sets and gets
	void set_JDay(double JD) {JDay=JD;}
	double get_JDay(void) {return JDay;}
	double get_fov(void) {return fov;}
	Vec3d get_equ_vision(void) {return equ_vision;}

private:

	void update_move(int deltaTime);

	// Matrices used for every coordinate transfo
	Mat4d mat_helio_to_local;		// Transform from Heliocentric to Observator local coordinate
	Mat4d mat_local_to_helio;		// Transform from Observator local coordinate to Heliocentric
	Mat4d mat_local_to_equ;			// Transform from Observator local coordinate to Earth Equatorial
	Mat4d mat_equ_to_local;			// Transform from Observator local coordinate to Earth Equatorial

	// Vision variables
	double fov;							// Field of view
    Vec3d local_vision, equ_vision;		// Viewing direction in local and equatorial coordinates
    double deltaFov,deltaAlt,deltaAz;	// View movement
	double move_speed;					// Speed of movement
	int FlagTraking;					// Define if the selected object is followed
	int FlagLockEquPos;					// Define if the equatorial position is locked

	// Automove
	auto_move move;				// Current auto movement
    int FlagAutoMove;			// Define if automove is on or off

	// Time variable
    double time_speed;			// Positive : forward, Negative : Backward, 1 = 1sec/sec
	double JDay;        		// Curent time in Julian day
};


extern navigator navigation; // Navigator instance used in stellarium


#endif //_NAVIGATION_H_

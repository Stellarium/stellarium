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

#ifndef _PLANET_H_
#define _PLANET_H_

#include "stellarium.h"
#include "stel_utility.h"
#include "s_font.h"
#include <list>
#include "vecmath.h"
#include "stel_object.h"
#include "callback.h"

// The callback type for the external position computation function
typedef CBFunctor4<double, double*, double*, double*> pos_func_type;
typedef CBFunctor4<double, double*, double*, double*>* p_pos_func_type;

// epoch J2000: 12 UT on 1 Jan 2000
#define J2000 2451545.0

using namespace std;

// Class used to store orbital elements
class rotation_elements
{
 public:
    rotation_elements();
    float period;        // sidereal rotation period
    float offset;        // rotation at epoch
    double epoch;
    float obliquity;     // tilt of rotation axis w.r.t. ecliptic
    float ascendingNode; // long. of ascending node of equator on the ecliptic
    float precessionRate; // rate of precession of rotation axis in rads/day
};

// Class ued to manage rings for planets like saturn
class ring
{
public:
	ring(float _radius, s_texture * _tex);
	virtual void draw(navigator* nav);
private:
	float radius;
	s_texture * tex;
};


class planet : public stel_object
{
public:
	planet(const char * _name, int _flagHalo, double _radius, vec3_t _color,
	const char* tex_map_name, const char* tex_halo_name, pos_func_type _coord_func);

    virtual ~planet();

	// Return the information string "ready to print" :)
	virtual void get_info_string(char * s, navigator * nav) const;

	// Compute the position in the parent planet coordinate system
	virtual void compute_position(double date);

	// Compute the transformation matrix from the local planet coordinate to the parent planet coordinate
    virtual void compute_trans_matrix(double date);

	// Draw the planet, if hint_ON is != 0 draw a circle and the name as well
    virtual void draw(int hint_ON, draw_utility * du, navigator * nav);

	// Add the given planet in the satellite list
	virtual void add_satellite(planet*);

	// Set the orbital elements
	virtual void set_rotation_elements(float _period, float _offset, double _epoch,
		float _obliquity, float _ascendingNode, float _precessionRate);

	// Get the planet position in the parent planet ecliptic coordinate
	virtual Vec3d get_ecliptic_pos() const;

	// Return the heliocentric ecliptical position
	virtual Vec3d get_heliocentric_ecliptic_pos() const;

	// Get a matrix which converts from heliocentric ecliptic coordinate to local geographic coordinate
	virtual Mat4d get_helio_to_geo_matrix();

	unsigned char get_type(void) const {return STEL_OBJECT_PLANET;}

	// Return the planet position in rectangular earth equatorial coordinate
	virtual Vec3d get_earth_equ_pos(navigator * nav) const;

	const char* get_name(void) const {return name;}

	static void set_font(s_font* f) {planet_name_font = f;}

protected:
	// Compute the z rotation to use from equatorial to geographic coordinates
	virtual void compute_geographic_rotation(double date);

	// Draw the 3D sphere
	void draw_sphere(void);

	// Draw the small star like 2D halo
	void draw_halo(draw_utility * du);

	// Draw the circle and name of the planet
	void draw_hints(Vec3d earthEquPos, draw_utility * du);

    char * name;
	int flagHalo;					// Set wether a little "star like" halo will be drawn
	rotation_elements re;			// Rotation param
	double radius;					// Planet radius in UA
	Vec3d ecliptic_pos; 			// Position in UA in the rectangular ecliptic coordinate system
									// centered on the parent planet
	Vec3d screenPos;				// Used to store temporarily the 2D position on screen
	vec3_t color;
	Mat4d mat_local_to_parent;		// Transfo matrix from local ecliptique to parent ecliptic
	float axis_rotation;			// Rotation angle of the planet on it's axis
    s_texture * tex_map;			// Planet map texture
	s_texture * tex_halo;			// Little halo texture
	// The callback for the calculation of the equatorial rect heliocentric position at time JD.
	pos_func_type coord_func;
	planet * parent;				// Planet parent i.e. sun for earth
	list<planet *> satellites;		// satellites of the planet

	static s_font* planet_name_font;// Font for names
};

#endif // _PLANET_H_

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

#include <list>
#include <string>
#include "stellarium.h"
#include "stel_utility.h"
#include "s_font.h"
#include "tone_reproductor.h"
#include "vecmath.h"
#include "stel_object.h"
#include "callbacks.hpp"

// The callback type for the external position computation function
typedef boost::callback<void, double, double*> pos_func_type;

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

// Class to manage rings for planets like saturn
class ring
{
public:
	ring(float _radius, const string& _texname);
	virtual ~ring();
	void draw(const Projector* prj, const Mat4d& mat);
	double get_size(void) const {return radius;}
private:
	float radius;
	s_texture * tex;
};


class planet : public stel_object
{
public:
	planet(const string& _name, int _flagHalo, int _flag_lighting, double _radius, Vec3f _color,
	float _albedo, const string& tex_map_name, const string& tex_halo_name, pos_func_type _coord_func);

    virtual ~planet();

	// Return the information string "ready to print" :)
	void get_info_string(char * s, const navigator * nav) const;
	void get_short_info_string(char * s, const navigator * nav) const;
	virtual double get_close_fov(const navigator * nav) const;
	virtual double get_satellites_fov(const navigator * nav) const;
	virtual float get_mag(const navigator * nav) const {return compute_magnitude(nav);}

	//	void setLabelColor(const Vec3f& v) {label_color = v;}

	// Compute the position in the parent planet coordinate system
	void compute_position(double date);

	// Compute the transformation matrix from the local planet coordinate to the parent planet coordinate
    void compute_trans_matrix(double date);

	// Get the phase angle for an observer at pos obs_pos in the heliocentric coordinate (in AU)
	double get_phase(Vec3d obs_pos) const;

	// Get the magnitude for an observer at pos obs_pos in the heliocentric coordinate (in AU)
	float compute_magnitude(const Vec3d obs_pos) const;
	float compute_magnitude(const navigator * nav) const;

	// Draw the planet, if hint_ON is != 0 draw a circle and the name as well
    void draw(int hint_ON, Projector* prj, const navigator* nav, const tone_reproductor* eye, int flag_point);

	// Add the given planet in the satellite list
	void add_satellite(planet*);

	// Set the orbital elements
	void set_rotation_elements(float _period, float _offset, double _epoch,
		float _obliquity, float _ascendingNode, float _precessionRate);

	// Get the planet position in the parent planet ecliptic coordinate
	Vec3d get_ecliptic_pos() const;

	// Return the heliocentric ecliptical position
	Vec3d get_heliocentric_ecliptic_pos() const;

	// Compute the distance to the given position in heliocentric coordinate (in AU)
	double compute_distance(const Vec3d& obs_helio_pos);
	double get_distance(void) const {return distance;}

	// Get a matrix which converts from heliocentric ecliptic coordinate to local geographic coordinate
	Mat4d get_helio_to_geo_matrix();

	STEL_OBJECT_TYPE get_type(void) const {return STEL_OBJECT_PLANET;}

	// Return the planet position in rectangular earth equatorial coordinate
	Vec3d get_earth_equ_pos(const navigator * nav) const;

	string get_name(void) const {return name;}

	void set_rings(ring* r) {rings = r;}

	void set_sphere_scale(float s) {sphere_scale = s;}
	float get_sphere_scale(void) const {return sphere_scale;}

	planet* get_parent(void) {return parent;}

	void set_big_halo(const string& halotexfile);
	void set_halo_size(float s) {big_halo_size = s;}

	static void set_font(s_font* f) {planet_name_font = f;}
	static void set_star_scale(float s) {star_scale = s;}
	static void set_gravity_label_flag(bool gl) {gravity_label = gl;}
	static void set_label_color(const Vec3f& lc) {label_color = lc;}

protected:
	// Return the radius of a circle containing the object on screen
	float get_on_screen_size(const Projector* prj, const navigator * nav);

	// Compute the z rotation to use from equatorial to geographic coordinates
	void compute_geographic_rotation(double date);

	// Draw the 3D sphere
	void draw_sphere(const Projector* prj, const Mat4d& mat, float screen_sz);

	// Draw the small star-like 2D halo
	void draw_halo(const navigator* nav, const Projector* prj, const tone_reproductor* eye);

	// Draw the small star-like point
	void draw_point_halo(const navigator* nav, const Projector* prj, const tone_reproductor* eye);

	// Draw the circle and name of the planet
	void draw_hints(const navigator* nav, const Projector* prj);

	// Draw the big halo (for sun or moon)
	void draw_big_halo(const navigator* nav, const Projector* prj, const tone_reproductor* eye);

    string name;
	int flagHalo;					// Set wether a little "star like" halo will be drawn
	int flag_lighting;				// Set wether light computation has to be proceed
	rotation_elements re;			// Rotation param
	double radius;					// Planet radius in UA
	Vec3d ecliptic_pos; 			// Position in UA in the rectangular ecliptic coordinate system
									// centered on the parent planet
	Vec3d screenPos;				// Used to store temporarily the 2D position on screen
	Vec3f color;
	float albedo;					// Planet albedo
	Mat4d mat_local_to_parent;		// Transfo matrix from local ecliptique to parent ecliptic
	float axis_rotation;			// Rotation angle of the planet on it's axis
    s_texture * tex_map;			// Planet map texture
	s_texture * tex_halo;			// Little halo texture
	s_texture * tex_big_halo;		// Big halo texture

	float big_halo_size;				// Halo size on screen

	ring* rings;					// Planet rings

	double distance;				// Temporary variable used to store the distance to a given point
									// it is used for sorting while drawing

	float sphere_scale;				// Artificial scaling for better viewing

	double lastJD;
	double deltaJD;

	// The callback for the calculation of the equatorial rect heliocentric position at time JD.
	pos_func_type coord_func;
	planet * parent;				// Planet parent i.e. sun for earth
	list<planet *> satellites;		// satellites of the planet

	static s_font* planet_name_font;// Font for names
	static float star_scale;
	static bool gravity_label;
	static Vec3f label_color;
};

#endif // _PLANET_H_

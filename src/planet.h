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
#include "fader.h"
#include "translator.h"

// The callback type for the external position computation function
typedef boost::callback<void, double, double*> pos_func_type;

// epoch J2000: 12 UT on 1 Jan 2000
#define J2000 2451545.0
#define ORBIT_SEGMENTS 72

using namespace std;

struct TrailPoint {
  Vec3d point;
  double date;
};



// Class used to store orbital elements
class RotationElements
{
 public:
    RotationElements();
    float period;        // rotation period
    float offset;        // rotation at epoch
    double epoch;
    float obliquity;     // tilt of rotation axis w.r.t. ecliptic
    float ascendingNode; // long. of ascending node of equator on the ecliptic
    float precessionRate; // rate of precession of rotation axis in rads/day
    double sidereal_period; // sidereal period (Planet year in earth days)
};

// Class to manage rings for planets like saturn
class Ring
{
public:
	Ring(float _radius, const string& _texname);
	virtual ~Ring();
	void draw(const Projector* prj, const Mat4d& mat);
	double get_size(void) const {return radius;}
private:
	float radius;
	s_texture * tex;
};


class Planet : public StelObject
{
public:
	Planet(Planet *parent,
           const string& englishName,
           int _flagHalo,
           int _flag_lighting,
           double _radius,
           Vec3f _color,
           float _albedo,
           const string& tex_map_name,
           const string& tex_halo_name,
           pos_func_type _coord_func);

    virtual ~Planet();

    double getRadius(void) const {return radius;}

	// Return the information string "ready to print" :)
	wstring get_info_string(const Navigator * nav) const;
	wstring get_short_info_string(const Navigator * nav) const;
	virtual double get_close_fov(const Navigator * nav) const;
	virtual double get_satellites_fov(const Navigator * nav) const;
	virtual float get_mag(const Navigator * nav) const {return compute_magnitude(nav);}

	/** Translate planet name using the passed translator */
	void translateName(Translator& trans) {nameI18 = trans.translate(englishName);}
	
    // Compute the z rotation to use from equatorial to geographic coordinates
    double getSiderealTime(double jd) const;
    Mat4d getRotEquatorialToVsop87(void) const;

	// Compute the position in the parent Planet coordinate system
	void compute_position(double date);

	// Compute the transformation matrix from the local Planet coordinate to the parent Planet coordinate
	void compute_trans_matrix(double date);

	// Get the phase angle for an observer at pos obs_pos in the heliocentric coordinate (in AU)
	double get_phase(Vec3d obs_pos) const;

	// Get the magnitude for an observer at pos obs_pos in the heliocentric coordinate (in AU)
	float compute_magnitude(const Vec3d obs_pos) const;
	float compute_magnitude(const Navigator * nav) const;

	// Draw the Planet, if hint_ON is != 0 draw a circle and the name as well
	void draw(Projector* prj, const Navigator* nav, const ToneReproductor* eye, 
		  int flag_point, bool stencil);

	// Set the orbital elements
	void set_rotation_elements(float _period, float _offset, double _epoch,
	float _obliquity, float _ascendingNode, float _precessionRate, double _sidereal_period);
    double getRotAscendingnode(void) const {return re.ascendingNode;}
    double getRotObliquity(void) const {return re.obliquity;}


	// Get the Planet position in the parent Planet ecliptic coordinate
	Vec3d get_ecliptic_pos() const;

	// Return the heliocentric ecliptical position
	Vec3d get_heliocentric_ecliptic_pos() const;

	// Compute the distance to the given position in heliocentric coordinate (in AU)
	double compute_distance(const Vec3d& obs_helio_pos);
	double get_distance(void) const {return distance;}

	// Get a matrix which converts from heliocentric ecliptic coordinate to local geographic coordinate
//	Mat4d get_helio_to_geo_matrix();

	STEL_OBJECT_TYPE get_type(void) const {return STEL_OBJECT_PLANET;}

	// Return the Planet position in rectangular earth equatorial coordinate
	Vec3d get_earth_equ_pos(const Navigator * nav) const;

	string getEnglishName(void) const {return englishName;}
	wstring getNameI18(void) const {return nameI18;}

	void set_rings(Ring* r) {rings = r;}

	void set_sphere_scale(float s) {sphere_scale = s;}
	float get_sphere_scale(void) const {return sphere_scale;}

	const Planet *get_parent(void) const {return parent;}

	void set_big_halo(const string& halotexfile);
	void set_halo_size(float s) {big_halo_size = s;}

	static void set_font(s_font* f) {planet_name_font = f;}
	
	static void setScale(float s) {object_scale = s;}
	static float getScale(void) {return object_scale;}
	
	static void set_gravity_label_flag(bool gl) {gravity_label = gl;}
	static void set_label_color(const Vec3f& lc) {label_color = lc;}
	static void set_orbit_color(const Vec3f& oc) {orbit_color = oc;}

	// draw orbital path of Planet
	void draw_orbit(const Navigator * nav, const Projector* prj);

	void update_trail(const Navigator* nav);
	void draw_trail(const Navigator * nav, const Projector* prj);
	static void set_trail_color(const Vec3f& c) { trail_color = c; }
	
	//! Start/stop accumulating new trail data (clear old data)
	void startTrail(bool b);

	void update(int delta_time);
	
	void setFlagHints(bool b){hint_fader = b;}
	bool getFlagHints(void) const {return hint_fader;}
	
	void setFlagOrbits(bool b){orbit_fader = b;}
	bool getFlagOrbits(void) const {return orbit_fader;}	
	
	void setFlagTrail(bool b){trail_fader = b;}
	bool getFlagTrail(void) const {return trail_fader;}
	
	static void setflagShow(bool b) {Planet::flagShow = b;}
	static bool getflagShow(void) {return Planet::flagShow;}
	
protected:
	// Return the radius of a circle containing the object on screen
	float get_on_screen_size(const Projector* prj, const Navigator * nav);

	// Draw the 3D sphere
	void draw_sphere(const Projector* prj, const Mat4d& mat, float screen_sz);

	// Draw the small star-like 2D halo
	void draw_halo(const Navigator* nav, const Projector* prj, const ToneReproductor* eye);

	// Draw the small star-like point
	void draw_point_halo(const Navigator* nav, const Projector* prj, const ToneReproductor* eye);

	// Draw the circle and name of the Planet
	void draw_hints(const Navigator* nav, const Projector* prj);

	// Draw the big halo (for sun or moon)
	void draw_big_halo(const Navigator* nav, const Projector* prj, const ToneReproductor* eye);


	string englishName; // english planet name
	wstring nameI18;				// International translated name
	int flagHalo;					// Set wether a little "star like" halo will be drawn
	int flag_lighting;				// Set wether light computation has to be proceed
	RotationElements re;			// Rotation param
	double radius;					// Planet radius in UA
	Vec3d orbit[ORBIT_SEGMENTS];                        // store heliocentric coordinates for drawing the orbit
	Vec3d ecliptic_pos; 			// Position in UA in the rectangular ecliptic coordinate system
									// centered on the parent Planet
	Vec3d screenPos;				// Used to store temporarily the 2D position on screen
	Vec3f color;
	float albedo;					// Planet albedo
	Mat4d rot_local_to_parent;
	Mat4d mat_local_to_parent;		// Transfo matrix from local ecliptique to parent ecliptic
	float axis_rotation;			// Rotation angle of the Planet on it's axis
    s_texture * tex_map;			// Planet map texture
	s_texture * tex_halo;			// Little halo texture
	s_texture * tex_big_halo;		// Big halo texture

	float big_halo_size;				// Halo size on screen

	Ring* rings;					// Planet rings

	double distance;				// Temporary variable used to store the distance to a given point
									// it is used for sorting while drawing

	float sphere_scale;				// Artificial scaling for better viewing

	double lastJD;
	double last_orbitJD;
	double deltaJD;
	double delta_orbitJD;
	bool orbit_cached;       // whether orbit calculations are cached for drawing orbit yet

	// The callback for the calculation of the equatorial rect heliocentric position at time JD.
	pos_func_type coord_func;
	const Planet *const parent;				// Planet parent i.e. sun for earth
	list<Planet *> satellites;		// satellites of the Planet

	static s_font* planet_name_font;// Font for names
	static float object_scale;
	static bool gravity_label;
	static Vec3f label_color;
	static Vec3f orbit_color;
	static Vec3f trail_color;

	list<TrailPoint>trail;
	bool trail_on;  // accumulate trail data if true
	double DeltaTrail;
	int MaxTrail;
	double last_trailJD;
	bool first_point;  // if need to take first point of trail still

	static LinearFader flagShow;
	
	LinearFader hint_fader;
	LinearFader orbit_fader;
	LinearFader trail_fader;

};

#endif // _PLANET_H_

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

#ifndef _PLANET_HPP_
#define _PLANET_HPP_

#include <list>
#include <QString>

#include "StelObject.hpp"
#include "ToneReproducer.hpp"
#include "vecmath.h"
#include "callbacks.hpp"
#include "Fader.hpp"
#include "Translator.hpp"
#include "STextureTypes.hpp"

// The callback type for the external position computation function
typedef boost::callback<void, double, double*> pos_func_type;

typedef void (OsulatingFunctType)(double jd0,double jd,double xyz[3]);

// epoch J2000: 12 UT on 1 Jan 2000
#define J2000 2451545.0
#define ORBIT_SEGMENTS 72

class SFont;

struct TrailPoint
{
  Vec3d point;
  double date;
};


// Class used to store orbital elements
class RotationElements
{
public:
    RotationElements(void) : period(1.), offset(0.), epoch(J2000), obliquity(0.), ascendingNode(0.), precessionRate(0.) {}
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
	Ring(double radius_min,double radius_max,const QString &texname);
	~Ring(void);
	void draw(const Projector* prj,const Mat4d& mat,double screen_sz);
	double get_size(void) const {return radius_max;}
private:
	const double radius_min;
	const double radius_max;
	STextureSP tex;
};


class Planet : public StelObject
{
public:
	Planet(Planet *parent,
	       const QString& englishName,
	       int flagHalo,
	       int flag_lighting,
	       double radius,
	       double oblateness,
	       Vec3f color,
	       float albedo,
	       const QString& tex_map_name,
	       const QString& tex_halo_name,
	       pos_func_type _coord_func,
	       OsulatingFunctType *osculating_func,
	       bool close_orbit,bool hidden);

	~Planet();

	///////////////////////////////////////////////////////////////////////////
	// Methods inherited from StelObject
	virtual QString getInfoString(const Navigator * nav) const;
	virtual QString getShortInfoString(const Navigator * nav) const;
	virtual double getCloseViewFov(const Navigator * nav) const;
	virtual double get_satellites_fov(const Navigator * nav) const;
	virtual double get_parent_satellites_fov(const Navigator * nav) const;
	virtual float getMagnitude(const Navigator * nav) const;
	virtual float getSelectPriority(const Navigator *nav) const;
	virtual Vec3f getInfoColor(void) const;
	// Return the radius of a circle containing the object on screen
	virtual float getOnScreenSize(const Projector *prj, const Navigator *nav = NULL) const;
	virtual QString getType(void) const {return "Planet";}
	// observer centered J2000 coordinates
	virtual Vec3d getObsJ2000Pos(const Navigator *nav) const;
	virtual QString getEnglishName(void) const {return englishName;}
	virtual QString getNameI18n(void) const {return nameI18;}
	
	///////////////////////////////////////////////////////////////////////////
	// Methods of SolarSystem object
	//! Translate planet name using the passed translator
	void translateName(Translator& trans) {nameI18 = trans.qtranslate(englishName);}
	
	// Draw the Planet
	// Return the squared distance in pixels between the current and the
	// previous position this planet was drawn at.
	double draw(Projector* prj, const Navigator* nav, const ToneReproducer* eye, bool stencil);
	
	///////////////////////////////////////////////////////////////////////////
	// Methods specific to Planet
	//! Get the radius of the planet in AU.
	//! @return the radius of the planet in astronomical units.
	double getRadius(void) const {return radius;}
	double getSiderealDay(void) const {return re.period;}
	
	// Compute the z rotation to use from equatorial to geographic coordinates
	double getSiderealTime(double jd) const;
	Mat4d getRotEquatorialToVsop87(void) const;
	void setRotEquatorialToVsop87(const Mat4d &m);

	const RotationElements &getRotationElements(void) const {return re;}

	// Compute the position in the parent Planet coordinate system
	void computePositionWithoutOrbits(const double date);
	void compute_position(const double date);

	// Compute the transformation matrix from the local Planet coordinate to the parent Planet coordinate
	void compute_trans_matrix(double date);

	// Get the phase angle for an observer at pos obs_pos in the heliocentric coordinate (in AU)
	double get_phase(Vec3d obs_pos) const;

	// Get the magnitude for an observer at pos obs_pos in the heliocentric coordinate (in AU)
	float compute_magnitude(const Vec3d obs_pos) const;

	// Set the orbital elements
	void set_rotation_elements(float _period, float _offset, double _epoch,
		float _obliquity, float _ascendingNode, float _precessionRate, double _sidereal_period);
	double getRotAscendingnode(void) const {return re.ascendingNode;}
	double getRotObliquity(void) const {return re.obliquity;}

	// Get the Planet position in the parent Planet ecliptic coordinate
	Vec3d get_ecliptic_pos() const;

	// Return the heliocentric ecliptical position
	Vec3d get_heliocentric_ecliptic_pos() const;
	void set_heliocentric_ecliptic_pos(const Vec3d &pos);

	// Compute the distance to the given position in heliocentric coordinate (in AU)
	double compute_distance(const Vec3d& obs_helio_pos);
	double get_distance(void) const {return distance;}

	void set_rings(Ring* r) {rings = r;}

	void set_sphere_scale(float s) {sphere_scale = s;}
	float get_sphere_scale(void) const {return sphere_scale;}

	const Planet *get_parent(void) const {return parent;}

	void set_big_halo(const QString& halotexfile);
	void set_halo_size(float s) {big_halo_size = s;}

	static void set_font(SFont* f) {planet_name_font = f;}
	
	static void set_label_color(const Vec3f& lc) {label_color = lc;}
	static const Vec3f& getLabelColor(void) {return label_color;}

	void update(int delta_time);
	
	void setFlagHints(bool b){hint_fader = b;}
	bool getFlagHints(void) const {return hint_fader;}
	
	void setFlagLabels(bool b){labelsFader = b;}
	bool getFlagLabels(void) const {return labelsFader;}
	
	///////////////////////////////////////////////////////////////////////////
	// DEPRECATED
	
	///// Trail related code
	// Should move to a TrailPath class which works on a StelObject, not on a Planet
	void update_trail(const Navigator* nav);
	void draw_trail(const Navigator * nav, const Projector* prj);
	//! Start/stop accumulating new trail data (clear old data)
	void startTrail(bool b);
	void setFlagTrail(bool b){if(b == trail_fader) return; trail_fader = b; startTrail(b);}
	bool getFlagTrail(void) const {return trail_fader;}
	static void set_trail_color(const Vec3f& c) { trail_color = c; }
	static const Vec3f& getTrailColor() { return trail_color; }
	static Vec3f trail_color;
	list<TrailPoint>trail;
	bool trail_on;                  // accumulate trail data if true
	double DeltaTrail;
	int MaxTrail;
	double last_trailJD;
	bool first_point;               // if need to take first point of trail still
	LinearFader trail_fader;
	
	///// Orbit related code
	// Should move to an OrbitPath class which works on a SolarSystemObject, not a Planet
	void setFlagOrbits(bool b){orbit_fader = b;}
	bool getFlagOrbits(void) const {return orbit_fader;}	
	static void set_orbit_color(const Vec3f& oc) {orbit_color = oc;}
	static const Vec3f& getOrbitColor() {return orbit_color;}
	// draw orbital path of Planet
	void draw_orbit(const Navigator * nav, const Projector* prj);
	Vec3d orbit[ORBIT_SEGMENTS];    // store heliocentric coordinates for drawing the orbit
	double last_orbitJD;
	double deltaJD;
	double delta_orbitJD;
	bool orbit_cached;              // whether orbit calculations are cached for drawing orbit yet
	bool close_orbit; // whether to connect the beginning of the orbit line to
	                  // the end: good for elliptical orbits, bad for parabolic
	                  // and hyperbolic orbits
	static Vec3f orbit_color;
	LinearFader orbit_fader;
	
protected:
	// Return the information string "ready to print" :)
	QString getSkyLabel(const Navigator * nav) const;
	
	// Draw the 3D sphere
	void draw_sphere(const Projector* prj, const Mat4d& mat, float screen_sz);

	// Draw the circle and name of the Planet
	void draw_hints(const Navigator* nav, const Projector* prj);

	// Draw the big halo (for sun or moon)
	void draw_big_halo(const Navigator* nav, const Projector* prj, const ToneReproducer* eye);


	QString englishName;            // english planet name
	QString nameI18;                // International translated name
	int flagHalo;                   // Set wether a little "star like" halo will be drawn
	int flag_lighting;              // Set wether light computation has to be proceed
	RotationElements re;            // Rotation param
	double radius;                  // Planet radius in UA
	double one_minus_oblateness;    // (polar radius)/(equatorial radius)

	Vec3d ecliptic_pos;             // Position in UA in the rectangular ecliptic coordinate system
	                                // centered on the parent Planet
	Vec3d screenPos;                // Used to store temporarily the 2D position on screen
	Vec3d previousScreenPos;        // The position of this planet in the previous frame.
	Vec3f color;
	float albedo;                   // Planet albedo
	Mat4d rot_local_to_parent;
	float axis_rotation;            // Rotation angle of the Planet on it's axis
	STextureSP tex_map;             // Planet map texture
	STextureSP tex_big_halo;        // Big halo texture

	float big_halo_size;            // Halo size on screen

	Ring* rings;                    // Planet rings

	double distance;                // Temporary variable used to store the distance to a given point
	                                // it is used for sorting while drawing

	float sphere_scale;             // Artificial scaling for better viewing

	double lastJD;
	
	// The callback for the calculation of the equatorial rect heliocentric position at time JD.
	pos_func_type coord_func;
	OsulatingFunctType *const osculating_func;

	const Planet *parent;           // Planet parent i.e. sun for earth
	list<Planet *> satellites;      // satellites of the Planet

	static SFont* planet_name_font; // Font for names
	static Vec3f label_color;
	
	LinearFader hint_fader;
	LinearFader labelsFader;
	
	bool hidden;  // useful for fake planets used as observation positions - not drawn or labeled
};

#endif // _PLANET_HPP_

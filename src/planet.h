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
#include "draw.h"
#include <list>
#include "vecmath.h"
#include "stel_object.h"

// epoch J2000: 12 UT on 1 Jan 2000
#define J2000 2451545.0


#define NO_HALO 0
#define WITH_HALO 1

extern s_font * planet_name_font;

using namespace std;

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

class planet : public stel_object
{
public:
	planet(char * _name, int _flagHalo, double _radius, vec3_t _color, s_texture * _planetTexture, s_texture * _haloTexture, void (*_coord_func)(double JD, double *, double *, double *));
    virtual ~planet();
	void planet::get_info_string(char * s);				// Return info string about the planet
	virtual void compute_position(double date); 		// Compute the position from the mother planet
    virtual void compute_trans_matrix(double date);		// Compute the transformation matrix from the mother planet
	virtual void compute_geographic_rotation(double date);	// Compute the z rotation to use from equatorial to geographic coordinates
    virtual void draw(int hint_ON, draw_utility * du);
	virtual void addSatellite(planet*);
	virtual void set_rotation_elements(float _period, float _offset, double _epoch, float _obliquity, float _ascendingNode, float _precessionRate);
	virtual Vec3d get_ecliptic_pos();
	virtual Vec3d get_heliocentric_ecliptic_pos();		// Return the heliocentric ecliptical position
	// Get a matrix which convert from heliocentric ecliptic coordinate to local geographic coordinate
	virtual Mat4d get_helio_to_geo_matrix();
	unsigned char get_type(void) {return STEL_OBJECT_PLANET;}
	virtual Vec3d get_earth_equ_pos(void);				// Return the rect earth equatorial position
	virtual planet * search(Vec3d);	// Search if any planet is close to position given in earth equatorial position.
	// Search if any planet is close to position given in earth equatorial position and return the distance
	virtual planet* planet::search(Vec3d pos, double * angleClosest);
protected:
    char * name;
	int flagHalo;							// Set if a little "star like" halo will be drawn
	rotation_elements re;					// Rotation param
	double radius;							// Planet radius in UA
	Vec3d ecliptic_pos; 					// Position in UA in the rectangular ecliptic coordinate system
											// The reference of the coord is the parent planet
	vec3_t color;
	Mat4d mat_local_to_parent;		        // Transfo matrix from local ecliptique to parent ecliptic
	Mat4d mat_parent_to_local;		        // Transfo matrix from parent ecliptique to local ecliptic
	float axis_rotation;					// Rotation angle of the planet on it's axis
    s_texture * planetTexture;				// Planet map texture
	s_texture * haloTexture;				// Little halo texture
	void (*coord_func)(double JD, double *, double *, double *); // The function called for the calculation of the equatorial rect heliocentric position at time JD.
	planet * parent;
	list<planet *> satellites;				// satellites of the planet
};


class ring
{
public:
	ring(float _radius, s_texture * _tex);
	virtual void draw();
private:
	float radius;
	s_texture * tex;
};

class ring_planet : public planet
{
public:
	ring_planet(char * _name, int _flagHalo, double _radius, vec3_t _color, s_texture * _planetTexture, s_texture * _haloTexture, void (*_coord_func)(double JD, double *, double *, double *), ring * _planet_ring);
	virtual void draw(int hint_ON, draw_utility * du);
protected:
	ring * planet_ring;						// Ring for ie saturn/uranus
};



class sun_planet : public planet
{
public:
	sun_planet(char * _name, int _flagHalo, double _radius, vec3_t _color, s_texture * _planetTexture, s_texture * _haloTexture, s_texture * _bigHaloTexture);
	virtual void compute_position(double date);
	virtual void compute_trans_matrix(double date);
	virtual void draw(int hint_ON, draw_utility * du);
protected:
	s_texture * bigHaloTexture;				// Big halo texture
};


#endif // _PLANET_H_

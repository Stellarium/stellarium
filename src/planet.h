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

#include "libnova.h"
#include "stellarium.h"
#include "stel_utility.h"
#include "s_font.h"
#include "draw.h"
#include <list>
#include "vecmath.h"
#include "stel_object.h"

#define NO_HALO 0
#define WITH_HALO 1


class RotationElements
{
 public:
    RotationElements();

    float period;        // sidereal rotation period
    float offset;        // rotation at epoch
    double epoch;
    float obliquity;     // tilt of rotation axis w.r.t. ecliptic
    float ascendingNode; // long. of ascending node of equator on the ecliptic
    float precessionRate; // rate of precession of rotation axis in rads/day
};

//class planet;

class planet : public stel_object
{
public:
	planet(char * _name, int _flagHalo, double _radius, vec3_t _color, s_texture * _planetTexture, s_texture * _haloTexture, void (*_coord_func)(double JD, struct ln_helio_posn * position));
    virtual ~planet();
	virtual void computePosition(double date); // Compute the position from the mother planet
    virtual Mat4d computeMatrix(double date);  // Compute the transformation matrix from the mother planet
    virtual void draw();
	virtual void addSatellite(planet*);
	virtual Vec3d getHelioPos();
	unsigned char get_type(void) {return STEL_OBJECT_PLANET;}
protected:
    char * name;
	int flagHalo;							// Set if a little "star like" halo will be drawn
	RotationElements re;					// Rotation param
	double radius;							// Planet radius
	Vec3d ecliptic_coord; 					// Coords in UA in the rectangular ecliptic coordinate system
											// The reference of the coord is the parent planet
	vec3_t color;
    s_texture * planetTexture;				// Planet map texture
	s_texture * haloTexture;				// Little halo texture
	void (*coord_func)(double JD, struct ln_helio_posn * position); // The function called for the calculation of the rect heliocentric position at time JD.
	planet * parent;
	list<planet *> satellites;				// satellites of the planet
};

class ring
{
public:
	s_texture tex;
	float size;
};

class ring_planet : public planet
{
public:
	ring_planet(char * _name, int _flagHalo, double _radius, vec3_t _color, s_texture * _planetTexture, s_texture * _haloTexture, void (*_coord_func)(double JD, struct ln_rect_posn * position), ring * _planetRing);
	virtual void setRing(ring*);
protected:
	ring * planetRing;						// Ring for ie saturn/uranus
};

class sun_planet : public planet
{
public:
	sun_planet(char * _name, int _flagHalo, double _radius, vec3_t _color, s_texture * _planetTexture, s_texture * _haloTexture, s_texture * _bigHaloTexture, void (*_coord_func)(double JD, struct ln_rect_posn * position));
    virtual void computeCoords(double date);
    virtual void draw();
protected:
	s_texture * bigHaloTexture;				// Big halo texture
};


#endif // _PLANET_H_

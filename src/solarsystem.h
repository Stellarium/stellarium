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

#ifndef _SOLARSYSTEM_H_
#define _SOLARSYSTEM_H_

#include <vector>
#include <functional>

#include "stellarium.h"
#include "planet.h"
#include "orbit.h"

class SolarSystem
{
public:
	SolarSystem();

    virtual ~SolarSystem();

	// Init and load the solar system data from the file "planetfile".
	void init(const char * font_fileName, const char* planetfile);

	// Compute the position for every elements of the solar system.
	void compute_positions(double date);

	// Compute the transformation matrix for every elements of the solar system.
    void compute_trans_matrices(double date);

	// Draw all the elements of the solar system
    void draw(int hint_ON, Projector * du, navigator * nav);

	// Search if any planet is close to position given in earth equatorial position.
	planet* search(Vec3d, navigator * nav);

	planet* get_earth(void) {return earth;}

	planet* get_moon(void) {return moon;}
	
private:
	planet* sun;
	planet* moon;
	planet* earth;

	s_font* planet_name_font;
	void load(const char* planetfile);	// Load the bodies data from a file
	vector<planet*> system_planets;		// Vector containing all the bodies of the system
	vector<EllipticalOrbit*> ell_orbits;			// Pointers on created elliptical orbits

	// And sort them from the furthest to the closest to the observer
	struct bigger_distance : public binary_function<planet*, planet*, bool>
	{
		bool operator()(planet* p1, planet* p2) { return p1->get_distance() > p2->get_distance(); }
	};
};


#endif // _SOLARSYSTEM_H_

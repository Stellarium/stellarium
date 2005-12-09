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

	void update(int delta_time, Navigator* nav);
	
	// Draw all the elements of the solar system
    void draw(Planet *selected, bool hint_ON, Projector * du, const Navigator * nav, 
			  const ToneReproductor* eye, bool _gravity_label, 
			  bool flag_point, bool flag_orbits, bool flag_trails);
	
	// Load the bodies data from a file
	void load(const string& planetfile);
	void load_names(const string& names_file);

	void set_font(float font_size, const string& font_name);
	void set_label_color(const Vec3f& c) {Planet::set_label_color(c);}
	void set_orbit_color(const Vec3f& c) {Planet::set_orbit_color(c);}
  
	// Compute the position for every elements of the solar system.
	void compute_positions(double date);

	// Compute the transformation matrix for every elements of the solar system.
    void compute_trans_matrices(double date);

	// Search if any Planet is close to position given in earth equatorial position.
	Planet* search(Vec3d, const Navigator * nav, const Projector * prj);

	// Return a stl vector containing the planets located inside the lim_fov circle around position v
	vector<StelObject*> search_around(Vec3d v, double lim_fov, const Navigator * nav, const Projector * prj);

	Planet* search(string object_name);
	Planet* searchByCommonNames(string planetCommonName);
	Planet* get_sun(void) {return sun;}
	Planet* get_earth(void) {return earth;}
	Planet* get_moon(void) {return moon;}

	void set_sky_locale(string _sky_locale);
	void start_trails(void);
	void end_trails(void);
	void set_trail_color(const Vec3f& c)  {Planet::set_trail_color(c);}
	void update_trails(const Navigator* nav);
	void set_object_scale(float scale);

	vector<string> getNames(void);
private:
	Planet* sun;
	Planet* moon;
	Planet* earth;

	// solar system related settings
	float object_scale;  // should be kept synchronized with star scale...

	s_font* planet_name_font;
	vector<Planet*> system_planets;		// Vector containing all the bodies of the system
	vector<EllipticalOrbit*> ell_orbits;// Pointers on created elliptical orbits
	bool near_lunar_eclipse(const Navigator * nav, Projector * prj);
	
	// draw earth shadow on moon for lunar eclipses
	void draw_earth_shadow(const Navigator * nav, Projector * prj);  

	// And sort them from the furthest to the closest to the observer
	struct bigger_distance : public binary_function<Planet*, Planet*, bool>
	{
		bool operator()(Planet* p1, Planet* p2) { return p1->get_distance() > p2->get_distance(); }
	};

	s_texture *tex_earth_shadow;  // for lunar eclipses
};


#endif // _SOLARSYSTEM_H_

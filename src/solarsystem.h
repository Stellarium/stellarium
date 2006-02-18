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
#include "translator.h"

class SolarSystem
{
public:
    SolarSystem();
    virtual ~SolarSystem();

	void update(int delta_time, Navigator* nav);
	
	// Draw all the elements of the solar system
    void draw(Projector * du, const Navigator * nav, 
			  const ToneReproductor* eye, bool _gravity_label, 
			  bool flag_point);
	
	// Load the bodies data from a file
	void load(const string& planetfile);

	//! @brief Update i18 names from english names according to passed translator
	//! The translation is done using gettext with translated strings defined in translations.h
	void translateNames(Translator& trans);

	void setFont(float font_size, const string& font_name);
	void setLabelColor(const Vec3f& c) {Planet::set_label_color(c);}
	void setOrbitColor(const Vec3f& c) {Planet::set_orbit_color(c);}
  
	// Compute the position for every elements of the solar system.
	void computePositions(double date);

	// Compute the transformation matrix for every elements of the solar system.
    void computeTransMatrices(double date);

	// Search if any Planet is close to position given in earth equatorial position.
	Planet* search(Vec3d, const Navigator * nav, const Projector * prj) const;

	// Return a stl vector containing the planets located inside the lim_fov circle around position v
	vector<StelObject*> search_around(Vec3d v,
                                      double lim_fov,
                                      const Navigator * nav,
                                      const Projector * prj) const;

	//! Return the matching planet pointer if exists or NULL
	Planet* searchByEnglishName(string planetEnglishName) const;
	
	//! Return the matching planet pointer if exists or NULL
	Planet* searchByNamesI18(wstring planetNameI18) const;
	
	Planet* getSun(void) const {return sun;}
	Planet* getEarth(void) const {return earth;}
	Planet* getMoon(void) const {return moon;}

	//! Activate/Deactivate planets display
	void setFlagPlanets(bool b) {Planet::setflagShow(b);}
	bool getFlagPlanets(void) const {return Planet::getflagShow();}

	//! Activate/Deactivate planets trails display
	void setFlagTrails(bool b);
	bool getFlagTrails(void) const;
	
	//! Activate/Deactivate planets hints display
	void setFlagHints(bool b);
	bool getFlagHints(void);	
	
	//! Activate/Deactivate planets hints display
	void setFlagOrbits(bool b);
	bool getFlagOrbits(void);	
	
	//! Start/stop accumulating new trail data (clear old data)
	void startTrails(bool b);
	
	void setTrailColor(const Vec3f& c)  {Planet::set_trail_color(c);}
	void updateTrails(const Navigator* nav);
	
	//! Set/Get base planets display scaling factor 
	void setScale(float scale) {Planet::setScale(scale);}
	float getScale(void) const {return Planet::getScale();};
	
	//! Get list of all the translated planets name
	vector<wstring> getNamesI18(void);
	
	//! Set selected planet by english name or "" to select none
	void setSelected(const string& englishName) {setSelected(searchByEnglishName(englishName));}
	
	//! Set selected object from its pointer
	void setSelected(Planet * p);
	
	//! Get selected object's pointer
	Planet* getSelected(void) const {return selected;}
	
private:
	Planet* sun;
	Planet* moon;
	Planet* earth;
	
	//! The currently selected planet
	Planet* selected;

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

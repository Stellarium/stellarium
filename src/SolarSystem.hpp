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
//sun is already defined in Sun C/Solaris
#if defined(sun)
#undef sun
#endif
#include <vector>
#include <functional>

#include "stellarium.h"
#include "Planet.hpp"
#include "StelObject.hpp"
#include "StelObjectModule.hpp"
#include "STextureTypes.hpp"

class Orbit;
class LoadingBar;
class Translator;
class InitParser;

class SolarSystem : public StelObjectModule
{
public:
    SolarSystem();
    virtual ~SolarSystem();

	///////////////////////////////////////////////////////////////////////////
	// Methods defined in the StelModule class
	virtual void init(const InitParser& conf, LoadingBar& lb);
	virtual string getModuleID() const {return "ssystem";}
	//! Return the maximum squared distance in pixels that any planet has travelled since the last update.
	virtual double draw(Projector *prj, const Navigator *nav, ToneReproducer *eye);
	virtual void update(double deltaTime);
	virtual void updateI18n();
	virtual void selectedObjectChangeCallBack();
	virtual void setColorScheme(const InitParser& conf, const std::string& section);
	
	///////////////////////////////////////////////////////////////////////////
	// Methods defined in StelObjectManager class
	//! Return a stl vector containing the planets located inside the limitFov circle around position v
	virtual vector<boost::intrusive_ptr<StelObject> > searchAround(const Vec3d& v, double limitFov, const Navigator * nav, const Projector * prj) const;
	//! Return the matching planet pointer if exists or NULL
	//! @param planetNameI18n The case sensistive translated planet name
	virtual boost::intrusive_ptr<StelObject> searchByNameI18n(const wstring& nameI18n) const;
	
	//! @brief Find and return the list of at most maxNbItem objects auto-completing the passed object I18n name
	//! @param objPrefix the case insensitive first letters of the searched object
	//! @param maxNbItem the maximum number of returned object names
	//! @return a vector of matching object name by order of relevance, or an empty vector if nothing match
	virtual vector<wstring> listMatchingObjectsI18n(const wstring& objPrefix, unsigned int maxNbItem=5) const;
	
	///////////////////////////////////////////////////////////////////////////
	// Properties setters and getters
	//! Activate/Deactivate planets display
	void setFlagPlanets(bool b) {Planet::setflagShow(b);}
	bool getFlagPlanets(void) const {return Planet::getflagShow();}

	//! Activate/Deactivate planets trails display
	void setFlagTrails(bool b);
	bool getFlagTrails(void) const;
	
	//! Activate/Deactivate planets hints display
	void setFlagHints(bool b);
	bool getFlagHints(void) const;	
	
	//! Activate/Deactivate planets hints display
	void setFlagOrbits(bool b);
	bool getFlagOrbits(void) const {return flagOrbits;}

	//! Activate/Deactivate light travel time correction
	void setFlagLightTravelTime(bool b) {flag_light_travel_time = b;}
	bool getFlagLightTravelTime(void) const {return flag_light_travel_time;}	
	
	//! Set planet names font size
	void setFontSize(float newFontSize);
	
	//! Set/Get planets names color
	void setNamesColor(const Vec3f& c) {Planet::set_label_color(c);}
	const Vec3f& getNamesColor(void) const {return Planet::getLabelColor();}
	
	//! Set/Get orbits lines color
	void setOrbitsColor(const Vec3f& c) {Planet::set_orbit_color(c);}
	Vec3f getOrbitsColor(void) const {return Planet::getOrbitColor();}
	
	//! Set/Get planets trails color
	void setTrailsColor(const Vec3f& c)  {Planet::set_trail_color(c);}
	Vec3f getTrailsColor(void) const {return Planet::getTrailColor();}
	
	//! Set/Get base planets display scaling factor 
	void setScale(float scale) {Planet::setScale(scale);}
	float getScale(void) const {return Planet::getScale();}
	
	//! Activate/Deactivate display of planets halos as GLPoints
	void setFlagPoint(bool b) {flagPoint = b;}
	bool getFlagPoint() {return flagPoint;}
	
	//! Set/Get if Moon display is scaled
	void setFlagMoonScale(bool b)
      {if (!b) getMoon()->set_sphere_scale(1);
       else getMoon()->set_sphere_scale(moonScale);
       flagMoonScale = b;}
	bool getFlagMoonScale(void) const {return flagMoonScale;}
	
	//! Set/Get Moon display scaling factor 
	void setMoonScale(float f) {moonScale = f; if (flagMoonScale) getMoon()->set_sphere_scale(moonScale);}
	float getMoonScale(void) const {return moonScale;}		
	
	///////////////////////////////////////////////////////////////////////////
	// Other public methods
	wstring getPlanetHashString();  // locale and ssystem.ini names, newline delimiter, for tui
 
	//! Compute the position and transform matrix for every elements of the solar system.
    //! observerPos is needed for light travel time computation.
    //! @param observerPos position of the observer in heliocentric ecliptic frame.
	void computePositions(double date, const Vec3d& observerPos = Vec3d(0,0,0));

	// Search if any Planet is close to position given in earth equatorial position.
	StelObject* search(Vec3d, const Navigator * nav, const Projector * prj) const;

	//! Return the matching planet pointer if exists or NULL
	Planet* searchByEnglishName(string planetEnglishName) const;
	
	Planet* getSun(void) const {return sun;}
	Planet* getEarth(void) const {return earth;}
	Planet* getMoon(void) const {return moon;}
	
	//! Start/stop accumulating new trail data (clear old data)
	void startTrails(bool b);
	
	void updateTrails(const Navigator* nav);
	
	//! Get list of all the translated planets name
	vector<wstring> getNamesI18(void);
	
private:
	// Compute the transformation matrix for every elements of the solar system.
    // observerPos is needed for light travel time computation
    void computeTransMatrices(double date, const Vec3d& observerPos = Vec3d(0,0,0));

	//! Draw a nice animated pointer around the object
	void drawPointer(const Projector* prj, const Navigator * nav);

	// Load the bodies data from a file
	void loadPlanets(LoadingBar& lb);

	Planet* sun;
	Planet* moon;
	Planet* earth;
	
	//! Set selected planets by englishName
	//! @param englishName The planet name or "" to select no planet
	void setSelected(const string& englishName) {setSelected(searchByEnglishName(englishName));}
	//! Set selected object from its pointer
	void setSelected(StelObject* obj);	
	//! Get selected object's pointer
	StelObject* getSelected(void) const {return selected;}	
	//! The currently selected planet
	StelObject* selected;

	// solar system related settings
	float object_scale;  // should be kept synchronized with star scale...

	bool flagMoonScale;
	float moonScale;	// Moon scale value

	double fontSize;
	SFont& planet_name_font;
	
	// Whether to display halo as GLpoints
	bool flagPoint;
	
	vector<Planet*> system_planets;		// Vector containing all the bodies of the system
	vector<Orbit*> orbits;// Pointers on created elliptical orbits
	bool near_lunar_eclipse(const Navigator * nav, Projector * prj);
	
	// draw earth shadow on moon for lunar eclipses
	void draw_earth_shadow(const Navigator * nav, Projector * prj);  

	// And sort them from the furthest to the closest to the observer
	struct bigger_distance : public binary_function<Planet*, Planet*, bool>
	{
		bool operator()(Planet* p1, Planet* p2) { return p1->get_distance() > p2->get_distance(); }
	};

	STextureSP tex_earth_shadow;  // for lunar eclipses

	// Master settings
	bool flagOrbits;
	bool flag_light_travel_time;
	
	STextureSP texPointer;			// The selection pointer texture
};


#endif // _SOLARSYSTEM_H_

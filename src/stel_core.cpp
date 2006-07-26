/*
 * Copyright (C) 2003 Fabien Chereau
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

// Main class for stellarium
// Manage all the objects to be used in the program

#include "stel_core.h"
#include "stellastro.h"
#include "stel_utility.h"

#include "hip_star_mgr.h"
#include "telescope_mgr.h"

void StelCore::setFlagTelescopes(bool b) {
  telescope_mgr->setFlagTelescopes(b);
}

bool StelCore::getFlagTelescopes(void) const {
  return telescope_mgr->getFlagTelescopes();
}

void StelCore::setFlagTelescopeName(bool b) {
  telescope_mgr->setFlagTelescopeName(b);
}

bool StelCore::getFlagTelescopeName(void) const {
  return telescope_mgr->getFlagTelescopeName();
}

void StelCore::telescopeGoto(int nr) {
  if (selected_object) {
    telescope_mgr->telescopeGoto(nr,
                                 selected_object.getObsJ2000Pos(navigation));
  }
}


StelCore::StelCore(const string& LDIR, const string& DATA_ROOT) :
		skyTranslator(APP_NAME, LOCALEDIR, ""),
		projection(NULL), selected_object(NULL), hip_stars(NULL),
		nebulas(NULL), ssystem(NULL), milky_way(NULL), telescope_mgr(NULL),
		deltaFov(0.), 
		deltaAlt(0.), deltaAz(0.), move_speed(0.00025), firstTime(1)
{
	localeDir = LDIR;
	dataRoot = DATA_ROOT;
	
	projection = Projector::create(Projector::PERSPECTIVE_PROJECTOR, Vec4i(0,0,800,600), 60);
	glFrontFace(projection->needGlFrontFaceCW()?GL_CW:GL_CCW);

	tone_converter = new ToneReproductor();
	atmosphere = new Atmosphere();
	hip_stars = new HipStarMgr();
	asterisms = new ConstellationMgr(hip_stars);
	ssystem = new SolarSystem();
	observatory = new Observator(*ssystem);
	navigation = new Navigator(observatory);
	nebulas = new NebulaMgr();
	milky_way = new MilkyWay();
	equ_grid = new SkyGrid(SkyGrid::EQUATORIAL);
	azi_grid = new SkyGrid(SkyGrid::ALTAZIMUTAL);
	equator_line = new SkyLine(SkyLine::EQUATOR);
	ecliptic_line = new SkyLine(SkyLine::ECLIPTIC);
	meridian_line = new SkyLine(SkyLine::MERIDIAN, 1, 36);
	cardinals_points = new Cardinals();
	meteors = new MeteorMgr(10, 60);
	landscape = new LandscapeOldStyle();
	skyloc = new SkyLocalizer(getDataDir()+ "sky_cultures");
	script_images = new ImageMgr();
    telescope_mgr = new TelescopeMgr;
		
	// Set textures directory and suffix
	s_texture::set_texDir(getDataRoot() + "/textures/");
	
	object_pointer_visibility = 1;
}

StelCore::~StelCore()
{
	  // release the previous StelObject:
	selected_object = StelObject();
	delete navigation;
	delete projection;
	delete asterisms;
	delete hip_stars;
	delete nebulas;
	delete equ_grid;
	delete azi_grid;
	delete equator_line;
	delete ecliptic_line;
	delete meridian_line;
	delete cardinals_points;
	delete landscape; landscape = NULL;
	delete observatory; observatory = NULL;
	delete milky_way;
	delete meteors; meteors = NULL;
	delete atmosphere;
	delete tone_converter;
	delete ssystem;
	delete skyloc; skyloc = NULL;
	delete script_images;
	delete telescope_mgr;
	StelObject::delete_textures(); // Unload the pointer textures
}

// Load core data and initialize with default values
void StelCore::init(const InitParser& conf)
{
	baseFontFile = getDataDir() + conf.get_str("gui", "base_font_name", "DejaVuSans.ttf");

	// Video Section
	setViewportSize(conf.get_int("video:screen_w"), conf.get_int("video:screen_h"));
	setViewportHorizontalOffset(conf.get_int    ("video:horizontal_offset"));
	setViewportVerticalOffset(conf.get_int    ("video:vertical_offset"));
	
	// Projector
	string tmpstr = conf.get_str("projection:type");
	setProjectionType(tmpstr);

	tmpstr = conf.get_str("projection:viewport");
	const Projector::PROJECTOR_MASK_TYPE projMaskType = Projector::stringToMaskType(tmpstr);
	projection->setMaskType(projMaskType);
	
	// Init the solar system first
	if(firstTime) ssystem->load(getDataDir() + "ssystem.ini");
	
 	ssystem->setFont(FontSizeSolarSystem, baseFontFile);
	setPlanetsScale(getStarScale());

	observatory->load(conf, "init_location");

	navigation->set_JDay(get_julian_from_sys());
	navigation->set_local_vision(Vec3f(1,1e-05,0.2));

	// Load hipparcos stars & names
	if(firstTime) {
		LoadingBar lb(projection, FontSizeGeneral, baseFontFile, "logo24bits.png", getViewportWidth(), getViewportHeight(), StelUtility::stringToWstring(VERSION), 45, 320, 121);
		hip_stars->init(FontSizeGeneral, baseFontFile, getDataDir() + "hipparcos.fab", getDataDir() + "sky_cultures/western/star_names.fab", getDataDir() + "name.fab", lb);

		// Init nebulas
		nebulas->read(FontSizeGeneral, baseFontFile, getDataDir() + "ngc2000.dat", getDataDir() + "ngc2000names.dat", getDataDir() + "nebula_textures.fab", lb);
	}

	// Init fonts : should be moved into the constructor
	equ_grid->set_font(FontSizeGeneral, baseFontFile);
	azi_grid->set_font(FontSizeGeneral, baseFontFile);
	equator_line->set_font(FontSizeGeneral, baseFontFile);
	ecliptic_line->set_font(FontSizeGeneral, baseFontFile);
	meridian_line->set_font(FontSizeGeneral, baseFontFile);
	cardinals_points->set_font(FontSizeCardinalPoints, baseFontFile);

	// Init milky way
	if(firstTime) milky_way->set_texture("milkyway.png");

	setLandscape(observatory->get_landscape_name());

	if(firstTime) telescope_mgr->init(conf);

	// Load the pointer textures
	StelObject::init_textures();

	// now redo this so we fill the autocomplete dialogs now UI inititalised
	// set_system_locale_by_name(SkyLocale); // and UILocale are the same but different format fra vs fr_FR!!!! TONY

	tone_converter->set_world_adaptation_luminance(3.75f + atmosphere->get_intensity()*40000.f);

	// Compute planets data and init viewing position
	// Position of sun and all the satellites (ie planets)
	ssystem->computePositions(navigation->get_JDay(),
	                          navigation->getHomePlanet());
	// Matrix for sun and all the satellites (ie planets)
	ssystem->computeTransMatrices(navigation->get_JDay(),
	                              navigation->getHomePlanet());

	// Compute transform matrices between coordinates systems
	navigation->update_transform_matrices();
	navigation->update_model_view_mat();

	// Load constellations from the correct sky culture
	string tmp = conf.get_str("localization", "sky_culture", "western");
	setSkyCultureDir(tmp);
	skyCultureDir = tmp;

	setPlanetsSelected("");	// Fix a bug on macosX! Thanks Fumio!

	string skyLocaleName = conf.get_str("localization", "sky_locale", "system");
	constellationFontSize = conf.get_double("viewing","constellation_font_size",FontSizeConstellations);
	setSkyLanguage(skyLocaleName);	
	
	// Star section
	setStarScale(conf.get_double ("stars:star_scale"));
	setPlanetsScale(conf.get_double ("stars:star_scale"));  // if reload config

	setStarMagScale(conf.get_double ("stars:star_mag_scale"));
	setStarTwinkleAmount(conf.get_double ("stars:star_twinkle_amount"));
	setMaxMagStarName(conf.get_double ("stars:max_mag_star_name"));
	setFlagStarTwinkle(conf.get_boolean("stars:flag_star_twinkle"));
	setFlagPointStar(conf.get_boolean("stars:flag_point_star"));
	setStarLimitingMag(conf.get_double("stars", "star_limiting_mag", 6.5f));
	
	FlagEnableZoomKeys	= conf.get_boolean("navigation:flag_enable_zoom_keys");
	FlagEnableMoveKeys  = conf.get_boolean("navigation:flag_enable_move_keys");
	FlagManualZoom		= conf.get_boolean("navigation:flag_manual_zoom");
	

	auto_move_duration	= conf.get_double ("navigation","auto_move_duration",1.5);
	move_speed			= conf.get_double("navigation","move_speed",0.0004);
	zoom_speed			= conf.get_double("navigation","zoom_speed", 0.0004);

	// Viewing Mode
	tmpstr = conf.get_str("navigation:viewing_mode");
	if (tmpstr=="equator") 	navigation->set_viewing_mode(Navigator::VIEW_EQUATOR);
	else
	{
		if (tmpstr=="horizon") navigation->set_viewing_mode(Navigator::VIEW_HORIZON);
		else
		{
			cerr << "ERROR : Unknown viewing mode type : " << tmpstr << endl;
			assert(0);
		}
	}

	InitFov				= conf.get_double ("navigation","init_fov",60.);
	projection->set_fov(InitFov);

	InitViewPos = StelUtility::str_to_vec3f(conf.get_str("navigation:init_view_pos").c_str());
	navigation->set_local_vision(InitViewPos);


	// Landscape section
	setFlagLandscape(conf.get_boolean("landscape", "flag_landscape", conf.get_boolean("landscape", "flag_ground", 1)));  // name change
	setFlagFog(conf.get_boolean("landscape:flag_fog"));
	setFlagAtmosphere(conf.get_boolean("landscape:flag_atmosphere"));
	setAtmosphereFadeDuration(conf.get_double("landscape","atmosphere_fade_duration",1.5));

	// Viewing section
	setFlagConstellationLines(		conf.get_boolean("viewing:flag_constellation_drawing"));
	setFlagConstellationNames(		conf.get_boolean("viewing:flag_constellation_name"));
	setFlagConstellationBoundaries(	conf.get_boolean("viewing","flag_constellation_boundaries",false));
	setFlagConstellationArt(		conf.get_boolean("viewing:flag_constellation_art"));
	setFlagConstellationIsolateSelected(conf.get_boolean("viewing", "flag_constellation_isolate_selected",conf.get_boolean("viewing", "flag_constellation_pick", 0)));
	setConstellationArtIntensity(conf.get_double("viewing","constellation_art_intensity", 0.5));
	setConstellationArtFadeDuration(conf.get_double("viewing","constellation_art_fade_duration",2.));

	setFlagAzimutalGrid(conf.get_boolean("viewing:flag_azimutal_grid"));
	setFlagEquatorGrid(conf.get_boolean("viewing:flag_equatorial_grid"));
	setFlagEquatorLine(conf.get_boolean("viewing:flag_equator_line"));
	setFlagEclipticLine(conf.get_boolean("viewing:flag_ecliptic_line"));
	setFlagMeridianLine(conf.get_boolean("viewing:flag_meridian_line"));
	cardinals_points->setFlagShow(conf.get_boolean("viewing:flag_cardinal_points"));
	setFlagGravityLabels( conf.get_boolean("viewing:flag_gravity_labels") );
	setFlagMoonScaled(conf.get_boolean("viewing", "flag_moon_scaled", conf.get_boolean("viewing", "flag_init_moon_scaled", false)));  // name change
	setMoonScale(conf.get_double ("viewing","moon_scale",5.));

	// Astro section
	setFlagStars(conf.get_boolean("astro:flag_stars"));
	setFlagStarName(conf.get_boolean("astro:flag_star_name"));
	setFlagTelescopes(conf.get_boolean("astro:flag_telescopes"));
	setFlagTelescopeName(conf.get_boolean("astro:flag_telescope_name"));
	setFlagPlanets(conf.get_boolean("astro:flag_planets"));
	setFlagPlanetsHints(conf.get_boolean("astro:flag_planets_hints"));
	setFlagPlanetsOrbits(conf.get_boolean("astro:flag_planets_orbits"));
	setFlagLightTravelTime(conf.get_boolean("astro:flag_light_travel_time"));
	setFlagPlanetsTrails(conf.get_boolean("astro", "flag_object_trails", false));
	startPlanetsTrails(conf.get_boolean("astro", "flag_object_trails", false));
	setFlagNebula(conf.get_boolean("astro:flag_nebula"));
	setFlagNebulaHints(conf.get_boolean("astro:flag_nebula_name"));
	setNebulaMaxMagHints(conf.get_double("astro", "max_mag_nebula_name", 99));
	setNebulaCircleScale(conf.get_double("astro", "nebula_scale",1.0f));
	setFlagNebulaDisplayNoTexture(conf.get_boolean("astro", "flag_nebula_display_no_texture", false));
	setFlagMilkyWay(conf.get_boolean("astro:flag_milky_way"));
	setMilkyWayIntensity(conf.get_double("astro","milky_way_intensity",1.));
	setFlagBrightNebulae(conf.get_boolean("astro:flag_bright_nebulae"));	

	setMeteorsRate(conf.get_int("astro", "meteor_rate", 10));

	firstTime = 0;
}

// Update all the objects in function of the time
void StelCore::update(int delta_time)
{
	// Update the position of observation and time etc...
	observatory->update(delta_time);
	navigation->update_time(delta_time);

	// Position of sun and all the satellites (ie planets)
	ssystem->computePositions(navigation->get_JDay(),
	                          navigation->getHomePlanet());
	// Matrix for sun and all the satellites (ie planets)
	ssystem->computeTransMatrices(navigation->get_JDay(),
	                              navigation->getHomePlanet());
	// communicate with the telescopes:
	telescope_mgr->communicate();

	// Transform matrices between coordinates systems
	navigation->update_transform_matrices();
	// Direction of vision
	navigation->update_vision_vector(delta_time, selected_object);
	// Field of view
	projection->update_auto_zoom(delta_time);

	// update faders and Planet trails (call after nav is updated)
	ssystem->update(delta_time, navigation);

	// Move the view direction and/or fov
	updateMove(delta_time);

	// Update info about selected object
	selected_object.update();

	// Update faders
	equ_grid->update(delta_time);
	azi_grid->update(delta_time);
	equator_line->update(delta_time);
	ecliptic_line->update(delta_time);
	meridian_line->update(delta_time);
	asterisms->update(delta_time);
	atmosphere->update(delta_time);
	landscape->update(delta_time);
	hip_stars->update(delta_time);
	nebulas->update(delta_time);
	cardinals_points->update(delta_time);
	milky_way->update(delta_time);
	telescope_mgr->update(delta_time);
	
	// Compute the sun position in local coordinate
	Vec3d temp(0.,0.,0.);
	Vec3d sunPos = navigation->helio_to_local(temp);


	// Compute the moon position in local coordinate
	Vec3d moon = ssystem->getMoon()->get_heliocentric_ecliptic_pos();
	Vec3d moonPos = navigation->helio_to_local(moon);

	// Compute the atmosphere color and intensity
	atmosphere->compute_color(navigation->get_JDay(), sunPos, moonPos,
	                          ssystem->getMoon()->get_phase(ssystem->getEarth()->get_heliocentric_ecliptic_pos()),
	                          tone_converter, projection, observatory->get_latitude(), observatory->get_altitude(),
	                          15.f, 40.f);	// Temperature = 15c, relative humidity = 40%
	tone_converter->set_world_adaptation_luminance(atmosphere->get_world_adaptation_luminance());

	sunPos.normalize();
	moonPos.normalize();
	// compute global sky brightness TODO : make this more "scientifically"
	// TODO: also add moonlight illumination

	if(sunPos[2] < -0.1/1.5 ) sky_brightness = 0.01;
	else sky_brightness = (0.01 + 1.5*(sunPos[2]+0.1/1.5));

	// TODO make this more generic for non-atmosphere planets
	if(atmosphere->get_fade_intensity() == 1) {
		// If the atmosphere is on, a solar eclipse might darken the sky
		// otherwise we just use the sun position calculation above
		sky_brightness *= (atmosphere->get_intensity()+0.1);
	}

	// TODO: should calculate dimming with solar eclipse even without atmosphere on
	landscape->set_sky_brightness(sky_brightness+0.05);
}

// Execute all the drawing functions
double StelCore::draw(int delta_time)
{
	// Init openGL viewing with fov, screen size and clip planes
	projection->set_clipping_planes(0.000001 ,50);

	// Init viewport to current projector values
	projection->applyViewport();

	// Give the updated standard projection matrices to the projector
	projection->set_modelview_matrices(	navigation->get_earth_equ_to_eye_mat(),
	                                    navigation->get_helio_to_eye_mat(),
	                                    navigation->get_local_to_eye_mat(),
	                                    navigation->get_j2000_to_eye_mat());

	// Set openGL drawings in equatorial coordinates
	navigation->switch_to_earth_equatorial();

	glBlendFunc(GL_ONE, GL_ONE);

	// Draw the milky way.
	milky_way->draw(tone_converter, projection, navigation);

	// Draw all the constellations
	asterisms->draw(projection, navigation);

	// Draw the nebula
	nebulas->draw(projection, navigation, tone_converter);

	// Draw the hipparcos stars
	Vec3d tempv = navigation->get_prec_equ_vision();
	Vec3f temp(tempv[0],tempv[1],tempv[2]);
	hip_stars->draw(temp, tone_converter, projection);

	// Draw the equatorial grid
	equ_grid->draw(projection);

	// Draw the altazimutal grid
	azi_grid->draw(projection);

	// Draw the celestial equator line
	equator_line->draw(projection);

	// Draw the ecliptic line
	ecliptic_line->draw(projection);

	// Draw the meridian line
	meridian_line->draw(projection);

	// Draw the pointer on the currently selected object
	if (selected_object && object_pointer_visibility) selected_object.draw_pointer(delta_time, projection, navigation);

	// Draw the planets
	double squaredDistance = ssystem->draw(projection, navigation, tone_converter, getFlagPointStar());

	// Set openGL drawings in local coordinates i.e. generally altazimuthal coordinates
	navigation->switch_to_local();

	// Upade meteors
	meteors->update(projection, navigation, tone_converter, delta_time);

	if(!getFlagAtmosphere() || sky_brightness<0.1)
	{
		projection->set_orthographic_projection();
		meteors->draw(projection, navigation);
		projection->reset_perspective_projection();
	}

	// Draw the atmosphere
	atmosphere->draw(projection, delta_time);

	// Draw the landscape
	landscape->draw(tone_converter, projection, navigation);

	// Draw the cardinal points
	//if (FlagCardinalPoints)
	cardinals_points->draw(projection, observatory->get_latitude());

	telescope_mgr->draw(projection,navigation);

	// draw images loaded by a script
	projection->set_orthographic_projection();
	script_images->draw(navigation, projection);

	projection->reset_perspective_projection();

	projection->draw_viewport_shape();

	return squaredDistance;
}

bool StelCore::setLandscape(const string& new_landscape_name)
{
	if (new_landscape_name.empty()) return 0;
	Landscape* newLandscape = Landscape::create_from_file(getDataDir() + "landscapes.ini", new_landscape_name);

	if(!newLandscape) return 0;

	if (landscape)
	{
		// Copy parameters from previous landscape to new one
		newLandscape->setFlagShow(landscape->getFlagShow());
		newLandscape->setFlagShowFog(landscape->getFlagShowFog());
		delete landscape;
		landscape = newLandscape;
	}
	observatory->set_landscape_name(new_landscape_name);
	return 1;
}


//! Load a landscape based on a hash of parameters mirroring the landscape.ini file
//! and make it the current landscape
bool StelCore::loadLandscape(stringHash_t& param) {

	Landscape* newLandscape = Landscape::create_from_hash(param);
	if(!newLandscape) return 0;

	if (landscape)
	{
		// Copy parameters from previous landscape to new one
		newLandscape->setFlagShow(landscape->getFlagShow());
		newLandscape->setFlagShowFog(landscape->getFlagShowFog());
		delete landscape;
		landscape = newLandscape;
	}
	observatory->set_landscape_name(param["name"]);
	// probably not particularly useful, as not in landscape.ini file

	return 1;
}


void StelCore::setViewportSize(int w, int h)
{
	if (w==getViewportWidth() && h==getViewportHeight()) return;
	projection->setViewportWidth(w);
	projection->setViewportHeight(h);
}

StelObject StelCore::searchByNameI18n(const wstring &name) const
{
	StelObject rval;
	rval = ssystem->searchByNamesI18(name);
	if (rval) return rval;
	rval = nebulas->searchByNameI18n(name);
	if (rval) return rval;
	rval = hip_stars->searchByNameI18n(name);
	if (rval) return rval;
	rval = asterisms->searchByNameI18n(name);
	if (rval) return rval;
	rval = telescope_mgr->searchByNameI18n(name);
	return rval;
}

//! Find and select an object from its translated name
//! @param nameI18n the case sensitive object translated name
//! @return true if an object was found with the passed name	
bool StelCore::findAndSelectI18n(const wstring &nameI18n)
{
	// Then look for another object
	StelObject obj = searchByNameI18n(nameI18n);
	if (!obj) return false;
	else return selectObject(obj);
}


//! Find and select an object based on selection type and standard name or number
//! @return true if an object was selected

bool StelCore::selectObject(const string &type, const string &id) {
/*
  std::wostringstream oss;
  oss << id.c_str();
  return findAndSelectI18n(oss.str());
*/
    if(type=="hp") {
      unsigned int hpnum;
      std::istringstream istr(id);
      istr >> hpnum;
      selected_object = hip_stars->searchHP(hpnum);
      asterisms->setSelected(selected_object);
      setPlanetsSelected("");

	} else if(type=="star") {
      selected_object = hip_stars->search(id);
      asterisms->setSelected(selected_object);
      setPlanetsSelected("");

    } else if(type=="planet"){
      setPlanetsSelected(id);
      selected_object = ssystem->getSelected();
      asterisms->setSelected(StelObject());

    } else if(type=="nebula"){
      selected_object = nebulas->search(id);
      setPlanetsSelected("");
      asterisms->setSelected(StelObject());
	 
    } else if(type=="constellation"){

		// Select only constellation, nothing else 	 
		asterisms->setSelected(id); 

		selected_object = NULL;
		setPlanetsSelected("");

    } else if(type=="constellation_star") {
	
		// For Find capability, select a star in constellation so can center view on constellation 	 
		asterisms->setSelected(id); 

        selected_object = asterisms->getSelected()
                           .getBrightestStarInConstellation();

/// what is this?
/// 1) Find the hp-number of the 1st star in the selected constellation,
/// 2) find the star of this hpnumber
/// 3) select the constellation of this star ???
///		const unsigned int hpnum = asterisms->getFirstSelectedHP();
///		selected_object = hip_stars->searchHP(hpnum);
///		asterisms->setSelected(selected_object);

		setPlanetsSelected("");

///		// Some stars are shared, so now force constellation
///		asterisms->setSelected(id);
	} else {
		cerr << "Invalid selection type specified: " << type << endl;
		return 0;
	}


    if (selected_object) {
      if (navigation->get_flag_traking()) navigation->set_flag_lock_equ_pos(1);
      navigation->set_flag_traking(0);

	  return 1;
    }

	return 0;
}


//! Find and select an object near given equatorial position
bool StelCore::findAndSelect(const Vec3d& pos)
{
	StelObject tempselect = clever_find(pos);
	return selectObject(tempselect);
}

//! Find and select an object near given screen position
bool StelCore::findAndSelect(int x, int y)
{
	Vec3d v;
	projection->unproject_earth_equ(x,getViewportHeight()-y,v);
	return findAndSelect(v);
}

// Find an object in a "clever" way 
StelObject StelCore::clever_find(const Vec3d& v) const
{
	StelObject sobj;
	vector<StelObject> candidates;
	vector<StelObject> temp;
	Vec3d winpos;

	// Field of view for a 30 pixel diameter circle on screen
	float fov_around = projection->get_fov()/MY_MIN(projection->getViewportWidth(), projection->getViewportHeight()) * 30.f;

	float xpos, ypos;
	projection->project_earth_equ(v, winpos);
	xpos = winpos[0];
	ypos = winpos[1];

	// Collect the planets inside the range
	if (getFlagPlanets())
	{
		temp = ssystem->search_around(v, fov_around, navigation, projection);
		candidates.insert(candidates.begin(), temp.begin(), temp.end());
	}

	// nebulas and stars used precessed equ coords
	Vec3d p = navigation->earth_equ_to_j2000(v);

	// The nebulas inside the range
	if (getFlagNebula())
	{
		temp = nebulas->search_around(p, fov_around);
		candidates.insert(candidates.begin(), temp.begin(), temp.end());
	}

	// And the stars inside the range
	if (getFlagStars())
	{
		temp = hip_stars->search_around(p, fov_around);
		candidates.insert(candidates.begin(), temp.begin(), temp.end());
	}

	if (getFlagTelescopes())
	{
		temp = telescope_mgr->search_around(p, fov_around);
		candidates.insert(candidates.begin(), temp.begin(), temp.end());
	}

	// Now select the object minimizing the function y = distance(in pixel) + magnitude
	float best_object_value;
	best_object_value = 100000.f;
	vector<StelObject>::iterator iter = candidates.begin();
	while (iter != candidates.end())
	{
		projection->project_earth_equ((*iter).get_earth_equ_pos(navigation), winpos);

		float distance = sqrt((xpos-winpos[0])*(xpos-winpos[0]) + (ypos-winpos[1])*(ypos-winpos[1]));
		float mag = (*iter).get_mag(navigation);
		if ((*iter).get_type()==STEL_OBJECT_NEBULA)
		{
			if( nebulas->getFlagHints() )
			{
				// make very easy to select IF LABELED
				mag = -1;  

			}
		}
		if ((*iter).get_type()==STEL_OBJECT_PLANET)
		{
			if( getFlagPlanetsHints() )
			{
				// easy to select, especially pluto
				mag -= 15.f;
			}
			else
			{
				mag -= 8.f;
			}
		}
		if (distance + mag < best_object_value)
		{
			best_object_value = distance + mag;
			sobj = *iter;
		}
		iter++;
	}

	return sobj;
}

StelObject StelCore::clever_find(int x, int y) const
{
	Vec3d v;
	projection->unproject_earth_equ(x,y,v);
	return clever_find(v);
}

// Go and zoom to the selected object.
void StelCore::autoZoomIn(float move_duration, bool allow_manual_zoom)
{
	float manual_move_duration;

	if (!selected_object) return;

	if (!navigation->get_flag_traking())
	{
		navigation->set_flag_traking(true);
		navigation->move_to(selected_object.get_earth_equ_pos(navigation),
                            move_duration, false, 1);
		manual_move_duration = move_duration;
	}
	else
	{
		// faster zoom in manual zoom mode once object is centered
		manual_move_duration = move_duration*.66f;
	}

	if( allow_manual_zoom && FlagManualZoom )
	{
		// if manual zoom mode, user can zoom in incrementally
		float newfov = projection->get_fov()*0.5f;
		projection->zoom_to(newfov, manual_move_duration);

	}
	else
	{
		float satfov = selected_object.get_satellites_fov(navigation);

		if (satfov>0.0 && projection->get_fov()*0.9>satfov)
          projection->zoom_to(satfov, move_duration);
		else {
		  float closefov = selected_object.get_close_fov(navigation);
          if (projection->get_fov()>closefov)
            projection->zoom_to(closefov, move_duration);
        }
	}
}


// Unzoom and go to the init position
void StelCore::autoZoomOut(float move_duration, bool full)
{

	if (selected_object && !full) {

		// If the selected object has satellites, unzoom to satellites view
        // unless specified otherwise
		float satfov = selected_object.get_satellites_fov(navigation);

		if (satfov>0.0 && projection->get_fov()<=satfov*0.9) {
          projection->zoom_to(satfov, move_duration);
          return;
	    }

		// If the selected object is part of a Planet subsystem (other than sun),
		// unzoom to subsystem view
        satfov = selected_object.get_parent_satellites_fov(navigation);
        if (satfov>0.0 && projection->get_fov()<=satfov*0.9) {
          projection->zoom_to(satfov, move_duration);
          return;
        }
	}	
  
	projection->zoom_to(InitFov, move_duration);
	navigation->move_to(InitViewPos, move_duration, true, -1);
	navigation->set_flag_traking(false);
	navigation->set_flag_lock_equ_pos(0);
	
}

// Set the current sky culture according to passed name
bool StelCore::setSkyCulture(const wstring& cultureName)
{
	return setSkyCultureDir(skyloc->skyCultureToDirectory(cultureName));
}

// Set the current sky culture from the passed directory
bool StelCore::setSkyCultureDir(const string& cultureDir)
{
	if(skyCultureDir == cultureDir) return 1;

	// make sure culture definition exists before attempting or will die
	// Do not comment this out! Rob
	if(skyloc->directoryToSkyCultureEnglish(cultureDir) == "")
 	{
 		cerr << "Invalid sky culture directory: " << cultureDir << endl;
 		return 0;
 	}

	skyCultureDir = cultureDir;

	if(!asterisms) return 1;

	LoadingBar lb(projection, FontSizeGeneral, baseFontFile, "logo24bits.png", getViewportWidth(), getViewportHeight(), StelUtility::stringToWstring(VERSION), 45, 320, 121);

	asterisms->loadLinesAndArt(getDataDir() + "sky_cultures/" + skyCultureDir + "/constellationship.fab",
	                           getDataDir() + "sky_cultures/" + skyCultureDir + "/constellationsart.fab", getDataDir() + "constellations_boundaries.dat", lb);
	asterisms->loadNames(getDataDir() + "sky_cultures/" + skyCultureDir + "/constellation_names.eng.fab");

	// Re-translated constellation names
	asterisms->translateNames(skyTranslator);

	// as constellations have changed, clear out any selection and retest for match!
	if (selected_object && selected_object.get_type()==STEL_OBJECT_STAR)
	{
		asterisms->setSelected(selected_object);
	}
	else
	{
		asterisms->setSelected(StelObject());
	}

	// Load culture star names in english
	hip_stars->load_common_names(getDataDir() + "sky_cultures/" + skyCultureDir + "/star_names.fab");
	// translate
	hip_stars->translateNames(skyTranslator);

	return 1;
}


//! @brief Set the sky locale and reload the sky objects names for gettext translation
void StelCore::setSkyLanguage(const std::string& newSkyLocaleName)
{
	if( !hip_stars || !cardinals_points || !asterisms) return; // objects not initialized yet

	string oldLocale = getSkyLanguage();

	// Update the translator with new locale name
	skyTranslator = Translator(PACKAGE, getLocaleDir(), newSkyLocaleName);
	// cout << "Sky locale is " << skyTranslator.getLocaleName() << endl;

	// see if fonts need to change
	string oldFontFile, newFontFile, tmpstr;
	float oldFontScale, newFontScale, tmpfloat;

	getFontForLocale(oldLocale, oldFontFile, oldFontScale, tmpstr, tmpfloat);
	getFontForLocale(newSkyLocaleName, newFontFile, newFontScale, tmpstr, tmpfloat);

	// If font has changed or init is being called for first time...
	if(oldFontFile != newFontFile || oldFontScale != newFontScale || firstTime) {
		
		cardinals_points->set_font(FontSizeCardinalPoints*newFontScale, newFontFile);
		asterisms->setFont(constellationFontSize*newFontScale, newFontFile);  // size is read from config
		ssystem->setFont(FontSizeSolarSystem*newFontScale, newFontFile);
		// not translating yet
		//		nebulas->setFont(FontSizeGeneral, font);
		hip_stars->setFont(FontSizeGeneral*newFontScale, newFontFile);
		telescope_mgr->setFont(FontSizeGeneral*newFontScale, newFontFile);

		// TODO: TUI short info font needs updating also
		// TEST - need different fixed font
		// ui->setFonts(newFontScale, newFontFile, newFontScale, newFontFile);

	}

	// Translate all labels with the new language
	cardinals_points->translateLabels(skyTranslator);
	asterisms->translateNames(skyTranslator);
	ssystem->translateNames(skyTranslator);
	nebulas->translateNames(skyTranslator);
	hip_stars->translateNames(skyTranslator);
}


// Please keep saveCurrentSettings up to date with any new color settings added here
void StelCore::setColorScheme(const string& skinFile, const string& section)
{
	InitParser conf;
	conf.load(skinFile);
	
	// simple default color, rather than black which doesn't show up
	string defaultColor = "0.6,0.4,0";

	// Load colors from config file
	nebulas->setLabelColor(StelUtility::str_to_vec3f(conf.get_str(section,"nebula_label_color", defaultColor)));
	nebulas->setCircleColor(StelUtility::str_to_vec3f(conf.get_str(section,"nebula_circle_color", defaultColor)));
	hip_stars->setLabelColor(StelUtility::str_to_vec3f(conf.get_str(section,"star_label_color", defaultColor)));
	hip_stars->setCircleColor(StelUtility::str_to_vec3f(conf.get_str(section,"star_circle_color", defaultColor)));
	telescope_mgr->set_label_color(StelUtility::str_to_vec3f(conf.get_str(section,"telescope_label_color", defaultColor)));
	telescope_mgr->set_circle_color(StelUtility::str_to_vec3f(conf.get_str(section,"telescope_circle_color", defaultColor)));
	ssystem->setLabelColor(StelUtility::str_to_vec3f(conf.get_str(section,"planet_names_color", defaultColor)));
	ssystem->setOrbitColor(StelUtility::str_to_vec3f(conf.get_str(section,"planet_orbits_color", defaultColor)));
	ssystem->setTrailColor(StelUtility::str_to_vec3f(conf.get_str(section,"object_trails_color", defaultColor)));
	equ_grid->setColor(StelUtility::str_to_vec3f(conf.get_str(section,"equatorial_color", defaultColor)));
	//equ_grid->set_top_transparancy(draw_mode==DM_NORMAL);
	azi_grid->setColor(StelUtility::str_to_vec3f(conf.get_str(section,"azimuthal_color", defaultColor)));
	//azi_grid->set_top_transparancy(draw_mode==DM_NORMAL);
	equator_line->setColor(StelUtility::str_to_vec3f(conf.get_str(section,"equator_color", defaultColor)));
	ecliptic_line->setColor(StelUtility::str_to_vec3f(conf.get_str(section,"ecliptic_color", defaultColor)));
	meridian_line->set_font(FontSizeGeneral, baseFontFile);
	meridian_line->setColor(StelUtility::str_to_vec3f(conf.get_str(section,"meridian_color", defaultColor)));
	cardinals_points->setColor(StelUtility::str_to_vec3f(conf.get_str(section,"cardinal_color", defaultColor)));
	asterisms->setLineColor(StelUtility::str_to_vec3f(conf.get_str(section,"const_lines_color", defaultColor)));
	asterisms->setBoundaryColor(StelUtility::str_to_vec3f(conf.get_str(section,"const_boundary_color", "0.8,0.3,0.3")));
	asterisms->setLabelColor(StelUtility::str_to_vec3f(conf.get_str(section,"const_names_color", defaultColor)));
	
	// Init milky way
// 	if (draw_mode == DM_NORMAL)	milky_way->set_texture("milkyway.png");
// 	else 
// 	{
// 		milky_way->set_texture("milkyway_chart.png",true);
// 	}
	
	chartColor = StelUtility::str_to_vec3f(conf.get_str(section,"chart_color", defaultColor));
}

//! Get a color used to display info about the currently selected object
Vec3f StelCore::getSelectedObjectInfoColor(void) const
{
	if (!selected_object)
	{
		cerr << "WARNING: StelCore::getSelectedObjectInfoColor was called while no object is currently selected!!" << endl;
		return Vec3f(1, 1, 1);
	}
	if (selected_object.get_type()==STEL_OBJECT_NEBULA) return nebulas->getLabelColor();
	if (selected_object.get_type()==STEL_OBJECT_PLANET) return ssystem->getLabelColor();
	if (selected_object.get_type()==STEL_OBJECT_STAR) return selected_object.get_RGB();
	return Vec3f(1, 1, 1);
}

void StelCore::drawChartBackground(void)
{
	int stepX = projection->getViewportWidth();
	int stepY = projection->getViewportHeight();
	int viewport_left = projection->getViewportPosX();
	int view_bottom = projection->getViewportPosY();

	glDisable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);
	glColor3fv(chartColor);
	projection->set_orthographic_projection();	// set 2D coordinate
	glBegin(GL_QUADS);
	glTexCoord2s(0, 0); glVertex2i(viewport_left, view_bottom);	// Bottom Left
	glTexCoord2s(1, 0); glVertex2i(viewport_left+stepX, view_bottom);	// Bottom Right
	glTexCoord2s(1, 1); glVertex2i(viewport_left+stepX,view_bottom+stepY);	// Top Right
	glTexCoord2s(0, 1); glVertex2i(viewport_left,view_bottom+stepY);	// Top Left
	glEnd();
	projection->reset_perspective_projection();
}

wstring StelCore::get_cursor_pos(int x, int y)
{
	Vec3d v;
	projection->unproject_earth_equ(x,y,v);
	float tempDE, tempRA;
	rect_to_sphe(&tempRA,&tempDE,v);
	return L"RA : " + StelUtility::printAngleHMS(tempRA) + L"\n" +
	L"DE : " + StelUtility::printAngleDMS(tempDE);
}

void StelCore::setProjectionType(const string& sptype)
{
	Projector::PROJECTOR_TYPE pType = Projector::stringToType(sptype);
	if (projection->getType()==pType) return;
	Projector *const ptemp = Projector::create(pType,
                                               projection->getViewport(),
                                               projection->get_fov());
	ptemp->setMaskType(projection->getMaskType());
	ptemp->setFlagGravityLabels(projection->getFlagGravityLabels());
	delete projection;
	projection = ptemp;
	glFrontFace(projection->needGlFrontFaceCW()?GL_CW:GL_CCW);
}

void StelCore::setFlipHorz(bool flip) {
  projection->setFlipHorz(flip);
  glFrontFace(projection->needGlFrontFaceCW()?GL_CW:GL_CCW);
}

void StelCore::setFlipVert(bool flip) {
  projection->setFlipVert(flip);
  glFrontFace(projection->needGlFrontFaceCW()?GL_CW:GL_CCW);
}

void StelCore::turn_right(int s)
{
	if (s && FlagEnableMoveKeys)
	{
		deltaAz = 1;
		setFlagTracking(false);
		setFlagLockSkyPosition(false);
	}
	else deltaAz = 0;
}

void StelCore::turn_left(int s)
{
	if (s && FlagEnableMoveKeys)
	{
		deltaAz = -1;
		setFlagTracking(false);
		setFlagLockSkyPosition(false);

	}
	else deltaAz = 0;
}

void StelCore::turn_up(int s)
{
	if (s && FlagEnableMoveKeys)
	{
		deltaAlt = 1;
		setFlagTracking(false);
		setFlagLockSkyPosition(false);
	}
	else deltaAlt = 0;
}

void StelCore::turn_down(int s)
{
	if (s && FlagEnableMoveKeys)
	{
		deltaAlt = -1;
		setFlagTracking(false);
		setFlagLockSkyPosition(false);
	}
	else deltaAlt = 0;
}


void StelCore::zoom_in(int s)
{
	if (FlagEnableZoomKeys) deltaFov = -1*(s!=0);
}

void StelCore::zoom_out(int s)
{
	if (FlagEnableZoomKeys) deltaFov = (s!=0);
}

//! Make the first screen position correspond to the second (useful for mouse dragging)
void StelCore::dragView(int x1, int y1, int x2, int y2)
{
	Vec3d tempvec1, tempvec2;
	double az1, alt1, az2, alt2;
	if (navigation->get_viewing_mode()==Navigator::VIEW_HORIZON)
	{
		projection->unproject_local(x2,getViewportHeight()-y2, tempvec2);
		projection->unproject_local(x1,getViewportHeight()-y1, tempvec1);
	}
	else
	{
		projection->unproject_earth_equ(x2,getViewportHeight()-y2, tempvec2);
		projection->unproject_earth_equ(x1,getViewportHeight()-y1, tempvec1);
	}
	rect_to_sphe(&az1, &alt1, tempvec1);
	rect_to_sphe(&az2, &alt2, tempvec2);
	navigation->update_move(az2-az1, alt1-alt2);
	setFlagTracking(false);
	setFlagLockSkyPosition(false);
}

// Increment/decrement smoothly the vision field and position
void StelCore::updateMove(int delta_time)
{
	// the more it is zoomed, the more the mooving speed is low (in angle)
	double depl=move_speed*delta_time*projection->get_fov();
	double deplzoom=zoom_speed*delta_time*projection->get_fov();
	if (deltaAz<0)
	{
		deltaAz = -depl/30;
		if (deltaAz<-0.2) deltaAz = -0.2;
	}
	else
	{
		if (deltaAz>0)
		{
			deltaAz = (depl/30);
			if (deltaAz>0.2) deltaAz = 0.2;
		}
	}
	if (deltaAlt<0)
	{
		deltaAlt = -depl/30;
		if (deltaAlt<-0.2) deltaAlt = -0.2;
	}
	else
	{
		if (deltaAlt>0)
		{
			deltaAlt = depl/30;
			if (deltaAlt>0.2) deltaAlt = 0.2;
		}
	}

	if (deltaFov<0)
	{
		deltaFov = -deplzoom*5;
		if (deltaFov<-0.15*projection->get_fov()) deltaFov = -0.15*projection->get_fov();
	}
	else
	{
		if (deltaFov>0)
		{
			deltaFov = deplzoom*5;
			if (deltaFov>20) deltaFov = 20;
		}
	}

	//	projection->change_fov(deltaFov);
	//	navigation->update_move(deltaAz, deltaAlt);

	if(deltaFov != 0 )
	{
		projection->change_fov(deltaFov);
//  		std::ostringstream oss;
//  		oss << "zoom delta_fov " << deltaFov;
//  		app->recordCommand(oss.str());
	}

	if(deltaAz != 0 || deltaAlt != 0)
	{
		navigation->update_move(deltaAz, deltaAlt);
//  		std::ostringstream oss;
//  		oss << "look delta_az " << deltaAz << " delta_alt " << deltaAlt;
//  		app->recordCommand(oss.str());
	}
	else
	{
		// must perform call anyway, but don't record!
		navigation->update_move(deltaAz, deltaAlt);
	}
}


bool StelCore::setHomePlanet(string planet) {

	// reset planet trails due to changed perspective
	ssystem->startTrails( ssystem->getFlagTrails() );

	return observatory->setHomePlanet(planet);
}


// For use by TUI
wstring StelCore::getPlanetHashString()
{
	return ssystem->getPlanetHashString();
}

//! Set stellarium time to current real world time
void StelCore::setTimeNow()
{
	navigation->set_JDay(get_julian_from_sys());
}

//! Get wether the current stellarium time is the real world time
bool StelCore::getIsTimeNow(void) const
{
	// cache last time to prevent to much slow system call
	static double lastJD = getJDay();
	static bool previousResult = (fabs(getJDay()-get_julian_from_sys())<JD_SECOND);
	if (fabs(lastJD-getJDay())>JD_SECOND/4)
	{
		lastJD = getJDay();
		previousResult = (fabs(getJDay()-get_julian_from_sys())<JD_SECOND);
	}
	return previousResult;
}

//! Select passed object
//! @return true if the object was selected (false if the same was already selected)
bool StelCore::selectObject(const StelObject &obj)
{
	// Unselect if it is the same object
	if (obj && selected_object==obj)
	{
		unSelect();
		return true;
	}

	if (obj.get_type()==STEL_OBJECT_CONSTELLATION)
	{
		return selectObject(obj.getBrightestStarInConstellation());
	}
	else
	{
		selected_object = obj;
	
		// If an object has been found
		if (selected_object)
		{	
			// If an object was selected keep the earth following
			if (getFlagTracking()) navigation->set_flag_lock_equ_pos(1);
			setFlagTracking(false);
	
			if (selected_object.get_type()==STEL_OBJECT_STAR)
			{
				asterisms->setSelected(selected_object);
	 			// potentially record this action
// 	 			std::ostringstream oss;
// 	 			oss << ((HipStar *)selected_object)->get_hp_number();
// 	 			app->recordCommand("select hp " + oss.str());
			}
			else
			{
				asterisms->setSelected(StelObject());
			}
	
			ssystem->setSelected(selected_object);
			// potentially record this action
//			app->recordCommand("select planet " + ((Planet *)selected_object)->getEnglishName());
	
			if (selected_object.get_type()==STEL_OBJECT_NEBULA)
			{
				// potentially record this action
// 				app->recordCommand("select nebula " + ((Nebula *)selected_object)->getEnglishName());
			}
			
			return true;
		}
		else
		{
			unSelect();
			return false;
		}
	}
	assert(0);	// Non reachable code
}


//! Find and return the list of at most maxNbItem objects auto-completing passed object I18 name
//! @param objPrefix the first letters of the searched object
//! @param maxNbItem the maximum number of returned object names
//! @return a vector of matching object name by order of relevance, or an empty vector if nothing match
vector<wstring> StelCore::listMatchingObjectsI18n(const wstring& objPrefix, unsigned int maxNbItem) const
{
	vector<wstring> result;
	vector <wstring>::const_iterator iter;
	
	// Get matching planets
	vector<wstring> matchingPlanets = ssystem->listMatchingObjectsI18n(objPrefix, maxNbItem);
	for (iter = matchingPlanets.begin(); iter != matchingPlanets.end(); ++iter)
		result.push_back(*iter);
	maxNbItem-=matchingPlanets.size();
	
	// Get matching constellations
	vector<wstring> matchingConstellations = asterisms->listMatchingObjectsI18n(objPrefix, maxNbItem);
	for (iter = matchingConstellations.begin(); iter != matchingConstellations.end(); ++iter)
		result.push_back(*iter);
	maxNbItem-=matchingConstellations.size();

	// Get matching nebulae
	vector<wstring> matchingNebulae = nebulas->listMatchingObjectsI18n(objPrefix, maxNbItem);
	for (iter = matchingNebulae.begin(); iter != matchingNebulae.end(); ++iter)
		result.push_back(*iter);
	maxNbItem-=matchingNebulae.size();
	
	// Get matching stars
	vector<wstring> matchingStars = hip_stars->listMatchingObjectsI18n(objPrefix, maxNbItem);
	for (iter = matchingStars.begin(); iter != matchingStars.end(); ++iter)
		result.push_back(*iter);
	maxNbItem-=matchingStars.size();
	
	// Get matching telescopes
	vector<wstring> matchingTelescopes = telescope_mgr->listMatchingObjectsI18n(objPrefix, maxNbItem);
	for (iter = matchingTelescopes.begin(); iter != matchingTelescopes.end(); ++iter)
		result.push_back(*iter);
	maxNbItem-=matchingStars.size();
	
	sort(result.begin(), result.end());
	
	return result;
}


//! TESTING
//! font file and scaling to use for a given locale
void StelCore::getFontForLocale(const string &_locale, string &_fontFile, float &_fontScale,
								string &_fixedFontFile, float &_fixedFontScale) {

	// TODO: Cache in data structure
	// TODO: check that font files exist
	// TODO: Move to translation or font class

	// Hardcoded default fonts here (override in fontmap.dat file)
	_fontFile = getDataDir() + "DejaVuSans.ttf";
	_fontScale = _fixedFontScale = 1.;
	_fixedFontFile = getDataDir() + "DejaVuSansMono.ttf";

	// Now see if another font should be used as default or for locale
	string mapFileName = getDataDir() + "fontmap.dat";
	ifstream *mapFile = new ifstream(mapFileName.c_str());

	if (! mapFile->is_open())
	{
		cout << "WARNING: Unable to open " << mapFileName << " resorting to default fonts." << endl;
		return;
	}

	char buffer[1000];
	string locale, font, fixedFont;
	float scale, fixedScale;

	while (mapFile->getline (buffer,999) && !mapFile->eof())
	{

		if( buffer[0] != '#' && buffer[0] != 0)
		{

			//			printf("Buffer is: %s\n", buffer);
			istringstream record(buffer);

			record >> locale >> font >> scale >> fixedFont >> fixedScale;

			// find matching records in map file
			if(locale == _locale || locale == "default") {

				//				cout << "locale " << _locale << endl;

				// Test that font files exist
				string fontFileName;
				ifstream *fontFile;

				fontFileName = getDataDir() + font;
				fontFile = new ifstream(fontFileName.c_str());

				if (! fontFile->is_open()) {
					cout << "WARNING: Unable to open " << fontFileName << " resorting to default font." << endl;
				} else {
					_fontFile = getDataDir() + font;
					_fontScale = scale;
				}
				delete fontFile;

				// normal font OK, test fixed font
				fontFileName = getDataDir() + fixedFont;
				fontFile = new ifstream(fontFileName.c_str());

				if (! fontFile->is_open()) {
					cout << "WARNING: Unable to open " << fontFileName << " resorting to default font." << endl;
				} else {
					_fixedFontFile = getDataDir() + fixedFont;
					_fixedFontScale = fixedScale;
				}
				delete fontFile;

				if(locale == _locale) {
					break;
				}
			}
		}
	}

	delete mapFile;
	return;

}



void StelCore::setFlagTracking(bool b) {

	if(!b || !selected_object) {
		navigation->set_flag_traking(0);
	} else {
		navigation->move_to(selected_object.get_earth_equ_pos(navigation),
							getAutomoveDuration());
		navigation->set_flag_traking(1);
	}

}






void StelCore::setFlagStars(bool b) {hip_stars->setFlagStars(b);}
bool StelCore::getFlagStars(void) const {return hip_stars->getFlagStars();}

void StelCore::setFlagStarName(bool b) {hip_stars->setFlagStarName(b);}
bool StelCore::getFlagStarName(void) const {return hip_stars->getFlagStarName();}

void StelCore::setFlagStarSciName(bool b) {hip_stars->setFlagStarSciName(b);}
bool StelCore::getFlagStarSciName(void) const {return hip_stars->getFlagStarSciName();}	

void StelCore::setFlagStarTwinkle(bool b) {hip_stars->setFlagStarTwinkle(b);}
bool StelCore::getFlagStarTwinkle(void) const {return hip_stars->getFlagStarTwinkle();}	

void StelCore::setFlagPointStar(bool b) {hip_stars->setFlagPointStar(b);}
bool StelCore::getFlagPointStar(void) const {return hip_stars->getFlagPointStar();}	

void StelCore::setMaxMagStarName(float f) {hip_stars->setMaxMagStarName(f);}
float StelCore::getMaxMagStarName(void) const {return hip_stars->getMaxMagStarName();}	

void StelCore::setMaxMagStarSciName(float f) {hip_stars->setMaxMagStarSciName(f);}
float StelCore::getMaxMagStarSciName(void) const {return hip_stars->getMaxMagStarSciName();}	

void StelCore::setStarScale(float f) {hip_stars->setStarScale(f);}
float StelCore::getStarScale(void) const {return hip_stars->getStarScale();}			

void StelCore::setStarMagScale(float f) {hip_stars->setStarMagScale(f);}
float StelCore::getStarMagScale(void) const {return hip_stars->getStarMagScale();}

void StelCore::setStarTwinkleAmount(float f) {hip_stars->setStarTwinkleAmount(f);}
float StelCore:: getStarTwinkleAmount(void) const {return hip_stars->getStarTwinkleAmount();}

void StelCore::setStarLimitingMag(float f) {hip_stars->setStarLimitingMag(f);}
float StelCore::getStarLimitingMag(void) const {return hip_stars->getStarLimitingMag();}


Vec3f StelCore::getColorStarNames(void) const {return hip_stars->getLabelColor();}			
Vec3f StelCore::getColorStarCircles(void) const {return hip_stars->getCircleColor();}			

void StelCore::setColorStarNames(const Vec3f& v) {hip_stars->setLabelColor(v);}
void StelCore::setColorStarCircles(const Vec3f& v) {hip_stars->setCircleColor(v);}

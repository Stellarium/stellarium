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

#include <algorithm>

#include "StelCore.hpp"
#include "StelApp.h"
#include "StelUtils.hpp"
#include "geodesic_grid.h"
#include "hip_star_mgr.h"
#include "telescope_mgr.h"
#include "constellation_mgr.h"
#include "solarsystem.h"
#include "nebula_mgr.h"
#include "GeodesicGridDrawer.h"
#include "constellation.h"
#include "LandscapeMgr.h"
#include "GridLinesMgr.h"
#include "MilkyWay.h"

#define LOADING_BAR_DEFAULT_FONT_SIZE 12.

void StelCore::setFlagTelescopes(bool b)
{
	telescope_mgr->setFlagTelescopes(b);
}

bool StelCore::getFlagTelescopes(void) const
{
	return telescope_mgr->getFlagTelescopes();
}

void StelCore::setFlagTelescopeName(bool b)
{
	telescope_mgr->setFlagTelescopeName(b);
}

bool StelCore::getFlagTelescopeName(void) const
{
	return telescope_mgr->getFlagTelescopeName();
}

void StelCore::telescopeGoto(int nr)
{
	if (selected_object)
	{
		telescope_mgr->telescopeGoto(nr,selected_object.getObsJ2000Pos(navigation));
	}
}


StelCore::StelCore(const string& LDIR, const string& DATA_ROOT, const boost::callback<void, string>& recordCallback) :
		projection(NULL), selected_object(NULL), hip_stars(NULL), asterisms(NULL),
		nebulas(NULL), ssystem(NULL), milky_way(NULL), telescope_mgr(NULL),
		deltaFov(0.), deltaAlt(0.), deltaAz(0.), move_speed(0.00025)
{
	recordActionCallback = recordCallback;

	projection = Projector::create(Projector::PERSPECTIVE_PROJECTOR, Vec4i(0,0,800,600), 60);
	glFrontFace(projection->needGlFrontFaceCW()?GL_CW:GL_CCW);

	tone_converter = new ToneReproductor();
	
	script_images = new ImageMgr();

	object_pointer_visibility = true;
	
	//geoDrawer = new GeodesicGridDrawer(9);
}

StelCore::~StelCore()
{
	// First delete external module
	StelModuleMgr& mmgr = StelApp::getInstance().getModuleMgr();
	for (StelModuleMgr::Iterator iter=mmgr.begin();iter!=mmgr.end();++iter)
	{
		if ((*iter)->isExternal())
			delete *iter;
	}
	
	//delete geoDrawer;
	// release the previous StelObject:
	selected_object = StelObject();
	delete navigation;
	delete projection;
	delete asterisms;
	delete hip_stars;
	delete nebulas;
	delete observatory;
	observatory = NULL;
	delete milky_way;
	delete meteors;
	meteors = NULL;
	delete tone_converter;
	delete ssystem;
	delete script_images;
	delete telescope_mgr;
	StelObject::delete_textures(); // Unload the pointer textures
	delete landscape;
	delete gridLines;
	if (geodesic_search_result)
	{
		delete geodesic_search_result;
		geodesic_search_result = NULL;
	}
	if (geodesic_grid)
	{
		delete geodesic_grid;
		geodesic_grid = NULL;
	}
}

// Load core data and initialize with default values
void StelCore::init(const InitParser& conf)
{	
	// Projector
	string tmpstr = conf.get_str("projection:type");
	setProjectionType(tmpstr);
	projection->init(conf);

	LoadingBar lb(projection, LOADING_BAR_DEFAULT_FONT_SIZE, "logo24bits.png",
	              projection->getViewportWidth(), projection->getViewportHeight(),
	              StelUtils::stringToWstring(VERSION), 45, 320, 121);
	
	// Init the solar system first
	ssystem = new SolarSystem();
	ssystem->init(conf, lb);
	StelApp::getInstance().getModuleMgr().registerModule(ssystem);
	
	// Observator
	observatory = new Observator(*ssystem);
	observatory->load(conf, "init_location");

	// Navigator
	navigation = new Navigator(observatory);
	navigation->init(conf, lb);
	
	// Load hipparcos stars & names
	hip_stars = new HipStarMgr();
	hip_stars->init(conf, lb);
	StelApp::getInstance().getModuleMgr().registerModule(hip_stars);
	int grid_level = hip_stars->getMaxGridLevel();
	geodesic_grid = new GeodesicGrid(grid_level);
	geodesic_search_result = new GeodesicSearchResult(*geodesic_grid);
	hip_stars->setGrid();
	
	// Init nebulas
	nebulas = new NebulaMgr();
	nebulas->init(conf, lb);
	StelApp::getInstance().getModuleMgr().registerModule(nebulas);
	
	// Init milky way
	milky_way = new MilkyWay();
	milky_way->init(conf, lb);
	StelApp::getInstance().getModuleMgr().registerModule(milky_way);
	
	// Telescope manager
	telescope_mgr = new TelescopeMgr();
	telescope_mgr->init(conf);
	setFlagTelescopes(conf.get_boolean("astro:flag_telescopes"));
	setFlagTelescopeName(conf.get_boolean("astro:flag_telescope_name"));
	
	// Constellations
	asterisms = new ConstellationMgr(hip_stars);
	asterisms->init(conf, lb);
	StelApp::getInstance().getModuleMgr().registerModule(asterisms);

	// Load the pointer textures
	StelObject::init_textures();

	// Navigation
	FlagEnableZoomKeys	= conf.get_boolean("navigation:flag_enable_zoom_keys");
	FlagEnableMoveKeys  = conf.get_boolean("navigation:flag_enable_move_keys");
	FlagManualZoom		= conf.get_boolean("navigation:flag_manual_zoom");
	auto_move_duration	= conf.get_double ("navigation","auto_move_duration",1.5);
	move_speed			= conf.get_double("navigation","move_speed",0.0004);
	zoom_speed			= conf.get_double("navigation","zoom_speed", 0.0004);

	// Landscape, atmosphere & cardinal points section
	landscape = new LandscapeMgr();
	landscape->init(conf, lb);
	StelApp::getInstance().getModuleMgr().registerModule(landscape);

	gridLines = new GridLinesMgr();
	gridLines->init(conf, lb);
	StelApp::getInstance().getModuleMgr().registerModule(gridLines);
	
	// Meteors
	meteors = new MeteorMgr(10, 60);
	setMeteorsRate(conf.get_int("astro", "meteor_rate", 10));
	
	tone_converter->set_world_adaptation_luminance(3.75f + landscape->getLuminance()*40000.f);
	
	// Load dynamic modules TODO loop here
	if (conf.find_entry("external_modules:module1"))
	{
		StelModule* m = StelApp::getInstance().getModuleMgr().loadExternalModule(conf.get_str("external_modules:module1"));
		if (m!=NULL)
		{
			m->init(conf, lb);
			StelApp::getInstance().getModuleMgr().registerModule(m);
		}
	}
}

// Update all the objects in function of the time
void StelCore::update(int delta_time)
{
	// Update the position of observation and time etc...
	observatory->update(delta_time);
	navigation->updateTime(delta_time);

	// Position of sun and all the satellites (ie planets)
	ssystem->computePositions(navigation->getJDay(),
	                          navigation->getHomePlanet()->get_heliocentric_ecliptic_pos());

	// communicate with the telescopes:
	telescope_mgr->communicate();

	// Transform matrices between coordinates systems
	navigation->updateTransformMatrices();
	// Direction of vision
	navigation->updateVisionVector(delta_time, selected_object);
	// Field of view
	projection->update_auto_zoom(delta_time);

	// update faders and Planet trails (call after nav is updated)
	ssystem->update((double)delta_time/1000);

	// Move the view direction and/or fov
	updateMove(delta_time);

	// Update info about selected object
	selected_object.update();

	// Update faders
	asterisms->update((double)delta_time/1000);
	hip_stars->update((double)delta_time/1000);
	nebulas->update((double)delta_time/1000);
	milky_way->update(delta_time);
	telescope_mgr->update(delta_time);


	// Give the updated standard projection matrices to the projector.
	// atmosphere->compute_color needs the projection matrices, so we must
	// set them before calling atmosphere->compute_color, otherwise
	// the first image will be rendered with invalid (nan)
	// inverse projection matrices.
	// On the other hand it must be called after ssystem->update
	// and updateMove in order to have the new observers position
	// and not interfere with vision vector movement.
	projection->set_modelview_matrices(	navigation->get_earth_equ_to_eye_mat(),
	                                    navigation->get_helio_to_eye_mat(),
	                                    navigation->get_local_to_eye_mat(),
	                                    navigation->get_j2000_to_eye_mat());

	gridLines->update((double)delta_time/1000);
	landscape->update((double)delta_time/1000);
	
	StelModuleMgr& mmgr = StelApp::getInstance().getModuleMgr();
	for (StelModuleMgr::Iterator iter=mmgr.begin();iter!=mmgr.end();++iter)
	{
		if ((*iter)->isExternal())
			(*iter)->update((double)delta_time/1000);
	}
}

// Execute all the drawing functions
double StelCore::draw(int delta_time)
{
	// Init openGL viewing with fov, screen size and clip planes
	projection->set_clipping_planes(0.000001 ,50);

	// Init viewport to current projector values
	projection->applyViewport();

	// Draw the milky way.
	milky_way->draw(projection, navigation, tone_converter);

	// Draw all the constellations
	asterisms->draw(projection, navigation, tone_converter);

	const Vec4i &v(projection->getViewport());
	Vec3d e0,e1,e2,e3;
	projection->unproject_j2000(v[0],v[1],e0);
	projection->unproject_j2000(v[0]+v[2],v[1]+v[3],e2);
	if (projection->needGlFrontFaceCW())
	{
		projection->unproject_j2000(v[0],v[1]+v[3],e3);
		projection->unproject_j2000(v[0]+v[2],v[1],e1);
	}
	else
	{
		projection->unproject_j2000(v[0],v[1]+v[3],e1);
		projection->unproject_j2000(v[0]+v[2],v[1],e3);
	}

	int max_search_level = hip_stars->getMaxSearchLevel(tone_converter, projection);
	// int h = nebulas->getMaxSearchLevel(tone_converter,projection);
	// if (max_search_level < h) max_search_level = h;
	geodesic_search_result->search(e0,e1,e2,e3,max_search_level);

	// Draw all external modules
	StelModuleMgr& mmgr = StelApp::getInstance().getModuleMgr();
	for (StelModuleMgr::Iterator iter=mmgr.begin();iter!=mmgr.end();++iter)
	{
		if ((*iter)->isExternal())
			(*iter)->draw(projection, navigation, tone_converter);
	}

	// Draw the nebula
	nebulas->draw(projection, navigation, tone_converter);

	// Draw the stars
	hip_stars->draw(projection, navigation, tone_converter);

	//geoDrawer->draw(projection,navigation, tone_converter);
	
	gridLines->draw(projection, navigation, tone_converter);
	
	// Draw the planets
	double squaredDistance = ssystem->draw(projection, navigation, tone_converter);

	// Draw the pointer on the currently selected object
	// TODO: this would be improved if pointer was drawn at same time as object for correct depth in scene
	// FC: Why? The pointer should be drawn over everthing else I think
	if (selected_object && object_pointer_visibility)
		selected_object.drawPointer(delta_time, projection, navigation);

	// Upade meteors
	meteors->update(delta_time);

	// TODO fix if(!landscape->getFlagAtmosphere() || sky_brightness<0.1)
	meteors->draw(projection, navigation, tone_converter);

	telescope_mgr->draw(projection,navigation);

	landscape->draw(projection,navigation,tone_converter);

	// draw images loaded by a script
	script_images->draw(navigation, projection);

	projection->draw_viewport_shape();

	return squaredDistance;
}


StelObject StelCore::searchByNameI18n(const wstring &name) const
{
	StelObject rval;
	rval = ssystem->searchByNameI18n(name);
	if (rval)
		return rval;
	rval = nebulas->searchByNameI18n(name);
	if (rval)
		return rval;
	rval = hip_stars->searchByNameI18n(name);
	if (rval)
		return rval;
	rval = asterisms->searchByNameI18n(name);
	if (rval)
		return rval;
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
	if (!obj)
		return false;
	else
		return selectObject(obj);
}


//! Find and select an object based on selection type and standard name or number
//! @return true if an object was selected

bool StelCore::selectObject(const string &type, const string &id)
{
	/*
	  std::wostringstream oss;
	  oss << id.c_str();
	  return findAndSelectI18n(oss.str());
	*/
	if(type=="hp")
	{
		unsigned int hpnum;
		std::istringstream istr(id);
		istr >> hpnum;
		selected_object = hip_stars->searchHP(hpnum);
		asterisms->setSelected(selected_object);
		ssystem->setSelected("");

	}
	else if(type=="star")
	{
		selected_object = hip_stars->search(id);
		asterisms->setSelected(selected_object);
		ssystem->setSelected("");

	}
	else if(type=="planet")
	{
		ssystem->setSelected(id);
		selected_object = ssystem->getSelected();
		asterisms->setSelected(StelObject());

	}
	else if(type=="nebula")
	{
		selected_object = nebulas->search(id);
		ssystem->setSelected("");
		asterisms->setSelected(StelObject());

	}
	else if(type=="constellation")
	{
		// Select only constellation, nothing else
		asterisms->setSelected(id);
		selected_object = NULL;
		ssystem->setSelected("");
	}
	else if(type=="constellation_star")
	{
		// For Find capability, select a star in constellation so can center view on constellation
		selected_object = asterisms->setSelectedStar(id);
		ssystem->setSelected("");
	}
	else
	{
		cerr << "Invalid selection type specified: " << type << endl;
		return 0;
	}


	if (selected_object)
	{
		if (navigation->getFlagTraking())
			navigation->setFlagLockEquPos(1);
		navigation->setFlagTraking(0);

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
	projection->unproject_earth_equ(x,projection->getViewportHeight()-y,v);
	return findAndSelect(v);
}

//! Deselect selected object if any
//! Does not deselect selected constellation
void StelCore::unSelect(void) {
	selected_object=NULL;
	ssystem->setSelected(StelObject());
}

//! Deselect all selected objects if any
//! Does deselect selected constellations
void StelCore::deselect(void) {
	unSelect();
	asterisms->deselect();
}


// Find an object in a "clever" way
StelObject StelCore::clever_find(const Vec3d& v) const
{
	StelObject sobj;
	vector<StelObject> candidates;
	vector<StelObject> temp;

	// Field of view for a 30 pixel diameter circle on screen
	float fov_around = projection->get_fov()/MY_MIN(projection->getViewportWidth(), projection->getViewportHeight()) * 30.f;

	// Collect the planets inside the range
	if (ssystem->getFlagPlanets())
	{
		temp = ssystem->searchAround(v, fov_around, navigation, projection);
		candidates.insert(candidates.begin(), temp.begin(), temp.end());
	}

	// nebulas and stars used precessed equ coords
	Vec3d p = navigation->earth_equ_to_j2000(v);

	// The nebulas inside the range
	if (nebulas->getFlagShow())
	{
		temp = nebulas->searchAround(p, fov_around, navigation, projection);
		candidates.insert(candidates.begin(), temp.begin(), temp.end());
	}

	// And the stars inside the range
	if (hip_stars->getFlagStars())
	{
		temp = hip_stars->searchAround(p, fov_around, NULL, NULL);
		candidates.insert(candidates.begin(), temp.begin(), temp.end());
	}

	if (getFlagTelescopes())
	{
		temp = telescope_mgr->search_around(p, fov_around);
		candidates.insert(candidates.begin(), temp.begin(), temp.end());
	}

	// Now select the object minimizing the function y = distance(in pixel) + magnitude
	Vec3d winpos;
	projection->project_earth_equ(v, winpos);
	float xpos = winpos[0];
	float ypos = winpos[1];

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
			if( ssystem->getFlagHints() )
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

	if (!selected_object)
		return;

	if (!navigation->getFlagTraking())
	{
		navigation->setFlagTraking(true);
		navigation->moveTo(selected_object.get_earth_equ_pos(navigation),
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
		else
		{
			float closefov = selected_object.get_close_fov(navigation);
			if (projection->get_fov()>closefov)
				projection->zoom_to(closefov, move_duration);
		}
	}
}


// Unzoom and go to the init position
void StelCore::autoZoomOut(float move_duration, bool full)
{
	if (selected_object && !full)
	{
		// If the selected object has satellites, unzoom to satellites view
		// unless specified otherwise
		float satfov = selected_object.get_satellites_fov(navigation);

		if (satfov>0.0 && projection->get_fov()<=satfov*0.9)
		{
			projection->zoom_to(satfov, move_duration);
			return;
		}

		// If the selected object is part of a Planet subsystem (other than sun),
		// unzoom to subsystem view
		satfov = selected_object.get_parent_satellites_fov(navigation);
		if (satfov>0.0 && projection->get_fov()<=satfov*0.9)
		{
			projection->zoom_to(satfov, move_duration);
			return;
		}
	}

	projection->zoom_to(projection->getInitFov(), move_duration);
	navigation->moveTo(navigation->getinitViewPos(), move_duration, true, -1);
	navigation->setFlagTraking(false);
	navigation->setFlagLockEquPos(0);
}

// Update the sky culture for all the modules
// TODO make generic
void StelCore::updateSkyCulture()
{
	LoadingBar lb(projection, LOADING_BAR_DEFAULT_FONT_SIZE, "logo24bits.png", projection->getViewportWidth(), projection->getViewportHeight(), StelUtils::stringToWstring(VERSION), 45, 320, 121);
	if (asterisms) asterisms->updateSkyCulture(lb);
	
	// as constellations have changed, clear out any selection and retest for match!
	if (selected_object && selected_object.get_type()==STEL_OBJECT_STAR)
	{
		if (asterisms) asterisms->setSelected(selected_object);
	}
	else
	{
		if (asterisms) asterisms->setSelected(StelObject());
	}
	
	if (hip_stars) hip_stars->updateSkyCulture(lb);
}


// Set the sky locale and update the sky objects names for all the modules
void StelCore::updateSkyLanguage()
{
	if( !hip_stars || !asterisms)
		return; // objects not initialized yet

	if (asterisms) asterisms->updateI18n();
	if (ssystem) ssystem->updateI18n();
	if (nebulas) nebulas->updateI18n();
	if (hip_stars) hip_stars->updateI18n();
}


// Please keep saveCurrentSettings up to date with any new color settings added here
void StelCore::setColorScheme(const string& skinFile, const string& section)
{
	InitParser conf;
	conf.load(skinFile);

	// simple default color, rather than black which doesn't show up
	string defaultColor = "0.6,0.4,0";

	// Load colors from config file
	nebulas->setNamesColor(StelUtils::str_to_vec3f(conf.get_str(section,"nebula_label_color", defaultColor)));
	nebulas->setCirclesColor(StelUtils::str_to_vec3f(conf.get_str(section,"nebula_circle_color", defaultColor)));
	hip_stars->setLabelColor(StelUtils::str_to_vec3f(conf.get_str(section,"star_label_color", defaultColor)));
	hip_stars->setCircleColor(StelUtils::str_to_vec3f(conf.get_str(section,"star_circle_color", defaultColor)));
	telescope_mgr->set_label_color(StelUtils::str_to_vec3f(conf.get_str(section,"telescope_label_color", defaultColor)));
	telescope_mgr->set_circle_color(StelUtils::str_to_vec3f(conf.get_str(section,"telescope_circle_color", defaultColor)));
	ssystem->setNamesColor(StelUtils::str_to_vec3f(conf.get_str(section,"planet_names_color", defaultColor)));
	ssystem->setOrbitsColor(StelUtils::str_to_vec3f(conf.get_str(section,"planet_orbits_color", defaultColor)));
	ssystem->setTrailsColor(StelUtils::str_to_vec3f(conf.get_str(section,"object_trails_color", defaultColor)));
	gridLines->setColorEquatorGrid(StelUtils::str_to_vec3f(conf.get_str(section,"equatorial_color", defaultColor)));
	gridLines->setColorAzimutalGrid(StelUtils::str_to_vec3f(conf.get_str(section,"azimuthal_color", defaultColor)));
	gridLines->setColorEquatorLine(StelUtils::str_to_vec3f(conf.get_str(section,"equator_color", defaultColor)));
	gridLines->setColorEclipticLine(StelUtils::str_to_vec3f(conf.get_str(section,"ecliptic_color", defaultColor)));
	gridLines->setColorMeridianLine(StelUtils::str_to_vec3f(conf.get_str(section,"meridian_color", defaultColor)));
	landscape->setColorCardinalPoints(StelUtils::str_to_vec3f(conf.get_str(section,"cardinal_color", defaultColor)));
	asterisms->setLinesColor(StelUtils::str_to_vec3f(conf.get_str(section,"const_lines_color", defaultColor)));
	asterisms->setBoundariesColor(StelUtils::str_to_vec3f(conf.get_str(section,"const_boundary_color", "0.8,0.3,0.3")));
	asterisms->setNamesColor(StelUtils::str_to_vec3f(conf.get_str(section,"const_names_color", defaultColor)));
}

//! Get a color used to display info about the currently selected object
Vec3f StelCore::getSelectedObjectInfoColor(void) const
{
	if (!selected_object)
	{
		cerr << "WARNING: StelCore::getSelectedObjectInfoColor was called while no object is currently selected!!" << endl;
		return Vec3f(1, 1, 1);
	}
	if (selected_object.get_type()==STEL_OBJECT_NEBULA)
		return nebulas->getNamesColor();
	if (selected_object.get_type()==STEL_OBJECT_PLANET)
		return ssystem->getNamesColor();
	if (selected_object.get_type()==STEL_OBJECT_STAR)
		return selected_object.get_RGB();
	return Vec3f(1, 1, 1);
}


void StelCore::setProjectionType(const string& sptype)
{
	Projector::PROJECTOR_TYPE pType = Projector::stringToType(sptype);
	if (projection->getType()==pType)
		return;
	Projector *const ptemp = Projector::create(pType,
	                         projection->getViewport(),
	                         projection->get_fov());
	ptemp->setMaskType(projection->getMaskType());
	ptemp->setFlagGravityLabels(projection->getFlagGravityLabels());
	delete projection;
	projection = ptemp;
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
	else
		deltaAz = 0;
}

void StelCore::turn_left(int s)
{
	if (s && FlagEnableMoveKeys)
	{
		deltaAz = -1;
		setFlagTracking(false);
		setFlagLockSkyPosition(false);

	}
	else
		deltaAz = 0;
}

void StelCore::turn_up(int s)
{
	if (s && FlagEnableMoveKeys)
	{
		deltaAlt = 1;
		setFlagTracking(false);
		setFlagLockSkyPosition(false);
	}
	else
		deltaAlt = 0;
}

void StelCore::turn_down(int s)
{
	if (s && FlagEnableMoveKeys)
	{
		deltaAlt = -1;
		setFlagTracking(false);
		setFlagLockSkyPosition(false);
	}
	else
		deltaAlt = 0;
}


void StelCore::zoom_in(int s)
{
	if (FlagEnableZoomKeys)
		deltaFov = -1*(s!=0);
}

void StelCore::zoom_out(int s)
{
	if (FlagEnableZoomKeys)
		deltaFov = (s!=0);
}

//! Make the first screen position correspond to the second (useful for mouse dragging)
void StelCore::dragView(int x1, int y1, int x2, int y2)
{
	Vec3d tempvec1, tempvec2;
	double az1, alt1, az2, alt2;
	if (navigation->getViewingMode()==Navigator::VIEW_HORIZON)
	{
		projection->unproject_local(x2,projection->getViewportHeight()-y2, tempvec2);
		projection->unproject_local(x1,projection->getViewportHeight()-y1, tempvec1);
	}
	else
	{
		projection->unproject_earth_equ(x2,projection->getViewportHeight()-y2, tempvec2);
		projection->unproject_earth_equ(x1,projection->getViewportHeight()-y1, tempvec1);
	}
	StelUtils::rect_to_sphe(&az1, &alt1, tempvec1);
	StelUtils::rect_to_sphe(&az2, &alt2, tempvec2);
	navigation->updateMove(az2-az1, alt1-alt2);
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
		if (deltaAz<-0.2)
			deltaAz = -0.2;
	}
	else
	{
		if (deltaAz>0)
		{
			deltaAz = (depl/30);
			if (deltaAz>0.2)
				deltaAz = 0.2;
		}
	}
	if (deltaAlt<0)
	{
		deltaAlt = -depl/30;
		if (deltaAlt<-0.2)
			deltaAlt = -0.2;
	}
	else
	{
		if (deltaAlt>0)
		{
			deltaAlt = depl/30;
			if (deltaAlt>0.2)
				deltaAlt = 0.2;
		}
	}

	if (deltaFov<0)
	{
		deltaFov = -deplzoom*5;
		if (deltaFov<-0.15*projection->get_fov())
			deltaFov = -0.15*projection->get_fov();
	}
	else
	{
		if (deltaFov>0)
		{
			deltaFov = deplzoom*5;
			if (deltaFov>20)
				deltaFov = 20;
		}
	}

	//	projection->change_fov(deltaFov);
	//	navigation->update_move(deltaAz, deltaAlt);

	if(deltaFov != 0 )
	{
		projection->change_fov(deltaFov);
		std::ostringstream oss;
		oss << "zoom delta_fov " << deltaFov;
		if (!recordActionCallback.empty())
			recordActionCallback(oss.str());
	}

	if(deltaAz != 0 || deltaAlt != 0)
	{
		navigation->updateMove(deltaAz, deltaAlt);
		std::ostringstream oss;
		oss << "look delta_az " << deltaAz << " delta_alt " << deltaAlt;
		if (!recordActionCallback.empty())
			recordActionCallback(oss.str());
	}
	else
	{
		// must perform call anyway, but don't record!
		navigation->updateMove(deltaAz, deltaAlt);
	}
}


bool StelCore::setHomePlanet(string planet)
{
	// reset planet trails due to changed perspective
	ssystem->startTrails( ssystem->getFlagTrails() );

	return observatory->setHomePlanet(planet);
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
			if (getFlagTracking())
				navigation->setFlagLockEquPos(1);
			setFlagTracking(false);

			if (selected_object.get_type()==STEL_OBJECT_STAR)
			{
				asterisms->setSelected(selected_object);
				// potentially record this action
				if (!recordActionCallback.empty())
					recordActionCallback("select " + selected_object.getEnglishName());
			}
			else
			{
				asterisms->setSelected(StelObject());
			}

			if (selected_object.get_type()==STEL_OBJECT_PLANET)
			{
				ssystem->setSelected(selected_object);
				// potentially record this action
				if (!recordActionCallback.empty())
					recordActionCallback("select planet " + selected_object.getEnglishName());
			}

			if (selected_object.get_type()==STEL_OBJECT_NEBULA)
			{
				// potentially record this action
				if (!recordActionCallback.empty())
					recordActionCallback("select nebula \"" + selected_object.getEnglishName() + "\"");
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


void StelCore::setFlagTracking(bool b)
{

	if(!b || !selected_object)
	{
		navigation->setFlagTraking(0);
	}
	else
	{
		navigation->moveTo(selected_object.get_earth_equ_pos(navigation),
		                    getAutomoveDuration());
		navigation->setFlagTraking(1);
	}

}

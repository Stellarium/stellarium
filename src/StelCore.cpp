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
#include "StelApp.hpp"
#include "StelUtils.hpp"
#include "GeodesicGrid.hpp"
#include "hip_star_mgr.h"
#include "telescope_mgr.h"
#include "Constellation.hpp"
#include "ConstellationMgr.hpp"
#include "solarsystem.h"
#include "NebulaMgr.hpp"
#include "GeodesicGridDrawer.h"
#include "LandscapeMgr.hpp"
#include "GridLinesMgr.hpp"
#include "MilkyWay.hpp"
#include "MovementMgr.hpp"
#include "image_mgr.h"
#include "meteor_mgr.h"

StelCore::StelCore() : projection(NULL),  hip_stars(NULL), asterisms(NULL),
		nebulas(NULL), ssystem(NULL), milky_way(NULL), telescope_mgr(NULL)
{
	tone_converter = new ToneReproducer();
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
	delete movementMgr;
}

// Load core data and initialize with default values
void StelCore::initProj(const InitParser& conf)
{
	// Projector
	projection = Projector::create(Projector::PERSPECTIVE_PROJECTOR, Vec4i(0,0,800,600), 60);
	string tmpstr = conf.get_str("projection:type");
	setProjectionType(tmpstr);
	projection->init(conf);
}

// Load core data and initialize with default values
void StelCore::init(const InitParser& conf, LoadingBar& lb)
{	
	script_images = new ImageMgr();
	script_images->init(conf, lb);
	StelApp::getInstance().getModuleMgr().registerModule(script_images);
	
	// Init the solar system first
	ssystem = new SolarSystem();
	ssystem->init(conf, lb);
	StelApp::getInstance().getModuleMgr().registerModule(ssystem);
	
	// Observer
	observatory = new Observer(*ssystem);
	observatory->load(conf, "init_location");

	// Navigator
	navigation = new Navigator(observatory);
	navigation->init(conf, lb);
	
	movementMgr = new MovementMgr(this);
	movementMgr->init(conf, lb);
	StelApp::getInstance().getModuleMgr().registerModule(movementMgr);
	
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
	telescope_mgr->init(conf, lb);
	StelApp::getInstance().getModuleMgr().registerModule(telescope_mgr);
	
	// Constellations
	asterisms = new ConstellationMgr(hip_stars);
	asterisms->init(conf, lb);
	StelApp::getInstance().getModuleMgr().registerModule(asterisms);

	// Load the pointer textures
	StelObject::init_textures();
	
	// Landscape, atmosphere & cardinal points section
	landscape = new LandscapeMgr();
	landscape->init(conf, lb);
	StelApp::getInstance().getModuleMgr().registerModule(landscape);

	gridLines = new GridLinesMgr();
	gridLines->init(conf, lb);
	StelApp::getInstance().getModuleMgr().registerModule(gridLines);
	
	// Meteors
	meteors = new MeteorMgr(10, 60);
	meteors->init(conf, lb);
	StelApp::getInstance().getModuleMgr().registerModule(meteors);
	
	tone_converter->set_world_adaptation_luminance(3.75f + landscape->getLuminance()*40000.f);
}

// Update all the objects in function of the time
void StelCore::update(int delta_time)
{
	// Update the position of observation and time etc...
	observatory->update(delta_time);
	navigation->updateTime(delta_time);

	// Position of sun and all the satellites (ie planets)
	ssystem->computePositions(navigation->getJDay(), navigation->getHomePlanet()->get_heliocentric_ecliptic_pos());

	// Transform matrices between coordinates systems
	navigation->updateTransformMatrices();
	
	// Update direction of vision/Zoom level
	movementMgr->update((double)delta_time/1000);	
	
	// Give the updated standard projection matrices to the projector.
	// atmosphere->compute_color needs the projection matrices, so we must
	// set them before calling atmosphere->compute_color, otherwise
	// the first image will be rendered with invalid (nan)
	// inverse projection matrices.
	// On the other hand it must be called after ssystem->update
	// and panView in order to have the new observers position
	// and not interfere with vision vector movement.
	projection->set_modelview_matrices(	navigation->get_earth_equ_to_eye_mat(),
	                                    navigation->get_helio_to_eye_mat(),
	                                    navigation->get_local_to_eye_mat(),
	                                    navigation->get_j2000_to_eye_mat());	
	
	// communicate with the telescopes:
	telescope_mgr->communicate();

	// update faders and Planet trails (call after nav is updated)
	ssystem->update((double)delta_time/1000);

	// Update faders
	asterisms->update((double)delta_time/1000);
	hip_stars->update((double)delta_time/1000);
	nebulas->update((double)delta_time/1000);
	milky_way->update(delta_time);
	telescope_mgr->update(delta_time);

	gridLines->update((double)delta_time/1000);
	landscape->update((double)delta_time/1000);
	
	// Upade meteors
	meteors->update(delta_time);
	
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


	// Draw the milky way.
	milky_way->draw(projection, navigation, tone_converter);

	// Draw all the constellations
	asterisms->draw(projection, navigation, tone_converter);

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

	// TODO fix if(!landscape->getFlagAtmosphere() || sky_brightness<0.1)
	meteors->draw(projection, navigation, tone_converter);

	telescope_mgr->draw(projection, navigation, tone_converter);

	landscape->draw(projection,navigation,tone_converter);

	// draw images loaded by a script
	script_images->draw(projection,navigation,tone_converter);

	projection->draw_viewport_shape();

	return squaredDistance;
}

void StelCore::setProjectionType(const string& sptype)
{
	Projector::PROJECTOR_TYPE pType = Projector::stringToType(sptype);
	if (projection->getType()==pType)
		return;
	Projector *const ptemp = Projector::create(pType,
	                         projection->getViewport(),
	                         projection->getFov());
	ptemp->setMaskType(projection->getMaskType());
	ptemp->setFlagGravityLabels(projection->getFlagGravityLabels());
	delete projection;
	projection = ptemp;
	glFrontFace(projection->needGlFrontFaceCW()?GL_CW:GL_CCW);
}


bool StelCore::setHomePlanet(string planet)
{
	// reset planet trails due to changed perspective
	ssystem->startTrails( ssystem->getFlagTrails() );

	return observatory->setHomePlanet(planet);
}

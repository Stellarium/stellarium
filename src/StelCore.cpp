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

#include "StelCore.hpp"
#include "StelApp.hpp"
#include "StelUtils.hpp"
#include "GeodesicGrid.hpp"
#include "hip_star_mgr.h"
#include "solarsystem.h"
#include "MovementMgr.hpp"


StelCore::StelCore() : projection(NULL)
{
	tone_converter = new ToneReproducer();
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
	delete navigation;
	delete projection;
	delete observatory;
	observatory = NULL;
	delete tone_converter;
	
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
	// Observer
	SolarSystem* solsystem = (SolarSystem*)StelApp::getInstance().getModuleMgr().getModule("ssystem");
	observatory = new Observer(*solsystem);
	observatory->load(conf, "init_location");

	// Navigator
	navigation = new Navigator(observatory);
	navigation->init(conf, lb);
	
	movementMgr = new MovementMgr(this);
	movementMgr->init(conf, lb);
	StelApp::getInstance().getModuleMgr().registerModule(movementMgr);	
	
	HipStarMgr* hip_stars = (HipStarMgr*)StelApp::getInstance().getModuleMgr().getModule("stars");
	int grid_level = hip_stars->getMaxGridLevel();
	geodesic_grid = new GeodesicGrid(grid_level);
	geodesic_search_result = new GeodesicSearchResult(*geodesic_grid);
	hip_stars->setGrid();
	//tone_converter->set_world_adaptation_luminance(3.75f + landscape->getLuminance()*40000.f);
}

// Update all the objects in function of the time
void StelCore::update(int delta_time)
{
	// Update the position of observation and time etc...
	observatory->update(delta_time);
	navigation->updateTime(delta_time);

	// Position of sun and all the satellites (ie planets)
	SolarSystem* solsystem = (SolarSystem*)StelApp::getInstance().getModuleMgr().getModule("ssystem");
	solsystem->computePositions(navigation->getJDay(), navigation->getHomePlanet()->get_heliocentric_ecliptic_pos());

	// Transform matrices between coordinates systems
	navigation->updateTransformMatrices();
	
	// Update direction of vision/Zoom level
	movementMgr->updateMotion((double)delta_time/1000);	
	
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
}

// Execute all the pre-drawing functions
void StelCore::preDraw(int delta_time)
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

	// This still needs a fix
	HipStarMgr* hip_stars = (HipStarMgr*)StelApp::getInstance().getModuleMgr().getModule("stars");
	int max_search_level = hip_stars->getMaxSearchLevel(tone_converter, projection);
	geodesic_search_result->search(e0,e1,e2,e3,max_search_level);
}


void StelCore::postDraw()
{
	projection->draw_viewport_shape();
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
	SolarSystem* ssystem = (SolarSystem*)StelApp::getInstance().getModuleMgr().getModule("ssystem");
	// reset planet trails due to changed perspective
	ssystem->startTrails( ssystem->getFlagTrails() );

	return observatory->setHomePlanet(planet);
}

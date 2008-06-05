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

#include "StelCore.hpp"
#include "Navigator.hpp"
#include "Observer.hpp"
#include "Projector.hpp"
#include "ToneReproducer.hpp"
#include "SkyDrawer.hpp"
#include "StelApp.hpp"
#include "StelUtils.hpp"
#include "GeodesicGrid.hpp"
#include "StarMgr.hpp"
#include "SolarSystem.hpp"
#include "MovementMgr.hpp"
#include "StelModuleMgr.hpp"
#include "Planet.hpp"

#include <QSettings>

/*************************************************************************
 Constructor
*************************************************************************/
StelCore::StelCore()
{
	tone_converter = new ToneReproducer();
	projection = new Projector(Vector4<GLint>(0,0,800,600), 60);
	projection->init();
	skyDrawer = new SkyDrawer(this);
}


/*************************************************************************
 Destructor
*************************************************************************/
StelCore::~StelCore()
{
	delete navigation; navigation=NULL;
	delete projection; projection=NULL;
	delete observatory; observatory=NULL;
	delete tone_converter; tone_converter=NULL;
	delete geodesic_grid; geodesic_grid=NULL;
	delete skyDrawer; skyDrawer=NULL;
}

/*************************************************************************
 Load core data and initialize with default values
*************************************************************************/
void StelCore::init()
{	
	// Observer
	SolarSystem* solsystem = (SolarSystem*)StelApp::getInstance().getModuleMgr().getModule("SolarSystem");
	observatory = new Observer(*solsystem);
	observatory->load(StelApp::getInstance().getSettings(), "init_location");

	// Navigator
	navigation = new Navigator(observatory);
	navigation->init();
	
	movementMgr = new MovementMgr(this);
	movementMgr->init();
	StelApp::getInstance().getModuleMgr().registerModule(movementMgr);	
	
	StarMgr* hip_stars = (StarMgr*)StelApp::getInstance().getModuleMgr().getModule("StarMgr");
	int grid_level = hip_stars->getMaxGridLevel();
	geodesic_grid = new GeodesicGrid(grid_level);
	hip_stars->setGrid(geodesic_grid);
	
	skyDrawer->init();
}


/*************************************************************************
 Update all the objects in function of the time
*************************************************************************/
void StelCore::update(double deltaTime)
{
	// Update the position of observation and time etc...
	observatory->update((int)(deltaTime*1000));
	navigation->updateTime(deltaTime);

	// Position of sun and all the satellites (ie planets)
	SolarSystem* solsystem = (SolarSystem*)StelApp::getInstance().getModuleMgr().getModule("SolarSystem");
	solsystem->computePositions(navigation->getJDay(), navigation->getHomePlanet()->get_heliocentric_ecliptic_pos());
	//qDebug() << "get_heliocentric_ecliptic_pos()[0]=" << navigation->getHomePlanet()->get_heliocentric_ecliptic_pos()[0];

	// Transform matrices between coordinates systems
	navigation->updateTransformMatrices();
	
	// Update direction of vision/Zoom level
	movementMgr->updateMotion(deltaTime);	
	
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
	
	skyDrawer->update(deltaTime);
}


/*************************************************************************
 Execute all the pre-drawing functions
*************************************************************************/
void StelCore::preDraw()
{
	// Init openGL viewing with fov, screen size and clip planes
	projection->set_clipping_planes(0.000001 ,50);

	// Init viewport to current projector values
	projection->applyViewport();

	projection->setCurrentFrame(Projector::FRAME_J2000);
}


/*************************************************************************
 Update core state after drawing modules
*************************************************************************/
void StelCore::postDraw()
{
	projection->draw_viewport_shape();
}

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
	toneConverter = new ToneReproducer();
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
	delete toneConverter; toneConverter=NULL;
	delete geodesicGrid; geodesicGrid=NULL;
	delete skyDrawer; skyDrawer=NULL;
}

// void printLog(GLuint obj)
// {
// 	int infologLength = 0;
// 	int maxLength;
// 	
// 	if(glIsShader(obj))
// 		glGetShaderiv(obj,GL_INFO_LOG_LENGTH,&maxLength);
// 	else
// 		glGetProgramiv(obj,GL_INFO_LOG_LENGTH,&maxLength);
// 			
// 	char infoLog[maxLength];
//  
// 	if (glIsShader(obj))
// 		glGetShaderInfoLog(obj, maxLength, &infologLength, infoLog);
// 	else
// 		glGetProgramInfoLog(obj, maxLength, &infologLength, infoLog);
//  
// 	if (infologLength > 0)
// 		printf("%s\n",infoLog);
// }

/*************************************************************************
 Load core data and initialize with default values
*************************************************************************/
void StelCore::init()
{	
	// Observer
	SolarSystem* solsystem = (SolarSystem*)StelApp::getInstance().getModuleMgr().getModule("SolarSystem");
	observatory = new Observer(*solsystem);
	observatory->init();

	// Navigator
	navigation = new Navigator(observatory);
	navigation->init();
	
	movementMgr = new MovementMgr(this);
	movementMgr->init();
	StelApp::getInstance().getModuleMgr().registerModule(movementMgr);	
	
	StarMgr* hip_stars = (StarMgr*)StelApp::getInstance().getModuleMgr().getModule("StarMgr");
	int grid_level = hip_stars->getMaxGridLevel();
	geodesicGrid = new GeodesicGrid(grid_level);
	hip_stars->setGrid(geodesicGrid);
	
	skyDrawer->init();
	
	// Debug
	// Invert colors fragment shader
// 	const QByteArray a("void main(void) {float gray = dot(gl_Color.rgb, vec3(0.299, 0.587, 0.114)); gl_FragColor = vec4(gray * vec3(1.2, 1.0, 0.8), 1.0);}");
// 	GLuint fs;	// Fragment Shader
// 	GLuint sp;	// Shader Program
// 	fs = glCreateShader(GL_FRAGMENT_SHADER);
// 	const char* str = a.constData();
// 	glShaderSource(fs, 1, &str, NULL);
// 	glCompileShader(fs);
// 	printLog(fs);
// 
// 	sp = glCreateProgram();
// 	glAttachShader(sp, fs);
// 	glLinkProgram(sp);
// 	printLog(sp);
// 	glUseProgram(sp);
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
	solsystem->computePositions(navigation->getJDay(), navigation->getHomePlanet()->getHeliocentricEclipticPos());

	// Transform matrices between coordinates systems
	navigation->updateTransformMatrices();
	
	// Update direction of vision/Zoom level
	movementMgr->updateMotion(deltaTime);	
	
	// Give the updated standard projection matrices to the projector.
	// atmosphere->computeColor needs the projection matrices, so we must
	// set them before calling atmosphere->computeColor, otherwise
	// the first image will be rendered with invalid (nan)
	// inverse projection matrices.
	// On the other hand it must be called after ssystem->update
	// and panView in order to have the new observers position
	// and not interfere with vision vector movement.
	projection->setModelviewMatrices(	navigation->getEarthEquToEyeMat(),
	                                    navigation->getHelioToEyeMat(),
	                                    navigation->getLocalToEyeMat(),
	                                    navigation->getJ2000ToEyeMat());
	
	skyDrawer->update(deltaTime);
}


/*************************************************************************
 Execute all the pre-drawing functions
*************************************************************************/
void StelCore::preDraw()
{
	// Init openGL viewing with fov, screen size and clip planes
	projection->setClippingPlanes(0.000001 ,50);

	// Init GL viewport to current projector values
	glViewport(projection->getViewportPosX(), projection->getViewportPosY(), projection->getViewportWidth(), projection->getViewportHeight());

	projection->setCurrentFrame(Projector::FrameJ2000);
	
	// Clear areas not redrawn by main viewport (i.e. fisheye square viewport)
	glClear(GL_COLOR_BUFFER_BIT);
	
	skyDrawer->preDraw();
}


/*************************************************************************
 Update core state after drawing modules
*************************************************************************/
void StelCore::postDraw()
{
	projection->drawViewportShape();
	
// 	// Inverted mode
// 	glPixelTransferi(GL_RED_BIAS, 1);
// 	glPixelTransferi(GL_GREEN_BIAS, 1);
// 	glPixelTransferi(GL_BLUE_BIAS, 1);
// 	glPixelTransferi(GL_RED_SCALE, -1);
// 	glPixelTransferi(GL_GREEN_SCALE, -1);
// 	glPixelTransferi(GL_BLUE_SCALE, -1);
	
// 	// Night red mode
//  glPixelTransferf(GL_GREEN_SCALE, 0.1f);
//  glPixelTransferf(GL_BLUE_SCALE, 0.1f);
// 	
// 	glDisable(GL_TEXTURE_2D);
// 	glShadeModel(GL_FLAT);
// 	glDisable(GL_DEPTH_TEST);
// 	glDisable(GL_CULL_FACE);
// 	glDisable(GL_LIGHTING);
// 	glDisable(GL_MULTISAMPLE);
// 	glDisable(GL_DITHER);
// 	glDisable(GL_ALPHA_TEST);
// 	glRasterPos2i(0,0);
// 	
// 	glReadBuffer(GL_BACK);
// 	glDrawBuffer(GL_BACK);
// 	glCopyPixels(1, 1, 200, 200, GL_COLOR);
}

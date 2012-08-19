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
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */

#include "MilkyWay.hpp"
#include "StelFader.hpp"
#include "renderer/StelGeometryBuilder.hpp"
#include "renderer/StelRenderer.hpp"
#include "renderer/StelTextureNew.hpp"
#include "StelUtils.hpp"

#include "StelProjector.hpp"
#include "StelToneReproducer.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelSkyDrawer.hpp"

#include <QDebug>
#include <QSettings>

// Class which manages the displaying of the Milky Way
MilkyWay::MilkyWay() 
	: tex(NULL)
	, color(1.f, 1.f, 1.f)
	, skySphere(NULL)
{
	setObjectName("MilkyWay");
	fader = new LinearFader();
}

MilkyWay::~MilkyWay()
{
	delete fader;
	fader = NULL;
	
	if(NULL != tex)
	{
		delete tex;
		tex = NULL;
	}
	if(NULL != skySphere)
	{
		delete skySphere;
		skySphere = NULL;
	}
}

void MilkyWay::init()
{
	QSettings* conf = StelApp::getInstance().getSettings();
	Q_ASSERT(conf);

	setFlagShow(conf->value("astro/flag_milky_way").toBool());
	setIntensity(conf->value("astro/milky_way_intensity",1.f).toFloat());

	const SphereParams params = SphereParams(1.0f).resolution(20, 20).orientInside();
	skySphere = StelGeometryBuilder().buildSphereUnlit(params);
}


void MilkyWay::update(double deltaTime) {fader->update((int)(deltaTime*1000));}

void MilkyWay::setFlagShow(bool b){*fader = b;}
bool MilkyWay::getFlagShow() const {return *fader;}

void MilkyWay::draw(StelCore* core, class StelRenderer* renderer)
{
	StelProjector::ModelViewTranformP transfo = core->getJ2000ModelViewTransform();
	transfo->combine(Mat4d::xrotation(M_PI/180.*23.)*
	                 Mat4d::yrotation(M_PI/180.*120.)*
	                 Mat4d::zrotation(M_PI/180.*7.));

	if(NULL == tex)
	{
		tex = renderer->createTexture("textures/milkyway.png");
	}

	const StelProjectorP prj = core->getProjection(transfo);
	StelToneReproducer* eye = core->getToneReproducer();

	Q_ASSERT_X(NULL != tex, Q_FUNC_INFO, "A texture must be loaded before calling this");

	// This color corresponds to the night blue scotopic color = 0.25, 0.25 in xyY mode.
	// since milky way is always seen white RGB value in the texture (1.0,1.0,1.0)
	Vec4f c = StelApp::getInstance().getVisionModeNight() 
	        ? Vec4f(0.34165f, 0.0f, 0.0f, 1.0f)
	        : Vec4f(0.34165f, 0.429666f, 0.63586f, 1.0f);

	const float lum = core->getSkyDrawer()->surfacebrightnessToLuminance(13.5f);

	// Get the luminance scaled between 0 and 1
	float aLum = eye->adaptLuminanceScaled(lum*fader->getInterstate());

	// Bound a maximum luminance
	aLum = qMin(0.38f, aLum * 2.0f);

	// intensity of 1.0 is "proper", but allow boost for dim screens
	c *= aLum * intensity;

	c[0] = std::max(c[0], 0.0f);
	c[1] = std::max(c[1], 0.0f);
	c[2] = std::max(c[2], 0.0f);
	c[3] = 1.0f;

	renderer->setGlobalColor(c[0], c[1], c[2]);
	renderer->setCulledFaces(CullFace_Back);
	renderer->setBlendMode(BlendMode_None);
	tex->bind();

	skySphere->draw(renderer, prj);
	renderer->setCulledFaces(CullFace_None);
}

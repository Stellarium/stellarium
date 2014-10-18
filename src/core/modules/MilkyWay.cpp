/*
 * Stellarium
 * Copyright (C) 2002 Fabien Chereau
 * Copyright (C) 2014 Georg Zotti (extinction parts)
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
#include "StelTexture.hpp"
#include "StelUtils.hpp"
#include "StelFileMgr.hpp"

#include "StelProjector.hpp"
#include "StelToneReproducer.hpp"
#include "StelApp.hpp"
#include "StelTextureMgr.hpp"
#include "StelCore.hpp"
#include "StelSkyDrawer.hpp"
#include "StelPainter.hpp"

#include <QDebug>
#include <QSettings>

// Class which manages the displaying of the Milky Way
MilkyWay::MilkyWay()
	: color(1.f, 1.f, 1.f)
	, intensity(1.)
	, vertexArray()
{
	setObjectName("MilkyWay");
	fader = new LinearFader();
}

MilkyWay::~MilkyWay()
{
	delete fader;
	fader = NULL;
	
	delete vertexArray;
	vertexArray = NULL;
}

void MilkyWay::init()
{
	QSettings* conf = StelApp::getInstance().getSettings();
	Q_ASSERT(conf);

	tex = StelApp::getInstance().getTextureManager().createTexture(StelFileMgr::getInstallationDir()+"/textures/milkyway.png");
	setFlagShow(conf->value("astro/flag_milky_way").toBool());
	setIntensity(conf->value("astro/milky_way_intensity",1.f).toFloat());
	// GZ: I cut the original milkyway.png 512x512 to 512x256. Center line was shifted southwards by 32 pixels to keep Magellanic Clouds visible.
	// 256px were 90 degrees. 128:45, 64:22.5, 32:11.25. So this spherical ring goes from -45-11.25 to 45-11.25.

	vertexArray = new StelVertexArray(StelPainter::computeSphereNoLight(1.f,1.f,45,15,1, true, (90.0f-33.75f)*M_PI/180.0f, (90.0f+56.25f)*M_PI/180.0f)); // GZ orig: slices=stacks=20. // GZ:
	vertexArray->colors.resize(vertexArray->vertex.length());
	vertexArray->colors.fill(Vec3f(1.0, 0.3, 0.9));

	// GZ It appears cleaner to properly transform the vertex coordinates already here.
	// The texture had to be rotated/flipped and shifted compared to 0.13.0
	StelCore* core=StelApp::getInstance().getCore();
	for (int i=0; i<vertexArray->vertex.size(); ++i)
	{
		Vec3d tmp=vertexArray->vertex.at(i);
		vertexArray->vertex.replace(i, core->galacticToJ2000(tmp));
	}
}


void MilkyWay::update(double deltaTime)
{
	fader->update((int)(deltaTime*1000));
}

void MilkyWay::setFlagShow(bool b){*fader = b;}
bool MilkyWay::getFlagShow() const {return *fader;}

void MilkyWay::draw(StelCore* core)
{
	if (!getFlagShow())
		return;

	StelProjector::ModelViewTranformP transfo = core->getJ2000ModelViewTransform();
	// GZ: No idea where this came from. Empirical correction? Now the vertices have been replaced already in init().
	// (the "official" angles are not integers...)
//	transfo->combine(Mat4d::xrotation(M_PI/180.*23.)*
//					 Mat4d::yrotation(M_PI/180.*120.)*
//					 Mat4d::zrotation(M_PI/180.*7.));

	const StelProjectorP prj = core->getProjection(transfo); // GZ: Maybe this can now be simplified?
	StelToneReproducer* eye = core->getToneReproducer();

	Q_ASSERT(tex);	// A texture must be loaded before calling this

	// This RGB color corresponds to the night blue scotopic color = 0.25, 0.25 in xyY mode.
	// since milky way is always seen white RGB value in the texture (1.0,1.0,1.0)
	Vec3f c = Vec3f(0.34165f, 0.429666f, 0.63586f);

	float lum = core->getSkyDrawer()->surfacebrightnessToLuminance(13.5f);

	// Get the luminance scaled between 0 and 1
	float aLum =eye->adaptLuminanceScaled(lum*fader->getInterstate());

	// Bound a maximum luminance
	aLum = qMin(0.38f, aLum*2.f);

	// intensity of 1.0 is "proper", but allow boost for dim screens
	c*=aLum*intensity;

	if (c[0]<0) c[0]=0;
	if (c[1]<0) c[1]=0;
	if (c[2]<0) c[2]=0;

	const bool withExtinction=(core->getSkyDrawer()->getFlagHasAtmosphere() && core->getSkyDrawer()->getExtinction().getExtinctionCoefficient()>=0.01f);

	if (withExtinction)
	{
		// We must process the vertices to find geometric altitudes in order to compute vertex colors.
		Extinction extinction=core->getSkyDrawer()->getExtinction();
		vertexArray->colors.clear();

		for (int i=0; i<vertexArray->vertex.size(); ++i)
		{
			Vec3d vertAltAz=core->j2000ToAltAz(vertexArray->vertex.at(i), StelCore::RefractionOn);
			Q_ASSERT(vertAltAz.lengthSquared()-1.0 < 0.001f);

			float oneMag=0.0f;
			extinction.forward(vertAltAz, &oneMag);
			float extinctionFactor=std::pow(0.4f , oneMag); // drop of one magnitude: factor 2.5 or 40%
			Vec3f thisColor=Vec3f(c[0]*extinctionFactor, c[1]*extinctionFactor, c[2]*extinctionFactor);
			vertexArray->colors.append(thisColor);
		}
	}
	else
		vertexArray->colors.fill(Vec3f(c[0], c[1], c[2]));


	StelPainter sPainter(prj);
	glEnable(GL_CULL_FACE);
	sPainter.enableTexture2d(true);
	glDisable(GL_BLEND);
	tex->bind();
	sPainter.drawStelVertexArray(*vertexArray);
	glDisable(GL_CULL_FACE);
}

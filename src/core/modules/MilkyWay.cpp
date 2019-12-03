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
#include "StelTranslator.hpp"
#include "StelModuleMgr.hpp"
#include "LandscapeMgr.hpp"
#include "StelMovementMgr.hpp"

#include <QDebug>
#include <QSettings>

// Class which manages the displaying of the Milky Way
MilkyWay::MilkyWay()
	: color(1.f, 1.f, 1.f)
	, intensity(1.)
	, intensityFovScale(1.0)
	, intensityMinFov(0.25) // when zooming in further, MilkyWay is no longer visible.
	, intensityMaxFov(2.5) // when zooming out further, MilkyWay is fully visible (when enabled).
	, vertexArray()
{
	setObjectName("MilkyWay");
	fader = new LinearFader();
}

MilkyWay::~MilkyWay()
{
	delete fader;
	fader = Q_NULLPTR;
	
	delete vertexArray;
	vertexArray = Q_NULLPTR;
}

void MilkyWay::init()
{
	QSettings* conf = StelApp::getInstance().getSettings();
	Q_ASSERT(conf);

	tex = StelApp::getInstance().getTextureManager().createTexture(StelFileMgr::getInstallationDir()+"/textures/milkyway.png");
	setFlagShow(conf->value("astro/flag_milky_way").toBool());
	setIntensity(conf->value("astro/milky_way_intensity",1.).toDouble());
	setSaturation(conf->value("astro/milky_way_saturation", 1.).toDouble());

	// A new texture was provided by Fabien. Better resolution, but in equatorial coordinates. I had to enhance it a bit, and shift it by 90 degrees.
	vertexArray = new StelVertexArray(StelPainter::computeSphereNoLight(1.,1.,45,15,1, true)); // GZ orig: slices=stacks=20.
	vertexArray->colors.resize(vertexArray->vertex.length());
	vertexArray->colors.fill(Vec3f(1.0f, 0.3f, 0.9f));

	QString displayGroup = N_("Display Options");
	addAction("actionShow_MilkyWay", displayGroup, N_("Milky Way"), "flagMilkyWayDisplayed", "M");
}


void MilkyWay::update(double deltaTime)
{
	fader->update(static_cast<int>(deltaTime*1000));
	//calculate FOV fade value, linear fade between intensityMaxFov and intensityMinFov
	double fov = StelApp::getInstance().getCore()->getMovementMgr()->getCurrentFov();
	intensityFovScale = qBound(0.0,(fov - intensityMinFov) / (intensityMaxFov - intensityMinFov),1.0);
}

/*************************************************************************
 Reimplementation of the getCallOrder method
*************************************************************************/
double MilkyWay::getCallOrder(StelModuleActionName actionName) const
{
	if (actionName==StelModule::ActionDraw)
		return 1;
	return 0;
}

void MilkyWay::setFlagShow(bool b)
{
	if (*fader != b)
	{
		*fader = b;
		emit milkyWayDisplayedChanged(b);
	}
}

bool MilkyWay::getFlagShow() const {return *fader;}

void MilkyWay::draw(StelCore* core)
{
	if (!fader->getInterstate())
		return;

	StelProjector::ModelViewTranformP transfo = core->getJ2000ModelViewTransform();

	const StelProjectorP prj = core->getProjection(transfo); // GZ: Maybe this can now be simplified?
	StelToneReproducer* eye = core->getToneReproducer();
	StelSkyDrawer *drawer=core->getSkyDrawer();

	Q_ASSERT(tex);	// A texture must be loaded before calling this

	// This RGB color corresponds to the night blue scotopic color = 0.25, 0.25 in xyY mode.
	// since milky way is always seen white RGB value in the texture (1.0,1.0,1.0)
	// Vec3f c = Vec3f(0.34165f, 0.429666f, 0.63586f);
	// This is the same color, just brighter to have Blue=1.
	//Vec3f c = Vec3f(0.53730381f, .675724216f, 1.0f);
	// The new texture (V0.13.1) is quite blue to start with. It is better to apply white color for it.
	//Vec3f c = Vec3f(1.0f, 1.0f, 1.0f);
	// Still better: Re-activate the configurable color!
	Vec3f c = color;

	// We must also adjust milky way to light pollution.
	// Is there any way to calibrate this?
	// We compute a float 1..9 from Bortle index and atmosphere display value (allows smooth fade when switching)
	float atmFadeIntensity = GETSTELMODULE(LandscapeMgr)->getAtmosphereFadeIntensity();
	int bortle=drawer->getBortleScaleIndex();
	float bortleIntensity = 1.f+ (bortle-1)*atmFadeIntensity; // Bortle index moderated by atmosphere fader.
	//aLum*=(11.0f-bortle)*0.1f;

	float lum = drawer->surfaceBrightnessToLuminance(12.f+0.15f*bortleIntensity); // was 11.5; Source? How to calibrate the new texture?

	// Get the luminance scaled between 0 and 1
	float aLum =eye->adaptLuminanceScaled(lum*fader->getInterstate());

	// Bound a maximum luminance. GZ: Is there any reference/reason, or just trial and error?
	aLum = qMin(0.38f, aLum*2.f);


	// intensity of 1.0 is "proper", but allow boost for dim screens
	c*=aLum*static_cast<float>(intensity*intensityFovScale);


	// TODO: Find an even better balance with sky brightness, MW should be hard to see during Full Moon and at least somewhat reduced in smaller phases.
	// adapt brightness by atmospheric brightness. This block developed for ZodiacalLight, hopefully similarly applicable...
	const float atmLum = GETSTELMODULE(LandscapeMgr)->getAtmosphereAverageLuminance();
	// 10cd/m^2 at sunset, 3.3 at civil twilight (sun at -6deg). 0.0145 sun at -12, 0.0004 sun at -18,  0.01 at Full Moon!?
	//qDebug() << "AtmLum: " << atmLum;
	float atmFactor=qMax(0.35f, 50.0f*(0.02f-atmLum)); // keep visible in twilight, but this is enough for some effect with the moon.
	c*=atmFactor*atmFactor;


	if (c[0]<0) c[0]=0;
	if (c[1]<0) c[1]=0;
	if (c[2]<0) c[2]=0;

	const bool withExtinction=(drawer->getFlagHasAtmosphere() && drawer->getExtinction().getExtinctionCoefficient()>=0.01f);

	if (withExtinction)
	{
		// We must process the vertices to find geometric altitudes in order to compute vertex colors.
		// Note that there is a visible boost of extinction for higher Bortle indices. I must reflect that as well.
		const Extinction& extinction=drawer->getExtinction();
		vertexArray->colors.clear();

		for (int i=0; i<vertexArray->vertex.size(); ++i)
		{
			Vec3d vertAltAz=core->j2000ToAltAz(vertexArray->vertex.at(i), StelCore::RefractionOn);
			Q_ASSERT(fabs(vertAltAz.lengthSquared()-1.0) < 0.001);

			float oneMag=0.0f;
			extinction.forward(vertAltAz, &oneMag);
			float extinctionFactor=std::pow(0.3f , oneMag) * (1.1f-bortleIntensity*0.1f); // drop of one magnitude: should be factor 2.5 or 40%. We take 30%, it looks more realistic.
			Vec3f thisColor=Vec3f(c[0]*extinctionFactor, c[1]*extinctionFactor, c[2]*extinctionFactor);
			vertexArray->colors.append(thisColor);
		}
	}
	else
		vertexArray->colors.fill(Vec3f(c[0], c[1], c[2]));

	StelPainter sPainter(prj);
	sPainter.setCullFace(true);
	sPainter.setBlending(true, GL_ONE, GL_ONE); // allow colored sky background
	tex->bind();
	sPainter.setSaturation(static_cast<float>(saturation));
	sPainter.drawStelVertexArray(*vertexArray);
	sPainter.setCullFace(false);
}

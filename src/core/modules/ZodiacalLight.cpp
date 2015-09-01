/*
 * Stellarium
 * Copyright (C) 2002 Fabien Chereau (MilkyWay class)
 * Copyright (C) 2014 Georg Zotti (followed pattern for ZodiacalLight)
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

#include "ZodiacalLight.hpp"
#include "SolarSystem.hpp"
#include "StelFader.hpp"
#include "StelTexture.hpp"
#include "StelUtils.hpp"
#include "StelFileMgr.hpp"
#include "StelModuleMgr.hpp"
#include "LandscapeMgr.hpp"

#include "StelProjector.hpp"
#include "StelToneReproducer.hpp"
#include "StelApp.hpp"
#include "StelTextureMgr.hpp"
#include "StelCore.hpp"
#include "StelSkyDrawer.hpp"
#include "StelPainter.hpp"
#include "StelTranslator.hpp"

#include <QDebug>
#include <QSettings>

// Class which manages the displaying of the Zodiacal Light
ZodiacalLight::ZodiacalLight()
	: color(1.f, 1.f, 1.f)
	, intensity(1.)
	, lastJD(-1.0E6)
	, vertexArray()
{
	setObjectName("ZodiacalLight");
	fader = new LinearFader();
}

ZodiacalLight::~ZodiacalLight()
{
	delete fader;
	fader = NULL;
	
	delete vertexArray;
	vertexArray = NULL;
}

void ZodiacalLight::init()
{
	QSettings* conf = StelApp::getInstance().getSettings();
	Q_ASSERT(conf);

	// The Paper describes brightness values over the complete sky, so also the texture covers the full sky. 
	// The data hole around the sun has been filled by useful values.
	tex = StelApp::getInstance().getTextureManager().createTexture(StelFileMgr::getInstallationDir()+"/textures/zodiacallight_2004.png");
	setFlagShow(conf->value("astro/flag_zodiacal_light", true).toBool());
	setIntensity(conf->value("astro/zodiacal_light_intensity",1.f).toFloat());

	vertexArray = new StelVertexArray(StelPainter::computeSphereNoLight(1.f,1.f,60,30,1, true)); // 6x6 degree quads
	vertexArray->colors.resize(vertexArray->vertex.length());
	vertexArray->colors.fill(Vec3f(1.0, 0.3, 0.9));

	eclipticalVertices=vertexArray->vertex;
	// This vector is used to keep original vertices, these will be modified in update().

	QString displayGroup = N_("Display Options");
	addAction("actionShow_ZodiacalLight", displayGroup, N_("Zodiacal Light"), "flagZodiacalLightDisplayed", "Ctrl+Shift+Z");
}


void ZodiacalLight::update(double deltaTime)
{
	fader->update((int)(deltaTime*1000));

	if (!getFlagShow() || (getIntensity()<0.01) )
		return;

	StelCore* core=StelApp::getInstance().getCore();
	// Test if we are not on Earth. Texture would not fit, so don't draw then.
	if (core->getCurrentLocation().planetName != "Earth") return;

	double currentJD=core->getJD();
	if (qAbs(currentJD - lastJD) > 0.25f) // should be enough to update position every 6 hours.
	{
		// update vertices
		Vec3d obsPos=core->getObserverHeliocentricEclipticPos();
		// For solar-centered texture, take minus, else plus:
		double solarLongitude=atan2(obsPos[1], obsPos[0]) - 0.5*M_PI;
		Mat4d transMat=core->matVsop87ToJ2000 * Mat4d::zrotation(solarLongitude);
		for (int i=0; i<eclipticalVertices.size(); ++i)
		{
			Vec3d tmp=eclipticalVertices.at(i);
			vertexArray->vertex.replace(i, transMat * tmp);
		}
		lastJD=currentJD;
	}
}

void ZodiacalLight::setFlagShow(bool b)
{
	*fader = b;
}

bool ZodiacalLight::getFlagShow() const
{
	return *fader;
}

void ZodiacalLight::draw(StelCore* core)
{
	if (!getFlagShow() || (getIntensity()<0.01) )
		return;

	// Test if we are not on Earth. Texture would not fit, so don't draw then.
	if (core->getCurrentLocation().planetName != "Earth") return;

	StelSkyDrawer *drawer=core->getSkyDrawer();
	int bortle=drawer->getBortleScaleIndex();
	// Test for light pollution, return if too bad.
	if ( (drawer->getFlagHasAtmosphere()) && (bortle > 5) ) return;

	StelProjector::ModelViewTranformP transfo = core->getJ2000ModelViewTransform();

	const StelProjectorP prj = core->getProjection(transfo);
	StelToneReproducer* eye = core->getToneReproducer();

	Q_ASSERT(tex);	// A texture must be loaded before calling this

	// Default ZL color is white (sunlight)
	Vec3f c = Vec3f(1.0f, 1.0f, 1.0f);

	// ZL is quite sensitive to light pollution. I scale to make it less visible.
	float lum = drawer->surfacebrightnessToLuminance(13.5f + 0.5f*bortle); // (8.0f + 0.5*bortle);

	// Get the luminance scaled between 0 and 1
	float aLum =eye->adaptLuminanceScaled(lum*fader->getInterstate());

	// Bound a maximum luminance
	aLum = qMin(0.15f, aLum*2.f); // 2*aLum at dark sky with mag13.5 at Bortle=1 gives 0.13
	//qDebug() << "aLum=" << aLum;

	// intensity of 1.0 is "proper", but allow boost for dim screens
	c*=aLum*intensity;

//	// In brighter twilight we should tune brightness down. So for sun above -18 degrees, we must tweak here:
//	const Vec3d& sunPos = GETSTELMODULE(SolarSystem)->getSun()->getAltAzPosGeometric(core);
//	if (drawer->getFlagHasAtmosphere())
//	{
//		if (sunPos[2] > -0.1) return; // Make ZL invisible during civil twilight and daylight.
//		if (sunPos[2] > -0.3)
//		{ // scale twilight down for sun altitude -18..-6, i.e. scale -0.3..-0.1 to 1..0,
//			float twilightScale= -5.0f* (sunPos[2]+0.1)  ; // 0(if bright)..1(dark)
//			c*=twilightScale;
//		}
//	}

	// Better: adapt brightness by atmospheric brightness
	const float atmLum = GETSTELMODULE(LandscapeMgr)->getAtmosphereAverageLuminance();
	if (atmLum>0.05f) return; // 10cd/m^2 at sunset, 3.3 at civil twilight (sun at -6deg). 0.0145 sun at -12, 0.0004 sun at -18,  0.01 at Full Moon!?
	//qDebug() << "AtmLum: " << atmLum;
	float atmFactor=20.0f*(0.05f-atmLum);
	Q_ASSERT(atmFactor<=1.0f);
	Q_ASSERT(atmFactor>=0.0f);
	c*=atmFactor*atmFactor;

	if (c[0]<0) c[0]=0;
	if (c[1]<0) c[1]=0;
	if (c[2]<0) c[2]=0;

	const bool withExtinction=(drawer->getFlagHasAtmosphere() && drawer->getExtinction().getExtinctionCoefficient()>=0.01f);

	if (withExtinction)
	{
		// We must process the vertices to find geometric altitudes in order to compute vertex colors.
		const Extinction& extinction=drawer->getExtinction();
		vertexArray->colors.clear();

		for (int i=0; i<vertexArray->vertex.size(); ++i)
		{
			Vec3d vertAltAz=core->j2000ToAltAz(vertexArray->vertex.at(i), StelCore::RefractionOn);
			Q_ASSERT(fabs(vertAltAz.lengthSquared()-1.0) < 0.001f);

			float oneMag=0.0f;
			extinction.forward(vertAltAz, &oneMag);
			float extinctionFactor=std::pow(0.4f , oneMag)/bortle; // drop of one magnitude: factor 2.5 or 40%, and further reduced by light pollution
			Vec3f thisColor=Vec3f(c[0]*extinctionFactor, c[1]*extinctionFactor, c[2]*extinctionFactor);
			vertexArray->colors.append(thisColor);
		}
	}
	else
		vertexArray->colors.fill(Vec3f(c[0], c[1], c[2]));

	StelPainter sPainter(prj);
	glEnable(GL_CULL_FACE);
	sPainter.enableTexture2d(true);
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);
	tex->bind();
	sPainter.drawStelVertexArray(*vertexArray);
	glDisable(GL_CULL_FACE);
}

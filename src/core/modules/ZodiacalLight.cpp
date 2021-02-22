/*
 * Stellarium
 * Copyright (C) 2002 Fabien Chereau (MilkyWay class)
 * Copyright (C) 2014-17 Georg Zotti (followed pattern for ZodiacalLight)
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
#include "StelMovementMgr.hpp"
#include "StelSkyDrawer.hpp"
#include "StelPainter.hpp"
#include "StelTranslator.hpp"
#include "precession.h"

#include <QDebug>
#include <QSettings>

// Class which manages the displaying of the Zodiacal Light
ZodiacalLight::ZodiacalLight()
	: propMgr(Q_NULLPTR)
	, color(1.f, 1.f, 1.f)
	, intensity(1.)
	, intensityFovScale(1.0)
	, intensityMinFov(0.25) // when zooming in further, Z.L. is no longer visible.
	, intensityMaxFov(2.5) // when zooming out further, Z.L. is fully visible (when enabled).
	, lastJD(-1.0E6)
	, vertexArray()
{
	setObjectName("ZodiacalLight");
	fader = new LinearFader();
	propMgr=StelApp::getInstance().getStelPropertyManager();
}

ZodiacalLight::~ZodiacalLight()
{
	delete fader;
	fader = Q_NULLPTR;
	
	delete vertexArray;
	vertexArray = Q_NULLPTR;
}

void ZodiacalLight::init()
{
	QSettings* conf = StelApp::getInstance().getSettings();
	Q_ASSERT(conf);

	// The Paper describes brightness values over the complete sky, so also the texture covers the full sky. 
	// The data hole around the sun has been filled by useful values.
	tex = StelApp::getInstance().getTextureManager().createTexture(StelFileMgr::getInstallationDir()+"/textures/zodiacallight_2004.png");
	setFlagShow(conf->value("astro/flag_zodiacal_light", true).toBool());
	setIntensity(conf->value("astro/zodiacal_light_intensity",1.).toDouble());

	vertexArray = new StelVertexArray(StelPainter::computeSphereNoLight(1.,1.,60,30,1, true)); // 6x6 degree quads
	vertexArray->colors.resize(vertexArray->vertex.length());
	vertexArray->colors.fill(color);

	eclipticalVertices=vertexArray->vertex;
	// This vector is used to keep original vertices, these will be modified in update().

	QString displayGroup = N_("Display Options");
	addAction("actionShow_ZodiacalLight", displayGroup, N_("Zodiacal Light"), "flagZodiacalLightDisplayed", "Ctrl+Shift+Z");

	StelCore* core=StelApp::getInstance().getCore();
	connect(core, SIGNAL(locationChanged(StelLocation)), this, SLOT(handleLocationChanged(StelLocation)));
}

void ZodiacalLight::handleLocationChanged(const StelLocation &loc)
{
	// This just forces update() to re-compute longitude.
	Q_UNUSED(loc)
	lastJD=-1e12;
}

void ZodiacalLight::update(double deltaTime)
{
	fader->update(static_cast<int>(deltaTime*1000));
	if (!fader->getInterstate()  || (getIntensity()<0.01) )
		return;

	//calculate FOV fade value, linear fade between intensityMaxFov and intensityMinFov
	double fov = StelApp::getInstance().getCore()->getMovementMgr()->getCurrentFov();
	intensityFovScale = qBound(0.0,(fov - intensityMinFov) / (intensityMaxFov - intensityMinFov),1.0);

	StelCore* core=StelApp::getInstance().getCore();
	// Test if we are not on Earth or Moon. Texture would not fit, so don't draw then.
	if (! QString("Earth Moon").contains(core->getCurrentLocation().planetName)) return;

	double currentJD=core->getJD();
	if (qAbs(currentJD - lastJD) > 0.25) // should be enough to update position every 6 hours.
	{
		// Allowed locations are only Earth or Moon. For Earth, we can compute ZL along ecliptic of date.
		// For the Moon, we can only show ZL along J2000 ecliptic.
		// In draw() we have different projector frames. But we also need separate solar longitude computations here.
		// The ZL texture has its bright spot in the winter solstice point.
		// The computation here returns solar longitude for Earth, and antilongitude for the Moon, therefore we either add or subtract pi/2.

		double lambdaSun;
		if (core->getCurrentLocation().planetName=="Earth")
		{
			double eclJDE = GETSTELMODULE(SolarSystem)->getEarth()->getRotObliquity(core->getJDE());
			double ra_equ, dec_equ, betaJDE;
			StelUtils::rectToSphe(&ra_equ,&dec_equ,GETSTELMODULE(SolarSystem)->getSun()->getEquinoxEquatorialPos(core));
			StelUtils::equToEcl(ra_equ, dec_equ, eclJDE, &lambdaSun, &betaJDE);
			lambdaSun+= M_PI*0.5;
		}
		else // currently Moon only...
		{
			Vec3d obsPos=core->getObserverHeliocentricEclipticPos();
			lambdaSun=atan2(obsPos[1], obsPos[0])  -M_PI*0.5;
		}

		Mat4d rotMat=Mat4d::zrotation(lambdaSun);
		for (int i=0; i<eclipticalVertices.size(); ++i)
		{
			Vec3d tmp=eclipticalVertices.at(i);
			vertexArray->vertex.replace(i, rotMat * tmp);
		}
		lastJD=currentJD;
	}
}

/*************************************************************************
 Reimplementation of the getCallOrder method
*************************************************************************/
double ZodiacalLight::getCallOrder(StelModuleActionName actionName) const
{
	if (actionName==StelModule::ActionDraw)
		return 8;
	return 0;
}

void ZodiacalLight::setFlagShow(bool b)
{
	if (*fader != b)
	{
		*fader = b;
		emit zodiacalLightDisplayedChanged(b);
	}
}

bool ZodiacalLight::getFlagShow() const
{
	return *fader;
}

void ZodiacalLight::draw(StelCore* core)
{
	if ((fader->getInterstate() == 0.f) || (getIntensity()<0.01) || !(propMgr->getStelPropertyValue("SolarSystem.planetsDisplayed").toBool()))
		return;

	// Test if we are not on Earth. Texture would not fit, so don't draw then.
	if (! QString("Earth Moon").contains(core->getCurrentLocation().planetName)) return;

	StelSkyDrawer *drawer=core->getSkyDrawer();
	int bortle=drawer->getBortleScaleIndex();
	// Test for light pollution, return if too bad.
	if ( (drawer->getFlagHasAtmosphere()) && (bortle > 5) ) return;

	float atmFadeIntensity = GETSTELMODULE(LandscapeMgr)->getAtmosphereFadeIntensity();
	float bortleIntensity = 1.f+ (bortle-1)*atmFadeIntensity; // Bortle index moderated by atmosphere fader.

	// The ZL is best observed from Earth only. On the Moon, we must be happy with ZL along the J2000 ecliptic. (Sorry for LP:1628765, I don't find a general solution.)
	StelProjector::ModelViewTranformP transfo;
	if (core->getCurrentLocation().planetName == "Earth")
		transfo = core->getObservercentricEclipticOfDateModelViewTransform();
	else
		transfo = core->getObservercentricEclipticJ2000ModelViewTransform();

	const StelProjectorP prj = core->getProjection(transfo);
	StelToneReproducer* eye = core->getToneReproducer();

	Q_ASSERT(tex);	// A texture must be loaded before calling this

	// Default ZL color is white (sunlight), or whatever has been set e.g. by script.
	Vec3f c = color;

	// ZL is quite sensitive to light pollution. I scale to make it less visible.
	float lum = drawer->surfaceBrightnessToLuminance(13.5f + 0.5f*bortleIntensity); // (8.0f + 0.5*bortle);

	// Get the luminance scaled between 0 and 1
	float aLum =eye->adaptLuminanceScaled(lum*fader->getInterstate());

	// Bound a maximum luminance
	aLum = qMin(0.15f, aLum*2.f); // 2*aLum at dark sky with mag13.5 at Bortle=1 gives 0.13
	//qDebug() << "aLum=" << aLum;

	// intensity of 1.0 is "proper", but allow boost for dim screens
	c*=aLum*static_cast<float>(intensity*intensityFovScale);

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

	if ((withExtinction) && (core->getCurrentLocation().planetName=="Earth")) // If anybody switches on atmosphere on the moon, there will be no extinction.
	{
		// We must process the vertices to find geometric altitudes in order to compute vertex colors.
		const Extinction& extinction=drawer->getExtinction();
		const double epsDate=getPrecessionAngleVondrakCurrentEpsilonA();
		vertexArray->colors.clear();

		for (int i=0; i<vertexArray->vertex.size(); ++i)
		{
			Vec3d eclPos=vertexArray->vertex.at(i);
			Q_ASSERT(fabs(eclPos.lengthSquared()-1.0) < 0.001);
			double ecLon, ecLat, ra, dec;
			StelUtils::rectToSphe(&ecLon, &ecLat, eclPos);
			StelUtils::eclToEqu(ecLon, ecLat, epsDate, &ra, &dec);
			Vec3d eqPos;
			StelUtils::spheToRect(ra, dec, eqPos);
			Vec3d vertAltAz=core->equinoxEquToAltAz(eqPos, StelCore::RefractionOn);
			Q_ASSERT(fabs(vertAltAz.lengthSquared()-1.0) < 0.001);

			float oneMag=0.0f;
			extinction.forward(vertAltAz, &oneMag);
			float extinctionFactor=std::pow(0.4f , oneMag) * (1.1f-bortleIntensity*0.1f);// Drop of one magnitude: factor 2.5 or 40%, and further reduced by light pollution.
			Vec3f thisColor=Vec3f(c[0]*extinctionFactor, c[1]*extinctionFactor, c[2]*extinctionFactor);
			vertexArray->colors.append(thisColor);
		}
	}
	else
		vertexArray->colors.fill(Vec3f(c[0], c[1], c[2]));

	StelPainter sPainter(prj);
	sPainter.setCullFace(true);
	sPainter.setBlending(true, GL_ONE, GL_ONE);
	tex->bind();
	sPainter.drawStelVertexArray(*vertexArray);
	sPainter.setCullFace(false);
}

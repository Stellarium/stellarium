/*
 * Stellarium
 * Copyright (C) 2007 Fabien Chereau
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

#include "StelSkyDrawer.hpp"
#include "StelProjector.hpp"

#include "StelToneReproducer.hpp"
#include "renderer/StelTextureNew.hpp"
#include "renderer/StelRenderer.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelUtils.hpp"
#include "StelMovementMgr.hpp"

#include <QStringList>
#include <QSettings>
#include <QDebug>
#include <QtGlobal>

// The 0.025 corresponds to the maximum eye resolution in degree
#define EYE_RESOLUTION (0.25f)
#define MAX_LINEAR_RADIUS (8.f)

StelSkyDrawer::StelSkyDrawer(StelCore* acore, StelRenderer* renderer)
	: core(acore)
	, renderer(renderer)
	, texHalo(NULL)
	, starPointBuffer(NULL)
	, starSpriteBuffer(NULL)
	, drawing(false)
	, texBigHalo(NULL)
	, texSunHalo(NULL)
	, texCorona(NULL)
	, statisticsInitialized(false)
	, bigHaloStatID(-1)
	, sunHaloStatID(-1)
	, starStatID(-1)
{
	eye = core->getToneReproducer();

	inScale = 1.f;
	bortleScaleIndex = 3;
	limitMagnitude = -100.f;
	limitLuminance = 0;
	oldLum=-1.f;
	maxLum = 0.f;
	setMaxAdaptFov(180.f);
	setMinAdaptFov(0.1f);

	starAbsoluteScaleF = 1.f;
	starRelativeScale = 1.f;
	starLinearScale = 19.569f;

	big3dModelHaloRadius = 150.f;

	QSettings* conf = StelApp::getInstance().getSettings();
	initColorTableFromConfigFile(conf);

	setFlagHasAtmosphere(conf->value("landscape/flag_atmosphere", true).toBool());
	setTwinkleAmount(conf->value("stars/star_twinkle_amount",0.3).toFloat());
	setFlagTwinkle(conf->value("stars/flag_star_twinkle",true).toBool());
	setDrawStarsAsPoints(conf->value("stars/flag_point_star",false).toBool());
	setMaxAdaptFov(conf->value("stars/mag_converter_max_fov",70.0).toFloat());
	setMinAdaptFov(conf->value("stars/mag_converter_min_fov",0.1).toFloat());
	setFlagLuminanceAdaptation(conf->value("viewing/use_luminance_adaptation",true).toBool());

	bool ok=true;

	setBortleScale(conf->value("stars/init_bortle_scale",3).toInt(&ok));
	if (!ok)
	{
		conf->setValue("stars/init_bortle_scale",3);
		setBortleScale(3);
		ok = true;
	}

	setRelativeStarScale(conf->value("stars/relative_scale",1.0).toFloat(&ok));
	if (!ok)
	{
		conf->setValue("stars/relative_scale",1.0);
		setRelativeStarScale(1.0);
		ok = true;
	}

	setAbsoluteStarScale(conf->value("stars/absolute_scale",1.0).toFloat(&ok));
	if (!ok)
	{
		conf->setValue("stars/absolute_scale",1.0);
		setAbsoluteStarScale(1.0);
		ok = true;
	}

	//GZ: load 3 values from config.
	setExtinctionCoefficient(conf->value("landscape/atmospheric_extinction_coefficient",0.2).toDouble(&ok));
	if (!ok)
	{
		conf->setValue("landscape/atmospheric_extinction_coefficient",0.2);
		setExtinctionCoefficient(0.2);
		ok = true;
	}
	setAtmosphereTemperature(conf->value("landscape/temperature_C",15.0).toDouble(&ok));
	if (!ok)
	{
		conf->setValue("landscape/temperature_C",15);
		setAtmosphereTemperature(15.0);
		ok = true;
	}
	setAtmospherePressure(conf->value("landscape/pressure_mbar",1013.0).toDouble(&ok));
	if (!ok)
	{
		conf->setValue("landscape/pressure_mbar",1013.0);
		setAtmospherePressure(1013.0);
		ok = true;
	}

	starPointBuffer   = renderer->createVertexBuffer<ColoredVertex>(PrimitiveType_Points);
	starSpriteBuffer  = renderer->createVertexBuffer<ColoredTexturedVertex>(PrimitiveType_Triangles);
	bigHaloBuffer     = renderer->createVertexBuffer<ColoredTexturedVertex>(PrimitiveType_Triangles);
	sunHaloBuffer     = renderer->createVertexBuffer<ColoredTexturedVertex>(PrimitiveType_Triangles);
	coronaBuffer      = renderer->createVertexBuffer<ColoredTexturedVertex>(PrimitiveType_Triangles);
}

StelSkyDrawer::~StelSkyDrawer()
{                               
	if(NULL != starPointBuffer)  {delete starPointBuffer;}
	if(NULL != starSpriteBuffer) {delete starSpriteBuffer;}
	if(NULL != bigHaloBuffer)    {delete bigHaloBuffer;}
	if(NULL != sunHaloBuffer)    {delete sunHaloBuffer;}
	if(NULL != coronaBuffer)     {delete coronaBuffer;}
	if(NULL != texBigHalo)       {delete texBigHalo;}
	if(NULL != texSunHalo)       {delete texSunHalo;}
	if(NULL != texHalo)          {delete texHalo;}
	if(NULL != texCorona)        {delete texCorona;}
}

// Init parameters from config file
void StelSkyDrawer::init()
{
	// Load star texture no mipmap:
	texHalo    = renderer->createTexture("textures/star16x16.png");
	texBigHalo = renderer->createTexture("textures/haloLune.png");
	texSunHalo = renderer->createTexture("textures/halo.png");
	texCorona  = renderer->createTexture("textures/corona.png");

	update(0);
}

void StelSkyDrawer::update(double)
{
	float fov = core->getMovementMgr()->getCurrentFov();
	if (fov > maxAdaptFov)
	{
		fov = maxAdaptFov;
	}
	else
	{
		if (fov < minAdaptFov)
			fov = minAdaptFov;
	}

	// GZ: Light pollution must take global atmosphere setting into acount!
	// moved parts from setBortleScale() here
	// These value have been calibrated by hand, looking at the faintest star in stellarium at around 40 deg FOV
	// They should roughly match the scale described at http://en.wikipedia.org/wiki/Bortle_Dark-Sky_Scale
	static const float bortleToInScale[9] = {2.45, 1.55, 1.0, 0.63, 0.40, 0.24, 0.23, 0.145, 0.09};
	if (getFlagHasAtmosphere())
	    setInputScale(bortleToInScale[bortleScaleIndex-1]);
	else
	    setInputScale(bortleToInScale[0]);

	// This factor is fully arbitrary. It corresponds to the collecting area x exposure time of the instrument
	// It is based on a power law, so that it varies progressively with the FOV to smoothly switch from human
	// vision to binocculares/telescope. Use a max of 0.7 because after that the atmosphere starts to glow too much!
	float powFactor = std::pow(60.f/qMax(0.7f,fov), 0.8f);
	eye->setInputScale(inScale*powFactor);

	// Set the fov factor for point source luminance computation
	// the division by powFactor should in principle not be here, but it doesn't look nice if removed
	lnfovFactor = std::log(1.f/50.f*2025000.f* 60.f*60.f / (fov*fov) / (EYE_RESOLUTION*EYE_RESOLUTION)/powFactor/1.4f);

	// Precompute
	starLinearScale = std::pow(35.f*2.0f*starAbsoluteScaleF, 1.40f/2.f*starRelativeScale);

	// update limit mag
	limitMagnitude = computeLimitMagnitude();

	// update limit luminance
	limitLuminance = computeLimitLuminance();
}

// Compute the current limit magnitude by dichotomy
float StelSkyDrawer::computeLimitMagnitude() const
{
	float a=-26.f;
	float b=30.f;
	float rcmag[2];
	float lim = 0.f;
	int safety=0;
	while (std::fabs(lim-a)>0.05)
	{
		computeRCMag(lim, rcmag);
		if (rcmag[0]<=0.f)
		{
			float tmp = lim;
			lim=(a+lim)*0.5;
			b=tmp;
		}
		else
		{
			float tmp = lim;
			lim=(b+lim)*0.5;
			a=tmp;
		}
		++safety;
		if (safety>20)
		{
			lim=-99;
			break;
		}
	}
	return lim;
}

// Compute the current limit luminance by dichotomy
float StelSkyDrawer::computeLimitLuminance() const
{
	float a=0.f;
	float b=500000.f;
	float lim=40.f;
	int safety=0;
	float adaptL;
	while (std::fabs(lim-a)>0.05)
	{
		adaptL = eye->adaptLuminanceScaled(lim);
		if (adaptL<=0.05f) // Object considered not visible if its adapted scaled luminance<0.05
		{
			float tmp = lim;
			lim=(b+lim)*0.5;
			a=tmp;
		}
		else
		{
			float tmp = lim;
			lim=(a+lim)*0.5;
			b=tmp;
		}
		++safety;
		if (safety>30)
		{
			lim=500000;
			break;
		}
	}
	return lim;
}

// Compute the ln of the luminance for a point source with the given mag for the current FOV
float StelSkyDrawer::pointSourceMagToLnLuminance(float mag) const
{
	return -0.92103f*(mag + 12.12331f) + lnfovFactor;
}

float StelSkyDrawer::pointSourceLuminanceToMag(float lum)
{
	return (std::log(lum) - lnfovFactor)/-0.92103f - 12.12331f;
}

// Compute the luminance for an extended source with the given surface brightness in Vmag/arcmin^2
float StelSkyDrawer::surfacebrightnessToLuminance(float sb)
{
	return 2.*2025000.f*std::exp(-0.92103f*(sb + 12.12331f))/(1./60.*1./60.);
}

// Compute the surface brightness from the luminance of an extended source
float StelSkyDrawer::luminanceToSurfacebrightness(float lum)
{
	return std::log(lum*(1./60.*1./60.)/(2.*2025000.f))/-0.92103f - 12.12331f;
}

// Compute RMag and CMag from magnitude for a point source.
bool StelSkyDrawer::computeRCMag(float mag, float rcMag[2]) const
{
	rcMag[0] = eye->adaptLuminanceScaledLn(pointSourceMagToLnLuminance(mag), starRelativeScale*1.40f/2.f);
	rcMag[0]*=starLinearScale;

	// Use now statically min_rmag = 0.5, because higher and too small values look bad
	if (rcMag[0] < 0.5f)
	{
		rcMag[0] = rcMag[1] = 0.f;
		return false;
	}

	// if size of star is too small (blink) we put its size to 1.2 --> no more blink
	// And we compensate the difference of brighteness with cmag
	if (rcMag[0]<1.2f)
	{
		rcMag[1] = rcMag[0] * rcMag[0] / 1.44f;
		if (rcMag[1] < 0.07f)
		{
			rcMag[0] = rcMag[1] = 0.f;
			return false;
		}
		rcMag[0] = 1.2f;
	}
	else
	{
		// cmag:
		rcMag[1] = 1.0f;
		if (rcMag[0]>MAX_LINEAR_RADIUS)
		{
			rcMag[0]=MAX_LINEAR_RADIUS+std::sqrt(1.f+rcMag[0]-MAX_LINEAR_RADIUS)-1.f;
		}
	}
	return true;
}

void StelSkyDrawer::preDrawPointSource()
{
	Q_ASSERT_X(!drawing, Q_FUNC_INFO,
	           "Attempting to start drawing point sources when it is already started");

	drawing = true;
}

// Helper function that sets Add blend mode, then draws and clears a vertex buffer.
//
// The buffer is cleared since we re-add the stars on each frame.
template<class VB>
void drawStars(StelTextureNew* texture, VB* vertices, StelRenderer* renderer, StelProjectorP projector)
{
	if(NULL != texture){texture->bind();}
	renderer->setBlendMode(BlendMode_Add);
	vertices->lock();

	// Vertices are already projected, so we use the dontProject
	// argument of drawVertexBuffer.
	//
	// This is a hack - it would be better to refactor StelSkyDrawer,
	// ZoneArray, etc, so projection gets done by the projector 
	// during drawing like elsewhere.
	
	renderer->drawVertexBuffer(vertices, NULL, projector, true);
	vertices->unlock();
	vertices->clear();
}

// Finalize the drawing of point sources
void StelSkyDrawer::postDrawPointSource(StelProjectorP projector)
{
	StelRendererStatistics& stats = renderer->getStatistics();
	if(!statisticsInitialized)
	{
		bigHaloStatID = stats.addStatistic("big_halo_draws", StatisticSwapMode_SetToZero);
		sunHaloStatID = stats.addStatistic("sun_halo_draws", StatisticSwapMode_SetToZero);
		starStatID    = stats.addStatistic("star_draws",     StatisticSwapMode_SetToZero);
		statisticsInitialized = true;
	}
	if(bigHaloBuffer->length() > 0)
	{
		drawStars(texBigHalo, bigHaloBuffer, renderer, projector);
		stats[bigHaloStatID] += 1.0;
	}
	if(sunHaloBuffer->length() > 0)
	{
		drawStars(texSunHalo, sunHaloBuffer, renderer, projector);
		stats[sunHaloStatID] += 1.0;
	}
	if(drawStarsAsPoints && starPointBuffer->length() > 0)
	{
		stats[starStatID] += 1.0;
		drawStars(NULL, starPointBuffer, renderer, projector);
	}
	else if(starSpriteBuffer->length() > 0)
	{
		stats[starStatID] += 1.0;
		drawStars(texHalo, starSpriteBuffer, renderer, projector);
	}

	drawing = false;
}

//Helper function that adds the halo of a star/other point source to given vertex buffer.
//
//The template parameter is only used so we don't need to write ColoredTexturedVertex
//every time, this function is only meant to be used from drawPointSource.
template<class V>
static void addStar(StelVertexBuffer<V>* vertices,
                    const float x, const float y,
                    const float radius, const Vec3f& color)
{
	// 1 triangle around the star sprite. We use the fact 
	// that edges of the halo textures are transparent and 
	// the clamp to edge draw mode. With that, we can draw 1 
	// larger triangle around the sprite quad instead of drawing
	// the quad as 2 triangles.
	
	const float yBase  = y - radius;
	const float yTop   = y + radius * 2.6666666;
	const float xLeft  = x - radius * 2.3333333;
	const float xRight = x + radius * 2.3333333;

	vertices->addVertex(V(Vec2f(xLeft,  yBase), color, Vec2f(-0.6666666f, 0.0f)));
	vertices->addVertex(V(Vec2f(xRight, yBase), color, Vec2f(1.6666666f,  0.0f)));
	vertices->addVertex(V(Vec2f(x,      yTop),  color, Vec2f(0.5f,        1.83333333f)));
}

// Draw a point source halo.
void StelSkyDrawer::drawPointSource
	(const Vec3f& win, const float rcMag[2], const Vec3f& bcolor)
{
	Q_ASSERT_X(drawing, Q_FUNC_INFO,
	           "Attempting to draw a point source without calling preDrawPointSource first.");
	const float radius    = rcMag[0];
	const float luminance = rcMag[1];

	const Vec3f color = StelApp::getInstance().getVisionModeNight() 
	                  ? Vec3f(bcolor[0], 0, 0) : bcolor;
	// Random coef for star twinkling
	const float tw = (flagStarTwinkle && flagHasAtmosphere) 
	                 ? (1.0f - twinkleAmount * rand() / RAND_MAX) * luminance
	                 : luminance;

	const float x = win[0];
	const float y = win[1];

	// If the rmag is big, draw a big halo
	if (radius > MAX_LINEAR_RADIUS + 5.0f)
	{
		const float cmag = qMin(1.0f, qMin(luminance, (radius - MAX_LINEAR_RADIUS + 5.0f) / 30.f));
		addStar(bigHaloBuffer, x, y, 150.0f, color * cmag);
	}

	if (drawStarsAsPoints)
	{
		starPointBuffer->addVertex(ColoredVertex(Vec2f(x, y), color * tw));
	}
	else
	{
		addStar(starSpriteBuffer, x, y, radius, color * tw);
	}
}

// Draw's the sun's corona during a solar eclipse on earth.
void StelSkyDrawer::drawSunCorona(StelProjectorP projector, const Vec3d &v, float radius, float alpha)
{
	Vec3d win;
	projector->project(v, win);
	addStar(coronaBuffer, win[0], win[1], radius * 2, Vec3f(alpha, alpha, alpha));
	drawStars(texCorona, coronaBuffer, renderer, projector);
}

// Terminate drawing of a 3D model, draw the halo
void StelSkyDrawer::postDrawSky3dModel
	(StelProjectorP projector, const Vec3d& v, float illuminatedArea, 
	 float mag, const Vec3f& color)
{
	const float pixPerRad = projector->getPixelPerRadAtCenter();
	// Assume a disk shape
	float pixRadius = std::sqrt(illuminatedArea/(60.*60.)*M_PI/180.*M_PI/180.*(pixPerRad*pixPerRad))/M_PI;

	bool noStarHalo = false;

	if (mag<-15.f)
	{
		const float rmag = big3dModelHaloRadius*(mag+15.f)/-11.f;
		float cmag = 1.f;
		if (rmag<pixRadius*3.f+100.0f)
		{
			cmag = qMax(0.f, 1.f-(pixRadius*3.f+100-rmag)/100);
		}
		Vec3d win;
		projector->project(v, win);
		Vec3f c = color;
		if (StelApp::getInstance().getVisionModeNight())
			c = StelUtils::getNightColor(c);

		addStar(sunHaloBuffer, win[0], win[1], rmag, c * cmag);
		noStarHalo = true;
	}

	// Now draw the halo according the object brightness
	bool save = flagStarTwinkle;
	flagStarTwinkle = false;

	float rcm[2];
	computeRCMag(mag, rcm);

	// We now have the radius and luminosity of the small halo
	// If the disk of the planet is big enough to be visible, we should adjust the eye adaptation luminance
	// so that the radius of the halo is small enough to be not visible (so that we see the disk)

	float tStart = 2.f;
	float tStop = 6.f;
	bool truncated=false;

	float maxHaloRadius = qMax(tStart*3., pixRadius*3.);
	if (rcm[0]>maxHaloRadius)
	{
		truncated = true;
		rcm[0]=maxHaloRadius+std::sqrt(rcm[0]-maxHaloRadius);
	}

	// Fade the halo away when the disk is too big
	if (pixRadius>=tStop)
	{
		rcm[1]=0.f;
	}
	if (pixRadius>tStart && pixRadius<tStop)
	{
		rcm[1]=(tStop-pixRadius)/(tStop-tStart);
	}

	if (truncated && flagLuminanceAdaptation)
	{
		float wl = findWorldLumForMag(mag, rcm[0]);
		if (wl>0)
		{
			const float f = core->getMovementMgr()->getCurrentFov();
			reportLuminanceInFov(qMin(700.f, qMin(wl/50, (60.f*60.f)/(f*f)*6.f)));
		}
	}

	if (!noStarHalo)
	{
		preDrawPointSource();
		const Vec3f vf(v[0], v[1], v[2]);
		Vec3f win;
		if(pointSourceVisible(&(*projector), vf, rcm, false, win))
		{
			drawPointSource(win, rcm, color);
		}
		postDrawPointSource(projector);
	}
	flagStarTwinkle=save;
}

float StelSkyDrawer::findWorldLumForMag(float mag, float targetRadius)
{
	const float saveLum = eye->getWorldAdaptationLuminance();	// save

	// Compute the luminance by dichotomy
	float a=0.001f;
	float b=500000.f;
	float rcmag[2];
	rcmag[0]=-99;
	float curLum = 500.f;
	int safety=0;
	while (std::fabs(rcmag[0]-targetRadius)>0.1)
	{
		eye->setWorldAdaptationLuminance(curLum);
		computeRCMag(mag, rcmag);
		if (rcmag[0]<=targetRadius)
		{
			float tmp = curLum;
			curLum=(a+curLum)/2;
			b=tmp;
		}
		else
		{
			float tmp = curLum;
			curLum=(b+curLum)/2;
			a=tmp;
		}
		++safety;
		if (safety>20)
		{
			if (curLum>490000.f)
			{
				curLum = 500000.f;
			}
			else
			{
				curLum=-1;
			}
			break;
		}
	}

	eye->setWorldAdaptationLuminance(saveLum);	// restore

	return curLum;
}

// Report that an object of luminance lum is currently displayed
void StelSkyDrawer::reportLuminanceInFov(float lum, bool fastAdaptation)
{
	if (lum > maxLum)
	{
		if (oldLum<0)
			oldLum=lum;
		// Use a log law for smooth transitions
		if (fastAdaptation==true && lum>oldLum)
		{
			maxLum = lum;
		}
		else
		{
			float transitionSpeed = 0.2f;
			maxLum = std::exp(std::log(oldLum)+(std::log(lum)-std::log(oldLum))* qMin(1.f, 1.f/StelApp::getInstance().getFps()/transitionSpeed));
		}
	}
}

void StelSkyDrawer::preDraw()
{
	eye->setWorldAdaptationLuminance(maxLum);
	// Re-initialize for next stage
	oldLum = maxLum;
	maxLum = 0;
}


// Set the parameters so that the stars disapear at about the limit given by the bortle scale
// See http://en.wikipedia.org/wiki/Bortle_Dark-Sky_Scale
void StelSkyDrawer::setBortleScale(int bIndex)
{
	// Associate the Bortle index (1 to 9) to inScale value
	if (bIndex<1)
	{
		qWarning() << "WARING: Bortle scale index range is [1;9], given" << bIndex;
		bIndex = 1;
	}
	if (bIndex>9)
	{
		qWarning() << "WARING: Bortle scale index range is [1;9], given" << bIndex;
		bIndex = 9;
	}

	bortleScaleIndex = bIndex;
	// GZ: I moved this block to update()
	// These value have been calibrated by hand, looking at the faintest star in stellarium at around 40 deg FOV
	// They should roughly match the scale described at http://en.wikipedia.org/wiki/Bortle_Dark-Sky_Scale
	// static const float bortleToInScale[9] = {2.45, 1.55, 1.0, 0.63, 0.40, 0.24, 0.23, 0.145, 0.09};
	// setInputScale(bortleToInScale[bIndex-1]);
}


// New colors
Vec3f StelSkyDrawer::colorTable[128] = {
	Vec3f(0.602745,0.713725,1.000000),
	Vec3f(0.604902,0.715294,1.000000),
	Vec3f(0.607059,0.716863,1.000000),
	Vec3f(0.609215,0.718431,1.000000),
	Vec3f(0.611372,0.720000,1.000000),
	Vec3f(0.613529,0.721569,1.000000),
	Vec3f(0.635490,0.737255,1.000000),
	Vec3f(0.651059,0.749673,1.000000),
	Vec3f(0.666627,0.762092,1.000000),
	Vec3f(0.682196,0.774510,1.000000),
	Vec3f(0.697764,0.786929,1.000000),
	Vec3f(0.713333,0.799347,1.000000),
	Vec3f(0.730306,0.811242,1.000000),
	Vec3f(0.747278,0.823138,1.000000),
	Vec3f(0.764251,0.835033,1.000000),
	Vec3f(0.781223,0.846929,1.000000),
	Vec3f(0.798196,0.858824,1.000000),
	Vec3f(0.812282,0.868236,1.000000),
	Vec3f(0.826368,0.877647,1.000000),
	Vec3f(0.840455,0.887059,1.000000),
	Vec3f(0.854541,0.896470,1.000000),
	Vec3f(0.868627,0.905882,1.000000),
	Vec3f(0.884627,0.916862,1.000000),
	Vec3f(0.900627,0.927843,1.000000),
	Vec3f(0.916627,0.938823,1.000000),
	Vec3f(0.932627,0.949804,1.000000),
	Vec3f(0.948627,0.960784,1.000000),
	Vec3f(0.964444,0.972549,1.000000),
	Vec3f(0.980261,0.984313,1.000000),
	Vec3f(0.996078,0.996078,1.000000),
	Vec3f(1.000000,1.000000,1.000000),
	Vec3f(1.000000,0.999643,0.999287),
	Vec3f(1.000000,0.999287,0.998574),
	Vec3f(1.000000,0.998930,0.997861),
	Vec3f(1.000000,0.998574,0.997148),
	Vec3f(1.000000,0.998217,0.996435),
	Vec3f(1.000000,0.997861,0.995722),
	Vec3f(1.000000,0.997504,0.995009),
	Vec3f(1.000000,0.997148,0.994296),
	Vec3f(1.000000,0.996791,0.993583),
	Vec3f(1.000000,0.996435,0.992870),
	Vec3f(1.000000,0.996078,0.992157),
	Vec3f(1.000000,0.991140,0.981554),
	Vec3f(1.000000,0.986201,0.970951),
	Vec3f(1.000000,0.981263,0.960349),
	Vec3f(1.000000,0.976325,0.949746),
	Vec3f(1.000000,0.971387,0.939143),
	Vec3f(1.000000,0.966448,0.928540),
	Vec3f(1.000000,0.961510,0.917938),
	Vec3f(1.000000,0.956572,0.907335),
	Vec3f(1.000000,0.951634,0.896732),
	Vec3f(1.000000,0.946695,0.886129),
	Vec3f(1.000000,0.941757,0.875526),
	Vec3f(1.000000,0.936819,0.864924),
	Vec3f(1.000000,0.931881,0.854321),
	Vec3f(1.000000,0.926942,0.843718),
	Vec3f(1.000000,0.922004,0.833115),
	Vec3f(1.000000,0.917066,0.822513),
	Vec3f(1.000000,0.912128,0.811910),
	Vec3f(1.000000,0.907189,0.801307),
	Vec3f(1.000000,0.902251,0.790704),
	Vec3f(1.000000,0.897313,0.780101),
	Vec3f(1.000000,0.892375,0.769499),
	Vec3f(1.000000,0.887436,0.758896),
	Vec3f(1.000000,0.882498,0.748293),
	Vec3f(1.000000,0.877560,0.737690),
	Vec3f(1.000000,0.872622,0.727088),
	Vec3f(1.000000,0.867683,0.716485),
	Vec3f(1.000000,0.862745,0.705882),
	Vec3f(1.000000,0.858617,0.695975),
	Vec3f(1.000000,0.854490,0.686068),
	Vec3f(1.000000,0.850362,0.676161),
	Vec3f(1.000000,0.846234,0.666254),
	Vec3f(1.000000,0.842107,0.656346),
	Vec3f(1.000000,0.837979,0.646439),
	Vec3f(1.000000,0.833851,0.636532),
	Vec3f(1.000000,0.829724,0.626625),
	Vec3f(1.000000,0.825596,0.616718),
	Vec3f(1.000000,0.821468,0.606811),
	Vec3f(1.000000,0.817340,0.596904),
	Vec3f(1.000000,0.813213,0.586997),
	Vec3f(1.000000,0.809085,0.577090),
	Vec3f(1.000000,0.804957,0.567183),
	Vec3f(1.000000,0.800830,0.557275),
	Vec3f(1.000000,0.796702,0.547368),
	Vec3f(1.000000,0.792574,0.537461),
	Vec3f(1.000000,0.788447,0.527554),
	Vec3f(1.000000,0.784319,0.517647),
	Vec3f(1.000000,0.784025,0.520882),
	Vec3f(1.000000,0.783731,0.524118),
	Vec3f(1.000000,0.783436,0.527353),
	Vec3f(1.000000,0.783142,0.530588),
	Vec3f(1.000000,0.782848,0.533824),
	Vec3f(1.000000,0.782554,0.537059),
	Vec3f(1.000000,0.782259,0.540294),
	Vec3f(1.000000,0.781965,0.543529),
	Vec3f(1.000000,0.781671,0.546765),
	Vec3f(1.000000,0.781377,0.550000),
	Vec3f(1.000000,0.781082,0.553235),
	Vec3f(1.000000,0.780788,0.556471),
	Vec3f(1.000000,0.780494,0.559706),
	Vec3f(1.000000,0.780200,0.562941),
	Vec3f(1.000000,0.779905,0.566177),
	Vec3f(1.000000,0.779611,0.569412),
	Vec3f(1.000000,0.779317,0.572647),
	Vec3f(1.000000,0.779023,0.575882),
	Vec3f(1.000000,0.778728,0.579118),
	Vec3f(1.000000,0.778434,0.582353),
	Vec3f(1.000000,0.778140,0.585588),
	Vec3f(1.000000,0.777846,0.588824),
	Vec3f(1.000000,0.777551,0.592059),
	Vec3f(1.000000,0.777257,0.595294),
	Vec3f(1.000000,0.776963,0.598530),
	Vec3f(1.000000,0.776669,0.601765),
	Vec3f(1.000000,0.776374,0.605000),
	Vec3f(1.000000,0.776080,0.608235),
	Vec3f(1.000000,0.775786,0.611471),
	Vec3f(1.000000,0.775492,0.614706),
	Vec3f(1.000000,0.775197,0.617941),
	Vec3f(1.000000,0.774903,0.621177),
	Vec3f(1.000000,0.774609,0.624412),
	Vec3f(1.000000,0.774315,0.627647),
	Vec3f(1.000000,0.774020,0.630883),
	Vec3f(1.000000,0.773726,0.634118),
	Vec3f(1.000000,0.773432,0.637353),
	Vec3f(1.000000,0.773138,0.640588),
	Vec3f(1.000000,0.772843,0.643824),
	Vec3f(1.000000,0.772549,0.647059),
};

static float Gamma(float gamma, float x)
{
	return ((x<=0.f) ? 0.f : std::exp(gamma*std::log(x)));
}

static Vec3f Gamma(float gamma,const Vec3f &x)
{
	return Vec3f(Gamma(gamma,x[0]),Gamma(gamma,x[1]),Gamma(gamma,x[2]));
}

// Load B-V conversion parameters from config file
void StelSkyDrawer::initColorTableFromConfigFile(QSettings* conf)
{
	std::map<float,Vec3f> color_map;
	for (float bV=-0.5f;bV<=4.0f;bV+=0.01)
	{
		char entry[256];
		sprintf(entry,"bv_color_%+5.2f",bV);
		const QStringList s(conf->value(QString("stars/") + entry).toStringList());
		if (!s.isEmpty())
		{
			Vec3f c;
			if (s.size()==1)
				c = StelUtils::strToVec3f(s[0]);
			else
				c =StelUtils::strToVec3f(s);
			color_map[bV] = Gamma(eye->getDisplayGamma(),c);
		}
	}

	if (color_map.size() > 1)
	{
		for (int i=0;i<128;i++)
		{
			const float bV = StelSkyDrawer::indexToBV(i);
			std::map<float,Vec3f>::const_iterator greater(color_map.upper_bound(bV));
			if (greater == color_map.begin())
			{
				colorTable[i] = greater->second;
			}
			else
			{
				std::map<float,Vec3f>::const_iterator less(greater);--less;
				if (greater == color_map.end())
				{
					colorTable[i] = less->second;
				}
				else
				{
					colorTable[i] = Gamma(1.f/eye->getDisplayGamma(), ((bV-less->first)*greater->second + (greater->first-bV)*less->second) *(1.f/(greater->first-less->first)));
				}
			}
		}
	}

// 	QString res;
// 	for (int i=0;i<128;i++)
// 	{
// 		res += QString("Vec3f(%1,%2,%3),\n").arg(colorTable[i][0], 0, 'g', 6).arg(colorTable[i][1], 0, 'g', 6).arg(colorTable[i][2], 0, 'g', 6);
// 	}
// 	qDebug() << res;
}

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
#include "StelFileMgr.hpp"

#include "StelToneReproducer.hpp"
#include "StelTextureMgr.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelMovementMgr.hpp"
#include "StelPainter.hpp"
#ifndef USE_OLD_QGLWIDGET
#include "StelMainView.hpp"
#endif

#include "StelModuleMgr.hpp"
#include "LandscapeMgr.hpp"
#include "Landscape.hpp"

#include <QOpenGLContext>
#include <QOpenGLShaderProgram>
#include <QStringList>
#include <QSettings>
#include <QDebug>
#include <QtGlobal>

// The 0.025 corresponds to the maximum eye resolution in degree
#define EYE_RESOLUTION (0.25f)
#define MAX_LINEAR_RADIUS 8.f

StelSkyDrawer::StelSkyDrawer(StelCore* acore) :
	core(acore),
	eye(acore->getToneReproducer()),
	maxAdaptFov(180.f),
	minAdaptFov(0.1f),
	lnfovFactor(0.f),
	flagStarTwinkle(false),
	flagForcedTwinkle(false),
	twinkleAmount(0.0),
	flagDrawBigStarHalo(true),
	flagStarSpiky(false),
	flagStarMagnitudeLimit(false),
	flagNebulaMagnitudeLimit(false),
	flagPlanetMagnitudeLimit(false),
	starRelativeScale(1.),
	starAbsoluteScaleF(1.),
	starLinearScale(19.569f),
	limitMagnitude(-100.f),
	limitLuminance(0.f),
	customStarMagLimit(0.0),
	customNebulaMagLimit(0.0),
	customPlanetMagLimit(0.0),
	bortleScaleIndex(3),
	inScale(1.f),
	starShaderProgram(Q_NULLPTR),
	starShaderVars(StarShaderVars()),
	nbPointSources(0),
	maxPointSources(1000),
	maxLum(0.f),
	oldLum(-1.f),
	flagLuminanceAdaptation(false),
	daylightLabelThreshold(250.0),
	big3dModelHaloRadius(150.f)
{
	setObjectName("StelSkyDrawer");
	QSettings* conf = StelApp::getInstance().getSettings();
	initColorTableFromConfigFile(conf);

	setFlagHasAtmosphere(conf->value("landscape/flag_atmosphere", true).toBool());
	setTwinkleAmount(conf->value("stars/star_twinkle_amount",0.3).toDouble());
	setFlagTwinkle(conf->value("stars/flag_star_twinkle",true).toBool());
	setFlagForcedTwinkle(conf->value("stars/flag_forced_twinkle",false).toBool());
	setFlagDrawBigStarHalo(conf->value("stars/flag_star_halo",true).toBool());
	flagStarSpiky=(conf->value("stars/flag_star_spiky", false).toBool()); // too early to use the set method here!
	setMaxAdaptFov(conf->value("stars/mag_converter_max_fov",70.0).toFloat());
	setMinAdaptFov(conf->value("stars/mag_converter_min_fov",0.1).toFloat());
	setFlagLuminanceAdaptation(conf->value("viewing/use_luminance_adaptation",true).toBool());
	setDaylightLabelThreshold(conf->value("viewing/sky_brightness_label_threshold", 250.0).toDouble());
	setFlagStarMagnitudeLimit((conf->value("astro/flag_star_magnitude_limit", false).toBool()));
	setCustomStarMagnitudeLimit(conf->value("astro/star_magnitude_limit", 6.5).toDouble());
	setFlagPlanetMagnitudeLimit((conf->value("astro/flag_planet_magnitude_limit", false).toBool()));
	setCustomPlanetMagnitudeLimit(conf->value("astro/planet_magnitude_limit", 6.5).toDouble());
	setFlagNebulaMagnitudeLimit((conf->value("astro/flag_nebula_magnitude_limit", false).toBool()));
	setCustomNebulaMagnitudeLimit(conf->value("astro/nebula_magnitude_limit", 8.5).toDouble());

	setBortleScaleIndex(conf->value("stars/init_bortle_scale", 3).toInt());
	setRelativeStarScale(conf->value("stars/relative_scale", 1.0).toDouble());
	setAbsoluteStarScale(conf->value("stars/absolute_scale", 1.0).toDouble());
	setExtinctionCoefficient(conf->value("landscape/atmospheric_extinction_coefficient", 0.13).toDouble());

	const QString extinctionMode = conf->value("astro/extinction_mode_below_horizon", "zero").toString();
	// zero by default
	if (extinctionMode=="mirror")
		extinction.setUndergroundExtinctionMode(Extinction::UndergroundExtinctionMirror);
	else if (extinctionMode=="max")
		extinction.setUndergroundExtinctionMode(Extinction::UndergroundExtinctionMax);

	setAtmosphereTemperature(conf->value("landscape/temperature_C", 15.0).toDouble());
	setAtmospherePressure(conf->value("landscape/pressure_mbar", 1013.0).toDouble());

	// Initialize buffers for use by gl vertex array	
	
	vertexArray = new StarVertex[maxPointSources*6];
	
	textureCoordArray = new unsigned char[maxPointSources*6*2];
	for (unsigned int i=0;i<maxPointSources; ++i)
	{
		static const unsigned char texElems[] = {0, 0, 255, 0, 255, 255, 0, 0, 255, 255, 0, 255};
		unsigned char* elem = &textureCoordArray[i*6*2];
		memcpy(elem, texElems, 12);
	}
	texImgHalo=QImage(StelFileMgr::getInstallationDir()+"/textures/star16x16.png");
	texImgHaloSpiky=QImage(StelFileMgr::getInstallationDir()+"/textures/star16x16_rays.png");
}

StelSkyDrawer::~StelSkyDrawer()
{
	delete[] vertexArray;
	vertexArray = Q_NULLPTR;
	delete[] textureCoordArray;
	textureCoordArray = Q_NULLPTR;
	
	delete starShaderProgram;
	starShaderProgram = Q_NULLPTR;
}

// Init parameters from config file
void StelSkyDrawer::init()
{
	initializeOpenGLFunctions();

	// Load star texture no mipmap:
	//texHalo = StelApp::getInstance().getTextureManager().createTexture(StelFileMgr::getInstallationDir()+"/textures/star16x16.png");
	//texHaloRayed = StelApp::getInstance().getTextureManager().createTexture(StelFileMgr::getInstallationDir()+"/textures/star16x16_rays.png");
	texHalo = StelApp::getInstance().getTextureManager().createTexture(flagStarSpiky ? texImgHaloSpiky : texImgHalo);
	texBigHalo = StelApp::getInstance().getTextureManager().createTexture(StelFileMgr::getInstallationDir()+"/textures/haloLune.png");
	texSunHalo = StelApp::getInstance().getTextureManager().createTexture(StelFileMgr::getInstallationDir()+"/textures/halo.png");	
	texSunCorona = StelApp::getInstance().getTextureManager().createTexture(StelFileMgr::getInstallationDir()+"/textures/corona.png");

	// Create shader program
	QOpenGLShader vshader(QOpenGLShader::Vertex);
	const char *vsrc =
		"attribute mediump vec2 pos;\n"
		"attribute mediump vec2 texCoord;\n"
		"attribute mediump vec3 color;\n"
		"uniform mediump mat4 projectionMatrix;\n"
		"varying mediump vec2 texc;\n"
		"varying mediump vec3 outColor;\n"
		"void main(void)\n"
		"{\n"
		"    gl_Position = projectionMatrix * vec4(pos.x, pos.y, 0, 1);\n"
		"    texc = texCoord;\n"
		"    outColor = color;\n"
		"}\n";
	vshader.compileSourceCode(vsrc);
	if (!vshader.log().isEmpty()) { qWarning() << "StelSkyDrawer::init(): Warnings while compiling vshader: " << vshader.log(); }

	QOpenGLShader fshader(QOpenGLShader::Fragment);
	const char *fsrc =
		"varying mediump vec2 texc;\n"
		"varying mediump vec3 outColor;\n"
		"uniform sampler2D tex;\n"
		"void main(void)\n"
		"{\n"
		"    gl_FragColor = texture2D(tex, texc)*vec4(outColor, 1.);\n"
		"}\n";
	fshader.compileSourceCode(fsrc);
	if (!fshader.log().isEmpty()) { qWarning() << "StelSkyDrawer::init(): Warnings while compiling fshader: " << fshader.log(); }

	starShaderProgram = new QOpenGLShaderProgram(QOpenGLContext::currentContext());
	starShaderProgram->addShader(&vshader);
	starShaderProgram->addShader(&fshader);
	StelPainter::linkProg(starShaderProgram, "starShader");
	starShaderVars.projectionMatrix = starShaderProgram->uniformLocation("projectionMatrix");
	starShaderVars.texCoord = starShaderProgram->attributeLocation("texCoord");
	starShaderVars.pos = starShaderProgram->attributeLocation("pos");
	starShaderVars.color = starShaderProgram->attributeLocation("color");
	starShaderVars.texture = starShaderProgram->uniformLocation("tex");

	update(0);
}

void StelSkyDrawer::update(double)
{
	float fov = static_cast<float>(core->getMovementMgr()->getCurrentFov());
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
	static const float bortleToInScale[9] = {2.45f, 1.55f, 1.0f, 0.63f, 0.40f, 0.24f, 0.23f, 0.145f, 0.09f};
	if (getFlagHasAtmosphere() && core->getJD()>2387627.5) // JD given is J1825.0; ignore Bortle scale index before that.
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
	starLinearScale = static_cast<float>(std::pow(35.*2.0*starAbsoluteScaleF, 1.40*0.5*starRelativeScale));

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
	RCMag rcmag;
	float lim = 0.f;
	int safety=0;
	while (std::fabs(lim-a)>0.05f)
	{
		computeRCMag(lim, &rcmag);
		if (rcmag.radius<=0.f)
		{
			float tmp = lim;
			lim=(a+lim)*0.5f;
			b=tmp;
		}
		else
		{
			float tmp = lim;
			lim=(b+lim)*0.5f;
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
	while (std::fabs(lim-a)>0.05f)
	{
		adaptL = eye->adaptLuminanceScaled(lim);
		if (adaptL<=0.05f) // Object considered not visible if its adapted scaled luminance<0.05
		{
			float tmp = lim;
			lim=(b+lim)*0.5f;
			a=tmp;
		}
		else
		{
			float tmp = lim;
			lim=(a+lim)*0.5f;
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

float StelSkyDrawer::pointSourceLuminanceToMag(float lum) const
{
	return (std::log(lum) - lnfovFactor)/-0.92103f - 12.12331f;
}

// Compute the luminance for an extended source with the given surface brightness in Vmag/arcmin^2
float StelSkyDrawer::surfaceBrightnessToLuminance(float sb)
{
	return 2.f*2025000.f*std::exp(-0.92103f*(sb + 12.12331f))/(1.f/60.f*1.f/60.f);
}

// Compute the surface brightness from the luminance of an extended source
float StelSkyDrawer::luminanceToSurfacebrightness(float lum)
{
	return std::log(lum*(1.f/60.f*1.f/60.f)/(2.f*2025000.f))/-0.92103f - 12.12331f;
}

// Compute RMag and CMag from magnitude for a point source.
bool StelSkyDrawer::computeRCMag(float mag, RCMag* rcMag) const
{
	rcMag->radius = eye->adaptLuminanceScaledLn(pointSourceMagToLnLuminance(mag), static_cast<float>(starRelativeScale)*1.40f*0.5f);
	rcMag->radius *=starLinearScale;
#ifndef USE_OLD_QGLWIDGET
	rcMag->radius *=StelMainView::getInstance().getCustomScreenshotMagnification();
#endif
	// Use now statically min_rmag = 0.5, because higher and too small values look bad
	if (rcMag->radius < 0.3f)
	{
		rcMag->radius = 0.f;
		rcMag->luminance = 0.f;
		return false;
	}

	// if size of star is too small (blink) we put its size to 1.2 --> no more blink
	// And we compensate the difference of brighteness with cmag
	if (rcMag->radius<1.2f)
	{
		rcMag->luminance= rcMag->radius * rcMag->radius * rcMag->radius / 1.728f;
		if (rcMag->luminance < 0.05f)
		{
			rcMag->radius = rcMag->luminance = 0.f;
			return false;
		}
		rcMag->radius = 1.2f;
	}
	else
	{
		// cmag:
		rcMag->luminance = 1.0f;
		if (rcMag->radius>MAX_LINEAR_RADIUS)
		{
			rcMag->radius=MAX_LINEAR_RADIUS+std::sqrt(1.f+rcMag->radius-MAX_LINEAR_RADIUS)-1.f;
		}
	}
	return true;
}

void StelSkyDrawer::preDrawPointSource(StelPainter* p)
{
	Q_ASSERT(p);
	Q_ASSERT(nbPointSources==0);

	// Blending is really important. Otherwise faint stars in the vicinity of
	// bright star will cause tiny black squares on the bright star, e.g. see Procyon.
	p->setBlending(true, GL_ONE, GL_ONE);
}

// Finalize the drawing of point sources
void StelSkyDrawer::postDrawPointSource(StelPainter* sPainter)
{
	Q_ASSERT(sPainter);

	if (nbPointSources==0)
		return;
	texHalo->bind();
	sPainter->setBlending(true, GL_ONE, GL_ONE);

	const Mat4f& m = sPainter->getProjector()->getProjectionMatrix();
	const QMatrix4x4 qMat(m[0], m[4], m[8], m[12], m[1], m[5], m[9], m[13], m[2], m[6], m[10], m[14], m[3], m[7], m[11], m[15]);
	
	starShaderProgram->bind();
	starShaderProgram->setAttributeArray(starShaderVars.pos, GL_FLOAT, reinterpret_cast<GLfloat*>(vertexArray), 2, sizeof(StarVertex));
	starShaderProgram->enableAttributeArray(starShaderVars.pos);
	starShaderProgram->setAttributeArray(starShaderVars.color, GL_UNSIGNED_BYTE, reinterpret_cast<GLubyte*>(&(vertexArray[0].color)), 3, sizeof(StarVertex));
	starShaderProgram->enableAttributeArray(starShaderVars.color);
	starShaderProgram->setUniformValue(starShaderVars.projectionMatrix, qMat);
	starShaderProgram->setAttributeArray(starShaderVars.texCoord, GL_UNSIGNED_BYTE, static_cast<GLubyte*>(textureCoordArray), 2, 0);
	starShaderProgram->enableAttributeArray(starShaderVars.texCoord);
	
	glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(nbPointSources)*6);
	
	starShaderProgram->disableAttributeArray(starShaderVars.pos);
	starShaderProgram->disableAttributeArray(starShaderVars.color);
	starShaderProgram->disableAttributeArray(starShaderVars.texCoord);
	starShaderProgram->release();
	
	nbPointSources = 0;
}

// Draw a point source halo.
bool StelSkyDrawer::drawPointSource(StelPainter* sPainter, const Vec3f& v, const RCMag& rcMag, const Vec3f& color, bool checkInScreen, float twinkleFactor)
{
	Q_ASSERT(sPainter);

	if (rcMag.radius<=0.f)
		return false;

	Vec3f win;
	if (!(checkInScreen ? sPainter->getProjector()->projectCheck(v, win) : sPainter->getProjector()->project(v, win)))
		return false;

	const float radius = rcMag.radius;
	// Random coef for star twinkling. twinkleFactor can introduce height-dependent twinkling.
	const float tw = ((flagStarTwinkle && (flagHasAtmosphere || flagForcedTwinkle))) ? (1.f-twinkleFactor*static_cast<float>(twinkleAmount)*(static_cast<float>(qrand())/static_cast<float>(RAND_MAX)))*rcMag.luminance : rcMag.luminance;

	// If the rmag is big, draw a big halo
	if (flagDrawBigStarHalo && radius>MAX_LINEAR_RADIUS+5.f)
	{
		float cmag = qMin(1.0f, qMin(rcMag.luminance, (radius-(MAX_LINEAR_RADIUS+5.f))/30.f));
		float rmag = 150.f;

		texBigHalo->bind();
		sPainter->setBlending(true, GL_ONE, GL_ONE);
		sPainter->setColor(color*cmag);
		sPainter->drawSprite2dModeNoDeviceScale(win[0], win[1], rmag);
	}

	unsigned char starColor[3] = {0, 0, 0};
	starColor[0] = static_cast<unsigned char>(std::min(static_cast<int>(color[0]*tw*255+0.5f), 255));
	starColor[1] = static_cast<unsigned char>(std::min(static_cast<int>(color[1]*tw*255+0.5f), 255));
	starColor[2] = static_cast<unsigned char>(std::min(static_cast<int>(color[2]*tw*255+0.5f), 255));
	
	// Store the drawing instructions in the vertex arrays
	StarVertex* vx = &(vertexArray[nbPointSources*6]);
	vx->pos.set(win[0]-radius,win[1]-radius); memcpy(vx->color, starColor, 3); ++vx;
	vx->pos.set(win[0]+radius,win[1]-radius); memcpy(vx->color, starColor, 3); ++vx;
	vx->pos.set(win[0]+radius,win[1]+radius); memcpy(vx->color, starColor, 3); ++vx;
	vx->pos.set(win[0]-radius,win[1]-radius); memcpy(vx->color, starColor, 3); ++vx;
	vx->pos.set(win[0]+radius,win[1]+radius); memcpy(vx->color, starColor, 3); ++vx;
	vx->pos.set(win[0]-radius,win[1]+radius); memcpy(vx->color, starColor, 3); ++vx;

	++nbPointSources;
	if (nbPointSources>=maxPointSources)
	{
		// Flush the buffer (draw all buffered stars)
		postDrawPointSource(sPainter);
	}
	return true;
}

// Draw's the Sun's corona during a solar eclipse on Earth.
void StelSkyDrawer::drawSunCorona(StelPainter* painter, const Vec3f& v, float radius, const Vec3f& color, const float alpha, const float angle)
{
	texSunCorona->bind();
	painter->setBlending(true, GL_ONE, GL_ONE);

	Vec3f win;
	painter->getProjector()->project(v, win);
	// For some reason we must mix color with the given alpha as well, else mixing does not work.
	painter->setColor(color*alpha, alpha);
	// pre-compensate the automatic scaling of sprite painting on HiDPI screens
	radius /= static_cast<float>(painter->getProjector()->getDevicePixelsPerPixel())*StelApp::getInstance().getGlobalScalingRatio();
	// Our corona image was made in 2008-08-01 near Khovd, Mongolia. It shows the correct parallactic angle for its location and time, we must add this, and subtract the ecliptic/equator angle from that date of 15.43 degrees.
	painter->drawSprite2dMode(win[0], win[1], radius, -angle+44.65f-15.43f);

	postDrawPointSource(painter);
}

// Terminate drawing of a 3D model, draw the halo
void StelSkyDrawer::postDrawSky3dModel(StelPainter* painter, const Vec3f& v, float illuminatedArea, float mag, const Vec3f& color)
{
	const float pixPerRad = painter->getProjector()->getPixelPerRadAtCenter();
	// Assume a disk shape
	float pixRadius = std::sqrt(illuminatedArea/(60.f*60.f)*M_PI_180f*M_PI_180f*(pixPerRad*pixPerRad))/M_PIf;

	bool noStarHalo = false;

	if (mag<-15.f)
	{
		// Sun, halo size varies in function of the magnitude because sun as seen from pluto should look dimmer
		// as the sun as seen from earth
		texSunHalo->bind();
		painter->setBlending(true, GL_ONE, GL_ONE);

		float rmag = big3dModelHaloRadius*(mag+15.f)/-11.f;
		float cmag = (rmag>=pixRadius*3.f+100.f) ? 1.f : qMax(0.f, 1.f-(pixRadius*3.f+100-rmag)/100);
		Vec3f win;
		painter->getProjector()->project(v, win);
		painter->setColor(color*cmag);
		painter->drawSprite2dModeNoDeviceScale(win[0], win[1], rmag);
		noStarHalo = true;
	}

	// Now draw the halo according the object brightness
	const bool saveTwinkle = flagStarTwinkle;
	flagStarTwinkle = false;
	const bool saveForcedTwinkle = flagForcedTwinkle;
	flagForcedTwinkle = false;

	RCMag rcm;
	computeRCMag(mag, &rcm);

	// We now have the radius and luminosity of the small halo
	// If the disk of the planet is big enough to be visible, we should adjust the eye adaptation luminance
	// so that the radius of the halo is small enough to be not visible (so that we see the disk)

	// TODO: Change drawing halo to more realistic view of stars and planets
	float tStart = 3.f; // Was 2.f: planet's halo is too dim. Atque 2020-11-12: No need to change these anymore. It appears that this has to do with halo size vs FOV (?).
	float tStop = 6.f;
	bool truncated=false;

	float maxHaloRadius = qMax(tStart*6.f, pixRadius*3.f); //Atque 2020-11-12: Careful, if tStart*6.f is too big (tStart*10.f or something), the Moon gets a ridiculously big halo.
	if (rcm.radius>maxHaloRadius)
	{
		truncated = true;
		rcm.radius=maxHaloRadius+std::sqrt(rcm.radius-maxHaloRadius);
	}

	// Fade the halo away when the disk is too big
	if (pixRadius>=tStop)
	{
		rcm.luminance=0.f;
	}
	if (pixRadius>tStart && pixRadius<tStop)
	{
		rcm.luminance=(tStop-pixRadius)/(tStop-tStart);
	}

	if (truncated && flagLuminanceAdaptation)
	{
		float wl = findWorldLumForMag(mag, rcm.radius);
		if (wl>0)
		{
			const float f = static_cast<float>(core->getMovementMgr()->getCurrentFov());
			// Report to the SkyDrawer that a very bright object (most notably Sun, Moon, bright planet)
			// is in view. LP:1138533 correctly wants no such effect if object is hidden by landscape horizon.
			LandscapeMgr* lmgr=GETSTELMODULE(LandscapeMgr);
			Q_ASSERT(lmgr);
			Landscape *landscape=lmgr->getCurrentLandscape();
			Q_ASSERT(landscape);
			// Preliminary: create new Vec3d here. Later: consider replacing to vec3d in the arguments!
			Vec3d vec=v.toVec3d();
			float opacity=(landscape->getFlagShow() ? landscape->getOpacity(core->j2000ToAltAz(vec, StelCore::RefractionAuto)) : 0.0f);
			reportLuminanceInFov(qMin(700.f, qMin(wl/50, (60.f*60.f)/(f*f)*6.f))*(1.0f-opacity));
		}
	}

	if (!noStarHalo)
	{
		preDrawPointSource(painter);
		drawPointSource(painter, v, rcm, color);
		postDrawPointSource(painter);
	}
	flagStarTwinkle=saveTwinkle;
	flagForcedTwinkle=saveForcedTwinkle;
}

float StelSkyDrawer::findWorldLumForMag(float mag, float targetRadius) const
{
	const float saveLum = eye->getWorldAdaptationLuminance();	// save

	// Compute the luminance by dichotomy
	float a=0.001f;
	float b=500000.f;
	RCMag rcmag;
	rcmag.radius=-99;
	float curLum = 500.f;
	int safety=0;
	while (std::fabs(rcmag.radius-targetRadius)>0.1f)
	{
		eye->setWorldAdaptationLuminance(curLum);
		computeRCMag(mag, &rcmag);
		if (rcmag.radius<=targetRadius)
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


// Set the parameters so that the stars disappear at about the limit given by the bortle scale
// See http://en.wikipedia.org/wiki/Bortle_Dark-Sky_Scale
void StelSkyDrawer::setBortleScaleIndex(int bIndex)
{
	if(bortleScaleIndex!=bIndex)
	{
		// Associate the Bortle index (1 to 9) to inScale value
		if ((bIndex<1) || (bIndex>9))
		{
			qWarning() << "WARNING: Bortle scale index range is [1;9], given" << bIndex;
		}
		bortleScaleIndex = qBound(1, bIndex, 9);
		emit bortleScaleIndexChanged(bortleScaleIndex);
		// GZ: I moved this block to update()
		// These value have been calibrated by hand, looking at the faintest star in stellarium at around 40 deg FOV
		// They should roughly match the scale described at http://en.wikipedia.org/wiki/Bortle_Dark-Sky_Scale
		// static const float bortleToInScale[9] = {2.45, 1.55, 1.0, 0.63, 0.40, 0.24, 0.23, 0.145, 0.09};
		// setInputScale(bortleToInScale[bIndex-1]);
	}
}

void StelSkyDrawer::setFlagStarSpiky(bool b)
{
	if (b!=flagStarSpiky)
	{
		flagStarSpiky=b;
		texHalo = StelApp::getInstance().getTextureManager().createTexture(flagStarSpiky ? texImgHaloSpiky : texImgHalo);
		emit flagStarSpikyChanged(flagStarSpiky);
	}
}

float StelSkyDrawer::getNELMFromBortleScale() const
{
	return getNELMFromBortleScale(getBortleScaleIndex());
}

float StelSkyDrawer::getNELMFromBortleScale(int idx)
{
	const float nelms[9] = {7.8f, 7.3f, 6.8f, 6.3f, 5.8f, 5.3f, 4.8f, 4.3f, 4.0f};
	idx = qBound(1, idx, 9);
	return nelms[idx-1];
}

// colors for B-V display
Vec3f StelSkyDrawer::colorTable[128] = {
	Vec3f(0.602745f,0.713725f,1.000000f),
	Vec3f(0.604902f,0.715294f,1.000000f),
	Vec3f(0.607059f,0.716863f,1.000000f),
	Vec3f(0.609215f,0.718431f,1.000000f),
	Vec3f(0.611372f,0.720000f,1.000000f),
	Vec3f(0.613529f,0.721569f,1.000000f),
	Vec3f(0.635490f,0.737255f,1.000000f),
	Vec3f(0.651059f,0.749673f,1.000000f),
	Vec3f(0.666627f,0.762092f,1.000000f),
	Vec3f(0.682196f,0.774510f,1.000000f),
	Vec3f(0.697764f,0.786929f,1.000000f),
	Vec3f(0.713333f,0.799347f,1.000000f),
	Vec3f(0.730306f,0.811242f,1.000000f),
	Vec3f(0.747278f,0.823138f,1.000000f),
	Vec3f(0.764251f,0.835033f,1.000000f),
	Vec3f(0.781223f,0.846929f,1.000000f),
	Vec3f(0.798196f,0.858824f,1.000000f),
	Vec3f(0.812282f,0.868236f,1.000000f),
	Vec3f(0.826368f,0.877647f,1.000000f),
	Vec3f(0.840455f,0.887059f,1.000000f),
	Vec3f(0.854541f,0.896470f,1.000000f),
	Vec3f(0.868627f,0.905882f,1.000000f),
	Vec3f(0.884627f,0.916862f,1.000000f),
	Vec3f(0.900627f,0.927843f,1.000000f),
	Vec3f(0.916627f,0.938823f,1.000000f),
	Vec3f(0.932627f,0.949804f,1.000000f),
	Vec3f(0.948627f,0.960784f,1.000000f),
	Vec3f(0.964444f,0.972549f,1.000000f),
	Vec3f(0.980261f,0.984313f,1.000000f),
	Vec3f(0.996078f,0.996078f,1.000000f),
	Vec3f(1.000000f,1.000000f,1.000000f),
	Vec3f(1.000000f,0.999643f,0.999287f),
	Vec3f(1.000000f,0.999287f,0.998574f),
	Vec3f(1.000000f,0.998930f,0.997861f),
	Vec3f(1.000000f,0.998574f,0.997148f),
	Vec3f(1.000000f,0.998217f,0.996435f),
	Vec3f(1.000000f,0.997861f,0.995722f),
	Vec3f(1.000000f,0.997504f,0.995009f),
	Vec3f(1.000000f,0.997148f,0.994296f),
	Vec3f(1.000000f,0.996791f,0.993583f),
	Vec3f(1.000000f,0.996435f,0.992870f),
	Vec3f(1.000000f,0.996078f,0.992157f),
	Vec3f(1.000000f,0.991140f,0.981554f),
	Vec3f(1.000000f,0.986201f,0.970951f),
	Vec3f(1.000000f,0.981263f,0.960349f),
	Vec3f(1.000000f,0.976325f,0.949746f),
	Vec3f(1.000000f,0.971387f,0.939143f),
	Vec3f(1.000000f,0.966448f,0.928540f),
	Vec3f(1.000000f,0.961510f,0.917938f),
	Vec3f(1.000000f,0.956572f,0.907335f),
	Vec3f(1.000000f,0.951634f,0.896732f),
	Vec3f(1.000000f,0.946695f,0.886129f),
	Vec3f(1.000000f,0.941757f,0.875526f),
	Vec3f(1.000000f,0.936819f,0.864924f),
	Vec3f(1.000000f,0.931881f,0.854321f),
	Vec3f(1.000000f,0.926942f,0.843718f),
	Vec3f(1.000000f,0.922004f,0.833115f),
	Vec3f(1.000000f,0.917066f,0.822513f),
	Vec3f(1.000000f,0.912128f,0.811910f),
	Vec3f(1.000000f,0.907189f,0.801307f),
	Vec3f(1.000000f,0.902251f,0.790704f),
	Vec3f(1.000000f,0.897313f,0.780101f),
	Vec3f(1.000000f,0.892375f,0.769499f),
	Vec3f(1.000000f,0.887436f,0.758896f),
	Vec3f(1.000000f,0.882498f,0.748293f),
	Vec3f(1.000000f,0.877560f,0.737690f),
	Vec3f(1.000000f,0.872622f,0.727088f),
	Vec3f(1.000000f,0.867683f,0.716485f),
	Vec3f(1.000000f,0.862745f,0.705882f),
	Vec3f(1.000000f,0.858617f,0.695975f),
	Vec3f(1.000000f,0.854490f,0.686068f),
	Vec3f(1.000000f,0.850362f,0.676161f),
	Vec3f(1.000000f,0.846234f,0.666254f),
	Vec3f(1.000000f,0.842107f,0.656346f),
	Vec3f(1.000000f,0.837979f,0.646439f),
	Vec3f(1.000000f,0.833851f,0.636532f),
	Vec3f(1.000000f,0.829724f,0.626625f),
	Vec3f(1.000000f,0.825596f,0.616718f),
	Vec3f(1.000000f,0.821468f,0.606811f),
	Vec3f(1.000000f,0.817340f,0.596904f),
	Vec3f(1.000000f,0.813213f,0.586997f),
	Vec3f(1.000000f,0.809085f,0.577090f),
	Vec3f(1.000000f,0.804957f,0.567183f),
	Vec3f(1.000000f,0.800830f,0.557275f),
	Vec3f(1.000000f,0.796702f,0.547368f),
	Vec3f(1.000000f,0.792574f,0.537461f),
	Vec3f(1.000000f,0.788447f,0.527554f),
	Vec3f(1.000000f,0.784319f,0.517647f),
	Vec3f(1.000000f,0.784025f,0.520882f),
	Vec3f(1.000000f,0.783731f,0.524118f),
	Vec3f(1.000000f,0.783436f,0.527353f),
	Vec3f(1.000000f,0.783142f,0.530588f),
	Vec3f(1.000000f,0.782848f,0.533824f),
	Vec3f(1.000000f,0.782554f,0.537059f),
	Vec3f(1.000000f,0.782259f,0.540294f),
	Vec3f(1.000000f,0.781965f,0.543529f),
	Vec3f(1.000000f,0.781671f,0.546765f),
	Vec3f(1.000000f,0.781377f,0.550000f),
	Vec3f(1.000000f,0.781082f,0.553235f),
	Vec3f(1.000000f,0.780788f,0.556471f),
	Vec3f(1.000000f,0.780494f,0.559706f),
	Vec3f(1.000000f,0.780200f,0.562941f),
	Vec3f(1.000000f,0.779905f,0.566177f),
	Vec3f(1.000000f,0.779611f,0.569412f),
	Vec3f(1.000000f,0.779317f,0.572647f),
	Vec3f(1.000000f,0.779023f,0.575882f),
	Vec3f(1.000000f,0.778728f,0.579118f),
	Vec3f(1.000000f,0.778434f,0.582353f),
	Vec3f(1.000000f,0.778140f,0.585588f),
	Vec3f(1.000000f,0.777846f,0.588824f),
	Vec3f(1.000000f,0.777551f,0.592059f),
	Vec3f(1.000000f,0.777257f,0.595294f),
	Vec3f(1.000000f,0.776963f,0.598530f),
	Vec3f(1.000000f,0.776669f,0.601765f),
	Vec3f(1.000000f,0.776374f,0.605000f),
	Vec3f(1.000000f,0.776080f,0.608235f),
	Vec3f(1.000000f,0.775786f,0.611471f),
	Vec3f(1.000000f,0.775492f,0.614706f),
	Vec3f(1.000000f,0.775197f,0.617941f),
	Vec3f(1.000000f,0.774903f,0.621177f),
	Vec3f(1.000000f,0.774609f,0.624412f),
	Vec3f(1.000000f,0.774315f,0.627647f),
	Vec3f(1.000000f,0.774020f,0.630883f),
	Vec3f(1.000000f,0.773726f,0.634118f),
	Vec3f(1.000000f,0.773432f,0.637353f),
	Vec3f(1.000000f,0.773138f,0.640588f),
	Vec3f(1.000000f,0.772843f,0.643824f),
	Vec3f(1.000000f,0.772549f,0.647059f),
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
	for (float bV=-0.5f;bV<=4.0f;bV+=0.01f)
	{
		char entry[256];
		sprintf(entry,"bv_color_%+5.2f",bV);
		const QStringList s(conf->value(QString("stars/") + entry).toStringList());
		if (!s.isEmpty())
		{
			Vec3f c;
			if (s.size()==1)
				c = Vec3f(s[0]);
			else
				c =Vec3f(s);
			color_map[bV] = Gamma(eye->getDisplayGamma(),c);
		}
	}

	if (color_map.size() > 1)
	{
		for (unsigned char i=0;i<128;i++)
		{
			const float bV = StelSkyDrawer::indexToBV(i);
			auto greater = color_map.upper_bound(bV);
			if (greater == color_map.begin())
			{
				colorTable[i] = greater->second;
			}
			else
			{
				auto less = greater;--less;
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

double StelSkyDrawer::getWorldAdaptationLuminance() const
{
	return static_cast<double>(eye->getWorldAdaptationLuminance());
}

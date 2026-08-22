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
#include "StelMainView.hpp"
#include "precession.h"

#include "StelModuleMgr.hpp"
#include "LandscapeMgr.hpp"
#include "Landscape.hpp"

#include <QOpenGLContext>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QStringList>
#include <QSettings>
#include <QDebug>
#include <QtGlobal>
#include <algorithm>
#include <cmath>

// The 0.025 corresponds to the maximum eye resolution in degree
#define EYE_RESOLUTION (0.25f)
#define MAX_LINEAR_RADIUS 8.f
// Keep very bright point-source PSF halos under control. This preserves contrast
// for objects such as Venus and bright supernovae while letting the full Moon
// stand out without using its full integrated magnitude.
#define DEFAULT_PSF_BRIGHT_SOURCE_MAG_LIMIT (-8.5f)

static float psfSmoothStep(float edge0, float edge1, float x)
{
	const float t = qBound(0.f, (x - edge0) / (edge1 - edge0), 1.f);
	return t * t * (3.f - 2.f * t);
}

StelSkyDrawer::StelSkyDrawer(StelCore* acore) :
	core(acore),
	eye(acore->getToneReproducer()),
	vao(new QOpenGLVertexArrayObject),
	psfVao(new QOpenGLVertexArrayObject),
	vbo(new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer)),
	maxAdaptFov(180.f),
	minAdaptFov(0.1f),
	lnfovFactor(0.f),
	flagStarTwinkle(false),
	flagForcedTwinkle(false),
	twinkleAmount(0.0),
	flagDrawBigStarHalo(true),
	flagStarSpiky(false),
	flagPsfStars(false),
	flagPsfStarProjectionCorrection(true),
	psfStarPointRadius(1.5f),
	psfStarFlareDecay(0.1f),
	psfStarFlareStrength(1.f),
	psfStarBrightSourceMagLimit(DEFAULT_PSF_BRIGHT_SOURCE_MAG_LIMIT),
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
	inScale(1.f),
	starShaderProgram(Q_NULLPTR),
	psfPointShaderProgram(Q_NULLPTR),
	psfGlowShaderProgram(Q_NULLPTR),
	starShaderVars(StarShaderVars()),
	psfPointShaderVars(PsfStarShaderVars()),
	psfGlowShaderVars(PsfStarShaderVars()),
	psfShaderProjector(),
	nbPointSources(0),
	psfPointVertices(),
	psfGlowVertices(),
	maxLum(0.f),
	oldLum(-1.f),
	coronaMeshDim(5), // low odd number: 3/5/7
	coronaTextureCoords(),
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
	setFlagPsfStars(conf->value("stars/flag_psf_stars", false).toBool());
	setFlagPsfStarProjectionCorrection(conf->value("stars/flag_psf_projection_correction", false).toBool());
	setPsfStarPointRadius(conf->value("stars/psf_star_point_radius", 1.5).toDouble());
	setPsfStarFlareDecay(conf->value("stars/psf_star_flare_decay", conf->value("stars/psf_star_optimization", 0.1)).toDouble());
	setPsfStarFlareStrength(conf->value("stars/psf_star_flare_strength", 1.0).toDouble());
	setPsfStarBrightSourceMagLimit(conf->value("stars/psf_star_bright_source_mag_limit", DEFAULT_PSF_BRIGHT_SOURCE_MAG_LIMIT).toDouble());
	setMaxAdaptFov(conf->value("stars/mag_converter_max_fov",70.0).toFloat());
	setMinAdaptFov(conf->value("stars/mag_converter_min_fov",0.1).toFloat());
	setFlagLuminanceAdaptation(conf->value("viewing/use_luminance_adaptation",true).toBool());
	setDaylightLabelThreshold(conf->value("viewing/sky_brightness_label_threshold", 250.0).toDouble());
	setFlagStarMagnitudeLimit(conf->value("astro/flag_star_magnitude_limit", false).toBool());
	setCustomStarMagnitudeLimit(conf->value("astro/star_magnitude_limit", 6.5).toDouble());
	setFlagPlanetMagnitudeLimit(conf->value("astro/flag_planet_magnitude_limit", false).toBool());
	setCustomPlanetMagnitudeLimit(conf->value("astro/planet_magnitude_limit", 6.5).toDouble());
	setFlagNebulaMagnitudeLimit(conf->value("astro/flag_nebula_magnitude_limit", false).toBool());
	setCustomNebulaMagnitudeLimit(conf->value("astro/nebula_magnitude_limit", 8.5).toDouble());

	setLightPollutionLuminance(conf->value("stars/init_light_pollution_luminance", StelCore::bortleScaleIndexToLuminance(3)).toFloat());
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

	// four extras for finetuning
	setFlagDrawSunAfterAtmosphere(conf->value("landscape/draw_sun_after_atmosphere",false).toBool());
	setFlagEarlySunHalo(conf->value("landscape/early_solar_halo",false).toBool());
	setFlagTfromK(conf->value("landscape/use_T_from_k",false).toBool());
	setT(conf->value("landscape/turbidity",5.).toDouble());

	// Initialize buffers for use by gl vertex array
	vertexArray = new StarVertex[maxPointSources*6];
	
	textureCoordArray = new unsigned char[maxPointSources*6*2];
	for (unsigned int i=0;i<maxPointSources; ++i)
	{
		static const unsigned char texElems[] = {0, 0, 255, 0, 255, 255, 0, 0, 255, 255, 0, 255};
		unsigned char* elem = &textureCoordArray[i*6*2];
		std::memcpy(elem, texElems, 12);
	}
	texImgHalo=QImage(StelFileMgr::getInstallationDir()+"/textures/star16x16.png");
	texImgHaloSpiky=QImage(StelFileMgr::getInstallationDir()+"/textures/star16x16_rays.png");

	// Tessellate texture into an equispaced 5x5 field. Vertices have to be computed per frame.
	for (int j=0;j<coronaMeshDim-1;++j)
	{
		for (int i=0;i<coronaMeshDim-1;++i)
		{
			coronaTextureCoords << Vec2f((float(i))/(coronaMeshDim-1),     (float(j))/(coronaMeshDim-1));
			coronaTextureCoords << Vec2f((float(i)+1.f)/(coronaMeshDim-1), (float(j))/(coronaMeshDim-1));
			coronaTextureCoords << Vec2f((float(i))/(coronaMeshDim-1),     (float(j)+1.f)/(coronaMeshDim-1));
			coronaTextureCoords << Vec2f((float(i)+1.f)/(coronaMeshDim-1), (float(j))/(coronaMeshDim-1));
			coronaTextureCoords << Vec2f((float(i)+1.f)/(coronaMeshDim-1), (float(j)+1.f)/(coronaMeshDim-1));
			coronaTextureCoords << Vec2f((float(i))/(coronaMeshDim-1),     (float(j)+1.f)/(coronaMeshDim-1));
		}
	}
}

StelSkyDrawer::~StelSkyDrawer()
{
	delete[] vertexArray;
	vertexArray = Q_NULLPTR;
	delete[] textureCoordArray;
	textureCoordArray = Q_NULLPTR;
	
	delete starShaderProgram;
	starShaderProgram = Q_NULLPTR;
	delete psfPointShaderProgram;
	psfPointShaderProgram = Q_NULLPTR;
	delete psfGlowShaderProgram;
	psfGlowShaderProgram = Q_NULLPTR;
	psfShaderProjector.clear();
}

// Init parameters from config file
void StelSkyDrawer::init()
{
	initializeOpenGLFunctions();

	// Load star texture no mipmap:
	texHalo = StelApp::getInstance().getTextureManager().createTexture(texImgHalo);
	texHaloRayed = StelApp::getInstance().getTextureManager().createTexture(texImgHaloSpiky);
	texBigHalo = StelApp::getInstance().getTextureManager().createTexture(StelFileMgr::getInstallationDir()+"/textures/haloLune.png");
	texSunHalo = StelApp::getInstance().getTextureManager().createTexture(StelFileMgr::getInstallationDir()+"/textures/halo.png");	
	texSunCorona = StelApp::getInstance().getTextureManager().createTexture(StelFileMgr::getInstallationDir()+"/textures/corona.png");

	// Create shader program
	QOpenGLShader vshader(QOpenGLShader::Vertex);
	const char *vsrc =
		"ATTRIBUTE mediump vec2 pos;\n"
		"ATTRIBUTE mediump vec2 texCoord;\n"
		"ATTRIBUTE mediump vec3 color;\n"
		"uniform mediump mat4 projectionMatrix;\n"
		"VARYING mediump vec2 texc;\n"
		"VARYING mediump vec3 outColor;\n"
		"void main(void)\n"
		"{\n"
		"    gl_Position = projectionMatrix * vec4(pos.x, pos.y, 0, 1);\n"
		"    texc = texCoord;\n"
		"    outColor = color;\n"
		"}\n";
	vshader.compileSourceCode(StelOpenGL::globalShaderPrefix(StelOpenGL::VERTEX_SHADER) + vsrc);
	if (!vshader.log().isEmpty()) { qWarning() << "StelSkyDrawer::init(): Warnings while compiling vshader: " << vshader.log(); }

	QOpenGLShader fshader(QOpenGLShader::Fragment);
	const char *fsrc =
		"VARYING mediump vec2 texc;\n"
		"VARYING mediump vec3 outColor;\n"
		"uniform sampler2D tex;\n"
		"void main(void)\n"
		"{\n"
		"    FRAG_COLOR = texture2D(tex, texc)*vec4(outColor, 1.);\n"
		"}\n";
	fshader.compileSourceCode(StelOpenGL::globalShaderPrefix(StelOpenGL::FRAGMENT_SHADER) + fsrc);
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

	vbo->create();
	vbo->bind();
	vbo->setUsagePattern(QOpenGLBuffer::StreamDraw);
	vbo->allocate(qMax(maxPointSources*6*sizeof(StarVertex) + maxPointSources*6*2,
	                   maxPointSources*6*sizeof(PsfStarVertex)));
	psfPointVertices.reserve(maxPointSources*6);
	psfGlowVertices.reserve(maxPointSources*6);

	if(vao->create())
	{
		vao->bind();
		setupCurrentVAO();
		vao->release();
	}
	psfVao->create();

	vbo->release();

	update(0);
}

void StelSkyDrawer::setupCurrentVAO()
{
	vbo->bind();
	starShaderProgram->setAttributeBuffer(starShaderVars.pos, GL_FLOAT, 0, 2, sizeof(StarVertex));
	starShaderProgram->setAttributeBuffer(starShaderVars.color, GL_UNSIGNED_BYTE, offsetof(StarVertex,color), 3, sizeof(StarVertex));
	starShaderProgram->setAttributeBuffer(starShaderVars.texCoord, GL_UNSIGNED_BYTE, maxPointSources*6*sizeof(StarVertex), 2, 0);
	vbo->release();
	starShaderProgram->enableAttributeArray(starShaderVars.pos);
	starShaderProgram->enableAttributeArray(starShaderVars.color);
	starShaderProgram->enableAttributeArray(starShaderVars.texCoord);
}

void StelSkyDrawer::bindVAO()
{
	if(vao->isCreated())
		vao->bind();
	else
		setupCurrentVAO();
}

void StelSkyDrawer::releaseVAO()
{
	if(vao->isCreated())
	{
		vao->release();
	}
	else
	{
		starShaderProgram->disableAttributeArray(starShaderVars.pos);
		starShaderProgram->disableAttributeArray(starShaderVars.color);
		starShaderProgram->disableAttributeArray(starShaderVars.texCoord);
	}
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

	if (getFlagHasAtmosphere() && core->getJD()>2387627.5) // JD given is J1825.0; ignore Bortle scale index before that.
	{
        // GZ: Light pollution must take global atmosphere setting into account!
        // moved parts from setBortleScale() here
        // This formula is a fit to a set of values calibrated by hand, looking at the faintest star in stellarium at around 40 deg FOV.
        // It should roughly match the scale described at http://en.wikipedia.org/wiki/Bortle_Dark-Sky_Scale
		const auto nelm = StelCore::luminanceToNELM(lightPollutionLuminance);
		setInputScale(3.3541f*std::exp(-0.404f*(16.5f-2*nelm)));
	}
	else
	    setInputScale(2.45f);

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
		float tmp = lim;
		if (rcmag.radius<=0.f)
		{
			lim=(a+lim)*0.5f;
			b=tmp;
		}
		else
		{
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
	while (std::fabs(lim-a)>0.05f)
	{
		float adaptL = eye->adaptLuminanceScaled(lim);
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
	rcMag->radius *= StelApp::getInstance().getScreenScale();
	return true;
}

bool StelSkyDrawer::computePsfRCMag(float mag, RCMag* rcMag) const
{
	float peakRadiance = 0.f;
	if (!computePsfPeakRadiance(mag, &peakRadiance))
	{
		rcMag->radius = 0.f;
		rcMag->luminance = 0.f;
		return false;
	}

	rcMag->radius = psfStarPointRadius * StelApp::getInstance().getScreenScale();
	rcMag->luminance = 1.f;
	return true;
}

void StelSkyDrawer::preDrawPointSource(StelPainter* p)
{
	Q_ASSERT(p);
	Q_ASSERT(nbPointSources==0);
	Q_ASSERT(psfPointVertices.isEmpty());
	Q_ASSERT(psfGlowVertices.isEmpty());

	// Blending is really important. Otherwise faint stars in the vicinity of
	// bright star will cause tiny black squares on the bright star, e.g. see Procyon.
	p->setBlending(true, GL_ONE, GL_ONE);
}

// Finalize the drawing of point sources
void StelSkyDrawer::postDrawPointSource(StelPainter* sPainter, bool drawInCorona)
{
	Q_ASSERT(sPainter);

	if (nbPointSources==0 && psfPointVertices.isEmpty() && psfGlowVertices.isEmpty())
		return;
	if (drawInCorona)
	{
		// Test whether this call is misplaced. TODO: Cleanup in summer of 2026.
		Q_ASSERT(0);
	}
	if (flagStarSpiky)
		texHaloRayed->bind();
	else
		texHalo->bind();
	sPainter->setBlending(true, GL_ONE, GL_ONE);

	flushPsfPointSources(sPainter);
	if (nbPointSources==0)
		return;

	const QMatrix4x4 qMat=sPainter->getProjector()->getProjectionMatrix().toQMatrix();

	vbo->bind();
	vbo->write(0, vertexArray, nbPointSources*6*sizeof(StarVertex));
	vbo->write(maxPointSources*6*sizeof(StarVertex), textureCoordArray, nbPointSources*6*2);
	vbo->release();

	starShaderProgram->bind();
	starShaderProgram->setUniformValue(starShaderVars.projectionMatrix, qMat);
	
	bindVAO();
	glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(nbPointSources)*6);
	releaseVAO();

	starShaderProgram->release();
	
	nbPointSources = 0;
}

// Draw a point source halo.
bool StelSkyDrawer::drawPointSource(StelPainter* sPainter, const Vec3d& v, const RCMag& rcMag, const Vec3f& color, bool checkInScreen, float twinkleFactor, float appMag)
{
	Q_ASSERT(sPainter);

	if (rcMag.radius<=0.f)
		return false;

	Vec3d win;
	if (!(checkInScreen ? sPainter->getProjector()->projectCheck(v, win) : sPainter->getProjector()->project(v, win)))
		return false;

	if (flagPsfStars && std::isfinite(appMag))
	{
		if (rcMag.luminance <= 0.f)
			return false;
		drawPsfPointSource(sPainter, v, Vec3f(static_cast<float>(win[0]), static_cast<float>(win[1]), static_cast<float>(win[2])), appMag, color, twinkleFactor, rcMag.luminance);
		return true;
	}

	const float radius = rcMag.radius;
	const float frand=StelApp::getInstance().getRandF();

	// Random coef for star twinkling. twinkleFactor can introduce height-dependent twinkling.
	const float tw = ((flagStarTwinkle && (flagHasAtmosphere || flagForcedTwinkle))) ? (1.f-twinkleFactor*static_cast<float>(twinkleAmount)*frand)*rcMag.luminance : rcMag.luminance;

	const float scale = StelApp::getInstance().getScreenScale();
	// If the rmag is big, draw a big halo
	const float bigHaloThresholdRadius = (MAX_LINEAR_RADIUS+5)*scale;
	if (flagDrawBigStarHalo && radius > bigHaloThresholdRadius)
	{
		const float cmag = qMin(1.0f, qMin(rcMag.luminance, (radius-bigHaloThresholdRadius)/30.f/scale));
		const float rmag = 150.f * scale;

		texBigHalo->bind();
		sPainter->setBlending(true, GL_ONE, GL_ONE);
		sPainter->setColor(color*cmag);
		sPainter->drawSprite2dModeNoDeviceScale(win[0], win[1], rmag);
	}

	unsigned char starColor[3] = {
		static_cast<unsigned char>(std::min(static_cast<int>(color[0]*tw*255+0.5f), 255)),
		static_cast<unsigned char>(std::min(static_cast<int>(color[1]*tw*255+0.5f), 255)),
		static_cast<unsigned char>(std::min(static_cast<int>(color[2]*tw*255+0.5f), 255))};
	
	// Store the drawing instructions in the vertex arrays
	StarVertex* vx = &(vertexArray[nbPointSources*6]);
	vx->pos.set(win[0]-radius,win[1]-radius); std::memcpy(vx->color, starColor, 3); ++vx;
	vx->pos.set(win[0]+radius,win[1]-radius); std::memcpy(vx->color, starColor, 3); ++vx;
	vx->pos.set(win[0]+radius,win[1]+radius); std::memcpy(vx->color, starColor, 3); ++vx;
	vx->pos.set(win[0]-radius,win[1]-radius); std::memcpy(vx->color, starColor, 3); ++vx;
	vx->pos.set(win[0]+radius,win[1]+radius); std::memcpy(vx->color, starColor, 3); ++vx;
	vx->pos.set(win[0]-radius,win[1]+radius); std::memcpy(vx->color, starColor, 3); ++vx;

	++nbPointSources;
	if (nbPointSources>=maxPointSources)
	{
		// Flush the buffer (draw all buffered stars)
		postDrawPointSource(sPainter);
	}
	return true;
}

bool StelSkyDrawer::computePsfPeakRadiance(float mag, float* peakRadiance) const
{
	const float r = qMax(psfStarPointRadius, 1.0e-3f);
	RCMag legacyRCMag;
	if (!computeRCMag(mag, &legacyRCMag))
		return false;

	const float screenScale = qMax(StelApp::getInstance().getScreenScale(), 1.0e-3f);
	const float legacyRadius = legacyRCMag.radius / screenScale;
	float legacyFlux = legacyRCMag.luminance * legacyRadius * legacyRadius;
	if (!std::isfinite(legacyFlux) || legacyFlux <= 0.f)
		return false;

	const float brightSourceBlend = psfSmoothStep(0.f, 13.f, -mag);
	if (brightSourceBlend > 0.f)
	{
		const float cappedMag = qMax(mag, psfStarBrightSourceMagLimit);
		const float rawRadius = eye->adaptLuminanceScaledLn(pointSourceMagToLnLuminance(cappedMag), static_cast<float>(starRelativeScale)*1.40f*0.5f) * starLinearScale;
		const float rawFlux = rawRadius * rawRadius;
		if (std::isfinite(rawFlux) && rawFlux > legacyFlux)
			legacyFlux += (rawFlux - legacyFlux) * brightSourceBlend;
	}

	float peak = 3.f * legacyFlux / (M_PIf * r * r);
	const float dimGate = 1.f / (255.f * 12.92f);
	if (peak <= dimGate)
		return false;

	peak = std::sqrt(peak * peak - dimGate * dimGate);
	*peakRadiance = peak;
	return true;
}

float StelSkyDrawer::getPsfPointSourceLabelOffset(const RCMag& rcMag, float appMag, const Vec3f& color, float baseOffset, float psfOffsetScale) const
{
	if (!flagPsfStars || !std::isfinite(appMag) || rcMag.luminance <= 0.f)
		return baseOffset;

	float peakRadiance = 0.f;
	if (!computePsfPeakRadiance(appMag, &peakRadiance))
		return baseOffset;
	peakRadiance *= rcMag.luminance;

	float greenScale = 1.f;
	psfGreenNormalization(color, 0.1f, greenScale);
	const float peakRadianceColor = peakRadiance * greenScale;
	float radius = psfStarPointRadius;
	const float flareOnset = psfSmoothStep(0.5f, 2.5f, peakRadianceColor);
	const float effectiveFlareStrength = psfStarFlareStrength * flareOnset;
	if (effectiveFlareStrength > 0.f && psfStarFlareDecay > 0.f)
		radius = qMax(radius, computePsfGlowRadius(peakRadianceColor, effectiveFlareStrength));

	const float scale = StelApp::getInstance().getScreenScale();
	const float psfOffset = qMin((radius * 0.45f + 6.f) * scale * psfOffsetScale, 96.f * scale);
	return qMax(baseOffset, psfOffset);
}

// The PSF glow approximation and its optimization parameter are adapted from
// Askaniy Anpilogov's Python prototype for point source rendering.
float StelSkyDrawer::computePsfGlowRadius(float peakRadiance, float alpha) const
{
	const float r = qMax(psfStarPointRadius, 1.0e-3f);
	const float a = psfStarFlareDecay / r;
	if (a <= 0.f || peakRadiance <= 0.f || alpha <= 0.f)
		return 0.f;

	const float denom = M_PIf / r - a;
	if (denom <= 0.f)
		return 0.f;

	const float b = 1.f / denom;
	const float p04 = std::pow(peakRadiance, 0.4f);
	const float rFull = p04 / a;
	const float minVisibleRadiance = 1.f / (255.f * 12.92f);
	const float tVal = minVisibleRadiance / qMax(alpha, 1.0e-3f);
	const float rEff = p04 / (a + std::pow(tVal, 0.4f) / b);
	return qMin(rFull, rEff);
}

Vec3f StelSkyDrawer::psfGreenNormalization(const Vec3f& c, float saturationLimit, float& greenScale) const
{
	float r = c[0];
	float g = c[1];
	float b = c[2];
	const float mx = std::max({r, g, b});
	if (mx <= 0.f)
	{
		greenScale = 1.f;
		return c;
	}

	r /= mx;
	g /= mx;
	b /= mx;

	const float mn = std::min({r, g, b});
	if (const float delta = saturationLimit - mn; delta > 0.f)
	{
		const float dr = 1.f - r;
		const float dg = 1.f - g;
		const float db = 1.f - b;
		r = qMin(1.f, r + delta * dr * dr);
		g = qMin(1.f, g + delta * dg * dg);
		b = qMin(1.f, b + delta * db * db);
	}

	greenScale = (g > 0.f) ? (1.f / g) : 1.f;
	return Vec3f(r, g, b);
}

void StelSkyDrawer::addPsfStarVertices(QVector<PsfStarVertex>& vertices, StelPainter* sPainter, const Vec3d& direction, const Vec3f& center, const Vec3f& color, float peakRadiance, float radius)
{
	static const Vec2f corners[] = {
		Vec2f(-1.f, -1.f), Vec2f( 1.f, -1.f), Vec2f( 1.f,  1.f),
		Vec2f(-1.f, -1.f), Vec2f( 1.f,  1.f), Vec2f(-1.f,  1.f)
	};
	unsigned char starColor[4] = {
		static_cast<unsigned char>(qBound(0, static_cast<int>(color[0]*255.f+0.5f), 255)),
		static_cast<unsigned char>(qBound(0, static_cast<int>(color[1]*255.f+0.5f), 255)),
		static_cast<unsigned char>(qBound(0, static_cast<int>(color[2]*255.f+0.5f), 255)),
		255};

	const float pixelRadius = radius * StelApp::getInstance().getScreenScale();
	Vec2f projectedBasisX(1.f, 0.f);
	Vec2f projectedBasisY(0.f, 1.f);
	float minX = center[0] - pixelRadius;
	float maxX = center[0] + pixelRadius;
	float minY = center[1] - pixelRadius;
	float maxY = center[1] + pixelRadius;
	bool useAngularPsf = false;
	if (sPainter && flagPsfStarProjectionCorrection)
	{
		Vec3d n(direction);
		n.normalize();

		Vec3d up(0., 0., 1.);
		if (std::fabs(n * up) > 0.95)
			up.set(0., 1., 0.);

		Vec3d tangentX(up ^ n);
		tangentX.normalize();
		Vec3d tangentY(n ^ tangentX);
		tangentY.normalize();

		const StelProjectorP projector = sPainter->getProjector();
		const double pixelPerRad = projector->getPixelPerRadAtCenter();
		const double angularStep = 1.0 / pixelPerRad;
		Vec3d projectedX;
		Vec3d projectedY;
		Vec3d directionX = n + angularStep * tangentX;
		Vec3d directionY = n + angularStep * tangentY;
		directionX.normalize();
		directionY.normalize();
		if (projector->project(directionX, projectedX))
		{
			const Vec2f candidateBasisX(static_cast<float>(projectedX[0] - center[0]), static_cast<float>(projectedX[1] - center[1]));
			if (candidateBasisX.normSquared() > 0.f)
				projectedBasisX = candidateBasisX;
		}
		if (projector->project(directionY, projectedY))
		{
			const Vec2f candidateBasisY(static_cast<float>(projectedY[0] - center[0]), static_cast<float>(projectedY[1] - center[1]));
			if (candidateBasisY.normSquared() > 0.f)
				projectedBasisY = candidateBasisY;
		}

		const float maxBasisComponent = qMax(qMax(std::fabs(projectedBasisX[0]), std::fabs(projectedBasisX[1])),
		                                      qMax(std::fabs(projectedBasisY[0]), std::fabs(projectedBasisY[1])));
		const bool crossesWrap = std::fabs(projectedBasisX[0]) > 64.f || std::fabs(projectedBasisY[0]) > 64.f;
		useAngularPsf = crossesWrap || maxBasisComponent > 8.f;

		if (useAngularPsf)
		{
			const double angularRadius = qMax(1.0e-6, static_cast<double>(pixelRadius) / pixelPerRad);
			const double sinRadius = std::sin(angularRadius);
			const double cosRadius = std::cos(angularRadius);
			bool haveSample = false;
			minX = maxX = center[0];
			minY = maxY = center[1];
			for (int i = 0; i < 8; ++i)
			{
				const double a = (2.0 * M_PI * i) / 8.0;
				Vec3d edgeDirection = cosRadius * n + sinRadius * (std::cos(a) * tangentX + std::sin(a) * tangentY);
				edgeDirection.normalize();
				Vec3d projectedEdge;
				if (projector->project(edgeDirection, projectedEdge))
				{
					const float x = static_cast<float>(projectedEdge[0]);
					const float y = static_cast<float>(projectedEdge[1]);
					minX = haveSample ? qMin(minX, x) : x;
					maxX = haveSample ? qMax(maxX, x) : x;
					minY = haveSample ? qMin(minY, y) : y;
					maxY = haveSample ? qMax(maxY, y) : y;
					haveSample = true;
				}
			}

			if (!haveSample)
			{
				minX = center[0] - pixelRadius;
				maxX = center[0] + pixelRadius;
				minY = center[1] - pixelRadius;
				maxY = center[1] + pixelRadius;
			}

			const float viewportMinX = static_cast<float>(projector->getViewportPosX());
			const float viewportMaxX = viewportMinX + static_cast<float>(projector->getViewportWidth());
			const float viewportMinY = static_cast<float>(projector->getViewportPosY());
			const float viewportMaxY = viewportMinY + static_cast<float>(projector->getViewportHeight());
			const float pad = qMax(2.f, pixelRadius * 0.25f);
			minX = qMax(minX - pad, viewportMinX);
			maxX = qMin(maxX + pad, viewportMaxX);
			minY = qMax(minY - pad, viewportMinY);
			maxY = qMin(maxY + pad, viewportMaxY);
			if (minX >= maxX || minY >= maxY)
				return;
		}
	}
	Vec3f normalizedDirection = direction.toVec3f();
	normalizedDirection.normalize();
	for (int i = 0; i < 6; ++i)
	{
		const Vec2f& corner = corners[i];
		PsfStarVertex vx;
		if (useAngularPsf)
		{
			vx.center = Vec2f(corner[0] < 0.f ? minX : maxX,
			                  corner[1] < 0.f ? minY : maxY);
		}
		else
		{
			vx.center = Vec2f(center[0], center[1]) + (corner[0] * pixelRadius) * projectedBasisX + (corner[1] * pixelRadius) * projectedBasisY;
		}
		vx.corner = corner;
		vx.direction = normalizedDirection;
		vx.angularMode = useAngularPsf ? 1.f : 0.f;
		vx.peakRadiance = peakRadiance;
		vx.psfRadius = radius;
		std::memcpy(vx.color, starColor, 4);
		vertices.append(vx);
	}
}

void StelSkyDrawer::drawPsfPointSource(StelPainter* sPainter, const Vec3d& direction, const Vec3f& win, float appMag, const Vec3f& color, float twinkleFactor, float luminanceScale)
{
	Q_UNUSED(twinkleFactor)

	if (!std::isfinite(luminanceScale) || luminanceScale <= 0.f)
		return;

	float peakRadiance = 0.f;
	if (!computePsfPeakRadiance(appMag, &peakRadiance))
		return;
	peakRadiance *= luminanceScale;

	float greenScale = 1.f;
	const Vec3f linearStarColor = psfGreenNormalization(color, 0.1f, greenScale);
	const float peakRadianceColor = peakRadiance * greenScale;

	addPsfStarVertices(psfPointVertices, sPainter, direction, win, linearStarColor, peakRadianceColor, psfStarPointRadius);

	const float flareOnset = psfSmoothStep(0.5f, 2.5f, peakRadianceColor);
	const float effectiveFlareStrength = psfStarFlareStrength * flareOnset;
	if (effectiveFlareStrength > 0.f && psfStarFlareDecay > 0.f)
	{
		const float glowPeak = peakRadianceColor;
		const float glowRadius = computePsfGlowRadius(glowPeak, effectiveFlareStrength);
		if (glowRadius > psfStarPointRadius)
			addPsfStarVertices(psfGlowVertices, sPainter, direction, win, linearStarColor * flareOnset, glowPeak, glowRadius);
	}

	if (psfPointVertices.size() >= static_cast<int>(maxPointSources*6) ||
	    psfGlowVertices.size() >= static_cast<int>(maxPointSources*6))
	{
		postDrawPointSource(sPainter);
	}
}

void StelSkyDrawer::flushPsfPointSources(StelPainter* sPainter)
{
	if (psfPointVertices.isEmpty() && psfGlowVertices.isEmpty())
		return;

	const StelProjectorP projector = sPainter->getProjector();
	if (!psfShaderProjector || !projector->isSameProjection(*psfShaderProjector))
	{
		delete psfPointShaderProgram;
		delete psfGlowShaderProgram;
		psfPointShaderProgram = Q_NULLPTR;
		psfGlowShaderProgram = Q_NULLPTR;
		psfShaderProjector = projector;

		const char *psfVsrc =
			"ATTRIBUTE highp vec2 center;\n"
			"ATTRIBUTE highp vec2 corner;\n"
			"ATTRIBUTE highp vec3 direction;\n"
			"ATTRIBUTE highp float angularMode;\n"
			"ATTRIBUTE mediump vec3 color;\n"
			"ATTRIBUTE highp float peakRadiance;\n"
			"ATTRIBUTE highp float psfRadius;\n"
			"uniform highp mat4 projectionMatrix;\n"
			"VARYING highp vec2 vCorner;\n"
			"VARYING highp vec3 outDirection;\n"
			"VARYING highp float outAngularMode;\n"
			"VARYING mediump vec3 outColor;\n"
			"VARYING highp float outPeakRadiance;\n"
			"VARYING highp float outPsfRadius;\n"
			"void main(void)\n"
			"{\n"
			"    gl_Position = projectionMatrix * vec4(center, 0.0, 1.0);\n"
			"    vCorner = corner;\n"
			"    outDirection = normalize(direction);\n"
			"    outAngularMode = angularMode;\n"
			"    outColor = color;\n"
			"    outPeakRadiance = peakRadiance;\n"
			"    outPsfRadius = psfRadius;\n"
			"}\n";

		const QByteArray psfDistanceFsrc =
			projector->getUnProjectShader() +
			R"(
uniform highp float pointScale;
uniform highp float psfPixelPerRad;
VARYING highp vec2 vCorner;
VARYING highp vec3 outDirection;
VARYING highp float outAngularMode;
VARYING mediump vec3 outColor;
VARYING highp float outPeakRadiance;
VARYING highp float outPsfRadius;
highp float psfDistancePx()
{
	if (outAngularMode < 0.5)
		return length(vCorner) * outPsfRadius;
	bool ok = false;
	highp vec3 pixelDirection = unProject(gl_FragCoord.x, gl_FragCoord.y, ok);
	if (!ok)
		return -1.0;
	pixelDirection = normalize(pixelDirection);
	highp float c = clamp(dot(pixelDirection, outDirection), -1.0, 1.0);
	highp float angle = sqrt(max(0.0, 2.0 * (1.0 - c)));
	return angle * psfPixelPerRad / max(pointScale, 0.001);
}
)";
		const QByteArray psfPointFsrc =
			psfDistanceFsrc +
			R"(
uniform highp float pointRadius;
void main(void)
{
	highp float px = psfDistancePx();
	if (px < 0.0 || px > pointRadius)
		discard;
	highp float x = clamp(px / pointRadius, 0.0, 1.0);
	highp float falloff = 1.0 - x * x;
	falloff *= falloff;
	FRAG_COLOR = vec4(outColor * (falloff * outPeakRadiance), 1.0);
}
)";
		const QByteArray psfGlowFsrc =
			psfDistanceFsrc +
			R"(
uniform highp float psfA;
uniform highp float psfB;
uniform highp float flareStrength;
void main(void)
{
	highp float px = psfDistancePx();
	if (px >= outPsfRadius || px <= 0.0)
		discard;
	highp float p04 = pow(outPeakRadiance, 0.4);
	highp float s = max((p04 / px - psfA) * psfB, 0.0);
	highp float val = min(s * s * sqrt(s), outPeakRadiance);
	FRAG_COLOR = vec4(outColor * val * flareStrength, 1.0);
}
)";
		auto createPsfProgram = [this, psfVsrc](const QByteArray& fsrc, const char* name, PsfStarShaderVars& vars)
		{
			QOpenGLShader v(QOpenGLShader::Vertex);
			v.compileSourceCode(StelOpenGL::globalShaderPrefix(StelOpenGL::VERTEX_SHADER) + psfVsrc);
			if (!v.log().isEmpty()) { qWarning() << "StelSkyDrawer::flushPsfPointSources(): Warnings while compiling" << name << "vshader:" << v.log(); }
			QOpenGLShader f(QOpenGLShader::Fragment);
			f.compileSourceCode(StelOpenGL::globalShaderPrefix(StelOpenGL::FRAGMENT_SHADER) + fsrc);
			if (!f.log().isEmpty()) { qWarning() << "StelSkyDrawer::flushPsfPointSources(): Warnings while compiling" << name << "fshader:" << f.log(); }

			QOpenGLShaderProgram* program = new QOpenGLShaderProgram(QOpenGLContext::currentContext());
			program->addShader(&v);
			program->addShader(&f);
			StelPainter::linkProg(program, name);
			vars.projectionMatrix = program->uniformLocation("projectionMatrix");
			vars.center = program->attributeLocation("center");
			vars.corner = program->attributeLocation("corner");
			vars.direction = program->attributeLocation("direction");
			vars.angularMode = program->attributeLocation("angularMode");
			vars.color = program->attributeLocation("color");
			vars.peakRadiance = program->attributeLocation("peakRadiance");
			vars.psfRadius = program->attributeLocation("psfRadius");
			vars.pointRadius = program->uniformLocation("pointRadius");
			vars.pointScale = program->uniformLocation("pointScale");
			vars.pixelPerRad = program->uniformLocation("psfPixelPerRad");
			vars.psfA = program->uniformLocation("psfA");
			vars.psfB = program->uniformLocation("psfB");
			vars.flareStrength = program->uniformLocation("flareStrength");
			return program;
		};
		psfPointShaderProgram = createPsfProgram(psfPointFsrc, "psfStarPointShader", psfPointShaderVars);
		psfGlowShaderProgram = createPsfProgram(psfGlowFsrc, "psfStarGlowShader", psfGlowShaderVars);
	}

	if (!psfPointShaderProgram || !psfGlowShaderProgram)
		return;

	const QMatrix4x4 qMat = projector->getProjectionMatrix().toQMatrix();
	auto drawBatch = [this, &qMat, &projector](QOpenGLShaderProgram* program, const PsfStarShaderVars& vars, const QVector<PsfStarVertex>& vertices, bool glow)
	{
		if (vertices.isEmpty())
			return;

		vbo->bind();
		vbo->write(0, vertices.constData(), vertices.size()*static_cast<int>(sizeof(PsfStarVertex)));
		vbo->release();

		program->bind();
		program->setUniformValue(vars.projectionMatrix, qMat);
		program->setUniformValue(vars.pointRadius, psfStarPointRadius);
		program->setUniformValue(vars.pointScale, StelApp::getInstance().getScreenScale());
		program->setUniformValue(vars.pixelPerRad, projector->getPixelPerRadAtCenter());
		projector->setUnProjectUniforms(*program);
		if (glow)
		{
			const float r = qMax(psfStarPointRadius, 1.0e-3f);
			const float a = psfStarFlareDecay / r;
			const float b = 1.f / (M_PIf / r - a);
			program->setUniformValue(vars.psfA, a);
			program->setUniformValue(vars.psfB, b);
			program->setUniformValue(vars.flareStrength, psfStarFlareStrength);
		}

		if (psfVao->isCreated())
			psfVao->bind();
		vbo->bind();
		auto setAttributeBuffer = [program](int location, GLenum type, int offset, int tupleSize)
		{
			if (location >= 0)
				program->setAttributeBuffer(location, type, offset, tupleSize, sizeof(PsfStarVertex));
		};
		setAttributeBuffer(vars.center, GL_FLOAT, offsetof(PsfStarVertex, center), 2);
		setAttributeBuffer(vars.corner, GL_FLOAT, offsetof(PsfStarVertex, corner), 2);
		setAttributeBuffer(vars.direction, GL_FLOAT, offsetof(PsfStarVertex, direction), 3);
		setAttributeBuffer(vars.angularMode, GL_FLOAT, offsetof(PsfStarVertex, angularMode), 1);
		setAttributeBuffer(vars.peakRadiance, GL_FLOAT, offsetof(PsfStarVertex, peakRadiance), 1);
		setAttributeBuffer(vars.psfRadius, GL_FLOAT, offsetof(PsfStarVertex, psfRadius), 1);
		setAttributeBuffer(vars.color, GL_UNSIGNED_BYTE, offsetof(PsfStarVertex, color), 3);
		vbo->release();
		auto enableAttributeArray = [program](int location)
		{
			if (location >= 0)
				program->enableAttributeArray(location);
		};
		enableAttributeArray(vars.center);
		enableAttributeArray(vars.corner);
		enableAttributeArray(vars.direction);
		enableAttributeArray(vars.angularMode);
		enableAttributeArray(vars.color);
		enableAttributeArray(vars.peakRadiance);
		enableAttributeArray(vars.psfRadius);

		glDrawArrays(GL_TRIANGLES, 0, vertices.size());

		if (psfVao->isCreated())
		{
			psfVao->release();
		}
		else
		{
			auto disableAttributeArray = [program](int location)
			{
				if (location >= 0)
					program->disableAttributeArray(location);
			};
			disableAttributeArray(vars.center);
			disableAttributeArray(vars.corner);
			disableAttributeArray(vars.direction);
			disableAttributeArray(vars.angularMode);
			disableAttributeArray(vars.color);
			disableAttributeArray(vars.peakRadiance);
			disableAttributeArray(vars.psfRadius);
		}
		program->release();
	};

	drawBatch(psfPointShaderProgram, psfPointShaderVars, psfPointVertices, false);
	drawBatch(psfGlowShaderProgram, psfGlowShaderVars, psfGlowVertices, true);
	psfPointVertices.clear();
	psfGlowVertices.clear();
}

// Draw's the Sun's corona during a solar eclipse on Earth.
// painter: in FrameJ2000
// posJ2000: solar position, J2000
// radius: angular radius of the sun, radians
// color: attenuated sunlight color (extinction may have reddened the sun)
// alpha: transparency
void StelSkyDrawer::drawSunCorona(StelPainter* painter, const Vec3d& posJ2000, double radius, const Vec3f& color, const float alpha)
{
	radius *= (512.f/193.f); // Texture size is 1024, solar radius within is 192 or 193. Increase radius to the width/height of actual image, in radians.
	double ra, dec;
	StelUtils::rectToSphe(&ra, &dec, posJ2000);

	// Define a rotation matrix around the sun that adjusts image rotation in the sky. We must unrotate from the original image orientation and adjust for the new angle of J2000 vs. ecliptic.
	// Our corona image was made in 2008-08-01 near Khovd, Mongolia. It shows the correct parallactic angle for its location and time, we must add this, and subtract the ecliptic/equatorial angle from that date of 15.43 degrees.
	// https://en.wikipedia.org/wiki/Rotation_matrix
	const double eclAngle=getPrecessionAngleVondrakCurrentEpsilonA()*cos(ra);
	const double theta=(44.65-15.43)*M_PI_180 - eclAngle; // dynamical rotation angle!
	const double cTh=cos(theta);
	const double mcTh=1.-cTh;
	const double sTh=sin(theta);
	const Mat3d R3(posJ2000[0]*posJ2000[0]*mcTh+cTh,             posJ2000[0]*posJ2000[1]*mcTh-posJ2000[2]*sTh, posJ2000[0]*posJ2000[2]*mcTh+posJ2000[1]*sTh,
	               posJ2000[0]*posJ2000[1]*mcTh+posJ2000[2]*sTh, posJ2000[1]*posJ2000[1]*mcTh+cTh,             posJ2000[1]*posJ2000[2]*mcTh-posJ2000[0]*sTh,
	               posJ2000[0]*posJ2000[2]*mcTh-posJ2000[1]*sTh, posJ2000[1]*posJ2000[2]*mcTh+posJ2000[0]*sTh, posJ2000[2]*posJ2000[2]*mcTh+cTh);

	static QVector<Vec3d> contour(coronaTextureCoords.size());
	contour.clear();
	const int centerPoint=coronaMeshDim/2; // point 5:2=2 is the point index in sun's center
	for (int j=-centerPoint;j<coronaMeshDim-1-centerPoint;++j)
	{
		for (int i=-centerPoint;i<coronaMeshDim-1-centerPoint;++i)
		{
			const double decJ0=dec+j    *(2.0*radius/(coronaMeshDim-1));
			const double decJ1=dec+(j+1)*(2.0*radius/(coronaMeshDim-1));
			const double cDecJ0=1./cos(decJ0);
			const double cDecJ1=1./cos(decJ1);
			const double dRA0=i*(2.0*radius/(coronaMeshDim-1));
			const double dRA1=(i+1)*(2.0*radius/(coronaMeshDim-1));

			Vec3d vertex;
			StelUtils::spheToRect(ra-dRA0*cDecJ0, decJ0, vertex);
			contour << R3*vertex;
			StelUtils::spheToRect(ra-dRA1*cDecJ0, decJ0, vertex);
			contour << R3*vertex;
			StelUtils::spheToRect(ra-dRA0*cDecJ1, decJ1, vertex);
			contour << R3*vertex;
			StelUtils::spheToRect(ra-dRA1*cDecJ0, decJ0, vertex);
			contour << R3*vertex;
			StelUtils::spheToRect(ra-dRA1*cDecJ1, decJ1, vertex);
			contour << R3*vertex;
			StelUtils::spheToRect(ra-dRA0*cDecJ1, decJ1, vertex);
			contour << R3*vertex;
		}
	}
	Q_ASSERT(contour.length()==coronaTextureCoords.length());

	coronaMesh.vertex=contour;
	coronaMesh.texCoords=coronaTextureCoords;
	coronaMesh.primitiveType=StelVertexArray::Triangles;

	texSunCorona->bind();
	// For some reason we must mix color with the given alpha as well, else mixing does not work.
	painter->setColor(color*alpha, alpha);
	painter->setBlending(true, GL_ONE, GL_ONE);
	painter->drawStelVertexArray(coronaMesh, false);

	// GZ: WHY DO WE NEED THIS?
	postDrawPointSource(painter, true);
}

// Terminate drawing of a 3D model, draw the halo
void StelSkyDrawer::postDrawSky3dModel(StelPainter* painter, const Vec3d& v, float illuminatedArea, float mag, const Vec3f& color, const bool isSun)
{
	const float scale = StelApp::getInstance().getScreenScale();
	const float pixPerRad = painter->getProjector()->getPixelPerRadAtCenter();
	// Assume a disk shape
	float pixRadius = std::sqrt(illuminatedArea/(60.f*60.f)*M_PI_180f*M_PI_180f*(pixPerRad*pixPerRad))/M_PIf;
	float pxRd = pixRadius*3.f+100.f*scale;
	bool noStarHalo = false;

	if (isSun)
	{
		// Sun, halo size varies in function of the magnitude because sun as seen from pluto should look dimmer
		// as the sun as seen from earth
		texSunHalo->bind();
		painter->setBlending(true, GL_ONE, GL_ONE);

		const float rmag = big3dModelHaloRadius*scale*(mag+15.f)/-11.f;
		const float cmag = (rmag>=pxRd) ? 1.f : qMax(0.15f, 1.f-(pxRd-rmag)/100/scale); // was qMax(0, .), but this would remove the halo when sun is dim.
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
	const bool saveSpiky = flagStarSpiky;
	if (mag<-5.f) // exclude very bright objects only
		flagStarSpiky = false;
	else
		flagStarSpiky = saveSpiky;

	RCMag rcm;
	computeRCMag(mag, &rcm);

	// We now have the radius and luminosity of the small halo
	// If the disk of the planet is big enough to be visible, we should adjust the eye adaptation luminance
	// so that the radius of the halo is small enough to be not visible (so that we see the disk)

	// TODO: Change drawing halo to more realistic view of stars and planets
	const float tStart = (flagPsfStars ? 1.5f : 3.f)*scale; // Was 2.f: planet's halo is too dim. Atque 2020-11-12: No need to change these anymore. It appears that this has to do with halo size vs FOV (?).
	const float tStop = (flagPsfStars ? 3.f : 6.f)*scale;
	bool truncated=false;

	float maxHaloRadius = qMax(tStart*6.f, pixRadius*3.f); //Atque 2020-11-12: Careful, if tStart*6.f is too big (tStart*10.f or something), the Moon gets a ridiculously big halo.
	if (rcm.radius>maxHaloRadius)
	{
		truncated = true;
		// One more factor of scale here is needed to compensate for taking a square root of the already included factor
		rcm.radius=maxHaloRadius+std::sqrt((rcm.radius-maxHaloRadius)*scale);
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
			const float fov = static_cast<float>(core->getMovementMgr()->getCurrentFov());
			// Report to the SkyDrawer that a very bright object (most notably Sun, Moon, bright planet)
			// is in view. LP:1138533 correctly wants no such effect if object is hidden by landscape horizon.
			LandscapeMgr* lmgr=GETSTELMODULE(LandscapeMgr);
			Q_ASSERT(lmgr);
			Landscape *landscape=lmgr->getCurrentLandscape();
			Q_ASSERT(landscape);
			float opacity=(landscape->getFlagShow() ? landscape->getOpacity(core->j2000ToAltAz(v, StelCore::RefractionAuto)) : 0.0f);
			reportLuminanceInFov(qMin(700.f, qMin(wl/50, (60.f*60.f)/(fov*fov)*6.f))*(1.0f-opacity));
		}
	}

	if (!noStarHalo)
	{
		preDrawPointSource(painter);
		drawPointSource(painter, v, rcm, color, false, 1.f, mag);
		postDrawPointSource(painter);
	}
	flagStarTwinkle=saveTwinkle;
	flagForcedTwinkle=saveForcedTwinkle;
	flagStarSpiky=saveSpiky;
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


void StelSkyDrawer::setLightPollutionLuminance(const double luminance)
{
	if(lightPollutionLuminance==luminance)
		return;
	lightPollutionLuminance=luminance;
	StelApp::immediateSave("stars/init_light_pollution_luminance", luminance);
	emit lightPollutionLuminanceChanged(luminance);
}

int StelSkyDrawer::getBortleScaleIndex() const
{
	return StelCore::luminanceToBortleScaleIndex(lightPollutionLuminance);
}

void StelSkyDrawer::setFlagStarSpiky(bool b)
{
	if (b!=flagStarSpiky)
	{
		flagStarSpiky=b;
		StelApp::immediateSave("stars/flag_star_spiky", b);
		emit flagStarSpikyChanged(flagStarSpiky);
	}
}

void StelSkyDrawer::setFlagPsfStars(bool b)
{
	if (b!=flagPsfStars)
	{
		flagPsfStars=b;
		StelApp::immediateSave("stars/flag_psf_stars", b);
		update(0);
		emit flagPsfStarsChanged(flagPsfStars);
	}
}

void StelSkyDrawer::setFlagPsfStarProjectionCorrection(bool b)
{
	if (b!=flagPsfStarProjectionCorrection)
	{
		flagPsfStarProjectionCorrection=b;
		StelApp::immediateSave("stars/flag_psf_projection_correction", b);
		update(0);
		emit flagPsfStarProjectionCorrectionChanged(flagPsfStarProjectionCorrection);
	}
}

void StelSkyDrawer::setPsfStarPointRadius(double r)
{
	const float value = qBound(0.5f, static_cast<float>(r), 5.f);
	if (qFuzzyCompare(psfStarPointRadius, value))
		return;
	psfStarPointRadius = value;
	StelApp::immediateSave("stars/psf_star_point_radius", psfStarPointRadius);
	update(0);
	emit psfStarPointRadiusChanged(psfStarPointRadius);
}

void StelSkyDrawer::setPsfStarFlareDecay(double decay)
{
	const float value = qBound(0.01f, static_cast<float>(decay), 1.f);
	if (qFuzzyCompare(psfStarFlareDecay, value))
		return;
	psfStarFlareDecay = value;
	StelApp::immediateSave("stars/psf_star_flare_decay", psfStarFlareDecay);
	update(0);
	emit psfStarFlareDecayChanged(psfStarFlareDecay);
}

void StelSkyDrawer::setPsfStarFlareStrength(double strength)
{
	const float value = qBound(0.f, static_cast<float>(strength), 20.f);
	if (qFuzzyCompare(psfStarFlareStrength, value))
		return;
	psfStarFlareStrength = value;
	StelApp::immediateSave("stars/psf_star_flare_strength", psfStarFlareStrength);
	update(0);
	emit psfStarFlareStrengthChanged(psfStarFlareStrength);
}

void StelSkyDrawer::setPsfStarBrightSourceMagLimit(double magLimit)
{
	const float value = qBound(-13.f, static_cast<float>(magLimit), -1.f);
	if (qFuzzyCompare(psfStarBrightSourceMagLimit, value))
		return;
	psfStarBrightSourceMagLimit = value;
	StelApp::immediateSave("stars/psf_star_bright_source_mag_limit", psfStarBrightSourceMagLimit);
	update(0);
	emit psfStarBrightSourceMagLimitChanged(psfStarBrightSourceMagLimit);
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
	for (int bV100=-50;bV100<=400;bV100++)
	{
		QString entry = QString::asprintf("bv_color_%+5.2f", static_cast<double>(bV100)*0.01);
		const QStringList s(conf->value(QString("stars/") + entry).toStringList());
		if (!s.isEmpty())
		{
			Vec3f c;
			if (s.size()==1)
				c = Vec3f(s[0]);
			else
				c =Vec3f(s);
			color_map[bV100*0.01f] = Gamma(eye->getDisplayGamma(),c);
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

void StelSkyDrawer::setRelativeStarScale(double b)
{
	starRelativeScale=b;
	StelApp::immediateSave("stars/relative_scale", b);
	emit relativeStarScaleChanged(b);
}

void StelSkyDrawer::setAbsoluteStarScale(double b)
{
	starAbsoluteScaleF=b;
	StelApp::immediateSave("stars/absolute_scale", b);
	emit absoluteStarScaleChanged(b);
}

void StelSkyDrawer::setTwinkleAmount(double b)
{
	twinkleAmount=b;
	StelApp::immediateSave("stars/star_twinkle_amount", b);
	emit twinkleAmountChanged(b);
}

void StelSkyDrawer::setFlagTwinkle(bool b)
{
	if(b!=flagStarTwinkle)
	{
		flagStarTwinkle=b;
		StelApp::immediateSave("stars/flag_star_twinkle", b);
		emit flagTwinkleChanged(b);
	}
}

void StelSkyDrawer::setFlagForcedTwinkle(bool b)
{
	if(b!=flagForcedTwinkle)
	{
		flagForcedTwinkle=b;
		StelApp::immediateSave("stars/flag_forced_twinkle", b);
		emit flagForcedTwinkleChanged(b);
	}
}

void StelSkyDrawer::setFlagDrawBigStarHalo(bool b)
{
	if(b!=flagDrawBigStarHalo)
	{
		flagDrawBigStarHalo=b;
		StelApp::immediateSave("stars/flag_star_halo", b);
		emit flagDrawBigStarHaloChanged(b);
	}
}

void StelSkyDrawer::setFlagStarMagnitudeLimit(bool b)
{
	if(b!=flagStarMagnitudeLimit)
	{
		flagStarMagnitudeLimit = b;
		StelApp::immediateSave("astro/flag_star_magnitude_limit", b);
		emit flagStarMagnitudeLimitChanged(b);
	}
}

void StelSkyDrawer::setFlagNebulaMagnitudeLimit(bool b)
{
	if(b!=flagNebulaMagnitudeLimit)
	{
		flagNebulaMagnitudeLimit = b;
		StelApp::immediateSave("astro/flag_nebula_magnitude_limit", b);
		emit flagNebulaMagnitudeLimitChanged(b);
	}
}

void StelSkyDrawer::setFlagPlanetMagnitudeLimit(bool b)
{
	if(b!=flagPlanetMagnitudeLimit)
	{
		flagPlanetMagnitudeLimit = b;
		StelApp::immediateSave("astro/flag_planet_magnitude_limit", b);
		emit flagPlanetMagnitudeLimitChanged(b);
	}
}

void StelSkyDrawer::setCustomStarMagnitudeLimit(double limit)
{
	customStarMagLimit=limit;
	StelApp::immediateSave("astro/star_magnitude_limit", limit);
	emit customStarMagLimitChanged(limit);
}

void StelSkyDrawer::setCustomNebulaMagnitudeLimit(double limit)
{
	customNebulaMagLimit=limit;
	StelApp::immediateSave("astro/nebula_magnitude_limit", limit);
	emit customNebulaMagLimitChanged(limit);
}

void StelSkyDrawer::setCustomPlanetMagnitudeLimit(double limit)
{
	customPlanetMagLimit=limit;
	StelApp::immediateSave("astro/planet_magnitude_limit", limit);
	emit customPlanetMagLimitChanged(limit);
}

void StelSkyDrawer::setFlagLuminanceAdaptation(bool b)
{
	if(b!=flagLuminanceAdaptation)
	{
		flagLuminanceAdaptation=b;
		StelApp::immediateSave("viewing/use_luminance_adaptation", b);
		emit flagLuminanceAdaptationChanged(b);
	}
}

void StelSkyDrawer::setDaylightLabelThreshold(double t)
{
	daylightLabelThreshold=t;
	StelApp::immediateSave("viewing/sky_brightness_label_threshold", t);
	emit daylightLabelThresholdChanged(t);
}

void StelSkyDrawer::setExtinctionCoefficient(double extCoeff)
{
	extinction.setExtinctionCoefficient(static_cast<float>(extCoeff));
	StelApp::immediateSave("landscape/atmospheric_extinction_coefficient", extCoeff);
	emit extinctionCoefficientChanged(static_cast<double>(extinction.getExtinctionCoefficient()));
}

void StelSkyDrawer::setAtmosphereTemperature(double celsius)
{
	refraction.setTemperature(static_cast<float>(celsius));
	StelApp::immediateSave("landscape/temperature_C", celsius);
	emit atmosphereTemperatureChanged(static_cast<double>(refraction.getTemperature()));
}

void StelSkyDrawer::setAtmospherePressure(double mbar)
{
	refraction.setPressure(static_cast<float>(mbar));
	StelApp::immediateSave("landscape/pressure_mbar", mbar);
	emit atmospherePressureChanged(static_cast<double>(refraction.getPressure()));
}

// These are to find out the best sky parameters. Program feature for debugging/expert mode.
// These will be connected from AtmosphereDialog and forward the settings to SkyLight class.

void StelSkyDrawer::setFlagDrawSunAfterAtmosphere(bool val)
{
	flagDrawSunAfterAtmosphere=val;
	QSettings* conf = StelApp::getInstance().getSettings();
	 conf->setValue("landscape/draw_sun_after_atmosphere",val);
	 conf->sync();
	 emit flagDrawSunAfterAtmosphereChanged(val);
}

void StelSkyDrawer::setFlagEarlySunHalo(bool val)
{
	flagEarlySunHalo= val;
	QSettings* conf = StelApp::getInstance().getSettings();
	conf->setValue("landscape/early_solar_halo", val);
	conf->sync();
	emit flagEarlySunHaloChanged(val);
}

void StelSkyDrawer::setFlagTfromK(bool val)
{
	flagTfromK=val;
	QSettings* conf = StelApp::getInstance().getSettings();
	conf->setValue("landscape/use_T_from_k", val);
	conf->sync();
	emit flagTfromKChanged(val);
}

void StelSkyDrawer::setT(double newT)
{
    turbidity=static_cast<float>(newT);
        QSettings* conf = StelApp::getInstance().getSettings();
        conf->setValue("landscape/turbidity", newT);
        emit turbidityChanged(newT);
}

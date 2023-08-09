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
// For debugging draw() only:
//#include "StelObjectMgr.hpp"
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
#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>

namespace
{
constexpr int FS_QUAD_COORDS_PER_VERTEX = 2;
constexpr int SKY_VERTEX_ATTRIB_INDEX = 0;
}

// Class which manages the displaying of the Zodiacal Light
ZodiacalLight::ZodiacalLight()
	: propMgr(Q_NULLPTR)
	, color(1.f, 1.f, 1.f)
	, intensity(1.)
	, intensityFovScale(1.0)
	, intensityMinFov(0.25) // when zooming in further, Z.L. is no longer visible.
	, intensityMaxFov(2.5) // when zooming out further, Z.L. is fully visible (when enabled).
	, lastJD(-1.0E6)
{
	setObjectName("ZodiacalLight");
	fader = new LinearFader();
	propMgr=StelApp::getInstance().getStelPropertyManager();
}

ZodiacalLight::~ZodiacalLight()
{
	delete fader;
	fader = Q_NULLPTR;
}

void ZodiacalLight::init()
{
	QSettings* conf = StelApp::getInstance().getSettings();
	Q_ASSERT(conf);

	auto& gl = *QOpenGLContext::currentContext()->functions();

	// The Paper describes brightness values over the complete sky, so also the texture covers the full sky. 
	// The data hole around the sun has been filled by useful values.
	mainTex = StelApp::getInstance().getTextureManager().createTexture(StelFileMgr::getInstallationDir()+"/textures/zodiacallight_2004.png");
	mainTex->bind(0);
	gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	mainTex->release();
	setFlagShow(conf->value("astro/flag_zodiacal_light", true).toBool());
	setIntensity(conf->value("astro/zodiacal_light_intensity",1.).toDouble());

	vbo.reset(new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer));
	vbo->create();
	vbo->bind();
	const GLfloat vertices[]=
	{
		// full screen quad
		-1, -1,
		 1, -1,
		-1,  1,
		 1,  1,
	};
	gl.glBufferData(GL_ARRAY_BUFFER, sizeof vertices, vertices, GL_STATIC_DRAW);
	vbo->release();

	vao.reset(new QOpenGLVertexArrayObject);
	vao->create();
	bindVAO();
	setupCurrentVAO();
	releaseVAO();

	QString displayGroup = N_("Display Options");
	addAction("actionShow_ZodiacalLight", displayGroup, N_("Zodiacal Light"), "flagZodiacalLightDisplayed", "Ctrl+Shift+Z");

	StelCore* core=StelApp::getInstance().getCore();
	connect(core, SIGNAL(locationChanged(StelLocation)), this, SLOT(handleLocationChanged(StelLocation)));
}

void ZodiacalLight::setupCurrentVAO()
{
	auto& gl = *QOpenGLContext::currentContext()->functions();
	vbo->bind();
	gl.glVertexAttribPointer(0, FS_QUAD_COORDS_PER_VERTEX, GL_FLOAT, false, 0, 0);
	vbo->release();
	gl.glEnableVertexAttribArray(SKY_VERTEX_ATTRIB_INDEX);
}

void ZodiacalLight::bindVAO()
{
	if(vao->isCreated())
		vao->bind();
	else
		setupCurrentVAO();
}

void ZodiacalLight::releaseVAO()
{
	if(vao->isCreated())
	{
		vao->release();
	}
	else
	{
		auto& gl = *QOpenGLContext::currentContext()->functions();
		gl.glDisableVertexAttribArray(SKY_VERTEX_ATTRIB_INDEX);
	}
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

	// The ZL is best observed from Earth only. On the Moon, we must be happy with ZL along the J2000 ecliptic. (Sorry for LP:1628765, I don't find a general solution.)
	StelProjector::ModelViewTranformP transfo;
	if (core->getCurrentLocation().planetName == "Earth")
		transfo = core->getObservercentricEclipticOfDateModelViewTransform();
	else
		transfo = core->getObservercentricEclipticJ2000ModelViewTransform();

	const StelProjectorP projector = core->getProjection(transfo);
	const auto drawer=core->getSkyDrawer();
	const auto& extinction=drawer->getExtinction();

	if(!renderProgram || !prevProjector || !projector->isSameProjection(*prevProjector))
	{
		renderProgram.reset(new QOpenGLShaderProgram);
		prevProjector = projector;

		const auto vert =
			StelOpenGL::globalShaderPrefix(StelOpenGL::VERTEX_SHADER) +
			R"(
ATTRIBUTE highp vec3 vertex;
VARYING highp vec3 ndcPos;
void main()
{
	gl_Position = vec4(vertex, 1.);
	ndcPos = vertex;
}
)";
		bool ok = renderProgram->addShaderFromSourceCode(QOpenGLShader::Vertex, vert);
		if(!renderProgram->log().isEmpty())
			qWarning().noquote() << "MilkyWay: Warnings while compiling vertex shader:\n" << renderProgram->log();
		if(!ok) return;

		const auto frag =
			StelOpenGL::globalShaderPrefix(StelOpenGL::FRAGMENT_SHADER) +
			projector->getUnProjectShader() +
			extinction.getForwardTransformShader() +
			R"(
VARYING highp vec3 ndcPos;
uniform sampler2D mainTex;
uniform vec3 brightness;
uniform mat4 projectionMatrixInverse;
uniform bool extinctionEnabled;
uniform float bortleIntensity;
uniform float lambdaSun;
void main(void)
{
	const float PI = 3.14159265;
	vec4 winPos = projectionMatrixInverse * vec4(ndcPos, 1);
	bool ok = false;
	vec3 modelPos = unProject(winPos.x, winPos.y, ok).xyz;
	if(!ok)
	{
		FRAG_COLOR = vec4(0);
		return;
	}
	modelPos = normalize(modelPos);
	float modelZenithAngle = acos(-modelPos.z);
	float modelLongitude = atan(modelPos.x, modelPos.y) + lambdaSun;
	vec2 texc = vec2(modelLongitude/(2.*PI), modelZenithAngle/PI);

	float extinctionFactor;
	if(extinctionEnabled)
	{
		vec3 worldPos = winPosToWorldPos(winPos.x, winPos.y, ok);
		vec3 altAzViewDir = worldPosToAltAzPos(worldPos);
		float mag = extinctionMagnitude(altAzViewDir);
		extinctionFactor=pow(0.4, mag) * (1.1-bortleIntensity*0.1); // Drop of one magnitude: factor 2.5 or 40%, and further reduced by light pollution.
	}
	else
	{
		extinctionFactor = 1.;
	}

    vec4 color = texture2D(mainTex, texc)*vec4(brightness,1)*extinctionFactor;
	FRAG_COLOR = color;
}
)";
		ok = renderProgram->addShaderFromSourceCode(QOpenGLShader::Fragment, frag);
		if(!renderProgram->log().isEmpty())
			qWarning().noquote() << "ZodiacalLight: Warnings while compiling fragment shader:\n" << renderProgram->log();

		if(!ok) return;

		renderProgram->bindAttributeLocation("vertex", SKY_VERTEX_ATTRIB_INDEX);

		if(!StelPainter::linkProg(renderProgram.get(), "ZodiacalLight render program"))
			return;

		renderProgram->bind();
		shaderVars.mainTex          = renderProgram->uniformLocation("mainTex");
		shaderVars.lambdaSun        = renderProgram->uniformLocation("lambdaSun");
		shaderVars.brightness       = renderProgram->uniformLocation("brightness");
		shaderVars.bortleIntensity  = renderProgram->uniformLocation("bortleIntensity");
		shaderVars.extinctionEnabled= renderProgram->uniformLocation("extinctionEnabled");
		shaderVars.projectionMatrixInverse = renderProgram->uniformLocation("projectionMatrixInverse");
		renderProgram->release();
	}
	if(!renderProgram || !renderProgram->isLinked())
		return;

	int bortle=drawer->getBortleScaleIndex();
	// Test for light pollution, return if too bad.
	if ( (drawer->getFlagHasAtmosphere()) && (bortle > 5) ) return;

	float atmFadeIntensity = GETSTELMODULE(LandscapeMgr)->getAtmosphereFadeIntensity();
	const float nelm = StelCore::luminanceToNELM(drawer->getLightPollutionLuminance());
	const float bortleIntensity = 1.f+(15.5f-2*nelm)*atmFadeIntensity; // smoothed Bortle index moderated by atmosphere fader.

	StelToneReproducer* eye = core->getToneReproducer();

	Q_ASSERT(mainTex);	// A texture must be loaded before calling this

	// Default ZL color is white (sunlight), or whatever has been set e.g. by script.
	Vec3f c = color;

	// ZL is quite sensitive to light pollution. I scale to make it less visible.
	const float lum = drawer->surfaceBrightnessToLuminance(13.5f + 0.5f*bortleIntensity); // (8.0f + 0.5*bortle);

	// Get the luminance scaled between 0 and 1
	float aLum =eye->adaptLuminanceScaled(lum*fader->getInterstate());

	// Bound a maximum luminance
	aLum = qMin(0.15f, aLum*2.f); // 2*aLum at dark sky with mag13.5 at Bortle=1 gives 0.13
	//qDebug() << "aLum=" << aLum;

	// intensity of 1.0 is "proper", but allow boost for dim screens
	c*=aLum*static_cast<float>(intensity*intensityFovScale);

	// Better: adapt brightness by atmospheric brightness
	const float atmLum = GETSTELMODULE(LandscapeMgr)->getAtmosphereAverageLuminance();
	if (atmLum>0.05f) return; // Approximate values for Preetham: 10cd/m^2 at sunset, 3.3 at civil twilight (sun at -6deg). 0.0145 sun at -12, 0.0004 sun at -18,  0.01 at Full Moon!?
	// The atmLum of Bruneton's model is about 1/2 higher than that of Preetham/Schaefer. We must rebalance that!
	float atmFactor=20.0f;
	if (GETSTELMODULE(LandscapeMgr)->getAtmosphereModel()=="showmysky")
		atmFactor=20.0f*(0.05f-0.2f*atmLum); // The factor 0.2f was found empirically. Nominally it should be 0.667, but 0.2 or at least 0.4 looks better.
	else
		atmFactor=20.0f*(0.05f-atmLum);

	//GETSTELMODULE(StelObjectMgr)->addToExtraInfoString(StelObject::DebugAid, QString("ZL AtmFactor: %1<br/>").arg(QString::number(atmFactor, 'f', 4)));
	Q_ASSERT(atmFactor<=1.0f);
	Q_ASSERT(atmFactor>=0.0f);
	c*=atmFactor*atmFactor;

	if (c[0]<0) c[0]=0;
	if (c[1]<0) c[1]=0;
	if (c[2]<0) c[2]=0;

	renderProgram->bind();

	const bool withExtinction=(drawer->getFlagHasAtmosphere() && drawer->getExtinction().getExtinctionCoefficient()>=0.01f);

	renderProgram->setUniformValue(shaderVars.extinctionEnabled, withExtinction);
	renderProgram->setUniformValue(shaderVars.bortleIntensity, bortleIntensity);

	const int mainTexSampler = 0;
	mainTex->bind(mainTexSampler);
	renderProgram->setUniformValue(shaderVars.mainTex, mainTexSampler);

	renderProgram->setUniformValue(shaderVars.projectionMatrixInverse, projector->getProjectionMatrix().toQMatrix().inverted());
	renderProgram->setUniformValue(shaderVars.brightness, c.toQVector());
	renderProgram->setUniformValue(shaderVars.lambdaSun, GLfloat(lambdaSun));

	projector->setUnProjectUniforms(*renderProgram);
	extinction.setForwardTransformUniforms(*renderProgram);

	auto& gl = *QOpenGLContext::currentContext()->functions();

	gl.glEnable(GL_BLEND);
	gl.glBlendFunc(GL_ONE,GL_ONE); // allow colored sky background
	bindVAO();
	gl.glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	releaseVAO();
	gl.glDisable(GL_BLEND);
}

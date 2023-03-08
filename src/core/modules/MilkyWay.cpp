/*
 * Stellarium
 * Copyright (C) 2002 Fabien Chereau
 * Copyright (C) 2014 Georg Zotti (extinction parts)
 * Copyright (C) 2022 Ruslan Kabatsayev
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
#include "Dithering.hpp"
#include "StelOpenGLArray.hpp"
#include "SaturationShader.hpp"
#include "StelTextureMgr.hpp"
#include "StelSkyDrawer.hpp"
#include "StelPainter.hpp"
#include "StelTranslator.hpp"
#include "StelModuleMgr.hpp"
#include "LandscapeMgr.hpp"
#include "StelMovementMgr.hpp"
#include "Planet.hpp"
// For debugging draw() only:
// #include "StelObjectMgr.hpp"

#include <QDebug>
#include <QSettings>
#include <QOpenGLShaderProgram>

namespace
{
constexpr int FS_QUAD_COORDS_PER_VERTEX = 2;
constexpr int SKY_VERTEX_ATTRIB_INDEX = 0;
}

// Class which manages the displaying of the Milky Way
MilkyWay::MilkyWay()
	: color(1.f, 1.f, 1.f)
	, intensity(1.)
	, intensityFovScale(1.0)
	, intensityMinFov(0.25) // when zooming in further, MilkyWay is no longer visible.
	, intensityMaxFov(2.5) // when zooming out further, MilkyWay is fully visible (when enabled).
{
	setObjectName("MilkyWay");
	fader = new LinearFader();
}

MilkyWay::~MilkyWay()
{
	delete fader;
	fader = Q_NULLPTR;
}

void MilkyWay::init()
{
	QSettings* conf = StelApp::getInstance().getSettings();
	Q_ASSERT(conf);

	auto& gl = *QOpenGLContext::currentContext()->functions();

	mainTex = StelApp::getInstance().getTextureManager().createTexture(StelFileMgr::getInstallationDir()+"/textures/milkyway.png");
	mainTex->bind(0);
	gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	mainTex->release();
	setFlagShow(conf->value("astro/flag_milky_way").toBool());
	setIntensity(conf->value("astro/milky_way_intensity",1.).toDouble());
	setSaturation(conf->value("astro/milky_way_saturation", 1.).toDouble());

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

	ditherPatternTex = StelApp::getInstance().getTextureManager().getDitheringTexture(0);

	QString displayGroup = N_("Display Options");
	addAction("actionShow_MilkyWay", displayGroup, N_("Milky Way"), "flagMilkyWayDisplayed", "M");
}

void MilkyWay::setupCurrentVAO()
{
	auto& gl = *QOpenGLContext::currentContext()->functions();
	vbo->bind();
	gl.glVertexAttribPointer(0, FS_QUAD_COORDS_PER_VERTEX, GL_FLOAT, false, 0, 0);
	vbo->release();
	gl.glEnableVertexAttribArray(SKY_VERTEX_ATTRIB_INDEX);
}

void MilkyWay::bindVAO()
{
	if(vao->isCreated())
		vao->bind();
	else
		setupCurrentVAO();
}

void MilkyWay::releaseVAO()
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

	const auto modelView = core->getJ2000ModelViewTransform();
	const auto projector = core->getProjection(modelView);
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
			core->getAberrationShader() +
			extinction.getForwardTransformShader() +
			makeDitheringShader()+
			makeSaturationShader()+
			R"(
VARYING highp vec3 ndcPos;
uniform sampler2D mainTex;
uniform lowp float saturation;
uniform vec3 brightness;
uniform mat4 projectionMatrixInverse;
uniform bool extinctionEnabled;
uniform float bortleIntensity;
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
	modelPos = applyAberrationToViewDir(modelPos);
	float modelZenithAngle = acos(-modelPos.z);
	float modelLongitude = atan(modelPos.x, modelPos.y);
	vec2 texc = vec2(modelLongitude/(2.*PI), modelZenithAngle/PI);

	float extinctionFactor;
	if(extinctionEnabled)
	{
		vec3 worldPos = winPosToWorldPos(winPos.x, winPos.y, ok);
		vec3 altAzViewDir = worldPosToAltAzPos(worldPos);
		float mag = extinctionMagnitude(altAzViewDir);
		extinctionFactor=pow(0.3, mag) * (1.1-bortleIntensity*0.1); // drop of one magnitude: should be factor 2.5 or 40%. We take 30%, it looks more realistic.
	}
	else
	{
		extinctionFactor = 1.;
	}

    vec4 color = texture2D(mainTex, texc)*vec4(brightness,1)*extinctionFactor;
	if(saturation != 1.0)
		color.rgb = saturate(color.rgb, saturation);
	FRAG_COLOR = dither(color);
}
)";
		ok = renderProgram->addShaderFromSourceCode(QOpenGLShader::Fragment, frag);
		if(!renderProgram->log().isEmpty())
			qWarning().noquote() << "MilkyWay: Warnings while compiling fragment shader:\n" << renderProgram->log();

		if(!ok) return;

		renderProgram->bindAttributeLocation("vertex", SKY_VERTEX_ATTRIB_INDEX);

		if(!StelPainter::linkProg(renderProgram.get(), "Milky Way render program"))
			return;

		renderProgram->bind();
		shaderVars.mainTex          = renderProgram->uniformLocation("mainTex");
		shaderVars.saturation       = renderProgram->uniformLocation("saturation");
		shaderVars.brightness       = renderProgram->uniformLocation("brightness");
		shaderVars.rgbMaxValue      = renderProgram->uniformLocation("rgbMaxValue");
		shaderVars.ditherPattern    = renderProgram->uniformLocation("ditherPattern");
		shaderVars.bortleIntensity  = renderProgram->uniformLocation("bortleIntensity");
		shaderVars.extinctionEnabled= renderProgram->uniformLocation("extinctionEnabled");
		shaderVars.projectionMatrixInverse = renderProgram->uniformLocation("projectionMatrixInverse");
		renderProgram->release();
	}
	if(!renderProgram || !renderProgram->isLinked())
		return;

	StelToneReproducer* eye = core->getToneReproducer();

	Q_ASSERT(mainTex);	// A texture must be loaded before calling this

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
	float atmFadeIntensity = GETSTELMODULE(LandscapeMgr)->getAtmosphereFadeIntensity();
	const float nelm = StelCore::luminanceToNELM(drawer->getLightPollutionLuminance());
	const float bortleIntensity = 1.f+(15.5f-2*nelm)*atmFadeIntensity; // smoothed Bortle index moderated by atmosphere fader.

	const float lum = drawer->surfaceBrightnessToLuminance(12.f+0.15f*bortleIntensity); // was 11.5; Source? How to calibrate the new texture?

	// Get the luminance scaled between 0 and 1
	float aLum =eye->adaptLuminanceScaled(lum*fader->getInterstate());

	// Bound a maximum luminance. GZ: Is there any reference/reason, or just trial and error?
	aLum = qMin(0.38f, aLum*2.f);


	// intensity of 1.0 is "proper", but allow boost for dim screens
	c*=aLum*static_cast<float>(intensity*intensityFovScale);

	//StelObjectMgr *omgr=GETSTELMODULE(StelObjectMgr); // Activate for debugging only
	//Q_ASSERT(omgr);
	// TODO: Find an even better balance with sky brightness, MW should be hard to see during Full Moon and at least somewhat reduced in smaller phases.
	// adapt brightness by atmospheric brightness. This block developed for ZodiacalLight, hopefully similarly applicable...
	const float atmLum = GETSTELMODULE(LandscapeMgr)->getAtmosphereAverageLuminance();
	// Approximate values for Preetham: 10cd/m^2 at sunset, 3.3 at civil twilight (sun at -6deg). 0.0145 sun at -12, 0.0004 sun at -18,  0.01 at Full Moon!?
	//omgr->setExtraInfoString(StelObject::DebugAid, QString("AtmLum: %1<br/>").arg(QString::number(atmLum, 'f', 4)));
	// The atmLum of Bruneton's model is about 1/2 higher than that of Preetham/Schaefer. We must rebalance that!
	float atmFactor=0.35;
	if (GETSTELMODULE(LandscapeMgr)->getAtmosphereModel()=="showmysky")
	{
		atmFactor=qMax(0.35f, 50.0f*(0.02f-0.2f*atmLum)); // The factor 0.2f was found empirically. Nominally it should be 0.667, but 0.2 or at least 0.4 looks better.
	}
	else
	{
		atmFactor=qMax(0.35f, 50.0f*(0.02f-atmLum)); // keep visible in twilight, but this is enough for some effect with the moon.
	}
	//omgr->addToExtraInfoString(StelObject::DebugAid, QString("AtmFactor: %1<br/>").arg(QString::number(atmFactor, 'f', 4)));
	c*=atmFactor*atmFactor;

	if (c[0]<0) c[0]=0;
	if (c[1]<0) c[1]=0;
	if (c[2]<0) c[2]=0;

	renderProgram->bind();

	renderProgram->setUniformValue(shaderVars.extinctionEnabled, drawer->getFlagHasAtmosphere() && extinction.getExtinctionCoefficient()>=0.01f);
	renderProgram->setUniformValue(shaderVars.bortleIntensity, bortleIntensity);

	const int mainTexSampler = 0;
	mainTex->bind(mainTexSampler);
	renderProgram->setUniformValue(shaderVars.mainTex, mainTexSampler);

	const int ditherTexSampler = 1;
	ditherPatternTex->bind(ditherTexSampler);
	renderProgram->setUniformValue(shaderVars.ditherPattern, ditherTexSampler);

	renderProgram->setUniformValue(shaderVars.projectionMatrixInverse, projector->getProjectionMatrix().toQMatrix().inverted());
	renderProgram->setUniformValue(shaderVars.brightness, c.toQVector());
	renderProgram->setUniformValue(shaderVars.saturation, GLfloat(saturation));
	renderProgram->setUniformValue(shaderVars.rgbMaxValue, calcRGBMaxValue(core->getDitheringMode()).toQVector());

	core->setAberrationUniforms(*renderProgram);
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

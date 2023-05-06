/*
 * Stellarium
 * Copyright (C) 2019 Ruslan Kabatsayev
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

#ifdef ENABLE_SHOWMYSKY

#include "AtmosphereShowMySky.hpp"
#include "StelUtils.hpp"
#include "Planet.hpp"
#include "StelApp.hpp"
#include "StelProjector.hpp"
#include "StelToneReproducer.hpp"
#include "StelTextureMgr.hpp"
#include "StelCore.hpp"
#include "StelPainter.hpp"
#include "Dithering.hpp"
#include "StelTranslator.hpp"
#include "TextureAverageComputer.hpp"

#include <cassert>
#include <cstring>

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QDebug>
#include <QVector>
#include <QSettings>
#include <QOpenGLShaderProgram>
#include <QOpenGLFunctions_3_3_Core>
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
# include <QOpenGLVersionFunctionsFactory>
#endif

// ShowMySky library API is documented online at https://10110111.github.io/CalcMySky/showmysky-api.html
// Or you can build the documentation from the CalcMySky sources, using CMake doc target: `cmake --build . --target doc`

namespace
{

constexpr float nonExtinctedSolarMagnitude=-26.74;

struct SkySettings : ShowMySky::Settings, QObject
{
	double altitude() override { return altitude_; }
	double sunAzimuth() override { return sunAzimuth_; }
	double sunZenithAngle() override { return sunZenithAngle_; }
	double sunAngularRadius() override { return sunAngularRadius_; }
	double moonAzimuth() override { return moonAzimuth_; }
	double moonZenithAngle() override { return moonZenithAngle_; }
	double earthMoonDistance() override { return earthMoonDistance_; }

	bool zeroOrderScatteringEnabled() override { init(); return zeroOrderScatteringEnabled_; }
	bool singleScatteringEnabled   () override { init(); return singleScatteringEnabled_; }
	bool multipleScatteringEnabled () override { init(); return multipleScatteringEnabled_; }

	double lightPollutionGroundLuminance() override { return lightPollutionGroundLuminance_; }

	bool onTheFlySingleScatteringEnabled       () override { return onTheFlySingleScatteringEnabled_; }
	bool onTheFlyPrecompDoubleScatteringEnabled() override { return onTheFlyPrecompDoubleScatteringEnabled_; }

	bool usingEclipseShader() override { return useEclipseShader_; }
	bool pseudoMirrorEnabled() override { init(); return pseudoMirrorEnabled_; }

	double altitude_ = 0;
	double sunAzimuth_ = 0;
	double sunZenithAngle_ = M_PI/4;
	double sunAngularRadius_ = 0.264 * M_PI/180;
	double moonAzimuth_ = 0;
	double moonZenithAngle_ = -M_PI/2;
	double earthMoonDistance_ = 400e6;

	double lightPollutionGroundLuminance_ = 0;

	bool onTheFlySingleScatteringEnabled_ = false;
	bool onTheFlyPrecompDoubleScatteringEnabled_ = false;
	bool zeroOrderScatteringEnabled_ = false;
	bool singleScatteringEnabled_ = true;
	bool multipleScatteringEnabled_ = true;
	bool pseudoMirrorEnabled_ = true;
	bool useEclipseShader_ = false;

	// These variables are not used by AtmosphereRenderer, but it's convenient to keep them here
	QMatrix4x4 projectionMatrixInverse_;

private:
	static constexpr const char* zeroOrderScatteringPropName() { return "LandscapeMgr.flagAtmosphereZeroOrderScattering"; }
	static constexpr const char* singleScatteringPropName()    { return "LandscapeMgr.flagAtmosphereSingleScattering"; }
	static constexpr const char* multipleScatteringPropName()  { return "LandscapeMgr.flagAtmosphereMultipleScattering"; }
	bool inited=false; // We have to lazy initialize because at the time of construction, LandscapeMgr isn't registered with StelPropertyMgr
	void init()
	{
		if(inited) return;

		const auto propMan = StelApp::getInstance().getStelPropertyManager();
		{
			const auto prop = propMan->getProperty(zeroOrderScatteringPropName());
			QObject::connect(prop, &StelProperty::changed, this, [this](const QVariant& v) { zeroOrderScatteringEnabled_=v.toBool(); });
			zeroOrderScatteringEnabled_ = prop->getValue().toBool();
		}
		{
			const auto prop = propMan->getProperty(singleScatteringPropName());
			QObject::connect(prop, &StelProperty::changed, this, [this](const QVariant& v) { singleScatteringEnabled_=v.toBool(); });
			singleScatteringEnabled_ = prop->getValue().toBool();
		}
		{
			const auto prop = propMan->getProperty(multipleScatteringPropName());
			QObject::connect(prop, &StelProperty::changed, this, [this](const QVariant& v) { multipleScatteringEnabled_=v.toBool(); });
			multipleScatteringEnabled_ = prop->getValue().toBool();
		}
		inited=true;
	}
};

QOpenGLFunctions_3_3_Core* glfuncs()
{
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
	return QOpenGLVersionFunctionsFactory::get<QOpenGLFunctions_3_3_Core>(QOpenGLContext::currentContext());
#else
	return QOpenGLContext::currentContext()->versionFunctions<QOpenGLFunctions_3_3_Core>();
#endif
}

double sqr(double x) { return x*x; }

/*
   R1,R2 - radii of the circles
   d - distance between centers of the circles
   returns area of intersection of these circles
 */
double circlesIntersectionArea(double R1, double R2, double d)
{
	using namespace std;
	if(d+min(R1,R2)<max(R1,R2)) return M_PI*sqr(min(R1,R2));
	if(d>=R1+R2) return 0;

	// Return area of the lens with radii R1 and R2 and offset d
	return sqr(R1)*acos(clamp( (sqr(d)+sqr(R1)-sqr(R2))/(2*d*R1) ,-1.,1.)) +
	       sqr(R2)*acos(clamp( (sqr(d)+sqr(R2)-sqr(R1))/(2*d*R2) ,-1.,1.)) -
	       0.5*sqrt(max( (-d+R1+R2)*(d+R1-R2)*(d-R1+R2)*(d+R1+R2) ,0.));
}

double visibleSolidAngleOfSun(const double sunAngularRadius, const double moonAngularRadius, const double angleBetweenSunAndMoon)
{
	const double Rs = sunAngularRadius;
	const double Rm = moonAngularRadius;
	double visibleSolidAngle = M_PI*sqr(Rs);

	const double dSM = angleBetweenSunAndMoon;
	if(dSM < Rs+Rm)
	{
		visibleSolidAngle -= circlesIntersectionArea(Rm,Rs,dSM);
	}

	return visibleSolidAngle;
}

double sunVisibilityDueToMoon(const double sunAngularRadius, const double moonAngularRadius, const double angleBetweenSunAndMoon)
{
	return visibleSolidAngleOfSun(sunAngularRadius, moonAngularRadius, angleBetweenSunAndMoon)/(M_PI*sqr(sunAngularRadius));
}

constexpr GLuint SKY_VERTEX_ATTRIB_INDEX=0;
}

void AtmosphereShowMySky::initProperties()
{
	if (propertiesInited_) return;

	const auto prop = StelApp::getInstance().getStelPropertyManager()->getProperty("LandscapeMgr.atmosphereEclipseSimulationQuality");
	auto updateEclipseSimQuality = [this](const QVariant& v)
	{
		const auto n = std::max(0, v.toInt());
		switch(n)
		{
		case 0:
			eclipseSimulationQuality_ = EclipseSimulationQuality::Fastest;
			break;
		case 1:
			eclipseSimulationQuality_ = EclipseSimulationQuality::AllPrecomputed;
			break;
		case 2:
			eclipseSimulationQuality_ = EclipseSimulationQuality::Order2OnTheFly_Order1Precomp;
			break;
		case 3:
			eclipseSimulationQuality_ = EclipseSimulationQuality::AllOnTheFly;
			break;
		default: // all larger values mean highest quality
			eclipseSimulationQuality_ = EclipseSimulationQuality::AllOnTheFly;
			break;
		}
	};
	QObject::connect(prop, &StelProperty::changed, this, updateEclipseSimQuality);
	updateEclipseSimQuality(prop->getValue());

	propertiesInited_ = true;
}

std::pair<QByteArray,QByteArray> AtmosphereShowMySky::getViewDirShaderSources(const StelProjector& projector) const
{
	static constexpr char viewDirVertShaderSrc[]=R"(
#version 330
in vec3 vertex;
out vec3 ndcPos;
void main()
{
	gl_Position = vec4(vertex, 1.);
	ndcPos = vertex;
}
)";
	const auto viewDirFragShaderSrc =
		"#version 330\n" +
		projector.getUnProjectShader() +
		R"(
in vec3 ndcPos;
uniform mat4 projectionMatrixInverse;
uniform bool isZenithProbe;
vec3 calcViewDir()
{
	if(isZenithProbe) return vec3(0,0,1);

	const float PI = 3.14159265;
	vec4 winPos = projectionMatrixInverse * vec4(ndcPos, 1);
	bool ok = false;
	vec3 modelPos = unProject(winPos.x, winPos.y, ok).xyz;

//	if(!ok) return vec3(0);

	return normalize(modelPos);
}
)";
	return {viewDirVertShaderSrc, viewDirFragShaderSrc};
}

void AtmosphereShowMySky::loadShaders()
{
	const auto handleCompileStatus=[](bool success, QOpenGLShader const& shader, const char* what)
	{
		if(!success)
		{
			qCritical("Error while compiling %s: %s", what, shader.log().toLatin1().constData());
			throw InitFailure("Shader compilation failed");
		}
		if(!shader.log().isEmpty())
		{
			qWarning("Warnings while compiling %s: %s", what, shader.log().toLatin1().constData());
		}
	};

	// Shader program that converts XYZW texture to sRGB image
	{
		static constexpr char vShaderSrc[]=R"(
#version 330
in vec4 vertex;
out vec2 texCoord;
void main()
{
	gl_Position=vertex;
	texCoord=vertex.xy*0.5+0.5;
}
)";
		static constexpr char fShaderSrc[]=R"(
#version 330

uniform sampler2D luminanceXYZW;

in vec2 texCoord;
out vec4 color;

vec3 dither(vec3);
vec3 xyYToRGB(float x, float y, float Y);

void main()
{
	vec3 XYZ=texture(luminanceXYZW, texCoord).xyz;
	vec3 srgb=xyYToRGB(XYZ.x/(XYZ.x+XYZ.y+XYZ.z), XYZ.y/(XYZ.x+XYZ.y+XYZ.z), XYZ.y);
	color=vec4(dither(srgb),1);
}
)";

		QOpenGLShader vShader(QOpenGLShader::Vertex);
		handleCompileStatus(vShader.compileSourceCode(vShaderSrc), vShader,
							"ShowMySky atmosphere luminance-to-screen vertex shader");
		luminanceToScreenProgram_->addShader(&vShader);

		QOpenGLShader ditherShader(QOpenGLShader::Fragment);
		const auto fPrefix = StelOpenGL::globalShaderPrefix(StelOpenGL::FRAGMENT_SHADER);
		handleCompileStatus(ditherShader.compileSourceCode(fPrefix + makeDitheringShader()),
							ditherShader, "ShowMySky atmosphere dithering shader");
		luminanceToScreenProgram_->addShader(&ditherShader);

		QOpenGLShader toneReproducerShader(QOpenGLShader::Fragment);
		auto xyYToRGBFile = QFile(":/shaders/xyYToRGB.glsl");
		if(!xyYToRGBFile.open(QFile::ReadOnly))
			throw InitFailure("Failed to open atmosphere tone reproducer fragment shader file");
		const auto xyYToRGBShader = xyYToRGBFile.readAll();
		handleCompileStatus(toneReproducerShader.compileSourceCode(fPrefix + xyYToRGBShader),
							toneReproducerShader, "ShowMySky atmosphere tone reproducer fragment shader");
		luminanceToScreenProgram_->addShader(&toneReproducerShader);

		QOpenGLShader luminanceToScreenShader(QOpenGLShader::Fragment);
		handleCompileStatus(luminanceToScreenShader.compileSourceCode(fShaderSrc), luminanceToScreenShader,
							"ShowMySky atmosphere luminance-to-screen fragment shader");
		luminanceToScreenProgram_->addShader(&luminanceToScreenShader);

		if(!StelPainter::linkProg(luminanceToScreenProgram_.get(), "atmosphere luminance-to-screen"))
			throw InitFailure("Shader program linking failed");
	}
	const auto& core = StelApp::getInstance().getCore();
	const auto projector = core->getProjection(StelCore::FrameAltAz, StelCore::RefractionOff);
	const auto shaderSources = getViewDirShaderSources(*projector);
	renderer_->initDataLoading(shaderSources.first, shaderSources.second,
							   {std::pair<std::string,GLuint>{"vertex", SKY_VERTEX_ATTRIB_INDEX}});
}

void AtmosphereShowMySky::resizeRenderTarget(int width, int height)
{
	const int physWidth = width/atmoRes;
	const int physHeight = height/atmoRes;
	renderer_->resizeEvent(physWidth, physHeight);
	textureAverager_.reset(new TextureAverageComputer(*glfuncs(), physWidth, physHeight, GL_RGBA32F));

	prevWidth_=width;
	prevHeight_=height;
}

void AtmosphereShowMySky::setupRenderTarget()
{
	auto& gl = *glfuncs();

	GLint viewport[4];
	GL(gl.glGetIntegerv(GL_VIEWPORT, viewport));
	resizeRenderTarget(viewport[2], viewport[3]);

	// force resize on first render because the renderer may change framebuffer size after data loading
	prevWidth_=0;
}

void AtmosphereShowMySky::setupBuffers()
{
	auto& gl = *glfuncs();

	GL(gl.glGenBuffers(1, &vbo_));
	GL(gl.glBindBuffer(GL_ARRAY_BUFFER, vbo_));
	const GLfloat vertices[]=
	{
		// full screen quad
		-1, -1,
		 1, -1,
		-1,  1,
		 1,  1,
	};
	constexpr int FS_QUAD_COORDS_PER_VERTEX = 2;
	constexpr auto FS_QUAD_OFFSET_IN_VBO = 0;
	GL(gl.glBufferData(GL_ARRAY_BUFFER, sizeof vertices, vertices, GL_STATIC_DRAW));

	{
		GL(gl.glGenVertexArrays(1, &mainVAO_));
		GL(gl.glBindVertexArray(mainVAO_));
		GL(gl.glVertexAttribPointer(0, FS_QUAD_COORDS_PER_VERTEX, GL_FLOAT, false, 0,
									reinterpret_cast<const void*>(FS_QUAD_OFFSET_IN_VBO)));
		GL(gl.glEnableVertexAttribArray(SKY_VERTEX_ATTRIB_INDEX));
	}
	{
		GL(gl.glGenVertexArrays(1, &zenithProbeVAO_));
		GL(gl.glBindVertexArray(zenithProbeVAO_));
		GL(gl.glVertexAttribPointer(SKY_VERTEX_ATTRIB_INDEX, FS_QUAD_COORDS_PER_VERTEX, GL_FLOAT, false, 0,
									reinterpret_cast<const void*>(FS_QUAD_OFFSET_IN_VBO)));
		GL(gl.glEnableVertexAttribArray(SKY_VERTEX_ATTRIB_INDEX));
	}

	GL(gl.glBindVertexArray(0));
	GL(gl.glBindBuffer(GL_ARRAY_BUFFER, 0));
}

void AtmosphereShowMySky::resolveFunctions()
{
	if(!showMySkyLib.load())
		throw InitFailure(q_("Failed to load ShowMySky library: %1").arg(showMySkyLib.errorString()));
	ShowMySky_AtmosphereRenderer_create=reinterpret_cast<decltype(ShowMySky_AtmosphereRenderer_create)>(
												showMySkyLib.resolve("ShowMySky_AtmosphereRenderer_create"));
	if(!ShowMySky_AtmosphereRenderer_create)
		throw InitFailure(q_("Failed to resolve the function to create AtmosphereRenderer"));

	const auto abi=reinterpret_cast<const quint32*>(showMySkyLib.resolve("ShowMySky_ABI_version"));
	if(!abi)
		throw InitFailure(q_("Failed to determine ABI version of ShowMySky library."));
	if(*abi != ShowMySky_ABI_version)
		throw InitFailure(q_("ABI version of ShowMySky library is %1, but this program has been compiled against version %2.")
		                  .arg(*abi).arg(ShowMySky_ABI_version));
}

AtmosphereShowMySky::AtmosphereShowMySky(const double initialAltitude)
#ifdef SHOWMYSKY_LIB_NAME
	: showMySkyLib(SHOWMYSKY_LIB_NAME, ShowMySky_ABI_version)
#else // for compatibility with legacy unversioned naming
	: showMySkyLib("ShowMySky")
#endif
	, viewport(0,0,0,0)
	, luminanceToScreenProgram_(new QOpenGLShaderProgram())
{
	StelOpenGL::checkGLErrors(__FILE__,__LINE__);

	setFadeDuration(1.5);

	const auto& conf=*StelApp::getInstance().getSettings();
	reducedResolution = conf.value("landscape/atmosphere_resolution_reduction", 1).toInt();
	flagDynamicResolution = conf.value("landscape/flag_atmosphere_dynamic_resolution", false).toBool();
	if (!flagDynamicResolution)
	{
		atmoRes = reducedResolution;
		if (reducedResolution>1)
			qDebug() << "Atmosphere runs with statically reduced resolution:" << reducedResolution;
	}

	resolveFunctions();
	try
	{
		const auto defaultPath = QDir::homePath() + "/cms";
		const auto pathToData = conf.value("landscape/atmosphere_model_path", defaultPath).toString();
		const auto gl = glfuncs();
		if(!gl)
			throw InitFailure(q_("Failed to get OpenGL 3.3 support functions"));

		qDebug() << "Will load CalcMySky atmosphere model from" << pathToData;
		skySettings_.reset(new SkySettings);
		auto& settings = *static_cast<SkySettings*>(skySettings_.get());
		settings.altitude_ = initialAltitude;
		renderSurfaceFunc_=[this](QOpenGLShaderProgram& prog)
		{
			const auto& settings = *static_cast<SkySettings*>(skySettings_.get());
			prog.setUniformValue("projectionMatrixInverse", settings.projectionMatrixInverse_);
			prog.setUniformValue("isZenithProbe", false);
			const auto& core = StelApp::getInstance().getCore();
			const auto projector = core->getProjection(StelCore::FrameAltAz, StelCore::RefractionOff);
			projector->setUnProjectUniforms(prog);

			auto& gl = *glfuncs();

			GL(gl.glViewport(viewport[0], viewport[1], viewport[2]/atmoRes, viewport[3]/atmoRes));
			GL(gl.glBindVertexArray(mainVAO_));
			GL(gl.glDrawArrays(GL_TRIANGLE_STRIP, 0, 4));
			GL(gl.glBindVertexArray(0));
			GL(gl.glViewport(viewport[0], viewport[1], viewport[2], viewport[3]));
		};
		renderer_.reset(ShowMySky_AtmosphereRenderer_create(gl, &pathToData, skySettings_.get(), &renderSurfaceFunc_));
		loadShaders();
		setupRenderTarget();
		setupBuffers();
	}
	catch(ShowMySky::Error const& error)
	{
		throw InitFailure(error.what());
	}
	{
		auto& prog=*luminanceToScreenProgram_;
		prog.bind();

		shaderAttribLocations.doSRGB                 = prog.uniformLocation("doSRGB");
		shaderAttribLocations.rgbMaxValue            = prog.uniformLocation("rgbMaxValue");
		shaderAttribLocations.ditherPattern          = prog.uniformLocation("ditherPattern");
		shaderAttribLocations.oneOverGamma           = prog.uniformLocation("oneOverGamma");
		shaderAttribLocations.brightnessScale        = prog.uniformLocation("brightnessScale");
		shaderAttribLocations.luminanceTexture       = prog.uniformLocation("luminance");
		shaderAttribLocations.alphaWaOverAlphaDa     = prog.uniformLocation("alphaWaOverAlphaDa");
		shaderAttribLocations.term2TimesOneOverMaxdL = prog.uniformLocation("term2TimesOneOverMaxdL");
		shaderAttribLocations.term2TimesOneOverMaxdLpOneOverGamma
		                                             = prog.uniformLocation("term2TimesOneOverMaxdLpOneOverGamma");
		shaderAttribLocations.flagUseTmGamma         = prog.uniformLocation("flagUseTmGamma");

		prog.release();
	}
}

AtmosphereShowMySky::~AtmosphereShowMySky()
{
	if(auto*const ctx=QOpenGLContext::currentContext())
	{
		auto& gl = *glfuncs();
		GL(gl.glDeleteBuffers(1, &vbo_));
		GL(gl.glDeleteVertexArrays(1, &mainVAO_));
		GL(gl.glDeleteVertexArrays(1, &zenithProbeVAO_));
	}
}

void AtmosphereShowMySky::probeZenithLuminances(const float altitude)
{
	StelOpenGL::checkGLErrors(__FILE__,__LINE__);
	// Here we'll draw zenith part of the sky into a 1×1 texture in several
	// modes and get the resulting colors to determine the coefficients to
	// render light pollution and airglow correctly

	auto& gl = *glfuncs();

	GLint origScissor[4];
	GL(gl.glGetIntegerv(GL_SCISSOR_BOX, origScissor));

	renderer_->setDrawSurfaceCallback([this](QOpenGLShaderProgram& prog)
	{
		const auto& settings = *static_cast<SkySettings*>(skySettings_.get());
		prog.setUniformValue("projectionMatrixInverse", settings.projectionMatrixInverse_);
		prog.setUniformValue("isZenithProbe", true);
		const auto& core = StelApp::getInstance().getCore();
		const auto projector = core->getProjection(StelCore::FrameAltAz, StelCore::RefractionOff);
		projector->setUnProjectUniforms(prog);

		auto& gl = *glfuncs();
		GL(gl.glBindVertexArray(zenithProbeVAO_));
		GL(gl.glDrawArrays(GL_TRIANGLE_STRIP, 0, 4));
		GL(gl.glBindVertexArray(0));
	});

	const auto unitMat = Mat4f(1,0,0,0,
	                           0,1,0,0,
	                           0,0,1,0,
	                           0,0,0,1);
	GL(gl.glScissor(0,prevHeight_-1,1,1));
	drawAtmosphere(unitMat, 0, M_PI, 0, 0, M_PI, 0, altitude, 1, 1, 0, false, true);
	lightPollutionUnitLuminance_ = renderer_->getPixelLuminance({0,0}).y();
	drawAtmosphere(unitMat, 0, M_PI, 0, 0, M_PI, 0, altitude, 1, 0, 1, false, true);
	airglowUnitLuminance_ = renderer_->getPixelLuminance({0,0}).y();

	renderer_->setDrawSurfaceCallback(renderSurfaceFunc_);
	GL(gl.glScissor(origScissor[0], origScissor[1], origScissor[2], origScissor[3]));
}

void AtmosphereShowMySky::drawAtmosphere(Mat4f const& projectionMatrix, const float sunAzimuth, const float sunZenithAngle,
	                                     const float sunAngularRadius, const float moonAzimuth, const float moonZenithAngle,
	                                     const float earthMoonDistance, const float altitude, const float brightness_,
	                                     const float lightPollutionGroundLuminance, const float airglowRelativeBrightness,
	                                     const bool drawAsEclipse, const bool clearTarget)
{
	StelOpenGL::checkGLErrors(__FILE__,__LINE__);
	Q_UNUSED(airglowRelativeBrightness)
	auto& settings = *static_cast<SkySettings*>(skySettings_.get());
	settings.projectionMatrixInverse_ = projectionMatrix.toQMatrix().inverted();
	settings.altitude_=altitude;
	settings.sunAzimuth_=sunAzimuth;
	settings.sunZenithAngle_=sunZenithAngle;
	settings.sunAngularRadius_=sunAngularRadius;
	settings.moonAzimuth_=moonAzimuth;
	settings.moonZenithAngle_=moonZenithAngle;
	settings.earthMoonDistance_=earthMoonDistance;
	settings.lightPollutionGroundLuminance_=lightPollutionGroundLuminance;

	const double brightness = brightness_; // Silence CodeQL's "Multiplication result converted to larger type"
	if(drawAsEclipse)
	{
		if(eclipseSimulationQuality_ == EclipseSimulationQuality::AllPrecomputed &&
		   !renderer_->canRenderPrecomputedEclipsedDoubleScattering())
		{
			// Fall back to lower quality
			eclipseSimulationQuality_ = EclipseSimulationQuality::Fastest;
		}

		switch(eclipseSimulationQuality_)
		{
		case EclipseSimulationQuality::Fastest:
			settings.useEclipseShader_ = false;
			settings.onTheFlySingleScatteringEnabled_ = false;
			settings.onTheFlyPrecompDoubleScatteringEnabled_ = false;
			renderer_->draw(eclipseFactor*brightness, clearTarget);
			break;
		case EclipseSimulationQuality::AllPrecomputed:
		{
			settings.onTheFlySingleScatteringEnabled_ = false;
			settings.onTheFlyPrecompDoubleScatteringEnabled_ = false;

			if(sunVisibility_ == 0)
			{
				// Total eclipse is well represented in both first and multiple scattering order textures
				settings.useEclipseShader_ = true;
				renderer_->draw(brightness, clearTarget);
				break;
			}

			const bool multipleScatteringWasEnabled = settings.multipleScatteringEnabled_;
			// First order scattering is represented relatively well in the precomputed textures
			settings.useEclipseShader_ = true;
			settings.multipleScatteringEnabled_ = false;
			renderer_->draw(brightness, clearTarget);

			if(multipleScatteringWasEnabled)
			{
				const bool singleScatteringWasEnabled = settings.singleScatteringEnabled_;
				// Multiple scattering texture is only valid for a total eclipse. In case of partial
				// or annular eclipse we need to interpolate this render with the non-eclipsed case.
				settings.singleScatteringEnabled_ = false;
				settings.multipleScatteringEnabled_ = true;
				settings.lightPollutionGroundLuminance_ = 0;
				renderer_->draw((1-sunVisibility_)*brightness, false);
				settings.useEclipseShader_ = false;
				renderer_->draw(sunVisibility_*brightness, false);

				settings.singleScatteringEnabled_ = singleScatteringWasEnabled;
			}
			settings.multipleScatteringEnabled_ = multipleScatteringWasEnabled;
			break;
		}
		case EclipseSimulationQuality::Order2OnTheFly_Order1Precomp:
			settings.useEclipseShader_ = true;
			settings.onTheFlySingleScatteringEnabled_ = false;
			settings.onTheFlyPrecompDoubleScatteringEnabled_ = true;
			renderer_->draw(brightness, clearTarget);
			break;
		case EclipseSimulationQuality::AllOnTheFly:
			settings.useEclipseShader_ = true;
			settings.onTheFlySingleScatteringEnabled_ = true;
			settings.onTheFlyPrecompDoubleScatteringEnabled_ = true;
			renderer_->draw(brightness, clearTarget);
			break;
		}
	}
	else
	{
		settings.useEclipseShader_ = false;
		settings.onTheFlySingleScatteringEnabled_ = false;
		settings.onTheFlyPrecompDoubleScatteringEnabled_ = false;
		renderer_->draw(brightness, clearTarget);
	}
}

Vec4f AtmosphereShowMySky::getMeanPixelValue()
{
	StelOpenGL::checkGLErrors(__FILE__,__LINE__);
	return textureAverager_->getTextureAverage(renderer_->getLuminanceTexture());
}

bool AtmosphereShowMySky::dynamicResolution(StelProjectorP prj, Vec3d &currPos, int width, int height)
{
	if (!flagDynamicResolution)
		return false;

	const auto currFov=prj->getFov(), currFad=fader.getInterstate();
	Vec3d currSun;
	prj->project(currPos,currSun);
	const auto dFad=1e3*(currFad-prevFad);				// per thousand of the fader
	const auto dFov=2e3*(currFov-prevFov)/(currFov+prevFov);	// per thousand of the field of view
	const auto dPos=1e3*(currPos-prevPos).norm();			// milli-AU :)
	const auto dSun=(currSun-prevSun).norm();			// pixel
	const auto changeOfView=Vec4d(dFad,dFov,dPos,dSun);
	const auto allowedChange=eclipseFactor<1?10e-3:1;		// for solar eclipses, prioritize speed over resolution
	const auto hysteresis=atmoRes==1?1:200e-3;			// hysteresis avoids frequent changing of the resolution
	const auto allowedChangeOfView=allowedChange*hysteresis;
	const auto changed=changeOfView.norm()>allowedChangeOfView;	// change is too big
	const auto timeout=dynResTimer<=0;				// do we have a timeout?
	// if we don't have a timeout or too much change, we skip the frame
	if (!changed && !timeout)
	{
		dynResTimer--;						// count down to redraw
		return true;
	}
	// if there is a timeout, we draw with full resolution
	// if the change is too large, we draw with reduced resolution
	atmoRes=timeout?1:reducedResolution;
	if (prevRes!=atmoRes)
	{
		resizeRenderTarget(width, height);
		bool verbose=qApp->property("verbose").toBool();
		if (verbose)
			qDebug() << "dynResTimer" << dynResTimer << "atmoRes" << atmoRes << "changeOfView" << changeOfView.norm() << changeOfView;
	}
	// At reduced resolution, we hurry to redraw - at full resolution, we have time.
	dynResTimer=timeout?17:5;
	prevRes=atmoRes;
	prevFov=currFov;
	prevFad=currFad;
	prevPos=currPos;
	prevSun=currSun;
	return false;
}

void AtmosphereShowMySky::computeColor(StelCore* core, const double JD, const Planet& currentPlanet, const Planet& sun,
				       const Planet*const moon, const StelLocation& location, const float temperature,
				       const float relativeHumidity, const float extinctionCoefficient, const bool noScatter)
{
	StelOpenGL::checkGLErrors(__FILE__,__LINE__);
	try
	{
		Q_UNUSED(JD)
		Q_UNUSED(temperature)
		Q_UNUSED(relativeHumidity)
		Q_UNUSED(extinctionCoefficient)
		initProperties();

		const auto prj = core->getProjection(StelCore::FrameAltAz, StelCore::RefractionOff);

		if(!prevProjector_ || !prj->isSameProjection(*prevProjector_))
		{
			const auto shaderSources = getViewDirShaderSources(*prj);
			renderer_->setViewDirShaders(shaderSources.first, shaderSources.second,
										 {std::pair<std::string,GLuint>{"vertex", SKY_VERTEX_ATTRIB_INDEX}});
			prevProjector_ = prj;
		}

		if (viewport != prj->getViewport())
			viewport = prj->getViewport();
		const auto width=viewport[2], height=viewport[3];
		if(width!=prevWidth_ || height!=prevHeight_)
		{
			resizeRenderTarget(width, height);
			dynResTimer=0;
		}

		if(location.altitude != lastUsedAltitude_)
		{
			lastUsedAltitude_ = location.altitude;
			probeZenithLuminances(location.altitude);
			dynResTimer=0;
		}

		auto sunPos  =  sun.getAltAzPosAuto(core);
		if (std::isnan(sunPos.norm()))
			sunPos.set(0, 0, -1);

		// if we run dynamic resolution mode and don't have a timeout or too much change, we skip the frame
		if (dynamicResolution(prj, sunPos, width, height))
			return;

		const auto sunDir = sunPos / sunPos.norm();
		const double sunAngularRadius = atan(sun.getEquatorialRadius()/sunPos.norm());

		// If we have no moon, just put it into nadir
		Vec3d moonDir{0.,0.,-1.};

		double earthMoonDistance = 0;

		if (moon)
		{
			auto moonPos = moon->getAltAzPosAuto(core);
			if (std::isnan(moonPos.norm()))
				moonPos.set(0, 0, -1);
			moonDir = moonPos / moonPos.norm();

			const double moonAngularRadius = atan(moon->getEquatorialRadius()/moonPos.norm());
			const double separationAngle = std::acos(sunDir.dot(moonDir));  // angle between them

			sunVisibility_ = sunVisibilityDueToMoon(sunAngularRadius, moonAngularRadius, separationAngle);
			const double min = 0.0025; // the sky still glows during the totality
			eclipseFactor = static_cast<float>(min + (1-min)*sunVisibility_);

			const Vec3d earthGeomPos = currentPlanet.getAltAzPosGeometric(core);
			const Vec3d moonGeomPos = moon->getAltAzPosGeometric(core);
			earthMoonDistance = (earthGeomPos - moonGeomPos).norm() * (AU*1000);
		}
		else
		{
			eclipseFactor = 1;
		}
		// TODO: compute eclipse factor also for Lunar eclipses! (lp:#1471546)

		// No need to calculate if not visible
		if (!fader.getInterstate())
		{
			// GZ 20180114: Why did we add light pollution if atmosphere was not visible?????
			// And what is the meaning of 0.001? Approximate contribution of stellar background? Then why is it 0.0001 below???
			averageLuminance = 0.001f;
			return;
		}

		const auto sunAzimuth = std::atan2(sunDir[1], sunDir[0]);
		const auto sunZenithAngle = std::acos(sunDir[2]);
		const auto moonAzimuth = std::atan2(moonDir[1], moonDir[0]);
		const auto moonZenithAngle = std::acos(moonDir[2]);

		const auto lightPollutionRelativeBrightness = fader.getInterstate() * lightPollutionLuminance / lightPollutionUnitLuminance_;
		const auto airglowRelativeBrightness = 1.f;
		const auto sunRelativeBrightness = noScatter ? 0.f : 1.f;
		drawAtmosphere(prj->getProjectionMatrix(), sunAzimuth, sunZenithAngle, sunAngularRadius, moonAzimuth, moonZenithAngle, earthMoonDistance,
					   location.altitude, sunRelativeBrightness, lightPollutionRelativeBrightness, airglowRelativeBrightness,
					   eclipseFactor < 1, true);
		const auto nonExtLunarMagnitude = moon ? moon->getVMagnitude(core) : 100.f;
		const auto moonRelativeBrightness = noScatter ? 0.f : std::pow(10.f, 0.4f*(nonExtinctedSolarMagnitude-nonExtLunarMagnitude));
		drawAtmosphere(prj->getProjectionMatrix(), moonAzimuth, moonZenithAngle, 0, 0, M_PI, 0, location.altitude,
					   moonRelativeBrightness, 0, 0, false, false);

		if (!overrideAverageLuminance)
		{
			const auto meanPixelValue=getMeanPixelValue();
			const auto meanY=meanPixelValue[1];
			Q_ASSERT(std::isfinite(meanY));

			averageLuminance = meanY+0.0001f; // Add (assumed) star background luminance
		}
	}
	catch(ShowMySky::Error const& error)
	{
		throw InitFailure(error.what());
	}
}

void AtmosphereShowMySky::draw(StelCore* core)
{
	StelOpenGL::checkGLErrors(__FILE__,__LINE__);
	if (StelApp::getInstance().getVisionModeNight())
		return;

	StelToneReproducer* eye = core->getToneReproducer();

	const float atm_intensity = fader.getInterstate();
	if (!atm_intensity)
		return;

	GL(luminanceToScreenProgram_->bind());

	float a, b, c, d;
	bool useTmGamma, sRGB;
	eye->getShadersParams(a, b, c, d, useTmGamma, sRGB);

	GL(luminanceToScreenProgram_->setUniformValue(shaderAttribLocations.alphaWaOverAlphaDa, a));
	GL(luminanceToScreenProgram_->setUniformValue(shaderAttribLocations.oneOverGamma, b));
	GL(luminanceToScreenProgram_->setUniformValue(shaderAttribLocations.term2TimesOneOverMaxdLpOneOverGamma, c));
	GL(luminanceToScreenProgram_->setUniformValue(shaderAttribLocations.term2TimesOneOverMaxdL, d));
	GL(luminanceToScreenProgram_->setUniformValue(shaderAttribLocations.doSRGB, true));
	GL(luminanceToScreenProgram_->setUniformValue(shaderAttribLocations.flagUseTmGamma, false));
	GL(luminanceToScreenProgram_->setUniformValue(shaderAttribLocations.brightnessScale, atm_intensity));

	StelPainter sPainter(core->getProjection2d());
	sPainter.setBlending(true, GL_ONE, GL_ONE);

	const auto rgbMaxValue=calcRGBMaxValue(core->getDitheringMode());
	GL(luminanceToScreenProgram_->setUniformValue(shaderAttribLocations.rgbMaxValue, rgbMaxValue[0], rgbMaxValue[1], rgbMaxValue[2]));

	auto& gl = *glfuncs();
	GL(gl.glActiveTexture(GL_TEXTURE0));
	GL(gl.glBindTexture(GL_TEXTURE_2D, renderer_->getLuminanceTexture()));
	GL(gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
	GL(luminanceToScreenProgram_->setUniformValue(shaderAttribLocations.luminanceTexture, 0));

	const int ditherTexSampler = 1;
	if(!ditherPatternTex_)
		ditherPatternTex_ = StelApp::getInstance().getTextureManager().getDitheringTexture(ditherTexSampler);
	else
		GL(ditherPatternTex_->bind(ditherTexSampler));
	GL(luminanceToScreenProgram_->setUniformValue(shaderAttribLocations.ditherPattern, ditherTexSampler));

	GL(gl.glBindVertexArray(mainVAO_));
	GL(gl.glDrawArrays(GL_TRIANGLE_STRIP, 0, 4));
	GL(gl.glBindVertexArray(0));

	GL(luminanceToScreenProgram_->release());
}

bool AtmosphereShowMySky::isLoading() const
{
	return renderer_->isLoading();
}

bool AtmosphereShowMySky::isReadyToRender() const
{
	return renderer_->isReadyToRender();
}

auto AtmosphereShowMySky::stepDataLoading() -> LoadingStatus
{
	StelOpenGL::checkGLErrors(__FILE__,__LINE__);
	try
	{
		const auto status = renderer_->stepDataLoading();
		return {status.stepsDone, status.stepsToDo};
	}
	catch(ShowMySky::Error const& error)
	{
		throw InitFailure(error.what());
	}
}

#endif // ENABLE_SHOWMYSKY

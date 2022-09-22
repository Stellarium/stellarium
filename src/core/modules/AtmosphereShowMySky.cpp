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
#include "StelCore.hpp"
#include "StelPainter.hpp"
#include "Dithering.hpp"
#include "StelTranslator.hpp"

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
	QMatrix4x4 projectionMatrix_;

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

QOpenGLFunctions_3_3_Core& glfuncs()
{
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
	return *QOpenGLVersionFunctionsFactory::get<QOpenGLFunctions_3_3_Core>(QOpenGLContext::currentContext());
#else
	return *QOpenGLContext::currentContext()->versionFunctions<QOpenGLFunctions_3_3_Core>();
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

constexpr GLuint SKY_VERTEX_ATTRIB_INDEX=0, VIEW_RAY_ATTRIB_INDEX=1;
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
		handleCompileStatus(ditherShader.compileSourceCode(makeDitheringShader()), ditherShader,
							"ShowMySky atmosphere dithering shader");
		luminanceToScreenProgram_->addShader(&ditherShader);

		QOpenGLShader toneReproducerShader(QOpenGLShader::Fragment);
		handleCompileStatus(toneReproducerShader.compileSourceFile(":/shaders/xyYToRGB.glsl"),
							toneReproducerShader, "ShowMySky atmosphere tone reproducer fragment shader");
		luminanceToScreenProgram_->addShader(&toneReproducerShader);

		QOpenGLShader luminanceToScreenShader(QOpenGLShader::Fragment);
		handleCompileStatus(luminanceToScreenShader.compileSourceCode(fShaderSrc), luminanceToScreenShader,
							"ShowMySky atmosphere luminance-to-screen fragment shader");
		luminanceToScreenProgram_->addShader(&luminanceToScreenShader);

		if(!StelPainter::linkProg(luminanceToScreenProgram_.get(), "atmosphere luminance-to-screen"))
			throw InitFailure("Shader program linking failed");
	}

	{
		static constexpr char viewDirVertShaderSrc[]=R"(
#version 330
uniform mat4 projectionMatrix;

in vec2 skyVertex;
in vec4 viewRay;

out vec3 viewDirUnnormalized;

void main()
{
	viewDirUnnormalized = viewRay.xyz;
	gl_Position = projectionMatrix*vec4(skyVertex, 0., 1.);
}
)";
		static constexpr char viewDirFragShaderSrc[]=R"(
#version 330
in vec3 viewDirUnnormalized;
vec3 calcViewDir()
{
	return normalize(viewDirUnnormalized);
}
)";
		renderer_->initDataLoading(viewDirVertShaderSrc, viewDirFragShaderSrc,
								   {std::pair<std::string,GLuint>{"viewRay", VIEW_RAY_ATTRIB_INDEX},
								    std::pair<std::string,GLuint>{"skyVertex", SKY_VERTEX_ATTRIB_INDEX}});
	}
}

void AtmosphereShowMySky::resizeRenderTarget(int width, int height)
{
	renderer_->resizeEvent(width/atmoRes, height/atmoRes);

	prevWidth_=width;
	prevHeight_=height;
}

void AtmosphereShowMySky::setupRenderTarget()
{
	auto& gl=glfuncs();

	GLint viewport[4];
	GL(gl.glGetIntegerv(GL_VIEWPORT, viewport));
	resizeRenderTarget(viewport[2], viewport[3]);

	prevWidth_=0;				// suspicious workaround :)
}

void AtmosphereShowMySky::setupBuffers()
{
	auto& gl=glfuncs();

	GL(gl.glGenBuffers(1, &vbo_));
	GL(gl.glBindBuffer(GL_ARRAY_BUFFER, vbo_));
	const GLfloat vertices[]=
	{
		// full screen quad
		-1, -1,
		 1, -1,
		-1,  1,
		 1,  1,
		// view ray for zenith probe (all to zenith)
		0, 0, 1, 1,
		0, 0, 1, 1,
		0, 0, 1, 1,
		0, 0, 1, 1,
	};
	constexpr int FS_QUAD_COORDS_PER_VERTEX = 2, VIEW_RAY_COORDS_PER_VERTEX = 4;
	constexpr int NUM_FS_QUAD_VERTICES = 4;
	constexpr auto FS_QUAD_OFFSET_IN_VBO = 0;
	constexpr auto VIEW_RAYS_OFFSET_IN_VBO = NUM_FS_QUAD_VERTICES * FS_QUAD_COORDS_PER_VERTEX * sizeof vertices[0];
	GL(gl.glBufferData(GL_ARRAY_BUFFER, sizeof vertices, vertices, GL_STATIC_DRAW));

	{
		GL(gl.glGenVertexArrays(1, &luminanceToScreenVAO_));
		GL(gl.glBindVertexArray(luminanceToScreenVAO_));
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
		GL(gl.glVertexAttribPointer(VIEW_RAY_ATTRIB_INDEX, VIEW_RAY_COORDS_PER_VERTEX, GL_FLOAT, false, 0,
									reinterpret_cast<const void*>(VIEW_RAYS_OFFSET_IN_VBO)));
		GL(gl.glEnableVertexAttribArray(VIEW_RAY_ATTRIB_INDEX));
	}

	{
		GL(gl.glGenVertexArrays(1, &renderVAO_));
		GL(gl.glBindVertexArray(renderVAO_));
		indexBuffer.bind();
		posGridBuffer.bind();
		constexpr int POS_GRID_BUFFER_COORDS_PER_VERTEX = 2;
		GL(gl.glVertexAttribPointer(SKY_VERTEX_ATTRIB_INDEX, POS_GRID_BUFFER_COORDS_PER_VERTEX, GL_FLOAT, false, 0, 0));
		GL(gl.glEnableVertexAttribArray(SKY_VERTEX_ATTRIB_INDEX));
		viewRayGridBuffer.bind();
		GL(gl.glVertexAttribPointer(VIEW_RAY_ATTRIB_INDEX, VIEW_RAY_COORDS_PER_VERTEX, GL_FLOAT, false, 0, 0));
		GL(gl.glEnableVertexAttribArray(VIEW_RAY_ATTRIB_INDEX));
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

AtmosphereShowMySky::AtmosphereShowMySky()
	: showMySkyLib("ShowMySky")
	, viewport(0,0,0,0)
	, gridMaxY(44)
	, gridMaxX(44)
	, posGridBuffer(QOpenGLBuffer::VertexBuffer)
	, indexBuffer(QOpenGLBuffer::IndexBuffer)
	, viewRayGridBuffer(QOpenGLBuffer::VertexBuffer)
	, luminanceToScreenProgram_(new QOpenGLShaderProgram())
	, atmoRes(1)
	, flagDynamicResolution(false)
{
	indexBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
	indexBuffer.create();
	viewRayGridBuffer.setUsagePattern(QOpenGLBuffer::DynamicDraw);
	viewRayGridBuffer.create();
	posGridBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
	posGridBuffer.create();

	setFadeDuration(1.5);

	resolveFunctions();
	try
	{
		const auto& conf=*StelApp::getInstance().getSettings();
		const auto defaultPath = QDir::homePath() + "/cms";
		const auto pathToData = conf.value("landscape/atmosphere_model_path", defaultPath).toString();
		flagDynamicResolution = conf.value("landscape/flag_dynamic_resolution", false).toBool();
		maxRes = atmoRes = conf.value("landscape/atmosphere_resolution", 1).toInt();
		auto& gl=glfuncs();
		qDebug() << "Will load CalcMySky atmosphere model from" << pathToData;
		skySettings_.reset(new SkySettings);
		renderSurfaceFunc_=[this](QOpenGLShaderProgram& prog)
			{
				const auto& settings = *static_cast<SkySettings*>(skySettings_.get());
				prog.setUniformValue("projectionMatrix", settings.projectionMatrix_);

				auto& gl=glfuncs();

				GL(gl.glBindVertexArray(renderVAO_));

				std::size_t shift=0;
				for (int y=0;y<gridMaxY;++y)
				{
					GL(gl.glDrawElements(GL_TRIANGLE_STRIP, (gridMaxX+1)*2, GL_UNSIGNED_SHORT,
										 reinterpret_cast<void*>(shift)));
					shift += (gridMaxX+1)*2*2;
				}

				GL(gl.glBindVertexArray(0));
			};
		renderer_.reset(ShowMySky_AtmosphereRenderer_create(&gl, &pathToData, skySettings_.get(), &renderSurfaceFunc_));
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
		shaderAttribLocations.doSRGB                 = prog.uniformLocation("doSRGB");

		prog.release();
	}
}

AtmosphereShowMySky::~AtmosphereShowMySky()
{
	if(auto*const ctx=QOpenGLContext::currentContext())
	{
		auto& gl=glfuncs();
		GL(gl.glDeleteBuffers(1, &vbo_));
		GL(gl.glDeleteVertexArrays(1, &luminanceToScreenVAO_));
		GL(gl.glDeleteVertexArrays(1, &zenithProbeVAO_));
	}
}

void AtmosphereShowMySky::regenerateGrid()
{
	QSettings* conf = StelApp::getInstance().getSettings();
//	flagDynamicResolution = conf->value("landscape/flag_dynamic_resolution", false).toBool();
//	maxRes = atmoRes = conf->value("landscape/atmosphere_resolution", 1).toInt();
	const float width=viewport[2]/atmoRes, height=viewport[3]/atmoRes;
	gridMaxY = conf->value("landscape/atmosphereybin", 44).toInt();
	gridMaxX = std::floor(0.5+gridMaxY*(0.5*std::sqrt(3.0))*width/height);
	const auto gridSize=(1+gridMaxX)*(1+gridMaxY);
	posGrid.resize(gridSize);
	viewRayGrid.resize(gridSize);
	const auto stepX = width / (gridMaxX-0.5);
	const auto stepY = height / gridMaxY;
	const float viewportLeft = viewport[0];
	const float viewportBottom = viewport[1];
	for(int y=0; y<=gridMaxY; ++y)
	{
		for(int x=0; x<=gridMaxX; ++x)
		{
			Vec2f& v=posGrid[y*(1+gridMaxX)+x];
			v[0] = viewportLeft + (x == 0 ? 0 : x == gridMaxX ? width : (x-0.5*(y&1))*stepX);
			v[1] = viewportBottom+y*stepY;
		}
	}
	posGridBuffer.bind();
	posGridBuffer.allocate(&posGrid[0], posGrid.size()*sizeof posGrid[0]);
	posGridBuffer.release();

	// Generate the indices used to draw the quads
	QVector<GLushort> indices((gridMaxX+1)*gridMaxY*2);
	for(int y=0, i=0; y<gridMaxY; ++y)
	{
		auto g0 = y*(1+gridMaxX);
		auto g1 = (y+1)*(1+gridMaxX);
		for(int x=0; x<=gridMaxX; ++x)
		{
			indices[i++]=g0++;
			indices[i++]=g1++;
		}
	}
	indexBuffer.bind();
	indexBuffer.allocate(&indices[0], (gridMaxX+1)*gridMaxY*2*2);
	indexBuffer.release();

	viewRayGridBuffer.bind();
	viewRayGridBuffer.allocate(&viewRayGrid[0], (1+gridMaxX)*(1+gridMaxY)*4*4);
	viewRayGridBuffer.release();
}

void AtmosphereShowMySky::probeZenithLuminances(const float altitude)
{
	// Here we'll draw zenith part of the sky into a 1×1 texture in several
	// modes and get the resulting colors to determine the coefficients to
	// render light pollution and airglow correctly

	auto& gl=glfuncs();

	GLint origScissor[4];
	GL(gl.glGetIntegerv(GL_SCISSOR_BOX, origScissor));

	renderer_->setDrawSurfaceCallback([this](QOpenGLShaderProgram& prog)
	                                  {
	                                      const auto& settings = *static_cast<SkySettings*>(skySettings_.get());
	                                      prog.setUniformValue("projectionMatrix", settings.projectionMatrix_);

	                                      auto& gl=glfuncs();
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
	Q_UNUSED(airglowRelativeBrightness)
	const auto& m = projectionMatrix;
	auto& settings = *static_cast<SkySettings*>(skySettings_.get());
	settings.projectionMatrix_ = QMatrix4x4(m[0], m[4], m[8] , m[12],
						m[1], m[5], m[9] , m[13],
						m[2], m[6], m[10], m[14],
						m[3], m[7], m[11], m[15]);
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
	auto& gl=glfuncs();

	GL(gl.glActiveTexture(GL_TEXTURE0));
	GL(gl.glBindTexture(GL_TEXTURE_2D, renderer_->getLuminanceTexture()));
	GL(gl.glGenerateMipmap(GL_TEXTURE_2D));

	int texW=-1, texH=-1;
	GL(gl.glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &texW));
	GL(gl.glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &texH));

	using namespace std;
	// Formula from the glspec, "Mipmapping" subsection in section 3.8.11 Texture Minification
	const auto totalMipmapLevels = 1+floor(log2(max(texW,texH)));
	const auto deepestLevel=totalMipmapLevels-1;

#ifndef NDEBUG
	// Sanity check
	int deepestMipmapLevelWidth=-1, deepestMipmapLevelHeight=-1;
	GL(gl.glGetTexLevelParameteriv(GL_TEXTURE_2D, deepestLevel, GL_TEXTURE_WIDTH, &deepestMipmapLevelWidth));
	GL(gl.glGetTexLevelParameteriv(GL_TEXTURE_2D, deepestLevel, GL_TEXTURE_HEIGHT, &deepestMipmapLevelHeight));
	assert(deepestMipmapLevelWidth==1);
	assert(deepestMipmapLevelHeight==1);
#endif

	Vec4f pixel;
	GL(gl.glGetTexImage(GL_TEXTURE_2D, deepestLevel, GL_RGBA, GL_FLOAT, &pixel[0]));
	return pixel;
}

bool AtmosphereShowMySky::dynamicResolution(StelProjectorP prj, Vec3d &currPos, int width, int height)
{
	if (!flagDynamicResolution)
		return false;

	const auto currFov=prj->getFov(), currFad=fader.getInterstate();
	Vec3d currSun;
	prj->project(currPos,currSun);
	const auto dFov=2e3*qAbs(currFov-prevFov)/(currFov+prevFov);	// per thousand of the field of view
	const auto dFad=1e3*qAbs(currFad-prevFad);			// per thousand of the fader
	const auto dPos=1e3*(currPos-prevPos).length();			// milli-AU :)
	const auto dSun=(currSun-prevSun).length();			// pixel
	const auto changeOfView=dFov+dFad+dPos+dSun;
	// hysteresis avoids frequent changing of the resolution
	const float allowedChangeOfView=atmoRes==1?1:200e-3;
	dynResTimer--;							// count down to redraw
	// if we have neither a timeout nor a change that is too large, we do nothing...
	if (changeOfView<allowedChangeOfView && dynResTimer>0)
		return true;

	// if there is a timeout, we draw with full resolution
	// if the change is too large, we draw with reduced resolution
	atmoRes=dynResTimer>0?maxRes:1;
	if (prevRes!=atmoRes)
	{
		regenerateGrid();
		resizeRenderTarget(width, height);
	}

	bool verbose=qApp->property("verbose").toBool();
	if (verbose)
//		qDebug() << "dynResTimer" << dynResTimer << "atmoRes" << atmoRes << "dFov" << dFov << "dFad" << dFad << "dPos" << dPos << "dSun" << dSun;
		qDebug() << "dynResTimer =" << dynResTimer << "\tatmoRes =" << atmoRes << "\tchangeOfView =" << changeOfView;

	// At reduced resolution, we hurry to redraw - at full resolution, we have time.
	dynResTimer=dynResTimer>0?4:18;
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
	Q_UNUSED(JD)
	Q_UNUSED(temperature)
	Q_UNUSED(relativeHumidity)
	Q_UNUSED(extinctionCoefficient)
	initProperties();

	// The majority of calculations is done in fragment shader, but we still need a nontrivial
	// grid to pass view rays, corresponding to the chosen projection, to the shader. Of course,
	// best quality results would be if we did projection inside the shader, but that'd require
	// having two implementations for each projection type: one for CPU and one for GPU.
	const auto prj = core->getProjection(StelCore::FrameAltAz, StelCore::RefractionOff);
	if (viewport != prj->getViewport())
	{
		viewport = prj->getViewport();
		regenerateGrid();
	}
	const auto width=viewport[2], height=viewport[3];
	if(width!=prevWidth_ || height!=prevHeight_)
		resizeRenderTarget(width, height);

	if(location.altitude != lastUsedAltitude_)
	{
		lastUsedAltitude_ = location.altitude;
		probeZenithLuminances(location.altitude);
		dynResTimer=0;
	}

	auto sunPos  =  sun.getAltAzPosAuto(core);
	if (std::isnan(sunPos.length()))
		sunPos.set(0.,0.,-1.*AU);

	// if we have neither a timeout nor a change that is too large, we do nothing...
	if (dynamicResolution(prj, sunPos, width, height))
		return;

	const auto sunDir = sunPos / sunPos.length();
	const double sunAngularRadius = atan(sun.getEquatorialRadius()/sunPos.length());

	// If we have no moon, just put it into nadir at 1 AU distance
	Vec3d moonPos{0.,0.,-1.*AU};
	Vec3d moonDir{0.,0.,-1.};

	double earthMoonDistance = 0;

	if (moon)
	{
		moonPos = moon->getAltAzPosAuto(core);
		if (std::isnan(moonPos.length()))
			moonPos.set(0.,0.,-1.*AU);
		moonDir = moonPos / moonPos.length();

		const double moonAngularRadius = atan(moon->getEquatorialRadius()/moonPos.length());
		const double separationAngle = std::acos(sunDir.dot(moonDir));  // angle between them

		sunVisibility_ = sunVisibilityDueToMoon(sunAngularRadius, moonAngularRadius, separationAngle);
		const double min = 0.0025; // the sky still glows during the totality
		eclipseFactor = static_cast<float>(min + (1-min)*sunVisibility_);

		const Vec3d earthGeomPos = currentPlanet.getAltAzPosGeometric(core);
		const Vec3d moonGeomPos = moon->getAltAzPosGeometric(core);
		earthMoonDistance = (earthGeomPos - moonGeomPos).length() * (AU*1000);
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

	// FIXME: ignoring the "additional luminance" like star background etc.; see AtmospherePreetham for all potentially needed terms
	const auto numViewRayGridPoints=(1+gridMaxX)*(1+gridMaxY);
	const auto ppxw=(double)width/(width/atmoRes);
	const auto ppxh=(double)height/(height/atmoRes);
//	qDebug() << "ppxw =" << ppxw << "ppxh =" << ppxh;
	for (int i=0; i<numViewRayGridPoints; ++i)
	{
		Vec3d point(1, 0, 0);
		prj->unProject(posGrid[i][0]*ppxw,posGrid[i][1]*ppxh,point);

		viewRayGrid[i].set(point[0], point[1], point[2], 0);
	}

	viewRayGridBuffer.bind();
	viewRayGridBuffer.write(0, &viewRayGrid[0], viewRayGrid.size()*sizeof viewRayGrid[0]);
	viewRayGridBuffer.release();

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

void AtmosphereShowMySky::draw(StelCore* core)
{
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
	GL(luminanceToScreenProgram_->setUniformValue(shaderAttribLocations.flagUseTmGamma, useTmGamma));
	GL(luminanceToScreenProgram_->setUniformValue(shaderAttribLocations.brightnessScale, atm_intensity));
	GL(luminanceToScreenProgram_->setUniformValue(shaderAttribLocations.doSRGB, sRGB));

	StelPainter sPainter(core->getProjection2d());
	sPainter.setBlending(true, GL_ONE, GL_ONE);

	const auto rgbMaxValue=calcRGBMaxValue(sPainter.getDitheringMode());
	GL(luminanceToScreenProgram_->setUniformValue(shaderAttribLocations.rgbMaxValue, rgbMaxValue[0], rgbMaxValue[1], rgbMaxValue[2]));

	auto& gl=glfuncs();
	GL(gl.glActiveTexture(GL_TEXTURE0));
	GL(gl.glBindTexture(GL_TEXTURE_2D, renderer_->getLuminanceTexture()));
	GL(gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
	GL(luminanceToScreenProgram_->setUniformValue(shaderAttribLocations.luminanceTexture, 0));

	GL(gl.glActiveTexture(GL_TEXTURE1));
	if(!ditherPatternTex_)
		ditherPatternTex_=makeDitherPatternTexture(*sPainter.glFuncs());
	GL(gl.glBindTexture(GL_TEXTURE_2D, ditherPatternTex_));
	GL(luminanceToScreenProgram_->setUniformValue(shaderAttribLocations.ditherPattern, 1));

	GL(gl.glBindVertexArray(luminanceToScreenVAO_));
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

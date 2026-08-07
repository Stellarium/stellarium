/*
 * Stellarium
 * Copyright (C) 2026 Ruslan Kabatsayev
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

#include "AtmosphereLightweight.hpp"
#include "Planet.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelPainter.hpp"
#include "StelFileMgr.hpp"
#include "StelMainView.hpp"
#include "StelProjector.hpp"
#include "StelToneReproducer.hpp"
#include "TextureAverageComputer.hpp"

#include <limits>
#include <cstring>
#include <stdexcept>
#include <QDir>
#include <QFile>
#include <QDebug>
#include <QOpenGLShaderProgram>
#include <QOpenGLFramebufferObject>

namespace
{
struct AtmosphereVertex
{
	uint16_t azimuth;
	uint16_t elevation;
	uint8_t  color_x;
	uint8_t  color_y;
	uint16_t color_Y;
};
using IndexType = uint16_t;
constexpr auto INDEX_GL_TYPE = GL_UNSIGNED_SHORT;

constexpr int PREP_FBO_WIDTH = 1024;
constexpr int PREP_FBO_HEIGHT = 512;
constexpr int LUM_PROBE_FBO_WIDTH = 128;
constexpr int LUM_PROBE_FBO_HEIGHT = 128;
constexpr int AZIMUTH_AND_ELEV_ATTRIB_LOC = 0;
constexpr int COLOR_XY_ATTRIB_LOC = 1;
constexpr int COLOR_Y_ATTRIB_LOC = 2;
constexpr int SCREEN_VERTEX_ATTRIB_LOC = 3;

constexpr float nonExtinctedSolarMagnitude=-26.74;

struct FileReadError : std::runtime_error
{
	FileReadError(const QString& message, const QString& path)
		: std::runtime_error(message.arg(path).toStdString())
	{
	}
};

void read(QFile& file, const QString& path, void* buf, qsizetype size)
{
	const auto pos = file.pos();
	if (file.read(static_cast<char*>(buf), size) != size)
		throw FileReadError(QString("Failed to read %1 bytes at position %2 in file %3")
		                    .arg(pos).arg(size), path);
}


inline GLenum getInternalFormatForFBO()
{
	const auto& glInfo = StelMainView::getInstance().getGLInformation();
	if (glInfo.isGLES && glInfo.majorVersion == 2)
		return GL_RGBA;
	return GL_RGBA16F;
}

}

AtmosphereLightweight::AtmosphereLightweight()
	: preparationVBO_(QOpenGLBuffer::VertexBuffer)
	, indexBuffer_(QOpenGLBuffer::IndexBuffer)
	, sunPrepFBO_(new QOpenGLFramebufferObject(PREP_FBO_WIDTH, PREP_FBO_HEIGHT,
	              QOpenGLFramebufferObject::NoAttachment, GL_TEXTURE_2D, getInternalFormatForFBO()))
	, moonPrepFBO_(new QOpenGLFramebufferObject(PREP_FBO_WIDTH, PREP_FBO_HEIGHT,
	               QOpenGLFramebufferObject::NoAttachment, GL_TEXTURE_2D, getInternalFormatForFBO()))
	, luminanceProbeFBO_(new QOpenGLFramebufferObject(LUM_PROBE_FBO_WIDTH, LUM_PROBE_FBO_HEIGHT,
	                                                  QOpenGLFramebufferObject::NoAttachment,
	                                                  GL_TEXTURE_2D, GL_RGBA))
	, textureAverager_(new TextureAverageComputer(StelOpenGL::highGraphicsFunctions(),
	                                              LUM_PROBE_FBO_WIDTH, LUM_PROBE_FBO_HEIGHT,
	                                              GL_RGBA, false))
{
	setFadeDuration(1.5f);

	auto& gl = *StelOpenGL::mainContext->functions();
	GL(gl.glBindTexture(GL_TEXTURE_2D, sunPrepFBO_->texture()));
	GL(gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
	GL(gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
	GL(gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
	GL(gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
	GL(gl.glBindTexture(GL_TEXTURE_2D, moonPrepFBO_->texture()));
	GL(gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
	GL(gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
	GL(gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
	GL(gl.glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));

	QOpenGLShader vShader(QOpenGLShader::Vertex);
	const auto vSrc = StelOpenGL::globalShaderPrefix(StelOpenGL::VERTEX_SHADER) +
		"ATTRIBUTE lowp vec2 color_xy;\n"
		"ATTRIBUTE mediump float color_Y;\n"
		"ATTRIBUTE mediump vec2 azimuthAndElevation;\n"
		"VARYING mediump vec3 skyColor;\n"
		"uniform mediump float colorScale;\n"
		"void main()\n"
		"{\n"
		"	gl_Position = vec4(2. * azimuthAndElevation - 1., 0., 1.);\n"
		"	skyColor=vec3(color_xy.x, color_xy.y, color_Y * colorScale);\n"
		"}\n";
	if (!vShader.compileSourceCode(vSrc))
		qFatal("Error while compiling atmosphere vertex shader: %s", vShader.log().toUtf8().constData());
	if (!vShader.log().isEmpty())
		qCWarning(Atmo) << "Warnings while compiling Lightweight atmosphere vertex shader: " << vShader.log();
	QOpenGLShader fShader(QOpenGLShader::Fragment);
	if (!fShader.compileSourceCode(StelOpenGL::globalShaderPrefix(StelOpenGL::FRAGMENT_SHADER) +
	                               "VARYING mediump vec3 skyColor;\n"
	                               "void main()\n"
	                               "{\n"
	                               "   // sqrt is a gamma-like mapping to reduce banding\n"
	                               "   // on GLESv2 where we use RGBA8 textures\n"
	                               "   FRAG_COLOR = vec4(skyColor.xy, sqrt(skyColor.z), 1.);\n"
	                               "}"))
	{
		qFatal("Error while compiling Lightweight atmosphere fragment shader: %s", fShader.log().toLatin1().constData());
	}
	if (!fShader.log().isEmpty())
	{
		qCWarning(Atmo) << "Warnings while compiling Lightweight atmosphere fragment shader: " << vShader.log();
	}
	preparationShaderProgram_.reset(new QOpenGLShaderProgram());
	preparationShaderProgram_->addShader(&vShader);
	preparationShaderProgram_->addShader(&fShader);
	preparationShaderProgram_->bindAttributeLocation("azimuthAndElevation", AZIMUTH_AND_ELEV_ATTRIB_LOC);
	preparationShaderProgram_->bindAttributeLocation("color_xy", COLOR_XY_ATTRIB_LOC);
	preparationShaderProgram_->bindAttributeLocation("color_Y", COLOR_Y_ATTRIB_LOC);
	StelPainter::linkProg(preparationShaderProgram_.get(), "Lightweight atmosphere");

	GL(prepProgramUniformLocations_.colorScale = preparationShaderProgram_->uniformLocation("colorScale"));

	preparationVBO_.setUsagePattern(QOpenGLBuffer::StaticDraw);
	preparationVBO_.create();
	indexBuffer_.setUsagePattern(QOpenGLBuffer::StaticDraw);
	indexBuffer_.create();
	loadMesh();

	renderVBO_.setUsagePattern(QOpenGLBuffer::StaticDraw);
	renderVBO_.create();
	const GLfloat vertices[]=
	{
		// full screen quad
		-1, -1,
		 1, -1,
		-1,  1,
		 1,  1,
	};
	renderVBO_.bind();
	renderVBO_.allocate(vertices, sizeof vertices);
	renderVAO_.create();
}

void AtmosphereLightweight::loadMesh()
{
	const auto path = QDir::toNativeSeparators(QString("%1/atmosphere/lightweight.amsh").arg(StelFileMgr::getInstallationDir()));
	QFile file(path);
	try
	{
		if (!file.open(QFile::ReadOnly))
			throw FileReadError("Failed to load atmosphere mesh %1", path);
		char header[12];
		read(file, path, header, sizeof header);
		if (std::memcmp(header, "AtmoMesh", 8) != 0)
			throw FileReadError("Bad header magic in atmosphere mesh file %1", path);
		const int version = header[8];
		if (version != 1)
			throw FileReadError(QString("Unsupported format version %1 of the atmosphere mesh file %2").arg(version), path);
		uint16_t numLayers;
		std::memcpy(&numLayers, header + 10, sizeof numLayers);
		layerSolarElevations_.resize(numLayers);
		layerNorms_.resize(numLayers);
		read(file, path, layerSolarElevations_.data(), numLayers * sizeof layerSolarElevations_[0]);
		read(file, path, layerNorms_.data(), numLayers * sizeof layerNorms_[0]);

		vaos_.resize(numLayers);
		std::vector<AtmosphereVertex> allVertices;
		std::vector<IndexType> allIndices;
		for (unsigned layer = 0; layer < numLayers; ++layer)
		{
			uint16_t numVertices;
			read(file, path, &numVertices, sizeof numVertices);
			uint16_t numTriangles;
			read(file, path, &numTriangles, sizeof numTriangles);
			const auto numIndices = numTriangles * 3;

			const auto oldVertSize = allVertices.size();
			allVertices.resize(allVertices.size() + numVertices);
			read(file, path, allVertices.data() + oldVertSize, numVertices * sizeof allVertices[0]);
			layerVertexOffsetsInVBO_.push_back(oldVertSize * sizeof allVertices[0]);
			const auto max = std::max_element(allVertices.data() + oldVertSize,
			                                  allVertices.data() + oldVertSize + numVertices,
			                                  [](const auto& a, const auto& b)
			                                  { return a.color_Y < b.color_Y; });
			constexpr float maxPossibleColorY = std::numeric_limits<decltype(max->color_Y)>::max();
			layerValueMaxima_.push_back(max->color_Y / maxPossibleColorY);

			const auto oldIndSize = allIndices.size();
			allIndices.resize(allIndices.size() + numIndices);
			read(file, path, allIndices.data() + oldIndSize, numIndices * sizeof allIndices[0]);
			layerIndexOffsetsInBuffer_.push_back(oldIndSize * sizeof allIndices[0]);

			vaos_[layer].reset(new QOpenGLVertexArrayObject);
			GL(vaos_[layer]->create());
			bindVAO(layer);
			setupCurrentVAO(layer);
			releaseVAO(layer);
		}
		layerIndexOffsetsInBuffer_.push_back(allIndices.size() * sizeof allIndices[0]);

		preparationVBO_.bind();
		preparationVBO_.allocate(allVertices.data(), allVertices.size() * sizeof allVertices[0]);
		preparationVBO_.release();
		indexBuffer_.bind();
		indexBuffer_.allocate(allIndices.data(), allIndices.size() * sizeof allIndices[0]);
		indexBuffer_.release();
		qCInfo(Atmo) << "Lightweight atmosphere mesh loaded: "
		             << allVertices.size() << "vertices," << allIndices.size() << "indices";
	}
	catch(const std::exception& ex)
	{
		qCCritical(Atmo) << ex.what();

		vaos_.clear();
		layerNorms_.clear();
		layerValueMaxima_.clear();
		layerSolarElevations_.clear();
		layerVertexOffsetsInVBO_.clear();
		layerIndexOffsetsInBuffer_.clear();
	}
}

void AtmosphereLightweight::setupCurrentVAO(const int layer)
{
	if (layer >= 0 && int(layerVertexOffsetsInVBO_.size()) <= layer) return;

	if (layer == VAO_RENDER)
	{
		GL(renderVBO_.bind());
		GL(renderShaderProgram_->setAttributeBuffer(SCREEN_VERTEX_ATTRIB_LOC, GL_FLOAT, 0, 2, 0));
		GL(renderShaderProgram_->enableAttributeArray(SCREEN_VERTEX_ATTRIB_LOC));
		GL(renderVBO_.release());
	}
	else
	{
		GL(preparationVBO_.bind());
		int offset = layerVertexOffsetsInVBO_[layer];
		constexpr int stride = sizeof(AtmosphereVertex);
		GL(preparationShaderProgram_->setAttributeBuffer(AZIMUTH_AND_ELEV_ATTRIB_LOC, GL_UNSIGNED_SHORT, offset, 2, stride));
		GL(preparationShaderProgram_->enableAttributeArray(AZIMUTH_AND_ELEV_ATTRIB_LOC));
		offset += sizeof(AtmosphereVertex::azimuth) + sizeof(AtmosphereVertex::elevation);
		GL(preparationShaderProgram_->setAttributeBuffer(COLOR_XY_ATTRIB_LOC, GL_UNSIGNED_BYTE, offset, 2, stride));
		GL(preparationShaderProgram_->enableAttributeArray(COLOR_XY_ATTRIB_LOC));
		offset += sizeof(AtmosphereVertex::color_x) + sizeof(AtmosphereVertex::color_y);;
		GL(preparationShaderProgram_->setAttributeBuffer(COLOR_Y_ATTRIB_LOC, GL_UNSIGNED_SHORT, offset, 1, stride));
		GL(preparationShaderProgram_->enableAttributeArray(COLOR_Y_ATTRIB_LOC));
		offset += sizeof(AtmosphereVertex::color_Y);
		GL(preparationVBO_.release());
	}
}

void AtmosphereLightweight::bindVAO(const int layer)
{
	if (layer >= 0 && int(vaos_.size()) <= layer) return;

	auto& vao = layer == VAO_RENDER ? renderVAO_ : *vaos_[layer];
	if (vao.isCreated())
		GL(vao.bind());
	else
		setupCurrentVAO(layer);
}

void AtmosphereLightweight::releaseVAO(const int layer)
{
	if (layer >= 0 && int(vaos_.size()) <= layer) return;

	auto& vao = layer == VAO_RENDER ? renderVAO_ : *vaos_[layer];
	if (vao.isCreated())
	{
		GL(vao.release());
	}
	else
	{
		if (layer == VAO_RENDER)
		{
			GL(preparationShaderProgram_->disableAttributeArray(SCREEN_VERTEX_ATTRIB_LOC));
		}
		else
		{
			GL(preparationShaderProgram_->disableAttributeArray(AZIMUTH_AND_ELEV_ATTRIB_LOC));
			GL(preparationShaderProgram_->disableAttributeArray(COLOR_XY_ATTRIB_LOC));
			GL(preparationShaderProgram_->disableAttributeArray(COLOR_Y_ATTRIB_LOC));
		}
	}
}

void AtmosphereLightweight::recompileRenderShaders(const StelProjector& projector)
{
	const auto vSrc = R"(
ATTRIBUTE mediump vec3 screenVertex;
VARYING mediump vec3 ndcPos;

void main()
{
	gl_Position = vec4(screenVertex, 1.);
	ndcPos = screenVertex;
}
)";
	const auto renderFragSrc = R"(
VARYING mediump vec3 ndcPos;
uniform mat4 projectionMatrixInverse;
uniform vec3 sunDir;
uniform highp float colorScale;
uniform sampler2D atmoTex;
const highp float PI = 3.14159265;

highp vec3 calcViewDir()
{
	highp vec4 winPos = projectionMatrixInverse * vec4(ndcPos, 1);
	bool ok = false;
	highp vec3 modelPos = unProject(winPos.x, winPos.y, ok).xyz;

//	if(!ok) return vec3(0);

	return normalize(modelPos);
}

float sqr(vec2 v) { return dot(v, v); }

void main()
{
	highp vec3 viewDir = calcViewDir();
	float relAzimuth = acos(clamp(dot(viewDir.xy, sunDir.xy) / sqrt(sqr(viewDir.xy) * sqr(sunDir.xy)), -1., 1.));
	float elev = asin(clamp(viewDir.z, -1., 1.));
	float elevTC = abs(elev / (PI/2.));
	elevTC = sqrt(elevTC); // elevation coordinate is stretched by squaring it
	float azimuthTC = relAzimuth / PI;
	vec3 color_xyY = texture2D(atmoTex, vec2(azimuthTC, elevTC)).xyz;
	// Undo the sqrt
	color_xyY.z *= color_xyY.z;
	vec3 color_RGB = xyYToRGB(color_xyY.x, color_xyY.y, color_xyY.z * colorScale);
	FRAG_COLOR = vec4(color_RGB, 1);
}
)";

	QOpenGLShader vShader(QOpenGLShader::Vertex);
	QFile toneRepro(":/shaders/xyYToRGB.glsl");
	if (!toneRepro.open(QFile::ReadOnly))
		qFatal("Failed to open ToneReproducer shader source");
	if (!vShader.compileSourceCode(StelOpenGL::globalShaderPrefix(StelOpenGL::VERTEX_SHADER) +
	                               vSrc))
	{
		qFatal("Error while compiling atmosphere vertex shader: %s", vShader.log().toUtf8().constData());
	}
	if (!vShader.log().isEmpty())
	{
		qCWarning(Atmo) << "Warnings while compiling Lightweight atmosphere vertex shader: " << vShader.log();
	}

	QOpenGLShader fShader(QOpenGLShader::Fragment);
	if (!fShader.compileSourceCode(StelOpenGL::globalShaderPrefix(StelOpenGL::FRAGMENT_SHADER) +
	                               projector.getUnProjectShader() + toneRepro.readAll() + renderFragSrc))
	{
		qFatal("Error while compiling Lightweight atmosphere fragment shader: %s", fShader.log().toUtf8().constData());
	}
	if (!fShader.log().isEmpty())
	{
		qCWarning(Atmo) << "Warnings while compiling Lightweight atmosphere fragment shader: " << vShader.log();
	}

	renderShaderProgram_.reset(new QOpenGLShaderProgram());
	renderShaderProgram_->addShader(&vShader);
	renderShaderProgram_->addShader(&fShader);
	renderShaderProgram_->bindAttributeLocation("screenVertex", SCREEN_VERTEX_ATTRIB_LOC);
	StelPainter::linkProg(renderShaderProgram_.get(), "Lightweight atmosphere");

	GL(renderShaderProgram_->bind());
	GL(renderUniformLocations_.doSRGB                  = renderShaderProgram_->uniformLocation("doSRGB"));
	GL(renderUniformLocations_.oneOverGamma            = renderShaderProgram_->uniformLocation("oneOverGamma"));
	GL(renderUniformLocations_.brightnessScale         = renderShaderProgram_->uniformLocation("brightnessScale"));
	GL(renderUniformLocations_.alphaWaOverAlphaDa      = renderShaderProgram_->uniformLocation("alphaWaOverAlphaDa"));
	GL(renderUniformLocations_.term2TimesOneOverMaxdL  = renderShaderProgram_->uniformLocation("term2TimesOneOverMaxdL"));
	GL(renderUniformLocations_.term2TimesOneOverMaxdLpOneOverGamma
	                                                   = renderShaderProgram_->uniformLocation("term2TimesOneOverMaxdLpOneOverGamma"));
	GL(renderUniformLocations_.flagUseTmGamma          = renderShaderProgram_->uniformLocation("flagUseTmGamma"));

	GL(renderUniformLocations_.sunDir                  = renderShaderProgram_->uniformLocation("sunDir"));
	GL(renderUniformLocations_.atmoTex                 = renderShaderProgram_->uniformLocation("atmoTex"));
	GL(renderUniformLocations_.colorScale              = renderShaderProgram_->uniformLocation("colorScale"));
	GL(renderUniformLocations_.projectionMatrixInverse = renderShaderProgram_->uniformLocation("projectionMatrixInverse"));

	GL(renderShaderProgram_->release());

	const auto luminanceProbeFragSrc = R"(
VARYING mediump vec3 ndcPos;
uniform mat4 projectionMatrixInverse;
uniform vec3 sunDir;
uniform bool isMoon;
uniform sampler2D atmoTex;
const highp float PI = 3.14159265;

vec3 calcViewDir()
{
	vec4 winPos = projectionMatrixInverse * vec4(ndcPos, 1);
	bool ok = false;
	vec3 modelPos = unProject(winPos.x, winPos.y, ok).xyz;

//	if(!ok) return vec3(0);

	return normalize(modelPos);
}

float sqr(vec2 v) { return dot(v, v); }

void main()
{
	vec3 viewDir = calcViewDir();
	float relAzimuth = acos(clamp(dot(viewDir.xy, sunDir.xy) / sqrt(sqr(viewDir.xy) * sqr(sunDir.xy)), -1., 1.));
	float elev = asin(clamp(viewDir.z, -1., 1.));
	float elevTC = abs(elev / (PI/2.));
	elevTC = sqrt(elevTC); // elevation coordinate is stretched by squaring it
	float azimuthTC = relAzimuth / PI;
	vec3 color_xyY = texture2D(atmoTex, vec2(azimuthTC, elevTC)).xyz;
	// Undo the sqrt
	color_xyY.z *= color_xyY.z;
	FRAG_COLOR = vec4(isMoon ? 0. : color_xyY.z, isMoon ? color_xyY.z : 0., 0, 1);
}
)";

	QOpenGLShader lumProbeFShader(QOpenGLShader::Fragment);
	if (!lumProbeFShader.compileSourceCode(StelOpenGL::globalShaderPrefix(StelOpenGL::FRAGMENT_SHADER) +
	                               projector.getUnProjectShader() + luminanceProbeFragSrc))
	{
		qFatal("Error while compiling Lightweight atmosphere luminance probe fragment shader: %s",
		       lumProbeFShader.log().toUtf8().constData());
	}
	if (!lumProbeFShader.log().isEmpty())
	{
		qCWarning(Atmo) << "Warnings while compiling Lightweight atmosphere luminance probe fragment shader: "
		                << vShader.log();
	}

	luminanceProbeProgram_.reset(new QOpenGLShaderProgram());
	luminanceProbeProgram_->addShader(&vShader);
	luminanceProbeProgram_->addShader(&lumProbeFShader);
	luminanceProbeProgram_->bindAttributeLocation("screenVertex", SCREEN_VERTEX_ATTRIB_LOC);
	StelPainter::linkProg(luminanceProbeProgram_.get(), "Lightweight atmosphere luminance probe");

	GL(luminanceProbeUniformLocations_.isMoon                  = luminanceProbeProgram_->uniformLocation("isMoon"));
	GL(luminanceProbeUniformLocations_.sunDir                  = luminanceProbeProgram_->uniformLocation("sunDir"));
	GL(luminanceProbeUniformLocations_.atmoTex                 = luminanceProbeProgram_->uniformLocation("atmoTex"));
	GL(luminanceProbeUniformLocations_.projectionMatrixInverse = luminanceProbeProgram_->uniformLocation("projectionMatrixInverse"));

	bindVAO(VAO_RENDER);
	setupCurrentVAO(VAO_RENDER);
	releaseVAO(VAO_RENDER);
}

float AtmosphereLightweight::computeAverageLuminance(const StelCore* core)
{
	StelOpenGL::checkGLErrors(__FILE__,__LINE__);
	auto& gl = *StelOpenGL::mainContext->functions();

	const auto prj = core->getProjection(StelCore::FrameAltAz, StelCore::RefractionOff);
	if(!prevProjector_ || !prj->isSameProjection(*prevProjector_))
	{
		prevProjector_ = prj;
		recompileRenderShaders(*prj);
	}
	if (!luminanceProbeProgram_) return 0;

	luminanceProbeFBO_->bind();
	luminanceProbeProgram_->bind();
	prj->setUnProjectUniforms(*luminanceProbeProgram_);

	// NOTE: we use a viewport that differs from that in StelProjector, which works because the
	// shader is based on NDC rather than window coordinates. But if you need to employ StelPainter,
	// its constructor will reset the viewport, which will result in failure to render the whole
	// scene to the FBO, so StelPainter must be constructed before the call below.
	GL(gl.glViewport(0, 0, LUM_PROBE_FBO_WIDTH, LUM_PROBE_FBO_HEIGHT));

	GL(gl.glActiveTexture(GL_TEXTURE0));
	luminanceProbeProgram_->setUniformValue(luminanceProbeUniformLocations_.atmoTex, 0);

	luminanceProbeProgram_->setUniformValue(luminanceProbeUniformLocations_.projectionMatrixInverse,
	                                        prj->getProjectionMatrix().toQMatrix().inverted());

	bindVAO(VAO_RENDER);

	for (int i = 0; i < DRAW_PARAM_COUNT; ++i)
	{
		const auto& p = drawParams[i];
		luminanceProbeProgram_->setUniformValue(luminanceProbeUniformLocations_.isMoon,
		                                        i == DRAW_PARAM_MOON);
		luminanceProbeProgram_->setUniformValue(luminanceProbeUniformLocations_.sunDir, p.dir);
		GL(gl.glBindTexture(GL_TEXTURE_2D, p.fbo->texture()));
		GL(gl.glDrawArrays(GL_TRIANGLE_STRIP, 0, 4));

		GL(gl.glEnable(GL_BLEND));
		GL(gl.glBlendFunc(GL_ONE, GL_ONE));
	}

	releaseVAO(VAO_RENDER);

	GL(gl.glDisable(GL_BLEND));
	GL(gl.glBlendFunc(GL_ONE, GL_ZERO));
	luminanceProbeProgram_->release();

	const auto texAvg = textureAverager_->getTextureAverageSimple(luminanceProbeFBO_->texture(),
	                                                              LUM_PROBE_FBO_WIDTH, LUM_PROBE_FBO_HEIGHT);
	float lum = 0;
	for (int i = 0; i < DRAW_PARAM_COUNT; ++i)
	{
		const auto& p = drawParams[i];
		auto currLum = texAvg[i] * p.relativeBrightness * p.eclipseFactor * p.fboColorScale;
		const float maxValue = std::max(layerValueMaxima_[p.layerToDrawA],
		                                layerValueMaxima_[p.layerToDrawB]);
		currLum *= maxValue; // restore the scale we used when creating the preparation FBO
		lum += currLum;
	}
	return lum;
}

void AtmosphereLightweight::computeDrawParams(const StelCore* core, const Planet* lightSource, const Planet* moon,
                                              const int paramIdx, const bool noScatter)
{
	if (layerSolarElevations_.empty()) return;

	auto& p = drawParams[paramIdx];
	if (lightSource)
	{
		auto pos = lightSource->getAltAzPosAuto(core);
		if (std::isnan(pos.norm()))
			pos.set(0, 0, -1);
		const auto dir = pos / pos.norm();
		p.dir = QVector3D(dir[0], dir[1], dir[2]);
	}
	else
	{
		// If we have no moon, just put it into nadir
		p.dir = {0, 0, -1};
	}

	float elevation = std::asin(std::clamp(p.dir[2], -1.f, 1.f));
	// The elevations are expected to be decreasing in the array
	if (elevation < layerSolarElevations_.back())
		elevation = layerSolarElevations_.back();

	const auto lowIt = std::lower_bound(layerSolarElevations_.begin(), layerSolarElevations_.end(),
	                                    elevation, std::greater<float>{});
	const auto highIt = lowIt == layerSolarElevations_.begin() ? lowIt : lowIt - 1;
	const float highElev = *highIt;
	const float lowElev = *lowIt;
	p.layerAlpha = (elevation - lowElev) / (highElev - lowElev);
	p.layerToDrawA = lowIt - layerSolarElevations_.begin();
	p.layerToDrawB = highIt - layerSolarElevations_.begin();
	if (p.layerToDrawB >= layerSolarElevations_.size())
		p.layerToDrawB = layerSolarElevations_.size() - 1;
	if (p.layerToDrawA >= layerSolarElevations_.size())
		p.layerToDrawA = layerSolarElevations_.size() - 1;
	p.fboColorScale = layerNorms_[p.layerToDrawA] + p.layerAlpha * (layerNorms_[p.layerToDrawB] - layerNorms_[p.layerToDrawA]);

	switch(paramIdx)
	{
	case DRAW_PARAM_SUN:
	{
		p.relativeBrightness = noScatter ? 0 : 1;
		p.fbo = sunPrepFBO_.get();

		p.eclipseFactor = 1; // default value for the case when there's no moon or when we get a NaN
		if (!moon) break;

		const auto*const sun = lightSource;
		const auto sunPos  = sun->getAltAzPosAuto(core);
		const auto moonPos = moon->getAltAzPosAuto(core);
		const auto sunDir = sunPos / sunPos.norm();
		const auto moonDir = moonPos / moonPos.norm();
		if (std::isnan(sunDir.norm()) || std::isnan(moonDir.norm()))
			break;

		const double sunAngularRadius = atan(sun->getEquatorialRadius()/sunPos.norm());
		const double moonAngularRadius = atan(moon->getEquatorialRadius()/moonPos.norm());
		const double separationAngle = std::acos(sunDir.dot(moonDir));  // angle between them

		const float sunVisibility = StelUtils::sunVisibilityDueToMoon(sunAngularRadius, moonAngularRadius, separationAngle);
		const float min = 0.0025; // the sky still glows during the totality
		p.eclipseFactor = min + (1-min)*sunVisibility;
		// TODO: compute eclipse factor also for Lunar eclipses! (lp:#1471546)
		break;
	}
	case DRAW_PARAM_MOON:
	{
		const auto nonExtLunarMagnitude = lightSource ? lightSource->getVMagnitude(core) : 100.f;
		p.relativeBrightness = noScatter ? 0.f : std::pow(10.f, 0.4f*(nonExtinctedSolarMagnitude-nonExtLunarMagnitude));
		p.fbo = moonPrepFBO_.get();
		p.eclipseFactor = 1;
		break;
	}
	default:
		assert(!"Unexpected draw parameter index!");
		return;
	}
}

void AtmosphereLightweight::computeColor(StelCore* core, const double JD, const Planet& currentPlanet, const Planet& sun,
                                         const Planet*const moon, const StelLocation& location, const float temperature,
                                         const float relativeHumidity, const float extinctionCoefficient, const bool noScatter)
{
	// No need to calculate if not visible
	if (!fader.getInterstate())
	{
		// GZ 20180114: Why did we add light pollution if atmosphere was not visible?????
		// And what is the meaning of 0.001? Approximate contribution of stellar background? Then why is it 0.0001 below???
		averageLuminance = 0.001f;
		return;
	}

	if (layerSolarElevations_.empty())
	{
		// Mesh isn't loaded, which is an error. Set some strangish value to pay user's attention.
		averageLuminance = 1000;
		return;
	}

	auto& gl = *StelOpenGL::mainContext->functions();

	computeDrawParams(core, &sun, moon, DRAW_PARAM_SUN, noScatter);
	computeDrawParams(core, moon, moon, DRAW_PARAM_MOON, noScatter);

	GLint origFBO=-1;
	GL(gl.glGetIntegerv(GL_FRAMEBUFFER_BINDING, &origFBO));
	preparationShaderProgram_->bind();

	GLint origViewport[4];
	GL(gl.glGetIntegerv(GL_VIEWPORT, origViewport));
	GL(gl.glViewport(0, 0, PREP_FBO_WIDTH, PREP_FBO_HEIGHT));

	for (int i = 0; i < DRAW_PARAM_COUNT; ++i)
	{
		const auto& p = drawParams[i];
		p.fbo->bind();
		const float maxValue = std::max(layerValueMaxima_[p.layerToDrawA],
		                                layerValueMaxima_[p.layerToDrawB]);
		for (const auto layer : {p.layerToDrawA, p.layerToDrawB})
		{
			if (layer == p.layerToDrawB)
			{
				GL(gl.glEnable(GL_BLEND));
				GL(gl.glBlendFunc(GL_CONSTANT_ALPHA, GL_ONE_MINUS_CONSTANT_ALPHA));
				GL(gl.glBlendColor(1,1,1, p.layerAlpha));
			}
			else
			{
				GL(gl.glDisable(GL_BLEND));
			}
			preparationShaderProgram_->setUniformValue(prepProgramUniformLocations_.colorScale,
			                                           1.f / maxValue);
			bindVAO(layer);
			GL(indexBuffer_.bind());
			const uintptr_t indicesOffset = layerIndexOffsetsInBuffer_[layer];
			assert(layer+1 < layerIndexOffsetsInBuffer_.size());
			const auto indicesEnd = layerIndexOffsetsInBuffer_[layer + 1];
			const auto numIndices = (indicesEnd - indicesOffset) / sizeof(IndexType);
			GL(gl.glDrawElements(GL_TRIANGLES, numIndices, INDEX_GL_TYPE, reinterpret_cast<void*>(indicesOffset)));
		}
	}
	GL(releaseVAO(0));
	GL(indexBuffer_.release());
	GL(gl.glDisable(GL_BLEND));
	GL(gl.glBlendFunc(GL_ONE, GL_ZERO));
	GL(gl.glBlendColor(0,0,0,0));

	if (!overrideAverageLuminance)
	{
		const auto meanY = computeAverageLuminance(core);
		Q_ASSERT(std::isfinite(meanY));

		const float bgLum = 0.0001f; // Add (assumed) star background luminance
		averageLuminance = bgLum + (meanY + lightPollutionLuminance) * fader.getInterstate();
	}

	GL(gl.glBindFramebuffer(GL_FRAMEBUFFER, origFBO));
	GL(gl.glViewport(origViewport[0], origViewport[1], origViewport[2], origViewport[3]));
}

void AtmosphereLightweight::applyToneReproducerParams(StelCore* core, const float atmosphereIntensity)
{
	StelToneReproducer* eye = core->getToneReproducer();
	float a, b, c, d;
	bool useTmGamma, sRGB;
	eye->getShadersParams(a, b, c, d, useTmGamma, sRGB);

	GL(renderShaderProgram_->setUniformValue(renderUniformLocations_.alphaWaOverAlphaDa, a));
	GL(renderShaderProgram_->setUniformValue(renderUniformLocations_.oneOverGamma, b));
	GL(renderShaderProgram_->setUniformValue(renderUniformLocations_.term2TimesOneOverMaxdLpOneOverGamma, c));
	GL(renderShaderProgram_->setUniformValue(renderUniformLocations_.term2TimesOneOverMaxdL, d));
	GL(renderShaderProgram_->setUniformValue(renderUniformLocations_.doSRGB, true));
	GL(renderShaderProgram_->setUniformValue(renderUniformLocations_.flagUseTmGamma, false));
	GL(renderShaderProgram_->setUniformValue(renderUniformLocations_.brightnessScale, atmosphereIntensity));
}

void AtmosphereLightweight::draw(StelCore*const core)
{
	if (layerSolarElevations_.empty()) return;
	if (layerVertexOffsetsInVBO_.empty()) return;
	if (layerIndexOffsetsInBuffer_.empty()) return;
	if (StelApp::getInstance().getVisionModeNight()) return;

	const float atmIntensity = fader.getInterstate();
	if (atmIntensity==0) return;

	const auto prj = core->getProjection(StelCore::FrameAltAz, StelCore::RefractionOff);
	if(!prevProjector_ || !prj->isSameProjection(*prevProjector_))
	{
		prevProjector_ = prj;
		recompileRenderShaders(*prj);
	}
	if (!renderShaderProgram_) return;

	auto& gl = *StelOpenGL::mainContext->functions();

	renderShaderProgram_->bind();
	applyToneReproducerParams(core, atmIntensity);
	prj->setUnProjectUniforms(*renderShaderProgram_);

	GL(gl.glActiveTexture(GL_TEXTURE0));
	renderShaderProgram_->setUniformValue(renderUniformLocations_.atmoTex, 0);

	renderShaderProgram_->setUniformValue(renderUniformLocations_.projectionMatrixInverse,
	                                      prj->getProjectionMatrix().toQMatrix().inverted());

	GL(gl.glEnable(GL_BLEND));
	GL(gl.glBlendFunc(GL_ONE, GL_ONE));
	bindVAO(VAO_RENDER);

	for (int i = 0; i < DRAW_PARAM_COUNT; ++i)
	{
		const auto& p = drawParams[i];
		const float maxValue = std::max(layerValueMaxima_[p.layerToDrawA],
		                                layerValueMaxima_[p.layerToDrawB]);
		GL(gl.glBindTexture(GL_TEXTURE_2D, p.fbo->texture()));
		renderShaderProgram_->setUniformValue(renderUniformLocations_.sunDir, p.dir);

		float colorScale = p.fboColorScale * p.relativeBrightness * p.eclipseFactor;
		colorScale *= maxValue; // restore the scale we used when creating the preparation FBO
		renderShaderProgram_->setUniformValue(renderUniformLocations_.colorScale, colorScale);

		GL(gl.glDrawArrays(GL_TRIANGLE_STRIP, 0, 4));
	}

	releaseVAO(VAO_RENDER);
	GL(gl.glDisable(GL_BLEND));
	GL(gl.glBlendFunc(GL_ONE, GL_ZERO));
	renderShaderProgram_->release();
}

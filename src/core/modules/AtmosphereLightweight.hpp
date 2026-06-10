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

#ifndef ATMOSPHERE_LIGHTWEIGHT_HPP
#define ATMOSPHERE_LIGHTWEIGHT_HPP

#include "Atmosphere.hpp"
#include <memory>
#include <QVector3D>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>

class StelCore;
class TextureAverageComputer;

class AtmosphereLightweight : public Atmosphere
{
public:
	AtmosphereLightweight();
	void computeColor(StelCore* core, double JD, const Planet& currentPlanet, const Planet& sun, const Planet* moon,
	                  const StelLocation& location, float temperature, float relativeHumidity,
	                  float extinctionCoefficient, bool noScatter) override;
	void draw(StelCore* core) override;
	void update(double deltaTime) {fader.update(static_cast<int>(deltaTime*1000));}
	bool isLoading() const override { return false; }
	bool isReadyToRender() const override { return true; }
	LoadingStatus stepDataLoading() override { return {1,1}; }

private:
	std::vector<std::unique_ptr<QOpenGLVertexArrayObject>> vaos_;
	QOpenGLVertexArrayObject renderVAO_;
	QOpenGLBuffer preparationVBO_, renderVBO_;
	QOpenGLBuffer indexBuffer_;
	float sunRelativeBrightness_ = 1, moonRelativeBrightness_ = 0;
	StelProjectorP prevProjector_;
	//! The FBOs where the mesh will be rendered and whose texture will be sampled on actual rendering to screen.
	std::unique_ptr<class QOpenGLFramebufferObject> sunPrepFBO_, moonPrepFBO_;
	std::unique_ptr<class QOpenGLFramebufferObject> luminanceProbeFBO_;
	std::unique_ptr<TextureAverageComputer> textureAverager_;
	std::vector<float> layerSolarElevations_;
	std::vector<float> layerValueMaxima_;
	std::vector<float> layerNorms_;
	std::vector<unsigned> layerVertexOffsetsInVBO_;
	std::vector<unsigned> layerIndexOffsetsInBuffer_;
	enum
	{
		DRAW_PARAM_SUN,
		DRAW_PARAM_MOON,

		DRAW_PARAM_COUNT
	};
	struct DrawParams
	{
		class QOpenGLFramebufferObject* fbo = nullptr;
		QVector3D dir;
		unsigned layerToDrawA = 0;
		unsigned layerToDrawB = 0;
		float layerAlpha = 0; //!< alpha-blending coefficient between #layerToDrawA_ and #layerToDrawB_
		//! Coefficient by which the image in #sunPrepFBO_/#moonPrepFBO_ must be multiplied to get final brightness Y
		float fboColorScale = 0;
		float relativeBrightness = 0;
		float eclipseFactor = 0;
	} drawParams[DRAW_PARAM_COUNT];

	std::unique_ptr<class QOpenGLShaderProgram> preparationShaderProgram_;
	std::unique_ptr<class QOpenGLShaderProgram> luminanceProbeProgram_;
	std::unique_ptr<class QOpenGLShaderProgram> renderShaderProgram_;
	struct
	{
		// ToneReproducer attributes
		int doSRGB;
		int oneOverGamma;
		int brightnessScale;
		int alphaWaOverAlphaDa;
		int term2TimesOneOverMaxdLpOneOverGamma; // original
		int term2TimesOneOverMaxdL;              // challenge by Ruslan
		int flagUseTmGamma;                      // switch between their use, true to use the first expression.

		// Renderer attributes
		int sunDir;
		int atmoTex;
		int colorScale;
		int projectionMatrixInverse;
	} renderUniformLocations_;

	struct
	{
		int isMoon;
		int sunDir;
		int atmoTex;
		int projectionMatrixInverse;
	} luminanceProbeUniformLocations_;

	struct
	{
		int colorScale;
	} prepProgramUniformLocations_;

	void loadMesh();
	//! Negative are special labels to denote single VAOs, others are offsets in #vaos_
	enum
	{
		VAO_RENDER = -1,
	};
	float computeAverageLuminance(const StelCore* core);
	//! Binds actual VAO if it's supported, sets up the relevant state manually otherwise.
	void bindVAO(int layer);
	//! Sets the vertex attribute states for the currently bound VAO so that glDraw* commands can work.
	void setupCurrentVAO(int layer);
	//! Binds zero VAO if VAO is supported, manually disables the relevant vertex attributes otherwise.
	void releaseVAO(int layer);
	void recompileRenderShaders(const StelProjector& projector);
	void applyToneReproducerParams(StelCore* core, float atmosphereIntensity);
	void computeDrawParams(const StelCore* core, const Planet* lightSource, const Planet* moon, int paramIdx, bool noScatter);
};

#endif // ATMOSTPHERE_HPP

/*
 * Stellarium
 * Copyright (C) 2020 Ruslan Kabatsayev
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

#ifndef ATMOSPHERE_SHOWMYSKY_HPP
#define ATMOSPHERE_SHOWMYSKY_HPP

#include "Atmosphere.hpp"
#include "VecMath.hpp"

#include "Skybright.hpp"

#include <array>
#include <cmath>
#include <memory>
#include <stdexcept>
#include <QLibrary>
#include <QOpenGLBuffer>

#ifdef ENABLE_SHOWMYSKY
#include <ShowMySky/AtmosphereRenderer.hpp>
#endif


class StelProjector;
class StelToneReproducer;
class StelCore;
class QOpenGLFunctions;

class AtmosphereShowMySky : public Atmosphere, public QObject
{
public:
	AtmosphereShowMySky();
	~AtmosphereShowMySky();

	void computeColor(StelCore* core, double JD, const Planet& currentPlanet, const Planet& sun, const Planet* moon,
					  const StelLocation& location, float temperature, float relativeHumidity,
					  float extinctionCoefficient, bool noScatter) override;
	void draw(StelCore* core) override;
	bool isLoading() override;
	bool isReadyToRender() override;
	LoadingStatus stepDataLoading() override;

	struct InitFailure : std::runtime_error
	{
		using std::runtime_error::runtime_error;
		InitFailure(QString const& what) : std::runtime_error(what.toStdString()) {}
	};

private:
#ifdef ENABLE_SHOWMYSKY
	QLibrary showMySkyLib;
	Vec4i viewport;
	int gridMaxY,gridMaxX;

	QVector<Vec2f> posGrid;
	QOpenGLBuffer posGridBuffer;
	QOpenGLBuffer indexBuffer;
	QVector<Vec4f> viewRayGrid;
	QOpenGLBuffer viewRayGridBuffer;

	std::unique_ptr<QOpenGLShaderProgram> luminanceToScreenProgram_;
	decltype(::ShowMySky_AtmosphereRenderer_create)* ShowMySky_AtmosphereRenderer_create=nullptr;

	struct {
		int rgbMaxValue;
		int bayerPattern;
		int oneOverGamma;
		int brightnessScale;
		int luminanceTexture;
		int alphaWaOverAlphaDa;
		int term2TimesOneOverMaxdLpOneOverGamma; // original
		int term2TimesOneOverMaxdL;              // challenge by Ruslan
		int flagUseTmGamma;                      // switch between their use, true to use the first expression.
	} shaderAttribLocations;

	GLuint bayerPatternTex_=0;

	float prevFad = 0, prevFov = 0;
	Vec3d prevSun = Vec3d(0,0,0);
	int prevWidth_=0, prevHeight_=0;
	GLuint renderVAO_=0, luminanceToScreenVAO_=0, zenithProbeVAO_=0, vbo_=0;
	std::unique_ptr<ShowMySky::AtmosphereRenderer> renderer_;
	std::unique_ptr<ShowMySky::Settings> skySettings_;
	std::function<void(QOpenGLShaderProgram&)> renderSurfaceFunc_;
	float sunVisibility_ = 1;
	float lastUsedAltitude_ = NAN;
	float lightPollutionUnitLuminance_=0, airglowUnitLuminance_=0;

	enum class EclipseSimulationQuality
	{
		Fastest,
		AllPrecomputed,
		Order2OnTheFly_Order1Precomp,
		AllOnTheFly,
	} eclipseSimulationQuality_ = EclipseSimulationQuality::AllPrecomputed;

	bool propertiesInited_ = false;

	void initProperties();
	void resolveFunctions();
	void loadShaders();
	void setupBuffers();
	void regenerateGrid();
	void setupRenderTarget();
	// Gets average value of the pixels rendered to the FBO texture as the value of the deepest mipmap level
	Vec4f getMeanPixelValue(int texW, int texH);
	void resizeRenderTarget(int width, int height);
	void drawAtmosphere(Mat4f const& projectionMatrix, float sunAzimuth, float sunZenithAngle, float sunAngularRadius,
						float moonAzimuth, float moonZenithAngle, float earthMoonDistance, float altitude,
	                    float brightness, float lightPollutionGroundLuminance, float airglowRelativeBrightness,
	                    bool drawAsEclipse, bool clearTarget);
	void probeZenithLuminances(float altitude);
#endif // ENABLE_SHOWMYSKY
};

#endif // ATMOSTPHERE_HPP

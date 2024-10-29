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
class TextureAverageComputer;

class AtmosphereShowMySky : public Atmosphere, public QObject
{
public:
	AtmosphereShowMySky(double initialAltitude);
	~AtmosphereShowMySky();

	void computeColor(StelCore* core, double JD, const Planet& currentPlanet, const Planet& sun, const Planet* moon,
					  const StelLocation& location, float temperature, float relativeHumidity,
					  float extinctionCoefficient, bool noScatter) override;
	void draw(StelCore* core) override;
	bool isLoading() const override;
	bool isReadyToRender() const override;
	LoadingStatus stepDataLoading() override;

private:
#ifdef ENABLE_SHOWMYSKY
	QLibrary showMySkyLib;
	Vec4i viewport;
	int gridMaxY,gridMaxX;
	/*!	To achieve higher frame rates on slow systems,
	 *	the configuration parameter in config.ini:
	 *		[landscape]
	 *		atmosphere_resolution_reduction
	 *	allows reducing the resolution of the skylight texture.
	 *	Preferred values are:
	 *		1 ~> full resolution (default)
	 *		2 ~> half resolution
	 *		4 ~> quarter resolution
	 */
	int reducedResolution;
	/*!	The configuration switch in config.ini:
	 *		[landscape]
	 *		flag_atmosphere_dynamic_resolution
	 *	allows to use the reduced resolution only while moving the view,
	 *	when panning, zooming, dimming or in time-lapse mode.
	 *	With the real-time display, on the other hand, the full resolution is retained.
	 *	Possible values are:
	 *		false ~> static resolution (default)
	 *		true ~> dynamic resolution
	 *	Note:
	 *	In dynamic resolution mode, a motion analyzer selects either full or reduced resolution.
	 *	The change in resolution could be particularly visible in close proximity to the Sun.
	 *	Especially at full resolution, frames will be skipped depending on the speed of movement.
	 */
	bool flagDynamicResolution;

	std::unique_ptr<QOpenGLShaderProgram> luminanceToScreenProgram_;
	decltype(::ShowMySky_AtmosphereRenderer_create)* ShowMySky_AtmosphereRenderer_create=nullptr;

	struct {
		int doSRGB;
		int oneOverGamma;
		int brightnessScale;
		int luminanceTexture;
		int alphaWaOverAlphaDa;
		int term2TimesOneOverMaxdLpOneOverGamma; // original
		int term2TimesOneOverMaxdL;              // challenge by Ruslan
		int flagUseTmGamma;                      // switch between their use, true to use the first expression.
	} shaderAttribLocations;

	StelProjectorP prevProjector_;
	std::unique_ptr<TextureAverageComputer> textureAverager_;

	float prevFad=0, prevFov=0;
	Vec3d prevPos=Vec3d(0,0,0), prevSun=Vec3d(0,0,0);
	int prevWidth_=0, prevHeight_=0, dynResTimer=0, prevRes=0, atmoRes=1;
	GLuint mainVAO_=0, zenithProbeVAO_=0, vbo_=0;
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
	Vec4f getMeanPixelValue();
	void resizeRenderTarget(int width, int height);
	void drawAtmosphere(Mat4f const& projectionMatrix, float sunAzimuth, float sunZenithAngle, float sunAngularRadius,
	                    float moonAzimuth, float moonZenithAngle, float earthMoonDistance, float altitude,
	                    float brightness, float lightPollutionGroundLuminance, float airglowRelativeBrightness,
	                    bool drawAsEclipse, bool clearTarget);
	bool dynamicResolution(StelProjectorP prj, Vec3d &sunPos, int width, int height);
	std::pair<QByteArray,QByteArray> getViewDirShaderSources(const StelProjector& projector) const;
	void probeZenithLuminances(float altitude);
#endif // ENABLE_SHOWMYSKY
};

#endif // ATMOSTPHERE_HPP

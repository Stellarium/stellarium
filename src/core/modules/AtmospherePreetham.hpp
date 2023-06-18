/*
 * Stellarium
 * Copyright (C) 2003 Fabien Chereau
 * Copyright (C) 2012 Timothy Reaves
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

#ifndef ATMOSPHERE_PREETHAM_HPP
#define ATMOSPHERE_PREETHAM_HPP

#include "Atmosphere.hpp"
#include "VecMath.hpp"

#include "Skybright.hpp"
#include "StelFader.hpp"

#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>

class StelProjector;
class StelToneReproducer;
class StelCore;
class Skylight;

//! Compute and display the daylight sky color using OpenGL.
//! The sky brightness is computed with the SkyBright class, the color with the SkyLight.
//! Don't use this class directly but use it through the LandscapeMgr.
class AtmospherePreetham : public Atmosphere
{
public:
	AtmospherePreetham(Skylight& sky);
	virtual ~AtmospherePreetham();

	//! Compute sky brightness values and average luminance.
	//! @param noScatter true to suppress the actual sky brightness modelling. This will keep refraction/extinction working for didactic reasons.
	void computeColor(StelCore* core, double JD, const Planet& currentPlanet, const Planet& sun, const Planet* moon,
					  const StelLocation& location, float temperature, float relativeHumidity,
					  float extinctionCoefficient, bool noScatter) override;
	void draw(StelCore* core) override;
	void update(double deltaTime) {fader.update(static_cast<int>(deltaTime*1000));}
	bool isLoading() const override { return false; }
	bool isReadyToRender() const override { return true; }
	LoadingStatus stepDataLoading() override { return {1,1}; }

private:
	Vec4i viewport;
	Skylight& sky;
	Skybright skyb;
	unsigned int skyResolutionY,skyResolutionX;

	Vec2f* posGrid;
	QOpenGLBuffer posGridBuffer;
	QOpenGLBuffer indicesBuffer;
	Vec4f* colorGrid;
	QOpenGLBuffer colorGridBuffer;
	QOpenGLVertexArrayObject vao;

	//! Vertex shader used for xyYToRGB computation
	class QOpenGLShaderProgram* atmoShaderProgram;
	struct {
		int ditherPattern;
		int rgbMaxValue;
		int alphaWaOverAlphaDa;
		int oneOverGamma;
		int term2TimesOneOverMaxdLpOneOverGamma; // original
		int term2TimesOneOverMaxdL;              // challenge by Ruslan
		int flagUseTmGamma;                      // switch between their use, true to use the first expression.
		int brightnessScale;
		int sunPos;
		int term_x, Ax, Bx, Cx, Dx, Ex;
		int term_y, Ay, By, Cy, Dy, Ey;
		int projectionMatrix;
		int skyVertex;
		int skyColor;
		int doSRGB;
	} shaderAttribLocations;

	StelTextureSP ditherPatternTex;

	//! Binds actual VAO if it's supported, sets up the relevant state manually otherwise.
	void bindVAO();
	//! Sets the vertex attribute states for the currently bound VAO so that glDraw* commands can work.
	void setupCurrentVAO();
	//! Binds zero VAO if VAO is supported, manually disables the relevant vertex attributes otherwise.
	void releaseVAO();
};

#endif // ATMOSPHERE_PREETHAM_HPP

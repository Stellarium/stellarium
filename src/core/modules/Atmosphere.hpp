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

#ifndef _ATMOSTPHERE_HPP_
#define _ATMOSTPHERE_HPP_

#include "Skylight.hpp"
#include "VecMath.hpp"

#include "renderer/StelIndexBuffer.hpp"
#include "renderer/StelVertexBuffer.hpp"
#include "Skybright.hpp"
#include "StelFader.hpp"
#include "StelProjector.hpp"

class StelProjector;
class StelToneReproducer;
class StelCore;

//! Compute and display the daylight sky color.
//! The sky brightness is computed with the SkyBright class, the color with the SkyLight.
//! Don't use this class directly but use it through the LandscapeMgr.
class Atmosphere
{
public:
	Atmosphere(void);
	virtual ~Atmosphere(void);

	//! Called on every update to recompute colors of the atmosphere.
	//!
	//! Must be called at least once after a call to draw(), as vertexGrid 
	//! is lazily initialized at the first draw call.
	void computeColor(double JD, Vec3d _sunPos, Vec3d moonPos, float moonPhase, StelCore* core,
					  float eclipseFac, float latitude = 45.f, float altitude = 200.f,
					  float temperature = 15.f, float relativeHumidity = 40.f);
	void draw(StelCore* core, class StelRenderer* renderer);
	void update(double deltaTime) {fader.update((int)(deltaTime*1000));}

	//! Set fade in/out duration in seconds
	void setFadeDuration(float duration) {fader.setDuration((int)(duration*1000.f));}
	//! Get fade in/out duration in seconds
	float getFadeDuration() {return (float)fader.getDuration()/1000.f;}

	//! Define whether to display atmosphere
	void setFlagShow(bool b){fader = b;}
	//! Get whether atmosphere is displayed
	bool getFlagShow() const {return fader;}

	//! Get the actual atmosphere intensity due to eclipses + fader
	//! @return the display intensity ranging from 0 to 1
	float getRealDisplayIntensityFactor(void) const {return fader.getInterstate()*eclipseFactor;}

	// let's you know how far faded in or out the atm is (0-1)
	float getFadeIntensity(void) const {return fader.getInterstate();}

	//! Get the average luminance of the atmosphere in cd/m2
	//! If atmosphere is off, the luminance includes the background starlight + light pollution.
	//! Otherwise it includes the atmosphere + background starlight + eclipse factor + light pollution.
	//! @return the last computed average luminance of the atmosphere in cd/m2.
	float getAverageLuminance(void) const {return averageLuminance;}

	//! Set the light pollution luminance in cd/m^2
	void setLightPollutionLuminance(float f) { lightPollutionLuminance = f; }
	//! Get the light pollution luminance in cd/m^2
	float getLightPollutionLuminance() const { return lightPollutionLuminance; }

private:
	// Vertex with a 2D position and a color.
	struct Vertex
	{
		Vec2f position;
		Vec4f color;
		Vertex(const Vec2f position, const Vec4f& color) : position(position), color(color) {}
		VERTEX_ATTRIBUTES(Vec2f Position, Vec4f Color);
	};

	Vec4i viewport;
	Skylight sky;
	Skybright skyb;
	int skyResolutionY, skyResolutionX;

	//! The average luminance of the atmosphere in cd/m2
	float averageLuminance;
	float eclipseFactor;
	LinearFader fader;
	float lightPollutionLuminance;

	//! Shader used for xyYToRGB computation. If NULL, shader is not used.
	class StelGLSLShader* shader;

	//! Rectangular grid of vertices making up the atmosphere.
	StelVertexBuffer<Vertex>* vertexGrid;

	//! Index buffers representing triangle strips for each row in the grid.
	QVector<StelIndexBuffer*> rowIndices;

	//! Renderer used to construct row index buffers at viewport changes.
	//!
	//! Lazily initialized - NULL until the first draw call.
	class StelRenderer* renderer;

	//! Lazily loads the shader used for drawing.
	//!
	//! Called at the first call to draw().
	//!
	//! This should only be called if the Renderer supports GLSL.
	//!
	//! @return true on success, false on failure (allowing for a shader-less fallback).
	bool lazyLoadShader(class StelRenderer* renderer);

	//! Update the vertex grid and index buffers.
	//!
	//! Called by computeColor after the viewport changes.
	void updateGrid(const StelProjectorP projector);
};

#endif // _ATMOSTPHERE_HPP_

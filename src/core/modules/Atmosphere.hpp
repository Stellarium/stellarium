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

#include "Skybright.hpp"
#include "StelFader.hpp"

#ifdef USE_OPENGL_ES2
 #include "GLES2/gl2.h"
#else
 #include <QtOpenGL>
#endif

class StelProjector;
class StelToneReproducer;
class StelCore;

//! Compute and display the daylight sky color using openGL.
//! The sky brightness is computed with the SkyBright class, the color with the SkyLight.
//! Don't use this class directly but use it through the LandscapeMgr.
class Atmosphere
{
public:
	Atmosphere(void);
	virtual ~Atmosphere(void);
	void computeColor(double JD, Vec3d _sunPos, Vec3d moonPos, float moonPhase, StelCore* core,
		float latitude = 45.f, float altitude = 200.f,
		float temperature = 15.f, float relativeHumidity = 40.f);
	void draw(StelCore* core);
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
	Vec4i viewport;
	Skylight sky;
	Skybright skyb;
	int skyResolutionY,skyResolutionX;

	Vec2f* posGrid;
	Vec4f* colorGrid;
	unsigned int* indices;

	//! The average luminance of the atmosphere in cd/m2
	float averageLuminance;
	float eclipseFactor;
	LinearFader fader;
	float lightPollutionLuminance;

	//! Whether vertex shader should be used
	bool useShader;

	//! Vertex shader used for xyYToRGB computation
	class QGLShaderProgram* atmoShaderProgram;
	struct {
		int alphaWaOverAlphaDa;
		int oneOverGamma;
		int term2TimesOneOverMaxdLpOneOverGamma;
		int brightnessScale;
		int sunPos;
		int term_x, Ax, Bx, Cx, Dx, Ex;
		int term_y, Ay, By, Cy, Dy, Ey;
		int projectionMatrix;
		int skyVertex;
		int skyColor;
	} shaderAttribLocations;
};

#endif // _ATMOSTPHERE_HPP_

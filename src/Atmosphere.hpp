/*
 * Stellarium
 * Copyright (C) 2003 Fabien Chereau
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef _STEL_ATMOSTPHERE_H_
#define _STEL_ATMOSTPHERE_H_

#include "Skylight.hpp"
#include "vecmath.h"
#include "Navigator.hpp"
#include "Skybright.hpp"
#include "Fader.hpp"

using namespace std;

class Projector;
class ToneReproducer;

//! @brief Compute and display the daylight sky color using openGL
//! the sky is computed with the Skylight class.
class Atmosphere
{
public:
    Atmosphere(void);
    virtual ~Atmosphere(void);
	void compute_color(double JD, Vec3d sunPos, Vec3d moonPos, float moon_phase, ToneReproducer * eye, Projector* prj,
		float latitude = 45.f, float altitude = 200.f,
		float temperature = 15.f, float relative_humidity = 40.f);
	void draw(Projector* prj);
	void update(double deltaTime) {fader.update((int)(deltaTime*1000));}
	
	//! Set fade in/out duration in seconds
	void setFadeDuration(float duration) {fader.set_duration((int)(duration*1000.f));}
	//! Get fade in/out duration in seconds
	float getFadeDuration() {return fader.get_duration()/1000.f;}
	
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

	void setLightPollutionLuminance(float f) { lightPollutionLuminance = f; }
	float getLightPollutionLuminance() const { return lightPollutionLuminance; }

private:
	Vector4<GLint> viewport;
	Skylight sky;
	Skybright skyb;
	int sky_resolution_y,sky_resolution_x;
	struct GridPoint {
		Vec2f pos_2d;
		Vec3f color;
	};
	GridPoint *grid;	// For Atmosphere calculation

	//! The average luminance of the atmosphere in cd/m2
	float averageLuminance;
	double eclipseFactor;
	ParabolicFader fader;
	float lightPollutionLuminance;
};

#endif // _STEL_ATMOSTPHERE_H_

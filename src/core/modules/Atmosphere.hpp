/*
 * Stellarium
 * Copyright (C) 2003 Fabien Chereau
 * Copyright (C) 2012 Timothy Reaves
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

#ifndef ATMOSPHERE_HPP
#define ATMOSPHERE_HPP

#include <stdexcept>
#include "StelCore.hpp"
#include "StelFader.hpp"

#include <QOpenGLBuffer>

class StelProjector;
class StelToneReproducer;
class StelLocation;
class StelCore;
class Planet;

Q_DECLARE_LOGGING_CATEGORY(Atmo)

class Atmosphere
{
public:
	struct LoadingStatus
	{
		int stepsDone;
		int stepsToDo;
	};

	struct InitFailure : std::runtime_error
	{
		using std::runtime_error::runtime_error;
		InitFailure(QString const& what) : std::runtime_error(what.toStdString()) {}
	};

public:
	virtual ~Atmosphere() = default;
	//! Compute sky brightness values and average luminance.
	//! @param noScatter true to suppress the actual sky brightness modelling. This will keep refraction/extinction working for didactic reasons.
	virtual void computeColor(StelCore* core, double JD, const Planet& currentPlanet, const Planet& sun, const Planet* moon,
							  const StelLocation& location, float temperature, float relativeHumidity, float extinctionCoefficient,
							  bool noScatter) = 0;
	virtual void draw(StelCore* core) = 0;
	virtual bool isLoading() const = 0;
	virtual bool isReadyToRender() const = 0;
	virtual LoadingStatus stepDataLoading() = 0;
	void update(double deltaTime) {fader.update(static_cast<int>(deltaTime*1000));}

	//! Set fade in/out duration in seconds
	void setFadeDuration(float duration) {fader.setDuration(static_cast<int>(duration*1000.f));}
	//! Get fade in/out duration in seconds
	float getFadeDuration() const {return static_cast<float>(fader.getDuration()/1000.f);}

	//! Define whether to display atmosphere
	void setFlagShow(bool b){fader = b;}
	//! Get whether atmosphere is displayed
	bool getFlagShow() const {return fader;}

	//! Get the actual atmosphere intensity due to eclipses + fader
	//! @return the display intensity ranging from 0 to 1
	float getRealDisplayIntensityFactor() const {return fader.getInterstate()*eclipseFactor;}

	// lets you know how far faded in or out the atmosphere is (0..1)
	float getFadeIntensity() const {return fader.getInterstate();}

	//! Get the average luminance of the atmosphere in cd/m2
	//! If atmosphere is off, the luminance equals the background starlight (0.001cd/m2).
	// TODO: Find reference for this value? Why 1 mcd/m2 without atmosphere and 0.1 mcd/m2 inside? Absorption?
	//! Otherwise it includes the (atmosphere + background starlight (0.0001cd/m2) * eclipse factor + light pollution.
	//! @return the last computed average luminance of the atmosphere in cd/m2.
	float getAverageLuminance() const {return averageLuminance;}

	//! override computable luminance. This is for special operations only, e.g. for scripting of brightness-balanced image export.
	//! To return to auto-computed values, set any negative value at the end of the script.
	void setAverageLuminance(float overrideLum)
	{
		if (overrideLum<0.f)
		{
			overrideAverageLuminance=false;
			averageLuminance=0.f;
		}
		else
		{
			overrideAverageLuminance=true;
			averageLuminance=overrideLum;
		}
	}

	//! Set the light pollution luminance in cd/m^2
	void setLightPollutionLuminance(float f) { lightPollutionLuminance = f; }
	//! Get the light pollution luminance in cd/m^2
	float getLightPollutionLuminance() const { return lightPollutionLuminance; }

	//! Get the multiplier applied to averageLuminance when computing the
	//! star-visibility adaptation luminance reported to StelSkyDrawer.
	//! Each atmosphere model produces averageLuminance in a different scale,
	//! so this multiplier must be tuned per model.  The default (1000) is
	//! calibrated for the Preetham/Schaefer model to reach full stellar
	//! visibility at approximately -18 deg sun altitude (astronomical twilight).
	//! ShowMySky uses the same value (see AtmosphereShowMySky.hpp).
	virtual float getStarAdaptationMultiplier() const { return 1000.f; }

	//! Store the natural sky adaptation luminance for atmosphere rendering.
	//! This is the un-inflated value used by the atmosphere shader so that
	//! the sky tone-maps correctly even when the star-visibility adaptation
	//! luminance reported via StelSkyDrawer::reportLuminanceInFov() differs.
	void setSkyAdaptationLuminance(float lum) { skyAdaptationLuminance = lum; }
	//! Get the natural sky adaptation luminance stored for atmosphere rendering.
	float getSkyAdaptationLuminance() const { return skyAdaptationLuminance; }

protected:
	//! The average luminance of the atmosphere in cd/m2
	float averageLuminance = 0;
	bool overrideAverageLuminance = false; // if true, don't compute but keep value set via setAverageLuminance(float)
	float eclipseFactor = 1;
	LinearFader fader;
	float lightPollutionLuminance = 0;
	//! Natural sky adaptation luminance for atmosphere shader tone-mapping,
	//! decoupled from the star-visibility adaptation luminance.
	float skyAdaptationLuminance = 50.f;
};

#endif // ATMOSPHERE_HPP

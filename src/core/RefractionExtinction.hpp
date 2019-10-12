/*
 * Stellarium
 * Copyright (C) 2010 Fabien Chereau
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
 *
 * Refraction and extinction computations.
 * Implementation: 2010-03-23 GZ=Georg Zotti, Georg.Zotti@univie.ac.at
 * 2010-12 FC split into 2 classes, implemented Refraction */

#ifndef REFRACTIONEXTINCTION_HPP
#define REFRACTIONEXTINCTION_HPP
// USABILITY: added 3 more flags/switches in GUI:
// Temperature [C] [influences refraction]
// Pressure [mbar]  [influences refraction]
// extinction Coeff. k=0...(0.01)...1, [if k=0, no ext. effect]
// SUGGESTION: Allow Temperature/Pressure/ex.Coeff./LightPollution set in the landscape files

#include "VecMath.hpp"
#include "StelProjector.hpp"

//! @class Extinction
//! This class performs extinction computations, following literature from atmospheric optics and astronomy.
//! Airmass computations are limited to meaningful altitudes.
//! The solution provided here will [hopefully!] result in a visible "zone of avoidance" near the horizon down to altitude -2,
//! and may show stars in their full brightness below -2 degrees.
//! Typical horizons do not go down below -1, so all natural sites should be covered.
//! Note that forward/backward are no absolute reverse operations!
//! All the computations should be in effect
//! (1) only if atmosphere effects are true
//! (2) only for terrestrial locations, not on Moon/Mars/Saturn etc
//! config.ini:astro/flag_extinction_below_horizon=true|false controls if extinction kills objects below -2 degrees altitude by setting airmass to 42.
class Extinction
{
public:
	//! Define the extinction strategy for rendering underground objects (useful when ground is not rendered)
	enum UndergroundExtinctionMode {
		UndergroundExtinctionZero = 0,	//!< Zero extinction: stars visible in full brightness
		UndergroundExtinctionMax = 1,   //!< Maximum extinction: coef 42, i.e practically invisible
		UndergroundExtinctionMirror = 2 //!< Mirror the extinction for the same altutide above the ground.
	};
	
	Extinction();
	
	//! Compute extinction effect for given position vector and magnitude.
	//! @param altAzPos is the NORMALIZED (!!) (geometrical) star position vectors, and its z component is therefore sin(geometric_altitude).
	//! This call must therefore be done before application of Refraction if atmospheric effects are on.
	//! Note that forward/backward are no absolute reverse operations!
	void forward(const Vec3d& altAzPos, float* mag) const
	{
		//Q_ASSERT(std::fabs(altAzPos.length()-1.)<0.001);
		*mag += airmass(static_cast<float>(altAzPos[2]), false) * ext_coeff;
	}
	
	void forward(const Vec3f& altAzPos, float* mag) const
	{
		//Q_ASSERT(std::fabs(altAzPos.length()-1.f)<0.001f);
		*mag += airmass(altAzPos[2], false) * ext_coeff;
	}

	//! Compute inverse extinction effect for given position vector and magnitude.
	//! @param altAzPos is the NORMALIZED (!!) (geometrical) star position vector, and its z component is therefore sin(geometric_altitude).
	//! This call must therefore be done after application of Refraction effects if atmospheric effects are on.
	//! Note that forward/backward are no absolute reverse operations!
	void backward(const Vec3d& altAzPos, float* mag) const
	{
		*mag -= airmass(static_cast<float>(altAzPos[2]), false) * ext_coeff;
	}
	
	void backward(const Vec3f& altAzPos, float* mag) const
	{
		*mag -= airmass(altAzPos[2], false) * ext_coeff;
	}

	//! Set visual extinction coefficient (mag/airmass), influences extinction computation.
	//! @param k= 0.1 for highest mountains, 0.2 for very good lowland locations, 0.35 for typical lowland, 0.5 in humid climates.
	void setExtinctionCoefficient(float k) { ext_coeff=k; }
	float getExtinctionCoefficient() const {return ext_coeff;}

	void setUndergroundExtinctionMode(UndergroundExtinctionMode mode) {undergroundExtinctionMode=mode;}
	UndergroundExtinctionMode getUndergroundExtinctionMode() const {return undergroundExtinctionMode;}
	
	//! airmass computation for @param cosZ = cosine of zenith angle z (=sin(altitude)!).
	//! The default (@param apparent_z = true) is computing airmass from observed altitude, following Rozenberg (1966) [X(90)~40].
	//! if (@param apparent_z = false), we have geometrical altitude and compute airmass from that,
	//! following Young: Air mass and refraction. Applied Optics 33(6), pp.1108-1110, 1994. [X(90)~32].
	//! A problem is that refraction depends on air pressure and temperature, but Young's formula assumes T=15C, p=1013.25mbar.
	//! So, it seems better to compute refraction first, and then use the Rozenberg formula here.
	//! Rozenberg is infinite at Z=92.17 deg, Young at Z=93.6 deg, so this function RETURNS SUBHORIZONTAL_AIRMASS BELOW -2 DEGREES!
	float airmass(float cosZ, const bool apparent_z=true) const;

private:
	//! k, magnitudes/airmass, in [0.00, ... 1.00], (default 0.20).
	float ext_coeff;

	//! Define what we are going to do for underground stars when ground is not rendered
	UndergroundExtinctionMode undergroundExtinctionMode;
};

//! @class Refraction
//! This class performs refraction computations, following literature from atmospheric optics and astronomy.
//! Refraction solutions can only be approximate, given the turbulent, unpredictable real atmosphere.
//! Typical horizons do not go down below -1, so strange effects (distortion) between -2 and -5 should be covered.
//! Note that forward/backward are no absolute reverse operations!
//! All the computations should be in effect
//! (1) only if atmosphere effects are true
//! (2) only for celestial objects, never for landscape images
//! (3) only for terrestrial locations, not on Moon/Mars/Saturn etc

class Refraction: public StelProjector::ModelViewTranform
{
public:
	Refraction();

	//! Apply refraction.
	//! @param altAzPos is the geometrical star position vector, to be transformed into apparent position.
	//! Note that forward/backward are no absolute reverse operations!
	void forward(Vec3d& altAzPos) const;

	//! Remove refraction from position ("reduce").
	//! @param altAzPos is the apparent star position vector, to be transformed into geometrical position.
	//! Note that forward/backward are no absolute reverse operations!
	void backward(Vec3d& altAzPos) const;

	//! Apply refraction.
	//! @param altAzPos is the geometrical star position vector, to be transformed into apparent position.
	//! Note that forward/backward are no absolute reverse operations!
	void forward(Vec3f& altAzPos) const;

	//! Remove refraction from position ("reduce").
	//! @param altAzPos is the apparent star position vector, to be transformed into geometrical position.
	//! Note that forward/backward are no absolute reverse operations!
	void backward(Vec3f& altAzPos) const;

	void combine(const Mat4d& m)
	{
		setPreTransfoMat(preTransfoMat*m);
	}

	Mat4d getApproximateLinearTransfo() const {return postTransfoMat*preTransfoMat;}

	StelProjector::ModelViewTranformP clone() const {Refraction* refr = new Refraction(); *refr=*this; return StelProjector::ModelViewTranformP(refr);}

	//! Set surface air pressure (mbars), influences refraction computation.
	void setPressure(float p_mbar);
	float getPressure() const {return pressure;}

	//! Set surface air temperature (degrees Celsius), influences refraction computation.
	void setTemperature(float t_C);
	float getTemperature() const {return temperature;}

	//! Set the transformation matrices used to transform input vector to AltAz frame.
	void setPreTransfoMat(const Mat4d& m);
	void setPostTransfoMat(const Mat4d& m);

private:
	//! Update precomputed variables.
	void updatePrecomputed();

	void innerRefractionForward(Vec3d& altAzPos) const;
	void innerRefractionBackward(Vec3d& altAzPos) const;
	
	//! These 3 Atmosphere parameters can be controlled by GUI.
	//! Pressure[mbar] (1013)
	float pressure;
	//! Temperature[Celsius deg] (10).
	float temperature;
	//! Correction factor for refraction formula, to be cached for speed.
	float press_temp_corr;

	//! Used to pretransform coordinates into AltAz frame.
	Mat4d preTransfoMat;
	Mat4d invertPreTransfoMat;
	Mat4f preTransfoMatf;
	Mat4f invertPreTransfoMatf;

	//! Used to postransform refracted coordinates from AltAz to view.
	Mat4d postTransfoMat;
	Mat4d invertPostTransfoMat;
	Mat4f postTransfoMatf;
	Mat4f invertPostTransfoMatf;
};

#endif  // REFRACTIONEXTINCTION_HPP

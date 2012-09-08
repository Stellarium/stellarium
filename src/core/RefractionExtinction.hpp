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

#ifndef _REFRACTIONEXTINCTION_HPP_
#define _REFRACTIONEXTINCTION_HPP_
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
	Extinction();
	//! Compute extinction effect for arrays of size @param num position vectors and magnitudes.
	//! @param altAzPos are the NORMALIZED (!!) (apparent) star position vectors, and their z components sin(apparent_altitude).
	//! @param mag the magnitudes
	//! This call must therefore be done after application of Refraction, and only if atmospheric effects are on.
	//! Note that forward/backward are no absolute reverse operations!
	void forward(const Vec3d *altAzPos, float *mag, const int num=1) const;
	void forward(const Vec3f *altAzPos, float *mag, const int num=1) const;
	void forward(const double *sinAlt,  float *mag, const int num) const;
	void forward(const float  *sinAlt,  float *mag, const int num) const;
	void forward(const double *sinAlt,  float *mag) const;
	void forward(const float  *sinAlt,  float *mag) const;

	//! Compute inverse extinction effect for arrays of size @param num position vectors and magnitudes.
	//! @param altAzPos are the NORMALIZED (!!) (apparent) star position vectors, and their z components sin(apparent_altitude).
	//! @param mag the magnitudes
	//! Note that forward/backward are no absolute reverse operations!
	void backward(const Vec3d *altAzPos, float *mag, const int num=1) const;
	void backward(const Vec3f *altAzPos, float *mag, const int num=1) const;
	void backward(const double *sinAlt,  float *mag, const int num=1) const;
	void backward(const float  *sinAlt,  float *mag, const int num=1) const;

	//! Set visual extinction coefficient (mag/airmass), influences extinction computation.
	//! @param k= 0.1 for highest mountains, 0.2 for very good lowland locations, 0.35 for typical lowland, 0.5 in humid climates.
	void setExtinctionCoefficient(float k) { ext_coeff=k; }
	float getExtinctionCoefficient() const {return ext_coeff;}

private:
	//! airmass computation for @param cosZ = cosine of zenith angle z (=sin(altitude)!).
	//! The default (@param apparent_z = true) is computing airmass from observed altitude, following Rozenberg (1966) [X(90)~40].
	//! if (@param apparent_z = false), we have geometrical altitude and compute airmass from that,
	//! following Young: Air mass and refraction. Applied Optics 33(6), pp.1108-1110, 1994. [X(90)~32].
	//! A problem ist that refraction depends on air pressure and temperature, but Young's formula assumes T=15C, p=1013.25mbar.
	//! So, it seems better to compute refraction first, and then use the Rozenberg formula here.
	//! Rozenberg is infinite at Z=92.17 deg, Young at Z=93.6 deg, so this function RETURNS SUBHORIZONTAL_AIRMASS BELOW -2 DEGREES!
	float airmass(const float cosZ, const bool apparent_z=true) const;

	//! k, magnitudes/airmass, in [0.00, ... 1.00], (default 0.20).
	float ext_coeff;
	//! should be either 0.0 (stars visible in full brightness below horizon) or 40.0 (or 42? ;-) practically invisible)
	//! Maybe make this a user-configurable option?
	static float SUBHORIZONTAL_AIRMASS;
};

//! @class Refraction
//! This class performs refraction computations, following literature from atmospheric optics and astronomy.
//! Refraction solutions can only be aproximate, given the turbulent, unpredictable real atmosphere.
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
	virtual void forward(Vec3d& altAzPos) const;

	//! Remove refraction from position ("reduce").
	//! @param altAzPos is the apparent star position vector, to be transformed into geometrical position.
	//! Note that forward/backward are no absolute reverse operations!
	virtual void backward(Vec3d& altAzPos) const;

	//! Apply refraction.
	//! @param altAzPos is the geometrical star position vector, to be transformed into apparent position.
	//! Note that forward/backward are no absolute reverse operations!
	virtual void forward(Vec3f& altAzPos) const;

	//! Remove refraction from position ("reduce").
	//! @param altAzPos is the apparent star position vector, to be transformed into geometrical position.
	//! Note that forward/backward are no absolute reverse operations!
	virtual void backward(Vec3f& altAzPos) const;

	virtual void combine(const Mat4d& m)
	{
		setPreTransfoMat(preTransfoMat*m);
	}

	virtual Mat4d getApproximateLinearTransfo() const {return postTransfoMat*preTransfoMat;}

	virtual StelProjector::ModelViewTranformP clone() const {Refraction* refr = new Refraction(); *refr=*this; return StelProjector::ModelViewTranformP(refr);}

	virtual bool setupGLSLTransform(StelGLSLShader* shader)
	{
		Q_UNUSED(shader);
		return false;

		// GL-REFACTOR:
		//
		// I reimplemented the forward() member function in GLSL, but the result is
		// not usable at the moment.
		//
		// On Intel drivers, the projection gets completely messed up.
		// On AMD, most of the time, the coordinates are projected in slightly 
		// different locations (e.g. a planet is slightly above/below where it's
		// supposed to be), and there is very nasty jitter on the individual vertex 
		// positions.
		// NVidia behaves the same, _and_ the viewport borders are messed up.
		//
		//
		// The most likely cause of the problem is the imprecision of GLSL 
		// sin, asin and tan (which AFAIK are implemented through low-resolution 
		// lookup tables in hardware).
		//
		// However, it is also possible that I incorrectly translated forward() to 
		// GLSL.
		//
		//
		// Different possible ways to implement refraction in GLSL would be 
		// to use custom, higher-resolution lookup tables (textures), or to use 
		// a different, maybe simpler (less trig) algorithm for refraction.

		// if(!shader->hasVertexShader("RefractionTransform"))
		// {
		// 	static const QString source(
		// 		"uniform mat4 preTransfoMat;\n"
		// 		"uniform mat4 postTransfoMat;\n"
		// 		"uniform float press_temp_corr_Saemundson;\n"
		// 		"// These values must match the C++ code.\n"
		// 		"const float MIN_GEO_ALTITUDE_DEG = -3.54;\n"
		// 		"const float TRANSITION_WIDTH_GEO_DEG = 1.46;\n"
		// 		"\n"
		// 		"vec4 modelViewForward(in vec4 altAzPos)\n"
		// 		"{\n"
		// 		"    vec4 localAltAzPos = preTransfoMat * altAzPos;\n"
		// 		"    float len = length(localAltAzPos.xyz);\n"
		// 		"    float geom_alt_deg = degrees(asin(localAltAzPos.z / len));\n"
		// 		"    if(geom_alt_deg > MIN_GEO_ALTITUDE_DEG)\n"
		// 		"    {\n"
		// 		"        // refraction from Saemundsson, S&T1986 p70 / in Meeus, Astr.Alg.\n"
		// 		"        float r = press_temp_corr_Saemundson / \n"
		// 		"                  tan(radians(geom_alt_deg + 10.3 / (geom_alt_deg + 5.11))) + 0.0019279;\n"
		// 		"        geom_alt_deg += r;\n"
		// 		"        geom_alt_deg = min(geom_alt_deg, 90.0); // SAFETY\n" 
		// 		"        localAltAzPos.z = sin(radians(geom_alt_deg)) * len;\n"
		// 		"    }\n"
		// 		"    else if(geom_alt_deg > (MIN_GEO_ALTITUDE_DEG - TRANSITION_WIDTH_GEO_DEG))\n"
		// 		"    {\n"
		// 		"        // Avoids the jump below -5 by interpolating linearly between\n"
		// 		"        // MIN_GEO_ALTITUDE_DEG and bottom of transition zone\n"
		// 		"        float r_m5 = press_temp_corr_Saemundson / \n"
		// 		"                     tan(radians(MIN_GEO_ALTITUDE_DEG + 10.3 / (MIN_GEO_ALTITUDE_DEG + 5.11)))\n"
		// 		"                     + 0.0019279;\n"
		// 		"        geom_alt_deg += r_m5 * \n"
		// 		"                        (geom_alt_deg - (MIN_GEO_ALTITUDE_DEG - TRANSITION_WIDTH_GEO_DEG)) /\n"
		// 		"                        TRANSITION_WIDTH_GEO_DEG;\n"
		// 		"        localAltAzPos.z = sin(radians(geom_alt_deg)) * len;\n"
		// 		"    }\n"
		// 		"    return postTransfoMat * localAltAzPos;\n"
		// 		"}\n");

		// 	if(!shader->addVertexShader("RefractionTransform", source))
		// 	{
		// 		return false;
		// 	}
		// 	qDebug() << "Build log after adding a refraction shader: " << shader->log();
		// }
		// shader->enableVertexShader("RefractionTransform");
		// return true;
	}

	virtual void setGLSLUniforms(StelGLSLShader* shader)
	{
		Q_UNUSED(shader);
		// shader->setUniformValue("preTransfoMat", preTransfoMatf);
		// shader->setUniformValue("postTransfoMat", postTransfoMatf);
		// shader->setUniformValue("press_temp_corr_Saemundson", press_temp_corr_Saemundson);
	}

	virtual void disableGLSLTransform(StelGLSLShader* shader)
	{
		Q_UNUSED(shader);
		// shader->disableVertexShader("RefractionTransform");
	}

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

	//! These 3 Atmosphere parameters can be controlled by GUI.
	//! Pressure[mbar] (1013)
	float pressure;
	//! Temperature[Celsius deg] (10).
	float temperature;
	//! Numerator of refraction formula, to be cached for speed.
	float press_temp_corr_Saemundson;
	//! Numerator of refraction formula, to be cached for speed.
	float press_temp_corr_Bennett;

	//! These constants are usable for experiments with the limits of refraction effects.
	static const double MIN_GEO_ALTITUDE_DEG;
	static const double MIN_GEO_ALTITUDE_RAD;
	static const double MIN_GEO_ALTITUDE_SIN;
	static const double MIN_APP_ALTITUDE_DEG;
	static const double MIN_APP_ALTITUDE_RAD;
	static const double MIN_APP_ALTITUDE_SIN;
	static const float MIN_GEO_ALTITUDE_DEG_F;
	static const float MIN_GEO_ALTITUDE_RAD_F;
	static const float MIN_GEO_ALTITUDE_SIN_F;
	static const float MIN_APP_ALTITUDE_DEG_F;
	static const float MIN_APP_ALTITUDE_RAD_F;
	static const float MIN_APP_ALTITUDE_SIN_F;
	static const double TRANSITION_WIDTH_GEO_DEG;
	static const double TRANSITION_WIDTH_GEO_DEG_F;
	static const double TRANSITION_WIDTH_APP_DEG;
	static const double TRANSITION_WIDTH_APP_DEG_F;

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

#endif  // _REFRACTIONEXTINCTION_HPP_

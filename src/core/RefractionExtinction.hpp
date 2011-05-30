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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * Refraction and extinction computations.
 * Implementation: 2010-03-23 GZ=Georg Zotti, Georg.Zotti@univie.ac.at
 */

#ifndef _REFRACTIONEXTINCTION_HPP_
#define _REFRACTIONEXTINCTION_HPP_
// TODO USABILITY: add 4 more flags/switches in GUI:
// (boolean) refraction, [could be omitted and linked to global "show atmosphere"]
// (value field) Temperature [C] [influences refraction]
// (value field) Pressure [mbar]  [influences refraction]
// (combo)   extinction Coeff. k=0...(0.01)...1, [if k=0, no ext. effect]
// SUGGESTION: Could Temperature/Pressure/ex.Coeff./LightPollution be linked to the landscape files?

#include "VecMath.hpp"
#include "StelProjector.hpp"

//! @class RefractionExtinction
//! This class performs refraction and extinction computations, following literature from atmospheric optics and astronomy.
//! Airmass computations are limited to meaningful altitudes,
//! and refraction solutions can only be aproximate, given the turbulent, unpredictable real atmosphere.
//! The solution provided here will [hopefully!] result in a visible "zone of avoidance" near the horizon down to altitude -2,
//! and will show stars in their full brightness at their geometrical positions below -2 degrees.
//! Of course, plotting stars below the horizon could be coupled to setting of horizon foreground,
//! it's another usability issue. Typical horizons do not go down below -1, so -2 should leave a fair amount of room.
//! Note that forward/backward are no absolute reverse operations!
//! All the computations should be in effect
//! (1) only if atmosphere effects are true
//! (2) only for celestial objects, never for landscape coordinates
//! (3) only for terrestrial locations, not on Moon/Mars/Saturn etc
class Extinction
{
public:
	Extinction();
	//! Compute extinction effect for arrays of size @param size position vectors and magnitudes.
	//! @param altAzPos are the normalized (true) star position vectors, and their z components sin(true_altitude).
	//! Note that forward/backward are no absolute reverse operations!
	void forward(Vec3d *altAzPos, float *mag, const int size) const;
	//! Compute extinction effect for arrays of size @param size position vectors and magnitudes.
	//! @param altAzPos are the normalized (apparent) star position vectors, and their z components sin(apparent_altitude).
	//! Note that forward/backward are no absolute reverse operations!
	void backward(Vec3d *altAzPos, float *mag, const int size) const;
	//! Set visual extinction coefficient (mag/airmass), influences extinction computation.
	//! @param k= 0.1 for highest mountains, 0.2 for very good lowland locations, 0.35 for typical lowland, 0.5 in humid climates.
	void setExtinctionCoefficient(float k);
	float getExtinctionCoefficient() const {return ext_coeff;}

private:
	//! airmass computation for @param cosZ = cosine of zenith angle z (=sin(altitude)!).
	//! The default (@param apparent_z = true) is computing airmass from observed altitude, following Rozenberg (1966) [X(90)~40].
	//! if (@param apparent_z = false), we have geometrical altitude and compute airmass from that,
	//! following Young: Air mass and refraction. Applied Optics 33(6), pp.1108-1110, 1994. [X(90)~32].
	//! A problem ist that refraction depends on air pressure and temperature, but Young's formula assumes T=15C, p=1013.25mbar.
	//! So, it seems better to compute refraction first, and then use the Rozenberg formula here.
	//! Rozenberg is infinite at Z=92.17 deg, Young at Z=93.6 deg, so this function RETURNS 0 BELOW -2 DEGREES!
	float airmass(float cosZ, bool apparent_z=true) const;

	//! k, magnitudes/airmass, in [0.00, ... 1.00], (default 0.20).
	float ext_coeff;
};


class Refraction: public StelProjector::ModelViewTranform
{
public:
	Refraction();

	//! Compute refraction.
	//! @param altAzPos is the apparent star position vector
	//! Note that forward/backward are no absolute reverse operations!
	void forward(Vec3d& altAzPos) const;

	//! Compute refraction and extinction effects for arrays of size @param size position vectors and magnitudes.
	//! @param altAzPos is the apparent star position vector
	//! Note that forward/backward are no absolute reverse operations!
	void backward(Vec3d& altAzPos) const;

	//! Compute refraction.
	//! @param altAzPos is the apparent star position vector
	//! Note that forward/backward are no absolute reverse operations!
	void forward(Vec3f& altAzPos) const;

	//! Compute refraction and extinction effects for arrays of size @param size position vectors and magnitudes.
	//! @param altAzPos is the apparent star position vector
	//! Note that forward/backward are no absolute reverse operations!
	void backward(Vec3f& altAzPos) const;

	//! Set surface air pressure (mbars), influences refraction computation.
	void setPressure(float p_mbar);
	float getPressure() const {return pressure;}

	//! Set surface air temperature (degrees Celsius), influences refraction computation.
	void setTemperature(float t_C);
	float getTemperature() const {return temperature;}

	//! Set the transformation matrice used to transform input vector to AltAz frame.
	void setPreTransfoMat(const Mat4d& m);
	void setPostTransfoMat(const Mat4d& m);

private:
	//! Update precomputed variables.
	void updatePrecomputed();

	//! Actually, these 3 Atmosphere parameters should be controlled by GUI.
	//! Pressure[mbar] (1013)
	float pressure;
	//! Temperature[Celsius deg] (10).
	float temperature;
	//! Numerator of refraction formula, to be cached for speed.
	float press_temp_corr_Saemundson;
	//! Numerator of refraction formula, to be cached for speed.
	float press_temp_corr_Bennett;

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

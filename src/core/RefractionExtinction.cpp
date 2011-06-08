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
 * Principal implementation: 2010-03-23 GZ=Georg Zotti, Georg.Zotti@univie.ac.at
 */

#include "RefractionExtinction.hpp"


// To be decided: The following should be either 0 or 40 (or 42? ;-)
const float Extinction::SUBHORIZONTAL_AIRMASS=0.0f;

Extinction::Extinction() : ext_coeff(0.20f)
{
}

//  altAzPos is the normalized star position vector AFTER REFRACTION, and its z component sin(altitude).
void Extinction::forward(const Vec3d *altAzPos, float *mag, const int num) const
{
	for (int i=0; i<num; ++i) mag[i] += airmass(altAzPos[i][2], true) * ext_coeff;
}
void Extinction::forward(const Vec3f *altAzPos, float *mag, const int num) const
{
	for (int i=0; i<num; ++i) mag[i] += airmass(altAzPos[i][2], true) * ext_coeff;
}
// If only sin(altitude) is available:
void Extinction::forward(const double *sinAlt, float *mag, const int num) const
{
	for (int i=0; i<num; ++i) mag[i] += airmass(sinAlt[i], true) * ext_coeff;
}
void Extinction::forward(const float *sinAlt, float *mag, const int num) const
{
	for (int i=0; i<num; ++i) mag[i] += airmass(sinAlt[i], true) * ext_coeff;
}
// from observed magnitude in apparent (observed) altitude to atmosphere-free mag, still in apparent, refracted altitude.
void Extinction::backward(const Vec3d *altAzPos, float *mag, const int num) const
{
	for (int i=0; i<num; ++i) mag[i] -= airmass(altAzPos[i][2], true) * ext_coeff;
}
void Extinction::backward(const Vec3f *altAzPos, float *mag, const int num) const
{
	for (int i=0; i<num; ++i) mag[i] -= airmass(altAzPos[i][2], true) * ext_coeff;
}
// If only sin(altitude) is available:
void Extinction::backward(const double *sinAlt, float *mag, const int num) const
{
	for (int i=0; i<num; ++i) mag[i] -= airmass(sinAlt[i], true) * ext_coeff;
}
void Extinction::backward(const float *sinAlt, float *mag, const int num) const
{
	for (int i=0; i<num; ++i) mag[i] -= airmass(sinAlt[i], true) * ext_coeff;
}

// airmass computation for cosine of zenith angle z
float Extinction::airmass(const float cosZ, const bool apparent_z) const
{
	if (cosZ<-0.035f)
	    return Extinction::SUBHORIZONTAL_AIRMASS; // Safety: 0 or 40 for below -2 degrees.

	float X;
	if (apparent_z)
	{
		// Rozenberg 1966, reported by Schaefer (1993-2000).
		X=1.0f/(cosZ+0.025f*std::exp(-11.f*cosZ));
	}
	else
	{
		//Young 1994
		float nom=(1.002432f*cosZ+0.148386f)*cosZ+0.0096467f;
		float denum=((cosZ+0.149864f)*cosZ+0.0102963f)*cosZ+0.000303978f;
		X=nom/denum;
	}
	return X;
}


// The following 3 are to be configured, the rest is derived.
// Recommendations: -4.9/-4.3/0.1: sharp but continuous transition, no effects below -5.
//                  -4.3/-4.3/0.7: sharp but continuous transition, no effects below -5. Maybe better for picking?
//                  -3/-3/2: "strange" zone 2 degrees wide. Both formulae are close near -3. Actually, refraction formulae dubious below 0
//                   0/0/1: "strange zone 1 degree wide just below horizon, no effect below -1. Actually, refraction formulae dubious below 0! But it looks stupid, see the sun!
// This must be -5 or higher
const double Refraction::MIN_GEO_ALTITUDE_DEG=-3;
// this must be -4.3 or higher
const double Refraction::MIN_APP_ALTITUDE_DEG=-3;
// this must be positive. Transition zone goes that far below the values just specified.
const double Refraction::TRANSITION_WIDTH_DEG=2;
const double Refraction::MIN_GEO_ALTITUDE_RAD=Refraction::MIN_GEO_ALTITUDE_DEG*M_PI/180.0;
const double Refraction::MIN_GEO_ALTITUDE_SIN=std::sin(Refraction::MIN_GEO_ALTITUDE_RAD);
const double Refraction::MIN_APP_ALTITUDE_RAD=Refraction::MIN_APP_ALTITUDE_DEG*M_PI/180.0;
const double Refraction::MIN_APP_ALTITUDE_SIN=std::sin(Refraction::MIN_APP_ALTITUDE_RAD);
const float Refraction::MIN_GEO_ALTITUDE_DEG_F=(float)Refraction::MIN_GEO_ALTITUDE_DEG;
const float Refraction::MIN_GEO_ALTITUDE_RAD_F=(float)Refraction::MIN_GEO_ALTITUDE_RAD;
const float Refraction::MIN_GEO_ALTITUDE_SIN_F=(float)Refraction::MIN_GEO_ALTITUDE_SIN;
const float Refraction::MIN_APP_ALTITUDE_DEG_F=(float)Refraction::MIN_APP_ALTITUDE_DEG;
const float Refraction::MIN_APP_ALTITUDE_RAD_F=(float)Refraction::MIN_APP_ALTITUDE_RAD;
const float Refraction::MIN_APP_ALTITUDE_SIN_F=(float)Refraction::MIN_APP_ALTITUDE_SIN;
const double Refraction::TRANSITION_WIDTH_DEG_F=(float)Refraction::TRANSITION_WIDTH_DEG;

Refraction::Refraction() : pressure(1013.f), temperature(10.f),
	preTransfoMat(Mat4d::identity()), invertPreTransfoMat(Mat4d::identity()), preTransfoMatf(Mat4f::identity()), invertPreTransfoMatf(Mat4f::identity()),
	postTransfoMat(Mat4d::identity()), invertPostTransfoMat(Mat4d::identity()), postTransfoMatf(Mat4f::identity()), invertPostTransfoMatf(Mat4f::identity())
{
	updatePrecomputed();
}

void Refraction::setPreTransfoMat(const Mat4d& m)
{
	preTransfoMat=m;
	invertPreTransfoMat=m.inverse();
	preTransfoMatf.set(m[0], m[1], m[2], m[3], m[4], m[5], m[6], m[7], m[8], m[9], m[10], m[11], m[12], m[13], m[14], m[15]);
	invertPreTransfoMatf.set(invertPreTransfoMat[0], invertPreTransfoMat[1], invertPreTransfoMat[2], invertPreTransfoMat[3],
							 invertPreTransfoMat[4], invertPreTransfoMat[5], invertPreTransfoMat[6], invertPreTransfoMat[7],
							 invertPreTransfoMat[8], invertPreTransfoMat[9], invertPreTransfoMat[10], invertPreTransfoMat[11],
							 invertPreTransfoMat[12], invertPreTransfoMat[13], invertPreTransfoMat[14], invertPreTransfoMat[15]);
}

void Refraction::setPostTransfoMat(const Mat4d& m)
{
	postTransfoMat=m;
	invertPostTransfoMat=m.inverse();
	postTransfoMatf.set(m[0], m[1], m[2], m[3], m[4], m[5], m[6], m[7], m[8], m[9], m[10], m[11], m[12], m[13], m[14], m[15]);
	invertPostTransfoMatf.set(invertPostTransfoMat[0], invertPostTransfoMat[1], invertPostTransfoMat[2], invertPostTransfoMat[3],
							 invertPostTransfoMat[4], invertPostTransfoMat[5], invertPostTransfoMat[6], invertPostTransfoMat[7],
							 invertPostTransfoMat[8], invertPostTransfoMat[9], invertPostTransfoMat[10], invertPostTransfoMat[11],
							 invertPostTransfoMat[12], invertPostTransfoMat[13], invertPostTransfoMat[14], invertPostTransfoMat[15]);
}

void Refraction::updatePrecomputed()
{
	press_temp_corr_Bennett=pressure/1010.f * 283.f/(273.f+temperature) / 60.f;
	press_temp_corr_Saemundson=1.02f*press_temp_corr_Bennett;
}

void Refraction::forward(Vec3d& altAzPos) const
{
	altAzPos.transfo4d(preTransfoMat);
	const double length = altAzPos.length();

	double geom_alt_deg=180./M_PI*std::asin(altAzPos[2]/length);
	if (geom_alt_deg > Refraction::MIN_GEO_ALTITUDE_DEG)
	{
		// refraction from Saemundsson, S&T1986 p70 / in Meeus, Astr.Alg.
		float r=press_temp_corr_Saemundson / std::tan((geom_alt_deg+10.3f/(geom_alt_deg+5.11f))*M_PI/180.f) + 0.0019279f;
		geom_alt_deg += r;
		if (geom_alt_deg > 90.)
			geom_alt_deg=90.; // SAFETY, SHOULD NOT BE NECESSARY
		altAzPos[2]=std::sin(geom_alt_deg*M_PI/180.)*length;
	}
	else if(geom_alt_deg>Refraction::MIN_GEO_ALTITUDE_DEG-Refraction::TRANSITION_WIDTH_DEG)
	{
		// Avoids the jump near -5 by interpolating linearly between MIN_GEO_ALTITUDE_DEG and bottom of transition zone
		float r_min=press_temp_corr_Saemundson / std::tan((Refraction::MIN_GEO_ALTITUDE_DEG+10.3f/(Refraction::MIN_GEO_ALTITUDE_DEG+5.11f))*M_PI/180.f) + 0.0019279f;
		geom_alt_deg += r_min*(geom_alt_deg-(Refraction::MIN_GEO_ALTITUDE_DEG-Refraction::TRANSITION_WIDTH_DEG))/Refraction::TRANSITION_WIDTH_DEG;
		altAzPos[2]=std::sin(geom_alt_deg*M_PI/180.)*length;
	}
	altAzPos.transfo4d(postTransfoMat);
}

void Refraction::backward(Vec3d& altAzPos) const
{
	altAzPos.transfo4d(invertPostTransfoMat);
	// going from apparent (observed) position to geometrical position.
	const double length = altAzPos.length();
	double obs_alt_deg=180./M_PI*std::asin(altAzPos[2]/length);
	if (obs_alt_deg > Refraction::MIN_APP_ALTITUDE_DEG)
	{
		// refraction from Bennett, in Meeus, Astr.Alg.
		float r=press_temp_corr_Bennett / std::tan((obs_alt_deg+7.31/(obs_alt_deg+4.4f))*M_PI/180.f) + 0.0013515f;
		if (r>0.5) r=0.5*(r-0.5)+0.5; // reduce silly lobe.
		obs_alt_deg -= r;
		altAzPos[2]=std::sin(obs_alt_deg*M_PI/180.)*length;
	}
	else if (obs_alt_deg > Refraction::MIN_APP_ALTITUDE_DEG-Refraction::TRANSITION_WIDTH_DEG)
	{
		// refraction from Bennett, in Meeus, Astr.Alg.
		float r_min=press_temp_corr_Bennett / std::tan((Refraction::MIN_APP_ALTITUDE_DEG+7.31/(Refraction::MIN_APP_ALTITUDE_DEG+4.4f))*M_PI/180.f) + 0.0013515f;
		if (r_min>0.5) r_min=0.5*(r_min-0.5)+0.5; // reduce silly lobe.
		obs_alt_deg -= r_min*(obs_alt_deg-(Refraction::MIN_APP_ALTITUDE_DEG-Refraction::TRANSITION_WIDTH_DEG))/Refraction::TRANSITION_WIDTH_DEG;
		altAzPos[2]=std::sin(obs_alt_deg*M_PI/180.)*length;
	}
	altAzPos.transfo4d(invertPreTransfoMat);
}

void Refraction::forward(Vec3f& altAzPos) const
{
	altAzPos.transfo4d(preTransfoMatf);
	const float length = altAzPos.length();
	float geom_alt_deg=180.f/M_PI*std::asin(altAzPos[2]/length);
	if (geom_alt_deg > Refraction::MIN_GEO_ALTITUDE_DEG_F)
	{
		// refraction from Saemundsson, S&T1986 p70 / in Meeus, Astr.Alg.
		float r=press_temp_corr_Saemundson / std::tan((geom_alt_deg+10.3f/(geom_alt_deg+5.11f))*M_PI/180.f) + 0.0019279f;
		geom_alt_deg += r;
		if (geom_alt_deg > 90.f)
			geom_alt_deg=90.f; // SAFETY, SHOULD NOT BE NECESSARY
		altAzPos[2]=std::sin(geom_alt_deg*M_PI/180.f)*length;
	}
	else if(geom_alt_deg>Refraction::MIN_GEO_ALTITUDE_DEG_F-Refraction::TRANSITION_WIDTH_DEG_F)
	{
		// Avoids the jump below -5 by interpolating linearly between MIN_GEO_ALTITUDE_DEG_F and bottom of transition zone
		float r_m5=press_temp_corr_Saemundson / std::tan((Refraction::MIN_GEO_ALTITUDE_DEG_F+10.3f/(Refraction::MIN_GEO_ALTITUDE_DEG_F+5.11f))*M_PI/180.f) + 0.0019279f;
		geom_alt_deg += r_m5*(geom_alt_deg-(Refraction::MIN_GEO_ALTITUDE_DEG_F-Refraction::TRANSITION_WIDTH_DEG_F))/Refraction::TRANSITION_WIDTH_DEG_F;
		altAzPos[2]=std::sin(geom_alt_deg*M_PI/180.)*length;
	}
	altAzPos.transfo4d(postTransfoMatf);
}

void Refraction::backward(Vec3f& altAzPos) const
{
	altAzPos.transfo4d(invertPostTransfoMatf);
	// going from observed position/magnitude to geometrical position and atmosphere-free mag.
	const float length = altAzPos.length();
	float obs_alt_deg=180.f/M_PI*std::asin(altAzPos[2]/length);
	if (obs_alt_deg > Refraction::MIN_APP_ALTITUDE_DEG_F)
	{
		// refraction from Bennett, in Meeus, Astr.Alg.
		float r=press_temp_corr_Bennett / std::tan((obs_alt_deg+7.31f/(obs_alt_deg+4.4f))*M_PI/180.f) + 0.0013515f;
		if (r>0.5f) r=0.5f*(r-0.5f)+0.5f; // reduce silly lobe.
		obs_alt_deg -= r;
		altAzPos[2]=std::sin(obs_alt_deg*M_PI/180.f)*length;
	}
	else if (obs_alt_deg > Refraction::MIN_APP_ALTITUDE_DEG_F-Refraction::TRANSITION_WIDTH_DEG_F)
	{
		// refraction from Bennett, in Meeus, Astr.Alg.
		float r_min=press_temp_corr_Bennett / std::tan((Refraction::MIN_APP_ALTITUDE_DEG_F+7.31f/(Refraction::MIN_APP_ALTITUDE_DEG_F+4.4f))*M_PI/180.f) + 0.0013515f;
		if (r_min>0.5f) r_min=0.5f*(r_min-0.5f)+0.5f; // reduce silly lobe.
		obs_alt_deg -= r_min*(obs_alt_deg-(Refraction::MIN_APP_ALTITUDE_DEG_F-Refraction::TRANSITION_WIDTH_DEG_F))/Refraction::TRANSITION_WIDTH_DEG_F;
		altAzPos[2]=std::sin(obs_alt_deg*M_PI/180.f)*length;
	}
	altAzPos.transfo4d(invertPreTransfoMatf);
}

void Refraction::setPressure(float p)
{
	pressure=p;
	updatePrecomputed();
}

void Refraction::setTemperature(float t)
{
	temperature=t;
	updatePrecomputed();
}

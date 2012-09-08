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
 * Principal implementation: 2010-03-23 GZ=Georg Zotti, Georg.Zotti@univie.ac.at
 */

#include <qsettings.h>
#include "StelApp.hpp"
#include "RefractionExtinction.hpp"


// To be decided: The following should be either 0 or 40 (or 42? ;-)
float Extinction::SUBHORIZONTAL_AIRMASS=0.0f;

Extinction::Extinction()
{
    QSettings* conf = StelApp::getInstance().getSettings();
    SUBHORIZONTAL_AIRMASS = (conf->value("astro/flag_extinction_below_horizon", true).toBool()? 42.0f : 0.0f);
    ext_coeff=conf->value("landscape/atmospheric_extinction_coefficient", 0.2f).toFloat();
}

//  altAzPos is the NORMALIZED (!!!) star position vector AFTER REFRACTION, and its z component sin(altitude).
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
void Extinction::forward(const double *sinAlt, float *mag) const
{
	*mag += airmass(*sinAlt, true) * ext_coeff;
}
void Extinction::forward(const float *sinAlt, float *mag) const
{
	*mag += airmass(*sinAlt, true) * ext_coeff;
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
	if (cosZ<-0.035f) // about -2 degrees. Here, RozenbergZ>574 and climbs fast!
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

/* ***************************************************************************************************** */

// The following 4 are to be configured, the rest is derived.
// Recommendations: -4.9/-4.3/0.1/0.1: sharp but continuous transition, no effects below -5.
//                  -4.3/-4.3/0.7/0.7: sharp but continuous transition, no effects below -5. Maybe better for picking?
//                  -3/-3/2/2: "strange" zone 2 degrees wide. Both formulae are close near -3. Actually, refraction formulae dubious below 0
//                   0/0/1/1: "strange zone 1 degree wide just below horizon, no effect below -1. Actually, refraction formulae dubious below 0! But it looks stupid, see the sun!
//                  Optimum:-3.54/-3.21783/1.46/1.78217. Here forward/backward are almost perfect inverses (-->good picking!), and nothing happens below -5 degrees.
// This must be -5 or higher.
const double Refraction::MIN_GEO_ALTITUDE_DEG=-3.54;
// this must be -4.3 or higher. -3.21783 is an optimal value to fit against forward refraction!
const double Refraction::MIN_APP_ALTITUDE_DEG=-3.21783;
// this must be positive. Transition zone goes that far below the values just specified.
const double Refraction::TRANSITION_WIDTH_GEO_DEG=1.46;
const double Refraction::TRANSITION_WIDTH_APP_DEG=1.78217;
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
const double Refraction::TRANSITION_WIDTH_GEO_DEG_F=(float)Refraction::TRANSITION_WIDTH_GEO_DEG;
const double Refraction::TRANSITION_WIDTH_APP_DEG_F=(float)Refraction::TRANSITION_WIDTH_APP_DEG;

Refraction::Refraction() : //pressure(1013.f), temperature(10.f),
	preTransfoMat(Mat4d::identity()), invertPreTransfoMat(Mat4d::identity()), preTransfoMatf(Mat4f::identity()), invertPreTransfoMatf(Mat4f::identity()),
	postTransfoMat(Mat4d::identity()), invertPostTransfoMat(Mat4d::identity()), postTransfoMatf(Mat4f::identity()), invertPostTransfoMatf(Mat4f::identity())
{
  QSettings* conf = StelApp::getInstance().getSettings();
  pressure=conf->value("landscape/pressure_mbar", 1013.0f).toFloat();
  temperature=conf->value("landscape/temperature_C", 15.0f).toFloat();
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
		if (geom_alt_deg > 90.) geom_alt_deg=90.; // SAFETY
		altAzPos[2]=std::sin(geom_alt_deg*M_PI/180.)*length;
	}
	else if(geom_alt_deg>Refraction::MIN_GEO_ALTITUDE_DEG-Refraction::TRANSITION_WIDTH_GEO_DEG)
	{
		// Avoids the jump near -5 by interpolating linearly between MIN_GEO_ALTITUDE_DEG and bottom of transition zone
		float r_min=press_temp_corr_Saemundson / std::tan((Refraction::MIN_GEO_ALTITUDE_DEG+10.3f/(Refraction::MIN_GEO_ALTITUDE_DEG+5.11f))*M_PI/180.f) + 0.0019279f;
		geom_alt_deg += r_min*(geom_alt_deg-(Refraction::MIN_GEO_ALTITUDE_DEG-Refraction::TRANSITION_WIDTH_GEO_DEG))/Refraction::TRANSITION_WIDTH_GEO_DEG;
		altAzPos[2]=std::sin(geom_alt_deg*M_PI/180.)*length;
	}
	altAzPos.transfo4d(postTransfoMat);
}

//Bennett's formula is not a strict inverse of Saemundsson's. There is a notable discrepancy (alt!=backward(forward(alt))) for
// geometric altitudes <-.3deg.  This is not a problem in real life, but if a user switches off landscape, click-identify may pose a problem.
// Below this altitude, we therefore use a polynomial fit that should represent a close inverse of Saemundsson.
void Refraction::backward(Vec3d& altAzPos) const
{
	altAzPos.transfo4d(invertPostTransfoMat);
	// going from apparent (observed) position to geometrical position.
	const double length = altAzPos.length();
	double obs_alt_deg=180./M_PI*std::asin(altAzPos[2]/length);
	if (obs_alt_deg > 0.22879)
	{
		// refraction directly from Bennett, in Meeus, Astr.Alg.
		float r=press_temp_corr_Bennett / std::tan((obs_alt_deg+7.31/(obs_alt_deg+4.4f))*M_PI/180.f) + 0.0013515f;
		obs_alt_deg -= r;
		altAzPos[2]=std::sin(obs_alt_deg*M_PI/180.)*length;
	}
	else if (obs_alt_deg > Refraction::MIN_APP_ALTITUDE_DEG)
	{
		// backward refraction from polynomial fit against Saemundson[-5...-0.3]
		float r=(((((0.0444*obs_alt_deg+.7662)*obs_alt_deg+4.9746)*obs_alt_deg+13.599)*obs_alt_deg+8.052)*obs_alt_deg-11.308)*obs_alt_deg+34.341;
		obs_alt_deg -= press_temp_corr_Bennett*r;
		altAzPos[2]=std::sin(obs_alt_deg*M_PI/180.)*length;
	}
	else if (obs_alt_deg > Refraction::MIN_APP_ALTITUDE_DEG-Refraction::TRANSITION_WIDTH_APP_DEG)
	{
		// use polynomial fit for topmost value, linear interpolation inside transition zone
		float r_min=(((((0.0444*Refraction::MIN_APP_ALTITUDE_DEG+.7662)*Refraction::MIN_APP_ALTITUDE_DEG
				+4.9746)*Refraction::MIN_APP_ALTITUDE_DEG+13.599)*Refraction::MIN_APP_ALTITUDE_DEG
			      +8.052)*Refraction::MIN_APP_ALTITUDE_DEG-11.308)*Refraction::MIN_APP_ALTITUDE_DEG+34.341;
		r_min*=press_temp_corr_Bennett;
		obs_alt_deg -= r_min*(obs_alt_deg-(Refraction::MIN_APP_ALTITUDE_DEG-Refraction::TRANSITION_WIDTH_APP_DEG))/Refraction::TRANSITION_WIDTH_APP_DEG;
		altAzPos[2]=std::sin(obs_alt_deg*M_PI/180.)*length;
	}
	altAzPos.transfo4d(invertPreTransfoMat);
}

void Refraction::forward(Vec3f& altAzPos) const
{
	// Using doubles internally to avoid jitter.
	// (This affects planet drawing - which is done using floats, 
	// as doubles are unsupported/slow on GPUs)
	Vec3d altAzPosD(altAzPos[0], altAzPos[1], altAzPos[2]);
	altAzPosD.transfo4d(preTransfoMat);
	const double length = altAzPosD.length();
	double geom_alt_deg=180./M_PI*std::asin(altAzPosD[2]/length);
	if (geom_alt_deg > Refraction::MIN_GEO_ALTITUDE_DEG)
	{
		// refraction from Saemundsson, S&T1986 p70 / in Meeus, Astr.Alg.
		double r=press_temp_corr_Saemundson / 
		        std::tan((geom_alt_deg+10.3/(geom_alt_deg+5.11))*M_PI/180.) + 0.0019279;
		geom_alt_deg += r;
		if (geom_alt_deg > 90.) geom_alt_deg=90.; // SAFETY
		altAzPosD[2]=std::sin(geom_alt_deg*M_PI/180.)*length;
	}
	else if(geom_alt_deg>Refraction::MIN_GEO_ALTITUDE_DEG-Refraction::TRANSITION_WIDTH_GEO_DEG)
	{
		// Avoids the jump below -5 by interpolating linearly between MIN_GEO_ALTITUDE_DEG and bottom of transition zone
		double r_m5=press_temp_corr_Saemundson /
		          std::tan((Refraction::MIN_GEO_ALTITUDE_DEG+10.3/
		                    (Refraction::MIN_GEO_ALTITUDE_DEG+5.11))*M_PI/180.) + 0.0019279;
		geom_alt_deg += 
			r_m5*(geom_alt_deg-(Refraction::MIN_GEO_ALTITUDE_DEG-
			                    Refraction::TRANSITION_WIDTH_GEO_DEG))
			/Refraction::TRANSITION_WIDTH_GEO_DEG;
		altAzPosD[2]=std::sin(geom_alt_deg*M_PI/180.)*length;
	}
	altAzPosD.transfo4d(postTransfoMat);

	altAzPos = Vec3f(altAzPosD[0], altAzPosD[1], altAzPosD[2]);
}

void Refraction::backward(Vec3f& altAzPos) const
{
	altAzPos.transfo4d(invertPostTransfoMatf);
	// going from observed position/magnitude to geometrical position and atmosphere-free mag.
	const float length = altAzPos.length();
	float obs_alt_deg=180.f/M_PI*std::asin(altAzPos[2]/length);
	if (obs_alt_deg > 0.22879f)
	{
		// refraction from Bennett, in Meeus, Astr.Alg.
		float r=press_temp_corr_Bennett / std::tan((obs_alt_deg+7.31f/(obs_alt_deg+4.4f))*M_PI/180.f) + 0.0013515f;
		obs_alt_deg -= r;
		altAzPos[2]=std::sin(obs_alt_deg*M_PI/180.f)*length;
	}
	else if (obs_alt_deg > Refraction::MIN_APP_ALTITUDE_DEG_F)
	{
		// backward refraction from polynomial fit against Saemundson[-5...-0.3]
		float r=(((((0.0444f*obs_alt_deg+.7662f)*obs_alt_deg+4.9746f)*obs_alt_deg+13.599f)*obs_alt_deg+8.052f)*obs_alt_deg-11.308f)*obs_alt_deg+34.341f;
		obs_alt_deg -= press_temp_corr_Bennett*r;
		altAzPos[2]=std::sin(obs_alt_deg*M_PI/180.f)*length;
	}
	else if (obs_alt_deg > Refraction::MIN_APP_ALTITUDE_DEG_F-Refraction::TRANSITION_WIDTH_APP_DEG_F)
	{
		// Compute top value from polynome, apply linear interpolation
		float r_min=(((((0.0444f*Refraction::MIN_APP_ALTITUDE_DEG_F+.7662f)*Refraction::MIN_APP_ALTITUDE_DEG_F
				+4.9746f)*Refraction::MIN_APP_ALTITUDE_DEG_F+13.599f)*Refraction::MIN_APP_ALTITUDE_DEG_F
			      +8.052f)*Refraction::MIN_APP_ALTITUDE_DEG_F-11.308f)*Refraction::MIN_APP_ALTITUDE_DEG_F+34.341f;

		r_min*=press_temp_corr_Bennett;
		obs_alt_deg -= r_min*(obs_alt_deg-(Refraction::MIN_APP_ALTITUDE_DEG_F-Refraction::TRANSITION_WIDTH_APP_DEG_F))/Refraction::TRANSITION_WIDTH_APP_DEG_F;
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

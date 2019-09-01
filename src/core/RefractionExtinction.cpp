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

#include "StelApp.hpp"
#include "StelUtils.hpp"
#include "RefractionExtinction.hpp"
#include "StelUtils.hpp"

Extinction::Extinction() : ext_coeff(50), undergroundExtinctionMode(UndergroundExtinctionMirror)
{
}

// airmass computation for cosine of zenith angle z
float Extinction::airmass(float cosZ, const bool apparent_z) const
{
	if (cosZ<-0.035f) // about -2 degrees. Here, RozenbergZ>574 and climbs fast!
	{
		switch (undergroundExtinctionMode)
		{
			case UndergroundExtinctionZero:
				return 0.f;
			case UndergroundExtinctionMax:
				return 42.f;
			case UndergroundExtinctionMirror:
				cosZ = std::min(1.f, -0.035f - (cosZ+0.035f));
		}
	}

	if (apparent_z)
	{
		// Rozenberg 1966, reported by Schaefer (1993-2000).
		return 1.0f/(cosZ+0.025f*std::exp(-11.f*cosZ));
	}
	else
	{
		//Young 1994
		const float nom=(1.002432f*cosZ+0.148386f)*cosZ+0.0096467f;
		const float denum=((cosZ+0.149864f)*cosZ+0.0102963f)*cosZ+0.000303978f;
		return nom/denum;
	}
}

/* ***************************************************************************************************** */

// The following 4 are to be configured, the rest is derived.
// Recommendations: -4.9/-4.3/0.1/0.1: sharp but continuous transition, no effects below -5.
//                  -4.3/-4.3/0.7/0.7: sharp but continuous transition, no effects below -5. Maybe better for picking?
//                  -3/-3/2/2: "strange" zone 2 degrees wide. Both formulae are close near -3. Actually, refraction formulae dubious below 0
//                   0/0/1/1: "strange zone 1 degree wide just below horizon, no effect below -1. Actually, refraction formulae dubious below 0! But it looks stupid, see the sun!
//                  Optimum:-3.54/-3.21783/1.46/1.78217. Here forward/backward are almost perfect inverses (-->good picking!), and nothing happens below -5 degrees.
// This must be -5 or higher.
static const float MIN_GEO_ALTITUDE_DEG=-3.54f;
// this must be -4.3 or higher. -3.21783 is an optimal value to fit against forward refraction!
static const float MIN_APP_ALTITUDE_DEG=-3.21783f;
// this must be positive. Transition zone goes that far below the values just specified.
static const float TRANSITION_WIDTH_GEO_DEG=1.46f;
static const float TRANSITION_WIDTH_APP_DEG=1.78217f;

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
	preTransfoMatf.set(static_cast<float>(m[0]),  static_cast<float>(m[1]),  static_cast<float>(m[2]),  static_cast<float>(m[3]),
			   static_cast<float>(m[4]),  static_cast<float>(m[5]),  static_cast<float>(m[6]),  static_cast<float>(m[7]),
			   static_cast<float>(m[8]),  static_cast<float>(m[9]),  static_cast<float>(m[10]), static_cast<float>(m[11]),
			   static_cast<float>(m[12]), static_cast<float>(m[13]), static_cast<float>(m[14]), static_cast<float>(m[15]));
	invertPreTransfoMatf.set(static_cast<float>(invertPreTransfoMat[0]),  static_cast<float>(invertPreTransfoMat[1]),  static_cast<float>(invertPreTransfoMat[2]),  static_cast<float>(invertPreTransfoMat[3]),
				 static_cast<float>(invertPreTransfoMat[4]),  static_cast<float>(invertPreTransfoMat[5]),  static_cast<float>(invertPreTransfoMat[6]),  static_cast<float>(invertPreTransfoMat[7]),
				 static_cast<float>(invertPreTransfoMat[8]),  static_cast<float>(invertPreTransfoMat[9]),  static_cast<float>(invertPreTransfoMat[10]), static_cast<float>(invertPreTransfoMat[11]),
				 static_cast<float>(invertPreTransfoMat[12]), static_cast<float>(invertPreTransfoMat[13]), static_cast<float>(invertPreTransfoMat[14]), static_cast<float>(invertPreTransfoMat[15]));
}

void Refraction::setPostTransfoMat(const Mat4d& m)
{
	postTransfoMat=m;
	invertPostTransfoMat=m.inverse();
	postTransfoMatf.set(static_cast<float>(m[0]),  static_cast<float>(m[1]),  static_cast<float>(m[2]),  static_cast<float>(m[3]),
			    static_cast<float>(m[4]),  static_cast<float>(m[5]),  static_cast<float>(m[6]),  static_cast<float>(m[7]),
			    static_cast<float>(m[8]),  static_cast<float>(m[9]),  static_cast<float>(m[10]), static_cast<float>(m[11]),
			    static_cast<float>(m[12]), static_cast<float>(m[13]), static_cast<float>(m[14]), static_cast<float>(m[15]));
	invertPostTransfoMatf.set(static_cast<float>(invertPostTransfoMat[0]),  static_cast<float>(invertPostTransfoMat[1]),  static_cast<float>(invertPostTransfoMat[2]),  static_cast<float>(invertPostTransfoMat[3]),
				  static_cast<float>(invertPostTransfoMat[4]),  static_cast<float>(invertPostTransfoMat[5]),  static_cast<float>(invertPostTransfoMat[6]),  static_cast<float>(invertPostTransfoMat[7]),
				  static_cast<float>(invertPostTransfoMat[8]),  static_cast<float>(invertPostTransfoMat[9]),  static_cast<float>(invertPostTransfoMat[10]), static_cast<float>(invertPostTransfoMat[11]),
				  static_cast<float>(invertPostTransfoMat[12]), static_cast<float>(invertPostTransfoMat[13]), static_cast<float>(invertPostTransfoMat[14]), static_cast<float>(invertPostTransfoMat[15]));
}

void Refraction::updatePrecomputed()
{
	press_temp_corr=pressure/1010.f * 283.f/(273.f+temperature) / 60.f;
}

void Refraction::innerRefractionForward(Vec3d& altAzPos) const
{
	const double length = altAzPos.length();
	if (length==0.0)
	{
		// Under some circumstances there are zero coordinates. Just leave them alone.
		//qDebug() << "Refraction::innerRefractionForward(): Zero vector detected - Continue with zero vector.";
		return;
	}

	Q_ASSERT(length>0.0);
	const double sinGeo = altAzPos[2]/length;
	Q_ASSERT(fabs(sinGeo)<=1.0);
	double geom_alt_rad = std::asin(sinGeo);
	float geom_alt_deg = M_180_PIf*static_cast<float>(geom_alt_rad);
	if (geom_alt_deg > MIN_GEO_ALTITUDE_DEG)
	{
		// refraction from Saemundsson, S&T1986 p70 / in Meeus, Astr.Alg.
		float r=press_temp_corr * ( 1.02f / std::tan((geom_alt_deg+10.3f/(geom_alt_deg+5.11f))*M_PI_180f) + 0.0019279f);
		geom_alt_deg += r;
		if (geom_alt_deg > 90.f)
			geom_alt_deg=90.f;
	}
	else if(geom_alt_deg>MIN_GEO_ALTITUDE_DEG-TRANSITION_WIDTH_GEO_DEG)
	{
		// Avoids the jump below -5 by interpolating linearly between MIN_GEO_ALTITUDE_DEG and bottom of transition zone
		float r_m5=press_temp_corr * ( 1.02f / std::tan((MIN_GEO_ALTITUDE_DEG+10.3f/(MIN_GEO_ALTITUDE_DEG+5.11f))*M_PI_180f) + 0.0019279f);
		geom_alt_deg += r_m5*(geom_alt_deg-(MIN_GEO_ALTITUDE_DEG-TRANSITION_WIDTH_GEO_DEG))/TRANSITION_WIDTH_GEO_DEG;
	}
	else return;
	// At this point we have corrected geometric altitude. Note that if we just change altAzPos[2], we would change vector length, so this would change our angles.
	// We have to shorten X,Y components of the vector as well by the change in cosines of altitude, or (sqrt(1-sin(alt))

	const double refr_alt_rad=static_cast<double>(geom_alt_deg)*M_PI_180;
	const double sinRef=std::sin(refr_alt_rad);

	const double shortenxy=((fabs(sinGeo)>=1.0) ? 1.0 :
			std::sqrt((1.-sinRef*sinRef)/(1.-sinGeo*sinGeo))); // we need double's mantissa length here, sorry!

	altAzPos[0]*=shortenxy;
	altAzPos[1]*=shortenxy;
	altAzPos[2]=sinRef*length;
}

// going from observed position to geometrical position.
void Refraction::innerRefractionBackward(Vec3d& altAzPos) const
{
	const double length = altAzPos.length();
	if (length==0.0)
	{
		// Under some circumstances there are zero coordinates. Just leave them alone.
		//qDebug() << "Refraction::innerRefractionBackward(): Zero vector detected - Continue with zero vector.";
		return;
	}
	Q_ASSERT(length>0.0);
	const double sinObs = altAzPos[2]/length;
	Q_ASSERT(fabs(sinObs)<=1.0);
	float obs_alt_deg=static_cast<float>(M_180_PI*std::asin(sinObs));
	if (obs_alt_deg > 0.22879f)
	{
		// refraction from Bennett, in Meeus, Astr.Alg.
		float r=press_temp_corr * (1.f / std::tan((obs_alt_deg+7.31f/(obs_alt_deg+4.4f))*M_PI_180f) + 0.0013515f);
		obs_alt_deg -= r;
	}
	else if (obs_alt_deg > MIN_APP_ALTITUDE_DEG)
	{
		// backward refraction from polynomial fit against Saemundson[-5...-0.3]
		float r=(((((0.0444f*obs_alt_deg+.7662f)*obs_alt_deg+4.9746f)*obs_alt_deg+13.599f)*obs_alt_deg+8.052f)*obs_alt_deg-11.308f)*obs_alt_deg+34.341f;
		obs_alt_deg -= press_temp_corr*r;
	}
	else if (obs_alt_deg > MIN_APP_ALTITUDE_DEG-TRANSITION_WIDTH_APP_DEG)
	{
		// Compute top value from polynome, apply linear interpolation
		static const float r_min=(((((0.0444f*MIN_APP_ALTITUDE_DEG+.7662f)*MIN_APP_ALTITUDE_DEG
				+4.9746f)*MIN_APP_ALTITUDE_DEG+13.599f)*MIN_APP_ALTITUDE_DEG
			      +8.052f)*MIN_APP_ALTITUDE_DEG-11.308f)*MIN_APP_ALTITUDE_DEG+34.341f;

		obs_alt_deg -= r_min*press_temp_corr*(obs_alt_deg-(MIN_APP_ALTITUDE_DEG-TRANSITION_WIDTH_APP_DEG))/TRANSITION_WIDTH_APP_DEG;
	}
	else return;
	// At this point we have corrected observed altitude. Note that if we just change altAzPos[2], we would change vector length, so this would change our angles.
	// We have to make X,Y components of the vector a bit longer as well by the change in cosines of altitude, or (sqrt(1-sin(alt))

	const double geo_alt_rad=static_cast<double>(obs_alt_deg)*M_PI_180;
	const double sinGeo=std::sin(geo_alt_rad);
	const double longerxy=((fabs(sinObs)>=1.0) ? 1.0 :
			std::sqrt((1.-sinGeo*sinGeo)/(1.-sinObs*sinObs)));
	altAzPos[0]*=longerxy;
	altAzPos[1]*=longerxy;
	altAzPos[2]=sinGeo*length;
}

void Refraction::forward(Vec3d& altAzPos) const
{
	altAzPos.transfo4d(preTransfoMat);
	innerRefractionForward(altAzPos);
	altAzPos.transfo4d(postTransfoMat);
}

//Bennett's formula is not a strict inverse of Saemundsson's. There is a notable discrepancy (alt!=backward(forward(alt))) for
// geometric altitudes <-.3deg.  This is not a problem in real life, but if a user switches off landscape, click-identify may pose a problem.
// Below this altitude, we therefore use a polynomial fit that should represent a close inverse of Saemundsson.
void Refraction::backward(Vec3d& altAzPos) const
{
	altAzPos.transfo4d(invertPostTransfoMat);
	innerRefractionBackward(altAzPos);
	altAzPos.transfo4d(invertPreTransfoMat);
}

void Refraction::forward(Vec3f& altAzPos) const
{
	Vec3d vf=altAzPos.toVec3d();
	vf.transfo4d(preTransfoMat);
	innerRefractionForward(vf);
	vf.transfo4d(postTransfoMat);
	altAzPos.set(static_cast<float>(vf[0]), static_cast<float>(vf[1]), static_cast<float>(vf[2]));
}

void Refraction::backward(Vec3f& altAzPos) const
{
	altAzPos.transfo4d(invertPostTransfoMatf);
	Vec3d vf=altAzPos.toVec3d();
	innerRefractionBackward(vf);
	altAzPos.set(static_cast<float>(vf[0]), static_cast<float>(vf[1]), static_cast<float>(vf[2]));
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

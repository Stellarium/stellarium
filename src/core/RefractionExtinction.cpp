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

#include <QOpenGLShaderProgram>

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

QByteArray Extinction::getForwardTransformShader() const
{
	return QByteArray(1+R"(
uniform int EXTINCTION_undergroundExtinctionMode;
float EXTINCTION_airmass(float cosZ, bool apparent_z)
{
	if(cosZ<-0.035) // about -2 degrees. Here, RozenbergZ>574 and climbs fast!
	{
		if(EXTINCTION_undergroundExtinctionMode == UndergroundExtinctionZero)
			return 0.;
		else if(EXTINCTION_undergroundExtinctionMode == UndergroundExtinctionMax)
			return 42.;
		else if(EXTINCTION_undergroundExtinctionMode == UndergroundExtinctionMirror)
			cosZ = min(1., -0.035 - (cosZ+0.035));
	}

	if(apparent_z)
	{
		// Rozenberg 1966, reported by Schaefer (1993-2000).
		return 1.0/(cosZ+0.025*exp(-11.*cosZ));
	}
	else
	{
		//Young 1994
		float nom=(1.002432*cosZ+0.148386)*cosZ+0.0096467;
		float denum=((cosZ+0.149864)*cosZ+0.0102963)*cosZ+0.000303978;
		return nom/denum;
	}
}

uniform float EXTINCTION_ext_coeff;
float extinctionMagnitude(vec3 altAzPos)
{
	return EXTINCTION_airmass(altAzPos[2], false) * EXTINCTION_ext_coeff;
}
)").replace("UndergroundExtinctionZero", std::to_string(int(UndergroundExtinctionZero)).c_str())
   .replace("UndergroundExtinctionMax", std::to_string(int(UndergroundExtinctionMax)).c_str())
   .replace("UndergroundExtinctionMirror", std::to_string(int(UndergroundExtinctionMirror)).c_str());
}

void Extinction::setForwardTransformUniforms(QOpenGLShaderProgram& program) const
{
	program.setUniformValue("EXTINCTION_undergroundExtinctionMode", int(undergroundExtinctionMode));
	program.setUniformValue("EXTINCTION_ext_coeff", GLfloat(ext_coeff));
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
	preTransfoMatf=toMat4f(m);
	invertPreTransfoMatf=toMat4f(invertPreTransfoMat);
}

void Refraction::setPostTransfoMat(const Mat4d& m)
{
	postTransfoMat=m;
	invertPostTransfoMat=m.inverse();
	postTransfoMatf=toMat4f(m);
	invertPostTransfoMatf=toMat4f(invertPostTransfoMat);
}

void Refraction::updatePrecomputed()
{
	press_temp_corr=pressure/1010.f * 283.f/(273.f+temperature) / 60.f;
}

void Refraction::innerRefractionForward(Vec3d& altAzPos) const
{
	const double length = altAzPos.norm();
	if (length==0.0)
	{
		// Under some circumstances there are zero coordinates. Just leave them alone.
		//qDebug() << "Refraction::innerRefractionForward(): Zero vector detected - Continue with zero vector.";
		return;
	}

	// NOTE: the calculations here *must* be in double, otherwise we'll get wobble of "small" objects like Callisto or Thebe
	Q_ASSERT(length>0.0);
	const double sinGeo = altAzPos[2]/length;
	Q_ASSERT(fabs(sinGeo)<=1.0);
	double geom_alt_rad = std::asin(sinGeo);
	double geom_alt_deg = M_180_PI*geom_alt_rad;
	if (geom_alt_deg > MIN_GEO_ALTITUDE_DEG)
	{
		// refraction from Saemundsson, S&T1986 p70 / in Meeus, Astr.Alg.
		double r=press_temp_corr * ( 1.02 / std::tan((geom_alt_deg+10.3/(geom_alt_deg+5.11))*M_PI_180) + 0.0019279);
		geom_alt_deg += r;
		if (geom_alt_deg > 90.)
			geom_alt_deg=90.;
	}
	else if(geom_alt_deg>MIN_GEO_ALTITUDE_DEG-TRANSITION_WIDTH_GEO_DEG)
	{
		// Avoids the jump below -5 by interpolating linearly between MIN_GEO_ALTITUDE_DEG and bottom of transition zone
		double r_m5=press_temp_corr * ( 1.02 / std::tan((MIN_GEO_ALTITUDE_DEG+10.3/(MIN_GEO_ALTITUDE_DEG+5.11))*M_PI_180) + 0.0019279);
		geom_alt_deg += r_m5*(geom_alt_deg-(MIN_GEO_ALTITUDE_DEG-TRANSITION_WIDTH_GEO_DEG))/TRANSITION_WIDTH_GEO_DEG;
	}
	else return;
	// At this point we have corrected geometric altitude. Note that if we just change altAzPos[2], we would change vector length, so this would change our angles.
	// We have to shorten X,Y components of the vector as well by the change in cosines of altitude, or (sqrt(1-sin(alt))

	const double refr_alt_rad=geom_alt_deg*M_PI_180;
	const double sinRef=std::sin(refr_alt_rad);

	const double shortenxy=((fabs(sinGeo)>=1.0) ? 1.0 : std::sqrt((1.-sinRef*sinRef)/(1.-sinGeo*sinGeo)));

	altAzPos[0]*=shortenxy;
	altAzPos[1]*=shortenxy;
	altAzPos[2]=sinRef*length;
}

// going from observed position to geometrical position.
void Refraction::innerRefractionBackward(Vec3d& altAzPos) const
{
	const double length = altAzPos.norm();
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

QByteArray Refraction::getForwardTransformShader() const
{
	return QByteArray(1+R"(
uniform float REFRACTION_press_temp_corr;
vec3 innerRefractionForward(vec3 altAzPos)
{
	const float PI = 3.14159265;
	const float M_180_PI = 180./PI;
	const float M_PI_180 = PI/180.;
	const float MIN_GEO_ALTITUDE_DEG=@MIN_GEO_ALTITUDE_DEG@;
	const float TRANSITION_WIDTH_GEO_DEG=@TRANSITION_WIDTH_GEO_DEG@;

	float press_temp_corr = REFRACTION_press_temp_corr;

	float len = length(altAzPos);
	if (len==0.0)
	{
		// Under some circumstances there are zero coordinates. Just leave them alone.
		return altAzPos;
	}

	float sinGeo = altAzPos[2]/len;
	float geom_alt_rad = asin(sinGeo);
	float geom_alt_deg = M_180_PI*geom_alt_rad;
	if (geom_alt_deg > MIN_GEO_ALTITUDE_DEG)
	{
		// refraction from Saemundsson, S&T1986 p70 / in Meeus, Astr.Alg.
		float r=press_temp_corr * ( 1.02 / tan((geom_alt_deg+10.3/(geom_alt_deg+5.11))*M_PI_180) + 0.0019279);
		geom_alt_deg += r;
		if (geom_alt_deg > 90.)
			geom_alt_deg=90.;
	}
	else if(geom_alt_deg>MIN_GEO_ALTITUDE_DEG-TRANSITION_WIDTH_GEO_DEG)
	{
		// Avoids the jump below -5 by interpolating linearly between MIN_GEO_ALTITUDE_DEG and bottom of transition zone
		float r_m5=press_temp_corr * ( 1.02 / tan((MIN_GEO_ALTITUDE_DEG+10.3/(MIN_GEO_ALTITUDE_DEG+5.11))*M_PI_180) + 0.0019279);
		geom_alt_deg += r_m5*(geom_alt_deg-(MIN_GEO_ALTITUDE_DEG-TRANSITION_WIDTH_GEO_DEG))/TRANSITION_WIDTH_GEO_DEG;
	}
	else return altAzPos;
	// At this point we have corrected geometric altitude. Note that if we just change altAzPos[2], we would change vector length, so this would change our angles.
	// We have to shorten X,Y components of the vector as well by the change in cosines of altitude, or (sqrt(1-sin(alt))

	float refr_alt_rad=geom_alt_deg*M_PI_180;
	float sinRef=sin(refr_alt_rad);

	// FIXME: do we really need double's mantissa length here as a comment in the C++ code says?
	float shortenxy = abs(sinGeo)>=1.0 ? 1.0 : sqrt((1.-sinRef*sinRef)/(1.-sinGeo*sinGeo));

	altAzPos[0]*=shortenxy;
	altAzPos[1]*=shortenxy;
	altAzPos[2]=sinRef*len;

	return altAzPos;
}

uniform mat4 REFRACTION_preTransfoMat;
uniform mat4 REFRACTION_postTransfoMat;

vec3 vertexToAltAzPos(vec3 v)
{
	vec3 altAzPosNotRefracted = (REFRACTION_preTransfoMat * vec4(v,1)).xyz;
	return innerRefractionForward(altAzPosNotRefracted);
}

vec3 modelViewForwardTransform(vec3 v)
{
	vec3 altAzPos = vertexToAltAzPos(v);
	return (REFRACTION_postTransfoMat * vec4(altAzPos, 1)).xyz;
}

#define HAVE_REFRACTION
)").replace("@MIN_GEO_ALTITUDE_DEG@", std::to_string(MIN_GEO_ALTITUDE_DEG).c_str())
   .replace("@TRANSITION_WIDTH_GEO_DEG@", std::to_string(TRANSITION_WIDTH_GEO_DEG).c_str());
}

QByteArray Refraction::getBackwardTransformShader() const
{
	return QByteArray(1+R"(
uniform float REFRACTION_press_temp_corr;

vec3 innerRefractionBackward(vec3 altAzPos)
{
	const float PI = 3.14159265;
	const float M_180_PI = 180./PI;
	const float M_PI_180 = PI/180.;
	const float MIN_APP_ALTITUDE_DEG=@MIN_APP_ALTITUDE_DEG@;
	const float TRANSITION_WIDTH_APP_DEG=@TRANSITION_WIDTH_APP_DEG@;
	float press_temp_corr = REFRACTION_press_temp_corr;

	float len = length(altAzPos);
	if (len==0.0)
	{
		// Under some circumstances there are zero coordinates. Just leave them alone.
		return altAzPos;
	}
	float sinObs = altAzPos[2]/len;
	float obs_alt_deg=M_180_PI*asin(sinObs);
	if (obs_alt_deg > 0.22879)
	{
		// refraction from Bennett, in Meeus, Astr.Alg.
		float r=press_temp_corr * (1. / tan((obs_alt_deg+7.31/(obs_alt_deg+4.4))*M_PI_180) + 0.0013515);
		obs_alt_deg -= r;
	}
	else if (obs_alt_deg > MIN_APP_ALTITUDE_DEG)
	{
		// backward refraction from polynomial fit against Saemundson[-5...-0.3]
		float r=(((((0.0444*obs_alt_deg+.7662)*obs_alt_deg+4.9746)*obs_alt_deg+13.599)*obs_alt_deg+8.052)*obs_alt_deg-11.308)*obs_alt_deg+34.341;
		obs_alt_deg -= press_temp_corr*r;
	}
	else if (obs_alt_deg > MIN_APP_ALTITUDE_DEG-TRANSITION_WIDTH_APP_DEG)
	{
		// Compute top value from polynome, apply linear interpolation
		const float r_min=(((((0.0444*MIN_APP_ALTITUDE_DEG+.7662)*MIN_APP_ALTITUDE_DEG
				+4.9746)*MIN_APP_ALTITUDE_DEG+13.599)*MIN_APP_ALTITUDE_DEG
			      +8.052)*MIN_APP_ALTITUDE_DEG-11.308)*MIN_APP_ALTITUDE_DEG+34.341;

		obs_alt_deg -= r_min*press_temp_corr*(obs_alt_deg-(MIN_APP_ALTITUDE_DEG-TRANSITION_WIDTH_APP_DEG))/TRANSITION_WIDTH_APP_DEG;
	}
	else return altAzPos;
	// At this point we have corrected observed altitude. Note that if we just change altAzPos[2], we would change vector length, so this would change our angles.
	// We have to make X,Y components of the vector a bit longer as well by the change in cosines of altitude, or (sqrt(1-sin(alt))

	float geo_alt_rad=obs_alt_deg*M_PI_180;
	float sinGeo=sin(geo_alt_rad);
	float longerxy=((abs(sinObs)>=1.0) ? 1.0 : sqrt((1.-sinGeo*sinGeo)/(1.-sinObs*sinObs)));
	altAzPos[0]*=longerxy;
	altAzPos[1]*=longerxy;
	altAzPos[2]=sinGeo*len;

	return altAzPos;
}

uniform mat4 REFRACTION_inversePreTransfoMat;
uniform mat4 REFRACTION_inversePostTransfoMat;

vec3 worldPosToAltAzPos(vec3 worldPos)
{
	return (REFRACTION_inversePostTransfoMat * vec4(worldPos,1)).xyz;
}

vec3 modelViewBackwardTransform(vec3 worldPos)
{
	vec3 altAzPosApparent = worldPosToAltAzPos(worldPos);
	vec3 altAzPosGeometric = innerRefractionBackward(altAzPosApparent);
	return (REFRACTION_inversePreTransfoMat * vec4(altAzPosGeometric,1)).xyz;
}

#define HAVE_REFRACTION
)").replace("@MIN_APP_ALTITUDE_DEG@", std::to_string(MIN_APP_ALTITUDE_DEG).c_str())
   .replace("@TRANSITION_WIDTH_APP_DEG@", std::to_string(TRANSITION_WIDTH_APP_DEG).c_str());
}

void Refraction::setForwardTransformUniforms(QOpenGLShaderProgram& program) const
{
	program.setUniformValue("REFRACTION_press_temp_corr", GLfloat(press_temp_corr));
	program.setUniformValue("REFRACTION_preTransfoMat", preTransfoMatf.toQMatrix());
	program.setUniformValue("REFRACTION_postTransfoMat", postTransfoMatf.toQMatrix());
}

void Refraction::setBackwardTransformUniforms(QOpenGLShaderProgram& program) const
{
	program.setUniformValue("REFRACTION_press_temp_corr", GLfloat(press_temp_corr));
	program.setUniformValue("REFRACTION_inversePreTransfoMat", invertPreTransfoMatf.toQMatrix());
	program.setUniformValue("REFRACTION_inversePostTransfoMat", invertPostTransfoMatf.toQMatrix());
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

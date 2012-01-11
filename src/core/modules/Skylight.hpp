/*
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
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */

// Class which computes the daylight sky color
// Fast implementation of the algorithm from the article
// "A Practical Analytic Model for Daylight" by A. J. Preetham, Peter Shirley and Brian Smits.

#ifndef _SKYLIGHT_HPP_
#define _SKYLIGHT_HPP_

#include <cmath>
#include <QDebug>
#include "StelUtils.hpp"

typedef struct {
	float zenithAngle;  // zenithAngle : angular distance to the zenith in radian
	float distSun;      // distSun     : angular distance to the sun in radian
	float color[3];     // 3 component color, can be RGB or CIE color system
} skylightStruct;

typedef struct {
	float pos[3];       // Vector to the position (vertical = pos[2])
	float color[3];     // 3 component color, can be RGB or CIE color system
} skylightStruct2;

class Skylight
{
public:
	Skylight();
	virtual ~Skylight();
	// Set the fixed parameters and precompute what can be
	// This function has to be called once before any call to get_*_value()
	void setParams(float sunZenithAngle, float turbidity);
	// Compute the sky color at the given position in the xyY color system and store it in position.color
	// void getxyYValue(skylightStruct * position);
	// Return the current zenith color
	inline void getZenithColor(float * v) const;

	// Same functions but in vector mode : faster because prevents extra cosine calculations
	// The position vectors MUST be normalized, and the vertical z component is the third one
	void setParamsv(const float * sunPos, float turbidity);
	
	// Compute the sky color at the given position in the CIE color system and store it in p.color
	// p.color[0] is CIE x color component
	// p.color[1] is CIE y color component
	// p.color[2] is undefined (CIE Y color component (luminance) if uncommented)
	void getxyYValuev(skylightStruct2& p) const
	{
		const float cosDistSun = sunPos[0]*p.pos[0] + sunPos[1]*p.pos[1] + sunPos[2]*p.pos[2];
		const float distSun = StelUtils::fastAcos(cosDistSun);
		const float cosDistSun_q = cosDistSun*cosDistSun;

		Q_ASSERT(p.pos[2] >= 0.f);
		const float oneOverCosZenithAngle = (p.pos[2]==0.) ? 1e99 : 1.f / p.pos[2];
		p.color[0] = term_x * (1.f + Ax * std::exp(Bx*oneOverCosZenithAngle))
				* (1.f + Cx * std::exp(Dx*distSun) + Ex * cosDistSun_q);

		p.color[1] = term_y * (1.f + Ay * std::exp(By*oneOverCosZenithAngle))
				* (1.f + Cy * std::exp(Dy*distSun) + Ey * cosDistSun_q);

// 		p.color[2] = term_Y * (1.f + AY * std::exp(BY*oneOverCosZenithAngle))
// 				* (1.f + CY * std::exp(DY*distSun) + EY * cosDistSun_q);


		if (/*p.color[2] < 0. || */p.color[0] < 0. || p.color[1] < 0.)
		{
			p.color[0] = 0.25;
			p.color[1] = 0.25;
			p.color[2] = 0.;
		}
	}

	void getShadersParams(Vec3f& asunPos, float& aterm_x, float& aAx, float& aBx, float& aCx, float& aDx, float& aEx,
		float& aterm_y, float& aAy, float& aBy, float& aCy, float& aDy, float& aEy) const
	{
		asunPos=sunPos;
		aterm_x=term_x;aAx=Ax;aBx=Bx;aCx=Cx;aDx=Dx;aEx=Ex;
		aterm_y=term_y;aAy=Ay;aBy=By;aCy=Cy;aDy=Dy;aEy=Ey;
	}
	
	
	
private:
	float thetas;  // angular distance between the zenith and the sun in radian
	float T;       // Turbidity : i.e. sky "clarity"
	               //  1 : pure air
	               //  2 : exceptionnally clear
	               //  4 : clear
	               //  8 : light haze
	               // 25 : haze
	               // 64 : thin fog

	// Computed variables depending on the 2 above
	float zenithLuminance;     // Y color component of the CIE color at zenith (luminance)
	float zenithColorX;        // x color component of the CIE color at zenith
	float zenithColorY;        // y color component of the CIE color at zenith

	float eyeLumConversion;    // luminance conversion for an eye adapted to screen luminance 
	                           // (around 40 cd/m^2)

	float AY, BY, CY, DY, EY;  // Distribution coefficients for the luminance distribution function
	float Ax, Bx, Cx, Dx, Ex;  // Distribution coefficients for x distribution function
	float Ay, By, Cy, Dy, Ey;  // Distribution coefficients for y distribution function

	float term_x;              // Precomputed term for x calculation
	float term_y;              // Precomputed term for y calculation
	float term_Y;              // Precomputed term for luminance calculation

	float sunPos[3];

	// Compute CIE Y (luminance) for zenith in cd/m^2
	inline void computeZenithLuminance(void);
	// Compute CIE x and y color components
	inline void computeZenithColor(void);
	// Compute the luminance distribution coefficients
	inline void computeLuminanceDistributionCoefs(void);
	// Compute the color distribution coefficients
	inline void computeColorDistributionCoefs(void);
};

// Return the current zenith color in xyY color system
inline void Skylight::getZenithColor(float * v) const
{
	v[0] = zenithColorX;
	v[1] = zenithColorY;
	v[2] = zenithLuminance;
}

// Compute CIE luminance for zenith in cd/m^2
inline void Skylight::computeZenithLuminance(void)
{
	zenithLuminance = 1000.f * ((4.0453f*T - 4.9710f) * std::tan( (0.4444f - T/120.f) * (M_PI-2.f*thetas) ) -
		0.2155f*T + 2.4192f);
	if (zenithLuminance<=0.f) zenithLuminance=0.00000000001;
}

// Compute CIE x and y color components
// Edit: changed some coefficients to get new sky color
inline void Skylight::computeZenithColor(void)
{
	static float thetas2;
	static float thetas3;
	static float T2;

	thetas2 = thetas * thetas;
	thetas3 = thetas2 * thetas;
	T2 = T * T;

	zenithColorX = ( 0.00216f*thetas3 - 0.00375f*thetas2 + 0.00209f*thetas) * T2 +
	               (-0.02903f*thetas3 + 0.06377f*thetas2 - 0.03202f*thetas + 0.00394f) * T +
	               ( 0.10169f*thetas3 - 0.21196f*thetas2 + 0.06052f*thetas + 0.25886f);

	zenithColorY = ( 0.00275f*thetas3 - 0.00610f*thetas2 + 0.00317f*thetas) * T2 +
	               (-0.04214f*thetas3 + 0.08970f*thetas2 - 0.04153f*thetas + 0.00516f) * T +
	               ( 0.14535f*thetas3 - 0.26756f*thetas2 + 0.06670f*thetas + 0.26688f);

}

// Compute the luminance distribution coefficients
// Edit: changed some coefficients to get new sky color
inline void Skylight::computeLuminanceDistributionCoefs(void)
{
	AY = 0.2787f*T - 1.0630f;
	BY =-0.3554f*T + 0.4275f;
	CY =-0.0227f*T + 6.3251f;
	DY = 0.1206f*T - 2.5771f;
	EY =-0.0670f*T + 0.3703f;
	// with BY>0 the formulas in getxyYValuev make no sense
	Q_ASSERT(BY <= 0.0);
}

// Compute the color distribution coefficients
// Edit: changed some coefficients to get new sky color
inline void Skylight::computeColorDistributionCoefs(void)
{
	Ax =-0.0148f*T - 0.1703f;
	Bx =-0.0664f*T + 0.0011f;
	Cx =-0.0005f*T + 0.2127f;
	Dx =-0.0641f*T - 0.8992f;
	Ex =-0.0035f*T + 0.0453f;

	Ay =-0.0131f*T - 0.2498f;
	By =-0.0951f*T + 0.0092f;
	Cy =-0.0082f*T + 0.2404f;
	Dy =-0.0438f*T - 1.0539f;
	Ey =-0.0109f*T + 0.0531f;
	// with Bx,By>0 the formulas in getxyYValuev make no sense
	Q_ASSERT(Bx <= 0.0);
	Q_ASSERT(By <= 0.0);
}


#endif // _SKYLIGHT_H_


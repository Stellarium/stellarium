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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

// Class which computes the daylight sky color
// Fast implementation of the algorithm from the article
// "A Practical Analytic Model for Daylight" by A. J. Preetham, Peter Shirley and Brian Smits.

#ifndef _SKYLIGHT_HPP_
#define _SKYLIGHT_HPP_

#include <cassert>
#include <cmath>

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
	// This funtion has to be called once before any call to get_*_value()
	void setParams(float sunZenithAngle, float turbidity);
	// Compute the sky color at the given position in the xyY color system and store it in position.color
	// void getxyYValue(skylightStruct * position);
	// Return the current zenith color
	inline void getZenithColor(float * v) const;

	// Same functions but in vector mode : faster because prevents extra cosine calculations
	// The position vectors MUST be normalized, and the vertical z component is the third one
	void setParamsv(const float * sunPos, float turbidity);
	void getxyYValuev(skylightStruct2& position) const;

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
inline void Skylight::computeZenithColor(void)
{
	static float thetas2;
	static float thetas3;
	static float T2;

	thetas2 = thetas * thetas;
	thetas3 = thetas2 * thetas;
	T2 = T * T;

	zenithColorX = ( 0.00166f*thetas3 - 0.00375f*thetas2 + 0.00209f*thetas) * T2 +
	               (-0.02903f*thetas3 + 0.06377f*thetas2 - 0.03202f*thetas + 0.00394f) * T +
	               ( 0.11693f*thetas3 - 0.21196f*thetas2 + 0.06052f*thetas + 0.25886f);

	zenithColorY = ( 0.00275f*thetas3 - 0.00610f*thetas2 + 0.00317f*thetas) * T2 +
	               (-0.04214f*thetas3 + 0.08970f*thetas2 - 0.04153f*thetas + 0.00516f) * T +
	               ( 0.15346f*thetas3 - 0.26756f*thetas2 + 0.06670f*thetas + 0.26688f);

}

// Compute the luminance distribution coefficients
inline void Skylight::computeLuminanceDistributionCoefs(void)
{
	AY = 0.1787f*T - 1.4630f;
	BY =-0.3554f*T + 0.4275f;
	CY =-0.0227f*T + 5.3251f;
	DY = 0.1206f*T - 2.5771f;
	EY =-0.0670f*T + 0.3703f;
	// with BY>0 the formulas in getxyYValuev make no sense
	assert(BY <= 0.0);
}

// Compute the color distribution coefficients
inline void Skylight::computeColorDistributionCoefs(void)
{
	Ax =-0.0193f*T - 0.2592f;
	Bx =-0.0665f*T + 0.0008f;
	Cx =-0.0004f*T + 0.2125f;
	Dx =-0.0641f*T - 0.8989f;
	Ex =-0.0033f*T + 0.0452f;

	Ay =-0.0167f*T - 0.2608f;
	By =-0.0950f*T + 0.0092f;
	Cy =-0.0079f*T + 0.2102f;
	Dy =-0.0441f*T - 1.6537f;
	Ey =-0.0109f*T + 0.0529f;
	// with Bx,By>0 the formulas in getxyYValuev make no sense
	assert(Bx <= 0.0);
	assert(By <= 0.0);
}


#endif // _SKYLIGHT_H_


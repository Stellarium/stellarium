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

#include <cmath>
#include <QDebug>
#include "StelUtils.hpp"

#ifndef HAVE_POW10
# define HAVE_POW10 1
# include <math.h>
# define pow10(x) std::exp((x) * 2.3025850930f)
#endif

#include "Skybright.hpp"
#include "StelUtils.hpp"

#ifdef __NetBSD__
#undef FS
#endif

Skybright::Skybright() : SN(1.f)
{
	setDate(2003, 8, 0);
	setLocation(M_PI_4, 1000., 25.f, 40.f);
	setSunMoon(0.5, 0.5);
}

// month : 1=Jan, 12=Dec
// moonPhase in radian 0=Full Moon, PI/2=First Quadrant/Last Quadran, PI=No Moon
void Skybright::setDate(int year, int month, float moonPhase)
{
	magMoon = -12.73f + 1.4896903f * std::fabs(moonPhase) + 0.04310727f * std::pow(moonPhase, 4.f);

	RA = (month - 3.f) * 0.52359878f;

	// Term for dark sky brightness computation
	bNightTerm = 1.0e-13 + 0.3e-13 * std::cos(0.57118f * (year-1992.f));
}


void Skybright::setLocation(float latitude, float altitude, float temperature, float relativeHumidity)
{
	float sign_latitude = (latitude>=0.f) * 2.f - 1.f;

	// extinction Coefficient for V band
	float KR = 0.1066f * std::exp(-altitude/8200.f);
	float KA = 0.1f * std::exp(-altitude/1500.f) * std::pow(1.f - 0.32f/std::log(relativeHumidity/100.f) ,1.33f) *
		(1.f + 0.33f * sign_latitude * std::sin(RA));
	float KO = 0.031f * std::exp(-altitude/8200.f) * ( 3.f + 0.4f * (latitude * std::cos(RA) - std::cos(3.f*latitude)) )/3.f;
	float KW = 0.031f * 0.94f * (relativeHumidity/100.f) * std::exp(temperature/15.f) * std::exp(-altitude/8200.f);
	K = KR + KA + KO + KW;
}

// Set the moon and sun zenith angular distance (cosin given)
// and precompute what can be
void Skybright::setSunMoon(float cosDistMoonZenith, float cosDistSunZenith)
{
	// Air mass for Moon
	if (cosDistMoonZenith<0) airMassMoon = 40.f;
	else airMassMoon = 1.f / (cosDistMoonZenith+0.025f*std::exp(-11.f*cosDistMoonZenith));

	// Air mass for Sun
	if (cosDistSunZenith<0) airMassSun = 40.f;
	else airMassSun = 1.f / (cosDistSunZenith+0.025f*std::exp(-11.f*cosDistSunZenith));

	bMoonTerm1 = pow10(-0.4f * (magMoon + 54.32f));

	// Moon should have no impact if below the horizon
	// .05 is ad hoc fadeout range - Rob
	if( cosDistMoonZenith < 0.f ) bMoonTerm1 *= 1.f + cosDistMoonZenith/0.05f;
	if(cosDistMoonZenith < -0.05f) bMoonTerm1 = 0.f;


	C3 = pow10(-0.4f*K*airMassMoon);	// Term for moon brightness computation

	bTwilightTerm = -6.724f + 22.918312f * (M_PI_2-std::acos(cosDistSunZenith));

	C4 = pow10(-0.4f*K*airMassSun);	// Term for sky brightness computation
}


// Compute the luminance at the given position
// Inputs : cosDistMoon = cos(angular distance between moon and the position)
//			cosDistSun  = cos(angular distance between sun  and the position)
//			cosDistZenith = cos(angular distance between zenith and the position)
float Skybright::getLuminance(float cosDistMoon,
                               float cosDistSun,
                               float cosDistZenith) const
{
	// Air mass
	const float bKX = pow10(-0.4f * K * (1.f / (cosDistZenith + 0.025f*StelUtils::fastExp(-11.f*cosDistZenith))));

	// Daylight brightness
	const float distSun = StelUtils::fastAcos(cosDistSun);
	const float FS = 18886.28f / (distSun*distSun + 0.0007f)
	               + pow10(6.15f - (distSun+0.001f)* 1.43239f)
	               + 229086.77f * ( 1.06f + cosDistSun*cosDistSun );
	const float b_daylight = 9.289663e-12 * (1.f - bKX) * (FS * C4 + 440000.f * (1.f - C4));

	//Twilight brightness
	const float b_twilight = pow10(bTwilightTerm + 0.063661977f * StelUtils::fastAcos(cosDistZenith)/(K> 0.05f ? K : 0.05f)) * (1.7453293f / distSun) * (1.f-bKX);

	// Total sky brightness
	float b_total = ((b_twilight<b_daylight) ? b_twilight : b_daylight);

	// Moonlight brightness, don't compute if less than 1% daylight
	if ((bMoonTerm1 * (1.f - bKX) * (28860205.1341274269 * C3 + 440000.f * (1.f - C3)))/b_total>0.01f)
	{
		float dist_moon;
		if (cosDistMoon >= 1.f) {cosDistMoon = 1.f;dist_moon = 0.f;}
		else
		{
			// Because the accuracy of our power serie is bad around 1, call the real acos if it's the case
			dist_moon = cosDistMoon > 0.99 ? std::acos(cosDistMoon) : StelUtils::fastAcos(cosDistMoon);
		}
		
		const float FM = 18886.28f / (dist_moon*dist_moon + 0.0005f)	// The last 0.0005 should be 0, but it causes too fast brightness change
			+ pow10(6.15f - dist_moon * 1.43239f)
			+ 229086.77f * ( 1.06f + cosDistMoon*cosDistMoon );
		b_total += bMoonTerm1 * (1.f - bKX) * (FM * C3 + 440000.f * (1.f - C3));
	}
	
	// Dark night sky brightness, don't compute if less than 1% daylight
	if ((bNightTerm*bKX)/b_total>0.01f)
	{
		b_total += (0.4f + 0.6f / std::sqrt(0.04f + 0.96f * cosDistZenith*cosDistZenith)) * bNightTerm * bKX;
	}
	
	return (b_total<0.f) ? 0.f : b_total * (900900.9f * M_PI * 1e-4 * 3239389.*2. *1.5);
	//5;	// In cd/m^2 : the 32393895 is empirical term because the
	// lambert -> cd/m^2 formula seems to be wrong...
}
/*
250 REM  Visual limiting magnitude
260 BL=B(3)/1.11E-15 : REM in nanolamberts*/

	// Airmass for each component
	//cosDistZenith = std::cos(dist_zenith);
	//float gaz_mass = 1.f / ( cosDistZenith + 0.0286f * std::exp(-10.5f * cosDistZenith) );
	//float aerosol_mass = 1.f / ( cosDistZenith + 0.0123f * std::exp(-24.5f * cosDistZenith) );
	//float ozone_mass = 1.f / std::sqrt( 0.0062421903f - cosDistZenith * cosDistZenith / 1.0062814f );
	// Total extinction for V band
	//float DM = KR*gaz_mass + KA*aerosol_mass + KO*ozone_mass + KW*gaz_mass;



	/*
	// Visual limiting magnitude
	if (BL>1500.0)
	{
		C1 = 4.466825e-9;
		C2 = 1.258925e-6;
	}
	else
	{
		C1 = 1.584893e-10;
		C2 = 0.012589254;
	}

	float TH = C1*std::pow(1.f+sqrt(C2*BL),2.f); // in foot-candles
	float MN = -16.57-2.5*log10f(TH)-DM+5.0*log10f(SN); // Visual Limiting Magnitude
	*/

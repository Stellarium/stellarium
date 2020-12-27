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

#include "Skybright.hpp"
#include "StelUtils.hpp"
#include "StelApp.hpp"
#include "StelModuleMgr.hpp"
#include "SolarSystem.hpp"

Skybright::Skybright() : SN(1.f)
{
	setDate(2003, 8, 0.f, 0.f);
	setLocation(M_PI_4f, 1000., 25.f, 40.f);
	setSunMoon(0.5, 0.5);
}

// month : 1=Jan, 12=Dec
// moonPhase in radian 0=Full Moon, PI/2=First Quadrant/Last Quadran, PI=No Moon
void Skybright::setDate(const int year, const int month, const float moonPhase, const float moonMag)
{
	// GZ The original formula set by Schaefer computes lunar magnitude here. But it does not take eclipse into account.
	//    For 0.16 we changed that. (Bug LP:#1471546)
	// Maybe we can use the moon mag formula elsewhere, don't delete yet!
	Q_UNUSED(moonPhase);
	//magMoon = -12.73f + 1.4896903f * fabsf(moonPhase) + 0.04310727f * powf(moonPhase, 4.f);
	magMoon=moonMag;

	// GZ: Bah, a very crude estimate for the solar position...
	RA = (month - 3) * 0.52359878f;

	// Term for dark sky brightness computation.
	// GZ: This works for a few 11-year solar cycles around 1992... ... cos((y-1992)/11 * 2pi)
	bNightTerm = 1.0e-13f + 0.3e-13f * cosf(0.57118f * (year-1992));
}


void Skybright::setLocation(const float latitude, const float altitude, const float temperature, const float relativeHumidity)
{
	float sign_latitude = (latitude>=0.f) * 2.f - 1.f;

	// extinction Coefficient for V band
	// GZ TODO: re-create UBVRI for colored extinction, and get RGB extinction factors from SkyBright!
	float KR = 0.1066f * expf(-altitude/8200.f); // Rayleigh
	float KA = 0.1f * expf(-altitude/1500.f) * powf(1.f - 0.32f/logf(relativeHumidity/100.f), 1.33f) *
		(1.f + 0.33f * sign_latitude * sinf(RA)); // Aerosol
	float KO = 0.031f * expf(-altitude/8200.f) * ( 3.f + 0.4f * (latitude * cosf(RA) - cosf(3.f*latitude)) )/3.f; // Ozone
	float KW = 0.031f * 0.94f * (relativeHumidity/100.f) * expf(temperature/15.f) * expf(-altitude/8200.f); // Water
	K = KR + KA + KO + KW; // Total extinction coefficient
}

// Set the moon and sun zenith angular distance (cosin given)
// and precompute what can be
void Skybright::setSunMoon(const float cosDistMoonZenith, const float cosDistSunZenith)
{
	// Air mass for Moon
	if (cosDistMoonZenith<0) airMassMoon = 40.f;
	else airMassMoon = 1.f / (cosDistMoonZenith+0.025f*expf(-11.f*cosDistMoonZenith));

	// Air mass for Sun
	if (cosDistSunZenith<0) airMassSun = 40.f;
	else airMassSun = 1.f / (cosDistSunZenith+0.025f*expf(-11.f*cosDistSunZenith));

	bMoonTerm1 = stelpow10f(-0.4f * (magMoon + 54.32f));

	// Moon should have no impact if below the horizon
	// .05 is ad hoc fadeout range - Rob
	if( cosDistMoonZenith < 0.f ) bMoonTerm1 *= 1.f + cosDistMoonZenith/0.05f;
	if(cosDistMoonZenith < -0.05f) bMoonTerm1 = 0.f;


	C3 = stelpow10f(-0.4f*K*airMassMoon);	// Term for moon brightness computation

	bTwilightTerm = -6.724f + 22.918312f * (M_PI_2f-acosf(cosDistSunZenith));

	C4 = stelpow10f(-0.4f*K*airMassSun);	// Term for sky brightness computation
}


// Compute the luminance at the given position
// Inputs : cosDistMoon = cos(angular distance between moon and the position)
//			cosDistSun  = cos(angular distance between sun  and the position)
//			cosDistZenith = cos(angular distance between zenith and the position)
float Skybright::getLuminance( float cosDistMoon,
                               const float cosDistSun,
                               const float cosDistZenith) const
{
	// No Sun and Moon on the sky
	// Details: https://bugs.launchpad.net/stellarium/+bug/1499699
	if (!GETSTELMODULE(SolarSystem)->getFlagPlanets())
		return 0.f;

	// Air mass
	const float bKX = stelpow10f(-0.4f * K * (1.f / (cosDistZenith + 0.025f*StelUtils::fastExp(-11.f*cosDistZenith))));

	// Daylight brightness
	const float distSun = StelUtils::fastAcos(cosDistSun);
	const float FSv = 18886.28f / (distSun*distSun + 0.0007f)
	               + stelpow10f(6.15f - (distSun+0.001f)* 1.43239f)
	               + 229086.77f * ( 1.06f + cosDistSun*cosDistSun );
	const float b_daylight = 9.289663e-12f * (1.f - bKX) * (FSv * C4 + 440000.f * (1.f - C4));

	//Twilight brightness
	const float b_twilight = stelpow10f(bTwilightTerm + 0.063661977f * StelUtils::fastAcos(cosDistZenith)/(K> 0.05f ? K : 0.05f)) * (1.7453293f / distSun) * (1.f-bKX);

	// Total sky brightness
	float b_total = ((b_twilight<b_daylight) ? b_twilight : b_daylight);

	// Moonlight brightness, don't compute if less than 1% daylight
	if ((bMoonTerm1 * (1.f - bKX) * (28860205.1341274269f * C3 + 440000.f * (1.f - C3)))/b_total>0.01f)
	{
		float dist_moon;
		if (cosDistMoon >= 1.f) {cosDistMoon = 1.f;dist_moon = 0.f;}
		else
		{
			// Because the accuracy of our power serie is bad around 1, call the real acos if it's the case
			dist_moon = cosDistMoon > 0.99f ? acosf(cosDistMoon) : StelUtils::fastAcos(cosDistMoon);
		}
		
		const float FM = 18886.28f / (dist_moon*dist_moon + 0.0005f)	// The last 0.0005 should be 0, but it causes too fast brightness change
			+ stelpow10f(6.15f - dist_moon * 1.43239f)
			+ 229086.77f * ( 1.06f + cosDistMoon*cosDistMoon );
		b_total += bMoonTerm1 * (1.f - bKX) * (FM * C3 + 440000.f * (1.f - C3));
	}
	
	// Dark night sky brightness, don't compute if less than 1% daylight
	if ((bNightTerm*bKX)/b_total>0.01f)
	{
		b_total += (0.4f + 0.6f / sqrtf(0.04f + 0.96f * cosDistZenith*cosDistZenith)) * bNightTerm * bKX;
	}
	
	return (b_total<0.f) ? 0.f : b_total * (900900.9f * M_PIf * 1e-4f * 3239389.f*2.f *1.5f);
	//5;	// In cd/m^2 : the 32393895 is empirical term because the
	// lambert -> cd/m^2 formula seems to be wrong...
}


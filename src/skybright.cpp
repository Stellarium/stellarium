/*
 * Copyright (C) 2003 Fabien Chéreau
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

#include <math.h>
#include "skybright.h"

skybright::skybright() : SN(1.f)
{
	set_date(2003, 8, 0.f);
	set_loc(45.f, 1000., 25.f, 40.f);
	set_sun_moon(0.5, 0.5);
}

// month : 1=Jan, 12=Dec
void skybright::set_date(int year, int month, float moon_phase)
{
	mag_moon = -12.73f + 0.026f * fabs(moon_phase) + 0.04310727f * pow(moon_phase, 4.f);
	RA = (month - 3.f) * 0.52359878f;

	// Term for dark sky brightness computation
	b_night_term = 1.0e-13 + 0.3e-13 * cosf(0.57118182f * (year-1992.f));
}


void skybright::set_loc(float latitude, float altitude, float temperature, float relative_humidity)
{
	float sign_latitude = (latitude>=0.f) * 2.f - 1.f;

	// extinction Coefficient for V band
	float KR = 0.1066f * expf(-altitude/8200.f);
	float KA = 0.1f * expf(-altitude/1500.f) * powf(1 - 0.32f/logf(relative_humidity/100.f) ,1.33f) *
		(1.f + 0.33f * sign_latitude * sinf(RA));
	float KO = 0.031f * ( 3.f + 0.4f * (latitude * cosf(RA) - cosf(3*latitude)) )/3.f;
	float KW = 0.031f * 0.94f * (relative_humidity/100.f) * expf(temperature/15.f) * expf(-altitude/8200.f);
	K = KR + KA + KO + KW;
}

// Set the moon and sun zenith angular distance (cosin given)
// and precompute what can be
void skybright::set_sun_moon(float cos_dist_moon_zenith, float cos_dist_sun_zenith)
{
	// Air mass for Moon
	if (cos_dist_moon_zenith<0) air_mass_moon = 40.f;
	else air_mass_moon = 1.f / (cos_dist_moon_zenith+0.025f*expf(-11.f*cos_dist_moon_zenith));

	// Air mass for Sun
	if (cos_dist_sun_zenith<0) air_mass_sun = 40;
	else air_mass_sun = 1.f / (cos_dist_sun_zenith+0.025f*expf(-11.f*cos_dist_sun_zenith));

	// moon magnitude
	if (cos_dist_moon_zenith>0) b_moon_term1 = 0.f;

	C3 = powf(10.f, -0.4f*K*air_mass_moon);	// Term for moon brightness computation
	b_moon_term1 = powf(10.f, -0.4 * (mag_moon + 54.32f));

	b_twilight_term = 105.62035f - 360.f * (M_PI_2-acosf(cos_dist_sun_zenith));

	C4 = powf(10.f, -0.4f*K*air_mass_sun);	// Term for sky brightness computation
}


// Compute the luminance at the given position
// Inputs : cos_dist_moon = cos(angular distance between moon and the position)
//			cos_dist_sun  = cos(angular distance between sun  and the position)
//			cos_dist_zenith = cos(angular distance between zenith and the position)
float skybright::get_luminance(float cos_dist_moon, float cos_dist_sun, float cos_dist_zenith)
{
	static float dist_moon;
	dist_moon = acosf(cos_dist_moon);
	static float dist_sun;
	dist_sun = acosf(cos_dist_sun);

	// Air mass
	float X = 1.f / (cos_dist_zenith+0.025f*expf(-11.f*cos_dist_zenith));
	float bKX = powf(10.f, -0.4f * K * X);

	// Dark night sky brightness
	b_night = 0.4f+0.6f/sqrt(0.04f - 0.96f * cos_dist_zenith*cos_dist_zenith);
	b_night *= b_night_term * bKX;

	// Moonlight brightness
	float FM = 2.035339e11 * dist_moon*dist_moon + powf(10.f, 6.15f - dist_moon * 1.43239f);
	FM += 229086.77f * ( 1.06f + cos_dist_moon*cos_dist_moon );
	b_moon = b_moon_term1 * (1.f - bKX) * (FM * C3 + 440000.f * (1.f - C3));

	//Twilight brightness
	b_twilight = powf(10.f, -M_2_PI * (b_twilight_term - dist_sun/K)) * (1.7453293f / dist_sun) * (1.f-bKX);

	// Daylight brightness
	float FS = 2.035339e11 * dist_sun*dist_sun + powf(10.f, 6.15f - dist_sun* 1.43239f);
	FS += 229086.77f * ( 1.06f + cos_dist_sun*cos_dist_sun );
	b_daylight = 9.289663e-12 * (1.f - bKX) * FS * C4 + 440000.f * (1.f - C4);

	// Total sky brightness
	if (b_daylight>b_twilight) b_total = b_night + b_twilight + b_moon;
	else b_total = b_night + b_daylight + b_moon;

	b_total /= 1.11e-15; //in nanolamberts
	return b_total * M_PI*1e-13;	// In cd/m^2
}


	// Airmass for each component
	//cos_dist_zenith = cosf(dist_zenith);
	//float gaz_mass = 1.f / ( cos_dist_zenith + 0.0286f * expf(-10.5f * cos_dist_zenith) );
	//float aerosol_mass = 1.f / ( cos_dist_zenith + 0.0123f * expf(-24.5f * cos_dist_zenith) );
	//float ozone_mass = 1.f / sqrtf( 0.0062421903f - cos_dist_zenith * cos_dist_zenith / 1.0062814f );
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

	float TH = C1*powf(1.f+sqrt(C2*BL),2.f); // in foot-candles
	float MN = -16.57-2.5*log10f(TH)-DM+5.0*log10f(SN); // Visual Limiting Magnitude
	*/

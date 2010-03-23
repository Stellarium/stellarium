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

#include "RefractionExtinction.hpp"

RefractionExtinction::RefractionExtinction() : pressure(1013.f), temperature(10.f), ext_coeff(0.20f)
{
	updatePrecomputed();
}

void RefractionExtinction::updatePrecomputed()
{
	press_temp_corr_Bennett=pressure/1010.f * 283.f/(273.f+temperature) / 60.f;
	press_temp_corr_Saemundson=1.02f*press_temp_corr_Bennett;
}

void RefractionExtinction::forward(Vec3d* altAzPos, float* mag, int size)
{
	// Assuming altAzPos is the normalized star position vector, and its z component sin(altitude).
	for (int i=0; i<size; ++i)
	{
		float geom_alt_deg=(180.f/M_PI)*std::asin(altAzPos[i][2]);
		if (geom_alt_deg > -2.f)
		{
			// refraction from Saemundsson, S&T1986 p70 / in Meeus, Astr.Alg.
			float r=press_temp_corr_Saemundson / std::tan((geom_alt_deg+10.3f/(geom_alt_deg+5.11f))*M_PI/180.f) + 0.0019279f;
			geom_alt_deg += r;
			if (geom_alt_deg > 90.f)
				geom_alt_deg=90.f; // SAFETY, SHOULD NOT BE NECESSARY
			altAzPos[i][2]=std::sin(geom_alt_deg*M_PI/180.f);

			// now altitude is corrected, but object still too bright.
			// note that sin(h)=cos(z)...
			mag[i] += airmass(altAzPos[i][2], true) * ext_coeff;
		}
	}
}

void RefractionExtinction::backward(Vec3d* altAzPos, float* mag, int size)
{
	// going from observed position/magnitude to geometrical position and atmosphere-free mag.
	for (int i=0; i<size; ++i)
	{
		float obs_alt_deg=(180.f/M_PI)*std::asin(altAzPos[i][2]);
		if (obs_alt_deg > -2.f)
		{
			mag[i] -= airmass(altAzPos[i][2], true) * ext_coeff;
			// refraction from Bennett, in Meeus, Astr.Alg.
			float r=press_temp_corr_Bennett / std::tan((obs_alt_deg+7.31/(obs_alt_deg+4.4f))*M_PI/180.f) + 0.0013515f;
			obs_alt_deg -= r;
			altAzPos[i][2]=std::sin(obs_alt_deg*M_PI/180.f);
		}
	}
}

// airmass computation for cosine of zenith angle z
float RefractionExtinction::airmass(float cosZ, bool apparent_z)
{
	if (cosZ<-0.035f)
		return 0.0f; // Safety: Do nothing for below -2 degrees.

	float X;
	if (apparent_z)
	{
		// Rozenberg 1966, reported by Schaefer (1993-2000).
		X=1.0f/(cosZ+0.025*std::exp(-11.0*cosZ));
	}
	else
	{
		//Young 1994
		float nom=(1.002432*cosZ+0.148386)*cosZ+0.0096467;
		float denum=((cosZ+0.149864)*cosZ+0.0102963)*cosZ+0.000303978;
		X=nom/denum;
	}
	return X;
}


void RefractionExtinction::setPressure(float p)
{
	pressure=p;
	updatePrecomputed();
}

void RefractionExtinction::setTemperature(float t)
{
	temperature=t;
	updatePrecomputed();
}

void RefractionExtinction::setExtinctionCoefficient(float k)
{
	ext_coeff=k;
}

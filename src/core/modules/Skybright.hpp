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

#ifndef SKYBRIGHT_HPP
#define SKYBRIGHT_HPP

//! @class Skybright
//! Compute the luminance of the sky according to some parameters like sun moon position
//! or time or altitude etc...
// GZ: tentative additions to the documentation.
//! This class makes use of the 1998 sky brightness model by Bradley Schaefer.
//! Further reading:
//! - B. Schaefer: Sky&Telescope 4/1987
//! - B. Schaefer: Astronomy and the Limits of Vision. Vistas in Astronomy 36, 311-361, 1993.
//! - B. Schaefer: To the Visual Limits. Sky&Telescope 5/1998 57-60.
//! - B. Schaefer: Archaeoastronomy XV, 2000.
//!
//! TASKS TO IMPROVE:
//! Some components of the Schaefer model as given in BASIC sourcecode in the 1998 S&T article are strongly simplified. E.g.,
//! - setDate should be taken always in the Gregorian Calendar, or circumvented altogether:
//! - RA takes the right ascension of the sun. --> Could be replaced with the proper value!
//! - bNightTerm includes an 11.0-year solar cycle, and will be erratic a few cycles from 1992. --> At least find a more accurate average period.
//! - Extinction could make use of the K computed in SetLocation. But first re-verify that
//! the code here really follows the Schaefer model (with obvious amendments).
class Skybright
{
public:
	//! Constructor
	Skybright();

	//! Set the sky date to use for atmosphere computation
	//! @param year the year in YYYY format
	//! @param month the month: 1=Jan, 12=Dec
	//! @param moonPhase the moon phase in radian 0=Full Moon, PI/2=First Quadrant/Last Quadran, PI=No Moon
	//! @param moonMag visual magnitude of the moon. Could be computed from geometry, but in case of eclipses we should deliver it here.
	void setDate(const int year, const int month, const float moonPhase, const float moonMag);

	//! Set the position parameters to use for atmosphere computation
	//! @param latitude observer latitude in radian
	//! @param altitude observer altitude in m
	//! @param temperature temperature in deg. C
	//! @param relativeHumidity air humidity in %
	void setLocation(const float latitude, const float altitude, const float temperature=15.f, const float relativeHumidity=40.f);

	//! Set the moon and sun zenith angular distance (cosin given) and precompute what can be
	//! This function has to be called once before any call to getLuminance()
	//! @param cosDistMoonZenith cos(angular distance between moon and zenith)
	//! @param cosDistSunZenith cos(angular distance between sun and zenith)
	void setSunMoon(const float cosDistMoonZenith, const float cosDistSunZenith);

	//! Compute the luminance at the given position
	//! @param cosDistMoon cos(angular distance between moon and the position)
	//! @param cosDistSun cos(angular distance between sun  and the position)
	//! @param cosDistZenith cos(angular distance between zenith and the position)
	float getLuminance(float cosDistMoon, const float cosDistSun, const float cosDistZenith) const;

private:
	float airMassMoon;  // Air mass for the Moon
	float airMassSun;   // Air mass for the Sun
	float magMoon;      // Moon magnitude
	float RA;           // Extremely crude estimate of Right Ascension of the Sun!
	float K;            // Summary extinction coefficient
	float C3;           // Term for moon brightness computation
	float C4;           // Term for sky brightness computation
	float SN;           // Snellen Ratio (20/20=1.0, good 20/10=2.0)

	// Optimisation variables
	float bNightTerm;
	float bMoonTerm1;
	float bTwilightTerm;
};

#endif // SKYBRIGHT_HPP

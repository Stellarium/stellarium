/*
 * Stellarium
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

/*
	Class which compute and display the daylight sky color using openGL
	the sky is computed with the skylight class.
*/

#ifndef _STEL_ATMOSTPHERE_H_
#define _STEL_ATMOSTPHERE_H_

#include "skylight.h"
#include "vecmath.h"
#include "navigator.h"
#include "tone_reproductor.h"
#include "skybright.h"
#include "fader.h"

using namespace std;

class stel_atmosphere
{
public:
    stel_atmosphere();
    virtual ~stel_atmosphere();
	void compute_color(double JD, Vec3d sunPos, Vec3d moonPos, float moon_phase, tone_reproductor * eye, Projector* prj,
		float latitude = 45.f, float altitude = 200.f,
		float temperature = 15.f, float relative_humidity = 40.f);
	void draw(Projector* prj, int delta_time);
	void update(int delta_time) {fader.update(delta_time);}
	void set_fade_duration(float duration) {fader.set_duration((int)(duration*1000.f));}
	void show(bool b){fader = b;}
	float get_intensity(void) {return atm_intensity; }  // tells you actual atm intensity due to eclipses + fader
	float get_fade_intensity(void) {return fader.get_interstate();}  // let's you know how far faded in or out the atm is (0-1)
	float get_world_adaptation_luminance(void) const {return world_adaptation_luminance;}
private:
	skylight sky;
	skybright skyb;
	int sky_resolution;
	Vec3f ** tab_sky;	// For Atmosphere calculation
	int startY;			// intern variable used to store the Horizon Y screen value
	float world_adaptation_luminance;
// 	bool atm_on;
	float atm_intensity;
// 	float ai;               // used to calculate atm_intensity via curve function
// 	float fade_duration;    // length of time, in miliseconds, for fade in and out
	linear_fader fader;
};

#endif // _STEL_ATMOSTPHERE_H_

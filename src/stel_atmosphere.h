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

using namespace std;

class stel_atmosphere
{
public:
    stel_atmosphere();
    virtual ~stel_atmosphere();
	void compute_color(navigator *, tone_reproductor *);
	void draw();
private:
	skylight sky;
	int sky_resolution;
	Vec3f ** tab_sky;                           // For Atmosphere calculation
};

#endif // _STEL_ATMOSTPHERE_H_

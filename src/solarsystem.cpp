/*
 * Stellarium
 * Copyright (C) 2002 Fabien Chéreau
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

#include "solarsystem.h"
#include "s_texture.h"
#include "stellplanet.h"

// Planet map textures
static s_texture * sun_map;
//static s_texture * mercury_map;
//static s_texture * venus_map;
//static s_texture * earth_map;
static s_texture * moon_map;

// Planet small halo texture
static s_texture * small_halo;
static s_texture * sun_halo;

 // Create and init the solar system
void InitSolarSystem(void)
{
    sun_map = new s_texture("sun",TEX_LOAD_TYPE_PNG_SOLID);
    // mercury_map = new s_texture("mercury",TEX_LOAD_TYPE_PNG_SOLID);
    // venus_map = new s_texture("venus",TEX_LOAD_TYPE_PNG_SOLID);
    moon_map = new s_texture("lune",TEX_LOAD_TYPE_PNG_SOLID);

	small_halo = new s_texture("star16x16");
	sun_halo = new s_texture("halo");

	Sun = new sun_planet("Sun", NO_HALO, 696000./UA, vec3_t(1.,1.,0.8), sun_map, NULL, sun_halo);
	Moon = new planet("Moon", NO_HALO, 1738./UA, vec3_t(1.,1.,1.), moon_map, NULL, get_lunar_geo_posn);
}

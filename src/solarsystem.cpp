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
static s_texture * mercury_map;
static s_texture * venus_map;
static s_texture * earth_map;
static s_texture * moon_map;

// Planet small halo texture
static s_texture * small_halo;
static s_texture * sun_halo;

planet * Sun;             	  		  // Sun, center of the solar system
planet * Moon;             	  		  // Moon, around earth
planet * Earth;
planet * Mercury;
planet * Venus;

 // Create and init the solar system
void InitSolarSystem(void)
{
    char tempName[255];
    strcpy(tempName,global.DataDir);
    strcat(tempName,"spacefont.txt");
    planet_name_font = new s_font(0.014*global.X_Resolution,"spacefont", tempName);
    if (!planet_name_font)
    {
	    printf("Can't create planet_name_font\n");
        exit(-1);
    }

    
    sun_map = new s_texture("sun",TEX_LOAD_TYPE_PNG_SOLID);
    earth_map = new s_texture("earthmap",TEX_LOAD_TYPE_PNG_SOLID);
    moon_map = new s_texture("lune",TEX_LOAD_TYPE_PNG_SOLID);
    mercury_map = new s_texture("mercury",TEX_LOAD_TYPE_PNG_SOLID);
    venus_map = new s_texture("venus",TEX_LOAD_TYPE_PNG_SOLID);
    
	small_halo = new s_texture("star16x16");
	sun_halo = new s_texture("halo");

	Sun = new sun_planet("Sun", NO_HALO, 696000./UA, vec3_t(1.,1.,0.8), sun_map, NULL, sun_halo);
	Moon = new planet("Moon", NO_HALO, 1738./UA, vec3_t(1.,1.,1.), moon_map, NULL, get_lunar_geo_posn);
	Earth = new planet("Earth", NO_HALO, 6378.1/UA, vec3_t(1.,1.,1.), earth_map, NULL, get_earth_helio_coords);
	Mercury = new planet("Mercury", NO_HALO, 2439.7/UA, vec3_t(1.,1.,1.), mercury_map, NULL, get_mercury_helio_coords);
	Venus = new planet("Venus", NO_HALO, 6052./UA, vec3_t(1.,1.,1.), venus_map, NULL, get_venus_helio_coords);
	
    Venus->set_rotation_elements(5832.479839/24., 137.45, J2000, -178.78*M_PI/180., 76.681*M_PI/180., 0);
    Mercury->set_rotation_elements(1407.509405/24., 291.20, J2000, -7.01*M_PI/180., 48.33167*M_PI/180., 0);
    Earth->set_rotation_elements(23.9344694/24., 280.5, J2000, 23.45*M_PI/180., -11.2606423*M_PI/180., 0);
	Moon->set_rotation_elements((27.321661*24)/24., 38, J2000, 23.45*M_PI/180., 0., 0);

	Sun->addSatellite(Mercury);
	Sun->addSatellite(Venus);
	Sun->addSatellite(Earth);
	Earth->addSatellite(Moon);
}

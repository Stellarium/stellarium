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
#include "orbit.h"
#include "stellarium.h"

// Planet map textures
static s_texture * sun_map;
static s_texture * mercury_map;
static s_texture * venus_map;
static s_texture * earth_map;
static s_texture * moon_map;
static s_texture * mars_map;
static s_texture * jupiter_map;
static s_texture * saturn_map;
static s_texture * uranus_map;
static s_texture * neptune_map;
static s_texture * pluto_map;

// Planet small halo texture
static s_texture * small_halo;
static s_texture * sun_halo;

// Planets objects
planet * Sun;		// Sun, center of the solar system
planet * Mercury;
planet * Venus;
planet * Mars;
planet * Earth;
planet * Moon;		// Moon, around earth
planet * Jupiter;
planet * Saturn;
planet * Uranus;
planet * Neptune;
planet * Pluto;

EllipticalOrbit * jupiter_orbit;
// In testing
void get_jupiter_orbit_helio_coords(double JD, double * X, double * Y, double * Z)
{
//	get_jupiter_helio_coords(JD, X, Y, Z);
//	printf("1 : %lf %lf %lf\n",*X,*Y,*Z);

//	jupiter_orbit->positionAtTime(JD, X, Y, Z);
//	printf("2 : %lf %lf %lf\n",*X,*Y,*Z);
}

 // Create and init the solar system
void InitSolarSystem(void)
{
    char tempName[255];
    strcpy(tempName,global.DataDir);
    strcat(tempName,"spacefont.txt");
    planet_name_font = new s_font(13,"spacefont", tempName);
    if (!planet_name_font)
    {
	    printf("Can't create planet_name_font\n");
        exit(-1);
    }

	jupiter_orbit = new EllipticalOrbit(5.2034 * (1.0 - 0.0484),
		0.0484, 1.3053*M_PI/180., 100.556*M_PI/180., (14.7539 - 100.556)*M_PI/180., -14.7539*M_PI/180., 11.8622, J2000);


    sun_map = new s_texture("sun",TEX_LOAD_TYPE_PNG_SOLID);
    earth_map = new s_texture("earthmap",TEX_LOAD_TYPE_PNG_SOLID);
    moon_map = new s_texture("lune",TEX_LOAD_TYPE_PNG_SOLID);
    mercury_map = new s_texture("mercury",TEX_LOAD_TYPE_PNG_SOLID);
    venus_map = new s_texture("venus",TEX_LOAD_TYPE_PNG_SOLID);
    jupiter_map = new s_texture("jupiter",TEX_LOAD_TYPE_PNG_SOLID);
    mars_map = new s_texture("mars",TEX_LOAD_TYPE_PNG_SOLID);
    saturn_map = new s_texture("saturn",TEX_LOAD_TYPE_PNG_SOLID);
    uranus_map = new s_texture("uranus",TEX_LOAD_TYPE_PNG_SOLID);
    neptune_map = new s_texture("neptune",TEX_LOAD_TYPE_PNG_SOLID);
    pluto_map = new s_texture("lune",TEX_LOAD_TYPE_PNG_SOLID);

	small_halo = new s_texture("star16x16");
	sun_halo = new s_texture("halo");

	Sun = new sun_planet("Sun", NO_HALO, 696000./AU, vec3_t(1.,1.,0.8), sun_map, NULL, sun_halo);
	Moon = new planet("Moon", NO_HALO, 1738./AU, vec3_t(1.,1.,1.), moon_map, NULL, get_lunar_geo_posn);
	Earth = new planet("Earth", NO_HALO, 6378.1/AU, vec3_t(1.,1.,1.), earth_map, NULL, get_earth_helio_coords);
	Mercury = new planet("Mercury", NO_HALO, 2439.7/AU, vec3_t(1.,1.,1.), mercury_map, NULL, get_mercury_helio_coords);
	Venus = new planet("Venus", NO_HALO, 6052./AU, vec3_t(1.,1.,1.), venus_map, NULL, get_venus_helio_coords);
	Mars = new planet("Mars", NO_HALO, 3394./AU, vec3_t(1.,1.,1.), mars_map, NULL, get_mars_helio_coords);
	Jupiter = new planet("Jupiter", NO_HALO, 71398./AU, vec3_t(1.,1.,1.), jupiter_map, NULL, get_jupiter_helio_coords);
	Saturn = new planet("Saturn", NO_HALO, 60330./AU, vec3_t(1.,1.,1.), saturn_map, NULL, get_saturn_helio_coords);
	Uranus = new planet("Uranus", NO_HALO, 26200./AU, vec3_t(1.,1.,1.), uranus_map, NULL, get_uranus_helio_coords);
	Neptune = new planet("Neptune", NO_HALO, 25225./AU, vec3_t(1.,1.,1.), neptune_map, NULL, get_neptune_helio_coords);
	Pluto = new planet("Pluto", NO_HALO, 1137./AU, vec3_t(1.,1.,1.), pluto_map, NULL, get_pluto_helio_coords);


    Mercury->set_rotation_elements(1407.509405/24., 291.20, J2000, -7.01*M_PI/180., 48.33167*M_PI/180., 0);
    Venus->set_rotation_elements(5832.479839/24., 137.45, J2000, -178.78*M_PI/180., 76.681*M_PI/180., 0);
    Earth->set_rotation_elements(23.9344694/24., 280.5, J2000, 23.438855*M_PI/180., -11.2606423*M_PI/180., 0);
	Moon->set_rotation_elements((27.321661*24)/24., 38, J2000, 23.45*M_PI/180., 0., 0);
    Mars->set_rotation_elements(24.622962/24., 136.005, J2000, -26.72*M_PI/180., 82.91*M_PI/180., 0);
    Jupiter->set_rotation_elements(9.927953/24., 16, J2000, -2.222461*M_PI/180., -22.203*M_PI/180., 0);
    Saturn->set_rotation_elements(10.65622/24., 358.922, J2000, -2.222461*M_PI/180., 169.530*M_PI/180., 0);
    Uranus->set_rotation_elements(17.24/24., 331.18, J2000, -97.81*M_PI/180., 167.76*M_PI/180., 0);
    Neptune->set_rotation_elements(16.11/24., 228.65, J2000, -28.03*M_PI/180., 49.235*M_PI/180., 0);
    Pluto->set_rotation_elements(248.54/24., 320.75, J2000, -115.60*M_PI/180., 228.34*M_PI/180., 0);


	Sun->addSatellite(Mercury);
	Sun->addSatellite(Venus);
	Sun->addSatellite(Earth);
	Earth->addSatellite(Moon);
	Sun->addSatellite(Mars);
	Sun->addSatellite(Jupiter);
	Sun->addSatellite(Saturn);
	Sun->addSatellite(Uranus);
	Sun->addSatellite(Neptune);
	Sun->addSatellite(Pluto);
}

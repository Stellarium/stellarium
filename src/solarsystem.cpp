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
#include "init_parser.h"

SolarSystem::SolarSystem() : sun(NULL), moon(NULL), earth(NULL)
{
}

SolarSystem::~SolarSystem()
{
    vector<planet*>::iterator iter = system_planets.begin();
    while (iter != system_planets.end())
    {
        if (*iter) delete *iter;
		*iter = NULL;
        iter++;
    }
	sun = NULL;
	moon = NULL;
	earth = NULL;

	if (planet_name_font) delete planet_name_font;
}


// Init and load the solar system data
void SolarSystem::init(const char* font_fileName, const char* planetfile)
{
    planet_name_font = new s_font(13,"spacefont", font_fileName);
    if (!planet_name_font)
    {
	    printf("Can't create planet_name_font\n");
        exit(-1);
    }
	planet::set_font(planet_name_font);
	load(planetfile);
}

// Init and load the solar system data
void SolarSystem::load(const char* planetfile)
{
	printf("Loading Solar System data...\n");
	init_parser pd(planetfile);	// The planet data ini file parser
	pd.load();


	pos_func_type posfunc;

	int nbSections = pd.get_nsec();
	for (int i = 0;i<nbSections;i++)
	{
		const char* secname = pd.get_secname(i);

		const char* funcname = pd.get_str(secname, "coord_func");

		if (!strcmp(funcname, "sun_special"))
			posfunc = makeFunctor((p_pos_func_type)0,get_sun_helio_coords);

		if (!strcmp(funcname, "mercury_special"))
			posfunc = makeFunctor((p_pos_func_type)0,get_mercury_helio_coords);

		if (!strcmp(funcname, "venus_special"))
			posfunc = makeFunctor((p_pos_func_type)0,get_venus_helio_coords);

		if (!strcmp(funcname, "earth_special"))
			posfunc = makeFunctor((p_pos_func_type)0,get_earth_helio_coords);

		if (!strcmp(funcname, "lunar_special"))
			posfunc = makeFunctor((p_pos_func_type)0,get_lunar_geo_posn);

		if (!strcmp(funcname, "mars_special"))
			posfunc = makeFunctor((p_pos_func_type)0,get_mars_helio_coords);

		// Create the planet and add it to the list
		planet* p = new planet(pd.get_str(secname, "name"), pd.get_boolean(secname, "halo"),
			pd.get_double(secname, "radius")/AU, str_to_vec3f(pd.get_str(secname, "color")),
			pd.get_str(secname, "tex_map"), pd.get_str(secname, "tex_halo"), posfunc);

		const char * str_parent = pd.get_str(secname, "parent");
		if (strcmp(str_parent, "none"))
		{
			// Look in the other planets the one named with str_parent
			bool have_parent = false;
    		vector<planet*>::iterator iter = system_planets.begin();
    		while (iter != system_planets.end())
    		{
        		if (!strcmp((*iter)->get_name(),str_parent))
				{
					(*iter)->add_satellite(p);
					have_parent = true;
				}
        		iter++;
    		}
			if (!have_parent)
			{
				printf("ERROR : can't find parent for %s\n",pd.get_str(secname, "name"));
				exit(-1);
			}
		}

		if (!strcmp(pd.get_str(secname, "name"), "Earth")) earth = p;
		if (!strcmp(pd.get_str(secname, "name"), "Sun")) sun = p;
		if (!strcmp(pd.get_str(secname, "name"), "Moon")) moon = p;

		system_planets.push_back(p);
	}

}

// Compute the position for every elements of the solar system.
// The order is not important since the position is computed relatively to the mother body
void SolarSystem::compute_positions(double date)
{
    vector<planet*>::iterator iter = system_planets.begin();
    while (iter != system_planets.end())
    {
        (*iter)->compute_position(date);
        iter++;
    }
}

// Compute the transformation matrix for every elements of the solar system.
// The elements have to be ordered hierarchically, eg. it's important to compute earth before moon.
void SolarSystem::compute_trans_matrices(double date)
{
    vector<planet*>::iterator iter = system_planets.begin();
    while (iter != system_planets.end())
    {
        (*iter)->compute_trans_matrix(date);
        iter++;
    }
}

// Draw all the elements of the solar system
void SolarSystem::draw(int hint_ON, draw_utility * du, navigator * nav)
{
	// We are supposed to be in heliocentric coordinate already so no matrix change

    glEnable(GL_TEXTURE_2D);
	glEnable(GL_CULL_FACE);
	glDisable(GL_BLEND);
	glDisable(GL_LIGHTING);

	// Set the light parameters taking sun as the light source
    float tmp[4] = {0,0,0,0};
	float tmp2[4] = {0.05,0.05,0.05,0.05};
    float tmp3[4] = {2,2,2,2};
    float tmp4[4] = {1,1,1,1};
    glLightfv(GL_LIGHT0,GL_AMBIENT,tmp2);
    glLightfv(GL_LIGHT0,GL_DIFFUSE,tmp3);
    glLightfv(GL_LIGHT0,GL_SPECULAR,tmp);

    glMaterialfv(GL_FRONT,GL_AMBIENT ,tmp2);
    glMaterialfv(GL_FRONT,GL_DIFFUSE ,tmp4);
    glMaterialfv(GL_FRONT,GL_EMISSION ,tmp);
    glMaterialfv(GL_FRONT,GL_SHININESS ,tmp);
    glMaterialfv(GL_FRONT,GL_SPECULAR ,tmp);

	// Light pos in zero
	float zero4[4] = {0.,0.,0.,1.};
    glLightfv(GL_LIGHT0,GL_POSITION,zero4);

	glEnable(GL_LIGHT0);

    // Draw the elements
    vector<planet*>::iterator iter = system_planets.begin();
    while (iter != system_planets.end())
    {
        (*iter)->draw(hint_ON, du, nav);
        iter++;
    }

	glDisable(GL_LIGHTING);
    glDisable(GL_CULL_FACE);
}


// Search if any planet is close to position given in earth equatorial position and return the distance
planet* SolarSystem::search(Vec3d pos, navigator * nav)
{
    pos.normalize();
    planet * closest = NULL;
	double cos_angle_closest = 0.;
	static Vec3d equPos;

    vector<planet*>::iterator iter = system_planets.begin();
    while (iter != system_planets.end())
    {
        equPos = (*iter)->get_earth_equ_pos(nav);
		equPos.normalize();
    	double cos_ang_dist = equPos[0]*pos[0] + equPos[1]*pos[1] + equPos[2]*pos[2];
		if (cos_ang_dist>cos_angle_closest)
		{
			closest = *iter;
			cos_angle_closest = cos_ang_dist;
		}

        iter++;
    }

    if (cos_angle_closest>0.999)
    {
	    return closest;
    }
    else return NULL;
}



/*
	ganymede_orbit = new EllipticalOrbit(1070000/AU * (1.0 - 0.0015),
		0.0015, 0.195*M_PI/180., 119.841*M_PI/180., (188.831 - 119.841)*M_PI/180.,
		-188.831*M_PI/180., 7.154553, 2443000.00038375);
	io_orbit = new EllipticalOrbit(421600/AU * (1.0 - 0.0041),
		0.0041, 0.040*M_PI/180., 312.981*M_PI/180., (97.735 - 312.981)*M_PI/180.,
		-97.735*M_PI/180., 1.769138, 2443000.00038375);


    sun_map = new s_texture("sun",TEX_LOAD_TYPE_PNG_SOLID);
    earth_map = new s_texture("earthmap",TEX_LOAD_TYPE_PNG_SOLID);
    moon_map = new s_texture("lune",TEX_LOAD_TYPE_PNG_SOLID);
    mercury_map = new s_texture("mercury",TEX_LOAD_TYPE_PNG_SOLID);
    venus_map = new s_texture("venus",TEX_LOAD_TYPE_PNG_SOLID);
    jupiter_map = new s_texture("jupiter",TEX_LOAD_TYPE_PNG_SOLID);
    ganymede_map = new s_texture("ganymede",TEX_LOAD_TYPE_PNG_SOLID);
    io_map = new s_texture("io",TEX_LOAD_TYPE_PNG_SOLID);
    mars_map = new s_texture("mars",TEX_LOAD_TYPE_PNG_SOLID);
    saturn_map = new s_texture("saturn",TEX_LOAD_TYPE_PNG_SOLID);
    uranus_map = new s_texture("uranus",TEX_LOAD_TYPE_PNG_SOLID);
    neptune_map = new s_texture("neptune",TEX_LOAD_TYPE_PNG_SOLID);
    pluto_map = new s_texture("lune",TEX_LOAD_TYPE_PNG_SOLID);

	small_halo = new s_texture("star16x16");
	sun_halo = new s_texture("halo");

	saturn_ring_tex = new s_texture("saturn_rings");
	saturn_ring = new ring(140000./AU,saturn_ring_tex);

	Sun = new sun_planet("Sun", NO_HALO, 696000./AU, vec3_t(1.,1.,0.8), sun_map, NULL, sun_halo);
	Moon = new planet("Moon", NO_HALO, 1738./AU, vec3_t(1.,1.,1.), moon_map, NULL, get_lunar_geo_posn);
	Earth = new planet("Earth", NO_HALO, 6378.1/AU, vec3_t(1.,1.,1.), earth_map,small_halo, get_earth_helio_coords);
	Mercury = new planet("Mercury", NO_HALO, 2439.7/AU, vec3_t(1.,1.,1.), mercury_map, small_halo, get_mercury_helio_coords);
	Venus = new planet("Venus", NO_HALO, 6052./AU, vec3_t(1.,1.,1.), venus_map, small_halo, get_venus_helio_coords);
	Mars = new planet("Mars", NO_HALO, 3394./AU, vec3_t(1.,1.,1.), mars_map, small_halo, get_mars_helio_coords);
	Jupiter = new planet("Jupiter", NO_HALO, 71398./AU, vec3_t(1.,1.,1.), jupiter_map, small_halo, get_jupiter_helio_coords);
	Ganymede = new planet("Ganymede", NO_HALO, 2634./AU, vec3_t(1.,1.,1.), ganymede_map, small_halo, get_ganymede_helio_coords);
	Io = new planet("Io", NO_HALO, 2634./AU, vec3_t(1.,1.,1.), io_map, small_halo, get_io_helio_coords);
	Saturn = new ring_planet("Saturn", NO_HALO, 60330./AU, vec3_t(1.,1.,1.), saturn_map, small_halo, get_saturn_helio_coords, saturn_ring);
	Uranus = new planet("Uranus", NO_HALO, 26200./AU, vec3_t(1.,1.,1.), uranus_map, small_halo, get_uranus_helio_coords);
	Neptune = new planet("Neptune", NO_HALO, 25225./AU, vec3_t(1.,1.,1.), neptune_map, small_halo, get_neptune_helio_coords);
	Pluto = new planet("Pluto", NO_HALO, 1137./AU, vec3_t(1.,1.,1.), pluto_map, small_halo, get_pluto_helio_coords);


    Mercury->set_rotation_elements(1407.509405/24., 291.20, J2000, -7.01*M_PI/180., 48.33167*M_PI/180., 0);
    Venus->set_rotation_elements(5832.479839/24., 137.45, J2000, -178.78*M_PI/180., 76.681*M_PI/180., 0);
    Earth->set_rotation_elements(23.9344694/24., 280.5, J2000, 23.438855*M_PI/180., -11.2606423*M_PI/180., 0);
	Moon->set_rotation_elements((27.321661*24)/24., 300, J2000, 23.45*M_PI/180., 0., 0);
    Mars->set_rotation_elements(24.622962/24., 136.005, J2000, -26.72*M_PI/180., 82.91*M_PI/180., 0);
    Jupiter->set_rotation_elements(9.927953/24., 16, J2000, -2.222461*M_PI/180., -22.203*M_PI/180., 0);
    Ganymede->set_rotation_elements(7.154553/24., 262.1, J2000, -0.1*M_PI/180., 161.6*M_PI/180., 0);
    Io->set_rotation_elements(1.769138/24., 220.8, J2000, 0., 0., 0);

	Saturn->set_rotation_elements(10.65622/24., 358.922, J2000, -2.222461*M_PI/180., 169.530*M_PI/180., 0);
    Uranus->set_rotation_elements(17.24/24., 331.18, J2000, -97.81*M_PI/180., 167.76*M_PI/180., 0);
    Neptune->set_rotation_elements(16.11/24., 228.65, J2000, -28.03*M_PI/180., 49.235*M_PI/180., 0);
    Pluto->set_rotation_elements(248.54/24., 320.75, J2000, -115.60*M_PI/180., 228.34*M_PI/180., 0);


	//Sun->addSatellite(Mercury);
	//Sun->addSatellite(Venus);
	Sun->addSatellite(Earth);
	Earth->addSatellite(Moon);
	Sun->addSatellite(Mars);
	Sun->addSatellite(Jupiter);
	Jupiter->addSatellite(Io);
	Jupiter->addSatellite(Ganymede);
	Sun->addSatellite(Saturn);
	Sun->addSatellite(Uranus);
	Sun->addSatellite(Neptune);
	Sun->addSatellite(Pluto);





// Text

// Planet map textures
static s_texture * sun_map;
static s_texture * mercury_map;
static s_texture * venus_map;
static s_texture * earth_map;
static s_texture * moon_map;
static s_texture * mars_map;
static s_texture * jupiter_map;
static s_texture * ganymede_map;
static s_texture * io_map;
static s_texture * saturn_map;
static s_texture * saturn_ring_tex; // Saturn rings
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
planet * Ganymede;
planet * Io;
planet * Saturn;
planet * Uranus;
planet * Neptune;
planet * Pluto;

// Rings
ring * saturn_ring;

// Elliptical orbits in testing
EllipticalOrbit * io_orbit;
void get_io_helio_coords(double JD, double * X, double * Y, double * Z)
{
	io_orbit->positionAtTime(JD, X, Y, Z);
}
EllipticalOrbit * ganymede_orbit;
void get_ganymede_helio_coords(double JD, double * X, double * Y, double * Z)
{
	ganymede_orbit->positionAtTime(JD, X, Y, Z);
}

*/

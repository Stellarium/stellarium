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

using namespace std;

#include <algorithm>
#include <iostream>
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
    for(vector<planet*>::iterator iter = system_planets.begin(); iter != system_planets.end(); ++iter)
    {
        if (*iter) delete *iter;
		*iter = NULL;
    }
    for(vector<EllipticalOrbit*>::iterator iter = ell_orbits.begin(); iter != ell_orbits.end(); ++iter)
    {
        if (*iter) delete *iter;
		*iter = NULL;
    }
	sun = NULL;
	moon = NULL;
	earth = NULL;

	if (planet_name_font) delete planet_name_font;
}


// Init and load the solar system data
void SolarSystem::init(const string& font_fileName, const string& planetfile)
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
void SolarSystem::load(const string& planetfile)
{
	printf("Loading Solar System data...\n");
	init_parser pd;	// The planet data ini file parser
	pd.load(planetfile);

	int nbSections = pd.get_nsec();
	for (int i = 0;i<nbSections;++i)
	{
		string secname = pd.get_secname(i);
		string tname = pd.get_str(secname, "name");
		string funcname = pd.get_str(secname, "coord_func");

		pos_func_type posfunc;
		EllipticalOrbit* orb = NULL;

		if (funcname=="ell_orbit")
		{
			// Read the orbital elements
			double period = pd.get_double(secname, "orbit_Period");
			double epoch = pd.get_double(secname, "orbit_Epoch",J2000);
			double semi_major_axis = pd.get_double(secname, "orbit_SemiMajorAxis")/AU;
			double eccentricity = pd.get_double(secname, "orbit_Eccentricity");
			double inclination = pd.get_double(secname, "orbit_Inclination")*M_PI/180.;
			double ascending_node = pd.get_double(secname, "orbit_AscendingNode")*M_PI/180.;
			double long_of_pericenter = pd.get_double(secname, "orbit_LongOfPericenter")*M_PI/180.;
			double mean_longitude = pd.get_double(secname, "orbit_MeanLongitude")*M_PI/180.;

			double arg_of_pericenter = long_of_pericenter - ascending_node;
			double anomaly_at_epoch = mean_longitude - (arg_of_pericenter + ascending_node);
			double pericenter_distance = semi_major_axis * (1.0 - eccentricity);

			// Create an elliptical orbit
			orb = new EllipticalOrbit(pericenter_distance, eccentricity, inclination, ascending_node,
				arg_of_pericenter, anomaly_at_epoch, period, epoch);
			ell_orbits.push_back(orb);

			posfunc = pos_func_type(orb, &EllipticalOrbit::positionAtTimev);
		}

		if (funcname=="sun_special")
			posfunc = pos_func_type(get_sun_helio_coordsv);

		if (funcname=="mercury_special")
			posfunc = pos_func_type(get_mercury_helio_coordsv);

		if (funcname=="venus_special")
			posfunc = pos_func_type(get_venus_helio_coordsv);

		if (funcname=="earth_special")
			posfunc = pos_func_type(get_earth_helio_coordsv);

		if (funcname=="lunar_special")
			posfunc = pos_func_type(get_lunar_geo_posnv);

		if (funcname=="mars_special")
			posfunc = pos_func_type(get_mars_helio_coordsv);

		if (funcname=="jupiter_special")
			posfunc = pos_func_type(get_jupiter_helio_coordsv);

		if (funcname=="saturn_special")
			posfunc = pos_func_type(get_saturn_helio_coordsv);

		if (funcname=="uranus_special")
			posfunc = pos_func_type(get_uranus_helio_coordsv);

		if (funcname=="neptune_special")
			posfunc = pos_func_type(get_neptune_helio_coordsv);

		if (funcname=="pluto_special")
			posfunc = pos_func_type(get_pluto_helio_coordsv);


		if (posfunc.empty())
		{
			cout << "ERROR : can't find posfunc %s for " << funcname << tname << endl;
			exit(-1);
		}

		// Create the planet and add it to the list
		planet* p = new planet(tname, pd.get_boolean(secname, "halo"),
			pd.get_boolean(secname, "lightning"), pd.get_double(secname, "radius")/AU,
			str_to_vec3f(pd.get_str(secname, "color").c_str()), pd.get_double(secname, "albedo"),
			pd.get_str(secname, "tex_map"), pd.get_str(secname, "tex_halo"), posfunc);

		string str_parent = pd.get_str(secname, "parent");

		if (str_parent!="none")
		{
			// Look in the other planets the one named with str_parent
			bool have_parent = false;
    		vector<planet*>::iterator iter = system_planets.begin();
    		while (iter != system_planets.end())
    		{
        		if ((*iter)->get_name()==str_parent)
				{
					(*iter)->add_satellite(p);
					have_parent = true;
				}
        		iter++;
    		}
			if (!have_parent)
			{
				cout << "ERROR : can't find parent for " << tname << endl;
				exit(-1);
			}
		}

		if (tname=="Earth") earth = p;
		if (tname=="Sun") sun = p;
		if (tname=="Moon") moon = p;

		p->set_rotation_elements(
			pd.get_double(secname, "rot_periode", pd.get_double(secname, "orbit_Period", 24.))/24.,
			pd.get_double(secname, "rot_rotation_offset",0.),
			pd.get_double(secname, "rot_epoch", J2000),
			pd.get_double(secname, "rot_obliquity",0.)*M_PI/180.,
			pd.get_double(secname, "rot_equator_ascending_node",0.)*M_PI/180.,
			pd.get_double(secname, "rot_precession_rate",0.) );

		if (pd.get_boolean(secname, "rings", 0))
		{
			ring* r = new ring(pd.get_double(secname, "ring_size")/AU, pd.get_str(secname, "tex_ring"));
			p->set_rings(r);
		}

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
// We are supposed to be in heliocentric coordinate
void SolarSystem::draw(int hint_ON, Projector * prj, const navigator * nav, const tone_reproductor* eye, bool _gravity_label)
{
	planet::set_gravity_label_flag(_gravity_label);

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

	// Light pos in zero (sun)
	nav->switch_to_heliocentric();
    glLightfv(GL_LIGHT0,GL_POSITION,Vec4f(0.f,0.f,0.f,1.f));
	glEnable(GL_LIGHT0);

	// Compute each planet distance to the observer
	Vec3d obs_helio_pos = nav->get_observer_helio_pos();
    vector<planet*>::iterator iter = system_planets.begin();
    while (iter != system_planets.end())
    {
        (*iter)->compute_distance(obs_helio_pos);
        ++iter;
    }

	// And sort them from the furthest to the closest
	sort(system_planets.begin(),system_planets.end(),bigger_distance());

    // Draw the elements
	iter = system_planets.begin();
    while (iter != system_planets.end())
    {
        if (*iter!=earth) (*iter)->draw(hint_ON, prj, nav, eye);
        ++iter;
    }
}


// Search if any planet is close to position given in earth equatorial position and return the distance
planet* SolarSystem::search(Vec3d pos, const navigator * nav)
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


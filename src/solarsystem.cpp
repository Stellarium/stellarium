/*
 * Stellarium
 * Copyright (C) 2002 Fabien Chereau
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
#include <string>
#include "solarsystem.h"
#include "s_texture.h"
#include "stellplanet.h"
#include "orbit.h"
#include "stellarium.h"
#include "init_parser.h"

#include "draw.h"

SolarSystem::SolarSystem() : sun(NULL), moon(NULL), earth(NULL), tex_earth_shadow(NULL)
{
	// Generate Gettext strings for traduction
	char * tmp = NULL;
	tmp = gettext_noop("Sun");
	tmp = gettext_noop("Mercury");
	tmp = gettext_noop("Venus");
	tmp = gettext_noop("Earth");
	tmp = gettext_noop("Moon");
	tmp = gettext_noop("Mars");
	tmp = gettext_noop("Deimos");
	tmp = gettext_noop("Phobos");
	tmp = gettext_noop("Jupiter");
	tmp = gettext_noop("Io");
	tmp = gettext_noop("Europa");
	tmp = gettext_noop("Ganymede");
	tmp = gettext_noop("Callisto");
	tmp = gettext_noop("Saturn");
	tmp = gettext_noop("Mimas");
	tmp = gettext_noop("Enceladus");
	tmp = gettext_noop("Tethys");
	tmp = gettext_noop("Dione");
	tmp = gettext_noop("Rhea");
	tmp = gettext_noop("Titan");
	tmp = gettext_noop("Hyperion");
	tmp = gettext_noop("Iapetus");
	tmp = gettext_noop("Phoebe");
	tmp = gettext_noop("Neptune");
	tmp = gettext_noop("Uranus");
	tmp = gettext_noop("Pluto");
	tmp = gettext_noop("Charon");	
}

void SolarSystem::set_font(float font_size, const string& font_name)
{
	planet_name_font = new s_font(font_size, font_name);
	if (!planet_name_font)
	{
		printf("Can't create planet_name_font\n");
		exit(-1);
	}
	Planet::set_font(planet_name_font);
}

SolarSystem::~SolarSystem()
{
    for(vector<Planet*>::iterator iter = system_planets.begin(); iter != system_planets.end(); ++iter)
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
	if(tex_earth_shadow) delete tex_earth_shadow;
}


// Init and load the solar system data
void SolarSystem::load(const string& planetfile)
{
	cout << _("Loading Solar System data...");
	InitParser pd;	// The Planet data ini file parser
	pd.load(planetfile);

	int nbSections = pd.get_nsec();
	for (int i = 0;i<nbSections;++i)
	{
		string secname = pd.get_secname(i);
		string tname = gettext(pd.get_str(secname, "name").c_str());
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

		if (funcname=="europa_special")
			posfunc = pos_func_type(get_europa_parent_coordsv);

		if (funcname=="calisto_special")
			posfunc = pos_func_type(get_callisto_parent_coordsv);

		if (funcname=="io_special")
			posfunc = pos_func_type(get_io_parent_coordsv);

		if (funcname=="ganymede_special")
			posfunc = pos_func_type(get_ganymede_parent_coordsv);

		if (funcname=="saturn_special")
			posfunc = pos_func_type(get_saturn_helio_coordsv);

		if (funcname=="mimas_special")
			posfunc = pos_func_type(get_mimas_parent_coordsv);

		if (funcname=="enceladus_special")
			posfunc = pos_func_type(get_enceladus_parent_coordsv);

		if (funcname=="tethys_special")
			posfunc = pos_func_type(get_tethys_parent_coordsv);

		if (funcname=="dione_special")
			posfunc = pos_func_type(get_dione_parent_coordsv);

		if (funcname=="rhea_special")
			posfunc = pos_func_type(get_rhea_parent_coordsv);

		if (funcname=="titan_special")
			posfunc = pos_func_type(get_titan_parent_coordsv);

		if (funcname=="iapetus_special")
			posfunc = pos_func_type(get_iapetus_parent_coordsv);

		if (funcname=="hyperion_special")
			posfunc = pos_func_type(get_hyperion_parent_coordsv);

		if (funcname=="uranus_special")
			posfunc = pos_func_type(get_uranus_helio_coordsv);

		if (funcname=="neptune_special")
			posfunc = pos_func_type(get_neptune_helio_coordsv);

		if (funcname=="pluto_special")
			posfunc = pos_func_type(get_pluto_helio_coordsv);


		if (posfunc.empty())
		{
			cout << "ERROR : can't find posfunc " << funcname << " for " << tname << endl;
			exit(-1);
		}

		// Create the Planet and add it to the list
		Planet* p = new Planet(secname, tname, pd.get_boolean(secname, "halo"),
			pd.get_boolean(secname, "lightning"), pd.get_double(secname, "radius")/AU,
			StelUtility::str_to_vec3f(pd.get_str(secname, "color").c_str()), pd.get_double(secname, "albedo"),
			pd.get_str(secname, "tex_map"), pd.get_str(secname, "tex_halo"), posfunc);

		string str_parent = pd.get_str(secname, "parent");

		if (str_parent!="none")
		{
			// Look in the other planets the one named with str_parent
			bool have_parent = false;
    		vector<Planet*>::iterator iter = system_planets.begin();
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

		if (secname=="earth")
			earth = p;
		if (secname=="sun") sun = p;
		if (secname=="moon") moon = p;

		p->set_rotation_elements(
			pd.get_double(secname, "rot_periode", pd.get_double(secname, "orbit_Period", 24.))/24.,
			pd.get_double(secname, "rot_rotation_offset",0.),
			pd.get_double(secname, "rot_epoch", J2000),
			pd.get_double(secname, "rot_obliquity",0.)*M_PI/180.,
			pd.get_double(secname, "rot_equator_ascending_node",0.)*M_PI/180.,
			pd.get_double(secname, "rot_precession_rate",0.),
			pd.get_double(secname, "sidereal_period",0.) );
		

		if (pd.get_boolean(secname, "rings", 0))
		{
			Ring* r = new Ring(pd.get_double(secname, "ring_size")/AU, pd.get_str(secname, "tex_ring"));
			p->set_rings(r);
		}

		string bighalotexfile = pd.get_str(secname, "tex_big_halo", "");
		if (!bighalotexfile.empty())
		{
			p->set_big_halo(bighalotexfile);
			p->set_halo_size(pd.get_double(secname, "big_halo_size", 50.f));
		}

		system_planets.push_back(p);
	}

	// special case: load earth shadow texture
	tex_earth_shadow = new s_texture("earth-shadow", TEX_LOAD_TYPE_PNG_ALPHA);

	cout << "(loaded)" << endl;
}

// Compute the position for every elements of the solar system.
// The order is not important since the position is computed relatively to the mother body
void SolarSystem::compute_positions(double date)
{
    vector<Planet*>::iterator iter = system_planets.begin();
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
    vector<Planet*>::iterator iter = system_planets.begin();
    while (iter != system_planets.end())
    {
        (*iter)->compute_trans_matrix(date);
        iter++;
    }
}

// Draw all the elements of the solar system
// We are supposed to be in heliocentric coordinate
void SolarSystem::draw(Planet* selected, bool hint_ON, Projector * prj, const Navigator * nav, 
					   const tone_reproductor* eye, bool _gravity_label, 
					   bool flag_point, bool flag_orbits, bool flag_trails)
{

	// if a Planet is selected and orbits are on, fade out non-selected ones
	bool orb;
	vector<Planet*>::iterator iter = system_planets.begin();
	if(flag_orbits && selected != NULL && selected->get_name() != "sun") {
		while (iter != system_planets.end()) {
			if((*iter)->get_name() == selected->get_name()) orb = 1;
			else orb = 0;
			(*iter)->show_orbit(orb);
			iter++;
		}
	} else {
		while (iter != system_planets.end()) {
			(*iter)->show_orbit(flag_orbits);
			iter++;
		}
	}


	Planet::set_gravity_label_flag(_gravity_label);

	// Set the light parameters taking sun as the light source
    float tmp[4] = {0,0,0,0};
	float tmp2[4] = {0.03,0.03,0.03,0.03};
    float tmp3[4] = {5,5,5,1};
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

	// Compute each Planet distance to the observer
	Vec3d obs_helio_pos = nav->get_observer_helio_pos();
    iter = system_planets.begin();
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

		if(*iter==moon && near_lunar_eclipse(nav, prj)) {

			// TODO: moon magnitude label during eclipse isn't accurate... 

			// special case to update stencil buffer for drawing lunar eclipses
			glClear(GL_STENCIL_BUFFER_BIT);
			glClearStencil(0x0);

			glStencilFunc(GL_ALWAYS, 0x1, 0x1);
			glStencilOp(GL_ZERO, GL_REPLACE, GL_REPLACE);

			(*iter)->draw(hint_ON, prj, nav, eye, flag_point, flag_trails, 1);

		} else if (*iter!=earth) (*iter)->draw(hint_ON, prj, nav, eye, flag_point, flag_trails, 0);
        
		++iter;
    }

    glDisable(GL_LIGHT0);


    // special case: draw earth shadow over moon if appropriate
    // stencil buffer is set up in moon drawing above
    draw_earth_shadow(nav, prj);

}

Planet* SolarSystem::search(string planet_name) {

	// side effect - bad?
	transform(planet_name.begin(), planet_name.end(), planet_name.begin(), ::tolower);

	vector<Planet*>::iterator iter = system_planets.begin();
	while (iter != system_planets.end()) {

		if( (*iter)->get_name() == planet_name ) return (*iter);  // also check standard ini file names 
		++iter;
	}
	return NULL;
}

Planet* SolarSystem::searchByCommonNames(string planetCommonName) {

	// side effect - bad?
	transform(planetCommonName.begin(), planetCommonName.end(), planetCommonName.begin(), ::tolower);

	vector<Planet*>::iterator iter = system_planets.begin();
	while (iter != system_planets.end()) {
		
		if( (*iter)->get_common_name() == planetCommonName ) return (*iter);  // also check standard ini file names 
		++iter;
	}
	return NULL;
}

// Search if any Planet is close to position given in earth equatorial position and return the distance
Planet* SolarSystem::search(Vec3d pos, const Navigator * nav, const Projector * prj)
{
    pos.normalize();
    Planet * closest = NULL;
	double cos_angle_closest = 0.;
	static Vec3d equPos;

    vector<Planet*>::iterator iter = system_planets.begin();
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

// Return a stl vector containing the planets located inside the lim_fov circle around position v
vector<StelObject*> SolarSystem::search_around(Vec3d v, double lim_fov, const Navigator * nav, const Projector * prj)
{
	vector<StelObject*> result;
    v.normalize();
	double cos_lim_fov = cos(lim_fov * M_PI/180.);
	static Vec3d equPos;

    vector<Planet*>::iterator iter = system_planets.begin();
    while (iter != system_planets.end())
    {
        equPos = (*iter)->get_earth_equ_pos(nav);
		equPos.normalize();
		if (equPos[0]*v[0] + equPos[1]*v[1] + equPos[2]*v[2]>=cos_lim_fov)
		{
			result.push_back(*iter);
		}
        iter++;
    }
	return result;
}

vector<string> SolarSystem::getNames(void)
{
    vector<string> names;
	vector < Planet * >::iterator iter;

	for (iter = system_planets.begin(); iter != system_planets.end(); ++iter)
        names.push_back((*iter)->get_common_name());
    return names;
}

void SolarSystem::update_trails(const Navigator* nav) {

  vector<Planet*>::iterator iter;      
  for( iter = system_planets.begin(); iter < system_planets.end(); iter++ ) {
    (*iter)->update_trail(nav);
  }

}

void SolarSystem::start_trails(void) {

  vector<Planet*>::iterator iter;      
  for( iter = system_planets.begin(); iter < system_planets.end(); iter++ ) {
    (*iter)->start_trail();
  }

}

void SolarSystem::end_trails(void) {

  vector<Planet*>::iterator iter;      
  for( iter = system_planets.begin(); iter < system_planets.end(); iter++ ) {
    (*iter)->end_trail();
  }

}

// draws earth shadow overlapping the moon using stencil buffer
// umbra and penumbra are sized separately for accuracy

void SolarSystem::draw_earth_shadow(const Navigator * nav, Projector * prj) {

    Vec3d e = get_earth()->get_ecliptic_pos();
    Vec3d m = get_moon()->get_ecliptic_pos();  // relative to earth
    Vec3d mh = get_moon()->get_heliocentric_ecliptic_pos();  // relative to sun
    float mscale = get_moon()->get_sphere_scale();

    // shadow location at earth + moon distance along earth vector from sun
    Vec3d en = e;
    en.normalize();
    Vec3d shadow = en * (e.length() + m.length());

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor3f(1,1,1);
    
    // find shadow radii in AU
    double r_penumbra = shadow.length()*702378.1/AU/e.length() - 696000/AU;
    double r_umbra = 6378.1/AU - m.length()*(689621.9/AU/e.length());

    // find vector orthogonal to sun-earth vector using cross product with
    // a non-parallel vector
    Vec3d rpt = shadow^Vec3d(0,0,1);
    rpt.normalize();
    Vec3d upt = rpt*r_umbra*mscale*1.02;  // point on umbra edge
    rpt *= r_penumbra*mscale;  // point on penumbra edge

    // modify shadow location for scaled moon
    Vec3d mdist = shadow - mh;
    if(mdist.length() > r_penumbra + 2000/AU) return;   // not visible so don't bother drawing

    shadow = mh + mdist*mscale;
    r_penumbra *= mscale;

    nav->switch_to_heliocentric();
    glEnable(GL_STENCIL_TEST);
	glStencilFunc(GL_EQUAL, 0x1, 0x1);
	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
	
	Mat4d mat = nav->get_helio_to_eye_mat();

    // shadow radial texture
    glBindTexture(GL_TEXTURE_2D, tex_earth_shadow->getID());

	Vec3d r, s;

    // umbra first
    glBegin(GL_TRIANGLE_FAN);
	glTexCoord2f(0,0);
	prj->sVertex3( shadow[0],shadow[1], shadow[2], mat);

    for (int i=0; i<=100; i++) {
		r = Mat4d::rotation(shadow, 2*M_PI*i/100.) * upt;
		s = shadow + r;

		glTexCoord2f(0.6,0);  // position in texture of umbra edge
		prj->sVertex3( s[0],s[1], s[2], mat);
    }
    glEnd();


    // now penumbra
	Vec3d u, sp;
    glBegin(GL_TRIANGLE_STRIP);
    for (int i=0; i<=100; i++) {
		r = Mat4d::rotation(shadow, 2*M_PI*i/100.) * rpt;
		u = Mat4d::rotation(shadow, 2*M_PI*i/100.) * upt;
		s = shadow + r;
		sp = shadow + u;

		glTexCoord2f(0.6,0);
		prj->sVertex3( sp[0],sp[1], sp[2], mat);

		glTexCoord2f(1.,0);  // position in texture of umbra edge
		prj->sVertex3( s[0],s[1], s[2], mat);
    }
    glEnd();
	
    glDisable(GL_STENCIL_TEST);

}


void SolarSystem::set_object_scale(float scale) {

	Planet::set_object_scale(scale);
	
}


void SolarSystem::update(int delta_time, Navigator* nav) {
    vector<Planet*>::iterator iter = system_planets.begin();
    while (iter != system_planets.end())
    {
        (*iter)->update_trail(nav);
        (*iter)->update(delta_time);
        iter++;
    }


}


// is a lunar eclipse close at hand?
bool SolarSystem::near_lunar_eclipse(const Navigator * nav, Projector *prj) {

	// TODO: could replace with simpler test

    Vec3d e = get_earth()->get_ecliptic_pos();
    Vec3d m = get_moon()->get_ecliptic_pos();  // relative to earth
    Vec3d mh = get_moon()->get_heliocentric_ecliptic_pos();  // relative to sun

    // shadow location at earth + moon distance along earth vector from sun
    Vec3d en = e;
    en.normalize();
    Vec3d shadow = en * (e.length() + m.length());

    // find shadow radii in AU
    double r_penumbra = shadow.length()*702378.1/AU/e.length() - 696000/AU;

    // modify shadow location for scaled moon
    Vec3d mdist = shadow - mh;
    if(mdist.length() > r_penumbra + 2000/AU) return 0;   // not visible so don't bother drawing

	return 1;

}

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

SolarSystem::SolarSystem()
            :sun(NULL),moon(NULL),earth(NULL),
             moonScale(1.),tex_earth_shadow(NULL),
             flagOrbits(false),flag_light_travel_time(false) {
}

void SolarSystem::setFont(float font_size, const string& font_name)
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
	cout << "Loading Solar System data...";
	InitParser pd;	// The Planet data ini file parser
	pd.load(planetfile);

	int nbSections = pd.get_nsec();
	for (int i = 0;i<nbSections;++i)
	{
		const string secname = pd.get_secname(i);
		const string englishName = pd.get_str(secname, "name");

		const string str_parent = pd.get_str(secname, "parent");
		Planet *parent = NULL;

		if (str_parent!="none")
		{
			// Look in the other planets the one named with str_parent
			vector<Planet*>::iterator iter = system_planets.begin();
			while (iter != system_planets.end())
			{
				if ((*iter)->getEnglishName()==str_parent)
				{
					parent = (*iter);
				}
				iter++;
			}
			if (parent == NULL)
			{
				cout << "ERROR : can't find parent for " << englishName << endl;
				exit(-1);
			}
		}

		const string funcname = pd.get_str(secname, "coord_func");

		pos_func_type posfunc;
		OsulatingFunctType *osculating_func = 0;
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

			  // when the parent is the sun use ecliptic rathe than sun equator:
			const double parent_rot_obliquity = parent->get_parent()
			                                  ? parent->getRotObliquity()
			                                  : 0.0;
			const double parent_rot_asc_node = parent->get_parent()
			                                  ? parent->getRotAscendingnode()
			                                  : 0.0;
			// Create an elliptical orbit
			orb = new EllipticalOrbit(pericenter_distance,
			                          eccentricity,
			                          inclination,
			                          ascending_node,
			                          arg_of_pericenter,
			                          anomaly_at_epoch,
			                          period,
			                          epoch,
			                          parent_rot_obliquity,
			                          parent_rot_asc_node);
			ell_orbits.push_back(orb);

			posfunc = pos_func_type(orb, &EllipticalOrbit::positionAtTimevInVSOP87Coordinates);
		}

		if (funcname=="sun_special")
			posfunc = pos_func_type(get_sun_helio_coordsv);

		if (funcname=="mercury_special") {
			posfunc = pos_func_type(get_mercury_helio_coordsv);
			osculating_func = &get_mercury_helio_osculating_coords;
		}
        
		if (funcname=="venus_special") {
			posfunc = pos_func_type(get_venus_helio_coordsv);
			osculating_func = &get_venus_helio_osculating_coords;
		}

		if (funcname=="earth_special") {
			posfunc = pos_func_type(get_earth_helio_coordsv);
			osculating_func = &get_earth_helio_osculating_coords;
		}

		if (funcname=="lunar_special")
			posfunc = pos_func_type(get_lunar_parent_coordsv);

		if (funcname=="mars_special") {
			posfunc = pos_func_type(get_mars_helio_coordsv);
			osculating_func = &get_mars_helio_osculating_coords;
		}

		if (funcname=="phobos_special")
			posfunc = pos_func_type(get_phobos_parent_coordsv);

		if (funcname=="deimos_special")
			posfunc = pos_func_type(get_deimos_parent_coordsv);

		if (funcname=="jupiter_special") {
			posfunc = pos_func_type(get_jupiter_helio_coordsv);
			osculating_func = &get_jupiter_helio_osculating_coords;
		}

		if (funcname=="europa_special")
			posfunc = pos_func_type(get_europa_parent_coordsv);

		if (funcname=="calisto_special")
			posfunc = pos_func_type(get_callisto_parent_coordsv);

		if (funcname=="io_special")
			posfunc = pos_func_type(get_io_parent_coordsv);

		if (funcname=="ganymede_special")
			posfunc = pos_func_type(get_ganymede_parent_coordsv);

		if (funcname=="saturn_special") {
			posfunc = pos_func_type(get_saturn_helio_coordsv);
			osculating_func = &get_saturn_helio_osculating_coords;
		}

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

		if (funcname=="uranus_special") {
			posfunc = pos_func_type(get_uranus_helio_coordsv);
			osculating_func = &get_uranus_helio_osculating_coords;
		}

		if (funcname=="miranda_special")
			posfunc = pos_func_type(get_miranda_parent_coordsv);

		if (funcname=="ariel_special")
			posfunc = pos_func_type(get_ariel_parent_coordsv);

		if (funcname=="umbriel_special")
			posfunc = pos_func_type(get_umbriel_parent_coordsv);

		if (funcname=="titania_special")
			posfunc = pos_func_type(get_titania_parent_coordsv);

		if (funcname=="oberon_special")
			posfunc = pos_func_type(get_oberon_parent_coordsv);

		if (funcname=="neptune_special") {
			posfunc = pos_func_type(get_neptune_helio_coordsv);
			osculating_func = &get_neptune_helio_osculating_coords;
		}

		if (funcname=="pluto_special")
			posfunc = pos_func_type(get_pluto_helio_coordsv);


		if (posfunc.empty())
		{
			cout << "ERROR : can't find posfunc " << funcname << " for " << englishName << endl;
			exit(-1);
		}

		// Create the Planet and add it to the list
		Planet* p = new Planet(parent,
                               englishName,
                               pd.get_boolean(secname, "halo"),
                               pd.get_boolean(secname, "lighting"),
                               pd.get_double(secname, "radius")/AU,
                               pd.get_double(secname, "oblateness", 0.0),
                               StelUtility::str_to_vec3f(pd.get_str(secname, "color").c_str()),
                               pd.get_double(secname, "albedo"),
                               pd.get_str(secname, "tex_map"),
                               pd.get_str(secname, "tex_halo"),
                               posfunc,osculating_func,
                               pd.get_boolean(secname, "hidden", 0));

		if (secname=="earth") earth = p;
		if (secname=="sun") sun = p;
		if (secname=="moon") moon = p;

		p->set_rotation_elements(
		    pd.get_double(secname, "rot_periode", pd.get_double(secname, "orbit_Period", 24.))/24.,
		    pd.get_double(secname, "rot_rotation_offset",0.),
		    pd.get_double(secname, "rot_epoch", J2000),
		    pd.get_double(secname, "rot_obliquity",0.)*M_PI/180.,
		    pd.get_double(secname, "rot_equator_ascending_node",0.)*M_PI/180.,
		    pd.get_double(secname, "rot_precession_rate",0.)*M_PI/(180*36525),
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
	tex_earth_shadow = new s_texture("earth-shadow.png", TEX_LOAD_TYPE_PNG_ALPHA);

	cout << "(loaded)" << endl;
}

// Compute the position for every elements of the solar system.
// The order is not important since the position is computed relatively to the mother body
void SolarSystem::computePositions(double date,const Planet *home_planet) {
  if (flag_light_travel_time) {
    for (vector<Planet*>::const_iterator iter(system_planets.begin());
         iter!=system_planets.end();iter++) {
      (*iter)->computePositionWithoutOrbits(date);
    }
    const Vec3d home_pos(home_planet->get_heliocentric_ecliptic_pos());
    for (vector<Planet*>::const_iterator iter(system_planets.begin());
         iter!=system_planets.end();iter++) {
      const double light_speed_correction =
        ((*iter)->get_heliocentric_ecliptic_pos()-home_pos).length()
        * (149597870000.0 / (299792458.0 * 86400));
//cout << "SolarSystem::computePositions: " << (*iter)->getEnglishName()
//     << ": " << (86400*light_speed_correction) << endl;
      (*iter)->compute_position(date-light_speed_correction);
    }
  } else {
    for (vector<Planet*>::const_iterator iter(system_planets.begin());
         iter!=system_planets.end();iter++) {
      (*iter)->compute_position(date);
    }
  }
}

// Compute the transformation matrix for every elements of the solar system.
// The elements have to be ordered hierarchically, eg. it's important to compute earth before moon.
void SolarSystem::computeTransMatrices(double date,const Planet *home_planet) {
  if (flag_light_travel_time) {
    const Vec3d home_pos(home_planet->get_heliocentric_ecliptic_pos());
    for (vector<Planet*>::const_iterator iter(system_planets.begin());
         iter!=system_planets.end();iter++) {
      const double light_speed_correction =
        ((*iter)->get_heliocentric_ecliptic_pos()-home_pos).length()
        * (149597870000.0 / (299792458.0 * 86400));
      (*iter)->compute_trans_matrix(date-light_speed_correction);
    }
  } else {
    for (vector<Planet*>::const_iterator iter(system_planets.begin());
         iter!=system_planets.end();iter++) {
      (*iter)->compute_trans_matrix(date);
    }
  }
}

// Draw all the elements of the solar system
// We are supposed to be in heliocentric coordinate
void SolarSystem::draw(Projector * prj, const Navigator * nav, const ToneReproductor* eye, bool flag_point)
{
	// Set the light parameters taking sun as the light source
	const float zero[4] = {0,0,0,0};
	const float ambient[4] = {0.03,0.03,0.03,0.03};
	const float diffuse[4] = {1,1,1,1};
	glLightfv(GL_LIGHT0,GL_AMBIENT, ambient);
	glLightfv(GL_LIGHT0,GL_DIFFUSE, diffuse);
	glLightfv(GL_LIGHT0,GL_SPECULAR,zero);

	glMaterialfv(GL_FRONT,GL_AMBIENT,  ambient);
	glMaterialfv(GL_FRONT,GL_DIFFUSE,  diffuse);
	glMaterialfv(GL_FRONT,GL_EMISSION, zero);
	glMaterialfv(GL_FRONT,GL_SHININESS,zero);
	glMaterialfv(GL_FRONT,GL_SPECULAR, zero);

	// Light pos in zero (sun)
	nav->switch_to_heliocentric();
	glLightfv(GL_LIGHT0,GL_POSITION,Vec4f(0.f,0.f,0.f,1.f));
	glEnable(GL_LIGHT0);

	// Compute each Planet distance to the observer
	Vec3d obs_helio_pos = nav->get_observer_helio_pos();
	
	vector<Planet*>::iterator iter;
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

		if(*iter==moon && near_lunar_eclipse(nav, prj))
		{

			// TODO: moon magnitude label during eclipse isn't accurate...

			// special case to update stencil buffer for drawing lunar eclipses
			glClear(GL_STENCIL_BUFFER_BIT);
			glClearStencil(0x0);

			glStencilFunc(GL_ALWAYS, 0x1, 0x1);
			glStencilOp(GL_ZERO, GL_REPLACE, GL_REPLACE);

			(*iter)->draw(prj, nav, eye, flag_point, 1);

		}
		else (*iter)->draw(prj, nav, eye, flag_point, 0);

		++iter;
	}

	glDisable(GL_LIGHT0);


	// special case: draw earth shadow over moon if appropriate
	// stencil buffer is set up in moon drawing above
	draw_earth_shadow(nav, prj);

}

Planet* SolarSystem::searchByEnglishName(string planetEnglishName) const
{
//printf("SolarSystem::searchByEnglishName(\"%s\"): start\n",
//       planetEnglishName.c_str());
	// side effect - bad?
//	transform(planetEnglishName.begin(), planetEnglishName.end(), planetEnglishName.begin(), ::tolower);

	vector<Planet*>::const_iterator iter = system_planets.begin();
	while (iter != system_planets.end())
	{
//printf("SolarSystem::searchByEnglishName(\"%s\"): %s\n",
//       planetEnglishName.c_str(),
//       (*iter)->getEnglishName().c_str());
		if( (*iter)->getEnglishName() == planetEnglishName ) return (*iter);  // also check standard ini file names
		++iter;
	}
//printf("SolarSystem::searchByEnglishName(\"%s\"): not found\n",
//       planetEnglishName.c_str());
	return NULL;
}

Planet* SolarSystem::searchByNamesI18(wstring planetNameI18) const
{

	// side effect - bad?
	transform(planetNameI18.begin(), planetNameI18.end(), planetNameI18.begin(), ::tolower);

	vector<Planet*>::const_iterator iter = system_planets.begin();
	while (iter != system_planets.end())
	{

		if( (*iter)->getNameI18() == planetNameI18 ) return (*iter);  // also check standard ini file names
		++iter;
	}
	return NULL;
}

// Search if any Planet is close to position given in earth equatorial position and return the distance
Planet* SolarSystem::search(Vec3d pos, const Navigator * nav,
                            const Projector * prj) const
{
	pos.normalize();
	Planet * closest = NULL;
	double cos_angle_closest = 0.;
	static Vec3d equPos;

	vector<Planet*>::const_iterator iter = system_planets.begin();
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
vector<StelObject*> SolarSystem::search_around(Vec3d v,
                                               double lim_fov,
                                               const Navigator * nav,
                                               const Projector * prj) const
{
	vector<StelObject*> result;
	v.normalize();
	double cos_lim_fov = cos(lim_fov * M_PI/180.);
	static Vec3d equPos;

	vector<Planet*>::const_iterator iter = system_planets.begin();
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

//! @brief Update i18 names from english names according to passed translator
//! The translation is done using gettext with translated strings defined in translations.h
void SolarSystem::translateNames(Translator& trans)
{
	vector<Planet*>::iterator iter;
	for( iter = system_planets.begin(); iter < system_planets.end(); iter++ )
	{
		(*iter)->translateName(trans);
	}
}

vector<wstring> SolarSystem::getNamesI18(void)
{
	vector<wstring> names;
	vector < Planet * >::iterator iter;

	for (iter = system_planets.begin(); iter != system_planets.end(); ++iter)
		names.push_back((*iter)->getNameI18());
	return names;
}


// returns a newline delimited hash of localized:standard planet names for tui
wstring SolarSystem::getPlanetHashString(void)
{
	wostringstream oss;
	vector < Planet * >::iterator iter;

	for (iter = system_planets.begin(); iter != system_planets.end(); ++iter) {
		oss << Translator::globalTranslator.translate((*iter)->getEnglishName()) << "\n";
		oss << StelUtility::stringToWstring((*iter)->getEnglishName()) << "\n";
	}

	return oss.str();

}


void SolarSystem::updateTrails(const Navigator* nav)
{
	vector<Planet*>::iterator iter;
	for( iter = system_planets.begin(); iter < system_planets.end(); iter++ )
	{
		(*iter)->update_trail(nav);
	}
}

void SolarSystem::startTrails(bool b)
{
	vector<Planet*>::iterator iter;
	for( iter = system_planets.begin(); iter < system_planets.end(); iter++ )
	{
		(*iter)->startTrail(b);
	}
}

void SolarSystem::setFlagTrails(bool b)
{
	vector<Planet*>::iterator iter;
	for( iter = system_planets.begin(); iter < system_planets.end(); iter++ )
	{
		(*iter)->setFlagTrail(b);
	}
}

bool SolarSystem::getFlagTrails(void) const
{
	for (vector<Planet*>::const_iterator iter = system_planets.begin();
	     iter != system_planets.end(); iter++ ) {
		if ((*iter)->getFlagTrail()) return true;
	}
	return false;
}

void SolarSystem::setFlagHints(bool b)
{
	vector<Planet*>::iterator iter;
	for( iter = system_planets.begin(); iter < system_planets.end(); iter++ )
	{
		(*iter)->setFlagHints(b);
	}
}

bool SolarSystem::getFlagHints(void) const
{
	for (vector<Planet*>::const_iterator iter = system_planets.begin();
        iter != system_planets.end(); iter++ ) {
		if ((*iter)->getFlagHints()) return true;
	}
	return false;
}

void SolarSystem::setFlagOrbits(bool b)
{
	flagOrbits = b;
	if (!b || selected == NULL || selected->getEnglishName() == "Sun")
	{
		vector<Planet*>::iterator iter;
		for( iter = system_planets.begin(); iter < system_planets.end(); iter++ )
		{
			(*iter)->setFlagOrbits(b);
		}
	}
	else
	{
		// if a Planet is selected and orbits are on, fade out non-selected ones
		vector<Planet*>::iterator iter;
		for( iter = system_planets.begin(); iter < system_planets.end(); iter++ )
		{
			if((*iter)!=selected) (*iter)->setFlagOrbits(false);
			else (*iter)->setFlagOrbits(b);
		}		
	}
}


void SolarSystem::setSelected(Planet * p)
{
	selected = p;
	// Undraw other objects hints, orbit, trails etc..
	setFlagHints(getFlagHints());
	setFlagOrbits(getFlagOrbits());
	setFlagTrails(getFlagTrails());
}

// draws earth shadow overlapping the moon using stencil buffer
// umbra and penumbra are sized separately for accuracy
void SolarSystem::draw_earth_shadow(const Navigator * nav, Projector * prj)
{

	Vec3d e = getEarth()->get_ecliptic_pos();
	Vec3d m = getMoon()->get_ecliptic_pos();  // relative to earth
	Vec3d mh = getMoon()->get_heliocentric_ecliptic_pos();  // relative to sun
	float mscale = getMoon()->get_sphere_scale();

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

	for (int i=0; i<=100; i++)
	{
		r = Mat4d::rotation(shadow, 2*M_PI*i/100.) * upt;
		s = shadow + r;

		glTexCoord2f(0.6,0);  // position in texture of umbra edge
		prj->sVertex3( s[0],s[1], s[2], mat);
	}
	glEnd();


	// now penumbra
	Vec3d u, sp;
	glBegin(GL_TRIANGLE_STRIP);
	for (int i=0; i<=100; i++)
	{
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


void SolarSystem::update(int delta_time, Navigator* nav)
{
	vector<Planet*>::iterator iter = system_planets.begin();
	while (iter != system_planets.end())
	{
		(*iter)->update_trail(nav);
		(*iter)->update(delta_time);
		iter++;
	}


}


// is a lunar eclipse close at hand?
bool SolarSystem::near_lunar_eclipse(const Navigator * nav, Projector *prj)
{

	// TODO: could replace with simpler test

	Vec3d e = getEarth()->get_ecliptic_pos();
	Vec3d m = getMoon()->get_ecliptic_pos();  // relative to earth
	Vec3d mh = getMoon()->get_heliocentric_ecliptic_pos();  // relative to sun

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

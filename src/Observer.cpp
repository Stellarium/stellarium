/*
 * Stellarium
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "Observer.hpp"
#include "StelUtils.hpp"
#include "InitParser.hpp"
#include "SolarSystem.hpp"
#include "Planet.hpp"
#include "Translator.hpp"

#include <cassert>

static
Vec3d getRot(const Planet *p) {
  Mat4d m(p->getRotEquatorialToVsop87());
  Vec3d r;
  r[1] = atan2(m.r[4],
               sqrt(m.r[0]*m.r[0]+m.r[8]*m.r[8]));
    // not well defined if cos(r[1])==0
  r[0] = atan2(-m.r[6],m.r[5]);
  r[2] = atan2( m.r[8],m.r[0]);
  return r;
}

class ArtificialPlanet : public Planet {
public:
  ArtificialPlanet(const Planet &p)
//    : Planet(0,"",0,0,0,0,Vec3f(0,0,0),0,"","",
//             pos_func_type(),0,false,true) {
    : Planet(p) {
    englishName = "";
    nameI18 = L"";
    flagHalo = 0;
    flag_lighting = 0;
    albedo = 0;
    color[0] = color[1] = color[2] = 0;
    rings = 0;
    satellites.clear();
    trail.clear();
    hidden = true;
    tex_map = STextureSP();
	tex_halo = STextureSP();
	tex_big_halo = STextureSP();
    coord_func = pos_func_type();
//    osculating_func = 0;
    radius = 0;
      // set parent = sun:
    if (parent) {
      while (parent->get_parent()) parent = parent->get_parent();
    } else {
      parent = &p; // sun
    }
cout << "parent: " << parent->getEnglishName() << endl;
    setRotEquatorialToVsop87(p.getRotEquatorialToVsop87());
    set_heliocentric_ecliptic_pos(p.get_heliocentric_ecliptic_pos());
  }
  void setRot(const Vec3d &r) {
    const double ca = cos(r[0]);
    const double sa = sin(r[0]);
    const double cd = cos(r[1]);
    const double sd = sin(r[1]);
    const double cp = cos(r[2]);
    const double sp = sin(r[2]);
    Mat4d m;
    m.r[ 0] = cd*cp;
    m.r[ 4] = sd;
    m.r[ 8] = cd*sp;
    m.r[12] = 0;
    m.r[ 1] = -ca*sd*cp -sa*sp;
    m.r[ 5] =  ca*cd;
    m.r[ 9] = -ca*sd*sp +sa*cp;
    m.r[13] = 0;
    m.r[ 2] =  sa*sd*cp -ca*sp;
    m.r[ 6] = -sa*cd;
    m.r[10] =  sa*sd*sp +ca*cp;
    m.r[14] = 0;
    m.r[ 3] = 0;
    m.r[ 7] = 0;
    m.r[11] = 0;
    m.r[15] = 0;
    setRotEquatorialToVsop87(m);
  }
};

Observer::Observer(const SolarSystem &ssystem)
         :ssystem(ssystem),
          planet(0), artificial_planet(0),
          longitude(0.), latitude(0.), altitude(0)
{
	name = L"Anonymous_Location";
	flag_move_to = false;
}

Observer::~Observer()
{
  if (artificial_planet) delete artificial_planet;
}

Vec3d Observer::getCenterVsop87Pos(void) const {
  return getHomePlanet()->get_heliocentric_ecliptic_pos();
}

double Observer::getDistanceFromCenter(void) const {
  return getHomePlanet()->getRadius() + (altitude/(1000*AU));
}

Mat4d Observer::getRotLocalToEquatorial(double jd) const {
  double lat = latitude;
  // TODO: Figure out how to keep continuity in sky as reach poles
  // otherwise sky jumps in rotation when reach poles in equatorial mode
  // This is a kludge
  if( lat > 89.5 )  lat = 89.5;
  if( lat < -89.5 ) lat = -89.5;
  return Mat4d::zrotation((getHomePlanet()->getSiderealTime(jd)+longitude)*(M_PI/180.))
       * Mat4d::yrotation((90.-lat)*(M_PI/180.));
}

Mat4d Observer::getRotEquatorialToVsop87(void) const {
  return getHomePlanet()->getRotEquatorialToVsop87();
}

void Observer::load(const string& file, const string& section)
{
	InitParser conf;
	conf.load(file);
	if (!conf.find_entry(section))
	{
		cerr << "ERROR : Can't find observator section " << section << " in file " << file << endl;
		assert(0);
	}
	load(conf, section);
}

void Observer::load(const InitParser& conf, const string& section)
{
	name = _(conf.get_str(section, "name").c_str());

	for (string::size_type i=0;i<name.length();++i)
	{
		if (name[i]=='_') name[i]=' ';
	}

    if (!setHomePlanet(conf.get_str(section, "home_planet", "Earth"))) {
      planet = ssystem.getEarth();
    }
    
    cout << "Loading location: \"" << StelUtils::wstringToString(name) <<"\", on " << planet->getEnglishName();
    
//    printf("(home_planet should be: \"%s\" is: \"%s\") ",
//           conf.get_str(section, "home_planet").c_str(),
//           planet->getEnglishName().c_str());
	latitude  = StelUtils::get_dec_angle(conf.get_str(section, "latitude"));
	longitude = StelUtils::get_dec_angle(conf.get_str(section, "longitude"));
	altitude = conf.get_int(section, "altitude");
}

void Observer::save(const string& file, const string& section) const
{
	printf("Saving location %s to file %s\n",StelUtils::wstringToString(name).c_str(), file.c_str());

	InitParser conf;
	conf.load(file);

	setConf(conf,section);

	conf.save(file);
}


// change settings but don't write to files
void Observer::setConf(InitParser & conf, const string& section) const
{
	conf.set_str(section + ":name", StelUtils::wstringToString(name));
	conf.set_str(section + ":home_planet", planet->getEnglishName());
	conf.set_str(section + ":latitude",
	             StelUtils::wstringToString(
	               StelUtils::printAngleDMS(latitude*M_PI/180.0,
	                                          true, true)));
	conf.set_str(section + ":longitude",
	             StelUtils::wstringToString(
	               StelUtils::printAngleDMS(longitude*M_PI/180.0,
	                                          true, true)));

	conf.set_int(section + ":altitude", altitude);

	// TODO: clear out old timezone settings from this section
	// if still in loaded conf?  Potential for confusion.
}


string Observer::getHomePlanetEnglishName(void) const {
  const Planet *p = getHomePlanet();
  return p ? p->getEnglishName() : "";
}

wstring Observer::getHomePlanetNameI18n(void) const {
  const Planet *p = getHomePlanet();
  return p ? p->getNameI18n() : L"";
}

wstring Observer::get_name(void) const {
  return name;
}


bool Observer::setHomePlanet(const string &english_name) {
  Planet *p = ssystem.searchByEnglishName(english_name);
  if (!p) return false;
  if (planet != p) {
    if (planet) {
      if (!artificial_planet) {
        artificial_planet = new ArtificialPlanet(*planet);
      }
      time_to_go = 2000; // milliseconds: 2 seconds
    }
    planet = p;
  }
  return true;
}

const Planet *Observer::getHomePlanet(void) const {
  return artificial_planet ? artificial_planet : planet;
}


// move gradually to a new observation location
void Observer::moveTo(double lat, double lon, double alt, int duration, const wstring& _name)
{
  flag_move_to = true;

  start_lat = latitude;
  end_lat = lat;

  start_lon = longitude;
  end_lon = lon;

  start_alt = altitude;
  end_alt = alt;

  move_to_coef = 1.0f/duration;
  move_to_mult = 0;

	name = _name;
  //  printf("coef = %f\n", move_to_coef);
}


// for moving observator position gradually
// TODO need to work on direction of motion...
void Observer::update(int delta_time) {
  if (artificial_planet) {
    time_to_go -= delta_time;
    if (time_to_go <= 0) {
      delete artificial_planet;
      artificial_planet = 0;
    } else {
      const double f1 = time_to_go/(double)(time_to_go + delta_time);
      const double f2 = 1.0 - f1;
         // helicentric positions:
      artificial_planet->set_heliocentric_ecliptic_pos(
        artificial_planet->get_heliocentric_ecliptic_pos()*f1
         + planet->get_heliocentric_ecliptic_pos()*f2);
         // 3 Euler angles:
      artificial_planet->setRot(
        getRot(artificial_planet)*f1 + getRot(planet)*f2);

      artificial_planet->averageRotationElements(
        planet->getRotationElements(),f1,f2);
    }
  }

  if (flag_move_to) {
    move_to_mult += move_to_coef*delta_time;

    if (move_to_mult >= 1.f) {
      move_to_mult = 1.f;
      flag_move_to = false;
    }

    latitude = start_lat - move_to_mult*(start_lat-end_lat);
    longitude = start_lon - move_to_mult*(start_lon-end_lon);
    altitude = int(start_alt - move_to_mult*(start_alt-end_alt));

  }
}


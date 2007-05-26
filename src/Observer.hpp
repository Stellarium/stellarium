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

#ifndef _OBSERVER_H_
#define _OBSERVER_H_

#include "vecmath.h"
#include <string>

using namespace std;

class InitParser;
class SolarSystem;
class Planet;
class ArtificialPlanet;

class Observer
{
public:

	Observer(const class SolarSystem &ssystem);
	~Observer();
    bool setHomePlanet(const string &english_name);
    bool setHomePlanet(const Planet *p);
    const Planet *getHomePlanet(void) const;
    string getHomePlanetEnglishName(void) const;
    wstring getHomePlanetNameI18n(void) const;
	wstring get_name(void) const;

    Vec3d getCenterVsop87Pos(void) const;
    double getDistanceFromCenter(void) const;
    Mat4d getRotLocalToEquatorial(double jd) const;
    Mat4d getRotEquatorialToVsop87(void) const;

	void save(const string& file, const string& section) const;
	void setConf(InitParser &conf, const string& section) const;
	void load(const string& file, const string& section);
	void load(const InitParser& conf, const string& section);

	void set_latitude(double l) {latitude=l;}
	double get_latitude(void) const {return latitude;}
	void set_longitude(double l) {longitude=l;}
	double get_longitude(void) const {return longitude;}
	void set_altitude(int a) {altitude=a;}
	int get_altitude(void) const {return altitude;}

	void moveTo(double lat, double lon, double alt, int duration, const wstring& _name);  // duration in ms
	void update(int delta_time);  // for moving observing position 

private:
    const SolarSystem &ssystem;
	wstring name;			// Position name

	const Planet *planet;
    ArtificialPlanet *artificial_planet;
    int time_to_go;

	double longitude;		// Longitude in degree
	double latitude;		// Latitude in degree
	int altitude;			// Altitude in meter

	// for changing position
	bool flag_move_to;
	double start_lat, end_lat;
	double start_lon, end_lon;
	double start_alt, end_alt;
	float move_to_coef, move_to_mult;

};

#endif

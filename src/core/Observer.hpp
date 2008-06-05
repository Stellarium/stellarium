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

#include <QObject>
#include <QString>
#include "vecmath.h"

class SolarSystem;
class Planet;
class ArtificialPlanet;
class QSettings;

class Observer : public QObject
{
	Q_OBJECT;

public:
	Observer(const class SolarSystem &ssystem);
	~Observer();

	void update(int delta_time);  // for moving observing position 
	void setHomePlanet(const Planet *p,float transit_seconds=2.f);
	const Planet *getHomePlanet(void) const;
	
	void setConf(QSettings* conf, const QString& section) const;
	void load(QSettings* conf, const QString& section);
	
	//! TODO: Move to MovementMgr
	//! @param duration in ms
	void moveTo(double lat, double lon, double alt, int duration, const QString& locationName);
	
	Vec3d getCenterVsop87Pos(void) const;
	double getDistanceFromCenter(void) const;
	Mat4d getRotLocalToEquatorial(double jd) const;
	Mat4d getRotEquatorialToVsop87(void) const;
	
public slots:
	///////////////////////////////////////////////////////////////////////////
	// Method callable from script and GUI
	//! Set the home planet from its english name
	bool setHomePlanet(const QString& englishName);
	
	//! Get the translated home planet name
	QString getHomePlanetNameI18n(void) const;
	
	//! Get the observatory name
	QString getLocationName(void) const;
	
	//! Get the latitude in degrees
	double getLatitude(void) const {return latitude;}
	//! Set the latitude in degrees
	void setLatitude(double l) {latitude=l;}
	
	//! Get the longitude in degrees
	double getLongitude(void) const {return longitude;}
	//! Set the longitude in degrees
	void setLongitude(double l) {longitude=l;}
	
	//! Get the altitude in meters
	int getAltitude(void) const {return altitude;}
	//! Set the altitude in meters
	void setAltitude(int a) {altitude=a;}

	//! Load observatory informations from the given config file
	void load(const QString& file, const QString& section);
	//! Save observatory informations to the given config file
	void save(const QString& file, const QString& section) const;
	
private:
    const SolarSystem &ssystem;
	QString locationName;			// Position name

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

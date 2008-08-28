/*
 * Copyright (C) 2008 Fabien Chereau
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

#ifndef _PLANET_LOCATION_MGR_HPP_
#define _PLANET_LOCATION_MGR_HPP_

#include <QString>
#include <QObject>
#include <QMetaType>
#include <QMap>

class QStringListModel;

//! Store the information for a location on a planet
class PlanetLocation
{
public:
	PlanetLocation();
	//! Return a short string which can be used in a list view
	QString toSmallString() const;
	//! Location/city name
	QString name;
	//! English country name or empty string
	QString country;
	//! State/region name
	QString state;
	//! English planet name
	QString planetName;
	//! Longitude in degree
	double longitude;
	//! Latitude in degree
	double latitude;
	//! Altitude in meter
	int altitude;
};

//! Base class for any astro image with a fixed position
class PlanetLocationMgr : public QObject
{
	Q_OBJECT;
	
public:
	//! Default constructor
	PlanetLocationMgr();
	//! Destructor
	~PlanetLocationMgr();
	
	//! Return the model containing all the city
	QStringListModel* getModelAll() {return modelAllLocation;}
	
	//! Return the PlanetLocation for the given row (match modelAllLocation index row)
	const PlanetLocation locationForSmallString(const QString& s) const {return locations.value(s);}
	
	static int PlanetLocationRole;
	
private:
	//! Load cities from a file
	void loadCities(const QString& fileName);
	
	//! Model containing all the city information
	QStringListModel* modelAllLocation;
	
	//! The list of all loaded locations
	QMap<QString, PlanetLocation> locations;
};

#endif // _PLANET_LOCATION_MGR_HPP_

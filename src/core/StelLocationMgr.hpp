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
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */

#ifndef _STELLOCATIONMGR_HPP_
#define _STELLOCATIONMGR_HPP_

#include "StelLocation.hpp"
#include <QString>
#include <QObject>
#include <QMetaType>
#include <QMap>

typedef QList<StelLocation> LocationList;
typedef QMap<QString,StelLocation> LocationMap;

//! @class StelLocationMgr
//! Manage the list of available location.
class StelLocationMgr : public QObject
{
	Q_OBJECT

public:
	//! Default constructor which loads the list of locations from the base and user location files.
	StelLocationMgr();

	//! Construct a StelLocationMgr which uses the locations given instead of loading them from the files.
	StelLocationMgr(const LocationList& locations);

	//! Replaces the loaded location list
	void setLocations(const LocationList& locations);

	//! Return the list of all loaded locations
	QList<StelLocation> getAll() const {return locations.values();}

	//! Returns a map of all loaded locations. The key is the location ID, suitable for a list view.
	LocationMap getAllMap() const { return locations; }

	//! Return the StelLocation from a CLI
	const StelLocation locationFromCLI() const;

	//! Return a valid location when no valid one was found.
	const StelLocation& getLastResortLocation() const {return lastResortLocation;}
	
	//! Get whether a location can be permanently added to the list of user locations
	//! The main constraint is that the small string must be unique
	bool canSaveUserLocation(const StelLocation& loc) const;

	//! Add permanently a location to the list of user locations
	//! It is later identified by its small string
	bool saveUserLocation(const StelLocation& loc);

	//! Get whether a location can be deleted from the list of user locations
	//! If the location comes from the base read only list, it cannot be deleted
	//! @param id the location ID
	bool canDeleteUserLocation(const QString& id) const;

	//! Delete permanently the given location from the list of user locations
	//! If the location comes from the base read only list, it cannot be deleted and false is returned
	//! @param id the location ID
	bool deleteUserLocation(const QString& id);

	//! Find location via online lookup of IP address
	void locationFromIP();

	//! Find list of locations within @param radiusDegrees of selected (usually screen-clicked) coordinates.
	LocationMap pickLocationsNearby(const QString planetName, const float longitude, const float latitude, const float radiusDegrees);
	//! Find list of locations in a particular country only.
	LocationMap pickLocationsInCountry(const QString country);

public slots:
	//! Return the StelLocation for a given string
	//! Can match location name, or coordinates
	const StelLocation locationForString(const QString& s) const;

	//! Process answer from online lookup of IP address
	void changeLocationFromNetworkLookup();

signals:
	//! Can be used to detect changes to the full location list
	//! i.e. when the user added or removed locations
	void locationListChanged();

private:
	void generateBinaryLocationFile(const QString& txtFile, bool isUserLocation, const QString& binFile) const;

	//! Load cities from a file
	static LocationMap loadCities(const QString& fileName, bool isUserLocation);
	static LocationMap loadCitiesBin(const QString& fileName);

	//! The list of all loaded locations
	LocationMap locations;
	
	StelLocation lastResortLocation;
};

#endif // _STELLOCATIONMGR_HPP_

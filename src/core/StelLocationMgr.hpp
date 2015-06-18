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

class QStringListModel;

//! @class StelLocationMgr
//! Manage the list of available location.
class StelLocationMgr : public QObject
{
	Q_OBJECT

public:
	//! Default constructor
	StelLocationMgr();
	//! Destructor
	~StelLocationMgr();

	//! Return the model containing all the city
	QStringListModel* getModelAll() {return modelAllLocation;}
	//! Return the model containing picked (nearby) cities or cities from a single country, or other preselection.
	QStringListModel* getModelPicked() {return modelPickedLocation;}

	//! Return the list of all loaded locations
	QList<StelLocation> getAll() const {return locations.values();}

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

	//! Preselect list of locations within @param radiusDegrees of selected (usually screen-clicked) coordinates.
	//! The list can be retrieved by calling @name getModelPicked().
	void pickLocationsNearby(const QString planetName, const float longitude, const float latitude, const float radiusDegrees);
	//! Preselect list of locations in a particular country only.
	//! The list can be retrieved by calling @name getModelPicked().
	void pickLocationsInCountry(const QString country);

public slots:
	//! Return the StelLocation for a given string
	//! Can match location name, or coordinates
	const StelLocation locationForString(const QString& s) const;

	//! Process answer from online lookup of IP address
	void changeLocationFromNetworkLookup();

private:
	void generateBinaryLocationFile(const QString& txtFile, bool isUserLocation, const QString& binFile) const;

	//! Load cities from a file
	QMap<QString, StelLocation> loadCities(const QString& fileName, bool isUserLocation) const;
	QMap<QString, StelLocation> loadCitiesBin(const QString& fileName) const;

	//! Model containing all the city information
	QStringListModel* modelAllLocation;
	//! Model containing selected city information
	QStringListModel* modelPickedLocation;

	//! The list of all loaded locations
	QMap<QString, StelLocation> locations;
	//! A list of locations generated on-the-fly by filtering from @name locations
	QMap<QString, StelLocation> pickedLocations;
	
	StelLocation lastResortLocation;
};

#endif // _STELLOCATIONMGR_HPP_

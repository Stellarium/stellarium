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

#ifndef _LOCATION_MGR_HPP_
#define _LOCATION_MGR_HPP_

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
	Q_OBJECT;
	
public:
	//! Default constructor
	StelLocationMgr();
	//! Destructor
	~StelLocationMgr();
	
	//! Return the model containing all the city
	QStringListModel* getModelAll() {return modelAllLocation;}
	
	//! Return the StelLocation for the given row (match modelAllLocation index row)
	const StelLocation locationForSmallString(const QString& s) const;
	
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
	
private:
	//! Load cities from a file
	void loadCities(const QString& fileName, bool isUserLocation);
	
	//! Model containing all the city information
	QStringListModel* modelAllLocation;
	
	//! The list of all loaded locations
	QMap<QString, StelLocation> locations;
};

#endif // _LOCATION_MGR_HPP_

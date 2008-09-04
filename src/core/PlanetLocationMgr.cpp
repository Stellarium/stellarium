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

#include "StelApp.hpp"
#include "StelFileMgr.hpp"
#include "PlanetLocationMgr.hpp"

#include <QStringListModel>
#include <QDebug>
#include <QFile>

PlanetLocationMgr::PlanetLocationMgr()
{
	loadCities("data/base_locations.txt");
	loadCities("data/user_locations.txt");
	
	modelAllLocation = new QStringListModel(this);
	modelAllLocation->setStringList(locations.keys());
}

void PlanetLocationMgr::loadCities(const QString& fileName)
{
	// Load the cities from data file
	QString cityDataPath;
	try
	{
		cityDataPath = StelApp::getInstance().getFileMgr().findFile(fileName);
	}
	catch (std::runtime_error& e)
	{
		qWarning() << "ERROR: Failed to locate city data: " << fileName << e.what();
		return;
	}

	QFile sourcefile(cityDataPath);
	if (!sourcefile.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		qWarning() << "ERROR: Could not open city data file: " << cityDataPath;
		return;
	}
	
	// Read the data serialized from the file.
	// Code below borrowed from Marble (http://edu.kde.org/marble/)
	QTextStream sourcestream(&sourcefile);
	sourcestream.setCodec("UTF-8");	
	while (!sourcestream.atEnd())
	{
		const QString& rawline=sourcestream.readLine();
		if (rawline.startsWith('#'))
			continue;
		PlanetLocation loc = PlanetLocation::createFromLine(rawline);
		
		if (locations.contains(loc.toSmallString()))
		{
			// Add the state in the name of the existing one and the new one to differentiate
			PlanetLocation loc2 = locations[loc.toSmallString()];
			if (!loc2.state.isEmpty())
				loc2.name += " ("+loc2.state+")";
			// remove and re-add the fixed version
			locations.remove(loc.toSmallString());
			locations[loc2.toSmallString()] = loc2;
			
			if (!loc.state.isEmpty())
				loc.name += " ("+loc.state+")";
			locations[loc.toSmallString()] = loc;
		}
		else
		{
			locations[loc.toSmallString()] = loc;
		}
	}
	sourcefile.close();
}

PlanetLocationMgr::~PlanetLocationMgr()
{
}

const PlanetLocation PlanetLocationMgr::locationForSmallString(const QString& s) const
{
	QMap<QString, PlanetLocation>::const_iterator iter = locations.find(s);
	if (iter==locations.end())
	{
		qWarning() << "Warning: location " << s << "is unknown, use Paris as default location.";
		return locations["Paris, France"];
	}
	else
	{
		return locations.value(s);
	}
}

// Get whether a location can be permanently added to the list of user locations
bool PlanetLocationMgr::canSaveUserLocation(const PlanetLocation& loc) const
{
	return locations.find(loc.toSmallString())==locations.end();
}

// Add permanently a location to the list of user locations
bool PlanetLocationMgr::saveUserLocation(const PlanetLocation& loc)
{
	if (!canSaveUserLocation(loc))
		return false;
	
	// Add in the program
	locations[loc.toSmallString()]=loc;
	
	// Append in the user file
	modelAllLocation->setStringList(locations.keys());
	return true;
}

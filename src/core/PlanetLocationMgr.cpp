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

int PlanetLocationMgr::PlanetLocationRole = Qt::UserRole+1;

QString PlanetLocation::toSmallString() const
{
	return name + ", " + state + ", " +countryCode;
}

PlanetLocationMgr::PlanetLocationMgr()
{
	loadCities("data/base_cities.txt");
	loadCities("data/user_cities.txt");
	
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
	
	QString rawline;
	QString popstring;
	QString latstring;
	QString lngstring;
	QStringList splitline;

	while (!sourcestream.atEnd())
	{
		PlanetLocation loc;
		rawline=sourcestream.readLine();
		splitline = rawline.split("\t");
		loc.name    = splitline[0];
		loc.state   = splitline[1];
		loc.countryCode = splitline[2];
		//loc.role    = splitline[3];
		popstring = splitline[4];
		latstring = splitline[5];
		lngstring = splitline[6];
		//loc.population = (int) ( 1000 * popstring.toFloat() );

		if (splitline.size()>7)
		{
			// Parse planet name
			loc.planetName = splitline[7];
		}
		else
		{
			// Earth by default
			loc.planetName = "Earth";
		}
		
		loc.longitude = lngstring.left(lngstring.size() - 2).toDouble();
		if (lngstring.contains("W"))
			loc.longitude=-loc.longitude;
		
		loc.latitude = latstring.left(latstring.size() - 2).toDouble();
		if (latstring.contains("S"))
			loc.latitude=-loc.latitude;
		
		locations[loc.toSmallString()] = loc;
	}
	
	sourcefile.close();
}

PlanetLocationMgr::~PlanetLocationMgr()
{
}

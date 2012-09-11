/*
 * Stellarium Location List Editor
 * Copyright (C) 2012  Bogdan Marinov
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
 * Foundation, Inc., 51 Franklin Street,
 * Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "Location.hpp"

#include <QDebug>
#include <QFile>
#include <QStringList>
#include <QTextStream>

QMap<QString,QString> Location::mapCodeToCountry;

Location::Location() :
    longitude(0.),
    latitude(0.),
    altitude(0.),
    bortleScaleIndex(-1.),
    role('N'),
    population(0)
{
	;
}


QString Location::getBasicId()
{
	if (name.isEmpty())
		return QString();
	
	if (countryName.isEmpty())
		return name;
	else
		return QString("%1, %2").arg(name, countryName);
}

QString Location::getExtendedId()
{
	if (name.isEmpty())
		return QString();
	
	if (countryName.isEmpty())
	{
		if (region.isEmpty())
			return name;
		else
			return QString("%1 (%2)").arg(name, region);
	}
	else
	{
		if (region.isEmpty())
			return getBasicId();
		else
			return QString("%1 (%2), %3").arg(name, region, country);
	}
}


// Write the location as a tab-separated string.
// Reuses code from the same function in StelLocation with some changes.
QString Location::toLine() const
{
	QString latStr;
	if (latitude < 0)
		latStr = QString("%1S").arg(-latitude, 0, 'f', 6);
	else
		latStr = QString("%1N").arg(latitude, 0, 'f', 6);
	
	QString longStr;
	if (longitude < 0)
		longStr = QString("%1W").arg(-longitude, 0, 'f', 6);
	else
		longStr = QString("%1E").arg(longitude, 0, 'f', 6);
	
	return QString("%1\t%2\t%3\t%4\t%5\t%6\t%7\t%8\t%9\t%10\t%11\t%12")
	        .arg(name)
	        .arg(region)
	        .arg(country)
	        .arg(role)
	        .arg(population)
	        .arg(latStr)
	        .arg(longStr)
	        .arg(altitude)
	        .arg(bortleScaleIndex < 0 ? "" : QString::number(bortleScaleIndex))
	        .arg(timeZone)
	        .arg(planetName)
	        .arg(landscapeKey);
}

// Read a location from a tab-separated string.
Location Location::fromLine(const QString& line)
{
	Location loc;
	const QStringList& splitline = line.split("\t");
	if (splitline.size() < 8)
		return loc;
	loc.name    = splitline.at(0);
	if (loc.name.isEmpty())
		return loc;
	loc.region  = splitline.at(1);
	loc.country = splitline.at(2);
	loc.countryName = Location::stringToCountry(loc.country);
	loc.role    = splitline.at(3).at(0);
	if (loc.role == '\0')
		loc.role = 'N'; // TODO: Define standard default/unknown value. ('X'?)
	loc.population = (int) (splitline.at(4).toFloat());
	
	const QString& latStr = splitline.at(5);
	loc.latitude = latStr.left(latStr.size() - 1).toFloat();
	if (latStr.endsWith('S'))
		loc.latitude =- loc.latitude;
	
	const QString& longStr = splitline.at(6);
	loc.longitude = longStr.left(longStr.size() - 1).toFloat();
	if (longStr.endsWith('W'))
		loc.longitude =- loc.longitude;
	
	loc.altitude = (int)splitline.at(7).toFloat();
	
	// Light pollution -1 is interpreted as system-determined.
	if (splitline.size() > 8)
	{
		bool ok;
		loc.bortleScaleIndex = splitline.at(8).toInt(&ok);
		if (ok == false)
			loc.bortleScaleIndex = -1.;
	}
	else
		loc.bortleScaleIndex = -1.;
	
	// Empty time zone is interpreted as default time.
	if (splitline.size() > 9)
	{
		loc.timeZone = splitline.at(9);
	}
	
	// Empty planet name is interpreted as Earth.
	if (splitline.size() > 10)
	{
		
		loc.planetName = splitline.at(10);
	}
	
	// Associated landscape is optional.
	if (splitline.size() > 11)
	{
		loc.landscapeKey = splitline.at(11);
	}
	
	return loc;
}

QString Location::stringToCountry(const QString& string)
{
	if (mapCodeToCountry.isEmpty())
		loadCountryCodes();
	
	return mapCodeToCountry.value(string, string);
}

void Location::loadCountryCodes()
{
	QFile file(":/locationListEditor/iso3166-1-alpha-2.utf8");
	file.open(QFile::ReadOnly | QFile::Text);
	QTextStream stream(&file);
	while (!stream.atEnd())
	{
		QString line = stream.readLine();
		if (line.isEmpty())
			continue;
		
		QStringList splitLine = line.split('\t');
		//qDebug() << splitLine;
		mapCodeToCountry.insert(splitLine.first(), splitLine.last());
	}
	file.close();
	//qDebug() << mapCodeToCountry;
}


QDataStream& operator<< (QDataStream& out, Location& loc)
{
	// The strange manglings are because of the discrepancies between my
	// data structures and Stellarium's model.
	// Note that there's no time zone field. :(
	bool dummyUserLocationFlag = false;
	out << loc.name
	    << loc.region
	    << loc.countryName
	    << loc.role
	    << ((int) loc.population*1000)
	    << loc.latitude
	    << loc.longitude
	    << loc.altitude
	    << loc.bortleScaleIndex
	    << (loc.planetName.isEmpty() ? QString("Earth") : loc.planetName)
	    << loc.landscapeKey
	    << dummyUserLocationFlag;
	return out;
}

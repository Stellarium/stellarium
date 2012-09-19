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
    role('X'),
    hasDuplicate(false),
    lineNum(0)
{
	;
}


QString Location::generateId()
{
	if (name.isEmpty() || stelName.isEmpty())
		return QString();
	
	if (countryName.isEmpty())
		stelId = stelName;
	else
		stelId = QString("%1, %2").arg(stelName, countryName);
	
	return stelId;
}

QString Location::extendId()
{
	if (name.isEmpty())
		return QString();
	
	if (region.isEmpty())
		stelName = name;
	else
		stelName = QString("%1 (%2)").arg(name, region);
	
	// This will also re-set stelId
	return generateId();
}


// Write the location as a tab-separated string.
// Reuses code from the same function in StelLocation with some changes.
QString Location::toLine() const
{
	//QString latStr = latitudeToString(latitude);
	//QString longStr = longitudeFromString(longitude);
	
	return QString("%1\t%2\t%3\t%4\t%5\t%6\t%7\t%8\t%9\t%10\t%11\t%12")
	        .arg(name)
	        .arg(region)
	        .arg(country)
	        .arg(role)
	        .arg(population)
	        .arg(latitudeStr)
	        .arg(longitudeStr)
	        .arg(altitude)
	        .arg(bortleScaleIndex < 0 ? "" : QString::number(bortleScaleIndex))
	        .arg(timeZone)
	        .arg(planetName)
	        .arg(landscapeKey)
	        .trimmed();
}

// Read a location from a tab-separated string.
Location* Location::fromLine(const QString& line)
{
	Location* loc = new Location();
	const QStringList& splitline = line.split("\t");
	if (splitline.size() < 8)
		return 0;
	loc->name    = splitline.at(0);
	if (loc->name.isEmpty())
		return 0;
	loc->stelName = loc->name;
	loc->region  = splitline.at(1);
	loc->country = splitline.at(2);
	loc->countryName = Location::stringToCountry(loc->country);
	loc->role    = splitline.at(3).at(0);
	if (loc->role == '\0')
		loc->role = 'X';
	loc->population = splitline.at(4);
	
	const QString& latStr = splitline.at(5);
	loc->latitude = latitudeFromString(latStr);
	loc->latitudeStr = latStr;
	
	const QString& longStr = splitline.at(6);
	loc->longitude = longitudeFromString(longStr);
	loc->longitudeStr = longStr;
	
	loc->altitude = (int) splitline.at(7).toFloat();
	
	// Light pollution -1 is interpreted as system-determined.
	if (splitline.size() > 8)
	{
		bool ok;
		loc->bortleScaleIndex = splitline.at(8).toInt(&ok);
		if (ok == false)
			loc->bortleScaleIndex = -1.;
	}
	else
		loc->bortleScaleIndex = -1.;
	
	// Empty time zone is interpreted as default time.
	if (splitline.size() > 9)
	{
		loc->timeZone = splitline.at(9);
	}
	
	// Empty planet name is interpreted as Earth.
	if (splitline.size() > 10)
	{
		
		loc->planetName = splitline.at(10);
	}
	
	// Associated landscape is optional.
	if (splitline.size() > 11)
	{
		loc->landscapeKey = splitline.at(11);
	}
	
	return loc;
}

void Location::toBinary(QDataStream& out)
{
	// The strange manglings are because of the discrepancies between my
	// data structures and Stellarium's model.
	// Note that there's no time zone field. :(
	bool dummyUserLocationFlag = false;
	out << stelName
	    << region
	    << countryName
	    << role
	    << ((int) (population.toFloat()*1000))
	    << latitude
	    << longitude
	    << altitude
	    << bortleScaleIndex
	    << (planetName.isEmpty() ? QString("Earth") : planetName)
	    << landscapeKey
	    << dummyUserLocationFlag;
}

QString Location::stringToCountry(const QString& string)
{
	if (mapCodeToCountry.isEmpty())
		loadCountryCodes();
	
	return mapCodeToCountry.value(string, string);
}

float Location::latitudeFromString(const QString& string, bool* ok)
{
	float latitude = string.left(string.size() - 1).toFloat(ok);
	if (string.endsWith('S') && latitude >= 0)
		latitude = -latitude;
	else if (ok && (!string.endsWith('N') || latitude < 0))
		*ok = false;
	return latitude;
}

float Location::longitudeFromString(const QString &string, bool* ok)
{
	float longitude = string.left(string.size() - 1).toFloat(ok);
	if (string.endsWith('W') && longitude >= 0)
		longitude = -longitude;
	else if (ok && (!string.endsWith('E') || longitude < 0))
		*ok = false;
	return longitude;
}

QString Location::latitudeToString(float latitude)
{
	if (latitude < 0)
		return QString("%1S").arg(-latitude, 0, 'g', 6);
	else
		return QString("%1N").arg(latitude, 0, 'g', 6);
}

QString Location::longitudeToString(float longitude)
{
	if (longitude < 0)
		return QString("%1W").arg(-longitude, 0, 'g', 6);
	else
		return QString("%1E").arg(longitude, 0, 'g', 6);
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


QDataStream& operator<<(QDataStream& out, const Location& loc)
{
	out << loc.name
	    << loc.stelName
	    << loc.country
	    << loc.countryName
	    << loc.region
	    << loc.planetName
	    << loc.longitude
	    << loc.longitudeStr
	    << loc.latitude
	    << loc.latitudeStr
	    << loc.altitude
	    << loc.timeZone
	    << loc.bortleScaleIndex
	    << loc.role
	    << loc.population
	    << loc.landscapeKey
	    << loc.stelId
	    << loc.hasDuplicate
	    << loc.lineNum;
	return out;
}

QDataStream& operator>>(QDataStream& in, Location& loc)
{
	in >> loc.name
	   >> loc.stelName
	   >> loc.country
	   >> loc.countryName
	   >> loc.region
	   >> loc.planetName
	   >> loc.longitude
	   >> loc.longitudeStr
	   >> loc.latitude
	   >> loc.latitudeStr
	   >> loc.altitude
	   >> loc.timeZone
	   >> loc.bortleScaleIndex
	   >> loc.role
	   >> loc.population
	   >> loc.landscapeKey
	   >> loc.stelId
	   >> loc.hasDuplicate
	   >> loc.lineNum;
	return in;
}

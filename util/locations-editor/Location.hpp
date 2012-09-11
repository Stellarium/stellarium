/*
 * Stellarium Location List Editor
 * Copyright (C) 2008 Fabien Chereau (original StelLocation class)
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

#ifndef LOCATION_HPP
#define LOCATION_HPP

#include <QDataStream>
#include <QMap>
#include <QString>

//! Simplified location class reusing code from StelLocation.
//! I couldn't use StelLocation, because it has a chain of header file
//! dependencies.
class Location
{
public:
	Location();
	
	//! @name Data fields, mostly reused from StelLocation.
	//! The entries marked by [IGNORED] are not (yet) used by Stellarium.
	//! @{
	//! Location name.
	QString name;
	//! Country (code or name, <strong>as it is in the source file</strong>).
	//! Allegedly can be empty, but people keep filling it.
	//! Note that in StelLocation it is the full country name. Here for now
	//! it is the contents of the file.
	QString country;
	//! Country (human-readable name, used for ID/disambiguation).
	QString countryName;
	//! Region (state, county, municipality, etc.), used for disambiguation.
	//! Can be empty. And should be empty, if there are no two with that name.
	QString region;
	//! Planet name (empty if Earth).
	QString planetName;
	//! Longitude in degrees
	float longitude;
	//! Latitude in degrees
	float latitude;
	//! Altitude in meters.
	int altitude;
	//! [IGNORED] Time zone string.
	//! Empty means "use system settings".
	//! Not even read by the current code in Stellarium, but this will 
	//! change. :)
	QString timeZone;
	//! [IGNORED] Light pollution index following the Bortle scale (0-9).
	//! -1 means "use system settings".
	float bortleScaleIndex;
	//! [IGNORED] Location role code.
	//! Possible values:
	//! * C/B for a capital city,
	//! * R for a regional capital
	//! * N ("normal") for any other human settlement
	//! * O for an observatory
	//! * L for a spacecraft lander
	//! * I for a spacecraft impact
	//! * A for a spacecraft crash
	QChar role;
	//! [IGNORED] Population in thousands of inhabitants.
	//! Note that StelLocation uses the exact population (this * 1000).
	int population;
	//! [IGNORED] A hint for associating a landscape to the location.
	QString landscapeKey;
	//! @}
	
	//! Identifier as used by LocationListModel and StelLocationMgr.
	QString id;
	
	//! Form the ID used by StelLocationMgr.
	//! Format: "Place, Country".
	QString getBasicId();
	//! Form the ID used by StelLocationMgr if there are duplicates.
	//! Format: "Place (Region), Country".
	QString getExtendedId();
	
	//! Output the location as a tab-delimited string.
	//! @author Fabien Chereau
	//! @author (and everyone else who edited this function in StelLocation)
	//! @author Bogdan Marinov
	QString toLine() const;
	//! Parse a tab-delimited line string location from a string line serialization.
	//! @author Fabien Chereau
	//! @author (and everyone else who edited this function in StelLocation)
	//! @author Bogdan Marinov
	static Location fromLine(const QString& line);
	
	//! Convert database field string to human-readable country name.
	//! @param string may contain a two-letter country code or a country name.
	static QString stringToCountry(const QString& string);

protected:
	//! Load the two-letter country codes list in the resources.
	static void loadCountryCodes();
	//! Map matching two-letter country codes (ISO 3166-1 alpha-2) to names.
	static QMap<QString,QString> mapCodeToCountry;
};

//! Serialize the passed Location into a binary blob.
//! No need for the other operator, as the app won't be reading binary lists.
//! @author Fabien Chereau (and possibly others)
QDataStream& operator<<(QDataStream& out, const Location& loc);

#endif // LOCATION_HPP

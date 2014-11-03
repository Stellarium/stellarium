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
	
	//! @brief %Location name <strong>as stored in the source file</strong>.
	QString name;
	//! %Location name as used in StelLocationMgr.
	//! Usually the same as #name, but can also contain the #region.
	//! Necessary for binary serialization. :(
	QString stelName;
	//! Country (code or name, <strong>as stored in the source file</strong>).
	//! Allegedly can be empty, but people keep filling it.
	//! In StelLocation it is populated with the full country name, which
	//! here is stored in #countryName.
	QString country;
	//! Country (human-readable name, used for ID/disambiguation).
	QString countryName;
	//! Region (state, county, municipality, etc.), used for disambiguation.
	//! Can be empty. And should be empty, if there are no two with that name.
	QString region;
	//! Planet name (empty if Earth).
	QString planetName;
	//! Longitude in degrees, negative is West.
	float longitude;
	//! Longitude in string form.
	//! Necessary to minimize changes to the source list when saving.
	QString longitudeStr;
	//! Latitude in degrees, negative is South.
	float latitude;
	//! Latitude in string form.
	//! Necessary to minimize changes to the source list when saving.
	QString latitudeStr;
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
	//! * X for unknown/user defined [DEFAULT value]
	QChar role;
	//! [IGNORED] Population in thousands of inhabitants.
	//! Note that StelLocation uses an int with the exact
	//! population (this * 1000).
	QString population;
	//! [IGNORED] A hint for associating a landscape to the location.
	QString landscapeKey;
	//! @}
	
	//! Identifier as used by LocationListModel and StelLocationMgr.
	//! Generated and set by generateId() and extendId().
	QString stelId;
	
	//! True if there's another Location with the same Stellarium ID.
	//! True for both the original and the later read.
	bool hasDuplicate;
	
	//! Line number in the file from which the location was loaded.
	int lineNum;
	
	//! Set and return the ID used by StelLocationMgr.
	//! Format: "stelName, countryName" or just "stelName".
	QString generateId();
	//! Set and return the ID used by StelLocationMgr if there are duplicates.
	//! Format: "name (region), countryName".
	//! Calls generateId() to re-set #stelName.
	QString extendId();
	
	//! Output the location as a tab-delimited string.
	//! @author Fabien Chereau
	//! @author (and everyone else who edited this function in StelLocation)
	//! @author Bogdan Marinov
	QString toLine() const;
	//! Parse a tab-delimited line string location from a string line serialization.
	//! @returns null pointer if the line does not describe a valid location.
	//! @author Fabien Chereau
	//! @author (and everyone else who edited this function in StelLocation)
	//! @author Bogdan Marinov
	static Location* fromLine(const QString& line);
	
	//! Serialize selected fields into a Stellarium-compatible blob.
	//! Necessary for saving location lists as binary files.
	//! No need for the other operator, as the app won't be reading binary lists.
	void toBinary(QDataStream& out);
	
	//! Convert database field string to human-readable country name.
	//! @param string may contain a two-letter country code or a country name.
	static QString stringToCountry(const QString& string);
	
	static float latitudeFromString(const QString& string, bool* ok = 0);
	static float longitudeFromString(const QString& string, bool* ok = 0);
	static QString latitudeToString(float latitude);
	static QString longitudeToString(float longitude);

protected:
	//! Load the two-letter country codes list in the resources.
	static void loadCountryCodes();
	//! Map matching two-letter country codes (ISO 3166-1 alpha-2) to names.
	static QMap<QString,QString> mapCodeToCountry;
};

//! Output \b all of the location's fields in a binary stream.
//! The old implementation was moved to Location::toBinary().
QDataStream& operator<<(QDataStream& out, const Location& loc);
//! 
QDataStream& operator>>(QDataStream& in, Location& loc);

#endif // LOCATION_HPP

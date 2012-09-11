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
#include <QString>

//! Simplified location class reusing code from StelLocation.
//! I couldn't use StelLocation, because it has a chain of header file
//! dependencies.
class Location
{
public:
	Location();
	
	//! @name Original data fields of StelLocation.
	//! The entries marked by [IGNORED] are not (yet) used by Stellarium.
	//! @{
	//! Location name.
	QString name;
	//! Country code or name.
	//! Allegedly can be empty, but people keep filling it.
	//! Note that in StelLocation it is the full country name. Here for now
	//! it is the contents of the file.
	QString country;
	//! Name of region (state, county, etc.), used for disambiguation.
	//! Can be empty.
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
	
	//! Output the location as a tab-delimited string.
	//! @author Fabien Chereau (and everyone else who edited StelLocation after him).
	QString toLine() const;
	//! Parse a tab-delimited line string location from a string line serialization.
	//! @author Fabien Chereau (and everyone else who edited StelLocation after him).
	static Location fromLine(const QString& line);
};

//! Serialize the passed Location into a binary blob.
//! @author Fabien Chereau (and possibly others)
QDataStream& operator<<(QDataStream& out, const Location& loc);
//! Load the Location from a binary blob.
//! @author Fabien Chereau (and possibly others)
QDataStream& operator>>(QDataStream& in, Location& loc);

#endif // LOCATION_HPP

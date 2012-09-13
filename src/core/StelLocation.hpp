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

#ifndef _STELLOCATION_HPP_
#define _STELLOCATION_HPP_

#include <QString>

//! @class StelLocation
//! Store the informations for a location on a planet
class StelLocation
{
public:
	StelLocation() : longitude(0.f), latitude(0.f), altitude(0), bortleScaleIndex(2.f), role('X'), isUserLocation(true) {;}

	//! Return a short string which can be used in a list view.
	QString getID() const
	{
		if (country.isEmpty())
			return name;
		else
			return name + ", " +country;
	}

	//! Output the location as a string ready to be stored in the user_location file
	QString serializeToLine() const;

	//! Location/city name
	QString name;
	//! English country name or empty string
	QString country;
	//! State/region name (usefull if 2 locations of the same country have the same name)
	QString state;
	//! English planet name
	QString planetName;
	//! Longitude in degree
	float longitude;
	//! Latitude in degree
	float latitude;
	//! Altitude in meter
	int altitude;
	//! Light pollution index following Bortle scale
	float bortleScaleIndex;
	//! A hint for associating a landscape to the location
	QString landscapeKey;
	//! Population in number of inhabitants
	int population;
	//! Location role code.
	//! Possible values:
	//! - \p C or \p B is a capital city
	//! - \p R is a regional capital
	//! - \p N is a normal city (any other type of settlement)
	//! - \p O is an observatory
	//! - \p L is a spacecraft lander
	//! - \p I is a spacecraft impact
	//! - \p A is a spacecraft crash
	//! - \p X is an unknown or user-defined location (the default value).
	QChar role;

	//! Parse a location from a line serialization
	static StelLocation createFromLine(const QString& line);

	//! Used privately by the StelLocationMgr
	bool isUserLocation;
};

//! Serialize the passed StelLocation into a binary blob.
QDataStream& operator<<(QDataStream& out, const StelLocation& loc);
//! Load the StelLocation from a binary blob.
QDataStream& operator>>(QDataStream& in, StelLocation& loc);

#endif // _STELLOCATION_HPP_

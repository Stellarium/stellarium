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

#ifndef STELLOCATION_HPP
#define STELLOCATION_HPP

#include <QString>
#include <QMetaType>

//! @class StelLocation
//! Store the informations for a location on a planet
class StelLocation
{
public:
	StelLocation() : longitude(0.f), latitude(0.f), altitude(0), bortleScaleIndex(2), population(0.f), role('X'), isUserLocation(true) {;}

	//! Return a short string which can be used in a list view.
	QString getID() const;

	bool isValid() const {return role!='!';}

	//! Output the location as a string ready to be stored in the user_location file
	QString serializeToLine() const;

	//! Location/city name
	QString name;
	//! English country name or empty string
	QString country;
	//! State/region name (useful if 2 locations of the same country have the same name)
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
	int bortleScaleIndex;
	//! A hint for associating a landscape to the location
	QString landscapeKey;
	//! Population in number of inhabitants
	int population;
	//! Location role code.
	//! Possible values:
	//! @li @p C or @p B is a capital city
	//! @li @p R is a regional capital
	//! @li @p N is a normal city (any other type of settlement)
	//! @li @p O is an observatory
	//! @li @p L is a spacecraft lander
	//! @li @p I is a spacecraft impact
	//! @li @p A is a spacecraft crash
	//! @li @p X is an unknown or user-defined location (the default value).
	//! @li @p ! is an invalid location.
	QChar role;
	//! IANA identificator of time zone.
	//! Note that timezone names under various OSes may be different than those used in Stellarium's
	//! location database (e.g. Ubuntu:Asia/Kolkata=Windows:Asia/Calcutta),
	//! which requires some translation effort during the loading process.
	//  See LP:1662132
	// GZ renamed to more clearly indicate these are IANA names.
	QString ianaTimeZone;

	//! Parse a location from a line serialization
	static StelLocation createFromLine(const QString& line);

	//! Compute great-circle distance between two locations
	static float distanceDegrees(const float long1, const float lat1, const float long2, const float lat2);

	//! Used privately by the StelLocationMgr
	bool isUserLocation;

	static const int DEFAULT_BORTLE_SCALE_INDEX;
private:
	//Register with Qt
	static int metaTypeId;
	static int initMetaType();
};

Q_DECLARE_METATYPE(StelLocation)

//! Serialize the passed StelLocation into a binary blob.
QDataStream& operator<<(QDataStream& out, const StelLocation& loc);
//! Load the StelLocation from a binary blob.
QDataStream& operator>>(QDataStream& in, StelLocation& loc);

#endif // STELLOCATION_HPP

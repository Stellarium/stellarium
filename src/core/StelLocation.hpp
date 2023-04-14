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
#include <QVariant>
#include <QMetaType>

class Planet;

//! @class StelLocation
//! Store the information for a location on a planet
class StelLocation
{
public:
	StelLocation() : altitude(0), population(0), role('X'), isUserLocation(true), longitude(0.f), latitude(0.f) {}
	//! constructor for ad-hoc locations.
	//! @arg lName location name
	//! @arg lState may be usedful if region has more than one such name
	//! @arg lRegion must be the long name of UN UM49 only! (E.g., "Western Europe")
	//! @arg lng geographical longitude, east-positive, degrees
	//! @arg lat geographical latitude, north-positive, degrees
	//! @arg alt altitude above mean sea level
	//! @arg populationK population in thousands
	//! @arg timeZone IANA timezone string like "Europe/Vienna" or "UT-7"
	//! @arg bortleIndex light pollution hint
	//! @arg roleKey code for location role.
	//! @arg landscapeID a fitting landscape
	StelLocation(const QString &lName, const QString &lState, const QString &lRegion, const float lng, const float lat, const int alt,
				 const int populationK, const QString &timeZone, const int bortleIndex, const QChar roleKey='X', const QString &landscapeID=QString());

	//! Return a short string which can be used in a list view.
	QString getID() const;

	bool isValid() const {return role!='!';}

	//! Output the location as a string ready to be stored in the user_location file
	QString serializeToLine() const;

	//! Compute great-circle distance from other location.
	//! arguments given in decimal degrees
	float distanceDegrees(const float otherLong, const float otherLat) const {return distanceDegrees(longitude, latitude, otherLong, otherLat);}

	//! Compute great-circle distance from other location.
	//! arguments given in decimal degrees
	double distanceKm(const double otherLong, const double otherLat) const;

	//! Compute azimuth towards Target. All angles (args and result) are in degrees.
	//! @return azimuth counted from north or south as set in the StelApp preferences, in [0...360].
	double getAzimuthForLocation(double longTarget, double latTarget) const;

	//! longitude and latitude are private to force use of special getters. These return a location on the north pole if we are located on an "Observer" (when "role" == 'o').
	//! By this we force useful view orientation when on an observer pseudo-planet.
	//! @param suppressObserver set to true to return the actual number in every case. (Required for some UI elements)
	float getLatitude(bool suppressObserver=false)  const;
	float getLongitude(bool suppressObserver=false) const;
	void setLatitude(float l)  { latitude=l;}
	void setLongitude(float l) { longitude=l;}

	//! Location/city name
	QString name;
	//! English region name (Northern Europe for example) or empty string
	QString region;
	//! State/region name (useful if 2 locations of the same country have the same name)
	QString state;
	//! English planet name
	QString planetName;
	//! Altitude in meter
	int altitude;
	//! Zenith luminance at moonless night as could be measured by a Sky Quality Meter, in cd/m²
	QVariant lightPollutionLuminance;
	//! A hint for associating a landscape to the location
	QString landscapeKey;
	//! Population in thousands of inhabitants
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
	//! @li @p o is an "observer" pseudo-planet. This is not stored in the location database but used ad-hoc in the LocationDialog.
	QChar role;
	//! IANA identificator of time zone.
	//! Note that timezone names under various OSes may be different than those used in Stellarium's
	//! location database (e.g. Ubuntu:Asia/Kolkata=Windows:Asia/Calcutta),
	//! which requires some translation effort during the loading process.
	//! After loading from the location DB, the ianaTimeZone should contain the full name which may differ from the database name.
	//  See LP:1662132
	// GZ renamed to more clearly indicate these are IANA names.
	// Besides IANA names, other valid names seem to be "LMST", "LTST" and "system_default".
	QString ianaTimeZone;

	//! Parse a location from a line serialization
	static StelLocation createFromLine(const QString& line);

	//! Compute great-circle distance between two locations on a spherical body
	//! arguments given in decimal degrees
	static float distanceDegrees(const float long1, const float lat1, const float long2, const float lat2);
	//! Compute great-circle distance between two locations on the current planet (takes flattening into account)
	//! arguments given in decimal degrees
	//! Source: Jean Meeus, Astronomical Algorithms, 2nd edition, ch.11.
	static double distanceKm(Planet *planet, const double long1, const double lat1, const double long2, const double lat2);
	//! Compute azimuth from Obs towards Target. All angles (args and result) are in degrees.
	//! @return azimuth counted from north or south as set in the StelApp preferences, in [0...360].
	static double getAzimuthForLocation(double longObs, double latObs, double longTarget, double latTarget);

	//! Used privately by the StelLocationMgr
	bool isUserLocation;

	static const float DEFAULT_LIGHT_POLLUTION_LUMINANCE;
private:
	static QString getRegionFromCode(const QString& code);
	//Register with Qt
	static int metaTypeId;
	static int initMetaType();
	//! Longitude in degree
	float longitude;
	//! Latitude in degree
	float latitude;
};

Q_DECLARE_METATYPE(StelLocation)

//! Serialize the passed StelLocation into a binary blob.
QDataStream& operator<<(QDataStream& out, const StelLocation& loc);
//! Load the StelLocation from a binary blob.
QDataStream& operator>>(QDataStream& in, StelLocation& loc);

#endif // STELLOCATION_HPP

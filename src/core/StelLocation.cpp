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

#include "StelLocation.hpp"
#include "StelLocaleMgr.hpp"
#include "StelUtils.hpp"
#include <QStringList>

const int StelLocation::DEFAULT_BORTLE_SCALE_INDEX = 2;

int StelLocation::metaTypeId = initMetaType();
int StelLocation::initMetaType()
{
	return qRegisterMetaType<StelLocation>();
}

// Output the location as a string ready to be stored in the user_location file
QString StelLocation::serializeToLine() const
{
	return QString("%1\t%2\t%3\t%4\t%5\t%6\t%7\t%8\t%9\t%10\t%11\t%12")
			.arg(name)
			.arg(state)
			.arg(country)
			.arg(role)
			.arg(population/1000)
			.arg(latitude<0 ? QString("%1S").arg(-latitude, 0, 'f', 6) : QString("%1N").arg(latitude, 0, 'f', 6))
			.arg(longitude<0 ? QString("%1W").arg(-longitude, 0, 'f', 6) : QString("%1E").arg(longitude, 0, 'f', 6))
			.arg(altitude)
			.arg(bortleScaleIndex)
			.arg("")		// Reserve for time zone
			.arg(planetName)
			.arg(landscapeKey);
}

QString StelLocation::getID() const
{
	if (name.isEmpty())
	{
		return QString("%1,%2").arg(latitude).arg(longitude);
	}
	QString ret = name;
	if (!country.isEmpty())
		ret += ", " + country;
	return ret;
}

QDataStream& operator<<(QDataStream& out, const StelLocation& loc)
{
	out << loc.name << loc.state << loc.country << loc.role << loc.population << loc.latitude << loc.longitude << loc.altitude << loc.bortleScaleIndex << loc.planetName << loc.landscapeKey << loc.isUserLocation;
	return out;
}

QDataStream& operator>>(QDataStream& in, StelLocation& loc)
{
	in >> loc.name >> loc.state >> loc.country >> loc.role >> loc.population >> loc.latitude >> loc.longitude >> loc.altitude >> loc.bortleScaleIndex >> loc.planetName >> loc.landscapeKey >> loc.isUserLocation;
	return in;
}

// Parse a location from a line serialization
StelLocation StelLocation::createFromLine(const QString& rawline)
{
	StelLocation loc;
	const QStringList& splitline = rawline.split("\t");
	loc.name    = splitline.at(0);
	loc.state   = splitline.at(1);
	loc.country = StelLocaleMgr::countryCodeToString(splitline.at(2));
	if (loc.country.isEmpty())
		loc.country = splitline.at(2);

	loc.role    = splitline.at(3).at(0);
	if (loc.role == '\0')
		loc.role = 'X';
	loc.population = (int) (splitline.at(4).toFloat()*1000);

	const QString& latstring = splitline.at(5);
	loc.latitude = latstring.left(latstring.size() - 1).toFloat();
	if (latstring.endsWith('S'))
		loc.latitude=-loc.latitude;

	const QString& lngstring = splitline.at(6);
	loc.longitude = lngstring.left(lngstring.size() - 1).toFloat();
	if (lngstring.endsWith('W'))
		loc.longitude=-loc.longitude;

	loc.altitude = (int)splitline.at(7).toFloat();

	if (splitline.size()>8)
	{
		bool ok;
		loc.bortleScaleIndex = splitline.at(8).toInt(&ok);
		if (ok==false)
			loc.bortleScaleIndex = DEFAULT_BORTLE_SCALE_INDEX;
	}
	else
		loc.bortleScaleIndex = DEFAULT_BORTLE_SCALE_INDEX;

	// Reserve for TimeZone
	// if (splitline.size()>9) {}

	if (splitline.size()>10)
	{
		// Parse planet name
		loc.planetName = splitline.at(10);
	}
	else
	{
		// Earth by default
		loc.planetName = "Earth";
	}

	if (splitline.size()>11)
	{
		// Parse optional associated landscape key
		loc.landscapeKey = splitline.at(11);
	}
	return loc;
}

// Compute great-circle distance between two locations
float StelLocation::distanceDegrees(const float long1, const float lat1, const float long2, const float lat2)
{
	const float DEGREES=M_PI/180.0f;
	return std::acos( std::sin(lat1*DEGREES)*std::sin(lat2*DEGREES) +
					  std::cos(lat1*DEGREES)*std::cos(lat2*DEGREES)*std::cos((long1-long2)*DEGREES) ) / DEGREES;
}

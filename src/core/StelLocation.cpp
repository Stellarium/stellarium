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
#include "StelLocationMgr.hpp"
#include "StelLocaleMgr.hpp"
#include "StelUtils.hpp"
#include "StelModuleMgr.hpp"
#include "StelApp.hpp"
#include "Planet.hpp"
#include "SolarSystem.hpp"
#include <QTimeZone>
#include <QStringList>

const int StelLocation::DEFAULT_BORTLE_SCALE_INDEX = 2;

int StelLocation::metaTypeId = initMetaType();
int StelLocation::initMetaType()
{
	int id = qRegisterMetaType<StelLocation>();
	qRegisterMetaTypeStreamOperators<StelLocation>();
	return id;
}

// Output the location as a string ready to be stored in the user_location file
QString StelLocation::serializeToLine() const
{
	QString sanitizedTZ=StelLocationMgr::sanitizeTimezoneStringForLocationDB(ianaTimeZone);
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
			.arg(sanitizedTZ)
			.arg(planetName)
			.arg(landscapeKey);
}

QString StelLocation::getID() const
{
	if (name.isEmpty())
		return QString("%1, %2").arg(latitude).arg(longitude);

	if (!country.isEmpty())
		return QString("%1, %2").arg(name).arg(country);
	else
		return name;
}

// GZ TODO: These operators may require sanitizing for timezone names!
QDataStream& operator<<(QDataStream& out, const StelLocation& loc)
{
	out << loc.name << loc.state << loc.country << loc.role << loc.population << loc.latitude << loc.longitude << loc.altitude << loc.bortleScaleIndex << loc.ianaTimeZone << loc.planetName << loc.landscapeKey << loc.isUserLocation;
	return out;
}

QDataStream& operator>>(QDataStream& in, StelLocation& loc)
{
	in >> loc.name >> loc.state >> loc.country >> loc.role >> loc.population >> loc.latitude >> loc.longitude >> loc.altitude >> loc.bortleScaleIndex >> loc.ianaTimeZone >> loc.planetName >> loc.landscapeKey >> loc.isUserLocation;
	return in;
}

// Parse a location from a line serialization
StelLocation StelLocation::createFromLine(const QString& rawline)
{
	StelLocation loc;
	const QStringList& splitline = rawline.split("\t");
	loc.name    = splitline.at(0).trimmed();
	loc.state   = splitline.at(1).trimmed();
	loc.country = StelLocaleMgr::countryCodeToString(splitline.at(2).trimmed());	
	if (loc.country.isEmpty())
		loc.country = splitline.at(2).trimmed();

	loc.role    = splitline.at(3).at(0).toUpper();
	if (loc.role.isNull())
		loc.role = QChar(0x0058); // char 'X'
	loc.population = static_cast<int> (splitline.at(4).toFloat()*1000);

	const QString& latstring = splitline.at(5).trimmed();
	loc.latitude = latstring.left(latstring.size() - 1).toFloat();
	if (latstring.endsWith('S'))
		loc.latitude=-loc.latitude;

	const QString& lngstring = splitline.at(6).trimmed();
	loc.longitude = lngstring.left(lngstring.size() - 1).toFloat();
	if (lngstring.endsWith('W'))
		loc.longitude=-loc.longitude;

	loc.altitude = static_cast<int>(splitline.at(7).toFloat());

	if (splitline.size()>8)
	{
		bool ok;
		loc.bortleScaleIndex = splitline.at(8).toInt(&ok);
		if (ok==false)
			loc.bortleScaleIndex = DEFAULT_BORTLE_SCALE_INDEX;
	}
	else
		loc.bortleScaleIndex = DEFAULT_BORTLE_SCALE_INDEX;

	if (splitline.size()>9)
	{
		// Parse time zone
		loc.ianaTimeZone = splitline.at(9).trimmed();
		// GZ Check whether the timezone ID is available in the current Qt/IANA list?
		if ( ! QTimeZone::isTimeZoneIdAvailable(loc.ianaTimeZone.toUtf8()))
		{
			// Try to find a currently used IANA string from our known replacements.
			QString fitName=StelLocationMgr::sanitizeTimezoneStringFromLocationDB(loc.ianaTimeZone);
			qDebug() << "StelLocation::createFromLine(): TimeZone name for " << loc.name << " not found. Translating" << loc.ianaTimeZone << " to " << fitName;
			loc.ianaTimeZone=fitName;
		}
	}

	if (splitline.size()>10)
	{
		// Parse planet name
		loc.planetName = splitline.at(10).trimmed();
	}
	else
	{
		// Earth by default
		loc.planetName = "Earth";
	}

	if (splitline.size()>11)
	{
		// Parse optional associated landscape key
		loc.landscapeKey = splitline.at(11).trimmed();
	}
	return loc;
}

// Compute great-circle distance between two locations
float StelLocation::distanceDegrees(const float long1, const float lat1, const float long2, const float lat2)
{
	static const float DEGREES=M_PIf/180.0f;
	return std::acos( std::sin(lat1*DEGREES)*std::sin(lat2*DEGREES) +
			  std::cos(lat1*DEGREES)*std::cos(lat2*DEGREES) *
			  std::cos((long1-long2)*DEGREES) ) / DEGREES;
}
double StelLocation::distanceKm(Planet *planet, const double long1, const double lat1, const double long2, const double lat2)
{
	static const double DEGREES=M_PI/180.0;
	const double f = 1.0 - planet->getOneMinusOblateness(); // flattening
	const double a = planet->getEquatorialRadius()*AU;

	const double F = (lat1+lat2)*0.5*DEGREES;
	const double G = (lat1-lat2)*0.5*DEGREES;
	const double L = (long1-long2)*0.5*DEGREES;

	const double sinG=sin(G), cosG=cos(G), sinF=sin(F), cosF=cos(F), sinL=sin(L), cosL=cos(L);
	const double S  = sinG*sinG*cosL*cosL+cosF*cosF*sinL*sinL;
	const double C  = cosG*cosG*cosL*cosL+sinF*sinF*sinL*sinL;
	const double om = atan(sqrt(S/C));
	const double R  = sqrt(S*C)/om;
	const double D  = 2.*om*a;
	const double H1 = (3.0*R-1.0)/(2.0*C);
	const double H2 = (3.0*R+1.0)/(2.0*S);
	return D*(1.0+f*(H1*sinF*sinF*cosG*cosG-H2*cosF*cosF*sinG*sinG));
}
double StelLocation::distanceKm(const double otherLong, const double otherLat) const
{
	PlanetP planet=GETSTELMODULE(SolarSystem)->searchByEnglishName(planetName);
	return distanceKm(planet.data(), static_cast<double>(longitude), static_cast<double>(latitude), otherLong, otherLat);
}

double StelLocation::getAzimuthForLocation(double longObs, double latObs, double longTarget, double latTarget)
{
	longObs    *= (M_PI/180.0);
	latObs     *= (M_PI/180.0);
	longTarget *= (M_PI/180.0);
	latTarget  *= (M_PI/180.0);

	double az = atan2(sin(longTarget-longObs), cos(latObs)*tan(latTarget)-sin(latObs)*cos(longTarget-longObs));
	if (StelApp::getInstance().getFlagSouthAzimuthUsage())
		az += M_PI;
	return StelUtils::fmodpos((180.0/M_PI) * az, 360.0);
}

double StelLocation::getAzimuthForLocation(double longTarget, double latTarget) const
{
	return getAzimuthForLocation(static_cast<double>(longitude), static_cast<double>(latitude), longTarget, latTarget);
}

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
#include "StelCore.hpp"
#include "StelLocationMgr.hpp"
#include "StelUtils.hpp"
#include "StelModuleMgr.hpp"
#include "StelApp.hpp"
#include "Planet.hpp"
#include "SolarSystem.hpp"
#include <QTimeZone>
#include <QStringList>

const float StelLocation::DEFAULT_LIGHT_POLLUTION_LUMINANCE = StelCore::bortleScaleIndexToLuminance(2);

int StelLocation::metaTypeId = initMetaType();
int StelLocation::initMetaType()
{
	int id = qRegisterMetaType<StelLocation>();
#if (QT_VERSION<QT_VERSION_CHECK(6,0,0))
	qRegisterMetaTypeStreamOperators<StelLocation>();
#endif
	return id;
}

StelLocation::StelLocation(const QString &lName, const QString &lState, const QString &lRegion, const float lng, const float lat, const int alt,
						   const int populationK, const QString &timeZone, const int bortleIndex, const QChar roleKey, const QString &landscapeID)
	: name(lName)
	, region(lRegion)
	, state(lState)
	, planetName("Earth")
	, altitude(alt)
	, lightPollutionLuminance(StelCore::bortleScaleIndexToLuminance(bortleIndex))
	, landscapeKey(landscapeID)
	, population(populationK)
	, role(roleKey)
	, ianaTimeZone(timeZone)
	, isUserLocation(true)
	, longitude(lng)
	, latitude(lat)
{
}

// Output the location as a string ready to be stored in the user_location file
QString StelLocation::serializeToLine() const
{
	QString sanitizedTZ=StelLocationMgr::sanitizeTimezoneStringForLocationDB(ianaTimeZone);
	return QString("%1\t%2\t%3\t%4\t%5\t%6\t%7\t%8\t%9\t%10\t%11\t%12").arg(
			name,
			state,
			region,
			role,
			QString::number(population/1000),
			latitude<0 ? QString("%1S").arg(-latitude, 0, 'f', 6) : QString("%1N").arg(latitude, 0, 'f', 6),
			longitude<0 ? QString("%1W").arg(-longitude, 0, 'f', 6) : QString("%1E").arg(longitude, 0, 'f', 6),
			QString::number(altitude),
            QString::number(lightPollutionLuminance.isValid() ? lightPollutionLuminance.toFloat() : DEFAULT_LIGHT_POLLUTION_LUMINANCE)
            ).arg(
			sanitizedTZ,
			planetName,
			landscapeKey);
}

QString StelLocation::getID() const
{
	if (name.isEmpty())
		return QString("%1, %2").arg(latitude).arg(longitude);

	if (!region.isEmpty())
		return QString("%1, %2").arg(name, region);
	else
		return name;
}

float StelLocation::getLatitude(bool suppressObserver)  const
{
	if (!suppressObserver && role==QChar('o'))
		return 90.f;
	else
		return latitude;
}

float StelLocation::getLongitude(bool suppressObserver) const
{
	if (!suppressObserver && role==QChar('o'))
		return 0.f;
	else
		return longitude;
}

// GZ TODO: These operators may require sanitizing for timezone names!
QDataStream& operator<<(QDataStream& out, const StelLocation& loc)
{
	const auto lum = loc.lightPollutionLuminance.toFloat();
	const int bortleScaleIndex = loc.lightPollutionLuminance.isValid() ? StelCore::luminanceToBortleScaleIndex(lum) : -1;
	out << loc.name << loc.state << loc.region << loc.role << loc.population << loc.getLatitude() << loc.getLongitude() << loc.altitude << bortleScaleIndex << loc.ianaTimeZone << loc.planetName << loc.landscapeKey << loc.isUserLocation;
	return out;
}

QDataStream& operator>>(QDataStream& in, StelLocation& loc)
{
	int bortleScaleIndex;
	float lng, lat;
	in >> loc.name >> loc.state >> loc.region >> loc.role >> loc.population >> lat >> lng >> loc.altitude >> bortleScaleIndex >> loc.ianaTimeZone >> loc.planetName >> loc.landscapeKey >> loc.isUserLocation;
	loc.setLongitude(lng);
	loc.setLatitude(lat);
	if(bortleScaleIndex > 0)
		loc.lightPollutionLuminance = StelCore::bortleScaleIndexToLuminance(bortleScaleIndex);
	return in;
}

QString StelLocation::getRegionFromCode(const QString &code)
{
	QString region;
	if (code.toInt()>0) // OK, this is code of geographical region
		region = StelLocationMgr::pickRegionFromCode(code.toInt());
	else
	{
		if (code.size() == 2) // The string equals to 2 chars - this is the ISO 3166-1 alpha 2 code for country
			region = StelLocationMgr::pickRegionFromCountryCode(code);
		else
			region = StelLocationMgr::pickRegionFromCountry(code); // This is the English name of country
	}
	if (region.isEmpty())
		region = code; // OK, this is just region

	return region;
}

// Parse a location from a line serialization
StelLocation StelLocation::createFromLine(const QString& rawline)
{
	StelLocation loc;
	const QStringList& splitline = rawline.split("\t");
	loc.name    = splitline.at(0).trimmed();
	loc.state   = splitline.at(1).trimmed();
	loc.region = getRegionFromCode(splitline.at(2).trimmed());
	loc.role    = splitline.at(3).at(0).toUpper();
	if (loc.role.isNull())
		loc.role = 'X';
	loc.population = static_cast<int> (splitline.at(4).toFloat()*1000);

	const QString& latstring = splitline.at(5).trimmed();
#if (QT_VERSION>=QT_VERSION_CHECK(6,0,0))
	loc.latitude = latstring.left(latstring.size() - 1).toFloat();
#else
	loc.latitude = latstring.leftRef(latstring.size() - 1).toFloat();
#endif
	if (latstring.endsWith('S'))
		loc.latitude=-loc.latitude;

	const QString& lngstring = splitline.at(6).trimmed();
#if (QT_VERSION>=QT_VERSION_CHECK(6,0,0))
	loc.longitude = lngstring.left(lngstring.size() - 1).toFloat();
#else
	loc.longitude = lngstring.leftRef(lngstring.size() - 1).toFloat();
#endif
	if (lngstring.endsWith('W'))
		loc.longitude=-loc.longitude;

	loc.altitude = static_cast<int>(splitline.at(7).toFloat());

	if (splitline.size()>8)
	{
		bool ok;
		const auto bortleScaleIndex = splitline.at(8).toInt(&ok);
		if(bortleScaleIndex > 0)
			loc.lightPollutionLuminance = StelCore::bortleScaleIndexToLuminance(bortleScaleIndex);
		if (ok==false)
			loc.lightPollutionLuminance = DEFAULT_LIGHT_POLLUTION_LUMINANCE;
	}
	else
		loc.lightPollutionLuminance = DEFAULT_LIGHT_POLLUTION_LUMINANCE;

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

	// Parse planet name
	if (splitline.size()>10)
		loc.planetName = splitline.at(10).trimmed();
	else
		loc.planetName = "Earth"; // Earth by default

	// Parse optional associated landscape key
	if (splitline.size()>11)
		loc.landscapeKey = splitline.at(11).trimmed();

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
	longObs    *= M_PI_180;
	latObs     *= M_PI_180;
	longTarget *= M_PI_180;
	latTarget  *= M_PI_180;

	double az = atan2(sin(longTarget-longObs), cos(latObs)*tan(latTarget)-sin(latObs)*cos(longTarget-longObs));
	if (StelApp::getInstance().getFlagSouthAzimuthUsage())
		az += M_PI;
	return StelUtils::fmodpos(M_180_PI * az, 360.0);
}

double StelLocation::getAzimuthForLocation(double longTarget, double latTarget) const
{
	return getAzimuthForLocation(static_cast<double>(longitude), static_cast<double>(latitude), longTarget, latTarget);
}

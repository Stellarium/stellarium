/*
 * Stellarium
 * Copyright (C) 2009, 2012 Matthew Gates
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

#include "Satellite.hpp"
#include "StelObject.hpp"
#include "StelObjectMgr.hpp"
#include "StelPainter.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelTexture.hpp"
#include "VecMath.hpp"
#include "StelUtils.hpp"
#include "StelTranslator.hpp"
#include "StelModuleMgr.hpp"
#include "StelLocaleMgr.hpp"

#include <QTextStream>
#include <QDebug>
#include <QVariant>
#include <QSettings>
#include <QByteArray>

#include <QVector3D>
#include <QMatrix4x4>

#include "gsatellite/gTime.hpp"
//#include "gsatellite/stdsat.h"

#include <cmath>

#define sqr(a) ((a)*(a))

const QString Satellite::SATELLITE_TYPE = QStringLiteral("Satellite");

// static data members - will be initialised in the Satellites class (the StelObjectMgr)
StelTextureSP Satellite::hintTexture;
bool Satellite::showLabels = true;
float Satellite::hintBrightness = 0.f;
float Satellite::hintScale = 1.f;
SphericalCap Satellite::viewportHalfspace = SphericalCap();
int Satellite::orbitLineSegments = 90;
int Satellite::orbitLineFadeSegments = 4;
int Satellite::orbitLineSegmentDuration = 20;
int Satellite::orbitLineThickness = 1;
bool Satellite::orbitLinesFlag = true;
bool Satellite::iconicModeFlag = false;
bool Satellite::hideInvisibleSatellitesFlag = false;
bool Satellite::coloredInvisibleSatellitesFlag = true;
Vec3f Satellite::invisibleSatelliteColor = Vec3f(0.2f,0.2f,0.2f);
Vec3f Satellite::transitSatelliteColor = Vec3f(0.f,0.f,0.f);
double Satellite::timeRateLimit = 1.0; // one JD per second by default
int Satellite::tleEpochAge = 30; // default age of TLE's epoch to mark TLE as outdated (used for filters)

bool Satellite::flagCFKnownStdMagnitude = false;
bool Satellite::flagCFApogee = false;
double Satellite::minCFApogee = 20000.;
double Satellite::maxCFApogee = 55000.;
bool Satellite::flagCFPerigee = false;
double Satellite::minCFPerigee = 200.;
double Satellite::maxCFPerigee = 1500.;
bool Satellite::flagCFEccentricity = false;
double Satellite::minCFEccentricity = 0.3;
double Satellite::maxCFEccentricity = 0.9;
bool Satellite::flagCFPeriod = false;
double Satellite::minCFPeriod = 0.;
double Satellite::maxCFPeriod = 150.;
bool Satellite::flagCFInclination = false;
double Satellite::minCFInclination = 120.;
double Satellite::maxCFInclination = 360.;
bool Satellite::flagCFRCS = false;
double Satellite::minCFRCS = 0.1;
double Satellite::maxCFRCS = 100.;
bool Satellite::flagVFAltitude = false;
double Satellite::minVFAltitude = 200.;
double Satellite::maxVFAltitude = 500.;
bool Satellite::flagVFMagnitude = false;
double Satellite::minVFMagnitude = 8.;
double Satellite::maxVFMagnitude = -8.;

#if (SATELLITES_PLUGIN_IRIDIUM == 1)
double Satellite::sunReflAngle = 180.;
//double Satellite::timeShift = 0.;
#endif

Satellite::Satellite(const QString& identifier, const QVariantMap& map)
	: initialized(false)
	, displayed(false)
	, orbitDisplayed(false)
	, userDefined(false)
	, newlyAdded(false)
	, orbitValid(false)
	, jdLaunchYearJan1(0)
	, stdMag(99.)
	, RCS(-1.)
	, status(StatusUnknown)
	, height(0.)
	, range(0.)
	, rangeRate(0.)
	, hintColor(0.f,0.f,0.f)
	, lastUpdated()	
	, isISS(false)	
	, pSatWrapper(nullptr)
	, visibility(gSatWrapper::UNKNOWN)
	, phaseAngle(0.)
	, infoColor(0.f,0.f,0.f)
	, orbitColor(0.f,0.f,0.f)
	, lastEpochCompForOrbit(0.)
	, epochTime(0.)
{
	// return initialized if the mandatory fields are not present
	if (identifier.isEmpty())
		return;
	if (!map.contains("name") || !map.contains("tle1") || !map.contains("tle2"))
		return;

	id = identifier;
	name  = map.value("name").toString();
	if (name.isEmpty())
		return;
	
	// If there are no such keys, these will be initialized with the default
	// values given them above.
	description = map.value("description", description).toString().trimmed();
	displayed = map.value("visible", displayed).toBool();
	orbitDisplayed = map.value("orbitVisible", orbitDisplayed).toBool();
	userDefined = map.value("userDefined", userDefined).toBool();
	stdMag = map.value("stdMag", 99.).toDouble();
	RCS = map.value("rcs", -1.).toDouble();
	status = map.value("status", StatusUnknown).toInt();
	// Satellite hint color
	QVariantList list = map.value("hintColor", QVariantList()).toList();
	if (list.count() == 3)
	{
		hintColor[0] = list.at(0).toFloat();
		hintColor[1] = list.at(1).toFloat();
		hintColor[2] = list.at(2).toFloat();
	}

	// Satellite orbit section color
	list = map.value("orbitColor", QVariantList()).toList();
	if (list.count() == 3)
	{
		orbitColor[0] = list.at(0).toFloat();
		orbitColor[1] = list.at(1).toFloat();
		orbitColor[2] = list.at(2).toFloat();
	}
	else
	{
		orbitColor = hintColor;
	}

	// Satellite info color
	list = map.value("infoColor", QVariantList()).toList();
	if (list.count() == 3)
	{
		infoColor[0] = list.at(0).toFloat();
		infoColor[1] = list.at(1).toFloat();
		infoColor[2] = list.at(2).toFloat();
	}
	else
	{
		infoColor = hintColor;
	}

	if (map.contains("comms"))
	{
		for (const auto& comm : map.value("comms").toList())
		{
			QVariantMap commMap = comm.toMap();
			CommLink c;
			if (commMap.contains("frequency")) c.frequency = commMap.value("frequency").toDouble();
			if (commMap.contains("modulation")) c.modulation = commMap.value("modulation").toString();
			if (commMap.contains("description")) c.description = commMap.value("description").toString();
			comms.append(c);
		}
	}

	QVariantList groupList =  map.value("groups", QVariantList()).toList();
	if (!groupList.isEmpty())
	{
		for (const auto& group : qAsConst(groupList))
			groups.insert(group.toString());
	}

	// TODO: Somewhere here - some kind of TLE validation.
	QString line1 = map.value("tle1").toString();
	QString line2 = map.value("tle2").toString();
	setNewTleElements(line1, line2);
	// This also sets the international designator and launch year.

	QString dateString = map.value("lastUpdated").toString();
	if (!dateString.isEmpty())
		lastUpdated = QDateTime::fromString(dateString, Qt::ISODate);

	orbitValid = true;
	initialized = true;
	isISS = (name=="ISS" || name=="ISS (ZARYA)" || name=="ISS (NAUKA)");
	moon = GETSTELMODULE(SolarSystem)->getMoon();
	sun = GETSTELMODULE(SolarSystem)->getSun();

	visibilityDescription={
		{ gSatWrapper::RADAR_SUN,	N_("The satellite and the observer are in sunlight") },
		{ gSatWrapper::VISIBLE,		N_("The satellite is sunlit and the observer is in the dark") },
		{ gSatWrapper::RADAR_NIGHT,	N_("The satellite isn't sunlit") },
		{ gSatWrapper::PENUMBRAL,	N_("The satellite is in penumbra") },
		{ gSatWrapper::ANNULAR,		N_("The satellite is eclipsed") },
		{ gSatWrapper::BELOW_HORIZON,   N_("The satellite is below the horizon") }
	};

	update(0.);
}

Satellite::~Satellite()
{
	if (pSatWrapper)
	{
		delete pSatWrapper;
		pSatWrapper = nullptr;
	}
}

double Satellite::roundToDp(float n, int dp)
{
	// round n to dp decimal places
	return floor(static_cast<double>(n) * pow(10., dp) + .5) / pow(10., dp);
}

QString Satellite::getNameI18n() const
{
	return q_(name);
}

QVariantMap Satellite::getMap(void)
{
	QVariantMap map;
	map["name"] = name;	
	map["stdMag"] = stdMag;
	map["rcs"] = RCS;
	map["status"] = status;
	map["tle1"] = tleElements.first;
	map["tle2"] = tleElements.second;

	if (!description.isEmpty())
		map["description"] = description;

	map["visible"] = displayed;
	map["orbitVisible"] = orbitDisplayed;
	if (userDefined)
		map.insert("userDefined", userDefined);
	QVariantList col, orbitCol, infoCol;
	col << roundToDp(hintColor[0],3) << roundToDp(hintColor[1], 3) << roundToDp(hintColor[2], 3);
	orbitCol << roundToDp(orbitColor[0], 3) << roundToDp(orbitColor[1], 3) << roundToDp(orbitColor[2],3);
	infoCol << roundToDp(infoColor[0], 3) << roundToDp(infoColor[1], 3) << roundToDp(infoColor[2],3);
	map["hintColor"] = col;
	map["orbitColor"] = orbitCol;
	map["infoColor"] = infoCol;
	QVariantList commList;
	for (const auto& c : qAsConst(comms))
	{
		QVariantMap commMap;
		commMap["frequency"] = c.frequency;
		if (!c.modulation.isEmpty()) commMap["modulation"] = c.modulation;
		if (!c.description.isEmpty()) commMap["description"] = c.description;
		commList << commMap;
	}
	map["comms"] = commList;
	QVariantList groupList;
	for (const auto& g : qAsConst(groups))
	{
		groupList << g;
	}
	map["groups"] = groupList;

	if (!lastUpdated.isNull())
	{
		// A raw QDateTime is not a recognised JSON data type. --BM
		map["lastUpdated"] = lastUpdated.toString(Qt::ISODate);
	}

	return map;
}

float Satellite::getSelectPriority(const StelCore* core) const
{
	float limit = -10.f;
	if (flagVFAltitude) // the visual filter is enabled
		limit = (minVFAltitude<=height && height<=maxVFAltitude) ? -10.f : 50.f;

	if (flagVFMagnitude) // the visual filter is enabled
		limit = ((maxVFMagnitude<=getVMagnitude(core) && getVMagnitude(core)<=minVFMagnitude) && (stdMag<99. || RCS>0.)) ? -10.f : 50.f;

	return limit;
}

QString Satellite::getInfoString(const StelCore *core, const InfoStringGroup& flags) const
{
	QString str;
	QTextStream oss(&str);
	QString degree = QChar(0x00B0);
	const bool withDecimalDegree = StelApp::getInstance().getFlagShowDecimalDegrees();
	
	if (flags & Name)
	{
		oss << "<h2>" << getNameI18n() << "</h2>";
		if (!description.isEmpty())
		{
			// Let's convert possible \n chars into <br/> in description of satellite
			oss << q_(description).replace("\n", "<br/>") << "<br/>";
		}
	}
	
	if (flags & CatalogNumber)
	{
		QString catalogNumbers;
		if (internationalDesignator.isEmpty())
			catalogNumbers = QString("NORAD %1").arg(id);
		else
			catalogNumbers = QString("NORAD %1; %2 (COSPAR/NSSDC): %3").arg(id, q_("International Designator"), internationalDesignator);
		oss << catalogNumbers << "<br/><br/>";
	}

	if (flags & ObjectType)
		oss << QString("%1: <b>%2</b>").arg(q_("Type"), getObjectTypeI18n())  << "<br/>";
	
	if ((flags & Magnitude) && ((stdMag<99.) || (RCS>0.)) && (visibility==gSatWrapper::VISIBLE))
	{
		const int decimals = 2;
		const float airmass = getAirmass(core);
		oss << QString("%1: <b>%2</b>").arg(q_("Approx. magnitude"), QString::number(getVMagnitude(core), 'f', decimals));
		if (airmass>-1.f) // Don't show extincted magnitude much below horizon where model is meaningless.
			oss << QString(" (%1 <b>%2</b> %3 <b>%4</b> %5)").arg(q_("reduced to"), QString::number(getVMagnitudeWithExtinction(core), 'f', decimals), q_("by"), QString::number(airmass, 'f', decimals), q_("Airmasses"));
		oss << "<br />";
	}

	// Ra/Dec etc.
	oss << getCommonInfoString(core, flags);

	if (flags&Distance)
	{
		QString km = qc_("km", "distance");
		// TRANSLATORS: Slant range: distance between the satellite and the observer
		oss << QString("%1: %2 %3").arg(q_("Range")).arg(qRound(range)).arg(km) << "<br/>";
		// TRANSLATORS: Rate at which the distance changes
		oss << QString("%1: %2 %3").arg(q_("Range rate")).arg(rangeRate, 5, 'f', 3).arg(qc_("km/s", "speed")) << "<br/>";
		// TRANSLATORS: Satellite altitude
		oss << QString("%1: %2 %3").arg(q_("Altitude")).arg(qRound(height)).arg(km) << "<br/>";
		oss << QString("%1 (WGS-84): %2 %3 / %4 %5").arg(q_("Perigee/apogee altitudes")).arg(qRound(perigee)).arg(km).arg(qRound(apogee)).arg(km) << "<br/>";
	}

	if (flags&Size && RCS>0.)
	{
		const double angularSize = getAngularRadius(core)*2.*M_PI_180;
		QString sizeStr = "";
		if (withDecimalDegree)
			sizeStr = StelUtils::radToDecDegStr(angularSize, 5, false, true);
		else
			sizeStr = StelUtils::radToDmsPStr(angularSize, 2);
		oss << QString("%1: %2").arg(q_("Approx. angular size"), sizeStr) << "<br />";
	}

	if (flags & Extra)
	{
		double orbitalPeriod = pSatWrapper->getOrbitalPeriod();
		if (orbitalPeriod>0.0)
		{
			// TRANSLATORS: Revolutions per day - measurement of the frequency of a rotation
			QString rpd = qc_("rpd","frequency");
			// TRANSLATORS: minutes - orbital period for artificial satellites
			QString mins = qc_("min", "period");
			oss << QString("%1: %2 %3 (%4 &mdash; %5 %6)")
			       .arg(q_("Orbital period")).arg(orbitalPeriod, 5, 'f', 2)
			       .arg(mins, StelUtils::hoursToHmsStr(orbitalPeriod/60.0, true))
			       .arg(1440.0/orbitalPeriod, 9, 'f', 5).arg(rpd) << "<br/>";
		}
		double inclination = pSatWrapper->getOrbitalInclination();
		oss << QString("%1: %2 (%3%4)").arg(q_("Inclination"), StelUtils::decDegToDmsStr(inclination), QString::number(inclination, 'f', 4), degree) << "<br/>";
		oss << QString("%1: %2&deg;/%3&deg;").arg(q_("SubPoint (Lat./Long.)")).arg(latLongSubPointPosition[0], 5, 'f', 2).arg(latLongSubPointPosition[1], 5, 'f', 3) << "<br/>";
		
		//TODO: This one can be done better
		const char* xyz = "<b>X:</b> %1, <b>Y:</b> %2, <b>Z:</b> %3";
		QString temeCoords = QString(xyz).arg(qRound(position[0])).arg(qRound(position[1])).arg(qRound(position[2]));
		// TRANSLATORS: TEME (True Equator, Mean Equinox) is an Earth-centered inertial coordinate system
		oss << QString("%1: %2 %3").arg(q_("TEME coordinates"), temeCoords, qc_("km", "distance")) << "<br/>";
		
		QString temeVel = QString(xyz).arg(velocity[0], 5, 'f', 2).arg(velocity[1], 5, 'f', 2).arg(velocity[2], 5, 'f', 2);
		// TRANSLATORS: TEME (True Equator, Mean Equinox) is an Earth-centered inertial coordinate system
		oss << QString("%1: %2 %3").arg(q_("TEME velocity"), temeVel, qc_("km/s", "speed")) << "<br/>";

		QString pha = StelApp::getInstance().getFlagShowDecimalDegrees() ?
				StelUtils::radToDecDegStr(phaseAngle,4,false,true) :
				StelUtils::radToDmsStr(phaseAngle, true);
		oss << QString("%1: %2").arg(q_("Phase angle"), pha) << "<br />";

#if (SATELLITES_PLUGIN_IRIDIUM == 1)
		if (sunReflAngle>0)
		{  // Iridium
			oss << QString("%1: %2%3").arg(q_("Sun reflection angle"))
			       .arg(sunReflAngle,0,'f',1)
			       .arg(degree);
			oss << "<br />";
		}
#endif
		QString updDate;
		if (!lastUpdated.isValid())
			updDate = qc_("unknown", "unknown date");
		else
		{
			QDate sd = lastUpdated.date();
			double hours = lastUpdated.time().hour() + lastUpdated.time().minute()/60. + lastUpdated.time().second()/3600.;
			updDate = QString("%1 %2 %3 %4 %5").arg(sd.day())
					.arg(StelLocaleMgr::longGenitiveMonthName(sd.month())).arg(sd.year())
					.arg(qc_("at","at time"), StelUtils::hoursToHmsStr(hours, true));
		}
		oss << QString("%1: %2").arg(q_("Last updated TLE"), updDate) << "<br />";
		oss << QString("%1: %2").arg(q_("Epoch of the TLE"), tleEpoch) << "<br />";
		if (RCS>0.)
			oss << QString("%1: %2 %3<sup>2</sup>").arg(q_("Radar cross-section (RCS)"), QString::number(RCS, 'f', 3), qc_("m","distance")) << "<br />";

		// Groups of the artificial satellites
		QStringList groupList;
		for (const auto&g : groups)
			groupList << q_(g);

		if (!groupList.isEmpty())
		{
			QString group = groups.count()>1 ? q_("Group") : q_("Groups");
			oss << QString("%1: %2").arg(group, groupList.join(", ")) << "<br />";
		}

		if (status!=StatusUnknown)
			oss << QString("%1: %2").arg(q_("Operational status"), getOperationalStatus()) << "<br />";
		//Visibility: Full text		
		oss << q_(visibilityDescription.value(visibility, "")) << "<br />";

		if (!comms.isEmpty())
		{
			oss << q_("Radio communication") << ":<br/>";
			for (const auto& c : comms)
			{
				oss << getCommLinkInfo(c);
			}
		}
		// Umbra circle info. May not generally be interesting.
		//oss << QString("%1: %2 km, %3: %4 km").arg(q_("Umbra distance"), QString::number(umbraDistance, 'f', 3), q_("Radius"), QString::number(umbraRadius, 'f', 3)) << "<br/>";
		//oss << QString("%1: %2 km, %3: %4 km").arg(q_("Penumbra distance"), QString::number(penumbraDistance, 'f', 3), q_("Radius"), QString::number(penumbraRadius, 'f', 3)) << "<br/>";
	}

	oss << getSolarLunarInfoString(core, flags);
	postProcessInfoString(str, flags);
	return str;
}

QString Satellite::getCommLinkInfo(CommLink comm) const
{
	QString commLinkData;

	if (!comm.modulation.isEmpty()) // OK, the signal modulation mode is exist
		commLinkData = comm.modulation;

	if (commLinkData.isEmpty()) // description cannot be empty!
		commLinkData = comm.description;
	else
		commLinkData.append(QString(" %1").arg(comm.description));

	if (commLinkData.isEmpty())
		return QString();

	// Translate some specific communications terms
	// See end of Satellites.cpp file to define translatable terms
	static const QStringList commTerms={ "uplink", "downlink", "beacon", "telemetry", "video", "broadband", "command" };
	for (auto& term: commTerms)
	{
		commLinkData.replace(term, q_(term));
	}
	commLinkData.replace("&", q_("and"));

	double dop = getDoppler(comm.frequency);
	double ddop = dop;
	QString sign;
	if (dop<0.)
	{
		sign='-';
		ddop*=-1;
	}
	else
		sign='+';

	commLinkData.append(QString(": %1 %2 (%3%4 %5)<br />").arg(QString::number(comm.frequency, 'f', 3), qc_("MHz", "frequency"), sign, QString::number(ddop, 'f', 3), qc_("kHz", "frequency")));

	return commLinkData;
}

// Calculate perigee and apogee altitudes for mean Earth radius
void Satellite::calculateSatDataFromLine2(QString tle)
{
	// Details: http://www.satobs.org/seesat/Dec-2002/0197.html
	const double k = 8681663.653;
	const double meanMotion = tle.left(63).right(11).toDouble();
	const double semiMajorAxis = std::cbrt((k/meanMotion)*(k/meanMotion));
	eccentricity = QString("0.%1").arg(tle.left(33).right(7)).toDouble();
	perigee = semiMajorAxis*(1.0 - eccentricity) - EARTH_RADIUS;
	apogee = semiMajorAxis*(1.0 + eccentricity) - EARTH_RADIUS;
	inclination = QString(tle.left(16).right(8)).toDouble();
}

// Calculate epoch of TLE
void Satellite::calculateEpochFromLine1(QString tle)
{
	QString epochStr;
	// Details: https://celestrak.org/columns/v04n03/ or https://en.wikipedia.org/wiki/Two-line_element_set
	int year = tle.left(20).right(2).toInt();
	if (year>=0 && year<57)
		year += 2000;
	else
		year += 1900;
	const double dayOfYear = tle.left(32).right(12).toDouble();
	QDate epoch = QDate(year, 1, 1).addDays(dayOfYear - 1);
	if (!epoch.isValid())
		epochStr = qc_("unknown", "unknown date");
	else
		epochStr = QString("%1 %2 %3, %4 UTC").arg(epoch.day())
				.arg(StelLocaleMgr::longGenitiveMonthName(epoch.month())).arg(year)
				.arg(StelUtils::hoursToHmsStr(24.*(dayOfYear-static_cast<int>(dayOfYear)), true));

	tleEpoch = epochStr;
	tleEpochJD = epoch.toJulianDay();
}

QVariantMap Satellite::getInfoMap(const StelCore *core) const
{
	QVariantMap map = StelObject::getInfoMap(core);

	map.insert("description", QString(description).replace("\n", " - "));
	map.insert("catalog", id);
	map.insert("tle1", tleElements.first);
	map.insert("tle2", tleElements.second);
	map.insert("tle-epoch", tleEpoch);

	if (!internationalDesignator.isEmpty())
		map.insert("international-designator", internationalDesignator);

	if (stdMag>98.) // replace whatever has been computed
	{
		map.insert("vmag", "?");
		map.insert("vmage", "?");
	}

	map.insert("range", range);
	map.insert("rangerate", rangeRate);
	map.insert("height", height);
	map.insert("subpoint-lat", latLongSubPointPosition[0]);
	map.insert("subpoint-long", latLongSubPointPosition[1]);
	map.insert("TEME-km-X", position[0]);
	map.insert("TEME-km-Y", position[1]);
	map.insert("TEME-km-Z", position[2]);
	map.insert("TEME-speed-X", velocity[0]);
	map.insert("TEME-speed-Y", velocity[1]);
	map.insert("TEME-speed-Z", velocity[2]);
	map.insert("inclination", pSatWrapper->getOrbitalInclination());
	map.insert("period", pSatWrapper->getOrbitalPeriod());
	map.insert("perigee-altitude", perigee);
	map.insert("apogee-altitude", apogee);
#if (SATELLITES_PLUGIN_IRIDIUM == 1)
	if (sunReflAngle>0.)
	{  // Iridium
		map.insert("sun-reflection-angle", sunReflAngle);
	}
#endif
	map.insert("operational-status", getOperationalStatus());
	map.insert("phase-angle", phaseAngle);
	map.insert("phase-angle-dms", StelUtils::radToDmsStr(phaseAngle));
	map.insert("phase-angle-deg", StelUtils::radToDecDegStr(phaseAngle));
	map.insert("visibility", visibilityDescription.value(visibility, ""));

	return map;
}

Vec3d Satellite::getJ2000EquatorialPos(const StelCore* core) const
{
	// Bugfix LP:1654331. I assume the elAzPosition has been computed without refraction! We must say this definitely.
	return core->altAzToJ2000(elAzPosition, StelCore::RefractionOff);
}

Vec3f Satellite::getInfoColor(void) const
{
	return infoColor;
}

float Satellite::getVMagnitude(const StelCore* core) const
{	
	Q_UNUSED(core)
	float vmag = 7.f; // Optimistic value of magnitude for artificial satellite without data for standard magnitude
	if (iconicModeFlag)
		vmag = 5.0;

	if (!iconicModeFlag && visibility != gSatWrapper::VISIBLE)
		vmag = 17.f; // Artificial satellite is invisible and 17 is hypothetical value of magnitude

	if (visibility==gSatWrapper::VISIBLE)
	{
#if(SATELLITES_PLUGIN_IRIDIUM == 1)
		sunReflAngle = -1.;
#endif
		if (pSatWrapper && name.startsWith("STARLINK"))
		{
			// Calculation of approx. visual magnitude for Starlink satellites
			// described here: http://www.satobs.org/seesat/Aug-2020/0079.html
			vmag = static_cast<float>(5.93 + 5 * std::log10 ( range / 1000 ));
			if (name.contains("DARKSAT", Qt::CaseInsensitive)) // See https://arxiv.org/abs/2006.08422
				vmag *= 0.78f;
		}
		else if (stdMag<99.) // OK, artificial satellite has value for standard magnitude
		{
			// Calculation of approx. visual magnitude for artificial satellites
			//
			// The standard magnitude may be an estimate based on the mean cross-
			// sectional area derived from its dimensions, or it may be a mean
			// value derived from visual observations. The former are denoted by a
			// letter "d" in column 37; the latter by a "v". To estimate the
			// magnitude at other ranges and illuminations, use the following formula:
			//
			// mag = stdmag - 15.75 + 2.5 * log10 (range * range / fracil)
			//
			// where : stdmag = standard magnitude as defined above
			//
			// range = distance from observer to satellite, km
			//
			// fracil = fraction of satellite illuminated,
			//	    [ 0 <= fracil <= 1 ]
			//
			// Original description: http://mmccants.org/tles/mccdesc.html

			double fracil = qMax(0.000001, static_cast<double>(calculateIlluminatedFraction()));

#if(SATELLITES_PLUGIN_IRIDIUM == 1)
			if (pSatWrapper && name.startsWith("IRIDIUM"))
			{
				Vec3d Sun3d = pSatWrapper->getSunECIPos();
				QVector3D sun(Sun3d.data()[0],Sun3d.data()[1],Sun3d.data()[2]);
				QVector3D sunN = sun; sunN.normalize();

				// position, velocity are known
				QVector3D Vx(velocity.data()[0],velocity.data()[1],velocity.data()[2]); Vx.normalize();

				Vec3d vy = (position^velocity);
				QVector3D Vy(vy.data()[0],vy.data()[1],vy.data()[2]); Vy.normalize();
				QVector3D Vz = QVector3D::crossProduct(Vx,Vy); Vz.normalize();

				// move this to constructor for optimizing
				QMatrix4x4 m0;
				m0.rotate(40, Vy);
				QVector3D Vx0 = m0.mapVector(Vx);

				QMatrix4x4 m[3];
				//m[2] = m[1] = m[0];
				m[0].rotate(0, Vz);
				m[1].rotate(120, Vz);
				m[2].rotate(-120, Vz);

				StelLocation loc   = StelApp::getInstance().getCore()->getCurrentLocation();
				const double  radLatitude    = loc.getLatitude() * KDEG2RAD;
				const double  theta          = pSatWrapper->getEpoch().toThetaLMST(loc.getLongitude() * KDEG2RAD);
				const double sinRadLatitude=sin(radLatitude);
				const double cosRadLatitude=cos(radLatitude);
				const double sinTheta=sin(theta);
				const double cosTheta=cos(theta);

				Vec3d observerECIPos;
				Vec3d observerECIVel;
				pSatWrapper->calcObserverECIPosition(observerECIPos, observerECIVel);

				sunReflAngle = 180.;
				QVector3D mirror;
				for (int i = 0; i<3; i++)
				{
					mirror = m[i].mapVector(Vx0);
					mirror.normalize();

					// reflection R = 2*(V dot N)*N - V
					QVector3D rsun =  2*QVector3D::dotProduct(sun,mirror)*mirror - sun;
					rsun = -rsun;
					Vec3d rSun(rsun.x(),rsun.y(),rsun.z());

					//Vec3d satECIPos  = getTEMEPos();
					Vec3d slantRange = rSun - observerECIPos;
					Vec3d topoRSunPos;
					//top_s
					topoRSunPos[0] = (sinRadLatitude * cosTheta * slantRange[0]
							+ sinRadLatitude * sinTheta * slantRange[1]
							- cosRadLatitude * slantRange[2]);
					//top_e
					topoRSunPos[1] = ((-1.0) * sinTheta * slantRange[0]
							+ cosTheta * slantRange[1]);

					//top_z
					topoRSunPos[2] = (cosRadLatitude * cosTheta * slantRange[0]
							+ cosRadLatitude * sinTheta * slantRange[1]
							+ sinRadLatitude * slantRange[2]);
					sunReflAngle = qMin(elAzPosition.angle(topoRSunPos) * KRAD2DEG, sunReflAngle) ;
				}

				// very simple flare model
				double iridiumFlare = 100;
				if (sunReflAngle<0.5)
					iridiumFlare = -8.92 + sunReflAngle*6;
				else	if (sunReflAngle<0.7)
					iridiumFlare = -5.92 + (sunReflAngle-0.5)*10;
				else
					iridiumFlare = -3.92 + (sunReflAngle-0.7)*5;

				 vmag = qMin(stdMag, iridiumFlare);
			}
			else // not Iridium
#endif
				vmag = static_cast<float>(stdMag);

			vmag += -15.75f + 2.5f * static_cast<float>(std::log10(range * range / fracil));
		}
		else if (RCS>0.) // OK, artificial satellite has RCS value and no standard magnitude
		{
			// Let's try calculate approx. magnitude from RCS value (see DOI: 10.1117/12.2014623)
			double albedo = 0.2;
			if (0.436<=phaseAngle && phaseAngle<1.745)
				albedo = (((((3.1765*phaseAngle - 22.0968)*phaseAngle + 62.182)*phaseAngle - 90.0993)*phaseAngle + 70.3031)*phaseAngle - 27.9227)*phaseAngle + 4.7373;
			else if (1.745<=phaseAngle && phaseAngle<=2.618)
				albedo = ((0.510905*phaseAngle - 2.72607)*phaseAngle + 4.96646)*phaseAngle - 3.02085;

			double rm = range*1000.;
			double fdiff = (std::sin(phaseAngle) + (M_PI - phaseAngle)*std::cos(phaseAngle))*(2.*albedo*RCS)/(3.* M_PI * M_PI * rm * rm);

			vmag = static_cast<float>(-26.74 - 2.5*std::log10(fdiff));
		}
	}
	return vmag;
}

// Calculate illumination fraction of artificial satellite
float Satellite::calculateIlluminatedFraction() const
{
	return (1.f + cos(static_cast<float>(phaseAngle)))*0.5f;
}

QString Satellite::getOperationalStatus() const
{
	const QMap<int,QString>map={
		{ StatusOperational,          qc_("operational", "operational status")},
		{ StatusNonoperational,       qc_("non-operational", "operational status")},
		{ StatusPartiallyOperational, qc_("partially operational", "operational status")},
		{ StatusStandby,              qc_("standby", "operational status")},
		{ StatusSpare,                qc_("spare", "operational status")},
		{ StatusExtendedMission,      qc_("extended mission", "operational status")},
		{ StatusDecayed,              qc_("decayed", "operational status")},
	};
	return map.value(status,              qc_("unknown", "operational status"));
}

double Satellite::getAngularRadius(const StelCore*) const
{
	double radius = 0.05 / 3600.; // assume 0.1 arcsecond default diameter
	if (RCS>0.)
	{
		double halfSize = isISS ?
			 109. * 0.5 :         // Special case: let's use max. size of ISS (109 meters: https://www.nasa.gov/feature/facts-and-figures)
			 std::sqrt(RCS/M_PI); // Let's assume spherical satellites/circular cross-section
		radius = std::atan(halfSize/(1000.*range))*M_180_PI; // Computing an angular size of artificial satellite ("halfSize" in metres, "range" in kilometres)
	}
	return radius;
}

void Satellite::setNewTleElements(const QString& tle1, const QString& tle2)
{
	if (pSatWrapper)
	{
		gSatWrapper *old = pSatWrapper;
		pSatWrapper = nullptr;
		delete old;
	}

	tleElements.first = tle1;
	tleElements.second = tle2;

	pSatWrapper = new gSatWrapper(id, tle1, tle2);
	orbitPoints.clear();
	visibilityPoints.clear();
	
	parseInternationalDesignator(tle1);
	calculateEpochFromLine1(tle1);
	calculateSatDataFromLine2(tle2);
}

void Satellite::recomputeSatData()
{
	calculateEpochFromLine1(tleElements.first);
	calculateSatDataFromLine2(tleElements.second);
}

void Satellite::update(double)
{
	if (pSatWrapper && orbitValid)
	{
		StelCore* core = StelApp::getInstance().getCore();
		epochTime = core->getJD(); // + timeShift; // We have "true" JD (UTC) from core, satellites don't need JDE!

		pSatWrapper->setEpoch(epochTime);
		position                 = pSatWrapper->getTEMEPos();
		velocity                 = pSatWrapper->getTEMEVel();
		latLongSubPointPosition  = pSatWrapper->getSubPoint();
		height                   = latLongSubPointPosition[2]; // km
		/*
		Vec2d pa                 = pSatWrapper->getPerigeeApogeeAltitudes();
		perigee                  = pa[0];
		apogee                   = pa[1];
		*/
		if (height < 0.1)
		{
			// The orbit is no longer valid.  Causes include very out of date
			// TLE, system date and time out of a reasonable range, and orbital
			// degradation and re-entry of a satellite.  In any of these cases
			// we might end up with a problem - usually a crash of Stellarium
			// because of a div/0 or something.
			qWarning() << "Satellite has invalid orbit:" << name << id;
			orbitValid = false;
			displayed = false; // It shouldn't be displayed!
			return;
		}

		elAzPosition = pSatWrapper->getAltAz();
		elAzPosition.normalize();
		XYZ = Satellite::getJ2000EquatorialPos(core);

		pSatWrapper->getSlantRange(range, rangeRate);
		visibility = pSatWrapper->getVisibilityPredict();
		phaseAngle = pSatWrapper->getPhaseAngle();

		// Compute orbit points to draw orbit line.
		if (orbitDisplayed) computeOrbitPoints();
	}
}

// Get radii and distances of shadow circles
Vec4d Satellite::getUmbraData()
{
	static PlanetP earth=GETSTELMODULE(SolarSystem)->getEarth();
	// Compute altitudes of umbra and penumbra circles. These should show where the satellite enters/exits umbra/penumbra.
	// The computation follows ideas from https://celestrak.org/columns/v03n01/
	// These sources mention ECI coordinates (Earth Centered Inertial). Presumably TEME (True Equator Mean Equinox) are equivalent, at least for our purposes.
	const double rhoE=position.norm(); // geocentric Satellite distance, km
	const double rS=earth->getHeliocentricEclipticPos().norm()*AU; // distance earth...sun
	const double thetaE=asin((earth->getEquatorialRadius()*AU)/(rhoE));
	// Accurate distance sat...sun from ECI sunpos
	Vec3d sunTEME=pSatWrapper->getSunECIPos() - pSatWrapper->getObserverECIPos(); // km
	const double thetaS=asin((sun->getEquatorialRadius()*AU)/(sunTEME.norm()));
	Q_ASSERT(thetaE>thetaS);
	const double theta=thetaE-thetaS; // angle so that satellite dives into umbra
	// angle at Sun:
	const double sigma=asin(sin(theta)*rhoE/rS);
	// angle in geocenter
	const double eta=M_PI-sigma-theta;
	// complement
	const double mu=M_PI-eta;
	// geocentric distance of shadow circle towards antisun
	umbraDistance=rhoE*cos(mu);
	// radius of shadow circle
	umbraRadius=rhoE*sin(mu);
	// Repeat for penumbra
	const double thetaP=thetaE+thetaS; // angle so that satellite touches penumbra
	// angle at Sun:
	const double sigmaP=asin(sin(thetaP)*rhoE/rS);
	// angle in geocenter
	const double etaP=M_PI-sigmaP-thetaP;
	// complement
	const double muP=M_PI-etaP;
	// geocentric distance of shadow circle towards antisun
	penumbraDistance=rhoE*cos(muP);
	// radius of shadow circle
	penumbraRadius=rhoE*sin(muP);
	//StelObjectMgr *om=GETSTELMODULE(StelObjectMgr);
	//om->setExtraInfoString(StelObject::DebugAid, QString("&rho;<sub>E</sub> %1, r<sub>S</sub> %2, &theta;<sub>E</sub> %3°, &theta;<sub>S</sub> %4° <br/>")
	//		       .arg(QString::number(rhoE, 'f', 3),
	//			    QString::number(rS, 'f', 3),
	//			    QString::number(thetaE*M_180_PI, 'f', 3),
	//			    QString::number(thetaS*M_180_PI, 'f', 3)
	//			    )
	//		       );
	//om->addToExtraInfoString(StelObject::DebugAid, QString("&theta; %1°, &sigma; %2°, &eta; %3°, &mu; %4° <br/>")
	//		       .arg(
	//			    QString::number(theta*M_180_PI, 'f', 3),
	//			    QString::number(sigma*M_180_PI, 'f', 3),
	//			    QString::number(eta*M_180_PI, 'f', 3),
	//			    QString::number(mu*M_180_PI, 'f', 3)
	//			    )
	//		       );
	//om->addToExtraInfoString(StelObject::DebugAid, QString("&theta;<sub>P</sub> %1°, &sigma; %2°, &eta; %3°, &mu; %4° <br/>")
	//		       .arg(
	//			    QString::number(thetaP*M_180_PI, 'f', 3),
	//			    QString::number(sigmaP*M_180_PI, 'f', 3),
	//			    QString::number(etaP*M_180_PI, 'f', 3),
	//			    QString::number(muP*M_180_PI, 'f', 3)
	//			    )
	//		       );
	return Vec4d(umbraDistance, umbraRadius, penumbraDistance, penumbraRadius);
}


double Satellite::getDoppler(double freq) const
{
	return  -freq*((rangeRate*1000.0)/SPEED_OF_LIGHT);
}

void Satellite::recalculateOrbitLines(void)
{
	orbitPoints.clear();
	visibilityPoints.clear();
}

SatFlags Satellite::getFlags() const
{
	// There's also a faster, but less readable way: treating them as uint.
	SatFlags flags;
	double orbitalPeriod = pSatWrapper->getOrbitalPeriod();
	if (displayed)
		flags |= SatDisplayed;
	else
		flags |= SatNotDisplayed;
	if (orbitDisplayed)
		flags |= SatOrbit;
	if (userDefined)
		flags |= SatUser;
	if (newlyAdded)
		flags |= SatNew;
	if (!orbitValid)
		flags |= SatError;
	if (RCS>0. && RCS <= 0.1)
		flags |= SatSmallSize;
	if (RCS>0.1 && RCS <= 1.0)
		flags |= SatMediumSize;
	if (RCS>1.0)
		flags |= SatLargeSize;
	if (eccentricity < 0.25 && (inclination>=0. && inclination<=180.) && apogee<4400.)
		flags |= SatLEO;
	if (eccentricity < 0.25 && inclination<25. && (orbitalPeriod>=1100. && orbitalPeriod<=2000.))
		flags |= SatGSO;
	if (eccentricity < 0.25 && (inclination>=0. && inclination<=180.) && apogee>=4400. && orbitalPeriod<1100.)
		flags |= SatMEO;
	if (eccentricity >= 0.25 && (inclination>=0. && inclination<=180.) && perigee<=70000. && orbitalPeriod<=14000.)
		flags |= SatHEO;
	if (eccentricity < 0.25 && (inclination>=25. && inclination<=180.) && (orbitalPeriod>=1100. && orbitalPeriod<=2000.))
		flags |= SatHGSO;
	if (qAbs(inclination) >= 89.5 && qAbs(inclination) <= 90.5)
		flags |= SatPolarOrbit;
	if (qAbs(inclination) <= 0.5)
		flags |= SatEquatOrbit;
	if ((qAbs(inclination) >= 97.5 && qAbs(inclination) <= 98.5) && (orbitalPeriod>=96. && orbitalPeriod<=100.))
		flags |= SatPSSO;
	// definition: https://en.wikipedia.org/wiki/High_Earth_orbit
	if (perigee>35786. && orbitalPeriod>1440.)
		flags |= SatHEarthO;
	if (qAbs(StelApp::getInstance().getCore()->getJD() - tleEpochJD) > tleEpochAge)
		flags |= SatOutdatedTLE;
	if (getCustomFiltersFlag())
		flags |= SatCustomFilter;
	if (!comms.isEmpty())
		flags |= SatCommunication;
	if (apogee<=100.0 || height<=100.0) // Karman line, atmosphere
		flags |= SatReentry;

	return flags;
}

void Satellite::setFlags(const SatFlags& flags)
{
	displayed = flags.testFlag(SatDisplayed);
	orbitDisplayed = flags.testFlag(SatOrbit);
	userDefined = flags.testFlag(SatUser);
}

bool Satellite::getCustomFiltersFlag() const
{
	double orbitalPeriod = pSatWrapper->getOrbitalPeriod();
	// Apogee
	bool cfa = true;
	if (flagCFApogee)
		cfa = (apogee>=minCFApogee && apogee<=maxCFApogee);
	// Perigee
	bool cfp = true;
	if (flagCFPerigee)
		cfp = (perigee>=minCFPerigee && perigee<=maxCFPerigee);
	// Eccentricity
	bool cfe = true;
	if (flagCFEccentricity)
		cfe = (eccentricity>=minCFEccentricity && eccentricity<=maxCFEccentricity);
	// Known standard magnitude
	bool cfm = true;
	if (flagCFKnownStdMagnitude)
		cfm = (stdMag<99.0);
	// Period
	bool cft = true;
	if (flagCFPeriod)
		cft = (orbitalPeriod>=minCFPeriod && orbitalPeriod<=maxCFPeriod);
	// Inclination
	bool cfi = true;
	if (flagCFInclination)
		cfi = (inclination>=minCFInclination && inclination<=maxCFInclination);
	// RCS
	bool cfr = true;
	if (flagCFRCS)
		cfr = (RCS>=minCFRCS && RCS<=maxCFRCS);

	return (cfa && cfp && cfe && cfm && cft && cfi && cfr);
}

void Satellite::parseInternationalDesignator(const QString& tle1)
{
	Q_ASSERT(!tle1.isEmpty());
	
	// The designator is encoded in chunk 3 on the first line.
	QStringList tleData = tle1.split(" ");
	QString rawString = tleData.at(2);
	bool ok;
	int year = rawString.left(2).toInt(&ok);
	if (!rawString.isEmpty() && ok)
	{
		// Y2K bug :) I wonder what NORAD will do in 2057. :)
		if (year < 57)
			year += 2000;
		else
			year += 1900;
		internationalDesignator = QString::number(year) + "-" + rawString.mid(2);
	}
	else
		year = 1957;
	
	StelUtils::getJDFromDate(&jdLaunchYearJan1, year, 1, 1, 0, 0, 0);	
}

bool Satellite::operator <(const Satellite& another) const
{
	// If interface strings are used, you'll need QString::localeAwareCompare()
	int comp = name.compare(another.name);
	if (comp < 0)
		return true;
	if (comp > 0)
		return false;
	
	// If the names are the same, compare IDs, i.e. NORAD numbers.
	return (id < another.id);
}

void Satellite::draw(StelCore* core, StelPainter& painter)
{
	// Separated because first test should be very fast.
	if (!displayed)
		return;

	// 1) Do not show satellites before Space Era begins!
	// 2) Do not show satellites when time rate is over limit (JD/sec)!
	if (core->getJD()<jdLaunchYearJan1 || qAbs(core->getTimeRate())>=timeRateLimit)
		return;

	if (flagVFAltitude)
	{
		// visual filter is activated!
		// is satellite located in valid range of altitudes?
		// yes, but... inverse the result and skip rendering!
		if (!(minVFAltitude<=height && height<=maxVFAltitude))
			return;
	}

	const float magSat = getVMagnitude(core);
	if (flagVFMagnitude)
	{
		// visual filter is activated and he is applicable!
		if (!(stdMag<99. || RCS>0.))
			return;
		if (!(maxVFMagnitude<=magSat && magSat<=minVFMagnitude))
			return;
	}

	Vec3d win;
	if (painter.getProjector()->projectCheck(XYZ, win))
	{
		if (!iconicModeFlag)
		{
			Vec3f color(1.f,1.f,1.f);
			// Special case: crossing of the satellite of the Moon or the Sun
			if (XYZ.angle(moon->getJ2000EquatorialPos(core))*M_180_PI <= moon->getSpheroidAngularRadius(core) || XYZ.angle(sun->getJ2000EquatorialPos(core))*M_180_PI <= sun->getSpheroidAngularRadius(core))
			{
				painter.setColor(transitSatelliteColor, 1.f);
				int screenSizeSat = static_cast<int>((getAngularRadius(core)*(2.*M_PI_180))*static_cast<double>(painter.getProjector()->getPixelPerRadAtCenter()));
				if (screenSizeSat>0)
				{
					painter.setBlending(true, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
					hintTexture->bind();
					painter.drawSprite2dMode(XYZ, qMin(screenSizeSat, 15));
				}

				if (showLabels)
				{
					if (!core->isBrightDaylight()) // crossing of the Moon
						painter.setColor(color, hintBrightness);
					painter.drawText(XYZ, name, 0, 10, 10, false);
				}
			}
			else
			{
				StelSkyDrawer* sd = core->getSkyDrawer();
				RCMag rcMag;

				// Draw the satellite
				if (magSat <= sd->getLimitMagnitude())
				{
					Vec3f vf(XYZ.toVec3f());
					Vec3f altAz(vf);
					altAz.normalize();
					core->j2000ToAltAzInPlaceNoRefraction(&altAz);
					sd->preDrawPointSource(&painter);
					sd->computeRCMag(magSat, &rcMag);
					// allow height-dependent twinkle and suppress twinkling in higher altitudes. Keep 0.1 twinkle amount in zenith.
					sd->drawPointSource(&painter, vf.toVec3d(), rcMag, color*hintBrightness, true, qMin(1.0f, 1.0f-0.9f*altAz[2]));
					sd->postDrawPointSource(&painter);
				}

				float txtMag = magSat;
				if (visibility != gSatWrapper::VISIBLE)
				{
					txtMag = magSat - 10.f; // Oops... Artificial satellite is invisible, but let's make the label visible
					painter.setColor(invisibleSatelliteColor, hintBrightness);
				}
				else
					painter.setColor(color, hintBrightness);

				// Draw the label of the satellite when it enabled
				if (txtMag <= sd->getLimitMagnitude() && showLabels)
					painter.drawText(XYZ, name, 0, 10, 10, false);
			}
		}
		else if (!(hideInvisibleSatellitesFlag && visibility != gSatWrapper::VISIBLE))
		{
			Vec3f drawColor = (coloredInvisibleSatellitesFlag && visibility != gSatWrapper::VISIBLE) ? invisibleSatelliteColor : hintColor; // Use hintColor for visible satellites only when coloredInvisibleSatellitesFlag is true
			painter.setColor(drawColor*hintBrightness, hintBrightness);
			if (XYZ.angle(moon->getJ2000EquatorialPos(core))*M_180_PI <= moon->getSpheroidAngularRadius(core) || XYZ.angle(sun->getJ2000EquatorialPos(core))*M_180_PI <= sun->getSpheroidAngularRadius(core))
				painter.setColor(transitSatelliteColor, 1.f);

			if (showLabels)
				painter.drawText(XYZ, name, 0, 10, 10, false);

			painter.setBlending(true, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
			hintTexture->bind();
			painter.drawSprite2dMode(XYZ, 11);
		}
	}

	if (orbitDisplayed && Satellite::orbitLinesFlag && orbitValid)
		drawOrbit(core, painter);
}

void Satellite::drawOrbit(StelCore *core, StelPainter& painter)
{
	int size = orbitPoints.size();
	const float ppx = static_cast<float>(painter.getProjector()->getDevicePixelsPerPixel());

	if (size>0)
	{
		Vec3d position;
		Vec3f drawColor;
		Vec4d op;
		QVector<Vec3d> vertexArray;
		QVector<Vec4f> colorArray;
		vertexArray.resize(size);
		colorArray.resize(size);

		//Rest of points
		for (int i=0; i<size; i++)
		{
			op = orbitPoints[i];
			position = core->altAzToJ2000(Vec3d(op[0],op[1],op[2]), StelCore::RefractionOff);
			position.normalize();
			vertexArray[i] = position;

			drawColor = (visibilityPoints[i] == gSatWrapper::VISIBLE) ? orbitColor : invisibleSatelliteColor;
			if (flagVFAltitude)
			{
				// visual filter is activated!
				// is satellite located in valid range of altitudes?
				if (minVFAltitude<=op[3] && op[3]<=maxVFAltitude)
					colorArray[i] = Vec4f(drawColor, hintBrightness * calculateOrbitSegmentIntensity(i));
				else
					colorArray[i] = Vec4f(0.f,0.f,0.f,0.f); // hide invisible part of orbit
			}
			else
			{
				if (hideInvisibleSatellitesFlag && visibilityPoints[i] != gSatWrapper::VISIBLE)
					colorArray[i] = Vec4f(0.f,0.f,0.f,0.f); // hide invisible part of orbit
				else
					colorArray[i] = Vec4f(drawColor, hintBrightness * calculateOrbitSegmentIntensity(i));
			}
		}

		painter.enableClientStates(true, false, false);
		if (orbitLineThickness>1 || ppx>1.f)
			painter.setLineWidth(orbitLineThickness*ppx);

		painter.drawPath(vertexArray, colorArray); // (does client state switching as needed internally)

		painter.enableClientStates(false);
		if (orbitLineThickness>1 || ppx>1.f)
			painter.setLineWidth(1);
	}
}

float Satellite::calculateOrbitSegmentIntensity(int segNum)
{
	int endDist = (orbitLineSegments/2) - abs(segNum-1 - (orbitLineSegments/2) % orbitLineSegments);
	if (endDist > orbitLineFadeSegments)
		return 1.0;
	else
		return (endDist  + 1) / (orbitLineFadeSegments + 1.0);
}

void Satellite::computeOrbitPoints()
{
	gTimeSpan computeInterval(0, 0, 0, orbitLineSegmentDuration);
	gTimeSpan orbitSpan(0, 0, 0, orbitLineSegments*orbitLineSegmentDuration/2);
	gTime epochTm;
	gTime epoch(epochTime);
	gTime lastEpochComp(lastEpochCompForOrbit);	
	int diffSlots;

	if (orbitPoints.isEmpty())//Setup orbitPoints
	{
		epochTm  = epoch - orbitSpan;

		for (int i=0; i<=orbitLineSegments; i++)
		{
			pSatWrapper->setEpoch(epochTm.getGmtTm());
			Vec3d sat = pSatWrapper->getAltAz();
			orbitPoints.append(Vec4d(sat[0],sat[1],sat[2],pSatWrapper->getSubPoint()[2]));
			visibilityPoints.append(pSatWrapper->getVisibilityPredict());
			epochTm    += computeInterval;
		}
		lastEpochCompForOrbit = epochTime;
	}
	else if (epochTime > lastEpochCompForOrbit)
	{
		// compute next orbit point when clock runs forward
		gTimeSpan diffTime = epoch - lastEpochComp;
		diffSlots          = static_cast<int>(diffTime.getDblSeconds()/orbitLineSegmentDuration);

		if (diffSlots > 0)
		{
			if (diffSlots > orbitLineSegments)
			{
				diffSlots = orbitLineSegments + 1;
				epochTm  = epoch - orbitSpan;
			}
			else
			{
				epochTm   = lastEpochComp + orbitSpan + computeInterval;
			}

			for (int i=0; i<diffSlots; i++)
			{
				//remove points at beginning of list and add points at end.
				orbitPoints.removeFirst();
				visibilityPoints.removeFirst();
				pSatWrapper->setEpoch(epochTm.getGmtTm());
				Vec3d sat = pSatWrapper->getAltAz();
				orbitPoints.append(Vec4d(sat[0],sat[1],sat[2],pSatWrapper->getSubPoint()[2]));
				visibilityPoints.append(pSatWrapper->getVisibilityPredict());
				epochTm    += computeInterval;
			}

			lastEpochCompForOrbit = epochTime;
		}
	}
	else if (epochTime < lastEpochCompForOrbit)
	{
		// compute next orbit point when clock runs backward
		gTimeSpan diffTime = lastEpochComp - epoch;
		diffSlots          = static_cast<int>(diffTime.getDblSeconds()/orbitLineSegmentDuration);

		if (diffSlots > 0)
		{
			if (diffSlots > orbitLineSegments)
			{
				diffSlots = orbitLineSegments + 1;
				epochTm   = epoch + orbitSpan;
			}
			else
			{
				epochTm   = epoch - orbitSpan - computeInterval;
			}
			for (int i=0; i<diffSlots; i++)
			{ //remove points at end of list and add points at beginning.
				orbitPoints.removeLast();
				visibilityPoints.removeLast();
				pSatWrapper->setEpoch(epochTm.getGmtTm());
				Vec3d sat = pSatWrapper->getAltAz();
				orbitPoints.push_front(Vec4d(sat[0],sat[1],sat[2],pSatWrapper->getSubPoint()[2]));
				visibilityPoints.push_front(pSatWrapper->getVisibilityPredict());
				epochTm -= computeInterval;
			}
			lastEpochCompForOrbit = epochTime;
		}
	}
}

bool operator <(const SatelliteP& left, const SatelliteP& right)
{
	if (left.isNull())
	{
		if (right.isNull())
			return false;
		else
			return true;
	}
	if (right.isNull())
		return false; // No sense to check the left one now
	
	return ((*left) < (*right));
}

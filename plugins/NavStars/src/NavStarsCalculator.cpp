/*
 * Navigational Stars plug-in
 * Copyright (C) 2020 Andy Kirkham
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

#include "StelUtils.hpp"
#include "StelTranslator.hpp"
#include "StelMainScriptAPI.hpp"
#include "planetsephems/sidereal_time.h"

#include "NavStarsCalculator.hpp"

QMap<QString,QString> NavStarsCalculator::fixedStrings;

NavStarsCalculator::NavStarsCalculator()
{
	setupFixedStrings();
}

NavStarsCalculator::NavStarsCalculator(const StelCore* c, const StelObjectP p) :
	  core(c)
	, selectedObject(p)
{
	setupFixedStrings();
}

NavStarsCalculator::~NavStarsCalculator()
{}

void NavStarsCalculator::setupFixedStrings()
{
#if !defined(WITH_TESTING_ON) || defined(_DEBUG)
	if (NavStarsCalculator::fixedStrings.size() == 0) {		
		NavStarsCalculator::fixedStrings["Hc_rad"]         = qc_("Hc", "horizontal coordinate system, computed altitude");
		NavStarsCalculator::fixedStrings["Zn_rad"]         = qc_("Zn", "horizontal coordinate system, computed azimuth");		
		NavStarsCalculator::fixedStrings["gha_rad"]        = qc_("GHA", "greenwich hour angle");
		NavStarsCalculator::fixedStrings["lha_rad"]        = qc_("LHA", "local hour angle");
		NavStarsCalculator::fixedStrings["lat_rad"]        = qc_("Lat", "geodetic cordinate system, latitude");
		NavStarsCalculator::fixedStrings["lon_rad"]        = qc_("Lon", "geodetic cordinate system, longitude");		
		NavStarsCalculator::fixedStrings["gmst_deg"]       = qc_("GMST", "greenwich mean sidereal time (ERA, Earth rotation angle)");
		NavStarsCalculator::fixedStrings["lmst_deg"]       = qc_("LMST", "local mean sidereal time");
		NavStarsCalculator::fixedStrings["gmst_rad"]       = qc_("GHA", "first point of aries greenwich hour angle");
		NavStarsCalculator::fixedStrings["alt_app_rad"]    = qc_("Ho", "horizontal coordinate system, altitude measured via a sextant");
		NavStarsCalculator::fixedStrings["object_sha_rad"] = qc_("SHA", "sidereal hour angle");
		NavStarsCalculator::fixedStrings["object_dec_rad"] = qc_("DEC", "celestial coordinate system");
	}
#endif
}

NavStarsCalculator& 
NavStarsCalculator::setWithTables(bool b)
{
	withTables = b;
	return *this;
}

NavStarsCalculator& 
NavStarsCalculator::setUTC(const QString& s)
{
	utc = s;
	return *this;
}

void NavStarsCalculator::extraInfoStrings(const QMap<QString, double>& data, QMap<QString, QString>& strings, QString extraText)
{
	QString ariesSymbol = "&#9800;";

	if(data.contains("alt_app_rad"))
		strings["alt_app_rad"] = 
			oneRowTwoCells(NavStarsCalculator::fixedStrings["alt_app_rad"], 
				radToDm(data["alt_app_rad"]), extraText);
	if(data.contains("gmst_deg"))
		strings["gmst_deg"] =
			oneRowTwoCells(NavStarsCalculator::fixedStrings["gmst_deg"], 
				QString("%1%2").arg(QString::number(data["gmst_deg"], 'f', 3)).arg("&deg;"), "");
	if(data.contains("lmst_deg"))
		strings["lmst_deg"] =
			oneRowTwoCells(NavStarsCalculator::fixedStrings["lmst_deg"], 
				QString("%1%2").arg(QString::number(data["lmst_deg"], 'f', 3)).arg("&deg;"), "");
	if(data.contains("gmst_rad"))
		strings["gmst_rad"] =
			oneRowTwoCells(NavStarsCalculator::fixedStrings["gmst_rad"] + ariesSymbol, 
				radToDm(data["gmst_rad"]), "");
	if(data.contains("object_sha_rad"))
		strings["object_sha_rad"] = 
			oneRowTwoCells(NavStarsCalculator::fixedStrings["object_sha_rad"], 
				radToDm(data["object_sha_rad"]), "");
	if(data.contains("object_dec_rad"))
		strings["object_dec_rad"] = 
			oneRowTwoCells(NavStarsCalculator::fixedStrings["object_dec_rad"], 
				radToDm(data["object_dec_rad"]), "");
	if(data.contains("gha_rad"))
		strings["gha_rad"] =
			oneRowTwoCells(NavStarsCalculator::fixedStrings["gha_rad"], 
				radToDm(data["gha_rad"]), "");
	if(data.contains("lha_rad"))
		strings["lha_rad"] =
			oneRowTwoCells(NavStarsCalculator::fixedStrings["lha_rad"], 
				radToDm(data["lha_rad"]), "");
	if(data.contains("lat_rad"))
		strings["lat_rad"] = 
			oneRowTwoCells(NavStarsCalculator::fixedStrings["lat_rad"], 
				radToDm(data["lat_rad"], "N", "S"), "");	
	if(data.contains("lon_rad"))
		strings["lon_rad"] = 
			oneRowTwoCells(NavStarsCalculator::fixedStrings["lon_rad"], 
				radToDm(data["lon_rad"], "E", "W"), "");	
	if(data.contains("Hc_rad"))
		strings["Hc_rad"] = 
			oneRowTwoCells(NavStarsCalculator::fixedStrings["Hc_rad"],
				radToDm(data["Hc_rad"]), "");
	if(data.contains("Zn_rad"))
		strings["Zn_rad"] = 
			oneRowTwoCells(NavStarsCalculator::fixedStrings["Zn_rad"],
				radToDm(data["Zn_rad"]), "");
}

void NavStarsCalculator::calculateCelestialNavData()
{
	lat = core->getCurrentLocation().latitude;
	lat_rad = lat * M_PI / 180.;
	lon = core->getCurrentLocation().longitude;
	lon_rad = lon * M_PI / 180.;
	StelUtils::rectToSphe(&object_ra, &object_dec, 
		selectedObject->getEquinoxEquatorialPosApparent(core));	
	object_sha = (2. * M_PI) - object_ra;
	jd = core->getJD();
	jde = core->getJDE();
	gmst_raw = get_mean_sidereal_time(jd, jde);
	gmst = wrap360(gmst_raw);
	lmst = wrap360(gmst + lon);
	gmst_rad = gmst * M_PI / 180.;
	lmst_rad = lmst * M_PI / 180.;
	gha_rad = wrap2pi(gmst_rad + object_sha);
	lha = wrap2pi(gha_rad + lon_rad);
	StelUtils::rectToSphe(&az,&alt,selectedObject->getAltAzPosGeometric(core));
	if (az > (2 * M_PI)) 
		az -= (2 * M_PI);
	StelUtils::rectToSphe(&az_app,&alt_app,selectedObject->getAltAzPosApparent(core));
	if (az_app > (2 * M_PI)) 
		az_app -= (2 * M_PI);
	hc = (std::sin(object_dec) * std::sin(lat_rad)) + 
		            (std::cos(object_dec) * std::cos(lat_rad) * std::cos(lha));
	hc = std::asin(hc);
	zn = (std::sin(object_dec) * std::cos(lat_rad)) - 
		 (std::cos(object_dec) * std::sin(lat_rad) * std::cos(lha));
	zn = std::acos(zn / std::cos(hc));
	if (lha < M_PI) 
		zn = (2*M_PI) - zn;
}

void NavStarsCalculator::getCelestialNavData(QMap<QString, double>& map)
{
	calculateCelestialNavData();
	map.clear();
	map["lat_deg"] = lat;
	map["lat_rad"] = lat_rad;
	map["lon_deg"] = lon;
	map["lon_rad"] = lon_rad;	
	map["object_ra_rad"] = object_ra;
	map["object_dec_rad"] = object_dec;
	map["object_sha_rad"] = object_sha;
	map["jd"] = jd;
	map["jde"] = jde;
	map["gmst_deg"] = gmst;
	map["lmst_deg"] = lmst;
	map["gmst_rad"] = gmst_rad;
	map["lmst_rad"] = lmst_rad;
	map["gha_rad"] = gha_rad;
	map["lha_rad"] = lha;
	map["az_rad"] = az;
	map["alt_rad"] = alt;
	map["az_app_rad"] = az_app;
	map["alt_app_rad"] = alt_app;
	map["Hc_rad"] = hc;
	map["Zn_rad"] = zn;
}

QString NavStarsCalculator::radToDm(double rad, const QString pos, const QString neg)
{
	QString rval;
	bool sign;
	double s, md;
	unsigned int d, m;
	StelUtils::radToDms(rad, sign, d, m, s);
	md = static_cast<double>(m);
	md += (s / 60.);
	rval += (sign ? pos : neg)
		+ QString::number(d, 'f', 0)
		+ QString("&deg;")
		+ QString::number(md, 'f', 1)
		+ "'";
	return rval;
}

QString NavStarsCalculator::oneRowTwoCells(const QString& a, const QString& b, QString c)
{
	QString rval;
	if (withTables) {
		rval += "<tr><td>" + a + ":</td><td>" + b + c + "</td></tr>";
	}
	else {
		rval += a + ": " + b + c + "<br/>";
	}
	return rval;
}

double NavStarsCalculator::wrap2pi(double d)
{
	if (d > (2. * M_PI)) d -= (2 * M_PI);
	else if (d < 0.) d += (2 * M_PI);
	return d;
}

double NavStarsCalculator::wrap360(double d)
{
	if (d > 360.) d -= 360.;
	else if (d < 0.) d += 360.;
	return d;
}


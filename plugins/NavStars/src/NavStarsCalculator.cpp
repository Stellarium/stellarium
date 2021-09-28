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
#include "NavStarsCalculator.hpp"

using std::sin;
using std::cos;
using std::asin;
using std::acos;

bool NavStarsCalculator::useExtraDecimals = false;
bool NavStarsCalculator::useDecimalDegrees = false;

#ifdef _DEBUG
static void examineCalc(NavStarsCalculator& calc);
#endif 

NavStarsCalculator::NavStarsCalculator()
{}

NavStarsCalculator::~NavStarsCalculator()
{}

NavStarsCalculator::NavStarsCalculator(const NavStarCalculatorDegreeInputs* ip)
	: lmst(0.), lmst_rad(0.), lha(0.), hc_rad(0.), zn_rad(0.), gha_rad(0.), gmst(0.)
	, gmst_rad(0.), gp_lat_deg(0.), gp_lat_rad(0.), gp_lon_deg(0.), gp_lon_rad(0.)
{
	setUTC(ip->utc);
	setLatDeg(ip->lat);
	setLonDeg(ip->lon);
	setJd(ip->jd);
	setJde(ip->jde);
	setGmst(ip->gmst);
	setRaRad(DEG2RAD(ip->ra));
	setDecRad(DEG2RAD(ip->dec));
	setAzRad(DEG2RAD(ip->az));
	setAltRad(DEG2RAD(ip->alt));
	setAzAppRad(DEG2RAD(ip->az_app));
	setAltAppRad(DEG2RAD(ip->alt_app));
}

NavStarsCalculator& 
NavStarsCalculator::setUTC(const QString& s)
{
	utc = s;
	return *this;
}


void NavStarsCalculator::execute()
{
	lmst = wrap360(gmst + lon_deg);
	gmst_rad = gmst * M_PI / 180.;
	lmst_rad = lmst * M_PI / 180.;
	gha_rad = wrap2pi(gmst_rad + sha_rad);
	lha = wrap2pi(gha_rad + lon_rad);
	if (az_rad > (2 * M_PI)) 
		az_rad -= (2 * M_PI);
	if (az_app_rad > (2 * M_PI)) 
		az_app_rad -= (2 * M_PI);
	hc_rad = asin((sin(dec_rad) * sin(lat_rad)) + (cos(dec_rad) * cos(lat_rad) * cos(lha)));
	zn_rad = (std::sin(dec_rad) * std::cos(lat_rad)) - 
		            (std::cos(dec_rad) * std::sin(lat_rad) * std::cos(lha));
	zn_rad = acos(zn_rad / cos(hc_rad));
	
	if (lha < M_PI) 
		zn_rad = (2*M_PI) - zn_rad;

	// Convert object GHA to geodetic longitude.
	double d = gha_rad;
	if (d < M_PI)
		d *= -1;
	else
		d = (2 * M_PI) - d;

	// Set the ground geodetic position.
	setGpLonRad(d);
	setGpLatRad(dec_rad);
}

QString NavStarsCalculator::radToDm(double rad, const QString& pos, const QString& neg)
{
	QString rval;
	bool sign;	
	if (useDecimalDegrees)
	{
		double dd;
		StelUtils::radToDecDeg(rad, sign, dd);
		rval = QString("%1%2&deg;").arg((sign ? pos : neg), QString::number(dd, 'f', useExtraDecimals ? 6 : 5));
	}
	else
	{
		double s, md;
		unsigned int d, m;
		StelUtils::radToDms(rad, sign, d, m, s);

		md = static_cast<double>(m);
		md += (s / 60.);
		rval = QString("%1%2&deg;%3'").arg((sign ? pos : neg), QString::number(d, 'f', 0), QString::number(md, 'f', useExtraDecimals ? 4 : 1));
	}
#ifdef _DEBUG
	// An easier to use display when working with Google Earth, 
	// Google Maps, custom software tools, etc. Keep everything
	// decimal. Only need DDMM.m for Almanac display.
	rval += " (" + ( sign ? pos : neg ) + QString::number(qAbs(RAD2DEG(rad)), 'f', 5) + ")";
#endif
	return rval;
}

double NavStarsCalculator::wrap2pi(double d)
{
	while(d > (2. * M_PI)) d -= (2 * M_PI);
	while(d < 0.) d += (2 * M_PI);
	return d;
}

double NavStarsCalculator::wrap360(double d)
{
	while(d > 360.) d -= 360.;
	while(d < 0.) d += 360.;
	return d;
}

#ifndef CRLF
#ifdef WIN32
#define CRLF "\r\n"
#else
#define CRLF "\n"
#endif
#endif

QMap<QString, QString> NavStarsCalculator::printable()
{
	QMap<QString, QString> rval;
	rval["getUTC"] = getUTC();
	rval["hcPrintable"] = hcPrintable();
	rval["altAppPrintable"] = altAppPrintable();
	rval["gmstDegreesPrintable"] = gmstDegreesPrintable();
	rval["lmstDegreesPrintable"] = lmstDegreesPrintable();
	rval["shaPrintable"] = shaPrintable();
	rval["decPrintable"] = decPrintable();
	rval["ghaPrintable"] = ghaPrintable();
	rval["lhaPrintable"] = lhaPrintable();
	rval["latPrintable"] = latPrintable();
	rval["lonPrintable"] = lonPrintable();
	rval["hcPrintable"] = hcPrintable();
	rval["znPrintable"] = znPrintable();
	return rval;
}

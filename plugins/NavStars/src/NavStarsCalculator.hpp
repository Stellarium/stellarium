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

#ifndef NAVSTARSCALCULATOR_HPP
#define NAVSTARSCALCULATOR_HPP

/*! @defgroup navigationalStars Navigational Stars Plug-in
@{
The Navigational Stars plugin marks a set of navigational stars.

The NavStars class is the main class of the plug-in. It manages the list of
navigational stars and manipulate show/hide markers of them. Markers
are not objects!

The plugin is also an example of a custom plugin that just marks existing stars.

<b>Configuration</b>

The plug-ins' configuration data is stored in Stellarium's main configuration
file (section [NavigationalStars]).

@}
*/

#include <cmath>

#include <QMap>
#include <QString>

#ifndef DEG2RAD
#define DEG2RAD(x) ((x * M_PI) / 180.)
#endif

#ifndef RAD2DEG
#define RAD2DEG(x) ((x * 180.) / M_PI)
#endif

//! Forward declaration for UT inputter.
struct NavStarCalculatorDegreeInputs;

//! @class NavStarsCalculator
//! @author Andy Kirkham
//! @ingroup navigationalStars
class NavStarsCalculator 
{
public:
	NavStarsCalculator();
	NavStarsCalculator(const NavStarCalculatorDegreeInputs* ip);
	~NavStarsCalculator();

	//! Calculate intermediates.
	void execute();

	//! Get a string that represents degrees and decimal minutes of an angle.
	//! Format is as per the standard Nautical Almanac, +/-DDDMM.m
	//! @param double rad The angle in radians.
	//! @param QString pos Use + or N or E. 
	//! @param QString neg Use - or S or W.
	//! @return QString the representation of angle rad.    
	static QString radToDm(double rad, const QString &pos = "+", const QString &neg = "-");


	//! Ensure the supplied angle, in radians, is with 0 to 2PI.
	//! @param double d The angle in radians.
	//! @return The wrapped value.
	static double wrap2pi(double d);

	//! Ensure the supplied angle, in degrees, is with 0 to 360.
	//! @param double d The angle in degrees.
	//! @return The wrapped value.
	static double wrap360(double d);

	static bool useExtraDecimals;
	static bool useDecimalDegrees;

private:
	QString utc;
public:
	NavStarsCalculator& setUTC(const QString& s);
	QString getUTC() {	return utc; }

	QMap<QString, QString> printable();

private:
	double lmst;
	double lmst_rad;
public:
	QString lmstDegreesPrintable()
	{
		QString sign = lmst < 0. ? "-" : "+";
		return QString("%1%2%3").arg(sign).arg(QString::number(lmst, 'f', 3)).arg("&deg;");
	}
	QString lmstPrintable() { return radToDm(lmst_rad); }

private:
	double sha_rad;
public:
	QString shaPrintable() { return radToDm(sha_rad); }

private:
	double lha;
public:
	QString lhaPrintable() { return radToDm(lha); }

private:
	double hc_rad;
public:
	QString hcPrintable() { return radToDm(hc_rad); }

private:
	double zn_rad;
public:
	QString znPrintable() { return radToDm(zn_rad); }

private:
	double gha_rad;
public:
	QString ghaPrintable() { return (radToDm(gha_rad)); }

private:
	double gmst;
	double gmst_rad;
public:
	double getGmst() { return gmst;  }
	NavStarsCalculator& setGmst(double d)
	{
		gmst = wrap360(d);
		return *this;
	}
	QString gmstDegreesPrintable() {
		QString sign = gmst < 0. ? "-" : "+";
		return QString("%1%2%3").arg(sign).arg(QString::number(gmst, 'f', 3)).arg("&deg;");
	}
	QString gmstPrintable() { return radToDm(gmst_rad); }

private:
	double lat_deg;
	double lat_rad;
public:
	NavStarsCalculator& setLatDeg(double d)
	{
		lat_deg = d;
		lat_rad = DEG2RAD(d);
		return *this;
	}
	QString latPrintable() { return radToDm(lat_rad, "N", "S"); }

private:
	double lon_deg;
	double lon_rad;
public:
	NavStarsCalculator& setLonDeg(double d)
	{
		lon_deg = d;
		lon_rad = DEG2RAD(d);
		return *this;
	}
	QString lonPrintable() { return radToDm(lon_rad, "E", "W");  }

private:
	double gp_lat_deg;
	double gp_lat_rad;
public:
	NavStarsCalculator& setGpLatRad(double d)
	{
		gp_lat_rad = d;
		gp_lat_deg = DEG2RAD(d);
		return *this;
	}
	QString gplatPrintable() { return radToDm(gp_lat_rad, "N", "S"); }

private:
	double gp_lon_deg;
	double gp_lon_rad;
public:
	NavStarsCalculator& setGpLonRad(double d)
	{
		gp_lon_rad = d;
		gp_lon_deg = DEG2RAD(d);
		return *this;
	}
	QString gplonPrintable() { return radToDm(gp_lon_rad, "E", "W"); }

private:
	double az_rad;
public:
	NavStarsCalculator& setAzRad(double d)
	{
		az_rad = d;
		return *this;
	}

private:
	double alt_rad;
public:
	NavStarsCalculator& setAltRad(double d)
	{
		alt_rad = d;
		return *this;
	}
    
private:
	double az_app_rad;
public:
	NavStarsCalculator& setAzAppRad(double d)
	{
		az_app_rad = d;
		return *this;
	}
	QString azAppPrintable() { return radToDm(az_app_rad); }

private:
	double alt_app_rad;
public:
	NavStarsCalculator& setAltAppRad(double d)
	{
		alt_app_rad = d;
		return *this;
	}
	QString altAppPrintable() { return radToDm(alt_app_rad); }
	NavStarsCalculator& addAltAppRad(double d)
	{
		alt_app_rad += d;
		return *this;
	}

private:
	double jd;
public:
	double getJd() { return jd; }
	NavStarsCalculator& setJd(double d)
	{
		jd = d;
		return *this;
	}

private:
	double jde;
public:
	double getJde() { return jde; }
	NavStarsCalculator& setJde(double d)
	{
		jde = d;
		return *this;
	}

private:
	double dec_rad;
public:
	NavStarsCalculator& setDecRad(double d)
	{
		dec_rad = d;
		return *this;
	}
	QString decPrintable() { return radToDm(dec_rad); }

private:
	double ra_rad;
public:
	NavStarsCalculator& setRaRad(double d)
	{
		ra_rad = d;
		sha_rad = (2. * M_PI) - ra_rad;
		return *this;
	}
};

// When exactly does code to support Unit Testing become intrusive?
// I've done worse than this before so it's not all that bad.
// See specialized constructor above.
struct NavStarCalculatorDegreeInputs
{
	QString utc;
	double ra;
	double dec;
	double lat;
	double lon;
	double jd;
	double jde;
	double gmst;
	double az;
	double alt;
	double az_app;
	double alt_app;
};

#endif /* NAVSTARSCALCULATOR_HPP */

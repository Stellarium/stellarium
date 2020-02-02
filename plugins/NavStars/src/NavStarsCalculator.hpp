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
The Navigational Stars plugin marks the 58 navigational stars of the
Nautical Almanach and the 2102-D Rude Star Finder on the sky.

The NavStars class is the main class of the plug-in. It manages the list of
navigational stars and manipulate show/hide markers of them. All markers
are not objects!

The plugin is also an example of a custom plugin that just marks existing stars.

<b>Configuration</b>

The plug-ins' configuration data is stored in Stellarium's main configuration
file (section [NavigationalStars]).

@}
*/

#include <cmath>

#include <QString>

//#include "StelUtils.hpp"

#define DEG2RAD(x) ((x * M_PI) / 180.)
#define RAD2DEG(x) ((x * 180.) / M_PI)

//! @class NavStarsCalculator
//! @author Andy Kirkham
//! @ingroup navigationalStars
class NavStarsCalculator 
{
public:	
    NavStarsCalculator();
    ~NavStarsCalculator();

    //! Calculate intermediates.
    void execute();
    
    //! Get a string that represents degrees and decimal minutes of an angle.
	//! Format is as per the standard Nautical Almanac, +/-DDDMM.m
	//! @param double rad The angle in radians.
	//! @param QString pos Use + or N or E. 
	//! @param QString neg Use - or S or W.
	//! @return QString the representation of angle rad.
    static QString radToDm(double rad, const QString pos = "+", const QString neg = "-");

    //! Ensure the supplied angle, in radians, is with 0 to 2PI.
	//! @param double d The angle in radians.
	//! @return The wrapped value.
    static double wrap2pi(double d);

    //! Ensure the supplied angle, in degrees, is with 0 to 360.
	//! @param double d The angle in degrees.
	//! @return The wrapped value.
    static double wrap360(double d);

private:
    QString utc;
public:
    NavStarsCalculator& setUTC(const QString& s);
    QString getUTC() {
        return utc;
    }

private:
    double lmst;
public:
    QString lmstDegreesPrintable() {
        QString sign = lmst < 0. ? "-" : "+";
        return QString("%1%2%3")
            .arg(sign)
            .arg(QString::number(lmst, 'f', 3))
            .arg("&deg;");
    }


private:
    double sha_rad;    
public:
    QString shaPrintable() {
        return radToDm(sha_rad);
    }

private:
    double lha;    
public:
    QString lhaPrintable() {
        return radToDm(lha);
    }

private:
    double gmst_rad;

private:
    double lmst_rad;
public:
    QString lmstPrintable() {
        return radToDm(lmst_rad);
    }

private:
    double hc_rad;
public:
    QString hcPrintable() {
        return radToDm(hc_rad);
    }


private:
    double zn_rad;
public:
    QString znPrintable() {
        return radToDm(zn_rad);
    }

private:
    double gha_rad;
public:
    QString ghaPrintable() {
        return (radToDm(gha_rad) + "&deg;");
    }

private:
    double gmst;
public:
    double getGmst() { return gmst;  }
    NavStarsCalculator& setGmst(double d) {
        gmst = wrap360(d);
        return *this;
    }
    QString gmstDegreesPrintable() {
        QString sign = gmst < 0. ? "-" : "+";
        return QString("%1%2%3")
            .arg(sign)
            .arg(QString::number(gmst, 'f', 3))
            .arg("&deg;");
    }

private:
    double lat_deg;    
    double lat_rad;
public:
    NavStarsCalculator& setLatDeg(double d) {
        lat_deg = d;
        lat_rad = DEG2RAD(d);
        return *this;
    }
    QString latPrintable() {
        return radToDm(lat_rad, "N", "S");
    }

private:
    double lon_deg;
    double lon_rad;
public:
    NavStarsCalculator& setLonDeg(double d) {
        lon_deg = d;
        lon_rad = DEG2RAD(d);
        return *this;
    }
    QString lonPrintable() {
        return radToDm(lon_rad, "E", "W");
    }

private:
    double az_rad;
public:
    NavStarsCalculator& setAzRad(double d) {
        az_rad = d;
        return *this;
    }

private:
    double alt_rad;
public:
    NavStarsCalculator& setAltRad(double d) {
        alt_rad = d;
        return *this;
    }
    
private:
    double az_app_rad;
public:
    NavStarsCalculator& setAzAppRad(double d) {
        az_app_rad = d;
        return *this;
    }
    QString azAppPrintable() {
        return radToDm(az_app_rad);
    }


private:
	double alt_app_rad;
public:
    NavStarsCalculator& setAltAppRad(double d) {
        alt_app_rad = d;
        return *this;
    }
    QString altAppPrintable() {
        return radToDm(alt_app_rad);
    }
    NavStarsCalculator& addAltAppRad(double d) {
        alt_app_rad += d;
        return *this;
    }

private:
    double jd;
public:
    double getJd() { return jd; }
    NavStarsCalculator& setJd(double d) {
        jd = d;
        return *this;
    }

private:
    double jde;
public:
    double getJde() { return jde; }
    NavStarsCalculator& setJde(double d) {
        jde = d;
        return *this;
    }

private:
	double dec_rad;
public:
    NavStarsCalculator& setDecRad(double d) {
        dec_rad = d;
        return *this;
    }
    QString decPrintable() {
        return radToDm(dec_rad);
    }

private:
	double ra_rad;		
public:
    NavStarsCalculator& setRaRad(double d) {
        ra_rad = d;
        sha_rad = (2. * M_PI) - ra_rad;
        return *this;
    }
};

#endif /* NAVSTARSCALCULATOR_HPP */

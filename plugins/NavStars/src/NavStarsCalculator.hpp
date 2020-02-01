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

#include <QMap>
#include <QString>

#include "StelUtils.hpp"

#define DEG2RAD(x) ((x * M_PI) / 180.)
#define RAD2DEG(x) ((x * 180.) / M_PI)

//! @class NavStarsCalculator
//! @author Andy Kirkham
//! @ingroup navigationalStars
class NavStarsCalculator 
{
public:	
    NavStarsCalculator();
    NavStarsCalculator(const StelCore* c, const StelObjectP p);
    ~NavStarsCalculator();

    //! For the provided StelObject return a QMap of data of Nautical Almanac data
	//! @param StelCore* core
	//! @param StelObjectP selectedObject
	//! @param QMap<QString, double>& map (Data output to this map)
	void getCelestialNavData(QMap<QString, double>& map);

    //! Calculate intermediates.
    void calculateCelestialNavData();
    
    //! Given raw data format into display strings.
    //! @param const QMap<QString, double>& data The raw data.
    //! @param QMap<QString, QString>& strings Where to place the output strings.
    //! @param QString extraText Fixed extra info to append to b above.
    void extraInfoStrings(const QMap<QString, double>& data, QMap<QString, QString>& strings, QString extraText);

    NavStarsCalculator& setWithTables(bool b);

    const QString getUTC() { return utc; }
    NavStarsCalculator& setUTC(const QString& s);
    
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

    //! Given two QStrings return in a format consistent with the
	//! property setting of "withTables".
	//! @param QString a The cell left value
	//! @param QString b The cell right value
	//! @param QString c The cell right extra value
	//! @return QString The representation of the extraString info.
    QString oneRowTwoCells(const QString& a, const QString& b, QString c);

protected:
    void setupFixedStrings();

private:
    QString utc;
    bool withTables = false;
    const StelCore* core;
    const StelObjectP selectedObject;

    static QMap<QString, QString> fixedStrings;

public:
    double az;
    double alt;
    double hc;
    double zn;
    double jd;
    double jde;
    double lha;
    double lat;
    double lat_rad;
    double lon;
    double lon_rad;
    double gmst;
    double gmst_rad;
    double gmst_raw;
    double lmst;
    double lmst_rad;
    double az_app;
	double alt_app;
    double gha_rad;
	double object_dec;
	double object_ra;		
	double object_sha;
};


#endif /* NAVSTARSCALCULATOR_HPP */

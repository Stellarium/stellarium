/*
 * Copyright (C) YEAR Your Name
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */
#ifndef OBSERVABILITY_HPP_
#define OBSERVABILITY_HPP_

#include "StelModule.hpp"
#include <QFont>
#include <QString>
#include "VecMath.hpp"


class Observability : public StelModule
{
public:
	Observability();
	virtual ~Observability();
	virtual void init();
	virtual void update(double) {;}
	virtual void draw(StelCore* core);
	virtual double getCallOrder(StelModuleActionName actionName) const;

//! Computes the Hour Angle (culmination=0h) in absolute value (from 0h to 12h).
//! @param latitude latitude of the observer (in radians).
//! @param elevation elevation angle of the object (horizon=0) in radians.
//! @param declination declination of the object in radians. 
	virtual double HourAngle(double latitude,double elevation,double declination);

//! Converts a time span in hours (given as double) in hh:mm:ss (integers).
//! @param t time span (double, in hours).
//! @param h hour (integer).
//! @param m minute (integer).
//! @param s second (integer).
	virtual void double2hms(double t, int &h,int &m,int &s);

//! Returns a string of date (e.g. "25 Apr") from a Day of Year (integer).
//! @param DoY Day of the year (from 1 to 365). Never returns "29 Feb".
	virtual QString CalenDate(int DoY);

//! Just subtracts/adds 24h to a RA (or HA), to make it fall within 0-24h.
//! @param RA right ascension (in hours).
	virtual void toUnsignedRA(double &RA);

//! Some useful constants (self-explanatory).
	double Rad2Deg, Rad2Hr;

//! RA, Dec, observer latitude, object's elevation, and Hour Angle at horizon.
	double selRA, selDec, mylat, alti, horizH;

//! Vectors to store Sun's RA, Dec, and Hour Angle at astronomical twilight.
	double SunRA[365], SunDec[365], SunHTwi[365];
	double AstroTwiAlti;

//! Cumulative sum of days of year for each month;
	int cumsum[13];

//! Useful auxiliary strings, to help checking changes in source/observer. Also to store results that must survive between iterations.
	QString selName, bestNight, ObsRange, objname;

//! Just the names of the months.
	QString months[12];

//! Equatorial coordinates of currently-selected source.
	Vec3d EquPos;

//! Boolean to check whether an objects belongs or not to the Solar System.
	bool isStar;

private:
	QFont font;
};

#include "fixx11h.h"
#include <QObject>
#include "StelPluginInterface.hpp"

class ObservabilityStelPluginInterface : public QObject, public StelPluginInterface
{
       Q_OBJECT
       Q_INTERFACES(StelPluginInterface)
public:
       virtual StelModule* getStelModule() const;
       virtual StelPluginInfo getPluginInfo() const;
};

#endif /*OBSERVABILITY_HPP_*/

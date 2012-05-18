/*
 * Copyright (C) 2012 Ivan Marti-Vidal
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
#include "SolarSystem.hpp"
#include "Planet.hpp"
#include "StelFader.hpp"

class QPixmap;
class StelButton;


class Observability : public StelModule
{
	Q_OBJECT
public:
	Observability();
	virtual ~Observability();
	virtual void init();
	virtual void update(double) {;}
	virtual void draw(StelCore* core);
	virtual double getCallOrder(StelModuleActionName actionName) const;


public slots:
//! Set whether observability will execute or not:
	void enableObservability(bool b);

private:
//! Computes the Hour Angle (culmination=0h) in absolute value (from 0h to 12h).
//! @param latitude latitude of the observer (in radians).
//! @param elevation elevation angle of the object (horizon=0) in radians.
//! @param declination declination of the object in radians. 
	virtual double HourAngle(double latitude,double elevation,double declination);

//! Solves Moon/Sun Rise/Set/Transit times for the current Julian day. This function updates the variables MoonRise, MoonSet, MoonCulm. Returns success status.
	virtual bool MoonSunSolve(StelCore* core);

//! Finds the heliacal rise/set dates of the year for the currently-selected object.
//! @param Rise day of year of the Heliacal rise.
//! @param Set day of year of the Heliacal set.
	virtual bool CheckHeli(int &Rise, int &Set);

//! Converts a time span in hours (given as double) in hh:mm:ss (integers).
//! @param t time span (double, in hours).
//! @param h hour (integer).
//! @param m minute (integer).
//! @param s second (integer).
	virtual void double2hms(double t, int &h,int &m,int &s);

//! Just returns the sign of a double;
	virtual double sign(double d);

//! Returns a string of date (e.g. "25 Apr") from a Day of Year (integer).
//! @param DoY Day of the year.
	virtual QString CalenDate(int DoY);

//! Just subtracts/adds 24h to a RA (or HA), to make it fall within 0-24h.
//! @param RA right ascension (in hours).
	virtual double toUnsignedRA(double RA);

//! Computes the RA, Dec and Rise/Set Sid. times of the selected planet for each day of the current year.
//! @param core the current Stellarium core.
//! @param Name name of the currently selected planet. 
	virtual void PlanetRADec(StelCore *core, QString Name);

//! Computes the Sun's RA and Dec for each day of a given year.
//! @param core current Stellarium core.
	virtual void SunRADec(StelCore* core);

//! Computes the Sun's Sid. Times at astronomical twilight (for each year's day)
	virtual void SunHTwi();

//! Just convert the Vec3d named TempLoc into RA/Dec:
	virtual void toRADec(Vec3d TempLoc, double &RA, double &Dec);

//! The opposite of above (set TempLoc from RA and Dec):
//	virtual void fromRADec(Vec3d* TempLoc, double RA, double Dec);


//! Vector to store the Julian Dates for the current year:
	double yearJD[366];

//! Check if a source is observable during a given date:
//! @aparm i the day of the year.
	virtual bool CheckRise(int i);

//! Some useful constants (self-explanatory).
	double Rad2Deg, Rad2Hr, AstroTwiAlti;

//! RA, Dec, observer latitude, object's elevation, and Hour Angle at horizon.
	double selRA, selDec, mylat, mylon, alti, horizH, myJD;

//! Vectors to store Sun's RA, Dec, and Sid. Time at twilight and rise/set.
	double SunRA[366], SunDec[366], SunSidT[4][366];

//! Vectors to store planet's RA, Dec, and Sid. Time at rise/set.
	double ObjectRA[366], ObjectDec[366], ObjectH0[366], ObjectSidT[2][366];

//! Rise/Set/Transit times for the Moon at current day:
	double MoonRise, MoonSet, MoonCulm, lastJDMoon;

//! Vector of Earth position through the year.
	Vec3d EarthPos[366];

//! Current simulation year and number of days in the year.;
	int currYear, nDays;

//! Useful auxiliary strings, to help checking changes in source/observer. Also to store results that must survive between iterations.
	QString selName, bestNight, ObsRange, objname, Heliacal;

//! Just the names of the months.
	QString months[12];

//! Equatorial and local coordinates of currently-selected source.
	Vec3d EquPos, LocPos;

//! Some booleans to check the kind of source selected.
	bool isStar,isMoon,isSun,isScreen, LastSun;

//! Some booleans to select the kind of output.
	bool show_Heliacal, show_Good_Nights, show_Best_Night, show_Today;

//! Parameters for the graphics:
	QFont font;
	Vec3f fontColor;
	bool flagShowObservability;
	int fontSize;
	QPixmap* OnIcon;
	QPixmap* OffIcon;
	QPixmap* GlowIcon;
	StelButton* toolbarButton;


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

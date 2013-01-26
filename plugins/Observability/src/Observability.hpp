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
class ObservabilityDialog;

class Observability : public StelModule
{
	Q_OBJECT
public:
	Observability();
	virtual ~Observability();
	virtual void init();
	virtual void update(double) {;}
	virtual void draw(StelCore* core, class StelRenderer* renderer);
	virtual double getCallOrder(StelModuleActionName actionName) const;


	//! Implement this to tell the main Stellarium GUI that there is a GUI element to configure this plugin.
	virtual bool configureGui(bool show=true);


	//! Set up the plugin with default values.
	void restoreDefaults(void);
	void restoreDefaultConfigIni(void);

	//! Read (or re-read) settings from the main config file.  This will be called from init and also
	//! when restoring defaults (i.e. from the configuration dialog / restore defaults button).
	void readSettingsFromConfig(void);

	//! Save the settings to the main configuration file.
	void saveSettingsToConfig(void);

	//! Set which output is shown.
	//! @param output is the index of the output (e.g., 1 for today's ephemeris, 5 for Full Moon).
	//! @param show is a boolean (true to show the output; false to hide it).
	void setShow(int output, bool show);

	//! Set the font colors. Color is (0,1,2) for (R,G,B):
	void setFontColor(int Color, int Value);

	//! Set the font size:
	void setFontSize(int);

	//! Set the Sun altitude at twilight:
	void setSunAltitude(int);

	//! Set the Sun altitude at twilight:
	void setHorizAltitude(int);


	//! get Show Flags from current configuration:
	bool getShowFlags(int);

	//! get the current font color:
	Vec3f getFontColor(void);
	
	//! get current font size:
	int getFontSize(void);

	//! get current Sun altitude at twilight:
	int getSunAltitude(void);

	//! get current Horizon altitude:
	int getHorizAltitude(void);


public slots:
//! Set whether observability will execute or not:
	void enableObservability(bool b);

private:


//! Stuff for the configuration GUI:
	ObservabilityDialog* configDialog;
	QByteArray normalStyleSheet;
	QByteArray nightStyleSheet;


//! Computes the Hour Angle (culmination=0h) in absolute value (from 0h to 12h).
//! @param latitude latitude of the observer (in radians).
//! @param elevation elevation angle of the object (horizon=0) in radians.
//! @param declination declination of the object in radians. 
	virtual double HourAngle(double latitude,double elevation,double declination);

//! Computes the Hour Angle for a given Right Ascension and Sidereal Time.
//! @param RA right ascension (hours).
//! @param ST sidereal time (degrees).
	virtual double HourAngle2(double RA, double ST);

//! Solves Moon/Sun/Planet Rise/Set/Transit times for the current Julian day. This function updates the variables MoonRise, MoonSet, MoonCulm. Returns success status.
//! @param Kind is 1 for Sun, 2 for Moon, 3 for Solar-System planet.
	virtual bool SolarSystemSolve(StelCore* core, int Kind);

//! Finds the heliacal rise/set dates of the year for the currently-selected object.
//! @param Rise day of year of the Acronycal rise.
//! @param Set day of year of the Acronycal set.
//! @param Rise2 day of year of the Cosmical rise.
//! @param Set2 day of year of the Cosmical set.
	virtual int CheckAcro(int &Rise, int &Set, int &Rise2, int &Set2);


//! computes the Sun or Moon coordinates at a given Julian date.
//! @param core the stellarium core.
//! @param JD double for the Julian date.
//! @param RASun right ascension of the Sun (in hours).
//! @param DecSun declination of the Sun (in radians).
//! @param RAMoon idem for the Moon.
//! @param DecMoon idem for the Moon.
//! @param EclLon is the module of the vector product of Heliocentric Ecliptic Coordinates of Sun and Moon (projected over the Ecliptic plane). Useful to derive the dates of Full Moon.
//! @param getBack controls whether Earth and Moon must be returned to their original positions after computation.
	virtual void getSunMoonCoords(StelCore* core, double JD, double &RASun, double &DecSun, double &RAMoon, double &DecMoon, double &EclLon, bool getBack);


//! computes the selected-planet coordinates at a given Julian date.
//! @param core the stellarium core.
//! @param JD double for the Julian date.
//! @param RA right ascension of the planet (in hours).
//! @param Dec declination of the planet (in radians).
//! @param getBack controls whether the planet must be returned to its original positions after computation.
	virtual void getPlanetCoords(StelCore* core, double JD, double &RA, double &Dec, bool getBack);

//! Comptues the Earth-Moon distance (in AU) at a given Julian date. The parameters are similar to those of getSunMoonCoords or getPlanetCoords.
	virtual void getMoonDistance(StelCore* core, double JD, double &Distance, bool getBack);

//! Returns the angular separation (in radians) between two points.
//! @param RA1 right ascension of point 1 (in hours)
//! @param Dec1 declination of point 1 (in radians)
//! @param RA2 idem for point 2
//! @param Dec2 idem for point 2
	virtual double Lambda(double RA1, double Dec1, double RA2, double Dec2);

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

//! Returns a string of range dates (e.g. "25 Apr - 10 May") from a two Days of Year (integer).
//! @param fDoY first Day of the year.
//! @param sDoY second Day of the year.
	virtual QString RangeCalenDates(int fDoY, int sDoY);

//! Just subtracts/adds 24h to a RA (or HA), to make it fall within 0-24h.
//! @param RA right ascension (in hours).
	virtual double toUnsignedRA(double RA);

//! Computes the RA, Dec and Rise/Set Sid. times of the selected planet for each day of the current year.
//! @param core the current Stellarium core.
	virtual void PlanetRADec(StelCore *core);

//! Computes the Sun's RA and Dec for each day of a given year.
//! @param core current Stellarium core.
	virtual void SunRADec(StelCore* core);

//! Computes the Sun's Sid. Times at astronomical twilight (for each year's day)
	virtual void SunHTwi();

//! Just convert the Vec3d named TempLoc into RA/Dec:
	virtual void toRADec(Vec3d TempLoc, double &RA, double &Dec);

//! Vector to store the Julian Dates for the current year:
	double yearJD[366];

//! Check if a source is observable during a given date:
//! @aparm i the day of the year.
	virtual bool CheckRise(int i);

//! Some useful constants and variables(almost self-explanatory).
	double Rad2Deg, Rad2Hr, AstroTwiAlti, UA, TFrac, JDsec, Jan1stJD, halfpi, MoonT, nextFullMoon, prevFullMoon, RefFullMoon, GMTShift, MoonPerilune,RefracHoriz,HorizAlti;

//! RA, Dec, observer latitude, object's elevation, and Hour Angle at horizon.
	double selRA, selDec, mylat, mylon, alti, horizH, culmAlt, myJD;

//! Vectors to store Sun's RA, Dec, and Sid. Time at twilight and rise/set.
	double SunRA[366], SunDec[366], SunSidT[4][366];

//! Vectors to store planet's RA, Dec, and Sid. Time at rise/set.
	double ObjectRA[366], ObjectDec[366], ObjectH0[366], ObjectSidT[2][366];

//! Rise/Set/Transit times for the Moon at current day:
	double MoonRise, MoonSet, MoonCulm, lastJDMoon;

//! Vector of Earth position through the year.
	Vec3d EarthPos[366];

//! Position of the observer relative to the Earth Center or other coordinates:
	Vec3d ObserverLoc, Pos0, Pos1, Pos2, RotObserver; //, Pos3;

//! Matrix to transform coordinates for Sun/Moon ephemeris:
	Mat4d LocTrans;

//! Pointer to the Earth, Moon, and planet:
	Planet* myEarth;
	Planet* myMoon;
	Planet* myPlanet;


//! Current simulation year and number of days in the year.;
	int currYear, nDays, iAltitude, iHorizAltitude;

//! Useful auxiliary strings, to help checking changes in source/observer. Also to store results that must survive between iterations.
	QString selName, bestNight, ObsRange, objname, AcroCos;

//! Strings to save ephemeris Times:
	QString RiseTime, SetTime, CulmTime;

//! Just the names of the months.
	QString months[12];

//! Equatorial and local coordinates of currently-selected source.
	Vec3d EquPos, LocPos;

//! Some booleans to check the kind of source selected and the kind of output to produce.
	bool isStar,isMoon,isSun,isScreen, raised, configChanged, souChanged;
	int LastObject;

//! Some booleans to select the kind of output.
	bool show_AcroCos, show_Good_Nights, show_Best_Night, show_Today, show_FullMoon; //, show_Crescent, show_SuperMoon;

//! Parameters for the graphics (i.e., font, icons, etc.):
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

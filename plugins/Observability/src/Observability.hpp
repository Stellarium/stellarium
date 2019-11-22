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
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */
#ifndef OBSERVABILITY_HPP
#define OBSERVABILITY_HPP

#include "StelModule.hpp"
#include <QFont>
#include <QString>
#include <QPair>
#include "VecMath.hpp"
#include "SolarSystem.hpp"
#include "Planet.hpp"
#include "StelFader.hpp"

class QPixmap;
class StelButton;
class ObservabilityDialog;

/*! @defgroup observability Observability Analysis Plug-in
@{
It provides an observability report for the currently selected object, or for
the point in the center of the screen if no object is selected.

The output is drawn directly onto the viewport. The color and font size can be
selected by the user from the plug-in's configuration window.

@see ObservabilityDialog
@todo Find a way to (optionally) put the report in the upper left corner infobox.
@todo Decide whether to use flags or separate getters/setters to communicate with
the configuration window; if using flags, implement them properly w Qt.
@todo Handle re-loading of the Solar System at runtime.
@todo For each suspicious member variable, check if it can't be actually a local variable.
@}
*/

//! Main class of the %Observability Analysis plug-in.
//! @ingroup observability
//! @class Observability
class Observability : public StelModule
{
	Q_OBJECT
	Q_PROPERTY(bool flagShowReport
		   READ isShownReport
		   WRITE showReport
		   NOTIFY flagReportVisibilityChanged
		   )
public:
	Observability();
	virtual ~Observability();
	virtual void init();
	virtual void update(double) {;}
	virtual void draw(StelCore* core);
	virtual double getCallOrder(StelModuleActionName actionName) const;

	//! Implement this to tell the main Stellarium GUI that there is a GUI element to configure this plugin.
	virtual bool configureGui(bool show=true);


	//! Read (or re-read) settings from the main config file.
	//! Default values are provided for all settings.
	//! Called in init() and resetConfiguration().
	void loadConfiguration();

	//! get Show Flags from current configuration:
	bool getShowFlags(int);

	//! get the current font color:
	Vec3f getFontColor(void);
	
	//! get current font size:
	int getFontSize(void);

	//! Get the user-defined Sun altitude at twilight.
	//! @returns A value in degrees.
	int getTwilightAltitude();

	//! Get the user-defined altitude of the visual horizon.
	int getHorizonAltitude();

	bool isShownReport() const {return flagShowReport;}

signals:
	void flagReportVisibilityChanged(bool b);

public slots:
	//! Restore and reload the default plug-in settings.
	void resetConfiguration();
	//! Save the plug-in's configuration to the main configuration file.
	void saveConfiguration();
	
	//! @name Fields displayed in the observability report.
	//! @{
	
	//! Display today's events (rise, set and transit times).
	void enableTodayField(bool enabled = true);
	//! Display acronychal and cosmical rising/setting.
	void enableAcroCosField(bool enabled = true);
	//! Display nights when the object is above the horizon after darkness.
	void enableGoodNightsField(bool enabled = true);
	//! Display when selected object is in opposition.
	void enableOppositionField(bool enabled = true);
	//! Display date of the full moon.
	//! Has any effect only if the Moon is selected.
	void enableFullMoonField(bool enabled = true);
	//! @}
	
	
	//! Set the color of the font used to display the report.
	//! Applies only to what is drawn on the viewport.
	//! @param color Color vector in Stellarium's RGB format.
	void setFontColor(const Vec3f& color);
	//! Set the size of the font used to display the report.
	//! Applies only to what is drawn on the viewport.
	void setFontSize(int size);

	//! Set the angular altitude below the horizon of the Sun at twilight.
	//! This determines the boundaries of day/night for observation purposes.
	//! @param altitude An angle in degrees; as this is altitude below
	//! the horizon, it should be negative.
	//! @note If you want the slider in the configuration dialog to be inverted,
	//! you can use the @c invertedAppearance property of QSlider.
	//! @todo Check if parameter is in the acceptable range?
	void setTwilightAltitude(int altitude);

	//! Set the angular altitude of the visual horizon.
	//! @param altitude An angle in degrees.
	//! @todo Check if parameter is in the acceptable range?
	void setHorizonAltitude(int altitude);
	
	//! Controls whether an observability report will be displayed.
	void showReport(bool b);

	
private slots:
	//! Retranslates the user-visible strings when the language is changed. 
	void updateMessageText();

	
private:
	//! Configuration window.
	ObservabilityDialog* configDialog;

	void setDateFormat(bool b) { dmyFormat=b; }
	bool getDateFormat(void) { return dmyFormat; }

	//! Computes the Hour Angle (culmination=0h) in absolute value (from 0h to 12h).
	//! @todo The hour angle of what, exactly? --BM
	//! @param latitude latitude of the observer (in radians).
	//! @param elevation elevation angle of the object (horizon=0) in radians.
	//! @param declination declination of the object in radians.
	double calculateHourAngle(double latitude, double elevation, double declination);

	//! Computes the Hour Angle for a given Right Ascension and Sidereal Time.
	//! @param RA right ascension (hours).
	//! @param ST sidereal time (degrees).
	double HourAngle2(double RA, double ST);

	//! Solves Moon/Sun/Planet Rise/Set/Transit times for the current Julian day.
	//! This function updates the variables MoonRise, MoonSet, MoonCulm.
	//! Returns success status.
	//! @param[in] bodyType is 1 for Sun, 2 for Moon, 3 for Solar System object.
	bool calculateSolarSystemEvents(StelCore* core, int bodyType);

	//! Finds the acronycal and cosmical rise/set dates of the year for the currently-selected object.
	//! @param[out] acroRise day of year of the Acronycal rise.
	//! @param[out] acroSet day of year of the Acronycal set.
	//! @param[out] cosRise day of year of the Cosmical rise.
	//! @param[out] cosSet day of year of the Cosmical set.
	//! @returns 0 if no dates found, 1 if acronycal dates exist,
	//! 2 if cosmical dates exist, and 3 if both are found.
	int calculateAcroCos(int& acroRise, int& acroSet, int& cosRise, int& cosSet);


	//! Finds the Heliacal rise/set dates of the year for the currently-selected object.
	//! @param imethod Determines the algorithm to use (not yet implemented).
	//! @param[out] heliRise day of year of the Heliacal rise.
	//! @param[out] heliSet day of year of the Heliacal set.
	//! @returns 0 if no dates found and 1 otherwise.
	int calculateHeli(int imethod, int& heliRise, int& heliSet);


	//! Computes the Sun or Moon coordinates at a given Julian date.
	//! @param core the stellarium core.
	//! @param JD QPair of double for the Julian date: first=JD_UT and .second=JDE_DT
	//! @param RASun right ascension of the Sun (in hours).
	//! @param DecSun declination of the Sun (in radians).
	//! @param RAMoon idem for the Moon.
	//! @param DecMoon idem for the Moon.
	//! @param EclLon is the module of the vector product of Heliocentric Ecliptic Coordinates
	//!        of Sun and Moon (projected over the Ecliptic plane). Useful to derive the dates
	//!        of Full Moon.
	//! @param getBack controls whether Earth and Moon must be returned to their original
	//!        positions after computation.
	void getSunMoonCoords(StelCore* core, QPair<double, double> JD,
			      double& raSun, double& decSun,
			      double& raMoon, double& decMoon,
			      double& eclLon, bool getBack);


	//! computes the selected-planet coordinates at a given Julian date.
	//! @param core the stellarium core.
	//! @param JD QPair for the Julian date: .first=JD(UT), .second=JDE
	//! @param RA right ascension of the planet (in hours).
	//! @param Dec declination of the planet (in radians).
	//! @param getBack controls whether the planet must be returned to its original positions
	//!        after computation.
	void getPlanetCoords(StelCore* core,QPair<double, double> JD,
			     double &RA, double &Dec, bool getBack);

	//! Computes the Earth-Moon distance (in AU) at a given Julian date.
	//! The parameters are similar to those of getSunMoonCoords() or getPlanetCoords().
	void getMoonDistance(StelCore* core, QPair<double, double> JD,
			     double& distance, bool getBack);

	//! Returns the angular separation (in radians) between two points.
	//! @param RA1 right ascension of point 1 (in hours)
	//! @param Dec1 declination of point 1 (in radians)
	//! @param RA2 idem for point 2
	//! @param Dec2 idem for point 2
	double Lambda(double RA1, double Dec1, double RA2, double Dec2);

	//! Converts a time span in hours (given as double) in hh:mm:ss (integers).
	//! @param t time span (double, in hours).
	//! @param h hour (integer).
	//! @param m minute (integer).
	//! @param s second (integer).
	void double2hms(double t, int &h,int &m,int &s);

	//! Just returns the sign of a double;
	double sign(double d);

	//! Get a date string ("25 Apr") from an ordinal date (Xth day of the year).
	//! @param dayNumber The ordinal number of a day of the year. (For example,
	//! 25 April is the 115 or 116 day of the year.)
	//! @todo Determine the exact format - leap year handling, etc.
	QString formatAsDate(int dayNumber);

	//! Get a date range string ("25 Apr - 10 May") from two ordinal dates.
	//! @see formatAsDate()
	//! @param startDay number of the first day in the period.
	//! @param endDay number of the last day in the period.
	QString formatAsDateRange(int startDay, int endDay);

	//! Just subtracts/adds 24h to a RA (or HA), to make it fall within 0-24h.
	//! @param RA right ascension (in hours).
	double toUnsignedRA(double RA);

	//! Prepare arrays with data for the selected object for each day of the year.
	//! Computes the RA, Dec and rise/set sidereal times of the selected planet
	//! for each day of the current year.
	//! @param core the current Stellarium core.
	void updatePlanetData(StelCore* core);

	//! Computes the Sun's RA and Dec for each day of a given year.
	//! @param core current Stellarium core.
	void updateSunData(StelCore* core);

	//! Computes the Sun's Sid. Times at astronomical twilight (for each year's day)
	void updateSunH();

	//! Convert an equatorial position vector to RA/Dec.
	void toRADec(Vec3d vec3d, double& ra, double& dec);

	//! Table containing the Julian Dates of the days of the current year.
	QPair<double, double> yearJD[366]; // GZ: This had to become a QPair of JD.first=JD_UT, JD.second=JDE

	//! Check if a source is observable during a given date.
	//! @param i the day of the year.
	bool CheckRise(int day);

	//! Some useful constants (almost self-explanatory).
	static const double Rad2Deg, Rad2Hr, UA, TFrac, halfpi, MoonT, RefFullMoon, MoonPerilune;

	//! Some useful variables(almost self-explanatory).
	double nextFullMoon, prevFullMoon, GMTShift, Jan1stJD;

	//! User-defined angular altitude of astronomical twilight in radians.
	//! See setTwilightAltitude() and getTwilightAltitude().
	double twilightAltRad;

	//! User-defined angular altitude of astronomical twilight in degrees.
	//! See setTwilightAltitude() and getTwilightAltitude().
	int twilightAltDeg;
	
	//! Geometric altitude at refraction-corrected horizon.
	double refractedHorizonAlt;

	//! User-defined angular altitude of the visual horizon in radians.
	double horizonAltitude;

	//! User-defined angular altitude of the visual horizon in degrees.
	int horizonAltDeg;

	//! RA, Dec, observer latitude, object's elevation, and Hour Angle at horizon.
	double selRA, selDec, mylat, mylon, alti, horizH, culmAlt;

	//! Some place to keep JD and JDE. .first is JD(UT), .second is for the fitting JDE.
	QPair<double, double> myJD;

	//! Vectors to store Sun's RA, Dec, and Sid. Time at twilight and rise/set.
	double sunRA[366];
	double sunDec[366];

	//! Sidereal time of the Sun at twilight and rise/set through the year.
	double sunSidT[4][366];

	//! Vectors to store planet's RA, Dec, and Sid. Time at rise/set.
	double objectRA[366]; 
	double objectDec[366];
	double objectH0[366];

	//! Table of the sidereal time of the object's rising/setting.
	double objectSidT[2][366];

	//! Rise/Set/Transit times for the Moon at current day:
	double MoonRise, MoonSet, MoonCulm, lastJDMoon;

	//! Vector of Earth position through the year.
	Vec3d EarthPos[366];

	//! Position of the observer relative to the Earth Center or other coordinates:
	Vec3d ObserverLoc, Pos1, Pos2, RotObserver; //, Pos3;

	//! Matrix to transform coordinates for Sun/Moon ephemeris:
	Mat4d LocTrans;

	//! Pointer to the Earth, Moon, and planet:
	Planet* myEarth;
	Planet* myMoon;
	Planet* myPlanet;

	//! Current simulation year.
	int curYear;
	//! Days in the current year (366 on leap years).
	int nDays;

	//! Untranslated name of the currently selected object.
	//! Used to check if the selection has changed.
	QString selName;
	//! Cached copy of the "best night" line in the report.
	QString lineBestNight;
	//! Cached copy of the line reporting when the target is observable.
	QString lineObservableRange;
	//! Cached copy of the line reporting the acronical/cosmical rise and set.
	QString lineAcroCos, lineHeli;

	//! Strings to save ephemeris Times:
	QString RiseTime, SetTime, CulmTime;

	//! Using for storage date format [i18n]
	bool dmyFormat;

	//! Equatorial and local coordinates of currently-selected source.
	Vec3d EquPos, LocPos;

	//! Some booleans to check the kind of source selected and the kind of output to produce.
	bool isStar, isMoon, isSun, isScreen;

	//! This really shouldn't be handled like this...
	bool hasRisen;
	bool configChanged;
	bool souChanged;
	//! The last object type for which calculateSolarSystemEvents() was called.
	int lastType;
	
	//! @name Flags controlling report contents.
	//! @{
	bool show_AcroCos, show_Good_Nights, show_Best_Night, show_Today, show_FullMoon; //, show_Crescent, show_SuperMoon;
	//! @}

	//! @name GUI elements
	//! @{
	//! Parameters for the graphics.
	QFont font;
	Vec3f fontColor;
	bool flagShowReport;
	int fontSize;
	StelButton* button;
	//! @}

	//! @name Cached translated GUI strings.
	//! @todo Decide whether to keep translation caching.
	//! @{
	QString msgSetsAt, msgRoseAt, msgSetAt, msgRisesAt, msgCircumpolar, msgNoRise, msgCulminatesAt, msgCulminatedAt, msgH, msgM, msgS;
	QString msgSrcNotObs, msgNoACRise, msgGreatElong, msgLargSSep, msgNone, msgAcroRise, msgNoAcroRise, msgCosmRise, msgNoCosmRise;
        QString msgHeliRise, msgHeliSet, msgNoHeliRise;
	QString msgWholeYear, msgNotObs, msgAboveHoriz, msgToday, msgThisYear, msgPrevFullMoon, msgNextFullMoon;
	//! @}
};

#include <QObject>
#include "StelPluginInterface.hpp"

class ObservabilityStelPluginInterface : public QObject, public StelPluginInterface
{
       Q_OBJECT
       Q_PLUGIN_METADATA(IID StelPluginInterface_iid)
       Q_INTERFACES(StelPluginInterface)
public:
       virtual StelModule* getStelModule() const;
       virtual StelPluginInfo getPluginInfo() const;
       virtual QObjectList getExtensionList() const { return QObjectList(); }
};

#endif /*OBSERVABILITY_HPP*/

/*
 * Copyright (C) 2020 Georg Zotti
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

#ifndef CALENDAR_HPP
#define CALENDAR_HPP

#include <QString>
#include <QObject>
#include "StelUtils.hpp"
#include "StelTranslator.hpp"
#include "StelLocation.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"

//! @class Calendar
//! Superclass for all calendars, capable of a multitude of astronomical calculations.
//! @author Georg Zotti
//! @ingroup calendars
//!
//! Stellarium uses Julian Day numbers internally, and the conventional approach of using the Gregorian calendar for dates after 1582-10-15.
//! For dates before that, the Julian calendar is used, in the form finalized by Augustus and running unchanged since 8AD.
//! Astronomical year counting implies having a year 0, while some calendars adhere to historical counting like 1 B.C., 1 A.D.
//! Some European countries, especially the Protestant countries, delayed the calendar switch well into the 18th century.
//! Other cultures have various other calendar schemes.
//! All calendars have a numerical vector of time elements and may have individual string lists for names of elements like "week days" and "months".
//! If the pattern of weeks and months does not apply, the names may refer to other structural elements.
//!
//! The most important source used here is:
//! Edward M. Reingold, Nachum Dershowitz: Calendrical Calculations
//! in various editions (1997-2018), referred to as CC. In particular CC:ME=2nd "Millennium edition", CC:UE=4th "Ultimate Edition", with amendments published until January 2022.
//! It describes the Gregorian calendar with year zero, and the Julian without. In this plugin only, we adhere to this convention to finally have all possible variations of dates.
//! It does not use JD directly but a number called Rata Die (RD=JD-1721424.5), a day count starting at midnight of January 1st, 1 AD (Proleptic Gregorian).
//! with functions including "fixed" in their names. Our calendar subclasses can use Calendar::fixedFromJD(jd) / Calendar::jdFromFixed(rd).
//! Other functions with "moment" in their names use double arguments which are RD with fractions of day. Argument names with "_ut" added indicate the RD is given in UT, not local time.
//!
//! Most functions are static, with names derived from the functions given in Lisp in CC:UE. Most calendars have dates which can be described in lists of integers.
//! Some functions require a StelLocation argument. For accessability as Stellarium scripting functions, argument order was changed to place the location argument as last
//! and declare it with a default value of the current location, so that it can be used in scripting. Currently, other, non-default StelLocation arguments cannot be used with scripting.
//!
//! @note The astronomical functions in parts make use of Stellarium's built-in astronomy instead of the algorithms given in CC. A notable difference consists already around DeltaT,
//! where Stellarium uses a special correction for the Lunar theory which other software titles don't use, therefore testing the results in the book's appendix will provide slight deviations.
class Calendar : public QObject
{
	Q_OBJECT

public:
	//! enum from CC.UE-ch1.12.
	typedef enum { sunday  = 0, monday, tuesday, wednesday, thursday, friday, saturday } Day;
	typedef enum { spring  = 0, summer       = 90, autumn   = 180, winter      = 270} Season; // CC.UE 3.5
	typedef enum { newMoon = 0, firstQuarter = 90, fullMoon = 180, lastQuarter = 270} Phase;  // CC.UE 14.59-62
	Q_ENUM(Day)
	Q_ENUM(Season)
	Q_ENUM(Phase)

	Calendar(double jd):JD(jd) { setObjectName("Calendar"); }

	virtual ~Calendar() Q_DECL_OVERRIDE {}

public slots:
	//! Translate e.g. stringlists of part names
	virtual void retranslate(){}

	//! Set a calendar date from the Julian day number
	//! Subclasses set JD and compute the parts and possibly other data
	//! This triggers the partsChanged() signal
	virtual void setJD(double JD){Q_UNUSED(JD)}

	//! Get Julian day number from a calendar date
	virtual double getJD() const { return JD;}

	//! set date from a vector of calendar date elements sorted from the largest to the smallest.
	//! This triggers the jdChanged() signal
	//! Note that this must not change the time of day! You must retrieve the time from the current JD before recomputing a new JD.
	//! You can also use this with scripting by calling the actual calendars, like
	//! GregorianCalendar.setDate([2023, 3, 4]); // This changes the date in the program!
	//! MayaLongCountCalendar.setDate([13, 0, 0, 0, 1]); // This changes the date in the program!
	//! @note No check is made for the number of elements, which depends on the actual calendar.
	virtual void setDate(QVector<int> parts){Q_UNUSED(parts)}

	//! get a vector of calendar date elements sorted from the largest to the smallest.
	//! The order depends on the actual calendar
	//! @note this also works with scripting. The result can be further processed. E.g.
	//! core.output("Baktun: " + MayaLongCountCalendar.getDateL()[0]);
	virtual QVector<int> getDate() const { return parts; }

	//! get a stringlist of calendar date elements sorted from the largest to the smallest.
	//! The order depends on the actual calendar
	//! For scripting use, you can extract single elements, like:
	//! core.output("Tzolkin name: " + MayaTzolkinCalendar.getDateStrings()[1]);
	virtual QStringList getDateStrings() const {return QStringList();}

	//! get a formatted complete string for a date. The default implementation just concatenates all strings from getDateStrings() with a space in between.
	virtual QString getFormattedDateString() const;

	//! get a formatted complete string for a date. This implementation just converts and concatenates all ints with sep in between.
	static QString getFormattedDateString(QVector<int> date, QString sep=" ");

public:
	//constexpr static const double J2000=2451545.0;
	constexpr static const double j2000=730120.5; //!< RD of J2000.0 (CC:UE 14.19)
	constexpr static const double jdEpoch=-1721424.5;
	constexpr static const double mjdEpoch=678576.0;
	constexpr static const int bogus=-1000000; // special value to indicate invalid result in some calendars.

public slots:

	//! Mostly for testing: return RD of current time.
	static double rdNow(){return momentFromJD(StelApp::getInstance().getCore()->getJD(), false);}
	static double rdJ2000() {return 730120.5;} // for testing only!
	//! Interfacing function from Reingold/Dershowitz: Calendrical Calculations
	//! Returns a "moment" in RD that represents JD.
	//! Stellarium extension: include local time zone offset.
	//! In most calendar interfacing functions, respectUTCoffset should be true.
	static double momentFromJD(double jd, bool respectUTCoffset);
	inline static int fixedFromMoment(double rd) { return std::lround(std::floor(rd));}
	inline static double timeFromMoment(double rd) { return StelUtils::fmodpos(rd, 1.);}
	//! Interfacing function from Reingold/Dershowitz: Calendrical Calculations
	//! Returns a fixed date in RD that represents noon of JD.
	//! Stellarium extension: optionally includes local time zone offset.
	//! In most calendar interfacing functions, respectUTCoffset should be true.
	inline static int fixedFromJD(double jd, bool respectUTCoffset) { return std::lround(std::floor(momentFromJD(jd, respectUTCoffset)));}
	inline static double momentFromMJD(double mjd) { return mjd+mjdEpoch;}

	//! interfacing function from Reingold/Dershowitz: Calendrical Calculations
	//! Returns a JD from an RD "moment" (including fractions of day)
	//! Stellarium extension: optionally includes local time zone offset.
	//! In most calendar interfacing functions, respectUTCoffset should be true.
	static double jdFromMoment(double rd, bool respectUTCoffset);
	//! interfacing function from Reingold/Dershowitz: Calendrical Calculations
	//! Returns a JD from an RD "moment" (including fractions of day)
	//! Stellarium extension: optionally includes local time zone offset.
	//! In most calendar interfacing functions, respectUTCoffset should be true.
	inline static double jdFromFixed(double rd, bool respectUTCoffset) { return jdFromMoment(rd, respectUTCoffset);}
	inline static double mjdFromFixed(double rd) { return rd-mjdEpoch;}

	//! weekday from RD date. CC.UE(1.60).
	inline static int dayOfWeekFromFixed(int rd) { return StelUtils::imod(rd, 7); }
	//! @Returns the R.D. of the nearest weekday k on or before rd
	inline static int kdayOnOrBefore(const Calendar::Day k, const int rd) { return rd-dayOfWeekFromFixed(rd-k);}
	//! @Returns the R.D. of the nearest weekday k on or after rd
	inline static int kdayOnOrAfter(const Calendar::Day k, const int rd) { return kdayOnOrBefore(k, rd+6);}
	//! @Returns the R.D. of the nearest weekday k around rd
	inline static int kdayNearest(const Calendar::Day k, const int rd) { return kdayOnOrBefore(k, rd+3);}
	//! @Returns the R.D. of the nearest weekday k before rd
	inline static int kdayBefore(const Calendar::Day k, const int rd) { return kdayOnOrBefore(k, rd-1);}
	//! @Returns the R.D. of the nearest weekday k after rd
	inline static int kdayAfter(const Calendar::Day k, const int rd) { return kdayOnOrBefore(k, rd+7);}

	//! Interval modulus, CC.UE 1.24
	//! @returns x shifted to lie inside [a...b), EXCLUDING b.
	static double modInterval(double x, double a, double b);
	//! Interval modulus, CC.UE 1.24: This EXCLUDES the upper limit! Use StelUtils::amod(x, b) for CC's (x)mod[1..b]
	static int modInterval(int x, int a, int b);

	//! Reingold-Dershowitz CC.UE 1.48.
	static int rdCorrSum(QVector<int>parts, QVector<int>factors, int corr);
	int rdCorrSum(QVector<int>factors, int corr){return rdCorrSum(parts, factors, corr);}

	//! Split integer to mixed-radix vector. Reingold-Dershowitz CC.UE 1.42
	static QVector<int> toRadix(int num, QVector<int>radix);

	//! Intersect a collection of candidates against a range of values.
	//! @return a new QVector<int> with all elements from candidates which lie in range
	static QVector<int> intersectWithRange(QVector<int>cand, QVector<int>range);

	// ASTRONOMICAL FUNCTIONS AND DEFINITIONS FROM CH.14
public:
	static const StelLocation urbana;
	static const StelLocation greenwich;
	static const StelLocation mecca;
	static const StelLocation tehran;
	static const StelLocation paris;
	static const StelLocation jerusalem;
	static const StelLocation acre;
	static const StelLocation padua; // (CC:UE 14.85)

public slots:
	//! retrieve a StelLocation from our database based on its name
	//! There is no check! Returned location may be default/empty.
	static StelLocation location(const QString &name);

	//! Return azimuth direction (degrees from North) from locFrom to locTo.
	static double direction(const StelLocation &locFrom, const StelLocation &locTo);
	static double direction(const QString &locFrom, const QString &locTo){return direction(location(locFrom), location(locTo));}

	// 14.2 Time
	//! @return longitude-dependent time offset in fractions of day.
	static double zoneFromLongitude(double lngDeg){return lngDeg/360.;}

	//! @return rd or fractions of day reduced from local mean (longitude dependent) time to UT
	static double universalFromLocal(double rd_loc, const StelLocation &loc=StelApp::getInstance().getCore()->getCurrentLocation());
	static double universalFromLocal(double rd_loc, const QString &loc){return universalFromLocal(rd_loc, location(loc));}
	//! @return rd or fractions of day augmented from UT to local mean time (longitude dependent)
	static double localFromUniversal(double rd_ut, const StelLocation &loc=StelApp::getInstance().getCore()->getCurrentLocation());
	static double localFromUniversal(double rd_ut, const QString &loc){return localFromUniversal(rd_ut, location(loc));}
	//! @return rd or fractions of day augmented from UT to local zone time
	static double standardFromUniversal(double rd_ut, const StelLocation &loc=StelApp::getInstance().getCore()->getCurrentLocation());
	static double standardFromUniversal(double rd_ut, const QString &loc){return standardFromUniversal(rd_ut, location(loc));}
	//! @return rd or fractions of day reduced from local zone time to UT
	static double universalFromStandard(double rd_zone, const StelLocation &loc=StelApp::getInstance().getCore()->getCurrentLocation());
	static double universalFromStandard(double rd_zone, const QString &loc){return universalFromStandard(rd_zone, location(loc));}
	//! @return rd or fractions of day in local zone time from local mean time
	static double standardFromLocal(double rd_loc, const StelLocation &loc=StelApp::getInstance().getCore()->getCurrentLocation());
	static double standardFromLocal(double rd_loc, const QString &loc){return standardFromLocal(rd_loc, location(loc));}
	//! @return rd or fractions of day in local mean time (longitude dependent) from local zone time
	static double localFromStandard(double rd_zone, const StelLocation &loc=StelApp::getInstance().getCore()->getCurrentLocation());
	static double localFromStandard(double rd_zone, const QString &loc){return localFromStandard(rd_zone, location(loc));}
	//! @return DeltaT in fractions of day. This utilizes Stellarium's solution.
	static double ephemerisCorrection(double rd);
	//! Correct rd_ut to Dynamical time.
	static double dynamicalFromUniversal(double rd_ut);
	//! Correct rd_dt to Universal time.
	static double universalFromDynamical(double rd_dt);
	//! @return Julian centuries since j2000 in dynamical time (for ephemeris calculations) CC:UE 14.18
	static double julianCenturies(double rd_ut);

	// 14.3 The Day
	//! @return equation of time in fractions of a day
	//! @note We use functions from Stellarium instead of the functions from CC:UE 14.20.
	static double equationOfTime(double rd_ut);
	//! @return moment (RD in local mean solar time) corrected by equation of time (CC:UE 14.21)
	static double apparentFromLocal(double rd_local_mean, const StelLocation &loc=StelApp::getInstance().getCore()->getCurrentLocation());
	static double apparentFromLocal(double rd_local_mean, const QString &loc){return apparentFromLocal(rd_local_mean, location(loc));}
	//! @return moment (RD in local apparent solar time) corrected by equation of time into local mean solar time (CC:UE 14.22)
	static double localFromApparent(double rd_local_app, const StelLocation &loc=StelApp::getInstance().getCore()->getCurrentLocation());
	static double localFromApparent(double rd_local_app, const QString &loc){return localFromApparent(rd_local_app, location(loc));}
	//! @return moment (RD in UT) corrected into apparent time by equation of time and location (CC:UE 14.23)
	static double apparentFromUniversal(double rd_ut, const StelLocation &loc=StelApp::getInstance().getCore()->getCurrentLocation());
	static double apparentFromUniversal(double rd_ut, const QString &loc){return apparentFromUniversal(rd_ut, location(loc));}
	//! @return moment (RD in local apparent solar time) corrected by equation of time into UT (CC:UE 14.24)
	static double universalFromApparent(double rd_local_app, const StelLocation &loc=StelApp::getInstance().getCore()->getCurrentLocation());
	static double universalFromApparent(double rd_local_app, const QString &loc){return universalFromApparent(rd_local_app, location(loc));}
	//! @return RD (UT) of True (solar) Midnight from RD and location (CC:UE 14.25)
	static double midnight(int rd, const StelLocation &loc=StelApp::getInstance().getCore()->getCurrentLocation());
	static double midnight(int rd, const QString &loc){return midnight(rd, location(loc));}
	//! @return RD (UT) of True (Solar) Noon from RD and location (CC:UE 14.26)
	static double midday(int rd, const StelLocation &loc=StelApp::getInstance().getCore()->getCurrentLocation());
	static double midday(int rd, const QString &loc){return midday(rd, location(loc));}
	//! @return RD (Tehran zone time) of meridian transit.
	//! @note for testing only
	static double middayTehran(int rd){return standardFromUniversal(midday(rd, tehran), tehran);}

	//! @return sidereal time (at Greenwich) at moment rd_ut (CC:UE 14.27, but we use our own solution)
	//! @note this does not take location into account, it's sidereal time at Greenwich.
	static double siderealFromMoment(double rd_ut);

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// 14.4 The Year
	//! @return obliquity of the ecliptic, degrees (CC:UE 14.28, but we use our own solution)
	static double obliquity(double rd_ut);
	//! @return declination for time-dependent ecliptical coordinates [degrees] (CC:UE 14.29, but we use our own solution)
	static double declination(double rd_ut, double eclLat, double eclLong);
	//! @return right ascension for time-dependent ecliptical coordinates [degrees] (CC:UE 14.30, but we use our own solution)
	static double rightAscension(double rd_ut, double eclLat, double eclLong);
	//! @return solar longitude [degrees] for moment RD_UT (CC:UE 14.33, but we use our own solution)
	static double solarLongitude(double rd_ut);

	//! @return nutation in degrees for moment rd_ut (CC:UE 14.34, but we use our own solution)
	static double nutation(double rd_ut);
	//! @return aberration in degrees for moment rd_ut (CC:UE 14.35)
	static double aberration(double rd_ut);

	//! binary search for the moment when solar longitude reaches lng in the time between rdA and rdB (used in CC:UE 14.36)
	static double solarLongitudeInv(double lng, double rdA, double rdB);
	//! @return moment (RD_UT) of when the sun first reaches lng [degrees] after rd_ut  (CC:UE 14.36)
	static double solarLongitudeAfter(double lng, double rd_ut);
	//! @return rd_ut of season begin  (CC:UE 14.37)
	static double seasonInGregorian(Calendar::Season season, int gYear);

	//! @return merely a test (CC:UE 14.38)
	static double urbanaWinter(int gYear);
	//! @return precession in ecliptical longitude since moment rd_dt (CC:UE 14.39, which is from Meeus AA 21.6/21.7.)
	static double precession(double rd_dt);
	//! @return altitude of the sun at loc, degrees. (CC:UE 14.41)
	static double solarAltitude(double rd_ut, const StelLocation &loc=StelApp::getInstance().getCore()->getCurrentLocation());
	static double solarAltitude(double rd_ut, const QString &loc){return solarAltitude(rd_ut, location(loc));}

	// 14.5 Astronomical Solar Calendars
	//! @return an estimate for the moment (RD_UT) before rd_ut when sun has reached lambda (CC:UE 14.42)
	static double estimatePriorSolarLongitude(double lambda, double rd_ut);
	// 14.6 The Month
	//! @return RD of n-th new moon after RD0. (CC:UE 14.45, following Meeus AA2, ch.49)
	static double nthNewMoon(int n);
	//! @return RD of New Moon before rd_ut (CC:UE 14.46)
	static double newMoonBefore(double rd_ut);
	//! @return RD of New Moon on or after rd_ut (CC:UE 14.47)
	static double newMoonAtOrAfter(double rd_ut);
	//! @return lunar longitude at rd_ut (CC:UE 14.48, actually Meeus AA2 ch.47)
	static double lunarLongitude(double rd_ut);
	//! @return lunar latitude at rd_ut (CC:UE 14.63, actually Meeus AA2 ch.47)
	static double lunarLatitude(double rd_ut);
	//! @return lunar distance [metres] at rd_ut (CC:UE 14.65, actually Meeus AA2 ch.47)
	static double lunarDistance(double rd_ut);
	//! @return mean lunar longitude at Julian century c (CC:UE 14.49)
	static double meanLunarLongitude(double c);
	//! @return lunar elongation at Julian century c (CC:UE 14.50)
	static double lunarElongation(double c);
	//! @return solar anomaly at Julian century c (CC:UE 14.51)
	static double solarAnomaly(double c);
	//! @return lunar anomaly at Julian century c (CC:UE 14.52)
	static double lunarAnomaly(double c);
	//! @return longitude of lunar node at Julian century c (CC:UE 14.53)
	static double moonNode(double c);
	//! @return longitude of lunar node at moment rd_ut (CC:UE 14.54)
	//! @note this is shifted into [-180...180)
	static double lunarNode(double rd_ut);
	//! @return sidereal lunar longitude counted from some other point. (CC:UE 14.55)
	static double siderealLunarLongitude(double rd_ut, double siderealStart);
	//! @return lunar phase (angular difference in ecliptical longitude from the sun)
	//! at moment rd_ut (CC:UE 14.56)
	static double lunarPhase(double rd_ut);
	//! binary search for the moment when lunar phase reaches phi in the time between rdA and rdB (CC:UE 14.57)
	static double lunarPhaseInv(double phi, double rdA, double rdB);
	//! @return rd of moment when phase was phi [degrees] before rd_ut (CC:UE 14.57)
	static double lunarPhaseAtOrBefore(double phi, double rd_ut);
	//! @return rd of moment when phase will next be phi [degrees] after rd_ut (CC:UE 14.58)
	static double lunarPhaseAtOrAfter(double phi, double rd_ut);
	//! @return altitude of the moon at loc, degrees. (CC:UE 14.64)
	//! @note The result has not been corrected for parallax or refraction
	static double lunarAltitude(double rd_ut, const StelLocation &loc=StelApp::getInstance().getCore()->getCurrentLocation());
	static double lunarAltitude(double rd_ut, const QString &loc){return lunarAltitude(rd_ut, location(loc));}
	//! @return lunar parallax [degrees] at RD rd_ut. (CC:UE 14.66)
	static double lunarParallax(double rd_ut, const StelLocation &loc=StelApp::getInstance().getCore()->getCurrentLocation());
	static double lunarParallax(double rd_ut, const QString &loc){return lunarParallax(rd_ut, location(loc));}
	//! @return altitude of the moon at loc, degrees. (CC:UE 14.67)
	static double topocentricLunarAltitude(double rd_ut, const StelLocation &loc=StelApp::getInstance().getCore()->getCurrentLocation());
	static double topocentricLunarAltitude(double rd_ut, const QString &loc){return topocentricLunarAltitude(rd_ut, location(loc));}

	// 14.7 Rising and setting
	//! @return rd of local mean solar time when sun is alpha degrees below math. horizon. (CC:UE 14.68)
	//! @note argument order is different from CC:UE
	//! @note not available to scripting.
	static double approxMomentOfDepression(double rd_loc, double alpha, bool early, const StelLocation &loc=StelApp::getInstance().getCore()->getCurrentLocation());
	//! @return sine of the angle alpha between where the sun is at
	//!   rd_ut and where it is at its position of interest (CC:UE 14.69)
	//! @note argument order is different from CC:UE
	//! @note not available to scripting.
	static double sineOffset(double rd_ut, double alpha, const StelLocation &loc=StelApp::getInstance().getCore()->getCurrentLocation());
	//! @return rd of , rd. (CC:UE 14.70)
	//! @note argument order is different from CC:UE
	//! @note not available to scripting.
	static double momentOfDepression(double rd_approx, double alpha, bool early, const StelLocation &loc=StelApp::getInstance().getCore()->getCurrentLocation());
	//! @return fraction of day (Zone Time!) of when sun is alpha degrees below horizon in the morning (or bogus when the sun does not reach it) (CC:UE 14.72)
	//! @note The angle argument alpha must be positive for negative solar elevations!
	//! @note argument order is different from CC:UE
	static double dawn(int rd, double alpha, const StelLocation &loc=StelApp::getInstance().getCore()->getCurrentLocation());
	static double dawn(int rd, double alpha, const QString &loc){return dawn(rd, alpha, location(loc));}
	//! @return fraction of day (Zone Time!) of when sun is alpha degrees below horizon in the evening (or bogus when the sun does not reach it) (CC:UE 14.74)
	//! @note The angle argument alpha must be positive for negative solar elevations!
	//! @note argument order is different from CC:UE
	static double dusk(int rd, double alpha, const StelLocation &loc=StelApp::getInstance().getCore()->getCurrentLocation());
	static double dusk(int rd, double alpha, const QString &loc){return dusk(rd, alpha, location(loc));}
	//! @return refraction at the mathematical horizon of location loc, taking into account dip correction for altitude (CC:UE 14.75)
	//! @note in the book a first argument t is specified but unused.
	static double refraction(const StelLocation &loc=StelApp::getInstance().getCore()->getCurrentLocation());
	static double refraction(const QString &loc){return refraction(location(loc));}
	//! @return fraction of day for the moment of sunrise in loc's standard (Zone) time (CC:UE 14.76)
	static double sunrise(int rd, const StelLocation &loc=StelApp::getInstance().getCore()->getCurrentLocation());
	static double sunrise(int rd, const QString &loc){return sunrise(rd, location(loc));}
	//! @return fraction of day for the moment of sunset in loc's standard (Zone) time (CC:UE 14.77)
	static double sunset(int rd, const StelLocation &loc=StelApp::getInstance().getCore()->getCurrentLocation());
	static double sunset(int rd, const QString &loc){return sunset(rd, location(loc));}

	//! For testing only:
	//! @return Standard (Zone) time of Paris dawn (-18°)
	static double dawnParis(int rd){return dawn(rd, 18., paris);}
	//! @return Standard (Zone) time of Jerusalem sunset
	static double sunsetJerusalem(int rd){return sunset(rd, jerusalem);}

	//! @return fraction of day (Zone Time!) of when sun is 7.5 degrees below horizon in the evening (or bogus when the sun does not reach it) (CC:UE 14.80)
	//! @note this is a criterium for some Jews only.
	static double jewishSabbathEnds(int rd, const StelLocation &loc=StelApp::getInstance().getCore()->getCurrentLocation()){return dusk(rd, 7.+5./60., loc);}
	static double jewishSabbathEnds(int rd, const QString &loc){return jewishSabbathEnds(rd, location(loc));}
	//! @return fraction of day (Zone Time!) of when sun is 4°40' degrees below horizon in the evening (or bogus when the sun does not reach it) (CC:UE 14.81)
	//! @note this may be the criterium for other days.
	static double jewishDusk(int rd, const StelLocation &loc=StelApp::getInstance().getCore()->getCurrentLocation()){return dusk(rd, 4.+40./60., loc);}
	static double jewishDusk(int rd, const QString &loc){return jewishDusk(rd, location(loc));}

	//! @return apparent altitude of moon (CC:UE 14.83)
	static double observedLunarAltitude(double rd_ut, const StelLocation &loc=StelApp::getInstance().getCore()->getCurrentLocation());
	static double observedLunarAltitude(double rd_ut, const QString &loc){return observedLunarAltitude(rd_ut, location(loc));}
	//! @return moment of moonrise in RD (Universal Time) (CC:UE 14.83)
	static double moonrise(int rd, const StelLocation &loc=StelApp::getInstance().getCore()->getCurrentLocation());
	static double moonrise(int rd, const QString &loc){return moonrise(rd, location(loc));}
	//! @return moment of moonset in RD (Universal Time) (CC:UE 14.84)
	static double moonset(int rd, const StelLocation &loc=StelApp::getInstance().getCore()->getCurrentLocation());
	static double moonset(int rd, const QString &loc){return moonset(rd, location(loc));}
	//! For testing only: Delivers local standard time
	static double moonriseMecca(int rd){return standardFromUniversal(moonrise(rd, mecca), mecca);}
	static double moonsetMecca(int rd){return standardFromUniversal(moonset(rd, mecca), mecca);}

	// 14.8 Times of Day
	//! @return local time of the start of Italian hour counting, a half hour after sunset (fraction of day)
	//! @note This extends CC:UE 14.86 by allowing another location. Use Calendar::padua for the default solution
	static double localZeroItalianHour(double rd_loc, const StelLocation &loc=StelApp::getInstance().getCore()->getCurrentLocation());
	static double localZeroItalianHour(double rd_loc, const QString &loc){return localZeroItalianHour(rd_loc, location(loc));}
	//! @return local time of the start of Italian-style hour counting, but starting at sunset (fraction of day)
	//! @note This extends CC:UE 14.86 by allowing another location and keeping the sunset time.
	static double localZeroSunsetHour(double rd_loc, const StelLocation &loc=StelApp::getInstance().getCore()->getCurrentLocation());
	static double localZeroSunsetHour(double rd_loc, const QString &loc){return localZeroSunsetHour(rd_loc, location(loc));}
	//! @return local time of the start of Babylonian hour counting with sunrise (fraction of day)
	//! @note This extends CC:UE 14.86 by allowing another location and showing the reverse counting.
	//! @todo Check this!
	static double localZeroBabylonianHour(double rd_loc, const StelLocation &loc=StelApp::getInstance().getCore()->getCurrentLocation());
	static double localZeroBabylonianHour(double rd_loc, const QString &loc){return localZeroBabylonianHour(rd_loc, location(loc));}
	//! @return local time from Italian time (CC:UE 14.87)
	static double localFromItalian(double rd_loc, const StelLocation &loc=StelApp::getInstance().getCore()->getCurrentLocation());
	static double localFromItalian(double rd_loc, const QString &loc){return localFromItalian(rd_loc, location(loc));}
	//! @return local time from Sunset time (extending idea from CC:UE 14.87)
	static double localFromSunsetHour(double rd_loc, const StelLocation &loc=StelApp::getInstance().getCore()->getCurrentLocation());
	static double localFromSunsetHour(double rd_loc, const QString &loc){return localFromSunsetHour(rd_loc, location(loc));}
	//! @return local time from Babylonian time (extending idea from CC:UE 14.87)
	//! @todo Check this!
	static double localFromBabylonian(double rd_loc, const StelLocation &loc=StelApp::getInstance().getCore()->getCurrentLocation());
	static double localFromBabylonian(double rd_loc, const QString &loc){return localFromBabylonian(rd_loc, location(loc));}
	//! @return Italian time from local time (CC:UE 14.88)
	static double italianFromLocal(double rd_loc, const StelLocation &loc=StelApp::getInstance().getCore()->getCurrentLocation());
	static double italianFromLocal(double rd_loc, const QString &loc){return italianFromLocal(rd_loc, location(loc));}
	//! @return Sunset time from local time (extending idea from CC:UE 14.88)
	static double sunsetHourFromLocal(double rd_loc, const StelLocation &loc=StelApp::getInstance().getCore()->getCurrentLocation());
	static double sunsetHourFromLocal(double rd_loc, const QString &loc){return sunsetHourFromLocal(rd_loc, location(loc));}
	//! @return Babylonian time from local time (extending idea from CC:UE 14.88)
	//! @todo Check this!
	static double babylonianFromLocal(double rd_loc, const StelLocation &loc=StelApp::getInstance().getCore()->getCurrentLocation());
	static double babylonianFromLocal(double rd_loc, const QString &loc){return babylonianFromLocal(rd_loc, location(loc));}

	//! @return length of a temporal day hour at date rd and location loc. (CC:UE 14.89)
	static double daytimeTemporalHour(const int rd, const StelLocation &loc=StelApp::getInstance().getCore()->getCurrentLocation());
	static double daytimeTemporalHour(const int rd, const QString &loc){return daytimeTemporalHour(rd, location(loc));}
	//! @return length of a temporal night hour at date rd and location loc.  (CC:UE 14.90)
	static double nighttimeTemporalHour(const int rd, const StelLocation &loc=StelApp::getInstance().getCore()->getCurrentLocation());
	static double nighttimeTemporalHour(const int rd, const QString &loc){return nighttimeTemporalHour(rd, location(loc));}
	//! @return standard time from "temporal" sundial time (CC:UE 14.91)
	static double standardFromSundial(const double rd_ut, const StelLocation &loc=StelApp::getInstance().getCore()->getCurrentLocation());
	static double standardFromSundial(const double rd_ut, const QString &loc){return standardFromSundial(rd_ut, location(loc));}

	// 14.9 Lunar Crescent Visibility
	//! @return elongation of the Moon (CC:UE 14.95)
	static double arcOfLight(double rd_loc);
	//! @return rd_ut of good lunar visibility (CC:UE 14.96)
	static double simpleBestView(int rd, const StelLocation &loc=StelApp::getInstance().getCore()->getCurrentLocation());
	static double simpleBestView(int rd, const QString &loc){return simpleBestView(rd, location(loc));}
	//! @return true or false according to Shaukat (CC:UE 14.97)
	static bool shaukatCriterion(int rd, const StelLocation &loc=StelApp::getInstance().getCore()->getCurrentLocation());
	static bool shaukatCriterion(int rd, const QString &loc){return shaukatCriterion(rd, location(loc));}
	static double arcOfVision(double rd_loc, const StelLocation &loc=StelApp::getInstance().getCore()->getCurrentLocation());
	static double arcOfVision(double rd_loc, const QString &loc){return arcOfVision(rd_loc, location(loc));}
	static double bruinBestView(int rd, const StelLocation &loc=StelApp::getInstance().getCore()->getCurrentLocation());
	static double bruinBestView(int rd, const QString &loc){return bruinBestView(rd, location(loc));}
	//! @return true or false according to Yallop (CC:UE 14.100)
	static bool yallopCriterion(int rd, const StelLocation &loc=StelApp::getInstance().getCore()->getCurrentLocation());
	static bool yallopCriterion(int rd, const QString &loc){return yallopCriterion(rd, location(loc));}
	static double lunarSemiDiameter(double rd_loc, const StelLocation &loc=StelApp::getInstance().getCore()->getCurrentLocation());
	static double lunarSemiDiameter(double rd_loc, const QString &loc){return lunarSemiDiameter(rd_loc, location(loc));}
	static double lunarDiameter(double rd_ut);
	//! @return whichever criterion is set. CC:UE 14.103 uses Shaukat's criterion.
	static bool visibleCrescent(int rd, const StelLocation &loc=StelApp::getInstance().getCore()->getCurrentLocation()){return shaukatCriterion(rd, loc);}
	static bool visibleCrescent(int rd, const QString &loc){return visibleCrescent(rd, location(loc));}
	//! @return previous date (RD) of first crescent (CC:UE 14.104)
	static int phasisOnOrBefore(int rd, const StelLocation &loc=StelApp::getInstance().getCore()->getCurrentLocation());
	static int phasisOnOrBefore(int rd, const QString &loc){return phasisOnOrBefore(rd, location(loc));}
	//! @return next date of first crescent (CC:UE 14.105)
	static int phasisOnOrAfter(int rd, const StelLocation &loc=StelApp::getInstance().getCore()->getCurrentLocation());
	static int phasisOnOrAfter(int rd, const QString &loc){return phasisOnOrAfter(rd, location(loc));}

signals:
	void partsChanged(QVector<int> parts);
	void jdChanged(double jd);

protected:
	double JD;		//! date expressed as JD(UT), including day fraction (ready to interact with the main application)
	QVector<int> parts;	//! date expressed in the numerical parts of the calendar (usually the smallest part represents a day count)

	static constexpr double meanTropicalYear=365.242189; //!< (CC:UE 14.31)
	static constexpr double meanSiderealYear=365.25636;  //!< (CC:UE 14.32)
	static constexpr double meanSynodicMonth=29.530588861;  //!< (CC:UE 14.44)
	static constexpr bool morning=true;  //!< CC:UE 14.71
	static constexpr bool evening=false; //!< CC:UE 14.73
};

#endif

/*
 * Copyright (C) 2022 Georg Zotti
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

#ifndef NEWHINDUCALENDAR_HPP
#define NEWHINDUCALENDAR_HPP

#include "OldHinduLuniSolarCalendar.hpp"

//! @class NewHinduCalendar
//! Functions for the New Hindu calendars described in CC:UE chapter 20.
//! @author Georg Zotti
//! @ingroup calendars
//! The new Hindu calendar as given in CC.UE is an astronomical Lunisolar calendar with Solar and Lunar months.
//! The name of a lunar month depends on the solar month that begins during that lunar month.
//! A (Lunar) month is leap and takes the following month’s name when no solar month begins within it.
//! This also may lead to skipped Lunar months.
//! There are two schemes for counting months. In the "amanta" scheme used here months begin and end at New Moon.
//! The other scheme, "purnimanta", has a few peculiarities for counting the leap month, described in CC:UE.
//! In this lunisolar calendar, there are leap months and expunged months. The phase of the moon at sunrise governs day numbers.
//!
//! @note This class contains all relevant functions from CC:UE chapter 20. They are also scriptable.
//! The Calendar interfacing functions deal with New Hindu Solar dates.
//!
//! @class NewHinduLunarCalendar   provides the New   Hindu Lunar dates in the overridden interfacing methods from Calendar.
//! @class AstroHinduSolarCalendar provides the Astro Hindu Solar dates in the overridden interfacing methods from Calendar.
//! @class AstroHinduLunarCalendar provides the Astro Hindu Lunar dates in the overridden interfacing methods from Calendar.


class NewHinduCalendar : public OldHinduLuniSolarCalendar
{
	Q_OBJECT

public:
	NewHinduCalendar(double jd);

	virtual ~NewHinduCalendar() Q_DECL_OVERRIDE {}

public slots:
	virtual void retranslate() Q_DECL_OVERRIDE;

	//! Set a calendar date from the Julian day number
	virtual void setJD(double JD) Q_DECL_OVERRIDE;

	//! set date from a vector of calendar date elements sorted from the largest to the smallest.
	//! Year-Month[1...12]-leap[0|1]-Day[1...30]
	virtual void setDate(QVector<int> parts) Q_DECL_OVERRIDE;

	//! get a stringlist of calendar date elements for the New Hindu Solar calendar sorted from the largest to the smallest.
	//! {Year, Month, MonthName, Day, WeekDayName}
	virtual QStringList getDateStrings() const Q_DECL_OVERRIDE;

	//! get a formatted complete string for the date in the New Hindu Solar calendar
	virtual QString getFormattedDateString() const Q_DECL_OVERRIDE;

	// 20.1 Hindu Astronomy
	//! @return table value within [0...3438]/3438 for the sine of an angle (CC:UE 20.4)
	//! within 0..90 degrees specified as integral number of steps of 225'.
	static double hinduSineTable(const int entry);
	//! @return sine value within [0...1] for the sine of an angle theta, interpolated from the 25-entry sine table (CC:UE 20.5)
	static double hinduSine(const double theta);
	//! @return arcsine value within [0...1] for the sine of an angle theta (CC:UE 20.6)
	static double hinduArcsin(const double amp);

	//! @return a mean position at RD of an object with period (CC:UE 20.7)
	static double hinduMeanPosition(const double rd_ut, const double period);
	//! @return a true position at RD of an object with period, ... (CC:UE 20.11)
	static double hinduTruePosition(const double rd_ut, const double period, const double size, const double anomalistic, const double change);
	//! @return solar longitude at RD (CC:UE 20.12)
	static double hinduSolarLongitude(const double rd_ut);
	//! @return solar Zodiac sign at RD (CC:UE 20.13)
	static int hinduZodiac(const double rd_ut);
	//! @return lunar longitude at RD (CC:UE 20.14)
	static double hinduLunarLongitude(const double rd_ut);
	//! @return lunar phase at RD (CC:UE 20.15)
	static double hinduLunarPhase(const double rd_ut);
	//! @return lunar day at RD (CC:UE 20.16)
	static int hinduLunarDayFromMoment(const double rd_ut);
	//! @return RD of New Moon before RD (CC:UE 20.17)
	//! The value is determined by binary search which terminates as soon as the solar zodiacal sign has been determined.
	static double hinduNewMoonBefore(const double rd_ut);

	// 20.2 Calendars
	//! @return calendar year from RD (CC:UE 20.18)
	static int hinduCalendarYear(const double rd_ut);

	//! @return { year, month, day} (CC:UE 20.20)
	static QVector<int> hinduSolarFromFixed(int rd);
	//! @return RD date from a New Hindu Solar date (CC:UE 20.21)
	//! parts={ year, month, day}
	static int fixedFromHinduSolar(QVector<int> parts);

	//! @return { year, month, leapMonth, day, leapDay } (CC:UE 20.23)
	static QVector<int> hinduLunarFromFixed(int rd);
	//! @return RD date from a New Hindu Lunar date (CC:UE 20.24)
	//! parts={ year, month, leapMonth, day, leapDay }
	static int fixedFromHinduLunar(QVector<int> parts);

	// 20.3 Sunrise
	//! @return the ascensional difference (CC:UE 20.27)
	static double hinduAscensionalDifference(const int rd, const StelLocation &loc=StelApp::getInstance().getCore()->getCurrentLocation());
	static double hinduAscensionalDifference(const int rd, const QString &loc){return hinduAscensionalDifference(rd, location(loc));}
	//! @return tropical longitude (CC:UE 20.28)
	static double hinduTropicalLongitude(const double rd_ut);
	//! @return solar sidereal difference (CC:UE 20.29)
	static double hinduSolarSiderealDifference(const double rd_ut);
	//! @return daily motion (CC:UE 20.30)
	static double hinduDailyMotion(const double rd_ut);
	//! @return the rising sign (CC:UE 20.31)
	static double hinduRisingSign(const double rd_ut);
	//! @return the Hindu equation of time (CC:UE 20.32)
	static double hinduEquationOfTime(const double rd_ut);
	//! @return hindu time of sunrise (CC:UE 20.33)
	static double hinduSunrise(const int rd);

	// 20.4 Alternatives
	//! @return hindu time of sunset (CC:UE 20.34)
	static double hinduSunset(const int rd);
	//! @return hindu time  (CC:UE 20.35)
	static double hinduStandardFromSundial(const int rd_ut);
	//! Alternative Lunar calendar counted from full moon to full moon
	//! @return { year, month, leapMonth, day, leapDay } (CC:UE 20.36)
	static QVector<int> hinduFullMoonFromFixed(int rd);
	//! @return RD date from a New Hindu Lunar date counted from full to full moon (CC:UE 20.37)
	//! parts={ year, month, leapMonth, day, leapDay }
	static int fixedFromHinduFullMoon(QVector<int> parts);
	//! test for expunged month (CC:UE 20.38)
	static bool hinduExpunged(const int lYear, const int lMonth);
	//! Alternative sunrise formula (CC:UE 20.39)
	static double altHinduSunrise(const int rd);

	// 20.5 Astronomical Versions
	// @return sidereal solar longitude, only used in Hindu calendar. (CC:UE 14.40)
	static double siderealSolarLongitude(const double rd_ut);

	//! @return Lahiri value of ayanamsha (CC:UE 20.40)
	static double ayanamsha(const double rd_ut);
	//! @return start of sidereal count (CC:UE 20.41)
	static double siderealStart();
	//! @return astronomical definition of hindu sunset (CC:UE 20.42)
	static double astroHinduSunset(const int rd);
	//! @return sidereal zodiac sign (CC:UE 20.43)
	static int siderealZodiac(const double rd_ut);
	//! @return astronomically defined calendar year (CC:UE 20.44)
	static int astroHinduCalendarYear(const double rd_ut);
	//! @return astronomically defined date in the Solar calendar (CC:UE 20.45)
	//! result is { year, month, day}
	static QVector<int> astroHinduSolarFromFixed(const int rd);
	//! @return RD from astronomically defined date in the Solar calendar (CC:UE 20.46)
	//! @arg is { year, month, day}
	static int fixedFromAstroHinduSolar(const QVector<int>date);
	//! (CC:UE 20.47)
	static int astroLunarDayFromMoment(const double rd_ut);
	//! @return { year, month, leapMonth, day, leapDay } in an astronomically defined Lunar calendar (CC:UE 20.48)
	static QVector<int> astroHinduLunarFromFixed(const int rd);
	//! @return RD date from an astronomically defined New Hindu Lunar date (CC:UE 20.49)
	//! parts={ year, month, leapMonth, day, leapDay }
	static int fixedFromAstroHinduLunar(const QVector<int> parts);

	// 20.6 Holidays
	//! binary search for the moment when solar longitude reaches lng in the time between rdA and rdB (used in CC:UE 20.50)
	static double hinduSolarLongitudeInv(double lng, double rdA, double rdB);
	//! @return moment of entry into Zodiacal sign (CC:UE 20.50)
	static double hinduSolarLongitudeAtOrAfter(const double lambda, const double rd_ut);
	//! @return moment of zero longitude in Gregorian year gYear (CC:UE 20.51)
	static double meshaSamkranti(const int gYear);
	//! binary search for the moment when lunar phase reaches phi in the time between rdA and rdB (used in CC:UE 20.52)
	static double hinduLunarPhaseInv(double phase, double rdA, double rdB);
	//! @return moment of kth Lunar day (CC:UE 20.52)
	static double hinduLunarDayAtOrAfter(const double k, const double rd_ut);
	//! @return moment of Lunar new year in Gregorian year gYear (CC:UE 20.53)
	static double hinduLunarNewYear(const int gYear);
	//! @return comparison of two lunar dates (CC:UE 20.54)
	//! @arg date1 and date2 are {year, month, leapMonth, day, leapDay}
	static bool hinduLunarOnOrBefore(const QVector<int>date1, const QVector<int>date2);
	//! @return RD of actually occurring date { lYear, lMonth, lDay} (CC:UE 20.55)
	static int hinduDateOccur(const int lYear, const int lMonth, const int lDay);
	//! @return a QVector<int> of RDs of actually occurring dates in a Gregorian year (CC:UE 20.56)
	static QVector<int> hinduLunarHoliday(const int lMonth, const int lDay, const int gYear);
	//! @return a QVector<int> of RDs of actually occurring Diwali dates in a Gregorian year (CC:UE 20.57)
	static QVector<int> diwali(const int gYear);
	//! @return RD of when a tithi occurs (CC:UE 20.58)
	static int hinduTithiOccur(const int lMonth, const int tithi, const double rd_ut, const int lYear);
	//! @return a QVector<int> of RDs of actually occurring tithis in a Gregorian year (CC:UE 20.59)
	static QVector<int> hinduLunarEvent(const int lMonth, const int tithi, const double rd_ut, const int gYear);
	//! @return dates of shiva in Gregorian year (CC:UE 20.60)
	static QVector<int>  shiva(const int gYear);
	//! @return dates of rama in Gregorian year (CC:UE 20.61)
	static QVector<int>  rama(const int gYear);

	//! @return return nakshatra for the day (CC:UE 20.62)
	static int hinduLunarStation(const int rd);
	//! @return karana index (CC:UE 20.63)
	static int karana(const int n);
	//! @return karana [1...60] for day rd. According to Wikipedia (https://en.wikipedia.org/wiki/Hindu_calendar#Kara%E1%B9%87a),
	//! the karana at sunrise prevails for the day, but this has yet to be confirmed.
	static int karanaForDay(const int rd);
	//! @return yoga (CC:UE 20.64)
	static int yoga(const int rd);
	//! @return the sacred Wednesdays in a Gregorian year. (CC:UE 20.65)
	static QVector<int> sacredWednesdays(const int gYear);
	//! @return the sacred Wednesdays in a certain range of RDs. (CC:UE 20.66)
	static QVector<int> sacredWednesdaysInRange(const QVector<int> range);

protected:
	constexpr static const double hinduSiderealYear=365.+279457./1080000.;   //!<  (CC:UE 20.1)
	constexpr static const double hinduSiderealMonth=27.+4644439./14438334.; //!<  (CC:UE 20.2)
	constexpr static const double hinduSynodicMonth=29.+7087771./13358334.;  //!<  (CC:UE 20.3)
	constexpr static const int hinduEpoch=-1132959; // specified in-class const initializer in OldHinduSolarCalendar, but the number is OK.
	constexpr static const double hinduCreation=hinduEpoch-1955880000.*hinduSiderealYear; //!<  (CC:UE 20.8)
	constexpr static const double hinduAnomalisticYear=1577917828000./(4320000000.-387.); //!<  (CC:UE 20.9)
	constexpr static const double hinduAnomalisticMonth=1577917828./(57753336.-488199.);  //!<  (CC:UE 20.10)
	constexpr static const int hinduSolarEra = 3179; //!< Saka era (CC:UE 20.19)
	constexpr static const int hinduLunarEra = 3044; //!< Vikrama era (CC:UE 20.22)

	static const StelLocation ujjain;    //!< Sacred city in India to which we relate the calendar. (CC:UE 20.25)
	static const StelLocation ujjainUTC; //!< Sacred city in India to which we relate the calendar, with timezone set to UTC. (CC:UE 20.25)
	//! convention to one site to which we relate the calendar. (CC:UE 20.26)
	//! @todo make this configurable?
	static const StelLocation hinduLocation;
	static QMap<int, QString>lunarStations;
	static QMap<int, QString>yogas;
	static QMap<int, QString>karanas;
};

#endif

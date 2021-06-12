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

//! Abstract superclass for all calendars.
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
//! in various editions (1997-2018), referred to as CC. In particular CC.ME=2nd "Millennium edition", CC.UE=4th "Ultimate Edition"
//! It describes the Gregorian calendar with year zero, and the Julian without. In this plugin only, we adhere to this convention to finally have all possible variations of dates.
//! It does not use JD directly but a number called Rata Die (RD=JD-1721424.5), a day count starting at midnight of January 1st, 1 AD (Proleptic Gregorian).
//! with functions including "fixed" in their names. Our calendar subclasses can use Calendar::fixedFromJD(jd) / Calendar::jdFromFixed(rd).
class Calendar : public QObject
{
	Q_OBJECT

public:
	//! enum from CC.UE-ch1.12.
	typedef enum { sunday = 0, monday, tuesday, wednesday, thursday, friday, saturday } Day;
	typedef enum { spring = 0, summer = 90, autumn = 180, winter = 270} Season; // CC.UE 3.5

	Calendar(double jd):JD(jd) {}

	virtual ~Calendar() {}

public slots:
	//! Translate e.g. stringlists of part names
	virtual void retranslate() = 0;

	//! Set a calendar date from the Julian day number
	//! Subclasses set JD and compute the parts and possibly other data
	//! This triggers the partsChanged() signal
	virtual void setJD(double JD) = 0;

	//! Get Julian day number from a calendar date
	virtual double getJD() const { return JD;}

	//! set date from a vector of calendar date elements sorted from the largest to the smallest.
	//! This triggers the jdChanged() signal
	//! Note that this must not change the time of day! You must retrieve the time from the current JD before recomputing a new JD.
	virtual void setDate(QVector<int> parts) = 0;

	//! get a vector of calendar date elements sorted from the largest to the smallest.
	//! The order depends on the actual calendar
	virtual QVector<int> getDate() const { return parts; }


	//! get a stringlist of calendar date elements sorted from the largest to the smallest.
	//! The order depends on the actual calendar
	virtual QStringList getDateStrings() const = 0;

	//! get a formatted complete string for a date. The default implementation just concatenates all strings from getDateStrings() with a space in between.
	virtual QString getFormattedDateString() const;

public:
	constexpr static const double J2000=2451545.0;
	constexpr static const double jdEpoch=-1721424.5;
	constexpr static const double mjdEpoch=678576.0;
	constexpr static const int bogus=-1000000; // special value to indicate invalid result in some calendars.

	//! Interfacing function from Reingold/Dershowitz: Calendrical Calculations
	//! Returns a "moment" in RD that represents JD.
	//! Stellarium extension: optionally includes local time zone offset.
	//! In most calendar interfacing functions, respectUTCoffset should be true.
	static double momentFromJD(double jd, bool respectUTCoffset=true);
	inline static int fixedFromMoment(double rd) { return std::lround(std::floor(rd));}
	inline static double timeFromMoment(double rd) { return StelUtils::fmodpos(rd, 1.);}
	//! Interfacing function from Reingold/Dershowitz: Calendrical Calculations
	//! Returns a fixed date in RD that represents noon of JD.
	//! Stellarium extension: optionally includes local time zone offset.
	//! In most calendar interfacing functions, respectUTCoffset should be true.
	inline static int fixedFromJD(double jd, bool respectUTCoffset=true) { return std::lround(std::floor(momentFromJD(jd, respectUTCoffset)));}
	inline static double momentFromMJD(double mjd) { return mjd+mjdEpoch;}

	//! interfacing function from Reingold/Dershowitz: Calendrical Calculations
	//! Returns a JD from an RD "moment" (including fractions of day)
	//! Stellarium extension: optionally includes local time zone offset.
	//! In most calendar interfacing functions, respectUTCoffset should be true.
	static double jdFromMoment(double rd, bool respectUTCoffset=true);
	//! interfacing function from Reingold/Dershowitz: Calendrical Calculations
	//! Returns a JD from an RD "moment" (including fractions of day)
	//! Stellarium extension: optionally includes local time zone offset.
	//! In most calendar interfacing functions, respectUTCoffset should be true.
	inline static double jdFromFixed(double rd, bool respectUTCoffset=true) { return jdFromMoment(rd, respectUTCoffset);}
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
signals:
	void partsChanged(QVector<int> parts);
	void jdChanged(double jd);

protected:
	double JD;		//! date expressed as JD(UT), including day fraction (ready to interact with the main application)
	QVector<int> parts;	//! date expressed in the numerical parts of the calendar (usually the smallest part represents a day count)
};

#endif

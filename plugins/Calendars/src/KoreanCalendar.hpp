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

#ifndef KOREANCALENDAR_HPP
#define KOREANCALENDAR_HPP

#include "ChineseCalendar.hpp"

//! @class KoreanCalendar
//! Functions for the Korean calendar (as derived from the Chinese calendar)
//! @author Georg Zotti
//! @ingroup calendars
//! The Korean calendar is derived from the Chinese calendar (like Japanese and Vietnamese).
//! The calendar's location for astronomical computations is Seoul.
//! Days begin at midnight. Lunar months begin on the day of New Moon.
//! In difference to the CC:UE book, we must introduce function names which replace "chinese" by "korean"
//! to take the changed location into account.
//!
//! Our implementation uses the 5-part QVector<int> {cycle, year, month, leap-month, day} inherited from the Chinese calendar.
//! From {cycle, year} the Korean year counted from 2333BCE can be found using koreanYear(cycle, year) which we show in the Calendars output.

class KoreanCalendar : public ChineseCalendar
{
	Q_OBJECT

public:
	KoreanCalendar(double jd);

	~KoreanCalendar() override {}

public slots:
	void retranslate() override;

	//! Set a calendar date from the Julian day number
	void setJD(double JD) override;

	//! set date from a vector of calendar date elements sorted from the largest to the smallest.
	//! {year, month, leap-month, day, leap-day}
	void setDate(QVector<int> parts5) override;

	//! get a stringlist of calendar date elements sorted from the largest to the smallest.
	//! {Cycle, Year, Month, MonthName, "leap"|"", Day, WeekDayName}
	//! The words "leap" (translated) are only given if the respective month is leap. Else an empty string is given.
	QStringList getDateStrings() const override;

	//! get a formatted complete string for a date
	QString getFormattedDateString() const override;

	//! get a pair of strings for the Solar Terms for a date
	QPair<QString,QString> getSolarTermStrings() const override;

	//! get a formatted string of the Solar Terms for a date
	QString getFormattedSolarTermsString() const override;


	//! find RD number for date in the Chinese calendar (CC:UE 19.17)
	//! @arg parts5={cycle, year, month, leap, day}
	static int fixedFromKorean(QVector<int> parts5);
	//! find date in the Chinese calendar from RD number (CC:UE 19.16)
	//! @return {cycle, year, month, leap, day}
	static QVector<int> koreanFromFixed(int rd);

	// Auxiliary functions

	static int currentMajorSolarTerm(int rd);
	//! Return location of Korean calendar computations (Seoul City Hall). Before April 1, 1908, this used LMST. CC:UE 19.36
	static StelLocation koreanLocation(double rd_t);

	//! Return the moment when solar longitude reaches lambda (CC:UE 19.3)
	//! @arg rd_t moment.
	//! @result valid for koreanLocation
	//! @note This would be called korean-solar-longitude-on-or-after in the book,
	//! but we use overwrites in the derived calendars, making the name without "korean" better.
	static double solarLongitudeOnOrAfter(double lambda, double rd_t);
	static int majorSolarTermOnOrAfter(int rd);

	//! Return current minor solar term for rd (CC:UE 19.5)
	static int currentMinorSolarTerm(int rd);

	//! Return minor solar term (CC:UE 19.6)
	static int minorSolarTermOnOrAfter(int rd);

	//! Return rd moment of midnight (CC:UE 19.7)
	//! This replaces midnight-in-china in the Chinese calendar functions used in Korea
	static double midnightInKorea(int rd);

	//! Return Korean Winter Solstice date (CC:UE 19.8)
	static int winterSolsticeOnOrBefore(int rd);

	//! Return Korean New Moon (CC:UE 19.9)
	static int newMoonOnOrAfter(int rd);

	//! Return Korean New Moon (CC:UE 19.10)
	static int newMoonBefore(int rd);

	//! Auxiliary function (CC:UE 19.11)
	static bool noMajorSolarTerm(int rd);

	//! Auxiliary function (CC:UE 19.12)
	static bool priorLeapMonth(int mP, int m);

	//! Return RD date of Chinese New Year in the Sui (year) of rd (CC:UE 19.13)
	static int newYearInSui(int rd);

	//! Return RD date of Chinese New Year in the Sui (year) of rd (CC:UE 19.14)
	static int newYearOnOrBefore(int rd);

//	//! Retrieve numerical components from the cycle number [1..60].  (following CC:UE 19.18)
//	static QPair<int, int> sexagesimalNumbers(int n);

	//! Retrieve name components from the cycle number [1..60].  (CC:UE 19.18)
	//! In contrast to chineseSexagesimalNumbers, this provides the pair (chinese double-name, translated double-name)
	static QPair<QString, QString> sexagesimalNames(int n);

//	//! Retrieve year difference between name pairs. [1..60].  (CC:UE 19.19)
//	static int chineseNameDifference(QPair<int,int>stemBranch1, QPair<int,int>stemBranch2);

	//! Retrieve pair of names (chinese stem_branch, translated stem_branch) for Chinese year year (CC:UE 19.20)
	static QPair<QString, QString> yearName(int year);

	//! Retrieve pair of names (chinese stem_branch, translated stem_branch) for Chinese month within a year (CC:UE 19.22)
	static QPair<QString, QString> monthName(int month, int year);

//	//! Retrieve one number (1...60) for Chinese day (after CC:UE 19.24)
//	static int DayNumber(int rd);
//	//! Retrieve pair of index numbers (stem, branch) for Chinese day (after CC:UE 19.24)
//	static QPair<int, int> DayNumbers(int rd);

	//! Retrieve pair of names (chinese stem_branch, translated stem_branch) for Chinese day (CC:UE 19.24)
	static QPair<QString, QString> dayName(int rd);

//	//! Retrieve RD of day number (1...60) on or before rd. (after CC:UE 19.25)
//	static int DayNumberOnOrBefore(QPair<int,int>stemBranch, int rd);

	//! Return Korean year number beginning in Winter of Gregorian year gYear (after CC:UE before 19.27, but with Korean year number)
	static int koreanNewYearInGregorianYear(int gYear);
//
//	//! Return Chinese year number beginning in Winter of Gregorian year gYear (CC:UE 19.27)
//	static int DragonFestivalInGregorianYear(int gYear);
//
//	//! Return RD of Winter minor term of Gregorian year gYear (CC:UE 19.28)
//	static int qingMing(int gYear);
//
	//! Return age of someone born on birthdate on date rd as expressed by the Korean calendar.
	//! This is a mix of Chinese Age and Western calendar, as Koreans consider January 1
	//! as turn of the Year in this calculation (https://www.90daykorean.com/korean-age-all-about-age-in-korea/)
	//! Therefore: A new-born is aged 1. Age increases at Gregorian New Year.
	//! Returns 0 for dates before birth. According to various sources, the time in the womb is countedc as first year,
	//! so it's unclear whether this is strictly correct.
	//! A use in scripting would probably be:
	//! @code
	//! core.output(KoreanCalendar.koreanAge(KoreanCalendar.koreanFromFixed(GregorianCalendar.fixedFromGregorian([1975, 3, 14])),
	//!                                      GregorianCalendar.fixedFromGregorian([2022, 12, 10]));
	//! @endcode
	static int koreanAge(QVector<int>birthdate, int rd);

	//! Return Korean Year in the Danki system, counting from 2333BCE. (CC:UE 19.37)
	static int koreanYear(int cycle, int year);

//public:
//	static const int chineseEpoch; //! RD of Gregorian {-2636, february, 15}. CC:UE 19.15

protected:
	static QMap<int, QString> majorSolarTerms;
	static QMap<int, QString> minorSolarTerms;
	static QMap<int, QString> celestialStems;
	static QMap<int, QString> celestialStemsElements;
	static QMap<int, QString> terrestrialBranches;
	static QMap<int, QString> terrestrialBranchesAnimalTotems;

	constexpr static const int chineseMonthNameEpoch=57; //! CC:UE 19.21
	constexpr static const int chineseDayNameEpoch=45; //! CC:UE 19.23
	constexpr static const int doubleBright=3; //! CC:UE 19.30
	constexpr static const int bright=2; //! CC:UE 19.31
	constexpr static const int blind=1; //! CC:UE 19.32
	constexpr static const int widow=0; //! CC:UE 19.33
};

#endif

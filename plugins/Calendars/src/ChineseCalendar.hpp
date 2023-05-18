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

#ifndef CHINESECALENDAR_HPP
#define CHINESECALENDAR_HPP

#include "Calendar.hpp"

//! @class ChineseCalendar
//! Functions for the Chinese calendar (1645 Qing dynasty version)
//! @author Georg Zotti
//! @ingroup calendars
//! The Chinese calendar (and related Japanese, Korean and Vietnamese) is a Lunisolar calendar based on astronomical events.
//! The calendar's location for astronomical computations is Beijing.
//! Days begin at midnight. Lunar months begin on the day of New Moon.
//!
//! Our implementation uses a 5-part QVector<int> {cycle, year, month, leap-month, day}

class ChineseCalendar : public Calendar
{
	Q_OBJECT

public:
	ChineseCalendar(double jd);

	~ChineseCalendar() override {}

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
	virtual QPair<QString,QString> getSolarTermStrings() const;

	//! get a formatted string of the Solar Terms for a date
	virtual QString getFormattedSolarTermsString() const;


	//! find RD number for date in the Chinese calendar (CC:UE 19.17)
	//! @arg parts5={cycle, year, month, leap, day}
	static int fixedFromChinese(QVector<int> parts5);
	//! find date in the Chinese calendar from RD number (CC:UE 19.16)
	//! @return {cycle, year, month, leap, day}
	static QVector<int> chineseFromFixed(int rd);

	// Auxiliary functions

	static int currentMajorSolarTerm(int rd);
	//! Return location of Chinese calendar computations (Beijing). Before 1929, this used LMST. CC:UE 19.2
	static StelLocation chineseLocation(double rd_t);

	//! Return the moment when solar longitude reaches lambda (CC:UE 19.3)
	//! @arg rd_t moment.
	//! @result valid for chineseLocation
	//! @note This is called chinese-solar-longitude-on-or-after in the book,
	//! but we use overwrites in the derived calendars, making the name without "chinese" better.
	static double solarLongitudeOnOrAfter(double lambda, double rd_t);
	static int majorSolarTermOnOrAfter(int rd);

	//! Return current minor solar term for rd (CC:UE 19.5)
	static int currentMinorSolarTerm(int rd);

	//! Return minor solar term (CC:UE 19.6)
	static int minorSolarTermOnOrAfter(int rd);

	//! Return rd moment of midnight (CC:UE 19.7)
	static double midnightInChina(int rd);

	//! Return Chinese Winter Solstice date (CC:UE 19.8)
	//! @note This is called chinese-winter-solstice-on-or-before, but we override it in derived calendars
	static int winterSolsticeOnOrBefore(int rd);

	//! Return Chinese New Moon (CC:UE 19.9)
	//! @note This is called chinese-new-moon-on-or-after in the book,
	//! but we use overwrites in the derived calendars, making the name without "chinese" better.
	static int newMoonOnOrAfter(int rd);

	//! Return Chinese New Moon (CC:UE 19.10)
	//! @note This is called chinese-new-moon-before in the book,
	//! but we use overwrites in the derived calendars, making the name without "chinese" better.
	static int newMoonBefore(int rd);

	//! Auxiliary function (CC:UE 19.11)
	//! @note This is called chinese-no-major-solar-term, but we override this in the derived calendars.
	static bool noMajorSolarTerm(int rd);

	//! Auxiliary function (CC:UE 19.12)
	//! @note This is called chinese-prior-leap-month, but we override this in the derived calendars.
	static bool priorLeapMonth(int mP, int m);

	//! Return RD date of Chinese New Year in the Sui (year) of rd (CC:UE 19.13)
	//! @note This is called chinese-new-year-in-sui, but we override this in the derived calendars.
	static int newYearInSui(int rd);

	//! Return RD date of Chinese New Year in the Sui (year) of rd (CC:UE 19.14)
	//! @note This is called chinese-new-year-on-or-before, but we override this in the derived calendars.
	static int newYearOnOrBefore(int rd);

	//! Retrieve numerical components from the cycle number [1..60].  (following CC:UE 19.18)
	static QPair<int, int> sexagesimalNumbers(int n);

	//! Retrieve name components from the cycle number [1..60].  (CC:UE 19.18)
	//! In contrast to chineseSexagesimalNumbers, this provides the pair (chinese double-name, translated double-name)
	//! @note this is named chinese-sexagesimal-names in the CC:UE book, but we use this name also in derived classes
	static QPair<QString, QString> sexagesimalNames(int n);

	//! Retrieve year difference between name pairs. [1..60].  (CC:UE 19.19)
	static int chineseNameDifference(QPair<int,int>stemBranch1, QPair<int,int>stemBranch2);

	//! Retrieve pair of names (chinese stem_branch, translated stem_branch) for Chinese year year (CC:UE 19.20)
	//! @note this is called chinese-year-name in the CC:UE book, but we overwrite this name in the derived classes.
	static QPair<QString, QString> yearName(int year);

	//! Retrieve pair of names (chinese stem_branch, translated stem_branch) for Chinese month within a year (CC:UE 19.22)
	//! @note this is called chinese-month-name in the CC:UE book, but we overwrite this name in the derived classes.
	static QPair<QString, QString> monthName(int month, int year);

	//! Retrieve one number (1...60) for Chinese day (after CC:UE 19.24)
	//! @note this should be called chinese-day-number following the CC:UE book strictly, but we use this name in favour of the derived classes.
	static int dayNumber(int rd);
	//! Retrieve pair of index numbers (stem, branch) for Chinese day (after CC:UE 19.24)
	//! @note this is called chinese-day-numbers in the CC:UE book, but we use this name in favour of the derived classes.
	static QPair<int, int> dayNumbers(int rd);

	//! Retrieve pair of names (chinese stem_branch, translated stem_branch) for Chinese day (CC:UE 19.24)
	//! @note this is called chinese-day-name in the CC:UE book, but we overwrite this name in the derived classes.
	static QPair<QString, QString> dayName(int rd);

	//! Retrieve RD of day number (1...60) on or before rd. (after CC:UE 19.25)
	//! @note this is called chinese-day-number-on-or-before in the CC:UE book, but we use this name in favour of the derived classes.
	static int dayNumberOnOrBefore(QPair<int,int>stemBranch, int rd);

	//! Return Chinese year number beginning in Winter of Gregorian year gYear (CC:UE before 19.27)
	static int ChineseNewYearInGregorianYear(int gYear);

	//! Return Chinese year number beginning in Winter of Gregorian year gYear (CC:UE 19.27)
	static int DragonFestivalInGregorianYear(int gYear);

	//! Return RD of Winter minor term of Gregorian year gYear (CC:UE 19.28)
	static int qingMing(int gYear);

	//! Return age of someone born on birthdate on date rd as expressed by Chinese (CC:UE 19.29)
	//! A new-born is aged 1. Age increases at Chinese New Year.
	//! Returns bogus on error
	static int chineseAge(QVector<int>birthdate, int rd);

	//! Determine marriage augury based on year number within a cycle. widows are worst, double-bright are best years.
	static int chineseYearMarriageAugury(int cycle, int year);

public:
	static const int chineseEpoch; //! RD of Gregorian {-2636, february, 15}. CC:UE 19.15

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

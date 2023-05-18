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

#ifndef JAPANESECALENDAR_HPP
#define JAPANESECALENDAR_HPP

#include "ChineseCalendar.hpp"

//! @class JapaneseCalendar
//! Functions for the Japanese calendar (as derived from the Chinese calendar)
//! @author Georg Zotti
//! @ingroup calendars
//! The Japanese calendar is derived from the Chinese calendar (like Korean and Vietnamese).
//! The calendar's location for astronomical computations is Tokyo.
//! Days begin at midnight. Lunar months begin on the day of New Moon.
//! In difference to the CC:UE book, we must introduce function names which replace "chinese" by "japanese",
//! to take the changed location into account or where the culture name is omitted, so that function overrides work.
//!
//! Our implementation uses the 5-part QVector<int> {cycle, year, month, leap-month, day} inherited from the Chinese calendar.
//! The years are given in Japanese eras.

class JapaneseCalendar : public ChineseCalendar
{
	Q_OBJECT

public:
	JapaneseCalendar(double jd);

	~JapaneseCalendar() override {}

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


	//! find RD number for date in the Japanese calendar (CC:UE 19.17)
	//! @arg parts5={cycle, year, month, leap, day}
	static int fixedFromJapanese(QVector<int> parts5);
	//! find date in the Japanese calendar from RD number (CC:UE 19.16)
	//! @return {cycle, year, month, leap, day}
	static QVector<int> japaneseFromFixed(int rd);

	// Auxiliary functions

	static int currentMajorSolarTerm(int rd);
	//! Return location of Japanese calendar computations (Tokyo). Before 1888, this used LMST. CC:UE 19.35
	static StelLocation japaneseLocation(double rd_t);

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
	//! This replaces midnight-in-china in the Chinese calendar functions used in Japan
	static double midnightInJapan(int rd);

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

	// We should even delete parent's inherited methods. But this does not work with Qt's MetaObject...
//	static QPair<int, int> sexagesimalNumbers(int n) = delete;
//	static QPair<QString, QString> sexagesimalNames(int n) = delete;
//	static int chineseNameDifference(QPair<int,int>stemBranch1, QPair<int,int>stemBranch2) = delete;
//	static QPair<QString, QString> yearName(int year) = delete;
//	static QPair<QString, QString> monthName(int month, int year) = delete;
//	static int DayNumber(int rd) = delete;
//	static QPair<int, int> DayNumbers(int rd) = delete;
//	static QPair<QString, QString> dayName(int rd) = delete;
//	static int DayNumberOnOrBefore(QPair<int,int>stemBranch, int rd) = delete;
//	static int ChineseNewYearInGregorianYear(int gYear) = delete;
//	static int DragonFestivalInGregorianYear(int gYear) = delete;
//	static int qingMing(int gYear) = delete;
//	static int chineseAge(QVector<int>birthdate, int rd) = delete;

	//! Return Japanese Year in the Tenno system, counting from XXX BCE. (CC:UE 19.37)
	static int japaneseYear(int cycle, int year);

public:
//	static const int chineseEpoch; //! RD of Gregorian {-2636, february, 15}. CC:UE 19.15

protected:
	static QMap<int, QString> majorSolarTerms;
	static QMap<int, QString> minorSolarTerms;
	//static QMap<int, QString> celestialStems;
	//static QMap<int, QString> celestialStemsElements;
	//static QMap<int, QString> terrestrialBranches;
	//static QMap<int, QString> terrestrialBranchesAnimalTotems;

//	constexpr static const int chineseMonthNameEpoch=57; //! CC:UE 19.21
//	constexpr static const int chineseDayNameEpoch=45; //! CC:UE 19.23
//	constexpr static const int doubleBright=3; //! CC:UE 19.30
//	constexpr static const int bright=2; //! CC:UE 19.31
//	constexpr static const int blind=1; //! CC:UE 19.32
//	constexpr static const int widow=0; //! CC:UE 19.33
};

#endif

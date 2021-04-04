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

#include "StelTranslator.hpp"
#include "HebrewCalendar.hpp"
#include "JulianCalendar.hpp"
#include "StelUtils.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"

const int HebrewCalendar::hebrewEpoch=JulianCalendar::fixedFromJulian({-3761, JulianCalendar::october, 7}); // RD -1373427

HebrewCalendar::HebrewCalendar(double jd): Calendar(jd)
{
	HebrewCalendar::retranslate();
}

QMap<int, QString> HebrewCalendar::weekDayNames;
QMap<int, QString> HebrewCalendar::monthNames;

void HebrewCalendar::retranslate()
{
	// fill the name lists with translated month and day names
	weekDayNames={
		{0, qc_("yom rishon (first day)"   , "Hebrew week day name")},
		{1, qc_("yom sheni (second day)"   , "Hebrew week day name")},
		{2, qc_("yom shelishi (third day)" , "Hebrew week day name")},
		{3, qc_("yom revi'i (fourth day)"  , "Hebrew week day name")},
		{4, qc_("yom ḥamishi (fifth day)"  , "Hebrew week day name")},
		{5, qc_("yom shishi (sixth day)"   , "Hebrew week day name")},
		{6, qc_("yom shabbat (sabbath day)", "Hebrew week day name")}};
	monthNames={
		{ 1, qc_("Nisan"     , "Hebrew month name")},
		{ 2, qc_("Iyyar"     , "Hebrew month name")},
		{ 3, qc_("Sivan"     , "Hebrew month name")},
		{ 4, qc_("Tammuz"    , "Hebrew month name")},
		{ 5, qc_("Av"        , "Hebrew month name")},
		{ 6, qc_("Elul"      , "Hebrew month name")},
		{ 7, qc_("Tishri"    , "Hebrew month name")},
		{ 8, qc_("Marḥeshvan", "Hebrew month name")},
		{ 9, qc_("Kislev"    , "Hebrew month name")},
		{10, qc_("Tevet"     , "Hebrew month name")},
		{11, qc_("Shevat"    , "Hebrew month name")},
		{12, qc_("Adar"      , "Hebrew month name")}};
}

// Set a calendar date from the Julian day number
void HebrewCalendar::setJD(double JD)
{
	this->JD=JD;

	int rd=fixedFromJD(JD, true);
	parts=hebrewFromFixed(rd);

	emit partsChanged(parts);
}

// get a stringlist of calendar date elements sorted from the largest to the smallest.
// Year, Month, MonthName, Day, DayName
// Again, in this plugin only, note no year zero, and AD/BC counting.
QStringList HebrewCalendar::getDateStrings() const
{	
	const int rd=fixedFromHebrew(parts);
	const int dow=dayOfWeekFromFixed(rd);

	QStringList list;
	list << QString::number(parts.at(0));            // 0:year
	list << QString::number(parts.at(1));            // 1:month
	QString monthName=monthNames.value(qMin(12, parts.at(1)), "error");
	if (isLeap(parts.at(0)))
	{
		static const QMap<int, QString>adarMap={{12, " I"}, {13, " II"}};
		monthName.append(adarMap.value(parts.at(1), ""));
	}
	list << monthName;                               // 2:monthName
	list << QString::number(parts.at(2));            // 3:day
	list << weekDayNames.value(dow);                 // 4:weekday

	return list;
}

// get a formatted complete string for a date
QString HebrewCalendar::getFormattedDateString() const
{
	QStringList str=getDateStrings();
	// TODO: Maybe use QDate with user's localisation here? Weekday has to be taken from our results, though.
	return QString("%1, %2 - %3 (%4) - %5")
			.arg(str.at(4)) // weekday
			.arg(str.at(3)) // day
			.arg(str.at(1)) // month, numerical
			.arg(str.at(2)) // month, name
			.arg(str.at(0));// year
}

// set date from a vector of calendar date elements sorted from the largest to the smallest.
// Year-Month[1...12]-Day[1...31]
// Time is not changed!
void HebrewCalendar::setDate(QVector<int> parts)
{
	//qDebug() << "JulianCalendar::setDate:" << parts;
	this->parts=parts;
	// For the Julian calendar, we really have no year 0 in this plugin.
	Q_ASSERT(parts.at(0) != 0);

	double rd=fixedFromHebrew(parts);
	// restore time from JD!
	double frac=StelUtils::fmodpos(JD+0.5+StelApp::getInstance().getCore()->getUTCOffset(JD)/24., 1.);
	JD=jdFromFixed(rd+frac, true);

	emit jdChanged(JD);
}

// returns true for leap years.
bool HebrewCalendar::isLeap(int hYear)
{
	return (StelUtils::imod(7*hYear+1, 19) < 7);
}

// return number of months in a hebrew year
HebrewCalendar::month HebrewCalendar::lastMonthOfHebrewYear(int hYear)
{
	return (isLeap(hYear) ? HebrewCalendar::adarii : HebrewCalendar::adar);
}

// return molad of a hebrew month
double HebrewCalendar::molad(int hYear, int hMonth)
{
	const int y = (hMonth<HebrewCalendar::tishri ? hYear+1 : hYear);
	const int monthsElapsed = hMonth-HebrewCalendar::tishri+StelUtils::intFloorDiv(235*y-234, 19);
	double ret=hebrewEpoch-876.0/25920.0;
	ret+=monthsElapsed*(29.5+793.0/25920.0);
	return ret;
}

int HebrewCalendar::hebrewCalendarElapsedDays(int hYear)
{
	const int monthsElapsed=StelUtils::intFloorDiv(235*hYear-234, 19);
	const int partsElapsed=12084+13753*monthsElapsed;
	const int days = 29*monthsElapsed+StelUtils::intFloorDiv(partsElapsed, 25920);
	if (StelUtils::imod(3*(days+1), 7)<3)
		return days+1;
	else
		return days;
}

int HebrewCalendar::hebrewYearLengthCorrection(int hYear)
{
	const int ny0=hebrewCalendarElapsedDays(hYear-1);
	const int ny1=hebrewCalendarElapsedDays(hYear);
	const int ny2=hebrewCalendarElapsedDays(hYear+1);
	if (ny2-ny1==356)
		return 2;
	else if (ny1-ny0==382)
		return 1;
	else
		return 0;
}

int HebrewCalendar::hebrewNewYear(int hYear)
{
	return hebrewEpoch+hebrewCalendarElapsedDays(hYear)+hebrewYearLengthCorrection(hYear);
}

int HebrewCalendar::daysInHebrewYear(int hYear)
{
	return hebrewNewYear(hYear+1)-hebrewNewYear(hYear);
}

bool HebrewCalendar::longMarheshvan(int hYear)
{
	return QVector<int>({355, 385}).contains(daysInHebrewYear(hYear));
}

bool HebrewCalendar::shortKislev(int hYear)
{
	return QVector<int>({353, 383}).contains(daysInHebrewYear(hYear));
}

// return number of days
int HebrewCalendar::lastDayOfHebrewMonth(int hYear, int hMonth)
{
	if (QVector<int>({iyyar, tammuz, elul, tevet, adarii}).contains(hMonth))
		return 29;
	if ((hMonth==HebrewCalendar::adar) && !isLeap(hYear))
		return 29;
	if ((hMonth==HebrewCalendar::marheshvan) && !longMarheshvan(hYear))
		return 29;
	if ((hMonth==HebrewCalendar::kislev) && shortKislev(hYear))
		return 29;
	return 30;
}

int HebrewCalendar::fixedFromHebrew(QVector<int> hebrew)
{
	const int year=hebrew.at(0);
	const int month=hebrew.at(1);
	const int day=hebrew.at(2);

	int ret=hebrewNewYear(year)+day-1;
	if (month<tishri)
	{
		for (int m=tishri; m<=lastMonthOfHebrewYear(year); m++)
			ret+=lastDayOfHebrewMonth(year, m);
		for (int m=nisan; m<month; m++)
			ret+=lastDayOfHebrewMonth(year, m);
	}
	else
	{
		for (int m=tishri; m<month; m++)
			ret+=lastDayOfHebrewMonth(year, m);
	}
	return ret;
}

QVector<int> HebrewCalendar::hebrewFromFixed(int rd)
{
	const int approx = StelUtils::intFloorDiv(98496*(rd-hebrewEpoch), 35975351)+1;

	int year=approx-1;
	while (hebrewNewYear(year+1)<=rd)
		year++;

	const int start= (rd<fixedFromHebrew({year, HebrewCalendar::nisan, 1}) ? HebrewCalendar::tishri : HebrewCalendar::nisan);

	int month=start;
	while (rd>fixedFromHebrew({year, month, lastDayOfHebrewMonth(month, year)}))
		month++;

	const int day=rd-fixedFromHebrew({year, month, 1})+1;
	return {year, month, day};
}

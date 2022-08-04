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

#include "StelTranslator.hpp"
#include "NewHinduLunarCalendar.hpp"
#include "StelUtils.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"


NewHinduLunarCalendar::NewHinduLunarCalendar(double jd): NewHinduCalendar(jd){}

// Set a calendar date from the Julian day number
void NewHinduLunarCalendar::setJD(double JD)
{
	this->JD=JD;

	int rd=fixedFromJD(JD, true);
	parts=hinduLunarFromFixed(rd);

	emit partsChanged(parts);
}

// get a stringlist of calendar date elements sorted from the largest to the smallest.
// {Year, Month, MonthName,  Day, DayName}
QStringList NewHinduLunarCalendar::getDateStrings() const
{
	const int rd=fixedFromHinduLunar(parts);
	const int dow=dayOfWeekFromFixed(rd);

	// parts={year, month, leap-month, day, leap-day}
	QStringList list;
	list << QString::number(parts.at(0));            // 0: year
	list << QString::number(parts.at(1));            // 1: month
	list << monthNames.value(parts.at(1), "error");  // 2: monthName
	list << (parts.at(2)==1 ? "1" : "0");            // 3: leapMonth? (1 only if leap)
	list << QString::number(parts.at(3));            // 4: day
	list << (parts.at(4)==1 ? "1" : "0");            // 5: leapDay? (1 only if leap)
	list << weekDayNames.value(dow);                 // 6: weekday

	return list;
}

// get a formatted complete string for a date
QString NewHinduLunarCalendar::getFormattedDateString() const
{
	QStringList str=getDateStrings();
	// TRANSLATORS: V.E. stands for Vikrama Era
	QString epoch = qc_("V.E.", "calendar epoch");
	QString leap = qc_("adhika", "Hindu leap month/day");

	// Format: [weekday], [day] - [month, numeral] ([month, name]) [leap] - [year] [epoch]
	return QString("%1, %2<sub>%3</sub> - %4 (%5)<sub>%6</sub> - %7 %8").arg(
				str.at(6),
				str.at(4),
				(str.at(5)=="1" ? leap : ""),
				str.at(1),
				str.at(2),
				(str.at(3)=="1" ? leap : ""),
				str.at(0),
				epoch);
}

// set date from a vector of calendar date elements sorted from the largest to the smallest.
// {Year, Month[1...12], leapMonth[0|1], Day[1...30], leapDay}
// Time is not changed!
void NewHinduLunarCalendar::setDate(QVector<int> parts)
{
	this->parts=parts;
	double rd=fixedFromHinduLunar(parts);
	// restore time from JD!
	double frac=StelUtils::fmodpos(JD+0.5+StelApp::getInstance().getCore()->getUTCOffset(JD)/24., 1.);
	JD=jdFromFixed(rd+frac, true);

	emit jdChanged(JD);
}

QString NewHinduLunarCalendar::getFormattedPanchangString()
{
	const int rd=fixedFromHinduLunar(parts);
	const int dow=dayOfWeekFromFixed(rd);

	QString tithi        = QString(qc_("Tithi",    "Hindu Calendar element"));
	QString nakshatraStr = QString(qc_("Nakṣatra", "Hindu Calendar element"));
	QString yogaStr      = QString(qc_("Yoga",     "Hindu Calendar element"));
	QString karanaStr    = QString(qc_("Karana",   "Hindu Calendar element"));
	return QString("%1: %2, %3: %4, %5: %6, %7: %8, %9: %10").arg(
				tithi       , QString::number(parts.value(3)),
				q_("Day")   , weekDayNames.value(dow, "error"),
				nakshatraStr, lunarStations.value(hinduLunarStation(rd)),
				yogaStr     , yogas.value(yoga(rd))).arg( // split args for Qt<5.14
				// Karanas are 1/2 lunar days. According to WP, the karana at sunrise governs the day.
				karanaStr   , karanas.value(karana(karanaForDay(rd)), QString::number(karanaForDay(rd)))
				);
}

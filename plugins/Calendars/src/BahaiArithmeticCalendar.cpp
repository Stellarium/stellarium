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
#include "BahaiArithmeticCalendar.hpp"
#include "GregorianCalendar.hpp"
#include "StelUtils.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"

BahaiArithmeticCalendar::BahaiArithmeticCalendar(double jd): Calendar (jd)
{
	BahaiArithmeticCalendar::retranslate();
}

const int BahaiArithmeticCalendar::bahaiEpoch=GregorianCalendar::fixedFromGregorian({1844, GregorianCalendar::march, 21});
QMap<int, QString> BahaiArithmeticCalendar::weekDayNames;
QMap<int, QString> BahaiArithmeticCalendar::cycleNames;
QMap<int, QString> BahaiArithmeticCalendar::yearNames;

void BahaiArithmeticCalendar::retranslate()
{
	// fill the name lists with translated month and day names
	weekDayNames={
		{0, qc_("Jalāl (Glory)"             , "Bahai day/month name")},  // Sat
		{1, qc_("Jamāl (Beauty)"            , "Bahai day/month name")},  // Sun
		{2, qc_("Kamāl (Perfection)"        , "Bahai day/month name")},  // Mon
		{3, qc_("Fiḍāl (Grace)"             , "Bahai day name")},  // Tue
		{4, qc_("‘Idāl (Justice)"           , "Bahai day name")},  // Wed
		{5, qc_("Istijlāl (Majesty)"        , "Bahai day name")},  // Thu
		{6, qc_("Istiqlāl (Independence)"   , "Bahai day name")}}; // Fri
	cycleNames={
		{ 0, qc_("Ayyām-i-Hā (Days of God)" , "Bahai day/month name")},
		{ 1, qc_("Bahā’ (Splendor)"         , "Bahai day/month and year-cycle (Vahid) name")},
		{ 2, qc_("Jalāl (Glory)"            , "Bahai day/month name")},
		{ 3, qc_("Jamāl (Beauty)"           , "Bahai day/month name")},
		{ 4, qc_("‘Aẓamat (Grandeur)"       , "Bahai day/month name")},
		{ 5, qc_("Nūr (Light)"              , "Bahai day/month name")},
		{ 6, qc_("Raḥmat (Mercy)"           , "Bahai day/month name")},
		{ 7, qc_("Kalimāt (Words)"          , "Bahai day/month name")},
		{ 8, qc_("Kamāl (Perfection)"       , "Bahai day/month name")},
		{ 9, qc_("Asmā’ (Names)"            , "Bahai day/month name")},
		{10, qc_("‘Izzat (Might)"           , "Bahai day/month name")},
		{11, qc_("Mashīyyat (Will)"         , "Bahai day/month name")},
		{12, qc_("‘Ilm (Knowledge)"         , "Bahai day/month name")},
		{13, qc_("Qudrat (Power)"           , "Bahai day/month name")},
		{14, qc_("Qawl (Speech)"            , "Bahai day/month name")},
		{15, qc_("Masā’il (Questions)"      , "Bahai day/month name")},
		{16, qc_("Sharaf (Honor)"           , "Bahai day/month name")},
		{17, qc_("Sulṭān (Sovereignty)"     , "Bahai day/month name")},
		{18, qc_("Mulk (Dominion)"          , "Bahai day/month name")},
		{19, qc_("‘Alā’ (Loftiness)"        , "Bahai day/month name")}};
	yearNames={
		{ 1, qc_("Alif (letter A)"          , "Bahai year-cycle (Vahid) name")},
		{ 2, qc_("Bā’ (letter B)"           , "Bahai year-cycle (Vahid) name")},
		{ 3, qc_("Ab (Father)"              , "Bahai year-cycle (Vahid) name")},
		{ 4, qc_("Dāl (letter D)"           , "Bahai year-cycle (Vahid) name")},
		{ 5, qc_("Bāb (Gate)"               , "Bahai year-cycle (Vahid) name")},
		{ 6, qc_("Vāv (letter V)"           , "Bahai year-cycle (Vahid) name")},
		{ 7, qc_("Abad (Eternity)"          , "Bahai year-cycle (Vahid) name")},
		{ 8, qc_("Jād (Generosity)"         , "Bahai year-cycle (Vahid) name")},
		{ 9, qc_("Bahā’ (Splendor)"         , "Bahai day/month and year-cycle (Vahid) name")},
		{10, qc_("Ḥubb (Love)"              , "Bahai year-cycle (Vahid) name")},
		{11, qc_("Bahhāj (Delightful)"      , "Bahai year-cycle (Vahid) name")},
		{12, qc_("Javāb (Answer)"           , "Bahai year-cycle (Vahid) name")},
		{13, qc_("Aḥad (Single)"            , "Bahai year-cycle (Vahid) name")},
		{14, qc_("Vahhāb (Bountiful)"       , "Bahai year-cycle (Vahid) name")},
		{15, qc_("Vidād (Affection)"        , "Bahai year-cycle (Vahid) name")},
		{16, qc_("Badī’ (Beginning)"        , "Bahai year-cycle (Vahid) name")},
		{17, qc_("Bahī (Luminous)"          , "Bahai year-cycle (Vahid) name")},
		{18, qc_("Abhā (Most Luminous)"     , "Bahai year-cycle (Vahid) name")},
		{19, qc_("Vāḥid (Unity)"            , "Bahai year-cycle (Vahid) name")}};
}


// Set a calendar date from the Julian day number
void BahaiArithmeticCalendar::setJD(double JD)
{
	this->JD=JD;

	int rd=fixedFromJD(JD, true);
	parts=bahaiArithmeticFromFixed(rd);
	emit partsChanged(parts);
}

// set date from a vector of calendar date elements sorted from the largest to the smallest.
// Major-Cycle-Year-Month[0...19]-Day[1...19]
void BahaiArithmeticCalendar::setDate(QVector<int> parts)
{
	//qDebug() << "BahaiArithmeticCalendar::setDate:" << parts;
	this->parts=parts;

	double rd=fixedFromBahaiArithmetic(parts);
	// restore time from JD!
	double frac=StelUtils::fmodpos(JD+0.5+StelApp::getInstance().getCore()->getUTCOffset(JD)/24., 1.);
	JD=jdFromFixed(rd+frac, true);

	emit jdChanged(JD);
}

// get a stringlist of calendar date elements sorted from the largest to the smallest.
// Major, Cycle, Year, YearNameInCycle, Month, MonthName, Day, DayName, WeekdayName
QStringList BahaiArithmeticCalendar::getDateStrings() const
{
//	const int dow=(int)floor(JD+2.5) % 7;

	const int rd=fixedFromBahaiArithmetic(parts);
	const int dow=dayOfWeekFromFixed(rd+1);

	QStringList list;
	list << QString::number(parts.at(0));    // 0 major
	list << QString::number(parts.at(1));    // 1 cycle
	list << QString::number(parts.at(2));    // 2 year
	list << yearNames.value(parts.at(2));    // 3 yearName
	list << QString::number(parts.at(3));    // 4 month
	list << cycleNames.value(parts.at(3));   // 5 monthName
	list << QString::number(parts.at(4));    // 6 day
	list << cycleNames.value(parts.at(4));   // 7 dayName
	list << weekDayNames.value(dow);         // 8 weekdayName

	return list;
}

// get a formatted complete string for a date
QString BahaiArithmeticCalendar::getFormattedDateString() const
{
	QStringList str=getDateStrings();
	// TRANSLATORS: Bahai Calendar output string
	return QString(qc_("%1, day of %2 (%3) of the month of %4 (%5), <br/>of the year %6 (%7), of Vāḥid %8, of Kull-i-shay %9 of the Bahá’í Era", "Bahai calendar output")).arg(
			str.at(8), // %1 weekday
			str.at(7), // %2 dayName
			str.at(6), // %3 dayNr
			str.at(5), // %4 monthName
			str.at(4), // %5 monthNr
			str.at(3), // %6 yearName
			str.at(2), // %7 yearNrInCycle
			str.at(1), // %8 Vahid (cycle)
			str.at(0)); // %9 major (Kull-i-shay)
}


/* ====================================================
 * Functions from CC.UE ch16.2
 */

int BahaiArithmeticCalendar::fixedFromBahaiArithmetic(QVector<int> bahai5)
{
	const int major=bahai5.value(0);
	const int cycle=bahai5.value(1);
	const int year =bahai5.value(2);
	const int month=bahai5.value(3);
	const int day  =bahai5.value(4);
	const int gYear=361*(major-1)+19*(cycle-1)+year-1+GregorianCalendar::gregorianYearFromFixed(bahaiEpoch);

	int rd=GregorianCalendar::fixedFromGregorian({gYear, GregorianCalendar::march, 20});
	if (month==ayyam_i_Ha)
		rd+=342;
	else if (month==19)
		rd+= (GregorianCalendar::isLeap(gYear+1)) ? 347 : 346;
	else
		rd+= 19*(month-1);
	rd+=day;

	return rd;
}

QVector<int> BahaiArithmeticCalendar::bahaiArithmeticFromFixed(int rd)
{
	static const int start = GregorianCalendar::gregorianYearFromFixed(bahaiEpoch);
	const int gYear = GregorianCalendar::gregorianYearFromFixed(rd);
	int years = gYear-start;
	if (rd<=GregorianCalendar::fixedFromGregorian({gYear, GregorianCalendar::march, 20}))
		years -= 1;

	const int major = StelUtils::intFloorDiv(years, 361)+1;
	const int cycle = StelUtils::intFloorDiv(StelUtils::imod(years, 361), 19)+1;
	const int year = StelUtils::imod(years, 19) + 1;
	const int days = rd-fixedFromBahaiArithmetic({major, cycle, year, 1, 1});
	int month = StelUtils::intFloorDiv(days, 19)+1;
	if (rd>=fixedFromBahaiArithmetic({major, cycle, year, 19,1}))
		month = 19;
	else if (rd>=fixedFromBahaiArithmetic({major, cycle, year, ayyam_i_Ha,1}))
		month = ayyam_i_Ha;
	const int day = rd+1-fixedFromBahaiArithmetic({major, cycle, year, month, 1});
	return {major, cycle, year, month, day};
}

int  BahaiArithmeticCalendar::bahaiNewYear(int gYear)
{
	return GregorianCalendar::fixedFromGregorian({gYear, GregorianCalendar::march, 21});
}

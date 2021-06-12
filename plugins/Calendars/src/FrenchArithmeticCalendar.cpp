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
#include "FrenchArithmeticCalendar.hpp"
#include "GregorianCalendar.hpp"
#include "RomanCalendar.hpp"
#include "StelUtils.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"

FrenchArithmeticCalendar::FrenchArithmeticCalendar(double jd): Calendar(jd)
{
	FrenchArithmeticCalendar::retranslate();
}

const int FrenchArithmeticCalendar::frenchEpoch=GregorianCalendar::fixedFromGregorian({1792, GregorianCalendar::september, 22});
QMap<int, QString> FrenchArithmeticCalendar::dayNames;
QMap<int, QString> FrenchArithmeticCalendar::sansculottides;
QMap<int, QString> FrenchArithmeticCalendar::monthNames;

void FrenchArithmeticCalendar::retranslate()
{
	// fill the name lists with translated month and day names
	dayNames={
		{ 1, qc_("Primidi" , "French Revolution Calendar day name")},
		{ 2, qc_("Duodi"   , "French Revolution Calendar day name")},
		{ 3, qc_("Tridi"   , "French Revolution Calendar day name")},
		{ 4, qc_("Quartidi", "French Revolution Calendar day name")},
		{ 5, qc_("Quintidi", "French Revolution Calendar day name")},
		{ 6, qc_("Sextidi" , "French Revolution Calendar day name")},
		{ 7, qc_("Septidi" , "French Revolution Calendar day name")},
		{ 8, qc_("Octidi"  , "French Revolution Calendar day name")},
		{ 9, qc_("Nonidi"  , "French Revolution Calendar day name")},
		{10, qc_("Décadi"  , "French Revolution Calendar day name")}};
	sansculottides={
		{ 1, qc_("Fête de la Vertu (Virtue Day)"         , "French Revolution Calendar day name")},
		{ 2, qc_("Fête du Génie (Genius Day)"            , "French Revolution Calendar day name")},
		{ 3, qc_("Fête du Travail (Labour Day)"          , "French Revolution Calendar day name")},
		{ 4, qc_("Fête de l'Opinion (Opinion Day)"       , "French Revolution Calendar day name")},
		{ 5, qc_("Fête de la Récompense (Reward Day)"    , "French Revolution Calendar day name")},
		{ 6, qc_("Jour de la Révolution (Revolution Day)", "French Revolution Calendar day name")}};
	monthNames={
		{ 1, qc_("Vendémiaire (vintage)", "French Revolution Calendar month name")},
		{ 2, qc_("Brumaire (fog)"       , "French Revolution Calendar month name")},
		{ 3, qc_("Frimaire (sleet)"     , "French Revolution Calendar month name")},
		{ 4, qc_("Nivôse (snow)"        , "French Revolution Calendar month name")},
		{ 5, qc_("Pluviôse (rain)"      , "French Revolution Calendar month name")},
		{ 6, qc_("Ventôse (wind)"       , "French Revolution Calendar month name")},
		{ 7, qc_("Germinal (seed)"      , "French Revolution Calendar month name")},
		{ 8, qc_("Floréal (blossom)"    , "French Revolution Calendar month name")},
		{ 9, qc_("Prairial (pasture)"   , "French Revolution Calendar month name")},
		{10, qc_("Messidor (harvest)"   , "French Revolution Calendar month name")},
		{11, qc_("Thermidor (heat)"     , "French Revolution Calendar month name")},
		{12, qc_("Fructidor (fruit)"    , "French Revolution Calendar month name")},
		{13, qc_("Sansculottides"       , "French Revolution Calendar month name")}};
}

// Set a calendar date from the Julian day number
void FrenchArithmeticCalendar::setJD(double JD)
{
	this->JD=JD;

	int rd=fixedFromJD(JD, true);
	parts=frenchArithmeticFromFixed(rd);

	emit partsChanged(parts);
}

// get a 6-part stringlist of calendar date elements sorted from the largest to the smallest.
// Year, Month, MonthName, Day, Decade, DayName
QStringList FrenchArithmeticCalendar::getDateStrings() const
{
	QStringList list;
	list << QString::number(parts.at(0));              // 0
	list << QString::number(parts.at(1));              // 1
	list << monthNames.value(parts.at(1), "ERROR");    // 2
	list << QString::number(parts.at(2));              // 3
	int decade=(parts.at(2)-1)/10 + 1;
	int dInDecade=StelUtils::amod(parts.at(2), 10);

	if (parts.at(1)==13)
	{
		list << monthNames.value(13);
		list << sansculottides.value(parts.at(2), "ERROR_sansculottides"); // 5
	}
	else
	{
		list << QString("%1 %2").arg(qc_("Décade", "French Revolution Calendar 'week'")).arg(RomanCalendar::romanNumber(decade)); // 4
		list << dayNames.value(dInDecade, "ERROR_dayNames");               // 5
	}

	return list;
}

// get a formatted complete string for a date
QString FrenchArithmeticCalendar::getFormattedDateString() const
{
	QStringList str=getDateStrings();
	QString year=QString("%1 %2 %3").arg(qc_("an", "French Revolution Calendar: year"),
					     str.at(0),
					     qc_("de la République Française", "French Revolution Calendar: year"));
	if (parts.at(1)==13)
		return QString("%1, %2").arg(str.at(5), year);
	else
	{
		QMap<int, QString>decadeMap={{1, qc_("première" , "French Revolution Calendar 'week'")},
					     {2, qc_("seconde"  , "French Revolution Calendar 'week'")},
					     {3, qc_("troisième", "French Revolution Calendar 'week'")}};

		QString dela=qc_("de la", "French Revolution Calendar: of the");

		return QString("%1 %2 %3 %4 (%5) %6 %7, %8")
				.arg(str.at(5)) // weekday
				.arg(dela) // day
				.arg(decadeMap.value((parts.at(2)-1)/10 + 1)) // decadeNr
				.arg(qc_("décade", "French Revolution Calendar 'week'"))
				.arg(str.at(3))
				.arg(qc_("du", "French Revolution Calendar: 'of'"))
				.arg(monthNames.value(parts.at(1))) // monthname
				.arg(year);// year

	}
}

// set date from a vector of calendar date elements sorted from the largest to the smallest.
// Year-Month[1...13]-Day[1...30]
// Time is not changed!
void FrenchArithmeticCalendar::setDate(QVector<int> parts)
{
	//qDebug() << "FrenchArithmeticCalendar::setDate:" << parts;
	this->parts=parts;

	double rd=fixedFromFrenchArithmetic(parts);
	// restore time from JD!
	double frac=StelUtils::fmodpos(JD+0.5+StelApp::getInstance().getCore()->getUTCOffset(JD)/24., 1.);
	JD=jdFromFixed(rd+frac, true);

	emit jdChanged(JD);
}

// returns true for leap years. This is arithmetic-french-leap-year?(f-year) [17.8] in the CC.UE book.
bool FrenchArithmeticCalendar::isLeap(int fYear)
{
	return (StelUtils::imod(fYear, 4) == 0
		&& !QVector<int>({100, 200, 300}).contains(StelUtils::imod(fYear, 400))
		&& StelUtils::imod(fYear, 4000) !=0 );
}

int FrenchArithmeticCalendar::fixedFromFrenchArithmetic(QVector<int> french)
{
	const int year=french.at(0);
	const int month=french.at(1);
	const int day=french.at(2);

	return frenchEpoch-1+365*(year-1)
		+StelUtils::intFloorDiv(year-1, 4)
		-StelUtils::intFloorDiv(year-1, 100)
		+StelUtils::intFloorDiv(year-1, 400)
		-StelUtils::intFloorDiv(year-1, 4000)
		+30*(month-1)+day;
}

QVector<int> FrenchArithmeticCalendar::frenchArithmeticFromFixed(int rd)
{
	const int approx=StelUtils::intFloorDivLL(4000LL*(rd-frenchEpoch+2), 1460969LL) + 1;
	const int year=(rd<fixedFromFrenchArithmetic({approx, 1, 1}) ? approx-1 : approx);

	const int month=1+StelUtils::intFloorDiv(rd-fixedFromFrenchArithmetic({year, 1, 1}), 30);
	const int day=1+rd-fixedFromFrenchArithmetic({year, month, 1});
	return {year, month, day};
}

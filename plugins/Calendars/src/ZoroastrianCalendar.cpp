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
#include "ZoroastrianCalendar.hpp"
#include "JulianCalendar.hpp"
#include "StelUtils.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"

const int ZoroastrianCalendar::zoroastrianEpoch=JulianCalendar::fixedFromJulian({632, JulianCalendar::june, 16}); //! RD 230638.


ZoroastrianCalendar::ZoroastrianCalendar(double jd): EgyptianCalendar(jd)
{
	ZoroastrianCalendar::retranslate();
}

QMap<int, QString> ZoroastrianCalendar::monthNames;
QMap<int, QString> ZoroastrianCalendar::dayNames;
QMap<int, QString> ZoroastrianCalendar::epagomenaeNames;

void ZoroastrianCalendar::retranslate()
{
	// fill the name lists with translated month and day names
	monthNames={
		{ 1, qc_("Ferverdīn"     , "Zoroastrian month name")},
		{ 2, qc_("Ardebehesht"   , "Zoroastrian month name")},
		{ 3, qc_("Khordād"       , "Zoroastrian month name")},
		{ 4, qc_("Tīr"           , "Zoroastrian month name")},
		{ 5, qc_("Mordād"        , "Zoroastrian month name")},
		{ 6, qc_("Sharīr"        , "Zoroastrian month name")},
		{ 7, qc_("Mihr"          , "Zoroastrian month name")},
		{ 8, qc_("Abān"          , "Zoroastrian month name")},
		{ 9, qc_("Āder"          , "Zoroastrian month name")},
		{10, qc_("Deï"           , "Zoroastrian month name")},
		{11, qc_("Bahmen"        , "Zoroastrian month name")},
		{12, qc_("Asfendārmed"   , "Zoroastrian month name")},
		{13, qc_("(epagomenae)"  , "Zoroastrian month name")}};
	dayNames={
		{ 1, qc_("Hormuz"      , "Zoroastrian day name")},
		{ 2, qc_("Bahman"      , "Zoroastrian day name")},
		{ 3, qc_("Ordībehesht" , "Zoroastrian day name")},
		{ 4, qc_("Shahrīvar"   , "Zoroastrian day name")},
		{ 5, qc_("Esfandārmud" , "Zoroastrian day name")},
		{ 6, qc_("Xordād"      , "Zoroastrian day name")},
		{ 7, qc_("Mordād"      , "Zoroastrian day name")},
		{ 8, qc_("Diy be Āzar" , "Zoroastrian day name")},
		{ 9, qc_("Āzar"        , "Zoroastrian day name")},
		{10, qc_("Ābān"        , "Zoroastrian day name")},
		{11, qc_("Xor"         , "Zoroastrian day name")},
		{12, qc_("Māh"         , "Zoroastrian day name")},
		{13, qc_("Tīr"         , "Zoroastrian day name")},
		{14, qc_("Goosh"       , "Zoroastrian day name")},
		{15, qc_("Diy be Mehr" , "Zoroastrian day name")},
		{16, qc_("Mehr"        , "Zoroastrian day name")},
		{17, qc_("Sorūsh"      , "Zoroastrian day name")},
		{18, qc_("Rashn"       , "Zoroastrian day name")},
		{19, qc_("Farvardīn"   , "Zoroastrian day name")},
		{20, qc_("Bahrām"      , "Zoroastrian day name")},
		{21, qc_("Rām"         , "Zoroastrian day name")},
		{22, qc_("Bād"         , "Zoroastrian day name")},
		{23, qc_("Diy be Dīn"  , "Zoroastrian day name")},
		{24, qc_("Dīn"         , "Zoroastrian day name")},
		{25, qc_("Ard"         , "Zoroastrian day name")},
		{26, qc_("Ashtād"      , "Zoroastrian day name")},
		{27, qc_("Asmān"       , "Zoroastrian day name")},
		{28, qc_("Zāmyād"      , "Zoroastrian day name")},
		{29, qc_("Māresfand"   , "Zoroastrian day name")},
		{30, qc_("Anīrān"      , "Zoroastrian day name")}};
	epagomenaeNames={
		{ 1, qc_("Ahnad"       , "Zoroastrian day name")},
		{ 2, qc_("Ashnad"      , "Zoroastrian day name")},
		{ 3, qc_("Esfandārmud" , "Zoroastrian day name")},
		{ 4, qc_("Axshatar"    , "Zoroastrian day name")},
		{ 5, qc_("Behesht"     , "Zoroastrian day name")}};
}

// Set a calendar date from the Julian day number
void ZoroastrianCalendar::setJD(double JD)
{
	this->JD=JD;

	int rd=fixedFromJD(JD, true);
	parts=zoroastrianFromFixed(rd);

	emit partsChanged(parts);
}

// get a stringlist of calendar date elements sorted from the largest to the smallest.
// Year, Month, MonthName, Day, dayName
QStringList ZoroastrianCalendar::getDateStrings() const
{
	QStringList list;
	list << QString::number(parts.at(0));
	list << QString::number(parts.at(1));
	list << monthNames.value(parts.at(1));
	list << QString::number(parts.at(2));
	if (parts.at(1)==13)
		list << epagomenaeNames.value(parts.at(2));
	else
		list << dayNames.value(parts.at(2));

	return list;
}

// get a formatted complete string for a date
QString ZoroastrianCalendar::getFormattedDateString() const
{
	QStringList str=getDateStrings();
	return QString("%1 (%2) - %3 (%4) - %5 %6")
			.arg(str.at(4)) // dayName
			.arg(str.at(3)) // day
			.arg(str.at(2)) // monthName
			.arg(str.at(1)) // month
			.arg(str.at(0)) // year
			.arg(q_("Jezdegerd Era"));// year
}

// set date from a vector of calendar date elements sorted from the largest to the smallest.
// Year-Month[1...13]-Day[1...30]
// Time is not changed!
void ZoroastrianCalendar::setDate(QVector<int> parts)
{
	this->parts=parts;

	double rd=fixedFromZoroastrian(parts);
	// restore time from JD!
	double frac=StelUtils::fmodpos(JD+0.5+StelApp::getInstance().getCore()->getUTCOffset(JD)/24., 1.);
	JD=jdFromFixed(rd+frac, true);

	emit jdChanged(JD);
}

// actually implementation of alt-fixed-from-egyptian
int ZoroastrianCalendar::fixedFromZoroastrian(QVector<int> zoroastrian)
{
	return zoroastrianEpoch + fixedFromEgyptian(zoroastrian)-EgyptianCalendar::egyptianEpoch;
}

QVector<int> ZoroastrianCalendar::zoroastrianFromFixed(int rd)
{
	return EgyptianCalendar::egyptianFromFixed(rd+EgyptianCalendar::egyptianEpoch-zoroastrianEpoch);
}

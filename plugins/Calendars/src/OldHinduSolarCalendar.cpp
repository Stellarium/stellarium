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
#include "OldHinduSolarCalendar.hpp"
#include "JulianCalendar.hpp"
#include "StelUtils.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"


const int OldHinduSolarCalendar::hinduEpoch=JulianCalendar::fixedFromJulian({-3102, JulianCalendar::february, 18}); // RD -1132959.

OldHinduSolarCalendar::OldHinduSolarCalendar(double jd): Calendar(jd), weekdayStyle(0), monthStyle(0)
{
	OldHinduSolarCalendar::retranslate();
}

QMap<int, QString> OldHinduSolarCalendar::weekDayNames;
QMap<int, QString> OldHinduSolarCalendar::monthNames;
QMap<int, QString> OldHinduSolarCalendar::jovianNames;

void OldHinduSolarCalendar::retranslate()
{
	// fill the name lists with translated month and day names.
	// We use 2 versions with offset of 10. The distinction is undocumented :-(
	weekDayNames={
		{ 0, qc_("Ravivāra"     , "old Hindu day name")},
		{ 1, qc_("Somavāra"     , "old Hindu day name")},
		{ 2, qc_("Mangalavāra"  , "old Hindu day name")},
		{ 3, qc_("Budhavāra"    , "old Hindu day name")},
		{ 4, qc_("Brihaspatvāra", "old Hindu day name")},
		{ 5, qc_("Śukravāra"    , "old Hindu day name")},
		{ 6, qc_("Śanivāra"     , "old Hindu day name")},
		{10, qc_("Ādityavāra"   , "old Hindu day name")},
		{11, qc_("Candravāra"   , "old Hindu day name")},
		{12, qc_("Bhaumavāra"   , "old Hindu day name")},
		{13, qc_("Saumyavāra"   , "old Hindu day name")},
		{14, qc_("Guruvāra"     , "old Hindu day name")},
		{15, qc_("Śukravāra"    , "old Hindu day name")},
		{16, qc_("Śanivāra"     , "old Hindu day name")},
	};
	// Again, several versions. The first set is Vedic, the second Sanskrit. Offset 100. A third (offset 200) gives Zodiacal signs.
	monthNames={
		{  1, qc_("Madhu"      , "old Hindu month name")},
		{  2, qc_("Mādhava"    , "old Hindu month name")},
		{  3, qc_("Śukra"      , "old Hindu month name")},
		{  4, qc_("Śuchi"      , "old Hindu month name")},
		{  5, qc_("Nabhas"     , "old Hindu month name")},
		{  6, qc_("Nabhasya"   , "old Hindu month name")},
		{  7, qc_("Issa"       , "old Hindu month name")},
		{  8, qc_("Ūrja"       , "old Hindu month name")},
		{  9, qc_("Sahas"      , "old Hindu month name")},
		{ 10, qc_("Sahasya"    , "old Hindu month name")},
		{ 11, qc_("Tapas"      , "old Hindu month name")},
		{ 12, qc_("Tapasya"    , "old Hindu month name")},
		{101, qc_("Meṣa"       , "old Hindu month name")},
		{102, qc_("Vṛṣabha"    , "old Hindu month name")},
		{103, qc_("Mithuna"    , "old Hindu month name")},
		{104, qc_("Karka"      , "old Hindu month name")},
		{105, qc_("Siṃha"      , "old Hindu month name")},
		{106, qc_("Kanyā"      , "old Hindu month name")},
		{107, qc_("Tulā"       , "old Hindu month name")},
		{108, qc_("Vṛścika"    , "old Hindu month name")},
		{109, qc_("Dhanus"     , "old Hindu month name")},
		{110, qc_("Makara"     , "old Hindu month name")},
		{111, qc_("Kumbha"     , "old Hindu month name")},
		{112, qc_("Mīna"       , "old Hindu month name")},
		{201, qc_("Aries"      , "Zodiacal sign")},
		{202, qc_("Taurus"     , "Zodiacal sign")},
		{203, qc_("Gemini"     , "Zodiacal sign")},
		{204, qc_("Cancer"     , "Zodiacal sign")},
		{205, qc_("Leo"        , "Zodiacal sign")},
		{206, qc_("Virgo"      , "Zodiacal sign")},
		{207, qc_("Libra"      , "Zodiacal sign")},
		{208, qc_("Scorpio"    , "Zodiacal sign")},
		{209, qc_("Sagittarius", "Zodiacal sign")},
		{210, qc_("Capricorn"  , "Zodiacal sign")},
		{211, qc_("Aquarius"   , "Zodiacal sign")},
		{212, qc_("Pisces"     , "Zodiacal sign")}};

	jovianNames={
		{ 1, qc_("Prabhava"     , "Old Hindu Jovian year name")},
		{ 2, qc_("Vibhava"      , "Old Hindu Jovian year name")},
		{ 3, qc_("Śukla"        , "Old Hindu Jovian year name")},
		{ 4, qc_("Pramoda"      , "Old Hindu Jovian year name")},
		{ 5, qc_("Prajāpati"    , "Old Hindu Jovian year name")},
		{ 6, qc_("Aṅgiras"      , "Old Hindu Jovian year name")},
		{ 7, qc_("Śrīmukha"     , "Old Hindu Jovian year name")},
		{ 8, qc_("Bhāva"        , "Old Hindu Jovian year name")},
		{ 9, qc_("Yuvan"        , "Old Hindu Jovian year name")},
		{10, qc_("Dhātṛ"        , "Old Hindu Jovian year name")},
		{11, qc_("Iśvara"       , "Old Hindu Jovian year name")},
		{12, qc_("Bahudhānya"   , "Old Hindu Jovian year name")},
		{13, qc_("Pramāthin"    , "Old Hindu Jovian year name")},
		{14, qc_("Vikrama"      , "Old Hindu Jovian year name")},
		{15, qc_("Vṛṣa"         , "Old Hindu Jovian year name")},
		{16, qc_("Citrabhānu"   , "Old Hindu Jovian year name")},
		{17, qc_("Subhānu"      , "Old Hindu Jovian year name")},
		{18, qc_("Tāraṇa"       , "Old Hindu Jovian year name")},
		{19, qc_("Pārthiva"     , "Old Hindu Jovian year name")},
		{20, qc_("Vyaya"        , "Old Hindu Jovian year name")},
		{21, qc_("Sarvajit"     , "Old Hindu Jovian year name")},
		{22, qc_("Sarvadhārin"  , "Old Hindu Jovian year name")},
		{23, qc_("Rākṣasa"      , "Old Hindu Jovian year name")},
		{24, qc_("Vikṛta"       , "Old Hindu Jovian year name")},
		{25, qc_("Khara"        , "Old Hindu Jovian year name")},
		{26, qc_("Nandana"      , "Old Hindu Jovian year name")},
		{27, qc_("Vijaya"       , "Old Hindu Jovian year name")},
		{28, qc_("Jaya"         , "Old Hindu Jovian year name")},
		{29, qc_("Manmatha"     , "Old Hindu Jovian year name")},
		{30, qc_("Durmukha"     , "Old Hindu Jovian year name")},
		{31, qc_("Hemalamba"    , "Old Hindu Jovian year name")},
		{32, qc_("Vilamba"      , "Old Hindu Jovian year name")},
		{33, qc_("Vikārin"      , "Old Hindu Jovian year name")},
		{34, qc_("Śarvari"      , "Old Hindu Jovian year name")},
		{35, qc_("Plava"        , "Old Hindu Jovian year name")},
		{36, qc_("Śubhakṛt"     , "Old Hindu Jovian year name")},
		{37, qc_("Śobhana"      , "Old Hindu Jovian year name")},
		{38, qc_("Krodhin"      , "Old Hindu Jovian year name")},
		{39, qc_("Viśvāvasu"    , "Old Hindu Jovian year name")},
		{40, qc_("Parābhava"    , "Old Hindu Jovian year name")},
		{41, qc_("Plavaṅga"     , "Old Hindu Jovian year name")},
		{42, qc_("Kīlaka"       , "Old Hindu Jovian year name")},
		{43, qc_("Saumya"       , "Old Hindu Jovian year name")},
		{44, qc_("Sādhāraṇa"    , "Old Hindu Jovian year name")},
		{45, qc_("Virodhakṛt"   , "Old Hindu Jovian year name")},
		{46, qc_("Paridhāvin"   , "Old Hindu Jovian year name")},
		{47, qc_("Pramāthin"    , "Old Hindu Jovian year name")},
		{48, qc_("Ānanda"       , "Old Hindu Jovian year name")},
		{49, qc_("Rākṣasa"      , "Old Hindu Jovian year name")},
		{50, qc_("Anala"        , "Old Hindu Jovian year name")},
		{51, qc_("Piṅgala"      , "Old Hindu Jovian year name")},
		{52, qc_("Kālayukta"    , "Old Hindu Jovian year name")},
		{53, qc_("Siddhārthin"  , "Old Hindu Jovian year name")},
		{54, qc_("Rāudra"       , "Old Hindu Jovian year name")},
		{55, qc_("Durmati"      , "Old Hindu Jovian year name")},
		{56, qc_("Dundubhi"     , "Old Hindu Jovian year name")},
		{57, qc_("Rudhirodgārin", "Old Hindu Jovian year name")},
		{58, qc_("Raktākṣa"     , "Old Hindu Jovian year name")},
		{59, qc_("Krodhana"     , "Old Hindu Jovian year name")},
		{60, qc_("Kṣaya"        , "Old Hindu Jovian year name")}};
}

// Set a calendar date from the Julian day number
void OldHinduSolarCalendar::setJD(double JD)
{
	this->JD=JD;

	int rd=fixedFromJD(JD, true);
	parts=oldHinduSolarFromFixed(rd);

	emit partsChanged(parts);
}

// get a stringlist of calendar date elements sorted from the largest to the smallest.
// {Year, JovianCycleNr, JovianCycleName, Month, MonthName, Day, DayName}
// Again, in this plugin only, note no year zero, and AD/BC counting.
QStringList OldHinduSolarCalendar::getDateStrings() const
{
	const int rd=fixedFromOldHinduSolar(parts);
	const int jovian=jovianYear(rd);
	const int dow=dayOfWeekFromFixed(rd);

	QStringList list;
	list << QString::number(parts.at(0));        // 0:year
	list << QString::number(jovian);             // 1:Jovian Cycle nr
	list << jovianNames.value(jovian, "error");  // 2:Jovian Cycle name
	list << QString::number(parts.at(1));        // 3:Month nr
	list << monthNames.value(100*monthStyle+parts.at(1));// 4:Month name
	list << QString::number(parts.at(2));        // 5:Day nr in month
	list << weekDayNames.value(10*weekdayStyle+dow, "error"); // 6:weekday
	return list;
}

// get a formatted complete string for a date
QString OldHinduSolarCalendar::getFormattedDateString() const
{
	QStringList str=getDateStrings();
	// TRANSLATORS: Old Hindu calendar phrase like "[Weekday], [number] - [MonthNumber] ([MonthName]) - [year] (Jovian [nr]:[name])"
	return QString(q_("%1, %2 - %3 (%4) - %5 K.Y. (Jovian %6:%7)"))
			.arg(str.at(6)) // weekday
			.arg(str.at(5)) // day
			.arg(str.at(3)) // month, numerical
			.arg(str.at(4)) // month, name
			.arg(str.at(0)) // year
			.arg(str.at(1)) // Jovian Nr
			.arg(str.at(2));// Jovian name
}

// set date from a vector of calendar date elements sorted from the largest to the smallest.
// Year-Month[1...12]-Day[1...31]
// Time is not changed!
void OldHinduSolarCalendar::setDate(QVector<int> parts)
{
	this->parts=parts;
	double rd=fixedFromOldHinduSolar(parts);
	// restore time from JD!
	double frac=StelUtils::fmodpos(JD+0.5+StelApp::getInstance().getCore()->getUTCOffset(JD)/24., 1.);
	JD=jdFromFixed(rd+frac, true);

	emit jdChanged(JD);
}


int OldHinduSolarCalendar::fixedFromOldHinduSolar(QVector<int> parts)
{
	int year=parts.at(0);
	int month=parts.at(1);
	int day=parts.at(2);
	return std::lround(std::ceil(hinduEpoch+year*aryaSolarYear+(month-1)*aryaSolarMonth+day-1.25));
}


QVector<int> OldHinduSolarCalendar::oldHinduSolarFromFixed(int rd)
{
	double sun=hinduDayCount(rd)+0.25;
	int year=std::lround(std::floor(sun/aryaSolarYear));
	int month=StelUtils::imod(std::lround(std::floor(sun/aryaSolarMonth)) , 12)+1;
	int day=std::lround(std::floor(StelUtils::fmodpos(sun, aryaSolarMonth)))+1;
	return {year, month, day};
}

int OldHinduSolarCalendar::jovianYear(int rd)
{
	return StelUtils::amod(27+std::lround(std::floor(12*hinduDayCount(rd)/aryaJovianPeriod)), 60);
}

// valid arguments: 0|1 (real difference not documented in CC.UE!)
void OldHinduSolarCalendar::setWeekdayStyle(int style)
{
	weekdayStyle=qBound(0, style, 1);
}

// valid arguments: 0=Vedic or 1=Sanskrit or 2=Zodiacal
void OldHinduSolarCalendar::setMonthStyle(int style)
{
	monthStyle=qBound(0, style, 2);
}

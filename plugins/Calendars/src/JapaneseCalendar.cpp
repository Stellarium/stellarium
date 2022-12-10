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
#include "JapaneseCalendar.hpp"
#include "GregorianCalendar.hpp"
#include "StelUtils.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"

JapaneseCalendar::JapaneseCalendar(double jd): ChineseCalendar(jd)
{
	JapaneseCalendar::retranslate();
}

QMap<int, QString> JapaneseCalendar::majorSolarTerms;
QMap<int, QString> JapaneseCalendar::minorSolarTerms;
//QMap<int, QString> JapaneseCalendar::celestialStems={};
//QMap<int, QString> JapaneseCalendar::celestialStemsElements={};
//QMap<int, QString> JapaneseCalendar::terrestrialBranches={};
//QMap<int, QString> JapaneseCalendar::terrestrialBranchesAnimalTotems={};

void JapaneseCalendar::retranslate()
{
	//ChineseCalendar::retranslate();
	// fill the name lists with translated month and day names
	majorSolarTerms={
		{ 1, qc_("Usui (Rain Water)"        , "Japanese calendar major Solar term")},
		{ 2, qc_("Shunbun (Spring Equinox)" , "Japanese calendar major Solar term")},
		{ 3, qc_("Kokuu (Grain Rain)"       , "Japanese calendar major Solar term")},
		{ 4, qc_("Shōman (Grain Full)"      , "Japanese calendar major Solar term")},
		{ 5, qc_("Geshi (Summer Solstice)"  , "Japanese calendar major Solar term")},
		{ 6, qc_("Taisho (Great Heat)"      , "Japanese calendar major Solar term")},
		{ 7, qc_("Shosho (Limit of Heat)"   , "Japanese calendar major Solar term")},
		{ 8, qc_("Shūbun (Autumnal Equinox)", "Japanese calendar major Solar term")},
		{ 9, qc_("Sōkō (Descent of Frost)"  , "Japanese calendar major Solar term")},
		{10, qc_("Shōsetsu (Slight Snow)"   , "Japanese calendar major Solar term")},
		{11, qc_("Tōji (Winter Solstice)"   , "Japanese calendar major Solar term")},
		{12, qc_("Taikan (Great Cold)"      , "Japanese calendar major Solar term")}};
	minorSolarTerms={
		{ 1, qc_("Risshun (Beginning of Spring)", "Japanese calendar minor Solar term")},
		{ 2, qc_("Keichitsu (Waking of Insects)", "Japanese calendar minor Solar term")},
		{ 3, qc_("Seimei (Pure Brightness)"     , "Japanese calendar minor Solar term")},
		{ 4, qc_("Rikka (Beginning of Summer)"  , "Japanese calendar minor Solar term")},
		{ 5, qc_("Bōshu (Grain in Ear)"         , "Japanese calendar minor Solar term")},
		{ 6, qc_("Shōsho (Slight Heat)"         , "Japanese calendar minor Solar term")},
		{ 7, qc_("Risshū (Beginning of Autumn)" , "Japanese calendar minor Solar term")},
		{ 8, qc_("Hakuro (White Dew)"           , "Japanese calendar minor Solar term")},
		{ 9, qc_("Kanro (Cold Dew)"             , "Japanese calendar minor Solar term")},
		{10, qc_("Rittō (Beginning of Winter)"  , "Japanese calendar minor Solar term")},
		{11, qc_("Taisetsu (Great Snow)"        , "Japanese calendar minor Solar term")},
		{12, qc_("Shōkan (Slight Cold)"         , "Japanese calendar minor Solar term")}};
}

// Set a calendar date from the Julian day number
void JapaneseCalendar::setJD(double JD)
{
	this->JD=JD;

	int rd=fixedFromJD(JD, true);
	parts=japaneseFromFixed(rd);

	emit partsChanged(parts);
}

// get a stringlist of calendar date elements sorted from the largest to the smallest.
// Year, Month, MonthName, "leap"|"", Day
QStringList JapaneseCalendar::getDateStrings() const
{
	// parts={cycle, year, month, leap-month, day}

	QStringList list;
	list << QString::number(japaneseYear(parts.at(0), parts.at(1)));     // 0:Japanese year (kigen)
	list << QString::number(parts.at(0));                                // 1:cycle
	list << QString::number(parts.at(1));                                // 2:year
	//list << QString("%1 (%2)").arg(yearNames.first, yearNames.second);   // 3:yearStemBranch
	list << QString::number(parts.at(2));                                // 3:monthNr
	//list << QString("%1 (%2)").arg(monthNames.first, monthNames.second); // 5:monthStemBranch
	list << (parts.at(3)==1 ? qc_("leap", "calendar term like leap year or leap day") : ""); // 4: leap? (only if leap)
	list << QString::number(parts.at(4));                                // 5:dayNr
	//list << QString("%1 (%2)").arg(dayNames.first, dayNames.second);     // 8:dayStemBranch

	return list;
}

// get a formatted complete string for a date
QString JapaneseCalendar::getFormattedDateString() const
{
	QStringList str=getDateStrings();

	// Format: "Day [D] - Month [M]<sub>[leap]</sub> - Year [Y]: (kigen)")
	// TRANSLATORS: Japanese Calendar output string
	return QString(qc_("Day %1 - Month %2<sub>%3</sub> - year %4 (kigen)", "Japanese Calendar output")).arg(
				str.at(5), // 1 dayNr
				str.at(3), // 3 monthNum
				str.at(4), // 4 monthLeap
				str.at(0) // 6 yearNrTotal_kigen
				);
}

// get a pair of strings for the Solar Terms for a date
QPair<QString,QString> JapaneseCalendar::getSolarTermStrings() const
{
	const int rd=fixedFromJapanese(parts);
	return {majorSolarTerms.value(currentMajorSolarTerm(rd)),
		minorSolarTerms.value(currentMinorSolarTerm(rd))};
}

// get a formatted string of the Solar Terms for a date
QString JapaneseCalendar::getFormattedSolarTermsString() const
{
	const int rd=fixedFromJapanese(parts);
	return QString("Major: <strong>%1</strong> - Minor: %2").arg(
		majorSolarTerms.value(currentMajorSolarTerm(rd)),
		minorSolarTerms.value(currentMinorSolarTerm(rd)));
}

// set date from a vector of calendar date elements sorted from the largest to the smallest.
// {cycle, year, month, leap-month, day}
// Time is not changed!
void JapaneseCalendar::setDate(QVector<int> parts)
{
	this->parts=parts;

	double rd=fixedFromJapanese(parts);
	// restore time from JD!
	double frac=StelUtils::fmodpos(JD+0.5+StelApp::getInstance().getCore()->getUTCOffset(JD)/24., 1.);
	JD=jdFromFixed(rd+frac, true);

	emit jdChanged(JD);
}

//! @arg parts5={cycle, year, month, leapMonth, day}
int JapaneseCalendar::fixedFromJapanese(QVector<int> parts5)
{
	const int cycle = parts5.value(0);
	const int year  = parts5.value(1);
	const int month = parts5.value(2);
	const int leap  = parts5.value(3);
	const int day   = parts5.value(4);

	const int midYear=qRound(floor(ChineseCalendar::chineseEpoch+((cycle-1)*60+year-1+0.5)*meanTropicalYear));
	const int newYear=newYearOnOrBefore(midYear);
	const int p = newMoonOnOrAfter(newYear+(month-1)*29);
	const QVector<int>d = japaneseFromFixed(p);
	int priorNewMoon;
	if ((month==d.at(2)) && (leap==d.at(3)))
		priorNewMoon=p;
	else
		priorNewMoon=newMoonOnOrAfter(p+1);

	return priorNewMoon+day-1;
}

// find date in the Chinese calendar from RD number (CC:UE 19.16)
QVector<int> JapaneseCalendar::japaneseFromFixed(int rd)
{
	const int s1 = winterSolsticeOnOrBefore(rd);
	const int s2 = winterSolsticeOnOrBefore(s1+370);
	const int m12 = newMoonOnOrAfter(s1+1);
	const int nextM11 = newMoonBefore(s2+1);
	const int m = newMoonBefore(rd+1);
	const bool leapYear = qRound((nextM11-m12)/meanSynodicMonth) == 12;
	int month = qRound((m-m12)/meanSynodicMonth);
	if (leapYear && priorLeapMonth(m12, m))
		month--;
	month=StelUtils::amod(month, 12);
	const bool leapMonth=leapYear && noMajorSolarTerm(m) && !priorLeapMonth(m12, newMoonBefore(m));
	const int elapsedYears=qRound(floor(1.5-month/12.+(rd-ChineseCalendar::chineseEpoch)/meanTropicalYear));
	const int cycle = StelUtils::intFloorDiv(elapsedYears-1, 60)+1;
	const int year = StelUtils::amod(elapsedYears, 60);
	const int day = rd-m+1;
	return {cycle, year, month, leapMonth, day};
}


int JapaneseCalendar::currentMajorSolarTerm(int rd)
{
	double s=solarLongitude(universalFromStandard(rd, japaneseLocation(rd)));
	return StelUtils::amod(2+qRound(floor(s/30.)), 12);
}

// Return location of Korean calendar computations (Seoul City Hall). Before 1908, this used LMST. CC:UE 19.2
StelLocation JapaneseCalendar::japaneseLocation(double rd_t)
{	
	const int year=GregorianCalendar::gregorianYearFromFixed(qRound(floor(rd_t)));

	if (year<1888)
		return StelLocation("Tokyo", "Japan", "Eastern Asia", 139.+(46./60.), 35.7, 24, 800, "UTC+09:19", 5);
	else
		return StelLocation("Tokyo", "Japan", "Eastern Asia", 135., 35., 24, 6000, "Asia/Tokyo", 9);
}

// Return the moment when solar longitude reaches lambda (CC:UE 19.3)
double JapaneseCalendar::solarLongitudeOnOrAfter(double lambda, double rd_t)
{
	const double sun=solarLongitudeAfter(lambda, universalFromStandard(rd_t, japaneseLocation(rd_t)));
	return standardFromUniversal(sun, japaneseLocation(sun));
}

int JapaneseCalendar::majorSolarTermOnOrAfter(int rd)
{
	const double s=solarLongitude(midnightInJapan(rd));
	const int l = StelUtils::imod(30*qRound(ceil(s/30.)), 360);

	return solarLongitudeOnOrAfter(l, rd);
}

// Return current minor solar term for rd (CC:UE 19.5)
int JapaneseCalendar::currentMinorSolarTerm(int rd)
{
	const double s=solarLongitude(universalFromStandard(rd, japaneseLocation(rd)));
	return StelUtils::amod(3+qRound(floor((s-15.)/30.)), 12);
}

// Return minor solar term (CC:UE 19.6)
int JapaneseCalendar::minorSolarTermOnOrAfter(int rd)
{
	const double s=solarLongitude(midnightInJapan(rd));
	const int l = StelUtils::imod(30*qRound(ceil((s-15.)/30.))+15, 360);

	return solarLongitudeOnOrAfter(l, rd);
}

// Return rd moment of midnight (CC:UE 19.7)
double JapaneseCalendar::midnightInJapan(int rd)
{
	return universalFromStandard(rd, japaneseLocation(rd));
}

// Return Chinese Winter Solstice date (CC:UE 19.8)
int JapaneseCalendar::winterSolsticeOnOrBefore(int rd)
{
	const double approx=estimatePriorSolarLongitude(double(Calendar::winter), midnightInJapan(rd+1));

	int day=qRound(floor(approx))-2;
	double lng;
	do {
		day++;
		//lng=modInterval(solarLongitude(midnghtInJapan(day+1)), -180., 180.);
		lng=solarLongitude(midnightInJapan(day+1));
	} while (double(Calendar::winter) >= lng);
	return day;
}

// Return Japanese New Moon (CC:UE 19.9)
int JapaneseCalendar::newMoonOnOrAfter(int rd)
{
	const double t=Calendar::newMoonAtOrAfter(midnightInJapan(rd));
	return qRound(floor(standardFromUniversal(t, japaneseLocation(t))));
}

// Return Japanese New Moon (CC:UE 19.10)
int JapaneseCalendar::newMoonBefore(int rd)
{
	const double t=Calendar::newMoonBefore(midnightInJapan(rd));
	return qRound(floor(standardFromUniversal(t, japaneseLocation(t))));
}

// Auxiliary function (CC:UE 19.11)
bool JapaneseCalendar::noMajorSolarTerm(int rd)
{
	return currentMajorSolarTerm(rd) == currentMajorSolarTerm(newMoonOnOrAfter(rd+1));
}

// Auxiliary function (CC:UE 19.12)
bool JapaneseCalendar::priorLeapMonth(int mP, int m)
{
	return (m>=mP) && (noMajorSolarTerm(m) || priorLeapMonth(mP, newMoonBefore(m)));
}

// Return RD date of Japanese New Year in the Sui (year) of rd (CC:UE 19.13)
int JapaneseCalendar::newYearInSui(int rd)
{
	const int s1=winterSolsticeOnOrBefore(rd);
	const int s2=winterSolsticeOnOrBefore(s1+370);
	const int m12=newMoonOnOrAfter(s1+1);
	const int m13=newMoonOnOrAfter(m12+1);
	const int nextM11=newMoonBefore(s2+1);

	if ((qRound((nextM11-m12)/meanSynodicMonth)==12) &&
		( noMajorSolarTerm(m12) || noMajorSolarTerm(m13) ))
		return newMoonOnOrAfter(m13+1);
	else
		return m13;
}

int JapaneseCalendar::newYearOnOrBefore(int rd)
{
	const int newYear=newYearInSui(rd);
	if (rd>=newYear)
		return newYear;
	else
		return newYearInSui(rd-180);
}



// Return Japanese Year in the kigen system, counting from 660 BCE. (CC:UE section 19.9)
int JapaneseCalendar::japaneseYear(int cycle, int year)
{
	// The cycle/year is identical to the Chinese cycle/year!
	return (cycle-34)*60+year+3;
}




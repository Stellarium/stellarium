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
#include "KoreanCalendar.hpp"
#include "GregorianCalendar.hpp"
#include "StelUtils.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"

KoreanCalendar::KoreanCalendar(double jd): ChineseCalendar(jd)
{
	KoreanCalendar::retranslate();
}

QMap<int, QString> KoreanCalendar::majorSolarTerms;
QMap<int, QString> KoreanCalendar::minorSolarTerms;
QMap<int, QString> KoreanCalendar::celestialStems;
QMap<int, QString> KoreanCalendar::celestialStemsElements;
QMap<int, QString> KoreanCalendar::terrestrialBranches;
QMap<int, QString> KoreanCalendar::terrestrialBranchesAnimalTotems;

void KoreanCalendar::retranslate()
{
	//ChineseCalendar::retranslate();
	// fill the name lists with translated month and day names
	majorSolarTerms={
		{ 1, qc_("Woo-Soo (Rain Water)"         , "Korean calendar major Solar term")},
		{ 2, qc_("Chun-Bun (Spring Equinox)"    , "Korean calendar major Solar term")},
		{ 3, qc_("Gok-Woo (Grain Rain)"         , "Korean calendar major Solar term")},
		{ 4, qc_("So-Man (Grain Full)"          , "Korean calendar major Solar term")},
		{ 5, qc_("Ha-Ji (Summer Solstice)"      , "Korean calendar major Solar term")},
		{ 6, qc_("Dae-Suh (Great Heat)"         , "Korean calendar major Solar term")},
		{ 7, qc_("Chu-Suh (Limit of Heat)"      , "Korean calendar major Solar term")},
		{ 8, qc_("Chu-Bun (Autumnal Equinox)"   , "Korean calendar major Solar term")},
		{ 9, qc_("Sang-Kang (Descent of Frost)" , "Korean calendar major Solar term")},
		{10, qc_("So-Sul (Slight Snow)"         , "Korean calendar major Solar term")},
		{11, qc_("Dong-Ji (Winter Solstice)"    , "Korean calendar major Solar term")},
		{12, qc_("Dae-Han (Great Cold)"         , "Korean calendar major Solar term")}};
	minorSolarTerms={
		{ 1, qc_("Ip-Chun (Beginning of Spring)" , "Korean calendar minor Solar term")},
		{ 2, qc_("Kyung-Chip (Waking of Insects)", "Korean calendar minor Solar term")},
		{ 3, qc_("Chyng-Myung (Pure Brightness)" , "Korean calendar minor Solar term")},
		{ 4, qc_("Ip-Ha (Beginning of Summer)"   , "Korean calendar minor Solar term")},
		{ 5, qc_("Mang-Jong (Grain in Ear)"      , "Korean calendar minor Solar term")},
		{ 6, qc_("So-Suh (Slight Heat)"          , "Korean calendar minor Solar term")},
		{ 7, qc_("Ip-Choo (Beginning of Autumn)" , "Korean calendar minor Solar term")},
		{ 8, qc_("Bak-Roo (White Dew)"           , "Korean calendar minor Solar term")},
		{ 9, qc_("Han-Roo (Cold Dew)"            , "Korean calendar minor Solar term")},
		{10, qc_("Ip-Dong (Beginning of Winter)" , "Korean calendar minor Solar term")},
		{11, qc_("Dae-Sul (Great Snow)"          , "Korean calendar minor Solar term")},
		{12, qc_("So-Han (Slight Cold)"          , "Korean calendar minor Solar term")}};
	celestialStems={
		{ 1, qc_("Kap"  , "Korean calendar, celestial stem name")},
		{ 2, qc_("El"   , "Korean calendar, celestial stem name")},
		{ 3, qc_("Byung", "Korean calendar, celestial stem name")},
		{ 4, qc_("Jung" , "Korean calendar, celestial stem name")},
		{ 5, qc_("Mu"   , "Korean calendar, celestial stem name")},
		{ 6, qc_("Ki"   , "Korean calendar, celestial stem name")},
		{ 7, qc_("Kyung", "Korean calendar, celestial stem name")},
		{ 8, qc_("Shin" , "Korean calendar, celestial stem name")},
		{ 9, qc_("Im"   , "Korean calendar, celestial stem name")},
		{10, qc_("Gye"  , "Korean calendar, celestial stem name")}};
	// N.B. Do not change the translation hint. This is identical to the Chinese!
	celestialStemsElements={
		{ 1, qc_("Tree, male"   , "Chinese calendar, celestial stem element")},
		{ 2, qc_("Tree, female" , "Chinese calendar, celestial stem element")},
		{ 3, qc_("Fire, male"   , "Chinese calendar, celestial stem element")},
		{ 4, qc_("Fire, female" , "Chinese calendar, celestial stem element")},
		{ 5, qc_("Earth, male"  , "Chinese calendar, celestial stem element")},
		{ 6, qc_("Earth, female", "Chinese calendar, celestial stem element")},
		{ 7, qc_("Metal, male"  , "Chinese calendar, celestial stem element")},
		{ 8, qc_("Metal, female", "Chinese calendar, celestial stem element")},
		{ 9, qc_("Water, male"  , "Chinese calendar, celestial stem element")},
		{10, qc_("Water, female", "Chinese calendar, celestial stem element")}};
	terrestrialBranches={
		{ 1, qc_("Ja"     , "Korean calendar, terrestrial branch name")},
		{ 2, qc_("Chuk"   , "Korean calendar, terrestrial branch name")},
		{ 3, qc_("In"     , "Korean calendar, terrestrial branch name")},
		{ 4, qc_("Myo"    , "Korean calendar, terrestrial branch name")},
		{ 5, qc_("Jin"    , "Korean calendar, terrestrial branch name")},
		{ 6, qc_("Sa"     , "Korean calendar, terrestrial branch name")},
		{ 7, qc_("Oh"     , "Korean calendar, terrestrial branch name")},
		{ 8, qc_("Mi"     , "Korean calendar, terrestrial branch name")},
		{ 9, qc_("Shin"   , "Korean calendar, terrestrial branch name")},
		{10, qc_("Yoo"    , "Korean calendar, terrestrial branch name")},
		{11, qc_("Sool"   , "Korean calendar, terrestrial branch name")},
		{12, qc_("Hae"    , "Korean calendar, terrestrial branch name")}};
	// N.B. Do not change the translation hint. This is identical to the Chinese!
	terrestrialBranchesAnimalTotems={
		{ 1, qc_("Rat"     , "Chinese calendar, terrestrial branch animal totem")},
		{ 2, qc_("Ox"      , "Chinese calendar, terrestrial branch animal totem")},
		{ 3, qc_("Tiger"   , "Chinese calendar, terrestrial branch animal totem")},
		{ 4, qc_("Hare"    , "Chinese calendar, terrestrial branch animal totem")},
		{ 5, qc_("Dragon"  , "Chinese calendar, terrestrial branch animal totem")},
		{ 6, qc_("Snake"   , "Chinese calendar, terrestrial branch animal totem")},
		{ 7, qc_("Horse"   , "Chinese calendar, terrestrial branch animal totem")},
		{ 8, qc_("Sheep"   , "Chinese calendar, terrestrial branch animal totem")},
		{ 9, qc_("Monkey"  , "Chinese calendar, terrestrial branch animal totem")},
		{10, qc_("Fowl"    , "Chinese calendar, terrestrial branch animal totem")},
		{11, qc_("Dog"     , "Chinese calendar, terrestrial branch animal totem")},
		{12, qc_("Pig"     , "Chinese calendar, terrestrial branch animal totem")}};
}

// Set a calendar date from the Julian day number
void KoreanCalendar::setJD(double JD)
{
	this->JD=JD;

	int rd=fixedFromJD(JD, true);
	parts=koreanFromFixed(rd);

	emit partsChanged(parts);
}

// get a stringlist of calendar date elements sorted from the largest to the smallest.
// Year, Month, MonthName, "leap"|"", Day
QStringList KoreanCalendar::getDateStrings() const
{
	// parts={cycle, year, month, leap-month, day}
	const int rd=fixedFromKorean(parts);
	const QPair<QString,QString>yearNames=yearName(parts.at(1));
	const QPair<QString,QString>monthNames=monthName(parts.at(2), parts.at(1));
	const QPair<QString,QString>dayNames=dayName(rd);

	QStringList list;
	list << QString::number(koreanYear(parts.at(0), parts.at(1)));       // 0:Korean year
	list << QString::number(parts.at(0));                                // 1:cycle
	list << QString::number(parts.at(1));                                // 2:year
	list << QString("%1 (%2)").arg(yearNames.first, yearNames.second);   // 3:yearStemBranch
	list << QString::number(parts.at(2));                                // 4:monthNr
	list << QString("%1 (%2)").arg(monthNames.first, monthNames.second); // 5:monthStemBranch
	list << (parts.at(3)==1 ? qc_("leap", "calendar term like leap year or leap day") : ""); // 6: leap? (only if leap)
	list << QString::number(parts.at(4));                                // 7:day
	list << QString("%1 (%2)").arg(dayNames.first, dayNames.second);     // 8:dayStemBranch

	return list;
}

// get a formatted complete string for a date
QString KoreanCalendar::getFormattedDateString() const
{
	QStringList str=getDateStrings();
	// Format: "Day [D] ([stem/branch]) - Month [M]<sub>[leap]</sub> ([stem/branch]) - year [Y] ([stem/branch])")
	// TRANSLATORS: Korean Calendar output string
	return QString(qc_("Day %1 (%2) - Month %3<sub>%4</sub> (%5) <br/>Danki Year %6 (%7)", "Korean Calendar output")).arg(
				str.at(7), // 1 dayNr
				str.at(8), // 2 dayStemBranch
				str.at(4), // 3 monthNum
				str.at(6), // 4 monthLeap
				str.at(5), // 5 monthStemBranch
				str.at(0), // 6 yearNrTotal
				str.at(3)  // 7 yearStemBranch
				);
}

// get a pair of strings for the Solar Terms for a date
QPair<QString,QString> KoreanCalendar::getSolarTermStrings() const
{
	const int rd=fixedFromKorean(parts);
	return {majorSolarTerms.value(currentMajorSolarTerm(rd)),
		minorSolarTerms.value(currentMinorSolarTerm(rd))};
}

// get a formatted string of the Solar Terms for a date
QString KoreanCalendar::getFormattedSolarTermsString() const
{
	const int rd=fixedFromKorean(parts);
	return QString("%1: <strong>%3</strong> - %2: %4").arg(
		q_("Major"), q_("Minor"),
		majorSolarTerms.value(currentMajorSolarTerm(rd)),
		minorSolarTerms.value(currentMinorSolarTerm(rd)));
}

// set date from a vector of calendar date elements sorted from the largest to the smallest.
// {cycle, year, month, leap-month, day}
// Time is not changed!
void KoreanCalendar::setDate(QVector<int> parts)
{
	this->parts=parts;

	double rd=fixedFromKorean(parts);
	// restore time from JD!
	double frac=StelUtils::fmodpos(JD+0.5+StelApp::getInstance().getCore()->getUTCOffset(JD)/24., 1.);
	JD=jdFromFixed(rd+frac, true);

	emit jdChanged(JD);
}

//! @arg parts5={cycle, year, month, leapMonth, day}
int KoreanCalendar::fixedFromKorean(QVector<int> parts5)
{
	const int cycle = parts5.value(0);
	const int year  = parts5.value(1);
	const int month = parts5.value(2);
	const int leap  = parts5.value(3);
	const int day   = parts5.value(4);

	const int midYear=qRound(floor(ChineseCalendar::chineseEpoch+((cycle-1)*60+year-1+0.5)*meanTropicalYear));
	const int newYear=newYearOnOrBefore(midYear);
	const int p = newMoonOnOrAfter(newYear+(month-1)*29);
	const QVector<int>d = koreanFromFixed(p);
	int priorNewMoon;
	if ((month==d.at(2)) && (leap==d.at(3)))
		priorNewMoon=p;
	else
		priorNewMoon=newMoonOnOrAfter(p+1);

	return priorNewMoon+day-1;
}

// find date in the Chinese calendar from RD number (CC:UE 19.16)
QVector<int> KoreanCalendar::koreanFromFixed(int rd)
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


int KoreanCalendar::currentMajorSolarTerm(int rd)
{
	double s=solarLongitude(universalFromStandard(rd, koreanLocation(rd)));
	return StelUtils::amod(2+qRound(floor(s/30.)), 12);
}

// Return location of Korean calendar computations (Seoul City Hall). Before 1908, this used LMST. CC:UE 19.2
StelLocation KoreanCalendar::koreanLocation(double rd_t)
{	
	StelLocation loc("Seoul City Hall", "South Korea", "Eastern Asia", 126.+(58./60.), 37.+(34./60.), 0, 8000, "Asia/Seoul", 9);
	// The IANA ruleset for Asia/Seoul already includes all these details!
	if (rd_t<GregorianCalendar::fixedFromGregorian({1908, GregorianCalendar::april, 1}))
		loc.ianaTimeZone="UTC+08:28";
	else if ((rd_t<GregorianCalendar::fixedFromGregorian({1912, GregorianCalendar::january, 1})) ||
		 ((rd_t>=GregorianCalendar::fixedFromGregorian({1954, GregorianCalendar::march, 21})) &&
		  (rd_t<GregorianCalendar::fixedFromGregorian({1961, GregorianCalendar::august, 10}))))
		loc.ianaTimeZone="UTC+08:30";

	return loc;
}

// Return the moment when solar longitude reaches lambda (CC:UE 19.3)
double KoreanCalendar::solarLongitudeOnOrAfter(double lambda, double rd_t)
{
	const double sun=solarLongitudeAfter(lambda, universalFromStandard(rd_t, koreanLocation(rd_t)));
	return standardFromUniversal(sun, koreanLocation(sun));
}

int KoreanCalendar::majorSolarTermOnOrAfter(int rd)
{
	const double s=solarLongitude(midnightInKorea(rd));
	const int l = StelUtils::imod(30*qRound(ceil(s/30.)), 360);

	return solarLongitudeOnOrAfter(l, rd);
}

// Return current minor solar term for rd (CC:UE 19.5)
int KoreanCalendar::currentMinorSolarTerm(int rd)
{
	const double s=solarLongitude(universalFromStandard(rd, koreanLocation(rd)));
	return StelUtils::amod(3+qRound(floor((s-15.)/30.)), 12);
}

// Return minor solar term (CC:UE 19.6)
int KoreanCalendar::minorSolarTermOnOrAfter(int rd)
{
	const double s=solarLongitude(midnightInKorea(rd));
	const int l = StelUtils::imod(30*qRound(ceil((s-15.)/30.))+15, 360);

	return solarLongitudeOnOrAfter(l, rd);
}

// Return rd moment of midnight (CC:UE 19.7)
double KoreanCalendar::midnightInKorea(int rd)
{
	return universalFromStandard(rd, koreanLocation(rd));
}

// Return Chinese Winter Solstice date (CC:UE 19.8)
int KoreanCalendar::winterSolsticeOnOrBefore(int rd)
{
	const double approx=estimatePriorSolarLongitude(double(Calendar::winter), midnightInKorea(rd+1));

	int day=qRound(floor(approx))-2;
	double lng;
	do {
		day++;
		//lng=modInterval(solarLongitude(midnightInKorea(day+1)), -180., 180.);
		lng=solarLongitude(midnightInKorea(day+1));
	} while (double(Calendar::winter) >= lng);
	return day;
}

// Return Korean New Moon (CC:UE 19.9)
int KoreanCalendar::newMoonOnOrAfter(int rd)
{
	const double t=Calendar::newMoonAtOrAfter(midnightInKorea(rd));
	return qRound(floor(standardFromUniversal(t, koreanLocation(t))));
}

// Return Korean New Moon (CC:UE 19.10)
int KoreanCalendar::newMoonBefore(int rd)
{
	const double t=Calendar::newMoonBefore(midnightInKorea(rd));
	return qRound(floor(standardFromUniversal(t, koreanLocation(t))));
}

// Auxiliary function (CC:UE 19.11)
bool KoreanCalendar::noMajorSolarTerm(int rd)
{
	return currentMajorSolarTerm(rd) == currentMajorSolarTerm(newMoonOnOrAfter(rd+1));
}

// Auxiliary function (CC:UE 19.12)
bool KoreanCalendar::priorLeapMonth(int mP, int m)
{
	return (m>=mP) && (noMajorSolarTerm(m) || priorLeapMonth(mP, newMoonBefore(m)));
}

// Return RD date of Chinese New Year in the Sui (year) of rd (CC:UE 19.13)
int KoreanCalendar::newYearInSui(int rd)
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

int KoreanCalendar::newYearOnOrBefore(int rd)
{
	const int newYear=newYearInSui(rd);
	if (rd>=newYear)
		return newYear;
	else
		return newYearInSui(rd-180);
}


//// Retrieve numerical components from the cycle number [1..60].  (following CC:UE 19.18)
//QPair<int, int> KoreanCalendar::sexagesimalNumbers(int n)
//{
//	const int first=StelUtils::amod(n, 10);
//	const int second=StelUtils::amod(n, 12);
//	return QPair<int,int>(first, second);
//}

// Retrieve name components from the cycle number [1..60].  (CC:UE 19.18)
// In contrast to chineseSexagesimalNumbers, this provides the pair (chinese double-name, translated double-name)
QPair<QString, QString> KoreanCalendar::sexagesimalNames(int n)
{
	const QPair<int,int>number=sexagesimalNumbers(n);
	const QString stem=celestialStems.value(number.first);
	const QString branch=terrestrialBranches.value(number.second);
	const QString stemTr=celestialStemsElements.value(number.first);
	const QString branchTr=terrestrialBranchesAnimalTotems.value(number.second);

	return {QString("%1%2").arg(stem, branch), QString("%1, %2").arg(stemTr, branchTr)};
}

//// Retrieve year difference between name pairs. [1..60].  (CC:UE 19.19)
//int KoreanCalendar::chineseNameDifference(QPair<int,int>stemBranch1, QPair<int,int>stemBranch2)
//{
//	const int stem1=stemBranch1.first;
//	const int branch1=stemBranch1.second;
//	const int stem2=stemBranch2.first;
//	const int branch2=stemBranch2.second;
//	const int stemDifference=stem2-stem1;
//	const int branchDifference=branch2-branch1;
//	return StelUtils::amod(stemDifference+25*(branchDifference-stemDifference) , 60);
//}

// Retrieve pair of names (chinese stem_branch, translated stem_branch) for Chinese year year (CC:UE 19.20)
QPair<QString, QString> KoreanCalendar::yearName(int year)
{
	return sexagesimalNames(year);
}

// Retrieve pair of names (chinese stem_branch, translated stem_branch) for Chinese month within a year (CC:UE 19.22)
QPair<QString, QString> KoreanCalendar::monthName(int month, int year)
{
	const int elapsedMonths=12*(year-1)+month-1;
	return sexagesimalNames(elapsedMonths-chineseMonthNameEpoch);
}

//// Retrieve one number (1...60) for Chinese day (after CC:UE 19.24)
//int KoreanCalendar::dayNumber(int rd)
//{
//	return StelUtils::amod(rd-chineseDayNameEpoch, 60);
//}
//
//// Retrieve pair of index numbers (stem, branch) for Chinese day (after CC:UE 19.24)
//QPair<int, int> KoreanCalendar::dayNumbers(int rd)
//{
//	return sexagesimalNumbers(rd-chineseDayNameEpoch);
//}

// Retrieve pair of names (chinese stem_branch, translated stem_branch) for Chinese day (CC:UE 19.24)
QPair<QString, QString> KoreanCalendar::dayName(int rd)
{
	return sexagesimalNames(rd-chineseDayNameEpoch);
}

//// Retrieve RD of day number (1...60) on or before rd. (after CC:UE 19.25)
//int KoreanCalendar::dayNumberOnOrBefore(QPair<int,int>stemBranch, int rd)
//{
//	return StelUtils::amod(chineseNameDifference(chineseDayNumbers(0), stemBranch), rd, rd-60);
//}

// Return Korean year number beginning in Winter of Gregorian year gYear (CC:UE before 19.27, but with Korean year number)
int KoreanCalendar::koreanNewYearInGregorianYear(int gYear)
{
	return gYear+1-GregorianCalendar::gregorianYearFromFixed(ChineseCalendar::chineseEpoch)-304;
}

//// Return Chinese year number beginning in Winter of Gregorian year gYear (CC:UE 19.27)
//int KoreanCalendar::DragonFestivalInGregorianYear(int gYear)
//{
//	int elapsedYears = gYear+1-GregorianCalendar::gregorianYearFromFixed(ChineseCalendar::chineseEpoch);
//	int cycle=StelUtils::intFloorDiv(elapsedYears, 60)+1;
//	int year=StelUtils::amod(elapsedYears, 60);
//	return fixedFromChinese({cycle, year, 5, false, 5});
//}
//
//// Return RD of Winter minor term of Gregorian year gYear (CC:UE 19.28)
//int KoreanCalendar::qingMing(int gYear)
//{
//	return qRound(floor(minorSolarTermOnOrAfter(GregorianCalendar::fixedFromGregorian({gYear, GregorianCalendar::march, 30}))));
//}
//
// Return age of someone born on birthdate on date rd as expressed by the Korean calendar.
// This is a mix of Chinese Age and Western calendar, as Koreans consider January 1
// as turn of the Year in this calculation (https://www.90daykorean.com/korean-age-all-about-age-in-korea/)
// Therefore: A new-born is aged 1. Age increases at Gregorian New Year.
int KoreanCalendar::koreanAge(QVector<int>birthdate, int rd)
{
	const int gYearRD=GregorianCalendar::gregorianFromFixed(rd).at(0);
	const int rdBirth=fixedFromKorean(birthdate);
	const int gYearBirth=GregorianCalendar::gregorianFromFixed(rdBirth).at(0);

	if (rd>=rdBirth)
		return 	gYearRD-gYearBirth+1;
	else
		return 0;
}

// Return Korean Year in the Danki system, counting from 2333BCE. (CC:UE 19.37)
int KoreanCalendar::koreanYear(int cycle, int year)
{
	return cycle*60+year-364;
}




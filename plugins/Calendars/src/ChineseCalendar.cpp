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
#include "ChineseCalendar.hpp"
#include "GregorianCalendar.hpp"
#include "StelUtils.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"

ChineseCalendar::ChineseCalendar(double jd): Calendar(jd)
{
	ChineseCalendar::retranslate();
}

const int ChineseCalendar::chineseEpoch=GregorianCalendar::fixedFromGregorian({-2636, GregorianCalendar::february, 15}); // CC:UE 19.15
QMap<int, QString> ChineseCalendar::majorSolarTerms;
QMap<int, QString> ChineseCalendar::minorSolarTerms;
QMap<int, QString> ChineseCalendar::celestialStems;
QMap<int, QString> ChineseCalendar::celestialStemsElements;
QMap<int, QString> ChineseCalendar::terrestrialBranches;
QMap<int, QString> ChineseCalendar::terrestrialBranchesAnimalTotems;

void ChineseCalendar::retranslate()
{
	// fill the name lists with translated month and day names
	majorSolarTerms={
		{ 1, qc_("Yŭshuĭ (Rain Water)"            , "Chinese calendar major Solar term")},
		{ 2, qc_("Chūnfēn (Spring Equinox)"       , "Chinese calendar major Solar term")},
		{ 3, qc_("Gŭyŭ (Grain Rain)"              , "Chinese calendar major Solar term")},
		{ 4, qc_("Xiăomăn (Grain Full)"           , "Chinese calendar major Solar term")},
		{ 5, qc_("Xiàzhì (Summer Solstice)"       , "Chinese calendar major Solar term")},
		{ 6, qc_("Dàshŭ (Great Heat)"             , "Chinese calendar major Solar term")},
		{ 7, qc_("Chŭshŭ (Limit of Heat)"         , "Chinese calendar major Solar term")},
		{ 8, qc_("Qiūfēn (Autumnal Equinox)"      , "Chinese calendar major Solar term")},
		{ 9, qc_("Shuāngjiàng (Descent of Frost)" , "Chinese calendar major Solar term")},
		{10, qc_("Xiăoxuĕ (Slight Snow)"          , "Chinese calendar major Solar term")},
		{11, qc_("Dōngzhì (Winter Solstice)"      , "Chinese calendar major Solar term")},
		{12, qc_("Dàhán (Great Cold)"             , "Chinese calendar major Solar term")}};
	minorSolarTerms={
		{ 1, qc_("Lìchūn (Beginning of Spring)" , "Chinese calendar minor Solar term")},
		{ 2, qc_("Jīngzhé (Waking of Insects)"  , "Chinese calendar minor Solar term")},
		{ 3, qc_("Qīngmíng (Pure Brightness)"   , "Chinese calendar minor Solar term")},
		{ 4, qc_("Lìxià (Beginning of Summer)"  , "Chinese calendar minor Solar term")},
		{ 5, qc_("Mángzhòng (Grain in Ear)"     , "Chinese calendar minor Solar term")},
		{ 6, qc_("Xiăoshŭ (Slight Heat)"        , "Chinese calendar minor Solar term")},
		{ 7, qc_("Lìqiū (Beginning of Autumn)"  , "Chinese calendar minor Solar term")},
		{ 8, qc_("Báilù (White Dew)"            , "Chinese calendar minor Solar term")},
		{ 9, qc_("Hánlù (Cold Dew)"             , "Chinese calendar minor Solar term")},
		{10, qc_("Lìdōng (Beginning of Winter)" , "Chinese calendar minor Solar term")},
		{11, qc_("Dàxuĕ (Great Snow)"           , "Chinese calendar minor Solar term")},
		{12, qc_("Xiăohán (Slight Cold)"        , "Chinese calendar minor Solar term")}};
	celestialStems={
		{ 1, qc_("Jiă"  , "Chinese calendar, celestial stem name")},
		{ 2, qc_("Yĭ"   , "Chinese calendar, celestial stem name")},
		{ 3, qc_("Bĭng" , "Chinese calendar, celestial stem name")},
		{ 4, qc_("Dīng" , "Chinese calendar, celestial stem name")},
		{ 5, qc_("Wù"   , "Chinese calendar, celestial stem name")},
		{ 6, qc_("Jĭ"   , "Chinese calendar, celestial stem name")},
		{ 7, qc_("Gēng" , "Chinese calendar, celestial stem name")},
		{ 8, qc_("Xīn"  , "Chinese calendar, celestial stem name")},
		{ 9, qc_("Rén"  , "Chinese calendar, celestial stem name")},
		{10, qc_("Guĭ"  , "Chinese calendar, celestial stem name")}};
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
		{ 1, qc_("Zĭ"      , "Chinese calendar, terrestrial branch name")},
		{ 2, qc_("Chŏu"    , "Chinese calendar, terrestrial branch name")},
		{ 3, qc_("Yín"     , "Chinese calendar, terrestrial branch name")},
		{ 4, qc_("Măo"     , "Chinese calendar, terrestrial branch name")},
		{ 5, qc_("Chén"    , "Chinese calendar, terrestrial branch name")},
		{ 6, qc_("Sì"      , "Chinese calendar, terrestrial branch name")},
		{ 7, qc_("Wŭ"      , "Chinese calendar, terrestrial branch name")},
		{ 8, qc_("Wèi"     , "Chinese calendar, terrestrial branch name")},
		{ 9, qc_("Shēn"    , "Chinese calendar, terrestrial branch name")},
		{10, qc_("Yŏu"     , "Chinese calendar, terrestrial branch name")},
		{11, qc_("Xū"      , "Chinese calendar, terrestrial branch name")},
		{12, qc_("Hài"     , "Chinese calendar, terrestrial branch name")}};
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
void ChineseCalendar::setJD(double JD)
{
	this->JD=JD;

	int rd=fixedFromJD(JD, true);
	parts=chineseFromFixed(rd);

	emit partsChanged(parts);
}

// get a stringlist of calendar date elements sorted from the largest to the smallest.
// Year, Month, MonthName, "leap"|"", Day
QStringList ChineseCalendar::getDateStrings() const
{
	// parts={cycle, year, month, leap-month, day}
	const int rd=fixedFromChinese(parts);
	const QPair<QString,QString>yearNames=yearName(parts.at(1));
	const QPair<QString,QString>monthNames=monthName(parts.at(2), parts.at(1));
	const QPair<QString,QString>dayNames=dayName(rd);

	QStringList list;
	list << QString::number((parts.at(0)-1)*60+parts.at(1)-1);           // 0:Chinese year
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
QString ChineseCalendar::getFormattedDateString() const
{
	QStringList str=getDateStrings();
	// Format: "Day [D] ([stem/branch]) - Month [M]<sub>[leap]</sub> ([stem/branch]) - year [Y]: [N] in cycle [C] ([stem/branch])")
	// TRANSLATORS: Chinese Calendar output string
	return QString(qc_("Day %1 (%2) - Month %3<sub>%4</sub> (%5) - Year %6: %7 (%8) in cycle %9", "Chinese calendar output")).arg(
				str.at(7), // 1 dayNr
				str.at(8), // 2 dayStemBranch
				str.at(4), // 3 monthNum
				str.at(6), // 4 monthLeap
				str.at(5), // 5 monthStemBranch
				str.at(0), // 6 yearNrTotal
				str.at(2), // 7 yearNrInCycle
				str.at(3), // 8 yearStemBranch
				str.at(1)  // 9 cycleNr
				);
}

// get a pair of strings for the Solar Terms for a date
QPair<QString,QString> ChineseCalendar::getSolarTermStrings() const
{
	const int rd=fixedFromChinese(parts);
	return {majorSolarTerms.value(currentMajorSolarTerm(rd)),
		minorSolarTerms.value(currentMinorSolarTerm(rd))};
}

// get a formatted string of the Solar Terms for a date
QString ChineseCalendar::getFormattedSolarTermsString() const
{
	const int rd=fixedFromChinese(parts);
	return QString("Major: <strong>%1</strong> - Minor: %2").arg(
		majorSolarTerms.value(currentMajorSolarTerm(rd)),
		minorSolarTerms.value(currentMinorSolarTerm(rd)));
}

// set date from a vector of calendar date elements sorted from the largest to the smallest.
// {cycle, year, month, leap-month, day}
// Time is not changed!
void ChineseCalendar::setDate(QVector<int> parts)
{
	this->parts=parts;

	double rd=fixedFromChinese(parts);
	// restore time from JD!
	double frac=StelUtils::fmodpos(JD+0.5+StelApp::getInstance().getCore()->getUTCOffset(JD)/24., 1.);
	JD=jdFromFixed(rd+frac, true);

	emit jdChanged(JD);
}

//! @arg parts5={cycle, year, month, leapMonth, day}
int ChineseCalendar::fixedFromChinese(QVector<int> parts5)
{
	const int cycle = parts5.value(0);
	const int year  = parts5.value(1);
	const int month = parts5.value(2);
	const int leap  = parts5.value(3);
	const int day   = parts5.value(4);

	const int midYear=qRound(floor(chineseEpoch+((cycle-1)*60+year-1+0.5)*meanTropicalYear));
	const int newYear=newYearOnOrBefore(midYear);
	const int p = newMoonOnOrAfter(newYear+(month-1)*29);
	const QVector<int>d = chineseFromFixed(p);
	int priorNewMoon;
	if ((month==d.at(2)) && (leap==d.at(3)))
		priorNewMoon=p;
	else
		priorNewMoon=newMoonOnOrAfter(p+1);

	return priorNewMoon+day-1;
}

// find date in the Chinese calendar from RD number (CC:UE 19.16)
QVector<int> ChineseCalendar::chineseFromFixed(int rd)
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
	const int elapsedYears=qRound(floor(1.5-month/12.+(rd-chineseEpoch)/meanTropicalYear));
	const int cycle = StelUtils::intFloorDiv(elapsedYears-1, 60)+1;
	const int year = StelUtils::amod(elapsedYears, 60);
	const int day = rd-m+1;
	return {cycle, year, month, leapMonth, day};
}


int ChineseCalendar::currentMajorSolarTerm(int rd)
{
	double s=solarLongitude(universalFromStandard(rd, chineseLocation(rd)));
	return StelUtils::amod(2+qRound(floor(s/30.)), 12);
}

// Return location of Chinese calendar computations (Beijing). Before 1929, this used LMST. CC:UE 19.2
StelLocation ChineseCalendar::chineseLocation(double rd_t)
{
	const int year=GregorianCalendar::gregorianYearFromFixed(qRound(floor(rd_t)));
	StelLocation loc("Beijing", "China", "Eastern Asia", 116.+(25./60.), 39.+(55./60.), 44, 8000, "Asia/Shanghai", 9);
	if (year<1929)
		loc.ianaTimeZone="UTC+07:45";
	return loc;
}

// Return the moment when solar longitude reaches lambda (CC:UE 19.3)
double ChineseCalendar::solarLongitudeOnOrAfter(double lambda, double rd_t)
{
	const double sun=solarLongitudeAfter(lambda, universalFromStandard(rd_t, chineseLocation(rd_t)));
	return standardFromUniversal(sun, chineseLocation(sun));
}

int ChineseCalendar::majorSolarTermOnOrAfter(int rd)
{
	const double s=solarLongitude(midnightInChina(rd));
	const int l = StelUtils::imod(30*qRound(ceil(s/30.)), 360);

	return solarLongitudeOnOrAfter(l, rd);
}

// Return current minor solar term for rd (CC:UE 19.5)
int ChineseCalendar::currentMinorSolarTerm(int rd)
{
	const double s=solarLongitude(universalFromStandard(rd, chineseLocation(rd)));
	return StelUtils::amod(3+qRound(floor((s-15.)/30.)), 12);
}

// Return minor solar term (CC:UE 19.6)
int ChineseCalendar::minorSolarTermOnOrAfter(int rd)
{
	const double s=solarLongitude(midnightInChina(rd));
	const int l = StelUtils::imod(30*qRound(ceil((s-15.)/30.))+15, 360);

	return solarLongitudeOnOrAfter(l, rd);
}

// Return rd moment of midnight (CC:UE 19.7)
double ChineseCalendar::midnightInChina(int rd)
{
	return universalFromStandard(rd, chineseLocation(rd));
}

// Return Chinese Winter Solstice date (CC:UE 19.8)
int ChineseCalendar::winterSolsticeOnOrBefore(int rd)
{
	const double approx=estimatePriorSolarLongitude(double(Calendar::winter), midnightInChina(rd+1));

	int day=qRound(floor(approx))-2;
	double lng;
	do {
		day++;
		//lng=modInterval(solarLongitude(midnightInChina(day+1)), -180., 180.);
		lng=solarLongitude(midnightInChina(day+1));
	} while (double(Calendar::winter) >= lng);
	return day;
}

// Return Chinese New Moon (CC:UE 19.9)
int ChineseCalendar::newMoonOnOrAfter(int rd)
{
	const double t=Calendar::newMoonAtOrAfter(midnightInChina(rd));
	return qRound(floor(standardFromUniversal(t, chineseLocation(t))));
}

// Return Chinese New Moon (CC:UE 19.10)
int ChineseCalendar::newMoonBefore(int rd)
{
	const double t=Calendar::newMoonBefore(midnightInChina(rd));
	return qRound(floor(standardFromUniversal(t, chineseLocation(t))));
}

// Auxiliary function (CC:UE 19.11)
bool ChineseCalendar::noMajorSolarTerm(int rd)
{
	return currentMajorSolarTerm(rd) == currentMajorSolarTerm(newMoonOnOrAfter(rd+1));
}

// Auxiliary function (CC:UE 19.12)
bool ChineseCalendar::priorLeapMonth(int mP, int m)
{
	return (m>=mP) && (noMajorSolarTerm(m) || priorLeapMonth(mP, newMoonBefore(m)));
}

// Return RD date of Chinese New Year in the Sui (year) of rd (CC:UE 19.13)
int ChineseCalendar::newYearInSui(int rd)
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

int ChineseCalendar::newYearOnOrBefore(int rd)
{
	const int newYear=newYearInSui(rd);
	if (rd>=newYear)
		return newYear;
	else
		return newYearInSui(rd-180);
}


// Retrieve numerical components from the cycle number [1..60].  (following CC:UE 19.18)
QPair<int, int> ChineseCalendar::sexagesimalNumbers(int n)
{
	const int first=StelUtils::amod(n, 10);
	const int second=StelUtils::amod(n, 12);
	return QPair<int,int>(first, second);
}

// Retrieve name components from the cycle number [1..60].  (CC:UE 19.18)
// In contrast to chineseSexagesimalNumbers, this provides the pair (chinese double-name, translated double-name)
QPair<QString, QString> ChineseCalendar::sexagesimalNames(int n)
{
	const QPair<int,int>number=sexagesimalNumbers(n);
	const QString stem=celestialStems.value(number.first);
	const QString branch=terrestrialBranches.value(number.second);
	const QString stemTr=celestialStemsElements.value(number.first);
	const QString branchTr=terrestrialBranchesAnimalTotems.value(number.second);

	return {QString("%1%2").arg(stem, branch), QString("%1, %2").arg(stemTr, branchTr)};
}

// Retrieve year difference between name pairs. [1..60].  (CC:UE 19.19)
int ChineseCalendar::chineseNameDifference(QPair<int,int>stemBranch1, QPair<int,int>stemBranch2)
{
	const int stem1=stemBranch1.first;
	const int branch1=stemBranch1.second;
	const int stem2=stemBranch2.first;
	const int branch2=stemBranch2.second;
	const int stemDifference=stem2-stem1;
	const int branchDifference=branch2-branch1;
	return StelUtils::amod(stemDifference+25*(branchDifference-stemDifference) , 60);
}

// Retrieve pair of names (chinese stem_branch, translated stem_branch) for Chinese year year (CC:UE 19.20)
QPair<QString, QString> ChineseCalendar::yearName(int year)
{
	return sexagesimalNames(year);
}

// Retrieve pair of names (chinese stem_branch, translated stem_branch) for Chinese month within a year (CC:UE 19.22)
QPair<QString, QString> ChineseCalendar::monthName(int month, int year)
{
	const int elapsedMonths=12*(year-1)+month-1;
	return sexagesimalNames(elapsedMonths-chineseMonthNameEpoch);
}

// Retrieve one number (1...60) for Chinese day (after CC:UE 19.24)
int ChineseCalendar::dayNumber(int rd)
{
	return StelUtils::amod(rd-chineseDayNameEpoch, 60);
}

// Retrieve pair of index numbers (stem, branch) for Chinese day (after CC:UE 19.24)
QPair<int, int> ChineseCalendar::dayNumbers(int rd)
{
	return sexagesimalNumbers(rd-chineseDayNameEpoch);
}

// Retrieve pair of names (chinese stem_branch, translated stem_branch) for Chinese day (CC:UE 19.24)
QPair<QString, QString> ChineseCalendar::dayName(int rd)
{
	return sexagesimalNames(rd-chineseDayNameEpoch);
}

// Retrieve RD of day number (1...60) on or before rd. (after CC:UE 19.25)
int ChineseCalendar::dayNumberOnOrBefore(QPair<int,int>stemBranch, int rd)
{
	return StelUtils::amod(chineseNameDifference(dayNumbers(0), stemBranch), rd, rd-60);
}

// Return Chinese year number beginning in Winter of Gregorian year gYear (CC:UE before 19.27)
int ChineseCalendar::ChineseNewYearInGregorianYear(int gYear)
{
	return gYear+1-GregorianCalendar::gregorianYearFromFixed(chineseEpoch);
}

// Return Chinese year number beginning in Winter of Gregorian year gYear (CC:UE 19.27)
int ChineseCalendar::DragonFestivalInGregorianYear(int gYear)
{
	int elapsedYears = gYear+1-GregorianCalendar::gregorianYearFromFixed(chineseEpoch);
	int cycle=StelUtils::intFloorDiv(elapsedYears, 60)+1;
	int year=StelUtils::amod(elapsedYears, 60);
	return fixedFromChinese({cycle, year, 5, false, 5});
}

// Return RD of Winter minor term of Gregorian year gYear (CC:UE 19.28)
int ChineseCalendar::qingMing(int gYear)
{
	return qRound(floor(minorSolarTermOnOrAfter(GregorianCalendar::fixedFromGregorian({gYear, GregorianCalendar::march, 30}))));
}

// Return age of someone born on birthdate on date rd as expressed by Chinese (CC:UE 19.29)
// Returns bogus on error
int ChineseCalendar::chineseAge(QVector<int>birthdate, int rd)
{
	const QVector<int>today=chineseFromFixed(rd);
	const int todayCycle=today.at(0);
	const int todayYear=today.at(1);
	const int birthdateCycle=birthdate.at(0);
	const int birthdateYear =birthdate.at(1);
	if (rd>=fixedFromChinese(birthdate))
		return 60*(todayCycle-birthdateCycle)+todayYear-birthdateYear+1;
	else
		return bogus;
}

// Determine marriage augury based on year number within a cycle. widows are worst, double-bright are best years.
int ChineseCalendar::chineseYearMarriageAugury(int cycle, int year)
{
	const int newYear=fixedFromChinese({cycle, year, 1, false, 1});
	const int c = (year==60) ? cycle+1 : cycle;
	const int y = (year==60) ? 1 : year+1;
	const int nextNewYear=fixedFromChinese({c, y, 1, false, 1});
	const int firstMinorTerm=currentMinorSolarTerm(newYear);
	const int nextFirstMinorTerm=currentMinorSolarTerm(nextNewYear);

	if (firstMinorTerm==1)
		return (nextFirstMinorTerm==12) ? widow : blind;
	else
		return (nextFirstMinorTerm==12) ? bright : doubleBright;
}





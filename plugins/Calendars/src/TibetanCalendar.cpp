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
#include "TibetanCalendar.hpp"
#include "GregorianCalendar.hpp"
#include "StelUtils.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"

TibetanCalendar::TibetanCalendar(double jd): Calendar(jd)
{
	TibetanCalendar::retranslate();
}

const int TibetanCalendar::tibetanEpoch=GregorianCalendar::fixedFromGregorian({-127, GregorianCalendar::december, 7});
QMap<int, QString> TibetanCalendar::weekDayNames;
QMap<int, QString> TibetanCalendar::monthNames;
QMap<int, QString> TibetanCalendar::animals;
QMap<int, QString> TibetanCalendar::elements;
QMap<int, QString> TibetanCalendar::yogas;
QMap<int, QString> TibetanCalendar::naksatras;
QMap<int, QString> TibetanCalendar::karanas;

void TibetanCalendar::retranslate()
{
	// fill the name lists with translated month and day names
	weekDayNames={
		{0, qc_("gza'nyi ma"   , "Tibetan day name")},
		{1, qc_("gza'zla ba"   , "Tibetan day name")},
		{2, qc_("gza'mig dmar" , "Tibetan day name")},
		{3, qc_("gza'lhag pa"  , "Tibetan day name")},
		{4, qc_("gza'phur bu"  , "Tibetan day name")},
		{5, qc_("gza'pa sangs" , "Tibetan day name")},
		{6, qc_("gza'spen pa"  , "Tibetan day name")}};
	monthNames={
		{ 1, qc_("dbo"      , "Tibetan month name")},
		{ 2, qc_("nag pa"   , "Tibetan month name")},
		{ 3, qc_("sa ga"    , "Tibetan month name")},
		{ 4, qc_("snron"    , "Tibetan month name")},
		{ 5, qc_("chu stod" , "Tibetan month name")},
		{ 6, qc_("gro bzhin", "Tibetan month name")},
		{ 7, qc_("khrums"   , "Tibetan month name")},
		{ 8, qc_("tha skar" , "Tibetan month name")},
		{ 9, qc_("smin drug", "Tibetan month name")},
		{10, qc_("mgo"      , "Tibetan month name")},
		{11, qc_("rgyal"    , "Tibetan month name")},
		{12, qc_("mchu"     , "Tibetan month name")}};
	animals={
		{ 1, qc_("byi ba (Mouse)" , "Tibetan animal totems")},
		{ 2, qc_("glang (Ox)"     , "Tibetan animal totems")},
		{ 3, qc_("stag (Tiger)"   , "Tibetan animal totems")},
		{ 4, qc_("yos (Hare)"     , "Tibetan animal totems")},
		{ 5, qc_("'brug (Dragon)" , "Tibetan animal totems")},
		{ 6, qc_("sbrul (Snake)"  , "Tibetan animal totems")},
		{ 7, qc_("rta (Horse)"    , "Tibetan animal totems")},
		{ 8, qc_("lug (Sheep)"    , "Tibetan animal totems")},
		{ 9, qc_("spre'u (Monkey)", "Tibetan animal totems")},
		{10, qc_("bya (Bird)"     , "Tibetan animal totems")},
		{11, qc_("khyi (Dog)"     , "Tibetan animal totems")},
		{12, qc_("phag (Pig)"     , "Tibetan animal totems")}};
	elements={
		{ 1, qc_("shing (Wood, Green)", "Tibetan calendar element")},
		{ 2, qc_("me (Fire, Red)"     , "Tibetan calendar element")},
		{ 3, qc_("sa (Earth, Yellow)" , "Tibetan calendar element")},
		{ 4, qc_("lcags (Iron, White)", "Tibetan calendar element")},
		{ 5, qc_("chu (Water, Blue)"  , "Tibetan calendar element")}};
	yogas={
		{ 1, qc_("sel ba"           , "Tibetan yoga")},
		{ 2, qc_("mdza’ ba"         , "Tibetan yoga")},
		{ 3, qc_("tshe dang ldan pa", "Tibetan yoga")},
		{ 4, qc_("skal bzang"       , "Tibetan yoga")},
		{ 5, qc_("bzang po"         , "Tibetan yoga")},
		{ 6, qc_("shin tu skrang"   , "Tibetan yoga")},
		{ 7, qc_("las bzang"        , "Tibetan yoga")},
		{ 8, qc_("’dzin pa"         , "Tibetan yoga")},
		{ 9, qc_("zug rngu"         , "Tibetan yoga")},
		{10, qc_("skrang"           , "Tibetan yoga")},
		{11, qc_("’phel ba"         , "Tibetan yoga")},
		{12, qc_("nges pa"          , "Tibetan yoga")},
		{13, qc_("kun ’joms"        , "Tibetan yoga")},
		{14, qc_("dga’ ba"          , "Tibetan yoga")},
		{15, qc_("rdo rje"          , "Tibetan yoga")},
		{16, qc_("grub pa"          , "Tibetan yoga")},
		{17, qc_("shin tu lhung"    , "Tibetan yoga")},
		{18, qc_("mchog can"        , "Tibetan yoga")},
		{19, qc_("yongs ’joms"      , "Tibetan yoga")},
		{20, qc_("zhi ba"           , "Tibetan yoga")},
		{21, qc_("grub pa"          , "Tibetan yoga")},
		{22, qc_("bsgrub bya"       , "Tibetan yoga")},
		{23, qc_("dge ba"           , "Tibetan yoga")},
		{24, qc_("dkar po"          , "Tibetan yoga")},
		{25, qc_("tshangs pa"       , "Tibetan yoga")},
		{26, qc_("dbang po"         , "Tibetan yoga")},
		{27, qc_("khon ’dzin"       , "Tibetan yoga")}};
	naksatras={
		{ 1, qc_("tha skar"         , "Tibetan nakshatra")},
		{ 2, qc_("bra nye"          , "Tibetan nakshatra")},
		{ 3, qc_("smin drug"        , "Tibetan nakshatra")},
		{ 4, qc_("snar ma"          , "Tibetan nakshatra")},
		{ 5, qc_("mgo"              , "Tibetan nakshatra")},
		{ 6, qc_("lag"              , "Tibetan nakshatra")},
		{ 7, qc_("nabs so"          , "Tibetan nakshatra")},
		{ 8, qc_("rgyal"            , "Tibetan nakshatra")},
		{ 9, qc_("skag"             , "Tibetan nakshatra")},
		{10, qc_("mchu"             , "Tibetan nakshatra")},
		{11, qc_("gre"              , "Tibetan nakshatra")},
		{12, qc_("dbo"              , "Tibetan nakshatra")},
		{13, qc_("me bzhi"          , "Tibetan nakshatra")},
		{14, qc_("nag pa"           , "Tibetan nakshatra")},
		{15, qc_("sa ri"            , "Tibetan nakshatra")},
		{16, qc_("sa ga"            , "Tibetan nakshatra")},
		{17, qc_("lha mtshams"      , "Tibetan nakshatra")},
		{18, qc_("snron"            , "Tibetan nakshatra")},
		{19, qc_("snrubs"           , "Tibetan nakshatra")},
		{20, qc_("chu stod"         , "Tibetan nakshatra")},
		{21, qc_("chu smad"         , "Tibetan nakshatra")},
		{22, qc_("gro bzhin"        , "Tibetan nakshatra")},
		{23, qc_("byi bzhin"        , "Tibetan nakshatra")},
		{24, qc_("mon gre"          , "Tibetan nakshatra")},
		{25, qc_("mon gru"          , "Tibetan nakshatra")},
		{26, qc_("khrums stod"      , "Tibetan nakshatra")},
		{27, qc_("khrums smad"      , "Tibetan nakshatra")},
		{28, qc_("nam gru"          , "Tibetan nakshatra")}};
	karanas={
		{ 0, qc_("mi sdug pa"       , "Tibetan karana")},
		{ 1, qc_("gdab pa"          , "Tibetan karana")},
		{ 2, qc_("byis pa"          , "Tibetan karana")},
		{ 3, qc_("rigs can"         , "Tibetan karana")},
		{ 4, qc_("til rdung"        , "Tibetan karana")},
		{ 5, qc_("khyim skyes"      , "Tibetan karana")},
		{ 6, qc_("tshong ba"        , "Tibetan karana")},
		{ 7, qc_("vishti"           , "Tibetan karana")},
		{ 8, qc_("bkra shis"        , "Tibetan karana")},
		{ 9, qc_("rkang bzhi"       , "Tibetan karana")},
		{10, qc_("klu"              , "Tibetan karana")}};
}

// Set a calendar date from the Julian day number
void TibetanCalendar::setJD(double JD)
{
	this->JD=JD;

	int rd=fixedFromJD(JD, true);
	parts=tibetanFromFixed(rd);

	emit partsChanged(parts);
}

// get a stringlist of calendar date elements sorted from the largest to the smallest.
// Year, Month, MonthName, "leap"|"", Day, "leap"|"", WeekDayName
QStringList TibetanCalendar::getDateStrings() const
{
	const int rd=fixedFromTibetan(parts);
	const int dow=dayOfWeekFromFixed(rd);

	// parts={year, month, leap-month, day, leap-day}
	QStringList list;
	list << QString::number(parts.at(0));            // 0:year
	list << QString::number(parts.at(1));            // 1:month
	list << monthNames.value(parts.at(1), "error");  // 2:monthName
	list << (parts.at(2)==1 ? qc_("leap", "calendar term like leap year or leap day") : ""); // 3: leap? (only if leap)
	list << QString::number(parts.at(3));            // 4:day
	list << (parts.at(4)==1 ? qc_("leap", "calendar term like leap year or leap day") : ""); // 5: leap? (only if leap)
	list << weekDayNames.value(dow);                 // 6:weekday

	return list;
}

// get a formatted complete string for a date
QString TibetanCalendar::getFormattedDateString() const
{
	QStringList str=getDateStrings();
	// TRANSLATORS: A.T. stands for "anno Tibetarum".
	QString epoch = qc_("A.T.", "epoch");
	// Format: [weekday], [day_{leap}] - [month, numeral] ([month, name]_{leap}) - [year] [epoch]
	return QString("%1, %2<sub>%3</sub> - %4 (%5)<sub>%6</sub> - %7 %8").arg(
				str.at(6), // 1 weekday
				str.at(4), // 2 day
				str.at(5), // 3 dayLeap (only displayed when indeed a leap day)
				str.at(1), // 4 monthNum
				str.at(2), // 5 monthName
				str.at(3), // 6 monthLeap (only displayed when indeed a leap month)
				str.at(0), // 7 year
				epoch);    // 8 epoch
}

// set date from a vector of calendar date elements sorted from the largest to the smallest.
// {year, month, leap-month, day, leap-day}
// Time is not changed!
void TibetanCalendar::setDate(QVector<int> parts)
{
	this->parts=parts;

	double rd=fixedFromTibetan(parts);
	// restore time from JD!
	double frac=StelUtils::fmodpos(JD+0.5+StelApp::getInstance().getCore()->getUTCOffset(JD)/24., 1.);
	JD=jdFromFixed(rd+frac, true);

	emit jdChanged(JD);
}

//! @arg tibetan={year, month, leapMonth, day, leapDay}
int TibetanCalendar::fixedFromTibetan(QVector<int> tibetan)
{
	const int year      = tibetan.value(0);
	const int month     = tibetan.value(1);
	const int leapMonth = tibetan.value(2);
	const int day       = tibetan.value(3);
	const int leapDay   = tibetan.value(4);

	const int months=qRound(floor(804./65.*(year-1) + (67./65.)*month + (leapMonth ? -1 : 0) + (64./65.)));
	const int days = 30*months+day;
	const double mean=days*(11135./11312.) - 30. + (leapDay ? 0. : -1.) +(1071./1616.);
	const double solarAnomaly = StelUtils::fmodpos(days*(13./4824.)+(2117./4824.), 1.);
	const double lunarAnomaly = StelUtils::fmodpos(days*(3781./105840.)+(2837./15120.), 1.);
	const double sun  = -tibetanSunEquation(12.*solarAnomaly);
	const double moon =  tibetanMoonEquation(28.*lunarAnomaly);

	return qRound(floor(tibetanEpoch+mean+sun+moon));
}

QVector<int> TibetanCalendar::tibetanFromFixed(int rd)
{
	static const double Y=365.+4975./18382.;
	const int years=qRound(ceil((rd-tibetanEpoch)/Y));

	int year0=years-2;
	do {
		year0++;
	} while (rd>=fixedFromTibetan({year0+1, 1, false, 1, false}));

	int month0=-1;
	do {
		month0++;
	} while (rd>=fixedFromTibetan({year0, month0+1, false, 1, false}));

	const int est=rd-fixedFromTibetan({year0, month0, false, 1, false});

	int day0=est-3;
	do {
		day0++;
	} while (rd>=fixedFromTibetan({year0, month0, false, day0+1, false}));

	const int leapMonth=day0>30;
	const int day=StelUtils::amod(day0, 30);
	const int month=StelUtils::amod(
				(day>day0 ? month0-1 :
					    leapMonth ? month0 + 1 : month0)
				, 12);
	const int year=((day>day0) && (month0==1) ? year0-1 :
						    leapMonth && (month==12) ? year0+1 : year0);
	const int leapDay=rd==fixedFromTibetan({year, month, leapMonth, day, 1});
	return {year, month, leapMonth, day, leapDay};
}

// return Tibetan Sun Equation, interpolation scheme of 12 discrete values (CC:UE 21.2)
// input alpha from [0...12]
double TibetanCalendar::tibetanSunEquation(const double alpha)
{
	if (alpha>6.) {
		return -tibetanSunEquation(alpha-6.);
	}
	if (alpha>3.) {
		return tibetanSunEquation(6.-alpha);
	}
	static const QMap<double,double>map({{0.,0.}, {1.,6./60.}, {2.,10./60.}, {3.,11./60.}});

	// It seems we cannot lookup double indices in a QMap.
	double res=0;
	if (qFuzzyCompare(alpha, 0.))
		res=map.value(0.);
	else if (qFuzzyCompare(alpha, 1.))
		res=map.value(1.);
	else if (qFuzzyCompare(alpha, 2.))
		res=map.value(2.);
	else if (qFuzzyCompare(alpha, 3.))
		res=map.value(3.);
	else res=map.value(alpha,
			   StelUtils::fmodpos(alpha, 1.)*tibetanSunEquation(ceil(alpha)) +
			   StelUtils::fmodpos(-alpha, 1.)*tibetanSunEquation(floor(alpha))
			   );
	return res;
}
// return Tibetan Moon Equation, one of 28 discrete values (CC:UE 21.3)
// input alpha from [0...28]
double TibetanCalendar::tibetanMoonEquation(const double alpha)
{
	if (alpha>14.) return -tibetanMoonEquation(alpha-14.);
	if (alpha> 7.) return tibetanMoonEquation(14.-alpha);

	static const QMap<double,double>map({{0.,0.},      {1.,5./60.},  {2.,10./60.}, {3.,15./60.},
					     {4.,19./60.}, {5.,22./60.}, {6.,24./60.}, {7.,25./60.}});
	double res=0;
	if (qFuzzyCompare(alpha, 0.))
		res=map.value(0.);
	else if (qFuzzyCompare(alpha, 1.))
		res=map.value(1.);
	else if (qFuzzyCompare(alpha, 2.))
		res=map.value(2.);
	else if (qFuzzyCompare(alpha, 3.))
		res=map.value(3.);
	else if (qFuzzyCompare(alpha, 4.))
		res=map.value(4.);
	else if (qFuzzyCompare(alpha, 5.))
		res=map.value(5.);
	else if (qFuzzyCompare(alpha, 6.))
		res=map.value(6.);
	else if (qFuzzyCompare(alpha, 7.))
		res=map.value(7.);
	else res= map.value(alpha,
			    StelUtils::fmodpos(alpha, 1.)*tibetanMoonEquation(ceil(alpha)) +
			    StelUtils::fmodpos(-alpha, 1.)*tibetanMoonEquation(floor(alpha))
			   );
	return res;
}

// Holidays
// return true for a Tibetan leap month (CC:UE 21.6)
bool TibetanCalendar::tibetanLeapMonth(const QVector<int> tYM)
{
	const int tYear  = tYM.value(0);
	const int tMonth = tYM.value(1);
	return tMonth == tibetanFromFixed(fixedFromTibetan({tYear, tMonth, true, 2, true})).value(1);
}

// return true for a Tibetan leap day (CC:UE 21.7)
bool TibetanCalendar::tibetanLeapDay(const QVector<int> tYMD)
{
	const int tYear  = tYMD.value(0);
	const int tMonth = tYMD.value(1);
	const int tDay   = tYMD.value(1);
	return (tDay==tibetanFromFixed(fixedFromTibetan({tYear, tMonth, false, tDay, true})).value(3))
		|| (tDay==tibetanFromFixed(fixedFromTibetan({tYear, tMonth, tibetanLeapMonth({tYear, tMonth}), tDay, true})).value(3));
}
// return RD of Losar (New Year) with year number in Tibetan calendar (CC:UE 21.8)
int TibetanCalendar::losar(const int tYear)
{
	return fixedFromTibetan({tYear, 1, tibetanLeapMonth({tYear, 1}), 1, false});
}
// return a QVector<int> of possible Losar (New Year) dates in a Gregorian year (CC:UE 21.9)
// note the QVector usually contains one element, but in 719 had zero, in 718 and 12698 two.
QVector<int> TibetanCalendar::tibetanNewYear(const int gYear)
{
	const int dec31=GregorianCalendar::gregorianYearEnd(gYear);
	const int tYear=tibetanFromFixed(dec31).value(0);
	QVector<int>cand({losar(tYear-1), losar(tYear)});
	QVector<int>range=GregorianCalendar::gregorianYearRange(gYear);

	return Calendar::intersectWithRange(cand, range);
}

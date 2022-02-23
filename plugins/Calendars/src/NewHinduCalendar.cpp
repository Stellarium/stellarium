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
#include "NewHinduCalendar.hpp"
#include "GregorianCalendar.hpp"
#include "StelUtils.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"

// timezone for Ujjain must be set to UT (acknowledged erratum in CC:UE)
//                                           Name   country region           long                   lat       alt popK  Timezone          Bortle Role
const StelLocation NewHinduCalendar::ujjain   ("Ujjain", "In", "Southern Asia", 75.+46./60.+6./3600.,  23.+9./60., 0, 550, "UTC+05:03", 7, 'N');
const StelLocation NewHinduCalendar::ujjainUTC("Ujjain", "In", "Southern Asia", 75.+46./60.+6./3600.,  23.+9./60., 0, 550, "UTC"      , 7, 'N');
const StelLocation NewHinduCalendar::hinduLocation=ujjain;

QMap<int, QString>NewHinduCalendar::lunarStations;
QMap<int, QString>NewHinduCalendar::yogas;
QMap<int, QString>NewHinduCalendar::karanas;

NewHinduCalendar::NewHinduCalendar(double jd): OldHinduLuniSolarCalendar(jd)
{
	NewHinduCalendar::retranslate();
}

void NewHinduCalendar::retranslate()
{
	OldHinduLuniSolarCalendar::retranslate();
	lunarStations={
		{  1, qc_("Aśvinī"            , "Hindu Lunar Station name")},
		{  2, qc_("Bharaṇī"           , "Hindu Lunar Station name")},
		{  3, qc_("Kṛttikā"           , "Hindu Lunar Station name")},
		{  4, qc_("Rohiṇī"            , "Hindu Lunar Station name")},
		{  5, qc_("Mṛigaśiras"        , "Hindu Lunar Station name")},
		{  6, qc_("Ārdrā"             , "Hindu Lunar Station name")},
		{  7, qc_("Punarvasu"         , "Hindu Lunar Station name")},
		{  8, qc_("Puṣya"             , "Hindu Lunar Station name")},
		{  9, qc_("Āśleṣā"            , "Hindu Lunar Station name")},
		{ 10, qc_("Maghā"             , "Hindu Lunar Station name")},
		{ 11, qc_("Pūrva-Phalgunī"    , "Hindu Lunar Station name")},
		{ 12, qc_("Uttara-Phalgunī"   , "Hindu Lunar Station name")},
		{ 13, qc_("Hasta"             , "Hindu Lunar Station name")},
		{ 14, qc_("Citrā"             , "Hindu Lunar Station name")},
		{ 15, qc_("Svāti"             , "Hindu Lunar Station name")},
		{ 16, qc_("Viśākhā"           , "Hindu Lunar Station name")},
		{ 17, qc_("Anřādhā"           , "Hindu Lunar Station name")},
		{ 18, qc_("Jyeṣṭhā"           , "Hindu Lunar Station name")},
		{ 19, qc_("Mūla"              , "Hindu Lunar Station name")},
		{ 20, qc_("Pūrva-Āṣāḍhā"      , "Hindu Lunar Station name")},
		{ 21, qc_("Uttara-Āṣāḍhā"     , "Hindu Lunar Station name")},
		{  0, qc_("Abhijit"           , "Hindu Lunar Station name")},  // What to do with it?
		{ 22, qc_("Śravaṇā"           , "Hindu Lunar Station name")},
		{ 23, qc_("Dhaniṣṭhā"         , "Hindu Lunar Station name")},
		{ 24, qc_("Śatatārakā"        , "Hindu Lunar Station name")},
		{ 25, qc_("Pūrva-Bhādrapadā"  , "Hindu Lunar Station name")},
		{ 26, qc_("Uttara-Bhādrapadā" , "Hindu Lunar Station name")},
		{ 27, qc_("Revatī"            , "Hindu Lunar Station name")}
	};
	yogas={
		{  1, qc_("Viṣkambha" , "Hindu yoga name")},
		{  2, qc_("Prīti"     , "Hindu yoga name")},
		{  3, qc_("Ayuṣmān"   , "Hindu yoga name")},
		{  4, qc_("Saubhāgya" , "Hindu yoga name")},
		{  5, qc_("Śobhana"   , "Hindu yoga name")},
		{  6, qc_("Atigaṇda"  , "Hindu yoga name")},
		{  7, qc_("Sukarman"  , "Hindu yoga name")},
		{  8, qc_("Dhṛti"     , "Hindu yoga name")},
		{  9, qc_("Śūla"      , "Hindu yoga name")},
		{ 10, qc_("Gaṇda"     , "Hindu yoga name")},
		{ 11, qc_("Vṛddhi"    , "Hindu yoga name")},
		{ 12, qc_("Dhruva"    , "Hindu yoga name")},
		{ 13, qc_("Vyāghāta"  , "Hindu yoga name")},
		{ 14, qc_("Harṣaṇa"   , "Hindu yoga name")},
		{ 15, qc_("Vajra"     , "Hindu yoga name")},
		{ 16, qc_("Siddhi"    , "Hindu yoga name")},
		{ 17, qc_("Vyatīpāta" , "Hindu yoga name")},
		{ 18, qc_("Varīyas"   , "Hindu yoga name")},
		{ 19, qc_("Parigha"   , "Hindu yoga name")},
		{ 20, qc_("Śiva"      , "Hindu yoga name")},
		{ 21, qc_("Siddha"    , "Hindu yoga name")},
		{ 22, qc_("Sādhya"    , "Hindu yoga name")},
		{ 23, qc_("Śubha"     , "Hindu yoga name")},
		{ 24, qc_("Śukla"     , "Hindu yoga name")},
		{ 25, qc_("Brahman"   , "Hindu yoga name")},
		{ 26, qc_("Indra"     , "Hindu yoga name")},
		{ 27, qc_("Vaidhṛti"  , "Hindu yoga name")}
	};
	karanas={
		{  0, qc_("Kiṃstughna", "Hindu karana name")},
		{  1, qc_("Bava"      , "Hindu karana name")},
		{  2, qc_("Vālava"    , "Hindu karana name")},
		{  3, qc_("Kaulava"   , "Hindu karana name")},
		{  4, qc_("Taitila"   , "Hindu karana name")},
		{  5, qc_("Gara"      , "Hindu karana name")},
		{  6, qc_("Vaṇija"    , "Hindu karana name")},
		{  7, qc_("Viṣṭi"     , "Hindu karana name")},
		{  8, qc_("Śakuni"    , "Hindu karana name")},
		{  9, qc_("Catuṣpada" , "Hindu karana name")},
		{ 10, qc_("Nāga"      , "Hindu karana name")},
	};
}

// Set a calendar date from the Julian day number
void NewHinduCalendar::setJD(double JD)
{
	this->JD=JD;

	int rd=fixedFromJD(JD, true);
	parts=hinduSolarFromFixed(rd);

	emit partsChanged(parts);
}

// set date from a vector of calendar date elements sorted from the largest to the smallest.
// Year-Month[1...12]-Day[1...31]
// Time is not changed!
void NewHinduCalendar::setDate(QVector<int> parts)
{
	this->parts=parts;
	double rd=fixedFromHinduSolar(parts);
	// restore time from JD!
	double frac=StelUtils::fmodpos(JD+0.5+StelApp::getInstance().getCore()->getUTCOffset(JD)/24., 1.);
	JD=jdFromFixed(rd+frac, true);

	emit jdChanged(JD);
}

// get a stringlist of calendar date elements sorted from the largest to the smallest.
// Input {year, month, day}
// {Year, MonthNr, MonthName, DayNr, DayName}
QStringList NewHinduCalendar::getDateStrings() const
{
	const int rd=fixedFromHinduSolar(parts);
	const int dow=dayOfWeekFromFixed(rd);

	QStringList list;
	list << QString::number(parts.at(0));        // 0:year
	list << QString::number(parts.at(1));        // 1:Month nr
	list << monthNames.value(parts.at(1));       // 2:Month name
	list << QString::number(parts.at(2));        // 3:Day nr in month
	list << weekDayNames.value(dow, "error");    // 4:weekday
	return list;
}

// get a formatted complete string for a date
QString NewHinduCalendar::getFormattedDateString() const
{
	QStringList str=getDateStrings();
	// TRANSLATORS: S.E. stands for Saka Era
	QString epoch = qc_("S.E.", "calendar epoch");
	// Format: [weekday], [day] - [month, numeral] ([month, name]) - [year] [epoch]
	return QString("%1, %2 - %3 (%4) - %5 %6").arg(
				str.at(4),
				str.at(3),
				str.at(1),
				str.at(2),
				str.at(0),
				epoch);
}

// return { year, month, day} (CC:UE 20.20)
QVector<int> NewHinduCalendar::hinduSolarFromFixed(int rd)
{
	const double critical=hinduSunrise(rd+1);
	const int month = hinduZodiac(critical);
	const int year=hinduCalendarYear(critical)-hinduSolarEra;
	const int approx=rd-3-StelUtils::imod(qRound(floor(hinduSolarLongitude(critical))), 30);

	int start=approx-1;
	do{
		start++;
	} while (hinduZodiac(hinduSunrise(start+1))!=month);

	const int day=rd-start+1;

	return {year, month, day};
}

// parts={ year, month, day}
int NewHinduCalendar::fixedFromHinduSolar(QVector<int> parts)
{
	const int year =parts.value(0);
	const int month=parts.value(1);
	const int day  =parts.value(2);

	const int start=qRound(floor( (year+hinduSolarEra+(month-1)/12.)*hinduSiderealYear )) + hinduEpoch;

	int d=start-4;
	do{
		d++;
	}
	while (hinduZodiac(hinduSunrise(d+1))!=month);

	return day-1+d;
}

// return { year, month, leapMonth, day, leapDay } (CC:UE 20.23)
QVector<int> NewHinduCalendar::hinduLunarFromFixed(int rd)
{
	const double critical=hinduSunrise(rd);
	const int day=hinduLunarDayFromMoment(critical);
	const bool leapDay= (day==hinduLunarDayFromMoment(hinduSunrise(rd-1)));
	const double lastNewMoon=hinduNewMoonBefore(critical);
	const double nextNewMoon=hinduNewMoonBefore(floor(lastNewMoon)+35.);
	const int solarMonth=hinduZodiac(lastNewMoon);
	const bool leapMonth= (solarMonth==hinduZodiac(nextNewMoon));
	const int month=StelUtils::amod(solarMonth+1, 12);
	const int year=hinduCalendarYear(month<=2 ? rd+180 : rd) - hinduLunarEra;

	return {year, month, leapMonth, day, leapDay};
}
// return RD date from a New Hindu Lunar date (CC:UE 20.24)
// parts={ year, month, leapMonth, day, leapDay }
int NewHinduCalendar::fixedFromHinduLunar(QVector<int> parts)
{
	const int  year      = parts.value(0);
	const int  month     = parts.value(1);
	const bool leapMonth = parts.value(2);
	const int  day       = parts.value(3);
	const bool leapDay   = parts.value(4);

	const double approx=hinduEpoch+hinduSiderealYear*(year+hinduLunarEra+(month-1)/12.);
	const int s=qRound(floor(approx-hinduSiderealYear*modInterval(hinduSolarLongitude(approx)/360.-(month-1)/12., -0.5, 0.5)));
	const int k=hinduLunarDayFromMoment(s+0.25);
	const QVector<int>mid=hinduLunarFromFixed(s-15); // Middle of preceding Solar month

	int est;
	if ((3<k) && (k<27))
		est=k;
	else if ( (mid.value(1)!=month) || (mid.value(2) && !leapMonth) )
		est=modInterval(k, -15, 15);
	else
		est=modInterval(k, 15, 45);
	est = s+day-est;
	const int tau=est-modInterval(hinduLunarDayFromMoment(est+0.25)-day, -15, 15);

	int hldfmHsr;
	int date = tau-2;
	do {
		date++;
		hldfmHsr=hinduLunarDayFromMoment(hinduSunrise(date));
	} while( ((hldfmHsr != day) &&
		  (hldfmHsr != StelUtils::amod(day+1, 30))) );

	return (leapDay ? date+1 : date);
}

double NewHinduCalendar::hinduSineTable(const int entry)
{
	const double exact=3438.*sin(entry*225./60.*M_PI_180);
	const double error=0.215*StelUtils::sign(exact)*StelUtils::sign(fabs(exact)-1716.);
	return (1./3438.)*qRound(exact+error);
}
double NewHinduCalendar::hinduSine(const double theta)
{
	const double entry=theta/(225./60.);
	const double fraction=StelUtils::fmodpos(entry, 1.);
	return fraction*hinduSineTable(qRound(ceil(entry)))+(1.-fraction)*hinduSineTable(qRound(floor(entry)));
}
double NewHinduCalendar::hinduArcsin(const double amp)
{
	if (amp<0.) return -hinduArcsin(-amp);

	int pos=-1;
	do {
		pos++;
	}
	while (amp>hinduSineTable(pos));
	const double below=hinduSineTable(pos-1);
	return (225./60.)*(pos-1.+(amp-below)/(hinduSineTable(pos)-below));
}

double NewHinduCalendar::hinduMeanPosition(const double rd_ut, const double period)
{
	return 360.*StelUtils::fmodpos((rd_ut-hinduCreation)/period, 1.);
}

double NewHinduCalendar::hinduTruePosition(const double rd_ut, const double period, const double size, const double anomalistic, const double change)
{
	const double lambda=hinduMeanPosition(rd_ut, period);
	const double offset=hinduSine(hinduMeanPosition(rd_ut, anomalistic));
	const double contraction=fabs(offset)*change*size;
	const double equation=hinduArcsin(offset*(size-contraction));
	return StelUtils::fmodpos(lambda-equation, 360.);
}

double NewHinduCalendar::hinduSolarLongitude(const double rd_ut)
{
	return hinduTruePosition(rd_ut, hinduSiderealYear, 14./360., hinduAnomalisticYear, 1./42.);
}

int NewHinduCalendar::hinduZodiac(const double rd_ut)
{
	return qRound(floor(hinduSolarLongitude(rd_ut)/30.))+1;
}

// return lunar longitude at RD (CC:UE 20.14)
double NewHinduCalendar::hinduLunarLongitude(const double rd_ut)
{
	return hinduTruePosition(rd_ut, hinduSiderealMonth, 32./360., hinduAnomalisticMonth, 1./96.);
}

// return lunar phase at RD (CC:UE 20.15)
double NewHinduCalendar::hinduLunarPhase(const double rd_ut)
{
	return StelUtils::fmodpos(hinduLunarLongitude(rd_ut)-hinduSolarLongitude(rd_ut), 360.);
}

// return lunar day at RD (CC:UE 20.16)
int NewHinduCalendar::hinduLunarDayFromMoment(const double rd_ut)
{
	return qRound(floor(hinduLunarPhase(rd_ut)/12.))+1;
}

// return RD of New Moon before RD (CC:UE 20.17)
double NewHinduCalendar::hinduNewMoonBefore(const double rd_ut)
{
	static const double eps=pow(2., -1000);
	const double tau=rd_ut-(hinduSynodicMonth/360.)*hinduLunarPhase(rd_ut);
	double l=tau-1.;
	double u=qMin(rd_ut, tau+1.);
	do{
		double mid=(l+u)*0.5;
		if (hinduLunarPhase(mid)>180.)
			l=mid;
		else
			u=mid;
	} while ( (hinduZodiac(l)!=hinduZodiac(u)) && (u-l>eps) );
	return (l+u)*0.5;
}

int NewHinduCalendar::hinduCalendarYear(const double rd_ut)
{
	return qRound((rd_ut-hinduEpoch)/hinduSiderealYear - hinduSolarLongitude(rd_ut)/360.);
}


// return the ascensional difference (CC:UE 20.27)
double NewHinduCalendar::hinduAscensionalDifference(const int rd, const StelLocation &loc)
{
	const double sinD=(1397./3438.)*hinduSine(hinduTropicalLongitude(rd));
	const double phi=static_cast<double>(loc.latitude);
	const double diurnalRadius=hinduSine(90.+hinduArcsin(sinD));
	const double tanPhi=hinduSine(phi)/hinduSine(90.+phi);
	const double earthSine=sinD*tanPhi;
	return hinduArcsin(-earthSine/diurnalRadius);
}
// return tropical longitude (CC:UE 20.28)
double NewHinduCalendar::hinduTropicalLongitude(const double rd_ut)
{
	const double days=rd_ut-hinduEpoch;
	const double precession=27.-fabs(108.*modInterval((600./1577917828.)*days-0.25, -0.5, 0.5));
	return StelUtils::fmodpos(hinduSolarLongitude(rd_ut)-precession, 360.);
}
// return solar sidereal difference (CC:UE 20.29)
double NewHinduCalendar::hinduSolarSiderealDifference(const double rd_ut)
{
	return hinduDailyMotion(rd_ut)*hinduRisingSign(rd_ut);
}
// return daily motion (CC:UE 20.30)
double NewHinduCalendar::hinduDailyMotion(const double rd_ut)
{
	static const double meanMotion=360./hinduSiderealYear;
	const double anomaly=hinduMeanPosition(rd_ut, hinduAnomalisticYear);
	const double epicycle=(14./360.)-(1./1080.)*fabs(hinduSine(anomaly));
	const int entry=qRound(floor(anomaly/(225./60.)));
	const double sineTableStep=hinduSineTable(entry+1)-hinduSineTable(entry);
	const double factor=-(3438./225.)*sineTableStep*epicycle;
	return meanMotion*(factor+1.);
}
// return the rising sign (CC:UE 20.31)
double NewHinduCalendar::hinduRisingSign(const double rd_ut)
{
	static const QVector<double>vec={1670., 1795., 1935., 1935., 1795., 1670.};
	const int i=qRound(floor(hinduTropicalLongitude(rd_ut)/30.));

	double sign=vec.at(i % 6);
	return sign/1800.;
}
// return the Hindu equation of time (CC:UE 20.32)
double NewHinduCalendar::hinduEquationOfTime(const double rd_ut)
{
	const double offset=hinduSine(hinduMeanPosition(rd_ut, hinduAnomalisticYear));
	const double equationSun=offset*(57.+18./60.)*(14./360. - fabs(offset)/1080.);
	return hinduDailyMotion(rd_ut)*equationSun*(hinduSiderealYear/(360.*360.));
}
// return hindu time of sunrise (CC:UE 20.33)
double NewHinduCalendar::hinduSunrise(const int rd)
{
	return rd+0.25+static_cast<double>(ujjain.longitude-hinduLocation.longitude)/360.-hinduEquationOfTime(rd)
			+ (1577917828./(1582237828.*360.))*(hinduAscensionalDifference(rd, hinduLocation)+0.25*hinduSolarSiderealDifference(rd));
}

// 20.4 Alternatives
// return hindu time of sunset (CC:UE 20.34)
double NewHinduCalendar::hinduSunset(const int rd)
{
	return rd+0.75+static_cast<double>(ujjain.longitude-hinduLocation.longitude)/360.-hinduEquationOfTime(rd)
			+ (1577917828./(1582237828.*360.))*(0.75*hinduSolarSiderealDifference(rd) - hinduAscensionalDifference(rd, hinduLocation));
}
// return hindu time  (CC:UE 20.35)
double NewHinduCalendar::hinduStandardFromSundial(const int rd_ut)
{
	const int date=fixedFromMoment(rd_ut);
	const double time=timeFromMoment(rd_ut);
	const int q=qRound(floor(4.*time));
	const double a=(q==0 ? hinduSunset(date-1) : (q==3 ? hinduSunset(date)    : hinduSunrise(date)));
	const double b=(q==0 ? hinduSunrise(date)  : (q==3 ? hinduSunrise(date+1) : hinduSunset(date)));
	return a+2.*(b-a)*(time-(q==3 ? 0.75 : (q==0 ? -0.25 : 0.25)));
}
//! Alternative Lunar calendar counted from full moon to full moon
// return { year, month, leapMonth, day, leapDay } (CC:UE 20.36)
QVector<int> NewHinduCalendar::hinduFullMoonFromFixed(int rd)
{
	QVector<int>lDate=hinduLunarFromFixed(rd); // { year, month, leapMonth, day, leapDay }
	const int day=lDate.at(3);

	if (day>=16)
		lDate.replace(1, hinduLunarFromFixed(rd+20).at(1) );
	return lDate;
}
// return RD date from a New Hindu Lunar date counted from full to full moon (CC:UE 20.37)
// parts={ year, month, leapMonth, day, leapDay }
int NewHinduCalendar::fixedFromHinduFullMoon(QVector<int> parts)
{
	const int year=parts.value(0);
	const int month=parts.value(1);
	const bool leapMonth=parts.value(2);
	const int day=parts.value(3);
	const int leapDay=parts.value(4);

	int m=StelUtils::amod(month-1, 12);
	if (leapMonth || (day<=15))
		m=month;
	else if (hinduExpunged(year, StelUtils::amod(month-1, 12)))
		m=StelUtils::amod(month-2, 12);

	return fixedFromHinduLunar({year, m, leapMonth, day, leapDay});
}
// test for expunged month (CC:UE 20.38)
bool NewHinduCalendar::hinduExpunged(const int lYear, const int lMonth)
{
	return lMonth != hinduLunarFromFixed(fixedFromHinduLunar({lYear, lMonth, false, 15, false})).at(1);
}
// Alternative sunrise formula (CC:UE 20.39)
double NewHinduCalendar::altHinduSunrise(const int rd)
{
	const double rise=dawn(rd, 47./60., hinduLocation);
	return 1./(60.*24.)*qRound(rise*24.*60.);
}

// 20.5 Astronomical Versions

double NewHinduCalendar::siderealSolarLongitude(const double rd_ut)
{
	return StelUtils::fmodpos(solarLongitude(rd_ut)-precession(rd_ut)+siderealStart(), 360.);
}
// return Lahiri value of ayanamsha (CC:UE 20.40)
double NewHinduCalendar::ayanamsha(const double rd_ut)
{
	return solarLongitude(rd_ut)-siderealSolarLongitude(rd_ut);
}
// return start of sidereal count (CC:UE 20.41)
double NewHinduCalendar::siderealStart()
{
	//return precession(universalFromLocal(meshaSamkranti(285), hinduLocation));
	// From the Errata of 2021-12-12
	return precession(solarLongitudeAfter(autumn, JulianCalendar::fixedFromJulian({285, JulianCalendar::september, 1})));
}
// return astronomical definition of hindu sunset (CC:UE 20.42)
double NewHinduCalendar::astroHinduSunset(const int rd)
{
	return dusk(rd, 0., hinduLocation);
}
// return sidereal zodiac sign (CC:UE 20.43)
int NewHinduCalendar::siderealZodiac(const double rd_ut)
{
	return qRound(floor(siderealSolarLongitude(rd_ut)/30.))+1;
}
// return astronomically defined calendar year (CC:UE 20.44)
int NewHinduCalendar::astroHinduCalendarYear(const double rd_ut)
{
	return qRound((rd_ut-hinduEpoch)/meanSiderealYear - siderealSolarLongitude(rd_ut)/360.);
}
// return astronomically defined date in the Solar calendar (CC:UE 20.45)
// result is { year, month, day}
QVector<int> NewHinduCalendar::astroHinduSolarFromFixed(const int rd)
{
	const double critical = astroHinduSunset(rd);
	const int month=siderealZodiac(critical);
	const int year=astroHinduCalendarYear(critical)-hinduSolarEra;
	const int approx=rd-3-StelUtils::imod(qRound(floor(siderealSolarLongitude(critical))), 30);

	int start=approx-1;
	do {
		start++;
	} while(siderealZodiac(astroHinduSunset(start))!=month);
	const int day=rd-start+1;

	return QVector<int>({year, month, day});
}
// return RD from astronomically defined date in the Solar calendar (CC:UE 20.46)
// arg is { year, month, day}
int NewHinduCalendar::fixedFromAstroHinduSolar(const QVector<int>date)
{
	const int  year      = date.value(0);
	const int  month     = date.value(1);
	const int  day       = date.value(2);

	const int approx=hinduEpoch-3+qRound(floor((year+hinduSolarEra+(month-1)/12.)*meanSiderealYear));

	int start=approx-1;
	do {
		start++;
	} while(siderealZodiac(astroHinduSunset(start))!=month);
	return start+day-1;
}
// (CC:UE 20.47)
int NewHinduCalendar::astroLunarDayFromMoment(const double rd_ut)
{
	return qRound(floor(lunarPhase(rd_ut)/12.))+1;
}
// return { year, month, leapMonth, day, leapDay } in an astronomically defined Lunar calendar (CC:UE 20.48)
QVector<int> NewHinduCalendar::astroHinduLunarFromFixed(const int rd)
{
	const double critical    = altHinduSunrise(rd);
	const int day            = astroLunarDayFromMoment(critical);
	const bool leapDay       = day == astroLunarDayFromMoment(altHinduSunrise(rd-1));
	const double lastNewMoon = newMoonBefore(critical);
	const double nextNewMoon = newMoonAtOrAfter(critical);
	const int solarMonth     = siderealZodiac(lastNewMoon);
	const bool leapMonth     = solarMonth==siderealZodiac(nextNewMoon);
	const int month          = StelUtils::amod(solarMonth+1, 12);
	const int year           = astroHinduCalendarYear(month<=2 ? rd+180 : rd) - hinduLunarEra;
	return QVector<int>({year, month, leapMonth, day, leapDay});
}
// return RD date from an astronomically defined New Hindu Lunar date (CC:UE 20.49)
// parts={ year, month, leapMonth, day, leapDay }
int NewHinduCalendar::fixedFromAstroHinduLunar(const QVector<int> parts)
{
	const int  year      = parts.value(0);
	const int  month     = parts.value(1);
	const bool leapMonth = parts.value(2);
	const int  day       = parts.value(3);
	const bool leapDay   = parts.value(4);
	const double approx=hinduEpoch+meanSiderealYear*(year+hinduLunarEra+(month-1)/12.);
	const int s=qRound(floor(approx-hinduSiderealYear*modInterval(siderealSolarLongitude(approx)/360.-(month-1)/12., -0.5, 0.5)));
	const int k = astroLunarDayFromMoment(s+0.25);
	const QVector<int>mid=astroHinduLunarFromFixed(s-15);

	int est=modInterval(k, 15, 45);
	if ((3<k) && (k<27))
		est=k;
	else if ( (mid.value(1)!=month) || (mid.value(2) && !leapMonth) )
		est=modInterval(k, -15, 15);
	est = s+day-est;
	const int tau=est-modInterval(astroLunarDayFromMoment(est+0.25)-day, -15, 15);

	int date = tau-2;
	do {
		date++;
	} while( (astroLunarDayFromMoment(altHinduSunrise(date)) != day) &&
		  astroLunarDayFromMoment(altHinduSunrise(date)) != StelUtils::amod(day+1, 30) );

	return (leapDay ? date+1 : date);

}

// 20.6 Holidays

// binary search for the moment when solar longitude reaches lng in the time between a and b (used in CC:UE 20.50)
double NewHinduCalendar::hinduSolarLongitudeInv(double lng, double rdA, double rdB)
{
	Q_ASSERT(lng>=0.);
	Q_ASSERT(lng<360.);
	Q_ASSERT(rdA<rdB);

	double lngA=hinduSolarLongitude(rdA);
	double lngB=hinduSolarLongitude(rdB);
	// Ensure lngB>lngA at the 360/0 discontinuity
	if (lngA>lngB)
		lngA-=360.; // we must then search around 0° for spring!
	do
	{
		double rdMid=(rdA+rdB)*0.5;
		double lngMid=hinduSolarLongitude(rdMid);
		// in case lngA<0 and lngB>0!
		if ((lngMid>lngA) && (lngMid>lngB))
			lngMid-=360.;
		Q_ASSERT(lngA<=lngMid);
		Q_ASSERT(lngMid<=lngB);

		if (lngMid<lng)
			rdA=rdMid;
		else
			rdB=rdMid;
	}
	while (rdB-rdA > 1e-5);
	return (rdA+rdB)*0.5;
}

// return moment of entry into Zodiacal sign (CC:UE 20.50)
double NewHinduCalendar::hinduSolarLongitudeAtOrAfter(const double lambda, const double rd_ut)
{
	const double tau=rd_ut+hinduSiderealYear/360.*StelUtils::fmodpos(lambda-hinduSolarLongitude(rd_ut), 360.);
	double a=qMax(rd_ut, tau-5);
	double b=tau+5;

	return hinduSolarLongitudeInv(lambda, a, b);
}
// return moment of zero longitude in Gregorian year gYear (CC:UE 20.51)
double NewHinduCalendar::meshaSamkranti(const int gYear)
{
	const int jan1=GregorianCalendar::gregorianNewYear(gYear);
	return hinduSolarLongitudeAtOrAfter(0., jan1);
}

// binary search for the moment when lunar phase reaches phase in the time between rdA and rdB (used in CC:UE 20.52)
double NewHinduCalendar::hinduLunarPhaseInv(double phase, double rdA, double rdB)
{
	Q_ASSERT(phase>=0.);
	Q_ASSERT(phase<360.);
	Q_ASSERT(rdA<rdB);

	double phaseA=hinduLunarPhase(rdA);
	double phaseB=hinduLunarPhase(rdB);
	// Ensure lngB>lngA at the 360/0 discontinuity
	if (phaseA>phaseB)
		phaseA-=360.; // we must then search around 0° for spring!
	do
	{
		double rdMid=(rdA+rdB)*0.5;
		double phaseMid=lunarPhase(rdMid);
		// in case lngA<0 and lngB>0!
		if ((phaseMid>phaseA) && (phaseMid>phaseB))
			phaseMid-=360.;
		Q_ASSERT(phaseA<=phaseMid);
		Q_ASSERT(phaseMid<=phaseB);

		if (phaseMid<phase)
			rdA=rdMid;
		else
			rdB=rdMid;
	}
	while (rdB-rdA > 1e-5);
	return (rdA+rdB)*0.5;
}

// return moment of kth Lunar day (CC:UE 20.52)
double NewHinduCalendar::hinduLunarDayAtOrAfter(const double k, const double rd_ut)
{
	const double phase=(k-1.)*12.;
	const double tau=rd_ut+(1./360.)*StelUtils::fmodpos((phase-hinduLunarPhase(rd_ut)), 360.)*hinduSynodicMonth;
	const double a=qMax(rd_ut, tau-2.);
	const double b=tau+2.;
	return hinduLunarPhaseInv(phase, a, b);
}
// return moment of Lunar new year in Gregorian year gYear (CC:UE 20.53)
double NewHinduCalendar::hinduLunarNewYear(const int gYear)
{
	const int jan1=GregorianCalendar::gregorianNewYear(gYear);
	const double mina=hinduSolarLongitudeAtOrAfter(330., jan1);
	const double newMoon=hinduLunarDayAtOrAfter(1, mina);
	const int hDay=qRound(floor(newMoon));
	const double critical=hinduSunrise(hDay);
	if ((newMoon<critical) || (hinduLunarDayFromMoment(hinduSunrise(hDay+1))==2))
		return hDay;
	else
		return hDay+1;
}
// return comparison of two lunar dates (CC:UE 20.54)
// date1 and date2 are {year, month, leapMonth, day, leapDay}
bool NewHinduCalendar::hinduLunarOnOrBefore(const QVector<int>date1, const QVector<int>date2)
{
	const int  year1      = date1.value(0);
	const int  month1     = date1.value(1);
	const bool leap1      = date1.value(2);
	const int  day1       = date1.value(3);
	const bool leapDay1   = date1.value(4);
	const int  year2      = date2.value(0);
	const int  month2     = date2.value(1);
	const bool leap2      = date2.value(2);
	const int  day2       = date2.value(3);
	const bool leapDay2   = date2.value(4);
	return (year1<year2) ||
		((year1==year2) && ((month1<month2) ||
				    ((month1==month2) && (((leap1 && !leap2) ||
							 (((leap1==leap2) && ((day1<day2) ||
										((day1==day2) && (!leapDay1 || leapDay2))))))))));
}
// return RD of actually occurring date { lYear, lMonth, lDay} (CC:UE 20.55)
int NewHinduCalendar::hinduDateOccur(const int lYear, const int lMonth, const int lDay)
{
	const QVector<int>lunar({lYear, lMonth, false, lDay, false});
	const int tryy=fixedFromHinduLunar(lunar);
	const QVector<int>mid=hinduLunarFromFixed((lDay>15 ? tryy-5 : tryy));
	const bool expunged= lMonth!=mid.at(1);
	const QVector<int>lDate({mid.at(0), mid.at(1), mid.at(2), lDay, false});

	if (expunged)
	{
		// upward search
		int d=tryy-1;
		do{
			d++;
		}while(hinduLunarOnOrBefore(hinduLunarFromFixed(d), lDate));
		return d-1;
	}
	else return (lDay!=hinduLunarFromFixed(tryy).at(3) ? tryy-1 : tryy);
}
// return a QVector<int> of RDs of actually occurring dates in a Gregorian year (CC:UE 20.56)
QVector<int> NewHinduCalendar::hinduLunarHoliday(const int lMonth, const int lDay, const int gYear)
{
	const int lYear=hinduLunarFromFixed(GregorianCalendar::gregorianNewYear(gYear)).value(0);
	const int date0=hinduDateOccur(lYear, lMonth, lDay);
	const int date1=hinduDateOccur(lYear+1, lMonth, lDay);

	QVector<int> cand({date0, date1});
	QVector<int> range=GregorianCalendar::gregorianYearRange(gYear);
	return intersectWithRange(cand, range);
}
// return a QVector<int> of RDs of actually occurring Diwali dates in a Gregorian year (CC:UE 20.57)
QVector<int> NewHinduCalendar::diwali(const int gYear)
{
	return hinduLunarHoliday(8, 1, gYear);
}
// return RD of when a tithi occurs (CC:UE 20.58)
int NewHinduCalendar::hinduTithiOccur(const int lMonth, const int tithi, const double rd_ut, const int lYear)
{
	const int approx=hinduDateOccur(lYear, lMonth, qRound(floor(tithi)));
	const double lunar=hinduLunarDayAtOrAfter(tithi, approx-2);
	const int tryy = fixedFromMoment(lunar);
	const double th=standardFromSundial(tryy+rd_ut, ujjain);

	if ((lunar<=th) || (hinduLunarPhase(standardFromSundial(tryy+1+rd_ut, ujjain))>12.*tithi))
		return tryy;
	else
		return tryy+1;
}
// return a QVector<int> of RDs of actually occurring tithis in a Gregorian year (CC:UE 20.59)
QVector<int> NewHinduCalendar::hinduLunarEvent(const int lMonth, const int tithi, const double rd_ut, const int gYear)
{
	const int lYear=hinduLunarFromFixed(GregorianCalendar::gregorianNewYear(gYear)).value(0);
	const int date0=hinduTithiOccur(lMonth, tithi, rd_ut, lYear);
	const int date1=hinduTithiOccur(lMonth, tithi, rd_ut, lYear+1);

	QVector<int> cand({date0, date1});
	QVector<int> range=GregorianCalendar::gregorianYearRange(gYear);
	return intersectWithRange(cand, range);
}
// return dates of shiva in Gregorian year (CC:UE 20.60)
QVector<int> NewHinduCalendar::shiva(const int gYear)
{
	return hinduLunarEvent(11, 29, 1., gYear);
}
// return dates of rama in Gregorian year (CC:UE 20.61)
QVector<int> NewHinduCalendar::rama(const int gYear)
{
	return hinduLunarEvent(1, 9, 0.5, gYear);
}

// return nakshatra for the day (CC:UE 20.62)
int NewHinduCalendar::hinduLunarStation(const int rd)
{
	const double critical=hinduSunrise(rd);
	return qRound(floor(hinduLunarLongitude(critical)/(800./60.)))+1;
}
// return karana index (CC:UE 20.63)
int NewHinduCalendar::karana(const int n)
{
	if (n==0)
		return 1;
	else if (n>57)
		return n-50;
	else
		return StelUtils::amod(n-1, 7);
}

// return karana [1...60] for day rd. According to Wikipedia (https://en.wikipedia.org/wiki/Hindu_calendar#Kara%E1%B9%87a),
// the karana at sunrise prevails for the day, but this has yet to be confirmed.
int NewHinduCalendar::karanaForDay(const int rd)
{
	const double sunrise=hinduSunrise(rd);
	int karanaCand=0;
	double latest=sunrise-30;
	for (int n=1; n<=60; n++)
	{
		double kBeg=hinduLunarDayAtOrAfter((n+1)*0.5, sunrise-30);
		if ( (kBeg<sunrise) && (kBeg>latest) )
		{
			karanaCand=n;
			latest=kBeg;
		}
	}
	return karanaCand;
}

// return yoga (CC:UE 20.64)
int NewHinduCalendar::yoga(const int rd)
{
	return 1+qRound(floor( StelUtils::fmodpos( (hinduSolarLongitude(rd)+hinduLunarLongitude(rd))/(800./60.), 27.)));
}
// return the sacred Wednesdays in a Gregorian year. (CC:UE 20.65)
QVector<int> NewHinduCalendar::sacredWednesdays(const int gYear)
{
	return sacredWednesdaysInRange(GregorianCalendar::gregorianYearRange(gYear));
}
// return the sacred Wednesdays in a certain range of RDs. (CC:UE 20.66)
QVector<int> NewHinduCalendar::sacredWednesdaysInRange(const QVector<int> range)
{
	const int a=range.value(0);
	const int b=range.value(1);
	QVector<int> res;
	int wed=kdayOnOrAfter(wednesday, a);
	QVector<int> hDate=hinduLunarFromFixed(wed);

	// we use iteration, not recursion here.
	while ((a<=wed) && (wed<=b))
	{
		if (hDate.value(3)==8)
			res.append(wed);
		wed=kdayOnOrAfter(wednesday, wed+5);
	}
	return res;
}

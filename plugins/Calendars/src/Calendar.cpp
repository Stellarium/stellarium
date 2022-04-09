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

#include <QDebug>
#include <QTimeZone>

#include "Calendar.hpp"
#include "GregorianCalendar.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelUtils.hpp"
#include "precession.h"
#include "StelModuleMgr.hpp"
#include "SolarSystem.hpp"
#include "StelLocationMgr.hpp"


QString Calendar::getFormattedDateString(QVector<int> date, QString sep)
{
	QString res;
	QVector<int>::const_iterator it;
	for (it=date.constBegin(); it!=date.constEnd(); ++it)
	{
		res.append(QString::number(*it));
		res.append(sep);
	}
	return res;
}

QString Calendar::getFormattedDateString() const
{
	return getDateStrings().join(' ');
}

// CC.UE 1.24: This EXCLUDES the upper limit!
double Calendar::modInterval(double x, double a, double b)
{
	if (fuzzyEquals(a,b)) return x;
	return a + StelUtils::fmodpos(x-a, b-a);
}

// CC.UE 1.24: This EXCLUDES the upper limit! Use StelUtils::amod(x, b) for CC's (x)mod[1..b]
int Calendar::modInterval(int x, int a, int b)
{
	if (a==b) return x;
	return a + StelUtils::imod(x-a, b-a);
}

double Calendar::momentFromJD(double jd, bool respectUTCoffset)
{
	double ret=jd+jdEpoch;
	if (respectUTCoffset)
	{
		ret+=StelApp::getInstance().getCore()->getUTCOffset(jd)/24.;
	}
	return ret;
}

double Calendar::jdFromMoment(double rd, bool respectUTCoffset)
{
	double ret=rd-jdEpoch;
	if (respectUTCoffset)
	{
		ret-=StelApp::getInstance().getCore()->getUTCOffset(ret)/24.;
	}
	return ret;
}

// Reingold-Dershowitz CC.UE 1.48
int Calendar::rdCorrSum(QVector<int>parts, QVector<int>factors, int corr)
{
	Q_ASSERT(parts.length()==factors.length());

	int sum=0;
	for (int i=0; i<parts.length(); ++i)
		sum+=(parts.at(i)+corr)*factors.at(i);
	return sum;
}

// Split integer to mixed-radix vector. Reingold-Dershowitz CC.UE 1.42
QVector<int> Calendar::toRadix(int num, QVector<int>radix)
{
	int div=1;
	int rest=num;
	QVector<int> res;
	QVector<int>::const_reverse_iterator i;
	for (i = radix.rbegin(); i != radix.rend(); ++i)
	{
		div = *i;

		int lsb=StelUtils::imod(rest, div);
		res.prepend(lsb);
		rest -= lsb;
		rest /= div;
	}
	res.prepend(rest);
	return res;
}

// Intersect a collection of candidates against a range of values.
// return a new QVector<int> with all elements from candidates which lie in range
QVector<int> Calendar::intersectWithRange(QVector<int>cand, QVector<int>range)
{
	QVector<int>res;
	for (int i=0; i<cand.length(); i++)
	{
		if ( (cand.value(i) >= range.value(0)) && (cand.value(i) <= range.value(1)) )
			res.append(cand.value(i));
	}
	return res;
}

// DEFINITIONS FROM CHAPTER 14: ASTRONOMICAL CALENDARS
// Crucial: Region MUST be full name - we cannot look up region codes in the tests!
//                                       Name        country   region               long         lat           alt    popK  Timezone          Bortle Role
const StelLocation Calendar::urbana   ("Urbana",    "Il",     "Northern America", -88.2f,        40.1f,        225,   250, "America/Chicago", 7, 'N');
const StelLocation Calendar::greenwich("Greenwich", "London", "Northern Europe",    0.f,         51.4777815f,   47, 100, "Europe/London",   7, 'O');
const StelLocation Calendar::mecca    ("MecKa",     "Mecca",  "Western Asia",      39.82333333f, 21.42333333f, 298,   200, "Asia/Riyadh", 9, 'R');
// Keep here, or use only in Persian?
const StelLocation Calendar::tehran   ( "Tehran",   "ir",     "Southern Asia",     51.42f,       35.68f,      1100,  7153, "Asia/Tehran", 9, 'C');
const StelLocation Calendar::paris    ( "Paris",    "fr",     "Western Europe",     2.34f,       48.84f,        27,  6000, "Europe/Paris", 9, 'C');
const StelLocation Calendar::jerusalem("Jerusalem", "Israel", "Western Asia",      35.24f,       31.78f,       740,  2000, "Asia/Jerusalem", 7, 'C');
const StelLocation Calendar::acre     ("Acre",      "Israel", "Western Asia",      35.09f,       32.94f,        22,    20, "Asia/Jerusalem", 6, 'N');
const StelLocation Calendar::padua    ("Padua",     "Italy",  "Southern Europe",   11.8858333f,  45.407778f,    18,   200, "Europe/Rome", 6, 'N');

StelLocation Calendar::location(const QString &name)
{
	StelLocation loc=StelApp::getInstance().getLocationMgr().locationForString(name);

	//qDebug() << "Calendar: Found location:" << loc.name << loc.ianaTimeZone << loc.longitude << "/" << loc.latitude;

	return loc;
}

double Calendar::direction(const StelLocation &locFrom, const StelLocation &locTo)
{
	// We could do that, but south azimuth may cause problems.
	//return StelLocation::getAzimuthForLocation(static_cast<double>(loc1.longitude), static_cast<double>(loc1.latitude),
	//					   static_cast<double>(loc2.longitude), static_cast<double>(loc2.latitude));
	const double longObs    = static_cast<double>(locFrom.longitude) * M_PI_180;
	const double latObs     = static_cast<double>(locFrom.latitude ) * M_PI_180;
	const double longTarget = static_cast<double>(locTo.longitude) * M_PI_180;
	const double latTarget  = static_cast<double>(locTo.latitude ) * M_PI_180;

	double az = atan2(sin(longTarget-longObs), cos(latObs)*tan(latTarget)-sin(latObs)*cos(longTarget-longObs));
	return StelUtils::fmodpos(M_180_PI * az, 360.0);
}
double Calendar::universalFromLocal(double rd_loc, const StelLocation &loc)
{
	//qDebug() << "universalFromLocal: Location " << loc.name << "longitude:" << loc.longitude;
	return rd_loc-zoneFromLongitude(static_cast<double>(loc.longitude));
}
double Calendar::localFromUniversal(double rd_ut, const StelLocation &loc)
{
	//qDebug() << "localFromUniversal: Location " << loc.name << "longitude:" << loc.longitude;
	return rd_ut+zoneFromLongitude(static_cast<double>(loc.longitude));
}
double Calendar::standardFromUniversal(double rd_ut, const StelLocation &loc)
{
	static const QDateTime j2k(QDate(2000, 1, 1), QTime(0, 0, 0), Qt::UTC);
	QTimeZone tz(loc.ianaTimeZone.toUtf8());
	//qDebug() << "standardFromUniversal: Location " << loc.name << "TZ:" << loc.ianaTimeZone << tz.standardTimeOffset(j2k);
	return rd_ut+tz.standardTimeOffset(j2k)/86400.;
}
double Calendar::universalFromStandard(double rd_zone, const StelLocation &loc)
{
	static const QDateTime j2k(QDate(2000, 1, 1), QTime(0, 0, 0), Qt::UTC);
	QTimeZone tz(loc.ianaTimeZone.toUtf8());
	//qDebug() << "universalFromStandard: Location " << loc.name << "TZ:" << loc.ianaTimeZone << tz.standardTimeOffset(j2k);
	return rd_zone-tz.standardTimeOffset(j2k)/86400.;
}
double Calendar::standardFromLocal(double rd_loc, const StelLocation &loc)
{
	return standardFromUniversal(universalFromLocal(rd_loc, loc), loc);
}
double Calendar::localFromStandard(double rd_zone, const StelLocation &loc)
{
	return localFromUniversal(universalFromStandard(rd_zone, loc), loc);
}
double Calendar::ephemerisCorrection(double rd)
{
	double jd=jdFromMoment(rd, false);
	double deltaT=StelApp::getInstance().getCore()->computeDeltaT(jd);
	return deltaT/86400.;
}
double Calendar::dynamicalFromUniversal(double rd_ut)
{
	return rd_ut+ephemerisCorrection(rd_ut);
}
double Calendar::universalFromDynamical(double rd_dt)
{
	return rd_dt-ephemerisCorrection(rd_dt);
}
// CC:UE 14.18
double Calendar::julianCenturies(double rd_ut)
{
	return (dynamicalFromUniversal(rd_ut)-j2000)/36525.;
}
double Calendar::equationOfTime(double rd_ut)
{
	// // The full solution to the equation of time requires lots of computation. We go the fast way from Meeus, AA2, 28.3, or its related version from CC:UE
	const double JDE=jdFromMoment(rd_ut, false); // actually JD.
	const double T=(JDE-2451545.0)/36525.;
	const double tau = T*0.1;
	const double epsRad=getPrecessionAngleVondrakEpsilon(JDE);
	const double e = (-0.0000001267*T-0.000042037)*T+0.016708634;
	const double M = ((-0.0001537*T+35999.05029)*T+357.52911)*M_PI_180;
	double Lo = StelUtils::fmodpos(280.4664567 + tau*(360007.6982779 + tau*(0.03032028 + tau*(1./49931. + tau*(-1./15300. - tau/2000000.)))), 360.);
	Lo *= M_PI_180;
	double y=tan(epsRad*0.5); y*=y;

	double E=y*sin(2.*Lo)+2.*e*sin(M)*(2.*y*cos(2.*Lo)-1.)-0.5*y*y*sin(4.*Lo)-1.25*e*e*sin(2.*M);
	// E in radians.
	return E*(M_1_PI*0.5);
}
// return moment (RD in local mean solar time) corrected by equation of time
double Calendar::apparentFromLocal(double rd_local_mean, const StelLocation &loc)
{
	return rd_local_mean+equationOfTime(universalFromLocal(rd_local_mean, loc));
}
// return moment (RD in local apparent solar time) corrected by equation of time into local mean solar time
double Calendar::localFromApparent(double rd_local_app, const StelLocation &loc)
{
	return rd_local_app-equationOfTime(universalFromLocal(rd_local_app, loc));
}
// return moment (RD in UT) corrected into apparent time by equation of time and location
double Calendar::apparentFromUniversal(double rd_ut, const StelLocation &loc)
{
	return apparentFromLocal(localFromUniversal(rd_ut, loc), loc);
}

// return moment (RD in local apparent solar time) corrected by equation of time into UT
double Calendar::universalFromApparent(double rd_local_app, const StelLocation &loc)
{
	return universalFromLocal(localFromApparent(rd_local_app, loc), loc);
}
// return RD (UT) of True (solar) Midnight from RD and location
double Calendar::midnight(int rd, const StelLocation &loc)
{
	return universalFromApparent(rd, loc);
}
// return RD (UT) of True (solar) Noon from RD and location
double Calendar::midday(int rd, const StelLocation &loc)
{
	return universalFromApparent(rd+0.5, loc);
}

// return sidereal time (at Greenwich) at moment rd_ut (CC:UE 14.27)
double Calendar::siderealFromMoment(double rd_ut)
{
	static StelCore *core = StelApp::getInstance().getCore();
	double jd=jdFromMoment(rd_ut, false);
	double jde=jd+core->computeDeltaT(jd)/86400.;
	return GETSTELMODULE(SolarSystem)->getEarth()->getSiderealTime(jd, jde);
}

double Calendar::obliquity(double rd_ut)
{
	static StelCore *core = StelApp::getInstance().getCore();
	double jde=jdFromMoment(rd_ut, false);
	jde+=core->computeDeltaT(jde)/86400.;
	return getPrecessionAngleVondrakEpsilon(jde)*M_180_PI;
}
double Calendar::declination(double rd_ut, double eclLat, double eclLong)
{
	static StelCore *core = StelApp::getInstance().getCore();
	double jde=jdFromMoment(rd_ut, false);
	jde+=core->computeDeltaT(jde)/86400.;
	double epsRad=getPrecessionAngleVondrakEpsilon(jde);
	double raRad, declRad;
	StelUtils::eclToEqu(eclLong*M_PI_180, eclLat*M_PI_180, epsRad, &raRad, &declRad);
	return declRad*M_180_PI;
}
double Calendar::rightAscension(double rd_ut, double eclLat, double eclLong)
{
	static StelCore *core = StelApp::getInstance().getCore();
	double jde=jdFromMoment(rd_ut, false);
	jde+=core->computeDeltaT(jde)/86400.;
	double epsRad=getPrecessionAngleVondrakEpsilon(jde);
	double raRad, declRad;
	StelUtils::eclToEqu(eclLong*M_PI_180, eclLat*M_PI_180, epsRad, &raRad, &declRad);
	return raRad*M_180_PI;
}

static const double solarXYZ[][3]={
	{ 403406, 270.54861,     0.9287892},
	{ 195207, 340.19128, 35999.1376958},
	{ 119433,  63.91854, 35999.4089666},
	{ 112392, 331.26220, 35998.7287385},
	{   3891, 317.843,   71998.20261},
	{   2819,  86.631,   71998.4403 },
	{   1721, 240.052,   36000.35726},
	{    660, 310.26,    71997.4812},
	{    350, 247.23,    32964.4678},
	{    334, 260.87,      -19.4410},
	{    314, 297.82,   445267.1117},
	{    268, 343.14,    45036.8840},
	{    242, 166.79,        3.1008},
	{    234,  81.53,    22518.4434},
	{    158,   3.50,      -19.9739},
	{    132, 132.75,    65928.9345},
	{    129, 182.95,     9038.0293},
	{    114, 162.03,     3034.7684},
	{     99,  29.8,     33718.148},
	{     93, 266.4,      3034.448},
	{     86, 249.2,     -2280.773},
	{     78, 157.6,     29929.992},
	{     72, 257.8,     31556.493},
	{     68, 185.1,       149.588},
	{     64,  69.9,      9037.750},
	{     46,   8  ,    107997.405},
	{     38, 197.1,     -4444.176},
	{     37, 250.4,       151.771},
	{     32,  65.3,     67555.316},
	{     29, 162.7,     31556.080},
	{     28, 341.5,     -4561.540},
	{     27, 291.6,    107996.706},
	{     27,  98.5,      1221.655},
	{     25, 146.7,     62894.167},
	{     24, 110  ,     31437.369},
	{     21,   5.2,     14578.298},
	{     21, 342.6,    -31931.757},
	{     20, 230.9,     34777.243},
	{     18, 256.1,      1221.999},
	{     17,  45.3,     62894.511},
	{     14, 242.9,     -4442.039},
	{     13, 115.2,    107997.909},
	{     13, 151.8,       119.066},
	{     13, 285.3,     16859.071},
	{     12,  53.3,        -4.578},
	{     10, 126.6,     26895.292},
	{     10, 205.7,       -39.127},
	{     10,  85.9,     12297.536},
	{     10, 146.1,     90073.778}};

// return solar longitude [degrees] for moment RD_UT (CC:UE 14.33)
// We cannot use Stellarium's VSOP87A solution here because we need longitude for equinox of date.
double Calendar::solarLongitude(double rd_ut)
{
/*
	static StelCore *core = StelApp::getInstance().getCore();
	double jde=jdFromMoment(rd_ut, false);
	jde+=core->computeDeltaT(jde)/86400.;
	Vec3d pos;
	Vec3d vel;
	GETSTELMODULE(SolarSystem)->getEarth()->computePosition(jde, pos, vel);
	double lng, lat;
	StelUtils::rectToSphe(&lng, &lat, pos);
	return StelUtils::fmodpos(M_180_PI*(lng+M_PI)+nutation(rd_ut)+aberration(rd_ut), 360.);
*/
	const double c=julianCenturies(rd_ut);
	double lambda=0.;
	for( int i=48; i>=0; i--)
		lambda += solarXYZ[i][0]*sin(M_PI_180*(solarXYZ[i][1]+solarXYZ[i][2]*c));
	lambda *= 0.000005729577951308232;
	lambda += StelUtils::fmodpos(36000.76953744*c, 360.) + 282.7771834;

	return StelUtils::fmodpos(lambda+nutation(rd_ut)+aberration(rd_ut), 360.);
}
// return nutation in degrees for moment rd_ut (CC:UE 14.34, but we use our own solution)
double Calendar::nutation(double rd_ut)
{
	static StelCore *core = StelApp::getInstance().getCore();
	double jde=jdFromMoment(rd_ut, false);
	jde+=core->computeDeltaT(jde)/86400.;
	double deltaPsi, deltaEps;
	getNutationAngles(jde, &deltaPsi, &deltaEps);
	return deltaPsi*M_180_PI;
}
// return aberration in degrees for moment rd_ut (CC:UE 14.35)
double Calendar::aberration(double rd_ut)
{
	double c=julianCenturies(rd_ut);
	return 0.0000974*cos((177.63+35999.01848*c)*M_PI_180)-0.005575;
}
// return moment (RD_UT) of when the sun first reaches lng [degrees] after rd_ut  (CC:UE 14.36)
double Calendar::solarLongitudeAfter(double lng, double rd_ut)
{
	static const double rate=meanTropicalYear/360.;
	double tau=rd_ut+rate*StelUtils::fmodpos(lng-solarLongitude(rd_ut), 360);
	double a=qMax(rd_ut, tau-5.);
	double b=tau+5.;
	return solarLongitudeInv(lng, a, b);
}
// binary search for the moment when solar longitude reaches lng in the time between rdA and rdB (CC:UE 14.36)
double Calendar::solarLongitudeInv(double lng, double rdA, double rdB)
{
	Q_ASSERT(lng>=0.);
	Q_ASSERT(lng<360.);
	Q_ASSERT(rdA<rdB);

	double lngA=solarLongitude(rdA);
	double lngB=solarLongitude(rdB);
	// Ensure lngB>lngA at the 360/0 discontinuity
	if (lngA>lngB)
		lngA-=360.; // we must then search around 0° for spring!
	do
	{
		double rdMid=(rdA+rdB)*0.5;
		double lngMid=solarLongitude(rdMid);
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

double Calendar::seasonInGregorian(Calendar::Season season, int gYear)
{
	return solarLongitudeAfter(season, GregorianCalendar::gregorianNewYear(gYear));
}

// return merely a test (CC:UE 14.38)
double Calendar::urbanaWinter(int gYear)
{
	return standardFromUniversal(
				solarLongitudeAfter(winter, GregorianCalendar::fixedFromGregorian({gYear, GregorianCalendar::january, 1})), urbana);
}

// return precession in ecliptical longitude since moment rd_dt (CC:UE 14.39, which is from Meeus AA2 21.6/21.7.)
double Calendar::precession(double rd_dt)
{
	double c=julianCenturies(rd_dt);
	double eta=StelUtils::fmodpos(((0.000060*c-0.03302)*c+47.0029)*c * (1./3600.), 360.);
	double Pi =StelUtils::fmodpos((0.03536*c-869.8089)*c*(1./3600.) + 174.876384, 360.);
	double p  =StelUtils::fmodpos(((-0.000006*c+1.11113)*c+5029.0966)*c * (1./3600.), 360.);
	double A=cos(eta*M_PI_180) * sin(Pi*M_PI_180);
	double B=cos(Pi*M_PI_180);
	double arg=atan2(A, B);
	return StelUtils::fmodpos(p+Pi-arg*M_180_PI, 360.);
}

// return altitude of the sun at loc, degrees
double Calendar::solarAltitude(double rd_ut, const StelLocation &loc)
{
	const double phiRad=static_cast<double>(loc.latitude)*M_PI_180;
	const double psi=static_cast<double>(loc.longitude);
	const double lambda=solarLongitude(rd_ut);

	static StelCore *core = StelApp::getInstance().getCore();
	double jde=jdFromMoment(rd_ut, false);
	jde+=core->computeDeltaT(jde)/86400.;
	double epsRad=getPrecessionAngleVondrakEpsilon(jde);
	double raRad, declRad;
	StelUtils::eclToEqu(lambda*M_PI_180, 0., epsRad, &raRad, &declRad);

	double theta0=siderealFromMoment(rd_ut);
	double capH=StelUtils::fmodpos(theta0-raRad*M_180_PI+psi, 360.); // local hour angle
	double sinAlt=sin(phiRad)*sin(declRad)+cos(phiRad)*cos(declRad)*cos(capH*M_PI_180);
	return asin(sinAlt)*M_180_PI;
}

// return an estimate for the moment (RD_UT) before rd_ut when sun has reached lambda (CC:UE 14.42)
double Calendar::estimatePriorSolarLongitude(double lambda, double rd_ut)
{
	static const double rate=meanTropicalYear/360.;
	double tau=rd_ut-rate*StelUtils::fmodpos(solarLongitude(rd_ut)-lambda, 360.);
	double Delta=modInterval(solarLongitude(tau)-lambda, -180., 180.);
	return qMin(rd_ut, tau-rate*Delta);
}

// return RD of n-th new moon after RD0. (CC:UE 14.45, following Meeus AA2, ch.49)
double Calendar::nthNewMoon(int n)
{
	const int k=(n-24724);
	const double c=k/1236.85;
	const double approx=j2000+(((0.00000000073*c-0.000000150)*c+0.00015437)*c+meanSynodicMonth*k+5.09766);
	const double E=1.-c*(0.002516+c*0.0000074);
	const double solarAnomaly=((-0.00000011*c-0.0000014)*c+1236.85*29.10535670)*c+2.5534;
	const double lunarAnomaly=(((-0.000000058*c+0.00001238)*c+0.0107582)*c+385.81693528*1236.85)*c+201.5643;
	const double moonArgument=(((0.000000011*c-0.00000227)*c-0.0016118)*c+390.67050284*1236.85)*c+160.7108;
	const double Omega=((0.00000215*c+0.0020672)*c-1.56375588*1236.85)*c+124.7746;
	const double extra=0.000325*sin(M_PI_180*( (-0.009173*c+132.8475848)*c+299.77));
	const double vwxyz[24][5]={
		{-0.40720, 0, 0, 1, 0},
		{ 0.17241, 1, 1, 0, 0},
		{ 0.01608, 0, 0, 2, 0},
		{ 0.01039, 0, 0, 0, 2},
		{ 0.00739, 1,-1, 1, 0},
		{-0.00514, 1, 1, 1, 0},
		{ 0.00208, 2, 2, 0, 0},
		{-0.00111, 0, 0, 1,-2},
		{-0.00057, 0, 0, 1, 2},
		{ 0.00056, 1, 1, 2, 0},
		{-0.00042, 0, 0, 3, 0},
		{ 0.00042, 1, 1, 0, 2},
		{ 0.00038, 1, 1, 0,-2},
		{-0.00024, 1,-1, 2, 0},
		{-0.00007, 0, 2, 1, 0},
		{ 0.00004, 0, 0, 2,-2},
		{ 0.00004, 0, 3, 0, 0},
		{ 0.00003, 0, 1, 1,-2},
		{ 0.00003, 0, 0, 2, 2},
		{-0.00003, 0, 1, 1, 2},
		{ 0.00003, 0,-1, 1, 2},
		{-0.00002, 0, 1, 3, 0},
		{-0.00002, 0, 1, 3, 0},
		{ 0.00002, 0, 0, 4, 0}};
	const double ijl[13][3]={
		{251.88,  0.016321, 0.000165},
		{251.83, 26.651886, 0.000164},
		{349.42, 36.412478, 0.000126},
		{ 84.66, 18.206239, 0.000110},
		{141.74, 53.303771, 0.000062},
		{207.14,  2.453732, 0.000060},
		{154.84,  7.306860, 0.000056},
		{ 34.52, 27.261239, 0.000047},
		{207.19,  0.121824, 0.000042},
		{291.34,  1.844379, 0.000040},
		{161.72, 24.198154, 0.000037},
		{239.56, 25.513099, 0.000035},
		{331.55,  3.592518, 0.000023}};
	double correction=0.;
	for (int i=23; i>=0; i--)
	{
		correction+=vwxyz[i][0]*pow(E, vwxyz[i][1])
				*sin(M_PI_180*(vwxyz[i][2]*solarAnomaly + vwxyz[i][3]*lunarAnomaly + vwxyz[i][4]*moonArgument ));
	}
	correction += -0.00017 * sin(Omega*M_PI_180);
	double additional=0.;
	for (int i=12; i>=0; i--)
	{
		additional+=ijl[i][2]*sin(M_PI_180*( ijl[i][0] + ijl[i][1]*k ));
	}
	return universalFromDynamical(approx+correction+extra+additional);
}

// return RD of New Moon before rd_ut (CC:UE 14.46)
double Calendar::newMoonBefore(double rd_ut)
{
	const double t0=nthNewMoon(0);
	const double phi=lunarPhase(rd_ut);
	const int n=qRound((rd_ut-t0)/meanSynodicMonth - phi/360.);
	int k=n-2;
	double rdK=nthNewMoon(k);
	double rdKnew;
	do {
		k++;
		rdKnew=nthNewMoon(k);
		if (rdKnew<rd_ut)
			rdK=rdKnew;
	}
	while (rdKnew<rd_ut);
	return rdK;
}
// return RD of New Moon on or after rd_ut (CC:UE 14.47)
double Calendar::newMoonAtOrAfter(double rd_ut)
{
	const double t0=nthNewMoon(0);
	const double phi=lunarPhase(rd_ut);
	const int n=qRound((rd_ut-t0)/meanSynodicMonth - phi/360.);
	int k=n+2;
	double rdK=nthNewMoon(k);
	double rdKnew;
	do {
		k--;
		rdKnew=nthNewMoon(k);
		if (rdKnew>rd_ut)
			rdK=rdKnew;
	}
	while (rdKnew>rd_ut);
	return rdK;
}

// Meeus AA2 table 47.A
static const double lunarLD[60][6]={
	{0, 0, 1, 0, 6288774, -20905355},
	{2, 0,-1, 0, 1274027,  -3699111},
	{2, 0, 0, 0,  658314,  -2955968},
	{0, 0, 2, 0,  213618,   -569925},
	{0, 1, 0, 0, -185116,     48888},
	{0, 0, 0, 2, -114332,     -3149},
	{2, 0,-2, 0,   58793,    246158},
	{2,-1,-1, 0,   57066,   -152138},
	{2, 0, 1, 0,   53322,   -170733},
	{2,-1, 0, 0,   45758,   -204586},
	{0, 1,-1, 0,  -40923,   -129620},
	{1, 0, 0, 0,  -34720,    108743},
	{0, 1, 1, 0,  -30383,    104755},
	{2, 0, 0,-2,   15327,     10321},
	{0, 0, 1, 2,  -12528,         0},
	{0, 0, 1,-2,   10980,     79661},
	{4, 0,-1, 0,   10675,    -34782},
	{0, 0, 3, 0,   10034,    -23210},
	{4, 0,-2, 0,    8548,    -21636},
	{2, 1,-1, 0,   -7888,     24208},
	{2, 1, 0, 0,   -6766,     30824},
	{1, 0,-1, 0,   -5163,     -8379},
	{1, 1, 0, 0,    4987,    -16675},
	{2,-1, 1, 0,    4036,    -12831},
	{2, 0, 2, 0,    3994,    -10445},
	{4, 0, 0, 0,    3861,    -11650},
	{2, 0,-3, 0,    3665,     14403},
	{0, 1,-2, 0,   -2689,     -7003},
	{2, 0,-1, 2,   -2602,         0},
	{2,-1,-2, 0,    2390,     10056},
	{1, 0, 1, 0,   -2348,      6322},
	{2,-2, 0, 0,    2236,     -9884},
	{0, 1, 2, 0,   -2120,      5751},
	{0, 2, 0, 0,   -2069,         0},
	{2,-2,-1, 0,    2048,     -4950},
	{2, 0, 1,-2,   -1773,      4130},
	{2, 0, 0, 2,   -1595,         0},
	{4,-1,-1, 0,    1215,     -3958},
	{0, 0, 2, 2,   -1110,         0},
	{3, 0,-1, 0,    -892,      3258},
	{2, 1, 1, 0,    -810,      2616},
	{4,-1,-2, 0,     759,     -1897},
	{0, 2,-1, 0,    -713,     -2117},
	{2, 2,-1, 0,    -700,      2354},
	{2, 1,-2, 0,     691,         0},
	{2,-1, 0,-2,     596,         0},
	{4, 0, 1, 0,     549,     -1423},
	{0, 0, 4, 0,     537,     -1117},
	{4,-1, 0, 0,     520,     -1571},
	{1, 0,-2, 0,    -487,     -1739},
	{2, 1, 0,-2,    -399,         0},
	{0, 0, 2,-2,    -381,     -4421},
	{1, 1, 1, 0,     351,         0},
	{3, 0,-2, 0,    -340,         0},
	{4, 0,-3, 0,     330,         0},
	{2,-1, 2, 0,     327,         0},
	{0, 2, 1, 0,    -323,      1165},
	{1, 1,-1, 0,     299,         0},
	{2, 0, 3, 0,     294,         0},
	{2, 0,-1,-2,       0,      8752}};
// Meeus AA2 table 47.B
static const double lunarB[60][5]={
	{0, 0, 0, 1, 5128122},
	{0, 0, 1, 1,  280602},
	{0, 0, 1,-1,  277693},
	{2, 0, 0,-1,  173237},
	{2, 0,-1, 1,   55413},
	{2, 0,-1,-1,   46271},
	{2, 0, 0, 1,   32573},
	{0, 0, 2, 1,   17198},
	{2, 0, 1,-1,    9266},
	{0, 0, 2,-1,    8822},
	{2,-1, 0,-1,    8216},
	{2, 0,-2,-1,    4324},
	{2, 0, 1, 1,    4200},
	{2, 1, 0,-1,   -3359},
	{2,-1,-1, 1,    2463},
	{2,-1, 0, 1,    2211},
	{2,-1,-1,-1,    2065},
	{0, 1,-1,-1,   -1870},
	{4, 0,-1,-1,    1828},
	{0, 1, 0, 1,   -1794},
	{0, 0, 0, 3,   -1749},
	{0, 1,-1, 1,   -1565},
	{1, 0, 0, 1,   -1491},
	{0, 1, 1, 1,   -1475},
	{0, 1, 1,-1,   -1410},
	{0, 1, 0,-1,   -1344},
	{1, 0, 0,-1,   -1335},
	{0, 0, 3, 1,    1107},
	{4, 0, 0,-1,    1021},
	{4, 0,-1, 1,     833},
	{0, 0, 1,-3,     777},
	{4, 0,-2, 1,     671},
	{2, 0, 0,-3,     607},
	{2, 0, 2,-1,     596},
	{2,-1, 1,-1,     491},
	{2, 0,-2, 1,    -451},
	{0, 0, 3,-1,     439},
	{2, 0, 2, 1,     422},
	{2, 0,-3,-1,     421},
	{2, 1,-1, 1,    -366},
	{2, 1, 0, 1,    -351},
	{4, 0, 0, 1,     331},
	{2,-1, 1, 1,     315},
	{2,-2, 0,-1,     302},
	{0, 0, 1, 3,    -283},
	{2, 1, 1,-1,    -229},
	{1, 1, 0,-1,     223},
	{1, 1, 0, 1,     223},
	{0, 1,-2,-1,    -220},
	{2, 1,-1,-1,    -220},
	{1, 0, 1, 1,    -185},
	{2,-1,-2,-1,     181},
	{0, 1, 2, 1,    -177},
	{4, 0,-2,-1,     176},
	{4,-1,-1,-1,     166},
	{1, 0, 1,-1,    -164},
	{4, 0, 1,-1,     132},
	{1, 0,-1,-1,    -119},
	{4,-1, 0,-1,     115},
	{2,-2, 0, 1,     107}};

// return lunar longitude at rd_ut (CC:UE 14.48, actually Meeus AA2, ch.47)
double Calendar::lunarLongitude(double rd_ut)
{
	const double c=julianCenturies(rd_ut);
	const double LP=meanLunarLongitude(c);
	const double D=lunarElongation(c);
	const double M=solarAnomaly(c);
	const double MP=lunarAnomaly(c);
	const double F=moonNode(c);
	const double E=(-0.0000074*c-0.002516)*c+1.;
	double correction=0.;
	for (int i=59; i>=0; i--)
	{
		correction+=lunarLD[i][4]*pow(E, fabs(lunarLD[i][1]))*sin(M_PI_180*(lunarLD[i][0]*D + lunarLD[i][1]*M + lunarLD[i][2]*MP + lunarLD[i][3]*F ));
	}
	const double venus  = 3958.   * sin(M_PI_180*(119.75 +    131.849*c));
	const double jupiter=  318.   * sin(M_PI_180*( 53.09 + 479264.290*c));
	const double flatEarth = 1962.* sin(M_PI_180*(LP-F));

	correction+=venus+jupiter+flatEarth;
	return StelUtils::fmodpos(LP + correction*1.e-6 +nutation(rd_ut), 360.);
}
// return lunar latitude at rd_ut (CC:UE 14.63, actually Meeus AA2, ch.47)
double Calendar::lunarLatitude(double rd_ut)
{
	const double c=julianCenturies(rd_ut);
	const double LP=meanLunarLongitude(c);
	const double D=lunarElongation(c);
	const double M=solarAnomaly(c);
	const double MP=lunarAnomaly(c);
	const double F=moonNode(c);
	const double E=(-0.0000074*c-0.002516)*c+1.;
	const double A1=119.75 +    131.849*c;
	const double A3=313.45 + 481266.484*c;
	double beta=0.;
	for (int i=59; i>=0; i--)
	{
		beta+=lunarB[i][4]*pow(E, fabs(lunarB[i][1]))*sin(M_PI_180*(lunarB[i][0]*D + lunarB[i][1]*M + lunarB[i][2]*MP + lunarB[i][3]*F ));
	}
	beta += -2235. * sin(M_PI_180*LP)
		 +382. * sin(M_PI_180 * A3)
		 +175. * sin(M_PI_180*(A1-F))
		 +175. * sin(M_PI_180*(A1+F))
		 +127. * sin(M_PI_180*(LP-MP))
		 -115. * sin(M_PI_180*(LP+MP));
	return beta*1e-6;
}

// return lunar distance [metres] at rd_ut (CC:UE 14.65, actually Meeus AA2 ch.47)
double Calendar::lunarDistance(double rd_ut)
{
	const double c=julianCenturies(rd_ut);
	const double D=lunarElongation(c);
	const double M=solarAnomaly(c);
	const double MP=lunarAnomaly(c);
	const double F=moonNode(c);
	const double E=(-0.0000074*c-0.002516)*c+1.;
	double correction=0.;
	for (int i=59; i>=0; i--)
	{
		correction+=lunarLD[i][5]*pow(E, fabs(lunarLD[i][1]))*cos(M_PI_180*(lunarLD[i][0]*D + lunarLD[i][1]*M + lunarLD[i][2]*MP + lunarLD[i][3]*F ));
	}
	return 385000560. + correction;
}

// return mean lunar longitude at Julian century c (CC:UE 14.49)
double Calendar::meanLunarLongitude(double c)
{
	return StelUtils::fmodpos((((-1./65194000.*c+1./538841.)*c-0.0015786)*c+481267.88123421)*c+218.3164477, 360.);
}
// return lunar elongation at Julian century c (CC:UE 14.50)
double Calendar::lunarElongation(double c)
{
	return StelUtils::fmodpos((((-1./113065000.*c+1./545868.)*c-0.0018819)*c+445267.1114034)*c+297.8501921, 360.);
}
// return solar anomaly at Julian century c (CC:UE 14.51)
double Calendar::solarAnomaly(double c)
{
	return StelUtils::fmodpos(((1./24490000.*c-0.0001536)*c+35999.0502909)*c+357.5291092, 360.);
}
// return lunar anomaly at Julian century c (CC:UE 14.52)
double Calendar::lunarAnomaly(double c)
{
	return StelUtils::fmodpos((((-1./14712000.*c+1./69699.)*c+0.0087414)*c+477198.8675055)*c+134.9633964, 360.);
}
// return longitude of lunar node at Julian century c (CC:UE 14.53)
double Calendar::moonNode(double c)
{
	return StelUtils::fmodpos((((1./863310000.*c-1./3526000.)*c-0.0036539)*c+483202.0175233)*c+93.2720950, 360.);
}
// return longitude of lunar node at moment rd_ut (CC:UE 14.54)
// note this is shifted into [-180...180)
double Calendar::lunarNode(double rd_ut)
{
	return modInterval(moonNode(julianCenturies(rd_ut)), -180., 180.);
}
// sidereal lunar longitude counted from some other point (CC:UE 14.55)
double Calendar::siderealLunarLongitude(double rd_ut, double siderealStart)
{
	return StelUtils::fmodpos(lunarLongitude(rd_ut)-precession(rd_ut)+siderealStart, 360.);
}
// return lunar phase (angular difference in ecliptical longitude from the sun)
// at moment rd_ut (CC:UE 14.56)
double Calendar::lunarPhase(double rd_ut)
{
	static const double t0=nthNewMoon(0);
	const double phi=StelUtils::fmodpos(lunarLongitude(rd_ut)-solarLongitude(rd_ut), 360.);
	const int n=qRound((rd_ut-t0)/meanSynodicMonth);
	const double phiP=360.*StelUtils::fmodpos((rd_ut-nthNewMoon(n))/meanSynodicMonth, 1.);
	return (fabs(phi-phiP)>180.) ? phiP : phi;
}

// binary search for the moment when solar longitude reaches lng in the time between rdA and rdB (CC:UE 14.36)
double Calendar::lunarPhaseInv(double phi, double rdA, double rdB)
{
	Q_ASSERT(phi>=0.);
	Q_ASSERT(phi<360.);
	Q_ASSERT(rdA<rdB);

	double phiA=lunarPhase(rdA);
	double phiB=lunarPhase(rdB);
	// Ensure lngB>lngA at the 360/0 discontinuity
	if (phiA>phiB)
		phiA-=360.; // we must then search around 0° for spring!
	do
	{
		double rdMid=(rdA+rdB)*0.5;
		double phiMid=lunarPhase(rdMid);
		// in case lngA<0 and lngB>0!
		if ((phiMid>phiA) && (phiMid>phiB))
			phiMid-=360.;
		Q_ASSERT(phiA<=phiMid);
		Q_ASSERT(phiMid<=phiB);

		if (phiMid<phi)
			rdA=rdMid;
		else
			rdB=rdMid;
	}
	while (rdB-rdA > 1e-5);
	return (rdA+rdB)*0.5;
}

// return rd of moment when phase was phi [degrees] before rd_ut (CC:UE 14.57)
double Calendar::lunarPhaseAtOrBefore(double phi, double rd_ut)
{
	double tau=rd_ut-meanSynodicMonth/360.*StelUtils::fmodpos(lunarPhase(rd_ut)-phi, 360.);
	double rdA=tau-2;
	double rdB=qMin(rd_ut, tau+2.);
	return lunarPhaseInv(phi, rdA, rdB);
}
// return rd of moment when phase will next be phi [degrees] after rd_ut (CC:UE 14.58)
double Calendar::lunarPhaseAtOrAfter(double phi, double rd_ut)
{
	double tau=rd_ut+meanSynodicMonth/360.*StelUtils::fmodpos(phi-lunarPhase(rd_ut), 360.);
	double rdA=qMax(rd_ut, tau-2);
	double rdB=tau+2.;
	return lunarPhaseInv(phi, rdA, rdB);
}

// return altitude of the moon at loc, degrees (CC:UE 14:64)
double Calendar::lunarAltitude(double rd_ut, const StelLocation &loc)
{
	double phiRad=static_cast<double>(loc.latitude)*M_PI_180;
	double psi=static_cast<double>(loc.longitude);
	double lambda=lunarLongitude(rd_ut);
	double beta=lunarLatitude(rd_ut);

	static StelCore *core = StelApp::getInstance().getCore();
	double jde=jdFromMoment(rd_ut, false);
	jde+=core->computeDeltaT(jde)/86400.;
	double epsRad=getPrecessionAngleVondrakEpsilon(jde);
	double raRad, declRad;
	StelUtils::eclToEqu(lambda*M_PI_180, beta*M_PI_180, epsRad, &raRad, &declRad);

	double theta0=siderealFromMoment(rd_ut);
	double H=StelUtils::fmodpos(theta0-raRad*M_180_PI+psi, 360.); // local hour angle
	double sinAlt=sin(phiRad)*sin(declRad)+cos(phiRad)*cos(declRad)*cos(H*M_PI_180);
	return M_180_PI*asin(sinAlt);
}

// return lunar parallax [degrees] at RD rd_ut. (CC:UE 14.66)
double Calendar::lunarParallax(double rd_ut, const StelLocation &loc)
{
	const double geo=lunarAltitude(rd_ut, loc);
	const double Delta=lunarDistance(rd_ut);
	const double alt=6378140./Delta;
	const double arg=alt*cos(M_PI_180*geo);
	return M_180_PI*asin(arg);
}
// return altitude of the moon at loc, degrees. (CC:UE 14.67)
double Calendar::topocentricLunarAltitude(double rd_ut, const StelLocation &loc)
{
	return lunarAltitude(rd_ut, loc)-lunarParallax(rd_ut, loc);
}

// 14.7 Rising and setting
// return rd of , rd. (CC:UE 14.68)
double Calendar::approxMomentOfDepression(double rd_loc, double alpha, bool early, const StelLocation &loc)
{
	const double trie=sineOffset(rd_loc, alpha, loc);
	const int date=fixedFromMoment(rd_loc);
	double alt=date+0.5;
	if (alpha>=0)
		alt= early ? date : date+1;
	const double value=fabs(trie)>1. ? sineOffset(alt, alpha, loc) : trie;
	const double offset=modInterval(asin(value)/(2.*M_PI), -12., 12.);

	if (fabs(value)<=1.)
	{
		if (early)
			return localFromApparent(date+0.25-offset, loc);
		else
			return localFromApparent(date+0.75+offset, loc);
	}
	else
		return bogus;
}
// return sine of ... (CC:UE 14.69)
double Calendar::sineOffset(double rd_loc, double alpha, const StelLocation &loc)
{
	const double phiRad=static_cast<double>(loc.latitude)*M_PI_180;
	const double tP=universalFromLocal(rd_loc, loc);
	const double deltaRad=M_PI_180*declination(tP, 0., solarLongitude(tP));
	return tan(phiRad)*tan(deltaRad)+sin(alpha*M_PI_180)/(cos(deltaRad)*cos(phiRad));
}
// return rd of , rd. (CC:UE 14.70)
double Calendar::momentOfDepression(double rd_approx, double alpha, bool early, const StelLocation &loc)
{
	const double t=approxMomentOfDepression(rd_approx, alpha, early, loc);
	if (qFuzzyCompare(t,bogus))
		return bogus;
	if (fabs(rd_approx-t)<30./86400.)
		return t;
	else
		return momentOfDepression(t, alpha, early, loc);
}

// return fraction of day (Zone Time!) of when sun is alpha degrees below horizon in the morning (or bogus) (CC:UE 14.72)
double Calendar::dawn(int rd, double alpha, const StelLocation &loc)
{
	const double result=momentOfDepression(rd+0.25, alpha, morning, loc);
	return qFuzzyCompare(result, bogus) ? bogus : standardFromLocal(result, loc);
}
// return fraction of day (Zone Time!) of when sun is alpha degrees below horizon in the evening (or bogus) (CC:UE 14.74)
double Calendar::dusk(int rd, double alpha, const StelLocation &loc)
{
	const double result=momentOfDepression(rd+0.75, alpha, evening, loc);
	return qFuzzyCompare(result, bogus) ? bogus : standardFromLocal(result, loc);
}

// return refraction at the mathematical horizon of location loc (CC:UE 14.75)
// note in the book a first argument t is specified but unused.
double Calendar::refraction(const StelLocation &loc)
{
	static const double R=6.372e6;
	const double h=qMax(0, loc.altitude);
	const double dip=acos(R/(R+h))*M_180_PI;
	return 34./60.+dip+19./3600.*sqrt(h);
}
// return fraction of day for the moment of sunrise (CC:UE 14.76)
double Calendar::sunrise(int rd, const StelLocation &loc)
{
	return dawn(rd, refraction(loc)+16./60., loc);
}
// return fraction of day for the moment of sunset (CC:UE 14.77)
double Calendar::sunset(int rd, const StelLocation &loc)
{
	return dusk(rd, refraction(loc)+16./60., loc);
}

// return apparent altitude of moon (CC:UE 14.83)
double Calendar::observedLunarAltitude(double rd_ut, const StelLocation &loc)
{
	return topocentricLunarAltitude(rd_ut, loc) + refraction(loc) + 16./60.;
}
// return moment of moonrise (CC:UE 14.83)
double Calendar::moonrise(int rd, const StelLocation &loc)
{
	const double t=universalFromStandard(rd, loc);
	const bool waning=lunarPhase(t) > 180.;
	const double alt=observedLunarAltitude(t, loc);
	const double lat=static_cast<double>(loc.latitude);
	const double offset=alt/(4.*(90.-fabs(lat)));

	static const QDateTime j2k(QDate(2000, 1, 1), QTime(0, 0, 0), Qt::UTC);
	QTimeZone tz(loc.ianaTimeZone.toUtf8());
	//qDebug() << "moonrise for: Location " << loc.name << "TZ:" << loc.ianaTimeZone << tz.standardTimeOffset(j2k);


	double approx = t + 0.5 + offset;
	if (waning)
		approx = offset>0. ? t+1.-offset : t-offset;

	// apply binary search to identify moonrise
	// TBD: It seems we can actually omit storing the altitudes when we can guarantee rdA and rdB enclose the event.
	double rdA=approx-0.25;
	double rdB=approx+0.25;
	double altA=observedLunarAltitude(rdA, loc);
	double altB=observedLunarAltitude(rdB, loc);
	Q_ASSERT(altA<altB);
	Q_ASSERT(altA<0.);
	Q_ASSERT(0.<altB);
	while (rdB-rdA > 1./(60.*24.)) // search to within 1 minute
	{
		double rdMid=(rdA+rdB)*0.5;
		double altMid=observedLunarAltitude(rdMid, loc);
		if (altMid<0.)
		{
			rdA=rdMid;
			altA=altMid;
		}
		else
		{
			rdB=rdMid;
			altB=altMid;
		}
	}
	return (rdA+rdB)*0.5;
}
// return moment of moonset (CC:UE 14.84)
double Calendar::moonset(int rd, const StelLocation &loc)
{
	const double t=universalFromStandard(rd, loc);
	const bool waxing=lunarPhase(t) < 180.;
	const double alt=observedLunarAltitude(t, loc);
	const double lat=static_cast<double>(loc.latitude);
	const double offset=alt/(4.*(90.-fabs(lat)));
	double approx = t + 0.5 - offset;
	if (waxing)
		approx = offset>0. ? t+offset : t+1.+offset;

	// apply binary search to identify moonset
	// TBD: It seems we can actually omit storing the altitudes when we can guarantee rdA and rdB enclose the event.
	double rdA=approx-0.25;
	double rdB=approx+0.25;
	double altA=observedLunarAltitude(rdA, loc);
	double altB=observedLunarAltitude(rdB, loc);
	Q_ASSERT(altA<altB);
	Q_ASSERT(altA>0.);
	Q_ASSERT(0.>altB);
	while (rdB-rdA > 1./(60.*24.)) // search to within 1 minute
	{
		double rdMid=(rdA+rdB)*0.5;
		double altMid=observedLunarAltitude(rdMid, loc);
		if (altMid>0.)
		{
			rdA=rdMid;
			altA=altMid;
		}
		else
		{
			rdB=rdMid;
			altB=altMid;
		}
	}
	return (rdA+rdB)*0.5;
}

// 14.8 Times of Day
// return local time of the start of Italian hour counting, a half hour after sunset (fraction of day)
// note This extends CC:UE 14.86 by allowing another location. Use Calendar::padua for the default solution
double Calendar::localZeroItalianHour(double rd_loc, const StelLocation &loc)
{
	int date=fixedFromMoment(rd_loc);
	return localFromStandard(dusk(date, 16./60., loc)+0.5/24., loc);
}
// return local time of the start of Italian-style hour counting, but starting at sunset (fraction of day)
// note This extends CC:UE 14.86 by allowing another location and keeping the sunset time.
double Calendar::localZeroSunsetHour(double rd_loc, const StelLocation &loc)
{
	int date=fixedFromMoment(rd_loc);
	return localFromStandard(dusk(date, 16./60., loc), loc);
}
// return local time of the start of Babylonian hour counting with sunrise (fraction of day)
// note This extends CC:UE 14.86 by allowing another location and showing the reverse counting.
double Calendar::localZeroBabylonianHour(double rd_loc, const StelLocation &loc)
{
	int date=fixedFromMoment(rd_loc);
	return localFromStandard(dawn(date, 16./60., loc), loc);
}
// return local time from Italian time (CC:UE 14.87)
double Calendar::localFromItalian(double rd_loc, const StelLocation &loc)
{
	int date=fixedFromMoment(rd_loc);
	double z=localZeroItalianHour(rd_loc-1, loc);
	return rd_loc-date+z;
}
// return local time from Sunset time (extending idea from CC:UE 14.87)
double Calendar::localFromSunsetHour(double rd_loc, const StelLocation &loc)
{
	int date=fixedFromMoment(rd_loc);
	double z=localZeroSunsetHour(rd_loc-1, loc);
	return rd_loc-date+z;
}
// return local time from Babylonian time (extending idea from CC:UE 14.87)
double Calendar::localFromBabylonian(double rd_loc, const StelLocation &loc)
{
	int date=fixedFromMoment(rd_loc);
	double z=localZeroBabylonianHour(rd_loc-1, loc);
	return rd_loc-date+z;
}
// return Italian time from local time (CC:UE 14.88)
double Calendar::italianFromLocal(double rd_loc, const StelLocation &loc)
{
	int date=fixedFromMoment(rd_loc);
	double z0=localZeroItalianHour(rd_loc-1., loc);
	double z=localZeroItalianHour(rd_loc, loc);
	if (rd_loc>z)
		return rd_loc+date-z;
	else
		return rd_loc+date-z0;
}
// return Sunset time from local time (extending idea from CC:UE 14.88)
double Calendar::sunsetHourFromLocal(double rd_loc, const StelLocation &loc)
{
	int date=fixedFromMoment(rd_loc);
	double z0=localZeroSunsetHour(rd_loc-1., loc);
	double z=localZeroSunsetHour(rd_loc, loc);
	if (rd_loc>z)
		return rd_loc+date-z;
	else
		return rd_loc+date-z0;
}
// return Babylonian time from local time (extending idea from CC:UE 14.88)
double Calendar::babylonianFromLocal(double rd_loc, const StelLocation &loc)
{
	int date=fixedFromMoment(rd_loc);
	double z=localZeroBabylonianHour(rd_loc, loc);
	double z1=localZeroBabylonianHour(rd_loc+1., loc);
	if (rd_loc<z)
		return rd_loc+date-z;
	else
		return rd_loc+date-z1;
}

// return length of a temporal day hour at date rd and location loc. (CC:UE 14.89)
double Calendar::daytimeTemporalHour(const int rd, const StelLocation &loc)
{
	if (qFuzzyCompare(sunrise(rd, loc), bogus ) || qFuzzyCompare(sunset(rd, loc), bogus))
		return bogus;
	return (1./12.) * (sunset(rd, loc)-sunrise(rd, loc));
}
// return length of a temporal night hour at date rd and location loc.  (CC:UE 14.90)
double Calendar::nighttimeTemporalHour(const int rd, const StelLocation &loc)
{
	if (qFuzzyCompare(sunrise(rd+1, loc), bogus ) || qFuzzyCompare(sunset(rd, loc), bogus))
		return bogus;
	return (1./12.) * (sunrise(rd+1, loc)-sunset(rd, loc));
}
// return standard time from "temporal" sundial time (CC:UE 14.91)
double Calendar::standardFromSundial(const double rd_ut, const StelLocation &loc)
{
	const int date=fixedFromMoment(rd_ut);
	const double hour=24.*timeFromMoment(rd_ut);
	double h; // duration of a temporal hour
	if ((6.<=hour) && (hour<=18.))
		h=daytimeTemporalHour(date, loc);
	else if (hour<6)
		h=nighttimeTemporalHour(date-1, loc);
	else
		h=nighttimeTemporalHour(date, loc);

	if (qFuzzyCompare(h, bogus))
		return bogus;
	else if ((6<=hour) && (hour<=18))
		return sunrise(date, loc)+(hour-6.)*h;
	else if (hour<6)
		return sunset(date-1, loc)+(hour+6)*h;
	else
		return sunset(date, loc)+(hour-18)*h;
}

// 14.9 Lunar Crescent Visibility
// return elongation of the Moon (CC:UE 14.95)
double Calendar::arcOfLight(double rd)
{
	return M_180_PI*acos(cos(M_PI_180*lunarLatitude(rd)) * cos(M_PI_180*lunarPhase(rd)));
}
// return rd_ut of good lunar visibility (CC:UE 14.96)
double Calendar::simpleBestView(int rd, const StelLocation &loc)
{
	double dark=dusk(rd, 4.5, loc);
	double best= qFuzzyCompare(dark,bogus) ? rd+1 : dark;
	return universalFromStandard(best, loc);
}

// return true or false according to Shaukat (CC:UE 14.97)
bool Calendar::shaukatCriterion(int rd, const StelLocation &loc)
{
	const double t=simpleBestView(rd-1, loc);
	const double phase=lunarPhase(t);
	const double h=lunarAltitude(t, loc);
	const double ARCL=arcOfLight(t);
	bool crit = (static_cast<double>(newMoon) < phase)
			&& ( phase < static_cast<double>(firstQuarter))
			&& (10.6 <= ARCL)
			&& (ARCL <= 90)
			&& (h > 4.1);
	return crit;
}
double Calendar::arcOfVision(double rd_loc, const StelLocation &loc)
{
	return lunarAltitude(rd_loc, loc) - solarAltitude(rd_loc, loc);
}
double Calendar::bruinBestView(int rd, const StelLocation &loc)
{
	const double sun = sunset(rd, loc);
	const double moon = moonset(rd, loc);
	double best=5./9.*sun + 4./9.*moon;
	if ( qFuzzyCompare(sun, bogus) || qFuzzyCompare(moon, bogus) )
		best=rd+1;
	return universalFromStandard(best, loc);
}
// return true or false according to Yallop (CC:UE 14.100)
bool Calendar::yallopCriterion(int rd, const StelLocation &loc)
{
	static const double e=-0.14;
	const double t = bruinBestView(rd-1, loc);
	const double phase = lunarPhase(t);
	const double D=lunarSemiDiameter(t, loc);
	const double ARCL=arcOfLight(t);
	const double W=D*(1-cos(ARCL*M_PI_180));
	const double ARCV=arcOfVision(t, loc);
	const double q1=11.8371-6.3226*W+0.7319*W*W-0.1018*W*W*W;
	bool crit = (static_cast<double>(newMoon) < phase)
			&& ( phase < static_cast<double>(firstQuarter))
			&& (ARCV > q1+e);
	return crit;
}
double Calendar::lunarSemiDiameter(double rd_loc, const StelLocation &loc)
{
	const double h=lunarAltitude(rd_loc, loc);
	const double p=lunarParallax(rd_loc, loc);
	return 0.27245*p*(sin(h*M_PI_180)*sin(p*M_PI_180)+1.);
}
double Calendar::lunarDiameter(double rd_ut)
{
	return 1792367000/(9*lunarDistance(rd_ut));
}
// return previous date of first crescent (CC:UE 14.104)
int Calendar::phasisOnOrBefore(int rd, const StelLocation &loc)
{
	const int moon=fixedFromMoment(lunarPhaseAtOrBefore(newMoon, rd));
	const int age=rd-moon;
	int tau=moon;
	if ((age<=3) && !visibleCrescent(rd, loc))
		tau -= 30;

	int d=tau-1;
	bool crit;
	do {
		d+=1;
		crit=visibleCrescent(d, loc);
	} while(!crit);
	return d;
}
// return next date of first crescent (CC:UE 14.105)
int Calendar::phasisOnOrAfter(int rd, const StelLocation &loc)
{
	const int moon=fixedFromMoment(lunarPhaseAtOrBefore(newMoon, rd));
	const int age=rd-moon;
	int tau=rd;
	if ((age>=4.) || visibleCrescent(rd-1, loc))
		tau += 29;

	int d=tau-1;
	bool crit;
	do {
		d+=1;
		crit=visibleCrescent(d, loc);
	} while(!crit);
	return d;
}

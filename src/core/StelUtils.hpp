/*
 * Stellarium
 * Copyright (C) 2002 Fabien Chereau
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

#ifndef STELUTILS_HPP
#define STELUTILS_HPP

#include <cmath>
#include "VecMath.hpp"

#include <QVariantMap>
#include <QDateTime>
#include <QString>

// astronomical unit (km)
#define AU 149597870.691
#define AUf 149597870.691f
#define AU_KM (1.0/149597870.691)
#define AU_KMf (1.0f/149597870.691f)
// Parsec (km)
#define PARSEC 30.857e12
// speed of light (km/sec)
#define SPEED_OF_LIGHT 299792.458
// Ecliptic obliquity of J2000.0, degrees
#define EPS_0 23.4392803055555555555556

// Add a few frequently used extra math-type literals
#ifndef M_PI_180
	#define M_PI_180    (M_PI/180.)
#endif
#ifndef M_180_PI
	#define M_180_PI    (180./M_PI)
#endif
// Add some math literals in float version to avoid many static_casts
#ifndef M_PIf
	#define M_PIf       3.14159265358979323846f   // pi
#endif
#ifndef M_PI_2f
	#define M_PI_2f     1.57079632679489661923f   // pi/2
#endif
#ifndef M_PI_4f
	#define M_PI_4f     0.785398163397448309616f  // pi/4
#endif
#ifndef M_1_PIf
	#define M_1_PIf     0.318309886183790671538f  // 1/pi
#endif
#ifndef M_2_PIf
	#define M_2_PIf     0.636619772367581343076f  // 2/pi
#endif
#ifndef M_PI_180f
	#define M_PI_180f   (M_PIf/180.f)
#endif
#ifndef M_180_PIf
	#define M_180_PIf   (180.f/M_PIf)
#endif

#define stelpow10f(x) std::exp((x) * 2.3025850930f)

//! @namespace StelUtils contains general purpose utility functions.
namespace StelUtils
{
	//! Return the full name of stellarium, i.e. "stellarium 0.9.0"
	QString getApplicationName();

	//! Return the version of stellarium, i.e. "0.9.0"
	QString getApplicationVersion();

	//! Return the name and the version of operating system, i.e. "Mac OS X 10.7"
	QString getOperatingSystemInfo();

	//! Return the user agent name, i.e. "Stellarium/0.15.0 (Linux)"
	QString getUserAgentString();

	inline const QString getEndLineChar() {
		#ifdef Q_OS_WIN
		const QString stelEndl="\r\n";
		#else
		const QString stelEndl="\n";
		#endif
		return stelEndl;
	}

	//! Convert hours, minutes, seconds to decimal hours
	inline double hmsToHours(const unsigned int h, const unsigned int m, const double s){
		return static_cast<double>(h)+static_cast<double>(m)/60.+s/3600.;
	}

	//! Convert an angle in hms format to radian.
	//! @param h hour component
	//! @param m minute component
	//! @param s second component
	//! @return angle in radian
	inline double hmsToRad(const unsigned int h, const unsigned int m, const double s){
		return hmsToHours(h, m, s)*M_PI/12.;
	}

	//! Convert an angle in +-dms format to radian.
	//! @param d degree component
	//! @param m arcmin component
	//! @param s arcsec component
	//! @return angle in radian
	inline double dmsToRad(const int d, const unsigned int m, const double s){
		double rad = M_PI_180*qAbs(d)+M_PI/10800.*m+s*M_PI/648000.;
		return (d<0 ? -rad : rad);
	}

	//! Convert an angle in radian to hms format.
	//! @param rad input angle in radian
	//! @param h hour component
	//! @param m minute component
	//! @param s second component
	void radToHms(double rad, unsigned int& h, unsigned int& m, double& s);

	//! Convert an angle in radian to +-dms format.
	//! @param rad input angle in radian
	//! @param sign true if positive, false otherwise
	//! @param d degree component
	//! @param m minute component
	//! @param s second component
	void radToDms(double rad, bool& sign, unsigned int& d, unsigned int& m, double& s);

	//! Convert an angle in radian to decimal degree.
	//! @param rad input angle in radian
	//! @param sign true if positive, false otherwise
	//! @param deg decimal degree
	void radToDecDeg(double rad, bool& sign, double& deg);

	//! Convert an angle in radian to a decimal degree string.
	//! @param angle input angle in radian
	//! @param precision
	//! @param useD Define if letter "d" must be used instead of deg sign
	//! @param useC Define if function should use 0-360 degrees
	QString radToDecDegStr(const double angle, const int precision = 4, const bool useD=false, const bool useC=false);

	//! Convert an angle in radian to a hms formatted string.
	//! If the second, minute part is == 0, it is not output
	//! @param angle input angle in radian
	QString radToHmsStrAdapt(const double angle);

	//! Convert an angle in radian to a hms formatted string.
	//! @param angle input angle in radian
	//! @param decimal output decimal second value
	QString radToHmsStr(const double angle, const bool decimal=false);

	//! Convert an angle in radian to a dms formatted string.
	//! If the second, minute part is == 0, it is not output
	//! @param angle input angle in radian
	//! @param useD Define if letter "d" must be used instead of deg sign
	QString radToDmsStrAdapt(const double angle, const bool useD=false);

	//! Convert an angle in radian to a dms formatted string.
	//! @param angle input angle in radian	
	//! @param decimal output second value with decimal fraction
	//! @param useD Define if letter "d" must be used instead of deg sign
	QString radToDmsStr(const double angle, const bool decimal=false, const bool useD=false);

	//! Convert an angle in radian to a dms formatted string.
	//! @param angle input angle in radian
	//! @param precision
	//! @param useD Define if letter "d" must be used instead of deg sign
	QString radToDmsPStr(const double angle, const int precision = 0, const bool useD=false);

	//! Convert an angle in decimal degree to +-dms format.
	//! @param angle input angle in decimal degree
	//! @param sign true if positive, false otherwise
	//! @param d degree component
	//! @param m minute component
	//! @param s second component
	void decDegToDms(double angle, bool& sign, unsigned int& d, unsigned int& m, double& s);

	//! Convert an angle in decimal degrees to a dms formatted string.
	//! @param angle input angle in decimal degrees
	QString decDegToDmsStr(const double angle);

	//! Convert a dms formatted string to an angle in radian
	//! @param s The input string
	double dmsStrToRad(const QString& s);

	//! Convert from spherical coordinates to Rectangular direction.
	//! @param lng longitude in radian
	//! @param lat latitude in radian
	//! @param v the resulting 3D unit vector
	inline void spheToRect(const double lng, const double lat, Vec3d& v){
		const double cosLat = cos(lat);
		v.set(cos(lng) * cosLat, sin(lng) * cosLat, sin(lat));
	}

	//! Convert from spherical coordinates to Rectangular direction.
	//! @param lng longitude in radian
	//! @param lat latitude in radian
	//! @param v the resulting 3D unit vector
	inline void spheToRect(const float lng, const float lat, Vec3f& v){
		const double dlng = static_cast<double>(lng), dlat = static_cast<double>(lat), cosLat = cos(dlat);
		v.set(static_cast<float>(cos(dlng) * cosLat), static_cast<float>(sin(dlng) * cosLat), sinf(lat));
	}

	//! Convert from spherical coordinates to Rectangular direction.
	//! @param lng longitude in radian
	//! @param lat latitude in radian
	//! @param v the resulting 3D unit vector
	inline void spheToRect(const double lng, const double lat, Vec3f& v){
		const float cosLat = cos(static_cast<float>(lat));
		v.set(cos(static_cast<float>(lng)) * cosLat, sin(static_cast<float>(lng)) * cosLat, sin(static_cast<float>(lat)));
	}

	//! Convert from spherical coordinates to Rectangular direction.
	//! @param lng longitude in radian
	//! @param lat latitude in radian
	//! @param v the resulting 3D unit vector
	inline void spheToRect(const float lng, const float lat, Vec3d& v){
		const double dlng = static_cast<double>(lng), dlat = static_cast<double>(lat), cosLat = cos(dlat);
		v.set(cos(dlng) * cosLat, sin(dlng) * cosLat, sin(dlat));
	}

	//! Convert from Rectangular direction to spherical coordinate components.
	//! @param lng double* to store longitude in radian
	//! @param lat double* to store latitude in radian
	//! @param v the input 3D vector
	inline void rectToSphe(double *lng, double *lat, const Vec3d& v){
		double r = v.length();
		*lat = asin(v[2]/r);
		*lng = atan2(v[1],v[0]);
	}

	//! Convert from Rectangular direction to spherical coordinate components.
	//! @param lng float* to store longitude in radian
	//! @param lat float* to store latitude in radian
	//! @param v the input 3D vector
	inline void rectToSphe(float *lng, float *lat, const Vec3d& v){
		double r = v.length();
		*lat = static_cast<float>(asin(v[2]/r));
		*lng = static_cast<float>(atan2(v[1],v[0]));
	}


	//! Convert from Rectangular direction to spherical coordinate components.
	//! @param lng float* to store longitude in radian
	//! @param lat float* to store latitude in radian
	//! @param v the input 3D vector
	inline void rectToSphe(float *lng, float *lat, const Vec3f& v){
		float r = v.length();
		*lat = asinf(v[2]/r);
		*lng = atan2f(v[1],v[0]);
	}

	//! Convert from Rectangular direction to spherical coordinate components.
	//! @param lng double* to store longitude in radian
	//! @param lat double* to store latitude in radian
	//! @param v the input 3D vector
	inline void rectToSphe(double *lng, double *lat, const Vec3f &v){
		double r = static_cast<double>(v.length());
		*lat = asin(static_cast<double>(v[2])/r);
		*lng = atan2(static_cast<double>(v[1]),static_cast<double>(v[0]));
	}

	//! Coordinate Transformation from equatorial to ecliptical
	inline void equToEcl(const double raRad, const double decRad, const double eclRad, double *lambdaRad, double *betaRad){
		*lambdaRad=std::atan2(std::sin(raRad)*std::cos(eclRad)+std::tan(decRad)*std::sin(eclRad), std::cos(raRad));
		*betaRad=std::asin(std::sin(decRad)*std::cos(eclRad)-std::cos(decRad)*std::sin(eclRad)*std::sin(raRad));
	}

	//! Coordinate Transformation from ecliptical to equatorial
	inline void eclToEqu(const double lambdaRad, const double betaRad, const double eclRad, double *raRad, double *decRad){
		*raRad = std::atan2(std::sin(lambdaRad)*std::cos(eclRad)-std::tan(betaRad)*std::sin(eclRad), std::cos(lambdaRad));
		*decRad = std::asin(std::sin(betaRad)*std::cos(eclRad)+std::cos(betaRad)*std::sin(eclRad)*std::sin(lambdaRad));
	}

	//! Convert a string longitude, latitude, RA or declination angle
	//! to radians.
	//! @param str the angle in a format according to:
	//!   angle ::= [sign¹] ( real [degs | mins | secs]
	//!                     | [integer degs] ( [integer mins] real secs
	//!                                       | real mins )
	//!                     ) [cardinal¹]            
	//!   sign ::= + | -
	//!   digit := 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9
	//!   integer ::= digit [digits]
	//!   real ::= integer [. integer]
	//!   degs ::= d | h² | U+00B0 | U+00BA³
	//!   mins ::= m | '
	//!   secs ::= s | "
	//!   cardinal ::= N² | S² | E | W
	//!   ¹) A cardinal point overrides any sign. N and E result in a positive,
	//!      W and S in a negative angle.
	//!   ²) The use of the cardinal points N and S together with the hour sign
	//!      'H' or 'h' is forbidden.
	//!   ³) The MASCULINE ORDINAL INDICATOR U+00BA is accepted, considering
	//!      Spanish QWERTY keyboards.
	//! The string is parsed without regarding to case, except that, after a
	//! single real, a solitary 's' indicates seconds whereas an 'S' indicates South.
	//! It is highly recommended to use lower case for hdms and upper case for NSEW.
	//! Latitude: North is positive, South is negative.
	//! Longitude: East is positive, West is negative.
	//! @return the angle in radians.
	double getDecAngle(const QString& str);

	//! Check if a number is a power of 2.
	inline bool isPowerOfTwo(const int value){
		return (value & -value) == value;
	}

	//! Return the first power of two bigger than the given value.
	int getBiggerPowerOfTwo(int value);

	//! Return the inverse sinus hyperbolic of z.
	inline double asinh(const double z){
		return (z>0 ? std::log(z + std::sqrt(z*z+1)) :
			     -std::log(-z + std::sqrt(z*z+1)));
	}

	//! Integer modulo where the result is always nonnegative. [0..b-1]
	inline int imod(const int a, const int b){
		int ret = a % b;
		if(ret < 0)
			ret+=b;
		return ret;
	}
	//! Integer modulo where the result is always positive. [1..b]
	inline int amod(const int a, const int b){
		int ret = a % b;
		if(ret <= 0)
			ret+=b;
		return ret;
	}
	//! Double modulo where the result is always nonnegative. [0..(b
	inline double fmodpos(const double a, const double b){
		double ret = fmod(a, b);
		if(ret < 0)
			ret+=b;
		return ret;
	}
	//! Float modulo where the result is always nonnegative. [0..(b
	inline float fmodpos(const float a, const float b){
		float ret = fmodf(a, b);
		if(ret < 0)
			ret+=b;
		return ret;
	}

	//! Floor integer division provides truncating to the next lower integer, also for negative numerators.
	//! https://stackoverflow.com/questions/2622441/c-integer-floor-function
	//! @returns floor(num/den)
	inline long intFloorDiv (long num, long den)
	{
	  if (0 < (num^den)) // lgtm [cpp/bitwise-sign-check]
	    return num/den;
	  else
	    {
	      ldiv_t res = ldiv(num,den);
	      return (res.rem)? res.quot-1
			      : res.quot;
	    }
	}

	//! version of intFloorDiv() for large integers.
	inline long intFloorDivLL(long long num, long long den)
	{
	  if (0 < (num^den)) // lgtm [cpp/bitwise-sign-check]
	    return static_cast<long>(num/den);
	  else
	    {
	      lldiv_t res = lldiv(num,den);
	      long long ret=  (res.rem)? res.quot-1
			      : res.quot;
	      return static_cast<long>(ret);
	    }
	}

	///////////////////////////////////////////////////
	// New Qt based General Calendar Functions.
	//! Make from julianDay a year, month, day for the Julian Date julianDay represents.
	void getDateFromJulianDay(const double julianDay, int *year, int *month, int *day);

	//! Make from julianDay an hour, minute, second.
	void getTimeFromJulianDay(const double julianDay, int *hour, int *minute, int *second, int *millis=Q_NULLPTR);

	//! Parse an ISO8601 date string.
	//! Also handles negative and distant years.
	bool getDateTimeFromISO8601String(const QString& iso8601Date, int* y, int* m, int* d, int* h, int* min, float* s);
	
	//! Format the given Julian Day in (UTC) ISO8601 date string.
	//! Also handles negative and distant years.
	QString julianDayToISO8601String(const double jd, bool addMS = false);

	//! Return the Julian Date matching the ISO8601 date string.
	//! Also handles negative and distant years.
	double getJulianDayFromISO8601String(const QString& iso8601Date, bool* ok);
	
	//! Format the date and day-of-week per the format in fmt
	//! (see QDateTime::toString()). Uses the @b system locale, not
	//! the one set in Stellarium.
	//! @return QString representing the formatted date
	QString localeDateString(const int year, const int month, const int day, const int dayOfWeek, const QString &fmt);

	//! Format the date and day-of-week per the @b system locale's
	//! QLocale::ShortFormat.
	//! @return QString representing the formatted date
	QString localeDateString(const int year, const int month, const int day, const int dayOfWeek);

	//! Return a day number of week for date
	//! @return number of day: 0 - sunday, 1 - monday,..
	int getDayOfWeek(int year, int month, int day);
	inline int getDayOfWeek(double JD){ double d= fmod(JD+1.5, 7); if (d<0) d+=7.0;
		return std::lround(floor(d));
	}

	//! Get the current Julian Date from system time.
	//! @return the current Julian Date
	double getJDFromSystem();

	//! Get the Julian Day Number (JD) from Besselian epoch.
	//! @param epoch Besselian epoch, expressed as year
	//! @return Julian Day number (JD) for B<Year>
	double getJDFromBesselianEpoch(const double epoch);

	//! Convert a time of day to the fraction of a Julian Day.
	//! Note that a Julian Day starts at 12:00, not 0:00, and
	//! so 12:00 == 0.0 and 0:00 == 0.5
	double qTimeToJDFraction(const QTime& time);

	//! Convert a fraction of a Julian Day to a QTime
	QTime jdFractionToQTime(const double jd);

	//! Convert a QT QDateTime class to julian day.
	//! @param dateTime the UTC QDateTime to convert
	//! @result the matching decimal Julian Day
	inline double qDateTimeToJd(const QDateTime& dateTime){
		return dateTime.date().toJulianDay()+static_cast<double>(1./(24*60*60*1000))*QTime(0, 0, 0, 0).msecsTo(dateTime.time())-0.5;
	}

	//! Convert a julian day to a QDateTime.
	//! @param jd to convert
	//! @result the matching UTC QDateTime
	QDateTime jdToQDateTime(const double& jd);

	//! Compute Julian day number from calendar date.
	//! Uses QDate functionality if possible, but also works for negative JD.
	//! Dates before 1582-10-15 are in the Julian Calendar.
	//! @param newjd pointer to JD
	//! @param y Calendar year.
	//! @param m month, 1=January ... 12=December
	//! @param d day
	//! @param h hour
	//! @param min minute
	//! @param s second
	//! @result true in all conceivable cases.
	bool getJDFromDate(double* newjd, const int y, const int m, const int d, const int h, const int min, const float s);

	int numberOfDaysInMonthInYear(const int month, const int year);
	//! @result true if year is a leap year. Observes 1582 switch from Julian to Gregorian Calendar.
	bool isLeapYear(const int year);
	//! Find day number for date in year.
	//! Meeus, Astronomical Algorithms 2nd ed., 1998, ch.7, p.65
	int dayInYear(const int year, const int month, const int day);
	//! Return a fractional year like YYYY.ddddd. For negative years, the year number is decreased. E.g. -500.5 occurs in -501.
	double yearFraction(const int year, const int month, const double day);

	bool changeDateTimeForRollover(int oy, int om, int od, int oh, int omin, int os,
				       int* ry, int* rm, int* rd, int* rh, int* rmin, int* rs);

	//! Output a QVariantMap to qDebug().  Formats like a tree where there are nested objects.
	void debugQVariantMap(const QVariant& m, const QString& indent="", const QString& key="");


	/// Compute acos(x)
	//! The taylor serie is not accurate around x=1 and x=-1
	inline float fastAcos(const float x)
	{
		return static_cast<float>(M_PI_2) - (x + x*x*x * (1.f/6.f + x*x * (3.f/40.f + 5.f/112.f * x*x)) );
	}

	//! Compute exp(x) for small exponents x
	inline float fastExp(const float x)
	{
		return (x>=0)?
			(1.f + x*(1.f+ x/2.f*(1.f+ x/3.f*(1.f+x/4.f*(1.f+x/5.f))))):
				1.f / (1.f -x*(1.f -x/2.f*(1.f- x/3.f*(1.f-x/4.f*(1.f-x/5.f)))));
	}

	// Calculate and return sidereal period in days from semi-major axis (in AU)
	//double calculateSiderealPeriod(const double SemiMajorAxis);  MOVED TO Orbit.h

	//! Convert decimal hours to hours, minutes, seconds
	QString hoursToHmsStr(const double hours, const bool lowprecision = false);
	QString hoursToHmsStr(const float hours, const bool lowprecision = false);

	//! Convert a hms formatted string to decimal hours
	double hmsStrToHours(const QString& s);

	//! Convert days (float) to a time string
	QString daysFloatToDHMS(float days);

	//! Get Delta-T estimation for a given date.
	//! This is just an "empty" correction function, returning 0.
	double getDeltaTwithoutCorrection(const double jDay);

	//! Get Delta-T estimation for a given date.
	//! Note that this method is recommended for the year range:
	//! -1999 to +3000. It gives details for -500...+2150.
	//! Implementation of algorithm by Espenak & Meeus (2006) for DeltaT computation
	//! @param jDay the date and time expressed as a Julian day
	//! @return Delta-T in seconds
	double getDeltaTByEspenakMeeus(const double jDay);

	//! Get Delta-T estimation for a given date.
	//! Implementation of algorithm by Schoch (1931) for DeltaT computation,
	//! outdated but may be useful for science-historical purposes.
	//! Source: Schoch, C. (1931). Die sekulare Accelaration des Mondes und der Sonne.
	//! Astronomische Abhandlungen, Ergnzungshefte zu den Astronomischen Nachrichten,
	//! Band 8, B2. Kiel.
	//! @param jDay the date and time expressed as a Julian day
	//! @return Delta-T in seconds
	double getDeltaTBySchoch(const double jDay);

	//! Get Delta-T estimation for a given date.
	//! Implementation of algorithm by Clemence (1948) for DeltaT computation,
	//! outdated but may be useful for science-historical purposes.
	//! Source: On the system of astronomical constants.
	//! Clemence, G. M.
	//! Astronomical Journal, Vol. 53, p. 169
	//! 1948AJ.....53..169C [http://adsabs.harvard.edu/abs/1948AJ.....53..169C]
	//! @param jDay the date and time expressed as a Julian day
	//! @return Delta-T in seconds
	double getDeltaTByClemence(const double jDay);

	//! Get Delta-T estimation for a given date.
	//! Implementation of algorithm by IAU (1952) for DeltaT computation,
	//! outdated but may be useful for science-historical purposes.
	//! Source: Spencer Jones, H., "The Rotation of the Earth, and the Secular Accelerations of the Sun, Moon and Planets",
	//! Monthly Notices of the Royal Astronomical Society, 99 (1939), 541-558
	//! http://adsabs.harvard.edu/abs/1939MNRAS..99..541S
	//! @param jDay the date and time expressed as a Julian day
	//! @return Delta-T in seconds
	double getDeltaTByIAU(const double jDay);

	//! Get Delta-T estimation for a given date.
	//! Implementation of algorithm by Astronomical Ephemeris (1960) for DeltaT computation.
	//! Sources: Spencer Jones, H., "The Rotation of the Earth, and the Secular Accelerations of the Sun, Moon and Planets",
	//! Monthly Notices of the Royal Astronomical Society, 99 (1939), 541-558
	//! http://adsabs.harvard.edu/abs/1939MNRAS..99..541S
	//! or Explanatory Supplement to the Astr. Ephemeris, 1961, p.87.
	//! Also used by Mucke&Meeus, Canon of Solar Eclipses, Vienna 1983.
	//! @param jDay the date and time expressed as a Julian day
	//! @return Delta-T in seconds
	double getDeltaTByAstronomicalEphemeris(const double jDay);

	//! Get Delta-T estimation for a given date.
	//! Implementation of algorithm by Tuckerman (1962, 1964) & Goldstine (1973) for DeltaT computation
	//! @param jDay the date and time expressed as a Julian day
	//! @return Delta-T in seconds
	double getDeltaTByTuckermanGoldstine(const double jDay);

	//! Get Delta-T estimation for a given date.
	//! Implementation of algorithm by Muller & Stephenson (1975) for DeltaT computation.
	//! Source: The accelerations of the earth and moon from early astronomical observations
	//! Muller, P. M.; Stephenson, F. R.
	//! Growth rhythms and the history of the earth's rotation; Proceedings of the Interdisciplinary
	//! Winter Conference on Biological Clocks and Changes in the Earth's Rotation: Geophysical and
	//! Astronomical Consequences, Newcastle-upon-Tyne, England, January 8-10, 1974. (A76-18126 06-46)
	//! London, Wiley-Interscience, 1975, p. 459-533; Discussion, p. 534.
	//! 1975grhe.conf..459M [http://adsabs.harvard.edu/abs/1975grhe.conf..459M]
	//! @param jDay the date and time expressed as a Julian day
	//! @return Delta-T in seconds
	double getDeltaTByMullerStephenson(const double jDay);

	//! Get Delta-T estimation for a given date.
	//! Implementation of algorithm by Stephenson (1978) for DeltaT computation.
	//! Source: Pre-Telescopic Astronomical Observations
	//! Stephenson, F. R.
	//! Tidal Friction and the Earth's Rotation, Proceedings of a Workshop, held in Bielefeld,
	//! September 26-30, 1977, Edited by P. Brosche, and J. Sundermann. Berlin: Springer-Verlag, 1978, p.5
	//! 1978tfer.conf....5S [http://adsabs.harvard.edu/abs/1978tfer.conf....5S]
	//! @param jDay the date and time expressed as a Julian day
	//! @return Delta-T in seconds
	double getDeltaTByStephenson1978(const double jDay);

	//! Get Delta-T estimation for a given date.
	//! Implementation of algorithm by Stephenson (1997) for DeltaT computation.
	//! Source: Book "Historical Eclipses and Earth's Rotation" by F. R. Stephenson (1997)
	//! http://ebooks.cambridge.org/ebook.jsf?bid=CBO9780511525186
	//! @param jDay the date and time expressed as a Julian day
	//! @return Delta-T in seconds
	double getDeltaTByStephenson1997(const double jDay);

	//! Get Delta-T estimation for a given date.
	//! Implementation of algorithm by Schmadel & Zech (1979) for DeltaT computation.
	//! Outdated, but may be useful for science-historical purposes.
	//! Source: Polynomial approximations for the correction delta T E.T.-U.T. in the period 1800-1975
	//! Schmadel, L. D.; Zech, G.
	//! Acta Astronomica, vol. 29, no. 1, 1979, p. 101-104.
	//! 1979AcA....29..101S [http://adsabs.harvard.edu/abs/1979AcA....29..101S]
	//! @param jDay the date and time expressed as a Julian day
	//! @return Delta-T in seconds
	//! @note The polynome is strictly applicable 1800...1975 only! Delivers values for the nearer edge (1800/1989) if jDay is outside.
	double getDeltaTBySchmadelZech1979(const double jDay);

	//! Get Delta-T estimation for a given date.
	//! Implementation of algorithm by Morrison & Stephenson (1982) for DeltaT computation
	//! @param jDay the date and time expressed as a Julian day
	//! @return Delta-T in seconds
	double getDeltaTByMorrisonStephenson1982(const double jDay);

	//! Get Delta-T estimation for a given date.
	//! Implementation of algorithm by Stephenson & Morrison (1984) for DeltaT computation
	//! Source: Long-term changes in the rotation of the earth - 700 B.C. to A.D. 1980.
	//! Stephenson, F. R.; Morrison, L. V.
	//! Philosophical Transactions, Series A (ISSN 0080-4614), vol. 313, no. 1524, Nov. 27, 1984, p. 47-70.
	//! 1984RSPTA.313...47S [http://adsabs.harvard.edu/abs/1984RSPTA.313...47S]
	//! @param jDay the date and time expressed as a Julian day
	//! @return Delta-T in seconds or Zero if date outside years -391..1600
	double getDeltaTByStephensonMorrison1984(const double jDay);

	//! Get Delta-T estimation for a given date.
	//! Implementation of algorithm by Stephenson & Morrison (1995) for DeltaT computation
	//! Source: Long-Term Fluctuations in the Earth's Rotation: 700 BC to AD 1990.
	//! Stephenson, F. R.; Morrison, L. V.
	//! Philosophical Transactions: Physical Sciences and Engineering, Volume 351, Issue 1695, pp. 165-202
	//! 1995RSPTA.351..165S [http://adsabs.harvard.edu/abs/1995RSPTA.351..165S]
	//! @param jDay the date and time expressed as a Julian day
	//! @return Delta-T in seconds
	double getDeltaTByStephensonMorrison1995(const double jDay);

	//! Get Delta-T estimation for a given date.
	//! Implementation of algorithm by Stephenson & Houlden (1986) for DeltaT computation
	//! @param jDay the date and time expressed as a Julian day
	//! @return Delta-T in seconds or 0 if year > 1600
	double getDeltaTByStephensonHoulden(const double jDay);

	//! Get Delta-T estimation for a given date.
	//! Implementation of algorithm by Espenak (1987, 1989) for DeltaT computation.
	//! This relation should not be used before around 1950 or after around 2100 (Espenak, pers. comm.).
	//! @param jDay the date and time expressed as a Julian day
	//! @return Delta-T in seconds
	double getDeltaTByEspenak(const double jDay);

	//! Get Delta-T estimation for a given date.
	//! Implementation of algorithm by Borkowski (1988) for DeltaT computation.
	//! Source: ELP 2000-85 and the dynamic time-universal time relation
	//! Borkowski, K. M.
	//! Astronomy and Astrophysics (ISSN 0004-6361), vol. 205, no. 1-2, Oct. 1988, p. L8-L10.
	//! 1988A&A...205L...8B [http://adsabs.harvard.edu/abs/1988A&A...205L...8B]
	//! @param jDay the date and time expressed as a Julian day
	//! @return Delta-T in seconds
	double getDeltaTByBorkowski(const double jDay);

	//! Get Delta-T estimation for a given date.
	//! Implementation of algorithm by Schmadel & Zech (1988) for DeltaT computation.
	//! Source: Empirical Transformations from U.T. to E.T. for the Period 1800-1988
	//! Schmadel, L. D.; Zech, G.
	//! Astronomische Nachrichten 309, 219-221
	//! 1988AN....309..219S [http://adsabs.harvard.edu/abs/1988AN....309..219S]
	//! @param jDay the date and time expressed as a Julian day
	//! @return Delta-T in seconds
	//! @note The polynome is strictly applicable 1800...1988 only! Delivers values for the nearer edge (1800/1989) if jDay is outside.
	double getDeltaTBySchmadelZech1988(const double jDay);

	//! Get Delta-T estimation for a given date.
	//! Implementation of algorithm by Chapront-Touzé & Chapront (1991) for DeltaT computation
	//! @param jDay the date and time expressed as a Julian day
	//! @return Delta-T in seconds or 0 if year not in -391..1600
	double getDeltaTByChaprontTouze(const double jDay);	

	//! Get Delta-T estimation for a given date.
	//! Implementation of the "historical" part of the algorithm by JPL Horizons for DeltaT computation.
	//! @param jDay the date and time expressed as a Julian day
	//! @return Delta-T in seconds or 0 if year not in -2999..1620 (!)
	double getDeltaTByJPLHorizons(const double jDay);

	//! Get Delta-T estimation for a given date.
	//! Implementation of algorithm by Morrison & Stephenson (2004, 2005) for DeltaT computation.
	//! Sources: Historical values of the Earth's clock error ΔT and the calculation of eclipses
	//! Morrison, L. V.; Stephenson, F. R.
	//! Journal for the History of Astronomy (ISSN 0021-8286), Vol. 35, Part 3, No. 120, p. 327 - 336 (2004)
	//! 2004JHA....35..327M [http://adsabs.harvard.edu/abs/2004JHA....35..327M]
	//! Addendum: Historical values of the Earth's clock error
	//! Morrison, L. V.; Stephenson, F. R.
	//! Journal for the History of Astronomy (ISSN 0021-8286), Vol. 36, Part 3, No. 124, p. 339 (2005)
	//! 2005JHA....36..339M [http://adsabs.harvard.edu/abs/2005JHA....36..339M]
	//! @param jDay the date and time expressed as a Julian day
	//! @return Delta-T in seconds
	double getDeltaTByMorrisonStephenson2004(const double jDay);

	//! Get Delta-T estimation for a given date.
	//! Implementation of algorithm by Reijs (2006) for DeltaT computation
	//! Details: http://www.iol.ie/~geniet/eng/DeltaTeval.htm
	//! @param jDay the date and time expressed as a Julian day
	//! @return Delta-T in seconds
	double getDeltaTByReijs(const double jDay);

	//! Get Delta-T estimation for a given date.
	//! Implementation of algorithm by Chapront, Chapront-Touze & Francou (1997) & Meeus (1998) for DeltaT computation
	//! @param jDay the date and time expressed as a Julian day
	//! @return Delta-T in seconds
	double getDeltaTByChaprontMeeus(const double jDay);

	//! Get Delta-T estimation for a given date.
	//! Implementation of algorithm by Meeus & Simons (2000) for DeltaT computation.
	//! Source: Polynomial approximations to Delta T, 1620-2000 AD
	//! Meeus, J.; Simons, L.
	//! Journal of the British Astronomical Association, vol.110, no.6, 323
	//! 2000JBAA..110..323M [http://adsabs.harvard.edu/abs/2000JBAA..110..323M]
	//! @param jDay the date and time expressed as a Julian day
	//! @return Delta-T in seconds or 0 if year not in 1620..2000
	double getDeltaTByMeeusSimons(const double jDay);

	//! Get Delta-T estimation for a given date.
	//! Implementation of algorithm by Montenbruck & Pfleger (2000) for DeltaT computation,
	//! a data fit through the table of values found in Meeus, Astronomical algorithms (1991).
	//! Book "Astronomy on the Personal Computer" by O. Montenbruck & T. Pfleger (4th ed., 2000), p.181-182
	//! @param jDay the date and time expressed as a Julian day
	//! @return Delta-T in seconds or 0 if not 1825<=year<2005
	double getDeltaTByMontenbruckPfleger(const double jDay);

	//! Get Delta-T estimation for a given date.
	//! Implementation of algorithm by Reingold & Dershowitz (1997, 2001, 2002, 2007, 2018) for DeltaT computation.
	//! This is again mostly a data fit based on the table in Meeus, Astronomical Algorithms (1991).
	//! This is the version given in the 4rd edition ("the ultimate edition"; 2018) which added the fit
	//! for -500..1699 and 2006..2150 omitted from previous editions.
	//! Calendrical Calculations: The Ultimate Edition / Edward M. Reingold, Nachum Dershowitz - 4th Edition,
	//! Cambridge University Press, 2018. - 660p. ISBN: 9781107057623, DOI: 10.1017/9781107415058
	//! @param jDay the date and time expressed as a Julian day
	//! @return Delta-T in seconds
	double getDeltaTByReingoldDershowitz(const double jDay);

	//! Get Delta-T estimation for a given date.
	//! Implementation of algorithm by Banjevic (2006) for DeltaT computation.
	//! Source: Ancient eclipses and dating the fall of Babylon
	//! Banjevic, B.
	//! Publications of the Astronomical Observatory of Belgrade, Vol. 80, p. 251-257 (2006)
	//! 2006POBeo..80..251B [http://adsabs.harvard.edu/abs/2006POBeo..80..251B]
	//! @param jDay the date and time expressed as a Julian day
	//! @return Delta-T in seconds
	double getDeltaTByBanjevic(const double jDay);

	//! Get Delta-T estimation for a given date.
	//! Implementation of algorithm by Islam, Sadiq & Qureshi (2008 + revisited 2013) for DeltaT computation.
	//! Source: Error Minimization of Polynomial Approximation of DeltaT
	//! Islam, S. & Sadiq, M. & Qureshi, M. S.
	//! Journal of Astrophysics & Astronomy, Vol. 29, p. 363-366 (2008)
	//! http://www.ias.ac.in/jaa/dec2008/JAA610.pdf
	//! Note: These polynomials are based on the uncorrected deltaT table from the Astronomical Almanac, thus
	//! ndot = -26.0 arcsec/cy^2. Meeus & Simons (2000) corrected the deltaT table for years before 1955.5 using
	//! ndot = -25.7376 arcsec/cy^2. Therefore the accuracies stated by Meeus & Simons are correct and cannot be
	//! compared with accuracies from Islam & Sadiq & Qureshi.
	//! @param jDay the date and time expressed as a Julian day
	//! @return Delta-T in seconds
	double getDeltaTByIslamSadiqQureshi(const double jDay);

	//! Get Delta-T estimation for a given date.
	//! Implementation of polinomial approximation of time period 1620-2013 for DeltaT by M. Khalid, Mariam Sultana and Faheem Zaidi (2014).
	//! Source: Delta T: Polynomial Approximation of Time Period 1620-2013
	//! Journal of Astrophysics, Vol. 2014, Article ID 480964
	//! https://doi.org/10.1155/2014/480964
	//! @param jDay the date and time expressed as a Julian day
	//! @return Delta-T in seconds
	double getDeltaTByKhalidSultanaZaidi(const double jDay);

	//! Get Delta-T estimation for a given date.
	//! Implementation of a spline approximation for time period -720-2016.0 for DeltaT by Stephenson, Morrison and Hohenkerk (2016).
	//! Source: Measurement of the Earth’s rotation: 720 BC to AD 2015
	//! Proc. R. Soc. A 472: 20160404.
	//! https://doi.org/10.1098/rspa.2016.0404
	//! @param jDay the date and time expressed as a Julian day
	//! @return Delta-T in seconds. For times outside the limits, return result from the fitting parabola.
	double getDeltaTByStephensonMorrisonHohenkerk2016(const double jDay);

	//! Get Secular Acceleration estimation for a given year.
	//! Method described is here: http://eclipse.gsfc.nasa.gov/SEcat5/secular.html
	//! For adapting from -26 to -25.858, use -0.91072 * (-25.858 + 26.0) = -0.12932224
	//! For adapting from -26 to -23.895, use -0.91072 * (-23.895 + 26.0) = -1.9170656
	//! @param jDay the JD
	//! @param ndot value of n-dot (secular acceleration of the Moon) which should be used in the lunar ephemeris instead of the default values.
	//! @param useDE43x true if function should adapt calculation of the secular acceleration of the Moon to the DE43x ephemeris
	//! @return SecularAcceleration in seconds
	//! @note n-dot for secular acceleration of the Moon in ELP2000-82B is -23.8946 "/cy/cy and for DE43x is -25.8 "/cy/cy
	double getMoonSecularAcceleration(const double jDay, const double ndot, const bool useDE43x);

	//! Get the standard error (sigma) for the value of DeltaT
	//! @param jDay the JD
	//! @return sigma in seconds
	double getDeltaTStandardError(const double jDay);

	//! Get value of the Moon fluctuation
	//! Source: The Rotation of the Earth, and the Secular Accelerations of the Sun, Moon and Planets
	//! Spencer Jones, H.
	//! Monthly Notices of the Royal Astronomical Society, 99 (1939), 541-558
	//! 1939MNRAS..99..541S [http://adsabs.harvard.edu/abs/1939MNRAS..99..541S]
	//! @param jDay the JD
	//! @return fluctuation in seconds
	double getMoonFluctuation(const double jDay);

	//! Sign function from http://stackoverflow.com/questions/1903954/is-there-a-standard-sign-function-signum-sgn-in-c-c
	template <typename T> int sign(T val)
	{
		return (T(0) < val) - (val < T(0));
	}
	
	//! Compute cosines and sines around a circle which is split in "slices" parts.
	//! Values are stored in the global static array cos_sin_theta.
	//! Used for the sin/cos values along a latitude circle, equator, etc. for a spherical mesh.
	//! @param slices number of partitions (elsewhere called "segments") for the circle
	float *ComputeCosSinTheta(const unsigned int slices);
	
	//! Compute cosines and sines around a half-circle which is split in "segments" parts.
	//! Values are stored in the global static array cos_sin_rho.
	//! Used for the sin/cos values along a meridian for a spherical mesh.
	//! @param segments number of partitions (elsewhere called "stacks") for the half-circle
	float *ComputeCosSinRho(const unsigned int segments);
	
	//! Compute cosines and sines around part of a circle (from top to bottom) which is split in "segments" parts.
	//! Values are stored in the global static array cos_sin_rho.
	//! Used for the sin/cos values along a meridian.
	//! This allows leaving away pole caps. The array now contains values for the region minAngle+segments*phi
	//! @param dRho a difference angle between the stops
	//! @param segments number of segments
	//! @param minAngle start angle inside the half-circle. maxAngle=minAngle+segments*phi
	float* ComputeCosSinRhoZone(const float dRho, const unsigned int segments, const float minAngle);

	//! Calculate fixed days (R.D.) from Gregorian date
	//! @param year
	//! @param month
	//! @param day
	//! @return days from Rata Die
	int getFixedFromGregorian(const int year, const int month, const int day);

	//! Comparison two string versions and return a result in range -1,0,1
	//! @param v1 string for version 1
	//! @param v2 string for version 2
	//! @return result (-1: v1<v2; 0: v1==v2; 1: v1>v2)
	int compareVersions(const QString v1, const QString v2);

	//! Uncompress gzip or zlib compressed data.
	QByteArray uncompress(const QByteArray& data);

	//! Uncompress (gzip/zlib) data from this QIODevice, which must be open and readable.
	//! @param device The device to read from, must already be opened with an OpenMode supporting reading
	//! @param maxBytes The max. amount of bytes to read from the device, or -1 to read until EOF.  Note that it
	//! always stops when inflate() returns Z_STREAM_END. Positive values can be used for interleaving compressed data
	//! with other data.
	QByteArray uncompress(QIODevice &device, qint64 maxBytes=-1);

	//! Greatest Common Divisor (Euclid's algorithm)
	//! @param a first number
	//! @param b second number
	//! @return Greatest Common Divisor
	int gcd(int a, int b);

	//! Least Common Multiple
	//! @param a first number
	//! @param b second number
	//! @return Least Common Multiple
	int lcm(int a, int b);

	//! Given regularly spaced steps x1, x2, x3 and curve values y1, y2, y3,
	//! calculate an intermediate value of the 3 arguments for the given interpolation point n.
	//! @param n Interpolation factor: steps from x2
	//! @param y1 Argument 1
	//! @param y2 Argument 2
	//! @param y3 Argument 3
	//! @return interpolation value
	template<class T> T interpolate3(T n, T y1, T y2, T y3)
	{
		// See "Astronomical Algorithms" by J. Meeus

		// Equation 3.2
		T a = y2-y1;
		T b = y3-y2;
		T c = b-a;

		// Equation 3.3
		return y2 + n * 0.5f * (a + b + n * c);
	}

	//! Given regularly spaced steps x1, x2, x3, x4, x5 and curve values y1, y2, y3, y4, y5,
	//! calculate an intermediate value of the 5 arguments for the given interpolation point n.
	//! @param n Interpolation factor: steps from x3
	//! @param y1 Argument 1
	//! @param y2 Argument 2
	//! @param y3 Argument 3
	//! @param y3 Argument 4
	//! @param y3 Argument 5
	//! @return interpolation value
	template<class T> T interpolate5(T n, T y1, T y2, T y3, T y4, T y5)
	{
		// See "Astronomical Algorithms" by J. Meeus
		// Eq. 3.8
		T A=y2-y1; T B=y3-y2; T C=y4-y3; T D=y5-y4;
		T E=B-A; T F=C-B; T G=D-C;
		T H=F-E; T J=G-F;
		T K=J-H;

		return (((K*(1.0/24.0)*n + (H+J)/12.0)*n  + (F*0.5-K/24.0))*n + ((B+C)*0.5 - (H+J)/12.0))*n +y3;
	}

	//! Interval test. This checks whether @param value is within [@param low, @param high]
	template <typename T> bool isWithin(const T& value, const T& low, const T& high)
	{
	    return !(value < low) && !(high < value);
	}


#ifdef _MSC_BUILD
	inline double trunc(double x)
	{
		return (x < 0 ? std::ceil(x) : std::floor(x));
	}
	inline float trunc(float x)
	{
		return (x < 0 ? std::ceil(x) : std::floor(x));
	}
#else
	inline double trunc(double x) { return ::trunc(x); }
	inline float trunc(float x) { return ::trunc(x); }
#endif
}

#endif // STELUTILS_HPP

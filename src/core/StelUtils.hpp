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

#ifndef _STELUTILS_HPP_
#define _STELUTILS_HPP_

#include "VecMath.hpp"

#include <QVariantMap>
#include <QDateTime>
#include <QSize>
#include <QString>

// astonomical unit (km)
#define AU 149597870.691
// Parsec (km)
#define PARSEC 30.857e12
// speed of light (km/sec)
#define SPEED_OF_LIGHT 299792.458

//! @namespace StelUtils contains general purpose utility functions.
namespace StelUtils
{
	//! Return the full name of stellarium, i.e. "stellarium 0.9.0"
	QString getApplicationName();

	//! Return the version of stellarium, i.e. "0.9.0"
	QString getApplicationVersion();

	//! Convert an angle in hms format to radian.
	//! @param h hour component
	//! @param m minute component
	//!	@param s second component
	//! @return angle in radian
	double hmsToRad(unsigned int h, unsigned int m, double s);

	//! Convert an angle in +-dms format to radian.
	//! @param d degree component
	//! @param m arcmin component
	//!	@param s arcsec component
	//! @return angle in radian
	double dmsToRad(int d, unsigned int m, double s);

	//! Convert an angle in radian to hms format.
	//! @param rad input angle in radian
	//! @param h hour component
	//! @param m minute component
	//!	@param s second component
	void radToHms(double rad, unsigned int& h, unsigned int& m, double& s);

	//! Convert an angle in radian to +-dms format.
	//! @param rad input angle in radian
	//! @param sign true if positive, false otherwise
	//! @param d degree component
	//! @param m minute component
	//!	@param s second component
	void radToDms(double rad, bool& sign, unsigned int& d, unsigned int& m, double& s);

	//! Convert an angle in radian to a hms formatted string.
	//! If the second, minute part is == 0, it is not output
	//! @param angle input angle in radian
	QString radToHmsStrAdapt(double angle);

	//! Convert an angle in radian to a hms formatted string.
	//! @param angle input angle in radian
	//! @param decimal output decimal second value
	QString radToHmsStr(double angle, bool decimal=false);

	//! Convert an angle in radian to a dms formatted string.
	//! If the second, minute part is == 0, it is not output
	//! @param angle input angle in radian
	//! @param useD Define if letter "d" must be used instead of deg sign
	QString radToDmsStrAdapt(double angle, bool useD=false);

	//! Convert an angle in radian to a dms formatted string.
	//! @param angle input angle in radian
	//! @param useD Define if letter "d" must be used instead of deg sign
	//! @param decimal output decimal second value
	QString radToDmsStr(double angle, bool decimal=false, bool useD=false);

	//! Convert a dms formatted string to an angle in radian
	//! @param s The input string
	double dmsStrToRad(const QString& s);

	//! Obtains a Vec3f from a string.
	//! @param s the string describing the Vector with the form "x,y,z"
	//! @return The corresponding vector
	//! @deprecated Use the >> operator from Vec3f class
	Vec3f strToVec3f(const QStringList& s);
	Vec3f strToVec3f(const QString& s);

	//! Converts a Vec3f to HTML color notation.
	//! @param v The vector
	//! @return The string in HTML color notation "#rrggbb".
	QString vec3fToHtmlColor(const Vec3f& v);

	//! Converts a color in HTML notation to a Vec3f.
	//! @param c The HTML spec color string
	Vec3f htmlColorToVec3f(const QString& c);

	//! Convert from spherical coordinates to Rectangular direction.
	//! @param lng longitude in radian
	//! @param lat latitude in radian
	//! @param v the resulting 3D unit vector
	void spheToRect(double lng, double lat, Vec3d& v);

	//! Convert from spherical coordinates to Rectangular direction.
	//! @param lng longitude in radian
	//! @param lat latitude in radian
	//! @param v the resulting 3D unti vector
	void spheToRect(float lng, float lat, Vec3f& v);

	//! Convert from spherical coordinates to Rectangular direction.
	//! @param lng double* to store longitude in radian
	//! @param lat double* to store latitude in radian
	//! @param v the input 3D vector
	void rectToSphe(double *lng, double *lat, const Vec3d& v);

	//! Convert from spherical coordinates to Rectangular direction.
	//! @param lng float* to store longitude in radian
	//! @param lat float* to store latitude in radian
	//! @param v the input 3D vector
	void rectToSphe(float *lng, float *lat, const Vec3d& v);

	//! Convert from spherical coordinates to Rectangular direction.
	//! @param lng float* to store longitude in radian
	//! @param lat float* to store latitude in radian
	//! @param v the input 3D vector
	void rectToSphe(float *lng, float *lat, const Vec3f& v);

	// GZ: some additions. I need those just for quick conversions for text display.
	//! Coordinate Transformation from equatorial to ecliptical
	void ctRadec2Ecl(const double raRad, const double decRad, const double eclRad, double *lambdaRad, double *betaRad);
	// GZ: done

	//! Convert a string longitude, latitude, RA or Declination angle
	//! to radians.
	//! @param str the angle in format something like these:
	//! - +53d  51'21.6"
	//! - +53d51'21.6"
	//! - -1d  10'31.8"
	//! - +46d6'31"
	//! - 50D46'0"N
	//! - 123D47'59"W
	//! - 123.567 N
	//! - 123.567W
	//! - -123.567
	//! - 12h 14m 6s
	//! The degree separator may be a degree symbol (\\xBA) in addition
	//! to a 'd' or 'D'.
	//! @return the angle in radians.
	//! Latitude: North are positive, South are negative.
	//! Longitude: East is positive, West is negative.
	//! Note: if there is a N, S, E or W suffix, any leading + or -
	//! characters are ignored.
	double getDecAngle(const QString& str);

	//! Check if a number is a power of 2.
	bool isPowerOfTwo(int value);

	//! Return the first power of two greater or equal to the given value.
	int smallestPowerOfTwoGreaterOrEqualTo(int value);

	//! Return the smallest size with power-of two dimensions at least as large as given size.
	QSize smallestPowerOfTwoSizeGreaterOrEqualTo(const QSize base);

	//! Return the inverse sinus hyperbolic of z.
	double asinh(double z);

	///////////////////////////////////////////////////
	// New Qt based General Calendar Functions.
	//! Make from julianDay a year, month, day for the Julian Date julianDay represents.
	void getDateFromJulianDay(double julianDay, int *year, int *month, int *day);

	//! Make from julianDay an hour, minute, second.
	void getTimeFromJulianDay(double julianDay, int *hour, int *minute, int *second);

	//! Parse an ISO8601 date string.
	//! Also handles negative and distant years.
	bool getDateTimeFromISO8601String(const QString& iso8601Date, int* y, int* m, int* d, int* h, int* min, float* s);
	
	//! Format the given Julian Day in (UTC) ISO8601 date string.
	//! Also handles negative and distant years.
	QString julianDayToISO8601String(double jd);

	//! Return the Julian Date matching the ISO8601 date string.
	//! Also handles negative and distant years.
	double getJulianDayFromISO8601String(const QString& iso8601Date, bool* ok);
	
	//! Format the date and day-of-week per the format in fmt
	//! (see QDateTime::toString()). Uses the @b system locale, not
	//! the one set in Stellarium.
	//! @return QString representing the formatted date
	QString localeDateString(int year, int month, int day, int dayOfWeek, QString fmt);

	//! Format the date and day-of-week per the @b system locale's
	//! QLocale::ShortFormat.
	//! @return QString representing the formatted date
	QString localeDateString(int year, int month, int day, int dayOfWeek);

	//! Get the current Julian Date from system time.
	//! @return the current Julian Date
	double getJDFromSystem();

	//! Convert a time of day to the fraction of a Julian Day.
	//! Note that a Julian Day starts at 12:00, not 0:00, and
	//! so 12:00 == 0.0 and 0:00 == 0.5
	double qTimeToJDFraction(const QTime& time);

	//! Convert a fraction of a Julian Day to a QTime
	QTime jdFractionToQTime(double jd);

	//! Return number of hours offset from GMT, using Qt functions.
	float getGMTShiftFromQT(double jd);

	//! Convert a QT QDateTime class to julian day.
	//! @param dateTime the UTC QDateTime to convert
	//! @result the matching decimal Julian Day
	double qDateTimeToJd(const QDateTime& dateTime);

	//! Convert a julian day to a QDateTime.
	//! @param jd to convert
	//! @result the matching UTC QDateTime
	QDateTime jdToQDateTime(const double& jd);

	bool getJDFromDate(double* newjd, int y, int m, int d, int h, int min, int s);

	int numberOfDaysInMonthInYear(int month, int year);
	bool changeDateTimeForRollover(int oy, int om, int od, int oh, int omin, int os,
								   int* ry, int* rm, int* rd, int* rh, int* rmin, int* rs);

	//! Output a QVariantMap to qDebug().  Formats like a tree where there are nested objects.
	void debugQVariantMap(const QVariant& m, const QString& indent="", const QString& key="");


	/// Compute acos(x)
	//! The taylor serie is not accurate around x=1 and x=-1
	inline float fastAcos(float x)
	{
		return M_PI_2 - (x + x*x*x * (1.f/6.f + x*x * (3.f/40.f + 5.f/112.f * x*x)) );
	}

	//! Compute exp(x) for small exponents x
	inline float fastExp(float x)
	{
		return (x>=0)?
			(1.f + x*(1.f+ x/2.f*(1.f+ x/3.f*(1.f+x/4.f*(1.f+x/5.f))))):
				1.f / (1.f -x*(1.f -x/2.f*(1.f- x/3.f*(1.f-x/4.f*(1.f-x/5.f)))));
	}

	//! Get a night mode version of a color.  That is find the brightness of a color and set that in the
	//! red channel only
	inline Vec3f getNightColor(const Vec3f& dayColor)
	{
		float max = 0.0;
		for(int i=0; i<3; i++)
		{
			max = dayColor[i] > max ? dayColor[i] : max;
		}
		return Vec3f(max, 0, 0);
	}

	//! Calculate and getting sidereal period in days from semi-major axis (in AU)
	double calculateSiderealPeriod(double SemiMajorAxis);

	//! Calculate duration of mean solar day
	double calculateSolarDay(double siderealPeriod, double siderealDay, bool forwardDirection);

	QString hoursToHmsStr(double hours);

	//! Get the number of seconds since program start.
	//!
	//! @note This is implemented in platform-specific ways to be as precise 
	//!       as possible, but there is a fallback for other platforms that 
	//!       might not be precise at all.
	//!       This is currently used e.g. to measure FPS, but it should never 
	//!       be used for critical functionality.
	long double secondsSinceStart();

	//! Get Delta-T estimation for a given date.
	//! Note that this method is valid for the year range:
	//! -1999 to +3000, outside of which "0" will be returned.
	//! @param jDay the date and time expressed as a julian day
	//! @return Delta-T in seconds
	double getDeltaT(double jDay);
	//! Get Secular Acceleration estimation for a given year.
	//! @param jDay the JD
	//! @return SecularAcceleration in seconds
	double getMoonSecularAcceleration(double jDay);
}

#endif // _STELUTILS_HPP_

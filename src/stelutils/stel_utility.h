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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef _S_UTILITY_H_
#define _S_UTILITY_H_

#include <string>
#include <map>
#include <ctime>
#include "vecmath.h"

using namespace std;

typedef map< string, string > stringHash_t;
typedef stringHash_t::const_iterator stringHashIter_t;

// astonomical unit (km)
#define AU 149597870.691

// speed of light (km/sec)
#define SPEED_OF_LIGHT 299792.458

#define MY_MAX(a,b) (((a)>(b))?(a):(b))
#define MY_MIN(a,b) (((a)<(b))?(a):(b))

namespace StelUtils {
	
	//! Dummy wrapper used to remove a boring warning when using strftime directly
	size_t my_strftime(char *s, size_t max, const char *fmt, const struct tm *tm);

	//! @brief Convert an angle in hms format to radian
	//! @param h hour component
	//! @param m minute component
	//!	@param s second component
	//! @return angle in radian
	double hms_to_rad(unsigned int h, unsigned int m, double s);
			   
	//! @brief Convert an angle in dms format to radian
	//! @param d degree component
	//! @param m arcmin component
	//!	@param s arcsec component
	//! @return angle in radian
	double dms_to_rad(int d, int m, double s);	  

	//! @brief Obtains a Vec3f from a string
	//! @param s the string describing the Vector with the form "x,y,z"
	//! @return The corresponding vector
	//! @deprecated Use the >> operator from Vec3f class
	Vec3f str_to_vec3f(const string& s);
	
	//! @brief Obtains a string from a Vec3f 
	//! @param v The vector
	//! @return the string describing the Vector with the form "x,y,z"
	//! @deprecated Use the << operator from Vec3f class
	string vec3f_to_str(const Vec3f& v);
	
	//! @brief Print the passed angle with the format ddÂ°mm'ss(.ss)"
	//! @param angle Angle in radian
	//! @param decimal Define if 2 decimal must also be printed
	//! @param useD Define if letter "d" must be used instead of Â°
	//! @return The corresponding string
	wstring printAngleDMS(double angle, bool decimals = false, bool useD = false);
	
	//! @brief Print the passed angle with the format +hh:mm:ss(.ss)"
	//! @param angle Angle in radian
	//! @param decimals Define if 2 decimal must also be printed
	//! @return The corresponding string
	wstring printAngleHMS(double angle, bool decimals = false);
		
	//! @brief Convert UTF-8 std::string to std::wstring using mbstowcs() function
	//! @param The input string in UTF-8 format
	//! @return The matching wide string
	wstring stringToWstring(const string& s);
	
	//! @brief Convert std::wstring to UTF-8 std::string using wcstombs() function
	//! @param The input wide character string
	//! @return The matching string in UTF-8 format
	string wstringToString(const wstring& ws);
	
	//! @brief Format the double value to a wstring (with current locale)
	//! Can't use directly wostringstream because it is not portable to MinGW32/STLPort..
	//! @param d The input double value
	//! @return The matching wstring
	wstring doubleToWstring(double d);
	
	//! @brief Format the int value to a wstring (with current locale)
	//! Can't use directly wostringstream because it is not portable to MinGW32/STLPort..
	//! @param i The input int value
	//! @return The matching wstring
	wstring intToWstring(int i);
	
	double str_to_double(string str);
	double str_to_pos_double(string str); // always positive
	int str_to_int(string str);
	int str_to_int(string str, int default_value);
	string double_to_str(double dbl);
	long int str_to_long(string str);
	
	//! @brief Convert from spherical coordinates to Rectangular direction
	//! @param lng longitude in radian
	//! @param lat latitude in radian
	void sphe_to_rect(double lng, double lat, Vec3d& v);
	
	//! @brief Convert from spherical coordinates to Rectangular direction
	//! @param lng longitude in radian
	//! @param lat latitude in radian
	void sphe_to_rect(float lng, float lat, Vec3f& v);
	
	//! @brief Convert from spherical coordinates to Rectangular direction
	//! @param lng double* to store longitude in radian
	//! @param lat double* to store latitude in radian
	void rect_to_sphe(double *lng, double *lat, const Vec3d& v);

	//! Obtains Latitude, Longitude, RA or Declination from a string.
	double get_dec_angle(const string&);
	
	//! Check if a file exist
	//! May require optimization
	bool fileExists(const std::string& fileName);
	
	//! Check if a number is a power of 2
	bool isPowerOfTwo(int value);
	
	//! Return the first power of two bigger than the given value 
	int getBiggerPowerOfTwo(int value);
	
	//! Download the file from the given URL to the given name using libcurl
	bool downloadFile(const std::string& url, const std::string& fullPath);
}

// General Calendar Functions

// Human readable (easy printf) date format
typedef struct
{
	int years; 		/*!< Years. All values are valid */
	int months;		/*!< Months. Valid values : 1 (January) - 12 (December) */
	int days; 		/*!< Days. Valid values 1 - 28,29,30,31 Depends on month.*/
	int hours; 		/*!< Hours. Valid values 0 - 23. */
	int minutes; 	/*!< Minutes. Valid values 0 - 59. */
	double seconds;	/*!< Seconds. Valid values 0 - 59.99999.... */
}ln_date;


/* Calculate the date from the julian day. */
void get_date(double JD, ln_date * date);

/* Calculate julian day from system time. */
double get_julian_from_sys();

// Calculate tm struct from julian day
void get_tm_from_julian(double JD, struct tm * tm_time);

//! Return the number of hours to add to gmt time to get the local time at time JD
//! taking the parameters from system. This takes into account the daylight saving
//! time if there is. (positive for Est of GMT)
float get_GMT_shift_from_system(double JD, bool _local=0);

//! Return the time zone name taken from system locale
wstring get_time_zone_name_from_system(double JD);

//! Return the time in ISO 8601 UTC format that is : %Y-%m-%d %H:%M:%S
string get_ISO8601_time_UTC(double JD);

// convert string in ISO 8601-like format [+/-]YYYY-MM-DDThh:mm:ss (no timezone offset) to julian day
int string_to_jday(string date, double &jd);

#endif

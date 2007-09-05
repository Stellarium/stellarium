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
#include <vector>
#include "vecmath.h"
#include <sstream>
#include <algorithm>
#include <stdexcept>

#include "fixx11h.h"
#include <QDateTime>

using namespace std;

// astonomical unit (km)
#define AU 149597870.691

// speed of light (km/sec)
#define SPEED_OF_LIGHT 299792.458

#define MY_MAX(a,b) (((a)>(b))?(a):(b))
#define MY_MIN(a,b) (((a)<(b))?(a):(b))

//! Errors occuring because of Stellarium's code should throw this exception,
//! or subclass of it.
class StellariumException : public std::exception
{
public:
	StellariumException(const std::string& msg) : message("StellariumException occured: " + msg) {;}
	~StellariumException() throw() {;}
	virtual const char* what() const throw() {return message.c_str();}
	std::string message;
};

namespace StelUtils {

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
	//! @param rad input angle in radian
	string radToHmsStrAdapt(double angle);
	
	//! Convert an angle in radian to a hms formatted wstring.
	//! If the second, minute part is == 0, it is not output
	//! @param rad input angle in radian
	wstring radToHmsWstrAdapt(double angle);
	
	//! Convert an angle in radian to a hms formatted string.
	//! @param rad input angle in radian
	string radToHmsStr(double angle, bool decimal=false);
	
	//! Convert an angle in radian to a hms formatted wstring.
	//! @param rad input angle in radian
	wstring radToHmsWstr(double angle, bool decimal=false);
	
	//! Convert an angle in radian to a dms formatted string.
	//! If the second, minute part is == 0, it is not output
	//! @param rad input angle in radian
	string radToDmsStrAdapt(double angle);
	
	//! Convert an angle in radian to a dms formatted wstring.
	//! If the second, minute part is == 0, it is not output
	//! @param rad input angle in radian
	//! @param useD Define if letter "d" must be used instead of �
	wstring radToDmsWstrAdapt(double angle, bool useD=false);
	
	//! Convert an angle in radian to a dms formatted string.
	//! @param rad input angle in radian
	string radToDmsStr(double angle, bool decimal=false);
	
	//! Convert an angle in radian to a dms formatted wstring.
	//! @param rad input angle in radian
	//! @param useD Define if letter "d" must be used instead of �
	wstring radToDmsWstr(double angle, bool decimal=false, bool useD=false);
	
	//! Obtains a Vec3f from a string.
	//! @param s the string describing the Vector with the form "x,y,z"
	//! @return The corresponding vector
	//! @deprecated Use the >> operator from Vec3f class
	Vec3f str_to_vec3f(const string& s);
	
	//! Obtains a string from a Vec3f.
	//! @param v The vector
	//! @return the string describing the Vector with the form "x,y,z"
	//! @deprecated Use the << operator from Vec3f class
	string vec3f_to_str(const Vec3f& v);
		
	//! Convert from UTF-8 to wchar_t, this is likely to be not very portable.
	//! @param The input string in UTF-8 format
	//! @return The matching wide string
	wstring stringToWstring(const string& s);
	
	//! Convert std::wstring to UTF-8 std::string using wcstombs() function.
	//! @param The input wide character string
	//! @return The matching string in UTF-8 format
	string wstringToString(const wstring& ws);
	
	//! Format the double value to a wstring (with current locale).
	//! Can't use directly wostringstream because it is not portable to MinGW32/STLPort..
	//! @param d the input double value
	//! @return the matching wstring
	wstring doubleToWstring(double d);
	
	//! Format the int value to a wstring (with current locale).
	//! Can't use directly wostringstream because it is not portable to MinGW32/STLPort..
	//! @param i the input int value
	//! @return the matching wstring
	wstring intToWstring(int i);
	
	//! Format the int value to a string (with current locale).
	//! @param i the input int value
	//! @return the matching string
	string intToString(int i);
	
	//! Extract the int value from a string.
	//! @param str the input string
	//! @return the matching int
	int stringToInt(const string& str);
	
	//! Extract the int value from a string, and use default value if the string is not convertible as an int.
	//! @param str the input string
	//! @param defaultValue the default fallback value
	//! @return the matching int or default value
	int stringToInt(const string& str, int defaultValue);
	
	//! Extract the double value from a string.
	//! @param str the input string
	//! @return the matching double value
	double stringToDouble(const string& str);
	
	//! Format the double value to a string (with current locale).
	//! @param dbl the input int value
	//! @return the matching string
	string doubleToString(double dbl);
	
	//! Extract the long int value from a string.
	//! @param str the input string
	//! @return the matching long int value
	long int stringToLong(const string& str);
	
	//! Convert from spherical coordinates to Rectangular direction.
	//! @param lng longitude in radian
	//! @param lat latitude in radian
	void sphe_to_rect(double lng, double lat, Vec3d& v);
	
	//! Convert from spherical coordinates to Rectangular direction.
	//! @param lng longitude in radian
	//! @param lat latitude in radian
	void sphe_to_rect(float lng, float lat, Vec3f& v);
	
	//! Convert from spherical coordinates to Rectangular direction.
	//! @param lng double* to store longitude in radian
	//! @param lat double* to store latitude in radian
	void rect_to_sphe(double *lng, double *lat, const Vec3d& v);

	//! Convert from spherical coordinates to Rectangular direction.
	//! @param lng float* to store longitude in radian
	//! @param lat float* to store latitude in radian
	void rect_to_sphe(float *lng, float *lat, const Vec3d& v);

	//! Obtains Latitude, Longitude, RA or Declination from a string.
	double get_dec_angle(const string&);
	
	//! Delete the file.
	bool deleteFile(const std::string& fileName);

	//! Check if the filename is an absolute path.
	bool checkAbsolutePath(const string& fileName);

	//! Check if a number is a power of 2.
	bool isPowerOfTwo(int value);
	
	//! Return the first power of two bigger than the given value.
	int getBiggerPowerOfTwo(int value);
	
	//! Download the file from the given URL to the given name using libcurl.
	bool downloadFile(const std::string& url, const std::string& fullPath, const std::string& referer="Stellarium", const std::string& cookiesFile="");
	
	//! Return the inverse sinus hyperbolic of z.
	double asinh(double z);
	
	//! Check if a vector of strings has a CLI-style option.
	//! @param args a vector of strings, think argv
	//! @param shortOpt a short-form option string, e.g, "-h"
	//! @param longOpt a long-form option string, e.g. "--help"
	//! @param modify whether to remove found options from args (default=true)
	//! @return true if the option exists in args
	bool argsHaveOption(vector<string>& args, string shortOpt, string longOpt, bool modify=true);
	
	//! Retrieve option with argument from vector of strings.
	//! given a vector of string command line arguments, this function will
	//! extract the argument to an option of type T.
	//! The option may be expresed using either the short form, e.g. -n arg,
	//! the long form with a space, e.g. --number arg, or the long form
	//! with an =, e.g. --number=arg
	//! Type checking is done on the result using the stringstream class, so
	//! any type which can be validated and converted using this class may be
	//! used with this function.
	//! @param args a vector of string arguments, think argv
	//! @param shortOpt the short form of the option, e.g. -n
	//! @param longOpt the long form of the option, e.g. --number
	//! @param defValue the default value to return if the option was not found in args
	//! @param mofify whether to remove found values from args or not (default=true)
	//! @exception runtime_error("no_optarg") the expected argument to the option was not found
	//! @exception runtime_error("optarg_type") the expected argument to the option was not found
	template<class T>
	T argsHaveOptionWithArg(vector<string>& args, string shortOpt, string longOpt, T defValue, bool modify=true)
	{
		vector<string>::iterator optArg;
	
		vector<string>::iterator lastOpt = find(args.begin(), args.end(), "--");
		for(vector<string>::iterator i=args.begin(); i!=lastOpt; i++)
		{
			if (*i == shortOpt || *i == longOpt)
			{
				optArg = i + 1;
				if (optArg == lastOpt)
					throw(runtime_error("no_optarg"));
				else
				{
					T result;
					stringstream ss(*optArg);
					ss >> result;
					if ( ! ss.fail() )
					{
						if (modify)
							args.erase(i, optArg + 1);

						return result;
					}
					else
					{
						throw(runtime_error("optarg_type"));
					}
				}
				
			}
			else if ( (*i).substr(0, longOpt.length()) == longOpt && (*i).substr(longOpt.length(), 1) == "=" )
			{
				string arg(*i);
				arg.erase(0,longOpt.length()+1);
				T result;
				stringstream ss(arg);
				ss >> result;
				if ( ! ss.fail() )
				{
					if (modify)
						args.erase(i);

					return result;
				}
				else
				{
					throw(runtime_error("optarg type"));
				}
			}
		}
		return defValue;
	}
	
	///////////////////////////////////////////////////
	// New Qt based General Calendar Functions. 
	
	//! Convert a QT QDateTime class to julian day.
	//! @param dateTime the UTC QDateTime to convert
	//! @result the matching decimal Julian Day
	double qDateTimeToJd(const QDateTime& dateTime);

	//! Convert a  julian day to a QT QDateTime class.
	//! Warning if JD < 0 the date is invalid (any date before 2 January 4713 B.C.)
	//! @param the decimal Julian Day
	//! @result the matching UTC QDateTime
	QDateTime jdToQDateTime(double jd);

	//! Get the current Julian Date from system time.
	//! @return the current Julian Date
	double getJDFromSystem(void);
}

///////////////////////////////////////////////////
// Old General Calendar Functions. 
// They should be migrated to more protable ones based on Qt

//! Return the number of hours to add to gmt time to get the local time at time JD.
//! taking the parameters from system. This takes into account the daylight saving
//! time if there is. (positive for Est of GMT)
float get_GMT_shift_from_system(double JD);

//! Return the time zone name taken from system locale.
wstring get_time_zone_name_from_system(double JD);

//! convert string in ISO 8601-like format to julian day.
//! @param date string in ISO 8601-like format [+/-]YYYY-MM-DDThh:mm:ss (no timezone offset)
int string_to_jday(string date, double &jd);


#endif

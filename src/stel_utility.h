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

#include <iostream>
#include <sstream>
#include <string>
#include <cmath>
#include <map>
#include "vecmath.h"

using namespace std;

typedef std::map< std::string, std::string > stringHash_t;
typedef stringHash_t::const_iterator stringHashIter_t;

class StelUtility {
public:
	//! @brief Convert an angle in hms format to radian
	//! @param h hour component
	//! @param m minute component
	//!	@param s second component
	//! @return angle in radian
	static double hms_to_rad(unsigned int h, unsigned int m, double s);
			   
	//! @brief Convert an angle in dms format to radian
	//! @param d degree component
	//! @param m arcmin component
	//!	@param s arcsec component
	//! @return angle in radian
	static double dms_to_rad(int d, int m, double s);	  

	//! @brief Obtains a Vec3f from a string
	//! @param s the string describing the Vector with the form "x,y,z"
	//! @return The correspondong vector
	static Vec3f str_to_vec3f(const string& s);
	
	//! @brief Obtains a string from a Vec3f 
	//! @param v The vector
	//! @return the string describing the Vector with the form "x,y,z"
	static string vec3f_to_str(const Vec3f& v);
	
	//! @brief Print the passed angle with the format dd°mm'ss(.ss)"
	//! @param angle Angle in radian
	//! @param decimal Define if 2 decimal must also be printed
	//! @param useD Define if letter "d" must be used instead of °
	//! @return The corresponding string
	static string printAngleDMS(double angle, bool decimals = false, bool useD = false);
	
	//! @brief Print the passed angle with the format +hh:mm:ss(.ss)"
	//! @param angle Angle in radian
	//! @param decimals Define if 2 decimal must also be printed
	//! @return The corresponding string
	static string printAngleHMS(double angle, bool decimals = false);
		
	//! @brief Convert UTF-8 std::string to std::wstring using mbstowcs() function
	//! @param The input string in UTF-8 format
	//! @return The matching wide string
	static wstring stringToWstring(const string& s);
	
	//! @brief Convert std::wstring to UTF-8 std::string using wcstombs() function
	//! @param The input wide character string
	//! @return The matching string in UTF-8 format
	static string wstringToString(const wstring& ws);
	
};

double hms_to_rad(unsigned int h, double m);
double dms_to_rad(int d, double m);


void sphe_to_rect(double lng, double lat, Vec3d& v);
void sphe_to_rect(double lng, double lat, double r, Vec3d& v);
void sphe_to_rect(float lng, float lat, Vec3f& v);
void rect_to_sphe(double *lng, double *lat, const Vec3d& v);
void rect_to_sphe(float *lng, float *lat, const Vec3f& v);

/* Obtains Latitude, Longitude, RA or Declination from a string. */
double get_dec_angle(const string&);


// Provide the luminance in cd/m^2 from the magnitude and the surface in arcmin^2
float mag_to_luminance(float mag, float surface);

// convert string int ISO 8601-like format [+/-]YYYY-MM-DDThh:mm:ss (no timzone offset)
// to julian day
int string_to_jday(string date, double &jd);

double str_to_double(string str);

// always positive
double str_to_pos_double(string str);

int str_to_int(string str);
int str_to_int(string str, int default_value);

string double_to_str(double dbl);
long int str_to_long(string str);

int fcompare(const string& _base, const string& _sub);

string translateGreek(const string& s);
string stripConstellation(const string& s);

enum DRAWMODE { DM_NORMAL=0, DM_CHART, DM_NIGHTCHART };
extern int draw_mode;

#endif

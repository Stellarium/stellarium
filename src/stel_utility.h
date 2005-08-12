/*
 * Stellarium
 * Copyright (C) 2002 Fabien Chéreau
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

// Angles and coordinate conversions
double hms_to_rad(unsigned int h, unsigned int m, double s);
double dms_to_rad(int d, int m, double s);

double hms_to_rad(unsigned int h, double m);
double dms_to_rad(int d, double m);

//void rad_to_hms(unsigned int *h, unsigned int *m, double *s, double r);
void sphe_to_rect(double lng, double lat, Vec3d& v);
void sphe_to_rect(double lng, double lat, double r, Vec3d& v);
void sphe_to_rect(float lng, float lat, Vec3f& v);
void rect_to_sphe(double *lng, double *lat, const Vec3d& v);
void rect_to_sphe(float *lng, float *lat, const Vec3f& v);

// Obtains a Vec3f from a string
Vec3f str_to_vec3f(const string& s);
// Obtains a string from a Vec3f with the form x,y,z
string vec3f_to_str(const Vec3f& v);

/* Obtains Latitude, Longitude, RA or Declination from a string. */
double get_dec_angle(const string&);

/* Obtains a human readable angle in the form: ddºmm'ss.ss" */
string print_angle_dms(double location);

/* Obtains a human readable angle in the form: dd\6mm'ss.ss" */
string print_angle_dms_stel(double location);

/* Obtains a human readable angle in the form: hhhmmmss.sss" */
string print_angle_hms(double location);

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

#endif

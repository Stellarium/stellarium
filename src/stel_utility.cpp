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


#include <sstream>
#include <cstdlib>
#include <malloc.h>	// Only for cygwin
#include "stel_utility.h"
#include "stellarium.h"


double hms_to_rad(unsigned int h, unsigned int m, double s)
{
	return (double)M_PI/24.*h*2.+(double)M_PI/12.*m/60.+s*M_PI/43200.;
}

double dms_to_rad(int d, int m, double s)
{
	double t = (double)M_PI/180.*d+(double)M_PI/10800.*m+s*M_PI/648000.;
	return fabs(t)*d/abs(d);
}

double hms_to_rad(unsigned int h, double m)
{
	return (double)M_PI/24.*h*2.+(double)M_PI/12.*m/60.;
}

double dms_to_rad(int d, double m)
{
	double t = (double)M_PI/180.*d+(double)M_PI/10800.*m;
	return fabs(t)*d/abs(d);
}


void sphe_to_rect(double lng, double lat, Vec3d& v)
{
	const double cosLat = cos(lat);
    v.set(cos(lng) * cosLat, sin(lng) * cosLat, sin(lat));
}

void sphe_to_rect(double lng, double lat, double r, Vec3d& v)
{
	const double cosLat = cos(lat);
    v.set(cos(lng) * cosLat * r, sin(lng) * cosLat * r, sin(lat) * r);
}

void sphe_to_rect(float lng, float lat, Vec3f& v)
{
	const double cosLat = cos(lat);
    v.set(cos(lng) * cosLat, sin(lng) * cosLat, sin(lat));
}

void rect_to_sphe(double *lng, double *lat, const Vec3d& v)
{
	double r = v.length();
    *lat = asin(v[2]/r);
    *lng = atan2(v[1],v[0]);
}

void rect_to_sphe(float *lng, float *lat, const Vec3f& v)
{
	double r = v.length();
    *lat = asin(v[2]/r);
    *lng = atan2(v[1],v[0]);
}


// Obtains a Vec3f from a string with the form x,y,z
Vec3f str_to_vec3f(const string& s)
{
	float x, y, z;
	if (s.empty() || (sscanf(s.c_str(),"%f,%f,%f",&x, &y, &z)!=3)) return Vec3f(0.f,0.f,0.f);
	return Vec3f(x,y,z);
}

// Obtains a string from a Vec3f with the form x,y,z
string vec3f_to_str(const Vec3f& v)
{
	ostringstream os;
	os << v[0] << "," << v[1] << "," << v[2];
	return os.str();
}

// strips trailing whitespaces from buf.
#define iswhite(c)  ((c)== ' ' || (c)=='\t')
static char *trim(char *x)
{
    char *y;

    if(!x)
        return(x);
    y = x + strlen(x)-1;
    while (y >= x && iswhite(*y))
        *y-- = 0; /* skip white space */
    return x;
}



// salta espacios en blanco
static void skipwhite(char **s)
{
   while(iswhite(**s))
        ++(*s);
}


double get_dec_angle(const string& str)
{
	const char* s = str.c_str();
	char *ptr, *dec, *hh;
	int negative = 0;
	char delim1[] = " :.,;ºDdHhMm'\n\t";
	char delim2[] = " NSEWnsew\"\n\t";
	int dghh = 0, minutes = 0;
	double seconds = 0.0, pos;
        short count;

	enum _type{
    	    HOURS, DEGREES, LAT, LONG
	}type;

	if (s == NULL || !*s)
		return(-0.0);
	count = strlen(s) + 1;
	if ((ptr = (char *) alloca(count)) == NULL)
		return (-0.0);
	memcpy(ptr, s, count);
	trim(ptr);
	skipwhite(&ptr);
        
        /* the last letter has precedence over the sign */
	if (strpbrk(ptr,"SsWw") != NULL) 
		negative = 1;

	if (*ptr == '+' || *ptr == '-')
		negative = (char) (*ptr++ == '-' ? 1 : negative);
	skipwhite(&ptr);
	if ((hh = strpbrk(ptr,"Hh")) != NULL && hh < ptr + 3)
            type = HOURS;
        else 
            if (strpbrk(ptr,"SsNn") != NULL)
		type = LAT;
	    else 
 	        type = DEGREES; /* unspecified, the caller must control it */

	if ((ptr = strtok(ptr,delim1)) != NULL)
		dghh = atoi (ptr);
	else
		return (-0.0);

	if ((ptr = strtok(NULL,delim1)) != NULL) {
		minutes = atoi (ptr);
		if (minutes > 59)
		    return (-0.0);
	}else
		return (-0.0);

	if ((ptr = strtok(NULL,delim2)) != NULL) {
		if ((dec = strchr(ptr,',')) != NULL)
			*dec = '.';
		seconds = strtod (ptr, NULL);
		if (seconds > 59)
			return (-0.0);
	}
	
	if ((ptr = strtok(NULL," \n\t")) != NULL) {
		skipwhite(&ptr);
		if (*ptr == 'S' || *ptr == 'W' || *ptr == 's' || *ptr == 'W')
			    negative = 1;
	}
        pos = dghh + minutes /60.0 + seconds / 3600.0;

	if (type == HOURS && pos > 24.0)
		return (-0.0);
	if (type == LAT && pos > 90.0)
		return (-0.0);
	else
	    if (pos > 180.0)
		return (-0.0);

	if (negative)
		pos = 0.0 - pos;

	return (pos);

}



/*! \fn char * get_humanr_location(double location)
* \param location Location angle in degress
* \return Angle string
*
* Obtains a human readable location in the form: ddºmm'ss.ss"
*/
string print_angle_dms(double location)
{
    static char buf[16];
    double deg = 0.0;
    double min = 0.0;
    double sec = 0.0;
    *buf = 0;
    sec = 60.0 * (modf(location, &deg));
    if (sec <= 0.0)
        sec *= -1;
    sec = 60.0 * (modf(sec, &min));
    sprintf(buf,"%+.2dº%.2d'%.2f\"",(int)deg, (int) min, sec);
    return buf;
}

string print_angle_dms_stel(double location)
{
    static char buf[16];
    double deg = 0.0;
    double min = 0.0;
    double sec = 0.0;
    *buf = 0;
    sec = 60.0 * (modf(location, &deg));
    if (sec <= 0.0)
        sec *= -1;
    sec = 60.0 * (modf(sec, &min));
    sprintf(buf,"%+.2d\6%.2d'%.2f\"",(int)deg, (int) min, sec);
    return buf;
}

/* Obtains a human readable angle in the form: hhhmmmss.sss" */
string print_angle_hms(double angle)
{
    static char buf[16];
    double hr = 0.0;
    double min = 0.0;
    double sec = 0.0;
    *buf = 0;
	while (angle<0) angle+=360;
	angle/=15.;
    min = 60.0 * (modf(angle, &hr));
    sec = 60.0 * (modf(min, &min));
    sprintf(buf,"%.2dh%.2dm%.2fs",(int)hr, (int) min, sec);
    return buf;
}

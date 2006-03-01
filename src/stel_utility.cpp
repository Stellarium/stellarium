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


#include <sstream>
#include <cstdlib>

#if defined( CYGWIN )
 #include <malloc.h>
#endif

#ifdef MINGW32
#define swprintf snwprintf
#endif

#include "stel_utility.h"
#include "stellarium.h"


wstring StelUtility::stringToWstring(const string& s)
{
	// Get wide string length
	size_t len = mbstowcs(NULL, s.c_str(), 0)+1;
	// Create wide string
	wchar_t* ws = new wchar_t[len];
	mbstowcs(ws, s.c_str(), len);
	wstring wws(ws);
	delete [] ws;
	return wws;
}


string StelUtility::wstringToString(const wstring& ws)
{
	// Get UTF-8 string length
	size_t len = wcstombs(NULL, ws.c_str(), 0)+1;
	// Create wide string
	char* s = new char[len];
	wcstombs(s, ws.c_str(), len);
	string ss(s);
	delete [] s;
	return ss;	
}

double StelUtility::hms_to_rad( unsigned int h, unsigned int m, double s )
{
	return (double)M_PI/24.*h*2.+(double)M_PI/12.*m/60.+s*M_PI/43200.;
}

double StelUtility::dms_to_rad(int d, int m, double s)
{
	return (double)M_PI/180.*d+(double)M_PI/10800.*m+s*M_PI/648000.;
}

double hms_to_rad(unsigned int h, double m)
{
	return (double)M_PI/24.*h*2.+(double)M_PI/12.*m/60.;
}

double dms_to_rad(int d, double m)
{
	double t = (double)M_PI/180.*d+(double)M_PI/10800.*m;
	return t;
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
Vec3f StelUtility::str_to_vec3f(const string& s)
{
	float x, y, z;
	if (s.empty() || (sscanf(s.c_str(),"%f,%f,%f",&x, &y, &z)!=3)) return Vec3f(0.f,0.f,0.f);
	return Vec3f(x,y,z);
}

// Obtains a string from a Vec3f with the form x,y,z
string StelUtility::vec3f_to_str(const Vec3f& v)
{
	ostringstream os;
	os << v[0] << "," << v[1] << "," << v[2];
	return os.str();
}

// Provide the luminance in cd/m^2 from the magnitude and the surface in arcmin^2
float mag_to_luminance(float mag, float surface)
{
	return expf(-0.4f * 2.3025851f * (mag - (-2.5f * log10f(surface)))) * 108064.73f;
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
	char *mptr, *ptr, *dec, *hh;
	int negative = 0;
	char delim1[] = " :.,;DdHhMm'\n\t\xBA";  // 0xBA was old degree delimiter
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
	if ((mptr = (char *) malloc(count)) == NULL)
		return (-0.0);
	ptr = mptr;
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
	{
		free(mptr);
		return (-0.0);
	}

	if ((ptr = strtok(NULL,delim1)) != NULL) {
		minutes = atoi (ptr);
		if (minutes > 59)
		{
			free(mptr);
			return (-0.0);
		}
	}else
	{
		free(mptr);
		return (-0.0);
	}

	if ((ptr = strtok(NULL,delim2)) != NULL) {
		if ((dec = strchr(ptr,',')) != NULL)
			*dec = '.';
		seconds = strtod (ptr, NULL);
		if (seconds > 59)
		{
			free(mptr);
			return (-0.0);
		}
	} 
	
	if ((ptr = strtok(NULL," \n\t")) != NULL)
	{
		skipwhite(&ptr);
		if (*ptr == 'S' || *ptr == 'W' || *ptr == 's' || *ptr == 'w') negative = 1;
	}

	free(mptr);

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




//! @brief Print the passed angle with the format ddÃÂ°mm'ss(.ss)"
//! @param angle Angle in radian
//! @param decimal Define if 2 decimal must also be printed
//! @param useD Define if letter "d" must be used instead of °
//! @return The corresponding string
wstring StelUtility::printAngleDMS(double angle, bool decimals, bool useD)
{
    wchar_t buf[32];
    buf[31]=L'\0';
    double deg = 0.0;
    double min = 0.0;
    double sec = 0.0;
	wchar_t sign = L'+';
	int d, m, s;
	wchar_t degsign = L'°';
	if (useD) degsign = L'd';

	angle *= 180./M_PI;

	if(angle<0) {
		angle *= -1;
		sign = '-';
	}

    sec = 60.0 * (modf(angle, &deg));
    sec = 60.0 * (modf(sec, &min));
    
    d = (int)deg;
    m = (int)min;
    s = (int)sec;
    
    if (decimals)
	    swprintf(buf,sizeof(buf),L"%lc%.2d%lc%.2d'%.2f\"", sign, d, degsign, m, sec);
	else
	{
		double sf = sec - s;
		if (sf > 0.5f)
		{
			s += 1;
			if (s == 60)
			{
				s = 0;
				m += 1;
				if (m == 60) 
				{
					m = 0;
					d += 1;
				}
			}
		}
	    swprintf(buf,sizeof(buf), L"%lc%.2d%lc%.2d'%.2d\"", sign, d, degsign, m, s);
	}
    return buf;
}

//! @brief Print the passed angle with the format +hhhmmmss(.ss)"
//! @param angle Angle in radian
//! @param decimals Define if 2 decimal must also be printed
//! @return The corresponding string
wstring StelUtility::printAngleHMS(double angle, bool decimals)
{
    static wchar_t buf[16];
    buf[15] = L'\0';
    double hr = 0.0;
    double min = 0.0;
    double sec = 0.0;
    angle *= 180./M_PI;
	while (angle<0) angle+=360;
	angle/=15.;
    min = 60.0 * (modf(angle, &hr));
    sec = 60.0 * (modf(min, &min));
    if (decimals) swprintf(buf,sizeof(buf),L"%.2dh%.2dm%.2fs",(int)hr, (int) min, sec);
    else swprintf(buf,sizeof(buf),L"%.2dh%.2dm%.0fs",(int)hr, (int) min, sec);
    return buf;
}


// convert string int ISO 8601-like format [+/-]YYYY-MM-DDThh:mm:ss (no timzone offset)
// to julian day

int string_to_jday(string date, double &jd) {

    char tmp;
    int year, month, day, hour, minute, second;
    year = month = day = hour = minute = second = 0;

    std::istringstream dstr( date );

	// TODO better error checking
    dstr >> year >> tmp >> month >> tmp >> day >> tmp >> hour >> tmp >> minute >> tmp >> second;
    
    // cout << year << " " << month << " " << day << " " << hour << " " << minute << " " << second << endl;

    // bounds checking (per s_tui time object)
    if( year > 100000 || year < -100000 || 
	month < 1 || month > 12 ||
	day < 1 || day > 31 ||
	hour < 0 || hour > 23 ||
	minute < 0 || minute > 59 ||
	second < 0 || second > 59) return 0;


    // code taken from s_tui.cpp
    if (month <= 2) {
        year--;
        month += 12;
    }

    // Correct for the lost days in Oct 1582 when the Gregorian calendar
    // replaced the Julian calendar.
    int B = -2;
    if (year > 1582 || (year == 1582 && (month > 10 || (month == 10 && day >= 15)))) {
      B = year / 400 - year / 100;
    }

    jd = ((floor(365.25 * year) +
            floor(30.6001 * (month + 1)) + B + 1720996.5 +
            day + hour / 24.0 + minute / 1440.0 + second / 86400.0));

    return 1;

}


double str_to_double(string str) {

	if(str=="") return 0;
	double dbl;
	std::istringstream dstr( str );
    
	dstr >> dbl;
	return dbl;
}

// always positive
double str_to_pos_double(string str) {

	if(str=="") return 0;
    double dbl;
    std::istringstream dstr( str );

    dstr >> dbl;
    if(dbl < 0 ) dbl *= -1;
    return dbl;
}


int str_to_int(string str) {

	if(str=="") return 0;
	int integer;
	std::istringstream istr( str );
    
	istr >> integer;
	return integer;
}


int str_to_int(string str, int default_value) {

	if(str=="") return default_value;
	int integer;
	std::istringstream istr( str );
    
	istr >> integer;
	return integer;
}

string double_to_str(double dbl) {

  std::ostringstream oss;
  oss << dbl;
  return oss.str();

}

long int str_to_long(string str) {

	if(str=="") return 0;
	long int integer;
	std::istringstream istr( str );
    
	istr >> integer;
	return integer;
}

int fcompare(const string& _base, const string& _sub)
{
     unsigned int i = 0; 
     while (i < _sub.length())
     { 
         if (toupper(_base[i]) == toupper(_sub[i])) i++;
         else return -1;  
     }
     return 0;
}

int fcompare(const wstring& _base, const wstring& _sub)
{
     unsigned int i = 0; 
     while (i < _sub.length())
     { 
         if (toupper(_base[i]) == toupper(_sub[i])) i++;
         else return -1;  
     }
     return 0;
}

/*
int str_compare_case_insensitive(const string& str1, const string& str2)
{

	string tmp1 = str1;
	string tmp2 = str2;

    unsigned int i = 0; 
	while (i < tmp1.length()) { 
		tmp1[i] = toupper(tmp1[i]);
		i++;
	}

	i = 0;
	while (i < tmp2.length()) { 
		tmp2[i] = toupper(tmp2[i]);
		i++;
	}

	return strcmp( tmp1.c_str(), tmp2.c_str() );

}
*/

/*string translateGreek(const string& s, bool greekOnly)
{
	int sz, n;
	unsigned char ch;
	
	string greek, first; 
//	string letters("ALF BET GAM DEL EPS ZET ETA THE IOT KAP LAM MU NU XI OMI PI RHO SIG TAU UPS PHI CHI PSI OME");
	const string letters("AL BE GA DE EP ZE ET TE IO KA LA MU NU KS OM PI RH SI TA UP PH KH PS OM");
	const int array[25] = { 3,3,3,3,3,3,3,3,3,3,3,2,2,2,3,2,3,3,3,3,3,3,3,3 };
	unsigned int loc;
	
	first = s.substr(0,2);
	
	loc = letters.find(first,0);
	if (loc != string::npos)
	{
		n = (int)loc/3;
		sz = array[n];
		
		ch = 130 + n;
		greek = string("a");
		greek[0] = ch;
		if (!greekOnly)
			greek += s.substr(sz);
		else
		{
			loc = s.find(" ",sz-1);
			if (loc != string::npos)
				greek+=s.substr(sz,loc-sz);
		}
		return greek;
	}

	else if (greekOnly) // (but not translateable)
	{
		// strip the stuff after the space if a number
		loc = s.find(" ",0);
		if (loc != string::npos)
			return s.substr(0,loc);

		return s;
	}
	else
		return s;
}
*/

//#define BUILDING_SAO_CAT

wstring translateGreek(const string& s)
{
	int sz, n;
	wostringstream oss;
	
//	string letters("ALF BET GAM DEL EPS ZET ETA THE IOT KAP LAM MU. NU. XI OMI PI RHO SIG TAU UPS PHI CHI PSI OME");
	const string letters("AL BE GA DE EP ZE ET TE IO KA LA MU NU KS OM PI RH SI TA UP PH KH PS OM");
#ifdef BUILDING_SAO_CAT
	const int array[25] = { 3,3,3,3,3,3,3,3,3,3,3,2,2,2,3,2,3,3,3,3,3,3,3,3 };
#else
	const int array[25] = { 3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3 };
#endif
	unsigned int loc;
	
	// Match the first 2 characters
	loc = letters.find(s.substr(0,2),0);

	// no match then return the original string	
	if (loc == string::npos) return StelUtility::stringToWstring(s);

	// get the corresponding number of characters in the greek string
	n = (int)loc/3;
	sz = array[n];
	oss << char(130 + n);

	// see if a number code after and translate to the supertext characters.
	loc = s.substr(sz).find(" ");
	if (loc != string::npos) 
	{
		string subscript = s.substr(sz,loc);
		if (loc > 0) 
		{
			unsigned int i = 0;
			if (loc > 1)
			{
				while (i < loc && (subscript[i] == '.' || subscript[i] == '0')) i++;
			}	
	
			while (i < loc)	oss << char(subscript[i++]+112);
		}
	}
	return oss.str();
}

string stripConstellation(const string& s)
{
	unsigned int loc = s.rfind(" ");
	if (loc != string::npos) return s.substr(0,loc);
	
	return s;
}

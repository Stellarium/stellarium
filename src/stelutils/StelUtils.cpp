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


#include <config.h>

#include <math.h> // fmod
#include <sstream>
#include <fstream>
#include <cstdlib>
#include <ctime>
#include <config.h>
#include <cstdio>
#include <iostream>
#include <iomanip>
#include <vector>
#include <algorithm>
#include <stdexcept>


#include "dirent.h"

#if defined( CYGWIN )
 #include <malloc.h>
#endif

#ifdef HAVE_LIBCURL
 #include <curl/curl.h>
#endif

#include "StelUtils.hpp"
#include "Translator.hpp"
#include "vecmath.h"
#include "GLee.h"

namespace StelUtils {

//! Dummy wrapper used to remove a boring warning when using strftime directly
size_t my_strftime(char *s, size_t max, const char *fmt, const struct tm *tm)
{
	return strftime(s, max, fmt, tm);
}


//! Convert from char* UTF-8 to wchar_t UCS4 - stolen from SDL_ttf library
wchar_t *UTF8_to_UNICODE(wchar_t *unicode, const char *utf8, int len)
{
	int i, j;
	unsigned short ch;  // 16 bits

	for ( i=0, j=0; i < len; ++i, ++j )
	{
		ch = ((const unsigned char *)utf8)[i];
		if ( ch >= 0xF0 )
		{
			ch  =  (unsigned short)(utf8[i]&0x07) << 18;
			ch |=  (unsigned short)(utf8[++i]&0x3F) << 12;
			ch |=  (unsigned short)(utf8[++i]&0x3F) << 6;
			ch |=  (unsigned short)(utf8[++i]&0x3F);
		}
		else
			if ( ch >= 0xE0 )
			{
				ch  =  (unsigned short)(utf8[i]&0x3F) << 12;
				ch |=  (unsigned short)(utf8[++i]&0x3F) << 6;
				ch |=  (unsigned short)(utf8[++i]&0x3F);
			}
			else
				if ( ch >= 0xC0 )
				{
					ch  =  (unsigned short)(utf8[i]&0x3F) << 6;
					ch |=  (unsigned short)(utf8[++i]&0x3F);
				}

		unicode[j] = ch;
	}
	unicode[j] = 0;

	return unicode;
}

//! Convert from UTF-8 to wchar_t
//! Warning this is likely to be not very portable
std::wstring stringToWstring(const string& s)
{
	wchar_t* outbuf = new wchar_t[s.length()+1];
	UTF8_to_UNICODE(outbuf, s.c_str(), s.length());
	wstring ws(outbuf);
	delete[] outbuf;
	return ws;
}

string wstringToString(const wstring& ws)
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

wstring doubleToWstring(double d)
{
	std::wostringstream woss;
	woss << d;
	return woss.str();
}

wstring intToWstring(int i)
{
	std::wostringstream woss;
	woss << i;
	return woss.str();
}

string intToString(int i)
{
	std::ostringstream oss;
	oss << i;
	return oss.str();
}

double hmsToRad(unsigned int h, unsigned int m, double s )
{
	return (double)M_PI/24.*h*2.+(double)M_PI/12.*m/60.+s*M_PI/43200.;
}

double dmsToRad(int d, unsigned int m, double s)
{
	if (d>=0)
		return (double)M_PI/180.*d+(double)M_PI/10800.*m+s*M_PI/648000.;
	return (double)M_PI/180.*d-(double)M_PI/10800.*m-s*M_PI/648000.;
}

/*************************************************************************
 Convert an angle in radian to hms
*************************************************************************/
void radToHms(double angle, unsigned int& h, unsigned int& m, double& s)
{
	angle = fmod(angle,2.0*M_PI);
	if (angle < 0.0) angle += 2.0*M_PI; // range: [0..2.0*M_PI)
		
	angle *= 12./M_PI;

	h = (unsigned int)angle;
	m = (unsigned int)((angle-h)*60);
	s = (angle-h)*3600.-60.*m;
}

/*************************************************************************
 Convert an angle in radian to dms
*************************************************************************/
void radToDms(double angle, bool& sign, unsigned int& d, unsigned int& m, double& s)
{
	angle = fmod(angle,2.0*M_PI);
	sign=true;
	if (angle<0)
	{
		angle *= -1;
		sign = false;
	}
	angle *= 180./M_PI;
	
	d = (unsigned int)angle;
	m = (unsigned int)((angle - d)*60);
	s = (angle-d)*3600-60*m;
}

/*************************************************************************
 Convert an angle in radian to a hms formatted string
 If the minute and second part are null are too small, don't print them
*************************************************************************/
string radToHmsStrAdapt(double angle)
{
	unsigned int h,m;
	double s;
	StelUtils::radToHms(angle+0.005*M_PI/12/(60*60), h, m, s);
	ostringstream os;
	os << h << 'h';
	if (std::fabs(s*100-(int)s*100)>=1)
	{
		os << m << 'm' << std::fixed << std::setprecision(1) << std::setw(4) << std::setfill('0') << s << 's';
	}
	else if ((int)s!=0)
	{
		os << m << 'm' << (int)s << 's';
	}
	else if (m!=0)
	{
		os << m << 'm';
	}
	return os.str();
}

/*************************************************************************
 Convert an angle in radian to a hms formatted wstring
 If the minute and second part are null are too small, don't print them
*************************************************************************/
wstring radToHmsWstrAdapt(double angle)
{
	return StelUtils::stringToWstring(radToHmsStrAdapt(angle));
}


/*************************************************************************
 Convert an angle in radian to a hms formatted string
*************************************************************************/
string radToHmsStr(double angle, bool decimal)
{
	unsigned int h,m;
	double s;
	StelUtils::radToHms(angle+0.005*M_PI/12/(60*60), h, m, s);
	ostringstream os;
	if (decimal)
		os << std::setprecision(1) << std::setw(4);
	else
		os << std::setprecision(0) << std::setw(2);
	
	os << h << 'h' << m << 'm' << std::fixed << std::setfill('0') << s << 's';
	return os.str();
}

/*************************************************************************
 Convert an angle in radian to a hms formatted wstring
*************************************************************************/
wstring radToHmsWstr(double angle, bool decimal)
{
	return StelUtils::stringToWstring(radToHmsStr(angle, decimal));
}

/*************************************************************************
 Convert an angle in radian to a dms formatted string
 If the minute and second part are null are too small, don't print them
*************************************************************************/
string radToDmsStrAdapt(double angle)
{
	bool sign;
	unsigned int d,m;
	double s;
	StelUtils::radToDms(angle+0.005*M_PI/180/(60*60)*(angle<0?-1.:1.), sign, d, m, s);
	ostringstream os;
	
	os << (sign?'+':'-') << d << 'd';
	if (std::fabs(s*100-(int)s*100)>=1)
	{
		os << m << '\'' << std::fixed << std::setprecision(2) << std::setw(5) << std::setfill('0') << s << '\"';
	}
	else if ((int)s!=0)
	{
		os << m << '\'' << (int)s << '\"';
	}
	else if (m!=0)
	{
		os << m << '\'';
	}
	return os.str();
}

/*************************************************************************
 Convert an angle in radian to a dms formatted wstring
 If the minute and second part are null are too small, don't print them
*************************************************************************/
wstring radToDmsWstrAdapt(double angle, bool useD)
{
	wchar_t degsign = L'\u00B0';
	if (useD) degsign = L'd';
	bool sign;
	unsigned int d,m;
	double s;
	StelUtils::radToDms(angle+0.005*M_PI/180/(60*60)*(angle<0?-1.:1.), sign, d, m, s);
	wostringstream os;
	
	os << (sign?L'+':L'-') << d << degsign;
	if (std::fabs(s*100-(int)s*100)>=1)
	{
		os << m << L'\'' << std::fixed << std::setprecision(2) << std::setw(5) << std::setfill(L'0') << s << L'\"';
	}
	else if ((int)s!=0)
	{
		os << m << L'\'' << (int)s << L'\"';
	}
	else if (m!=0)
	{
		os << m << L'\'';
	}
	return os.str();
}

/*************************************************************************
 Convert an angle in radian to a dms formatted string
*************************************************************************/
string radToDmsStr(double angle, bool decimal)
{
	bool sign;
	unsigned int d,m;
	double s;
	StelUtils::radToDms(angle+0.005*M_PI/180/(60*60)*(angle<0?-1.:1.), sign, d, m, s);
	ostringstream os;
	
	os << (sign?'+':'-') << d << 'd';
		
	os << m << '\'' << std::fixed << std::setfill('0');
	
	if (decimal)
		os << std::setprecision(1) << std::setw(4);
	else
		os << std::setprecision(0) << std::setw(2);
		
	os << s << '\"';
	return os.str();
}

/*************************************************************************
 Convert an angle in radian to a dms formatted wstring
*************************************************************************/
wstring radToDmsWstr(double angle, bool decimal, bool useD)
{
	wchar_t degsign = L'\u00B0';
	if (useD) degsign = L'd';
	bool sign;
	unsigned int d,m;
	double s;
	StelUtils::radToDms(angle+0.005*M_PI/180/(60*60)*(angle<0?-1.:1.), sign, d, m, s);
	wostringstream os;
	
	os << (sign?L'+':L'-') << d << degsign;
	
	if (decimal)
		os << std::setprecision(1) << std::setw(4);
	else
		os << std::setprecision(0) << std::setw(2);
		
	os << m << L'\'' << std::fixed << std::setfill(L'0') << s << L'\"';
	return os.str();
}

/*************************************************************************
 Convert an angle in radian to a dms formatted string
*************************************************************************/
string radToDmsWstr(double angle)
{
	bool sign;
	unsigned int d,m;
	double s;
	StelUtils::radToDms(angle+0.005*M_PI/180/(60*60)*(angle<0?-1.:1.), sign, d, m, s);
	ostringstream os;
	
	os << (sign?'+':'-') << d << 'd';
	if (std::fabs(s*100-(int)s*100)>=1)
	{
		os << m << '\'' << std::fixed << std::setprecision(2) << std::setw(5) << std::setfill('0') << s << '\"';
	}
	else if ((int)s!=0)
	{
		os << m << '\'' << (int)s << '\"';
	}
	else if (m!=0)
	{
		os << m << '\'';
	}
	return os.str();
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

double stringToDouble(const string& str)
{
	if(str.empty()) return 0;
	double dbl;
	std::istringstream dstr( str );

	dstr >> dbl;
	return dbl;
}

int stringToInt(const string& str)
{
	if(str.empty()) return 0;
	int integer;
	std::istringstream istr( str );

	istr >> integer;
	return integer;
}


int stringToInt(const string& str, int default_value)
{
	if(str.empty()) return default_value;
	int integer;
	std::istringstream istr( str );

	istr >> integer;
	return integer;
}

string doubleToString(double dbl)
{
	std::ostringstream oss;
	oss << dbl;
	return oss.str();
}

long int stringToLong(const string& str)
{
	if(str.empty()) return 0;
	long int integer;
	std::istringstream istr( str );

	istr >> integer;
	return integer;
}

void sphe_to_rect(double lng, double lat, Vec3d& v)
{
	const double cosLat = cos(lat);
	v.set(cos(lng) * cosLat, sin(lng) * cosLat, sin(lat));
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

void rect_to_sphe(float *lng, float *lat, const Vec3d& v)
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

void rect_to_sphe(double *lng, double *lat, const Vec3f& v)
{
	double r = v.length();
	*lat = asin(v[2]/r);
	*lng = atan2(v[1],v[0]);
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

	if ((ptr = strtok(NULL,delim1)) != NULL)
	{
		minutes = atoi (ptr);
		if (minutes > 59)
		{
			free(mptr);
			return (-0.0);
		}
	}
	else
	{
		free(mptr);
		return (-0.0);
	}

	if ((ptr = strtok(NULL,delim2)) != NULL)
	{
		if ((dec = strchr(ptr,',')) != NULL)
			*dec = '.';
		seconds = strtod (ptr, NULL);
		if (seconds >= 60.0)
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

	pos = ((dghh*60+minutes)*60 + seconds) / 3600.0;
	if (type == HOURS && pos > 24.0)
		return (-0.0);
	if (type == LAT && pos > 90.0)
		return (-0.0);
	else
		if (pos > 180.0)
			return (-0.0);

	if (negative)
		pos = -pos;

	return (pos);
}

// Check if a file exist
bool fileExists(const std::string& fileName)
{
	std::fstream fin;
	fin.open(fileName.c_str(),std::ios::in|std::ios::binary);
	if(fin.is_open())
	{
		fin.close();
		return true;
	}
	fin.close();
	return false;
}

bool copyFile(const std::string& fromFile, const std::string& toFile)
{
	std::fstream fin, fout;
	char buf[1024];

	fin.open(fromFile.c_str(),std::ios::in|std::ios::binary);
	fout.open(toFile.c_str(),std::ios::out|std::ios::binary);

	cerr << "Copying "<<fromFile<<" to "<<toFile<<endl;
	while(!fin.eof()) 
    {
		memset(buf,0,1024);
		fin.read(buf,1024);
        fout.write(buf,1024);
    }

	fin.close();
	fout.close();
	return true;
}

// Delete the file
bool deleteFile(const std::string& fileName)
{
	if (std::remove(fileName.c_str())==-1)
	{
		return false;
	}
	return true;
}

bool checkAbsolutePath(const string& fileName)
{
	// Absolute path if starts by '/' or by 'Drive:/' (MS)

	if (fileName!="")
	{
		if (fileName[0]=='/')
			return true;
		if (fileName[1] == ':' && (fileName[2] == '/' || fileName[2]=='\\'))
			return true;
	}
	return false;
}

// Check if a number is a power of 2
bool isPowerOfTwo (int value) {return (value & -value) == value;}

// Return the first power of two bigger than the given value 
int getBiggerPowerOfTwo(int value)
{
	int p=1;
	while (p<value)
		p<<=1;
	return p;
}

//! Download the file from the given URL to the given name using libcurl
bool downloadFile(const std::string& url, const std::string& fullPath, 
	const std::string& referer, const string& cookiesFile)
{
#ifndef HAVE_LIBCURL
	cerr << "Stellarium was compiled without libCurl support. Can't access remote URLs." << endl;
	return false;
#else
	// Download the file using libCurl
	CURL* handle = curl_easy_init();
	curl_easy_setopt(handle, CURLOPT_URL, url.c_str());
	curl_easy_setopt(handle, CURLOPT_REFERER, referer.c_str());
	curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 1);
	curl_easy_setopt(handle, CURLOPT_SSL_VERIFYPEER, 0);
	if (!cookiesFile.empty())
	{
		// Activate cookies
		curl_easy_setopt(handle, CURLOPT_COOKIEFILE, cookiesFile.c_str());
		cerr << "Using cookies file: " << cookiesFile << endl;
	}
	FILE* fic = fopen(fullPath.c_str(), "wb");
	if (!fic)
	{
		cerr << "Can't create file: " << fullPath << endl;
		return false;
	}
	curl_easy_setopt(handle, CURLOPT_WRITEDATA, fic);
	//curl_easy_setopt(handle, CURLOPT_VERBOSE, 1);
	if (curl_easy_perform(handle)!=0)
	{
		cerr << "There was an error while getting file: " << url << endl;
		fclose(fic);
		curl_easy_cleanup(handle);
		return false;
	}
	fclose(fic);
	curl_easy_cleanup(handle);
	return true;
#endif
}
	
	// Return the inverse sinus hyperbolic of z
double asinh(double z)
{
	return std::log(z+std::sqrt(z*z+1));
}
} // end of the StelUi namespace

bool argsHaveOption(vector<string>& args, string shortOpt, string longOpt, bool modify=true)
{
	bool result=false;
	vector<string>::iterator lastOpt = find(args.begin(), args.end(), "--");

	vector<string>::iterator foundShort = find(args.begin(), lastOpt, shortOpt);
	if (foundShort != lastOpt)
	{
		result=true;
		if (modify)
			args.erase(foundShort);
	}

	vector<string>::iterator foundLong = find(args.begin(), lastOpt, longOpt);
	if (foundLong != lastOpt)
	{
		result=true;
		if (modify)
			args.erase(foundLong);
	}

	return result;
}

// convert string int ISO 8601-like format [+/-]YYYY-MM-DDThh:mm:ss (no timzone offset)
// to julian day
int string_to_jday(string date, double &jd)
{
	char tmp;
	int year, month, day, hour, minute, second;
	year = month = day = hour = minute = second = 0;

	std::istringstream dstr( date );

	// TODO better error checking
	dstr >> year >> tmp >> month >> tmp >> day >> tmp >> hour >> tmp >> minute >> tmp >> second;

	// bounds checking (per s_tui time object)
	if( year > 100000 || year < -100000 ||
	        month < 1 || month > 12 ||
	        day < 1 || day > 31 ||
	        hour < 0 || hour > 23 ||
	        minute < 0 || minute > 59 ||
	        second < 0 || second > 59) return 0;


	// code taken from s_tui.cpp
	if (month <= 2)
	{
		year--;
		month += 12;
	}

	// Correct for the lost days in Oct 1582 when the Gregorian calendar
	// replaced the Julian calendar.
	int B = -2;
	if (year > 1582 || (year == 1582 && (month > 10 || (month == 10 && day >= 15))))
	{
		B = year / 400 - year / 100;
	}

	jd = ((floor(365.25 * year) +
	       floor(30.6001 * (month + 1)) + B + 1720996.5 +
	       day + hour / 24.0 + minute / 1440.0 + second / 86400.0));

	return 1;
}

/* Calculate the julian day from a calendar day.
 * Valid for positive and negative years but not for negative JD.
 * Formula 7.1 on pg 61 */
 // Code originally from libnova which appeared to be totally wrong... New code from celestia
double get_julian_day (const ln_date * cdate)
{
	ln_date date;
	int y, m, B;

	date = *cdate;

	y = date.years;
	m = date.months;
	if (date.months <= 2)
	{
		y = date.years - 1;
		m = date.months + 12;
	}

    // Correct for the lost days in Oct 1582 when the Gregorian calendar
    // replaced the Julian calendar.
	B = -2;
	if (date.years > 1582 || (date.years == 1582 && (date.months > 10 || (date.months == 10 && date.days >= 15))))
	{
		B = y / 400 - y / 100;
	}

	return (floor(365.25 * y) +
			floor(30.6001 * (m + 1)) + B + 1720996.5 +
			date.days + date.hours / 24.0 + date.minutes / 1440.0 + date.seconds / 86400.0);
}

// Calculate tm struct from julian day
void get_tm_from_julian(double JD, struct tm * tm_time)
{
	ln_date date;
	get_date(JD, &date);
	tm_time->tm_sec = (int)date.seconds;
	tm_time->tm_min = date.minutes;
	tm_time->tm_hour = date.hours;
	tm_time->tm_mday = date.days;
	tm_time->tm_mon = date.months - 1;
	tm_time->tm_year = date.years - 1900;
	tm_time->tm_isdst = -1;
}

/* Calculate the date from the Julian day
 * params : JD Julian day, date Pointer to new calendar date. */
 // Code originally from libnova which appeared to be totally wrong... New code from celestia
void get_date(double jd, ln_date * date)
{
	int a, d, e, f;
	double c, b;
	double dday, dhour, dminute;

	a = (int) (jd + 0.5);

	if (a < 2299161)
	{
		c = a + 1524;
	}
	else
	{
		b = (int) ((a - 1867216.25) / 36524.25);
		c = a + b - (int) (b / 4) + 1525;
	}

	d = (int) ((c - 122.1) / 365.25);
	e = (int) (365.25 * d);
	f = (int) ((c - e) / 30.6001);

	dday = c - e - (int) (30.6001 * f) + ((jd + 0.5) - (int) (jd + 0.5));

    /* This following used to be 14.0, but gcc was computing it incorrectly, so
	it was changed to 14 */
	date->months = f - 1 - 12 * (int) (f / 14);
	date->years = d - 4715 - (int) ((7.0 + date->months) / 10.0);
	date->days = (int) dday;

	dhour = (dday - date->days) * 24;
	date->hours = (int) dhour;

	dminute = (dhour - date->hours) * 60;
	date->minutes = (int) dminute;

	date->seconds = (dminute - date->minutes) * 60;
}


/* Calculate gmt date from system date.
 * param : date Pointer to store date. */
void get_ln_date_from_sys(ln_date * date)
{
	time_t rawtime;
	struct tm * ptm;

	/* get current time */
	time ( &rawtime );

	/* convert to gmt time representation */
	ptm = gmtime ( &rawtime );

	/* fill in date struct */
	date->seconds = ptm->tm_sec;
	date->minutes = ptm->tm_min;
	date->hours = ptm->tm_hour;
	date->days = ptm->tm_mday;
	date->months = ptm->tm_mon + 1;
	date->years = ptm->tm_year + 1900;
}

/* Calculate julian day from system time. */
double get_julian_from_sys(void)
{
	ln_date date;
	/* get sys date */
	get_ln_date_from_sys(&date);
	return get_julian_day(&date);
}

// Calculate time_t from julian day
time_t get_time_t_from_julian(double JD)
{
	struct tm loctime;
	ln_date date;

	get_date(JD, &date);

	loctime.tm_sec = (int)date.seconds;
	loctime.tm_min = date.minutes;
	loctime.tm_hour = date.hours;
	loctime.tm_mday =date.days;
	loctime.tm_mon = date.months -1;
	loctime.tm_year = date.years - 1900;
	loctime.tm_isdst = -1;

	return mktime(&loctime);
}


// Return the number of hours to add to gmt time to get the local time in day JD
// taking the parameters from system. This takes into account the daylight saving
// time if there is. (positive for Est of GMT)
// TODO : %z in strftime only works on GNU compiler
// Fixed 31-05-2004 Now use the extern variables set by tzset()
float get_GMT_shift_from_system(double JD, bool _local)
{
	/* Doesn't seem like MACOSX is a special case... ??? rob
#if defined( MACOSX ) || defined(WIN32)
	struct tm *timeinfo;
	time_t rawtime; time(&rawtime);
	timeinfo = localtime(&rawtime);
	return (float)timeinfo->tm_gmtoff/3600 + (timeinfo->tm_isdst!=0); 
#else */

#if !defined(WIN32)

	struct tm * timeinfo;

	if(!_local)
	{
		// JD is UTC
		struct tm rawtime;
		get_tm_from_julian(JD, &rawtime);
		
#ifdef HAVE_TIMEGM
		time_t ltime = timegm(&rawtime);
#elif HAVE_MKTIME
		time_t ltime = mktime(&rawtime);
#else
		// This does not work
		time_t ltime = my_timegm(&rawtime);
#endif
		
		timeinfo = localtime(&ltime);
	} else {
		time_t rtime;
		rtime = get_time_t_from_julian(JD);
		timeinfo = localtime(&rtime);
	}

	static char heure[20];
	heure[0] = '\0';

	StelUtils::my_strftime(heure, 19, "%z", timeinfo);
	
	heure[5] = '\0';
	float min = 1.f/60.f * atoi(&heure[3]);
	heure[3] = '\0';
	return min + atoi(heure);
#else
     struct tm *timeinfo;
     time_t rawtime;
	 time(&rawtime);
	 timeinfo = localtime(&rawtime);
	 return -(float)timezone/3600 + (timeinfo->tm_isdst!=0);
#endif
}

// Return the time zone name taken from system locale
wstring get_time_zone_name_from_system(double JD)
{
	// Windows will crash if date before 1970
	// And no changes on Linux before that year either
	// TODO: ALSO, on Win XP timezone never changes anyway??? 
	if(JD < 2440588 ) JD = 2440588;

	// The timezone name depends on the day because of the summer time
	time_t rawtime = get_time_t_from_julian(JD);

	struct tm * timeinfo;
	timeinfo = localtime(&rawtime);
	static char timez[255];
	timez[0] = 0;
	StelUtils::my_strftime(timez, 254, "%Z", timeinfo);
	return StelUtils::stringToWstring(timez);
}

// Return the time in ISO 8601 format that is : %Y-%m-%d %H:%M:%S
string get_ISO8601_time_UTC(double JD)
{
	struct tm time_utc;
	get_tm_from_julian(JD, &time_utc);

	static char isotime[255];
	StelUtils::my_strftime(isotime, 254, "%Y-%m-%d %H:%M:%S", &time_utc);
	return isotime;
}

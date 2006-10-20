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


#include <math.h> // fmod
#include <sstream>
#include <fstream>
#include <cstdlib>
#include <ctime>

#if defined( CYGWIN )
 #include <malloc.h>
#endif

#include "stel_utility.h"
#include "translator.h"
#include "vecmath.h"

namespace StelUtils {
	
	//! Dummy wrapper used to remove a boring warning when using strftime directly
	size_t my_strftime(char *s, size_t max, const char *fmt, const struct tm *tm)
	{
		return strftime(s, max, fmt, tm);
	}

	wstring stringToWstring(const string& s)
	{
		return Translator::UTF8stringToWstring(s);
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
	
	double hms_to_rad( unsigned int h, unsigned int m, double s )
	{
		return (double)M_PI/24.*h*2.+(double)M_PI/12.*m/60.+s*M_PI/43200.;
	}
	
	double dms_to_rad(int d, int m, double s)
	{
		return (double)M_PI/180.*d+(double)M_PI/10800.*m+s*M_PI/648000.;
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
	
		
	//! @brief Print the passed angle with the format ddÃÆÃâÃâÃÂ°mm'ss(.ss)"
	//! @param angle Angle in radian
	//! @param decimal Define if 2 decimal must also be printed
	//! @param useD Define if letter "d" must be used instead of ÃÂ°
	//! @return The corresponding string
	wstring printAngleDMS(double angle, bool decimals, bool useD)
	{
		wchar_t buf[32];
		buf[31]=L'\0';
		wchar_t sign = L'+';
	// wchar_t degsign = L'ÃÂ°'; ???
		wchar_t degsign = L'\u00B0';
		if (useD) degsign = L'd';

		angle *= 180./M_PI;

		if (angle<0) {
			angle *= -1;
			sign = '-';
		}

		if (decimals) {
			int d = (int)(0.5+angle*(60*60*100));
			const int centi = d % 100;
			d /= 100;
			const int s = d % 60;
			d /= 60;
			const int m = d % 60;
			d /= 60;
			swprintf(buf,
#ifndef MINGW32
				sizeof(buf),
#endif
				L"%lc%.2d%lc%.2d'%.2d.%02d\"",
				sign, d, degsign, m, s, centi);
		} else {
			int d = (int)(0.5+angle*(60*60));
			const int s = d % 60;
			d /= 60;
			const int m = d % 60;
			d /= 60;
			swprintf(buf,
#ifndef MINGW32
				sizeof(buf),
#endif
				L"%lc%.2d%lc%.2d'%.2d\"",
				sign, d, degsign, m, s);
		}
		return buf;
	}
	
	//! @brief Print the passed angle with the format +hhhmmmss(.ss)"
	//! @param angle Angle in radian
	//! @param decimals Define if 2 decimal must also be printed
	//! @return The corresponding string
	wstring printAngleHMS(double angle, bool decimals)
	{
		wchar_t buf[16];
		buf[15] = L'\0';
		angle = fmod(angle,2.0*M_PI);
		if (angle < 0.0) angle += 2.0*M_PI; // range: [0..2.0*M_PI)
		angle *= 12./M_PI; // range: [0..24)
		if (decimals) {
			angle = 0.5+angle*(60*60*100); // range:[0.5,24*60*60*100+0.5)
			if (angle >= (24*60*60*100)) angle -= (24*60*60*100);
			int h = (int)angle;
			const int centi = h % 100;
			h /= 100;
			const int s = h % 60;
			h /= 60;
			const int m = h % 60;
			h /= 60;
			swprintf(buf,
#ifndef MINGW32
				sizeof(buf),
#endif
				L"%.2dh%.2dm%.2d.%02ds",h,m,s,centi);
		} else {
			angle = 0.5+angle*(60*60); // range:[0.5,24*60*60+0.5)
			if (angle >= (24*60*60)) angle -= (24*60*60);
			int h = (int)angle;
			const int s = h % 60;
			h /= 60;
			const int m = h % 60;
			h /= 60;
			swprintf(buf,
#ifndef MINGW32
				sizeof(buf),
#endif
				L"%.2dh%.2dm%.2ds",h,m,s);
		}
		return buf;
	}
	
	
	double str_to_double(string str)
	{
		if(str=="") return 0;
		double dbl;
		std::istringstream dstr( str );

		dstr >> dbl;
		return dbl;
	}

// always positive
	double str_to_pos_double(string str)
	{
		if(str=="") return 0;
		double dbl;
		std::istringstream dstr( str );

		dstr >> dbl;
		if(dbl < 0 ) dbl *= -1;
		return dbl;
	}


	int str_to_int(string str)
	{
		if(str=="") return 0;
		int integer;
		std::istringstream istr( str );

		istr >> integer;
		return integer;
	}


	int str_to_int(string str, int default_value)
	{
		if(str=="") return default_value;
		int integer;
		std::istringstream istr( str );

		istr >> integer;
		return integer;
	}

	string double_to_str(double dbl)
	{
		std::ostringstream oss;
		oss << dbl;
		return oss.str();
	}

	long int str_to_long(string str)
	{
		if(str=="") return 0;
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
		fin.open(fileName.c_str(),std::ios::in);
		if(fin.is_open())
		{
			fin.close();
			return true;
		}
		fin.close();
		return false;
	}
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

#if !defined(MINGW32)

	struct tm * timeinfo;

	if(!_local)
	{
		// JD is UTC
		struct tm rawtime;
		get_tm_from_julian(JD, &rawtime);
		
#ifdef HAVE_TIMEGM
		time_t ltime = timegm(&rawtime);
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


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

#ifdef CYGWIN
 #include <malloc.h>
#endif

#ifdef HAVE_LIBCURL
 #include <curl/curl.h>
#endif

#include "StelUtils.hpp"
#include "vecmath.h"
#include "GLee.h"

#include "fixx11h.h"
#include <QString>
#include <QTextStream>

namespace StelUtils {

//! Convert from UTF-8 to wchar_t
std::wstring stringToWstring(const string& s)
{
	return QString::fromUtf8(s.c_str()).toStdWString();
}

string wstringToString(const wstring& ws)
{
	return QString::fromStdWString(ws).toUtf8().constData();
}

wstring doubleToWstring(double d)
{
	return QString::number(d).toStdWString();
}

wstring intToWstring(int i)
{
	return QString::number(i).toStdWString();
}

string intToString(int i)
{
	return QString::number(i).toStdString();
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
	QString degsign('d');
	if (!useD)
	{
		const wchar_t degsignw = L'\u00B0';
		degsign = QString::fromWCharArray(&degsignw, 1);
	}
	bool sign;
	unsigned int d,m;
	double s;
	StelUtils::radToDms(angle+0.005*M_PI/180/(60*60)*(angle<0?-1.:1.), sign, d, m, s);
	QString str;
	QTextStream os(&str);
	
	os << (sign?'+':'-') << d << degsign;
	if (std::fabs(s*100-(int)s*100)>=1)
	{
		os << m << '\'' << fixed << qSetRealNumberPrecision(2) << qSetFieldWidth(5) << qSetPadChar('0') << s << qSetFieldWidth(0) << '\"';
	}
	else if ((int)s!=0)
	{
		os << m << '\'' << (int)s << '\"';
	}
	else if (m!=0)
	{
		os << m << '\'';
	}
	return str.toStdWString();
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
	QString degsign('d');
	if (!useD)
	{
		const wchar_t degsignw = L'\u00B0';
		degsign = QString::fromWCharArray(&degsignw, 1);
	}
	bool sign;
	unsigned int d,m;
	double s;
	StelUtils::radToDms(angle+0.005*M_PI/180/(60*60)*(angle<0?-1.:1.), sign, d, m, s);
	QString str;
	QTextStream os(&str);
	os << (sign?'+':'-') << d << degsign;

	int width = 2;
	if (decimal)
	{
		os << qSetRealNumberPrecision(1);
		width = 4;
	}
	else
	{
		os << qSetRealNumberPrecision(0);
		width = 2;
	}
		
	os << qSetFieldWidth(width) << m << qSetFieldWidth(0) << '\'' 
		<< fixed << qSetFieldWidth(width) << qSetPadChar('0') << s 
		<< qSetFieldWidth(0) << '\"';

	return str.toStdWString();
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
#ifdef NAN
	if(str.empty())
		return NAN;
#else
	if(str.empty())
		return 0.;
#endif
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

/*************************************************************************
 Convert a QT QDateTime class to julian day
*************************************************************************/
double qDateTimeToJd(const QDateTime& dateTime)
{
	return (double)(dateTime.date().toJulianDay())+(double)1./(24*60*60*1000)*QTime().msecsTo(dateTime.time())-0.5;
}

/*************************************************************************
 Convert a julian day to a QT QDateTime class. 
 Warning if JD < 0 the date is invalid (any date before 2 January 4713 B.C.)
*************************************************************************/
QDateTime jdToQDateTime(double jd)
{
	jd+=0.5;
	const int ms = (int)( (jd-(int)jd) *24.*60.*60.*1000. +0.5);
	QDateTime dateTime(QDate::fromJulianDay((int)jd));
	return dateTime.addMSecs(ms).toUTC();
}

/*************************************************************************
 Calculate julian day from system time.
*************************************************************************/
double getJDFromSystem(void)
{
	return StelUtils::qDateTimeToJd(QDateTime::currentDateTime().toUTC());
}

} // end of the StelUi namespace
////////////////////////////////////////////////////////////////////////////////////////////






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
	// Handle also strings with : instead of - in date part
	for(int i=0; i<2; i++)
	{
		string::size_type p = date.find(":", 0);
		if (p != string::npos)
			date.replace(p, 1, "-");
	}

	// handle lack of 0 padding
	if (date.find("-",5) == 6)
		date.replace(5, 0, "0");

	if (date.find("T",7) == 9)
		date.replace(8, 0, "0");

	if (date.find(":",11) == 12)
		date.replace(11, 0, "0");

	if (date.find(":",14) == 15)
		date.replace(14, 0, "0");

	if (date.length() == 18)
		date.replace(17, 0, "0");

	jd = StelUtils::qDateTimeToJd(QDateTime::fromString(date.c_str(), Qt::ISODate));
	return 1;
}


// Calculate tm struct from julian day
void get_tm_from_julian(double JD, struct tm * tm_time)
{
	QDateTime dateTime = StelUtils::jdToQDateTime(JD);
	tm_time->tm_sec = dateTime.time().second();
	tm_time->tm_min = dateTime.time().minute();
	tm_time->tm_hour = dateTime.time().hour();
	tm_time->tm_mday = dateTime.date().day();
	tm_time->tm_mon = dateTime.date().month() - 1;
	tm_time->tm_year = dateTime.date().year() - 1900;
	tm_time->tm_isdst = -1;
}

//! Dummy wrapper used to remove a boring warning when using strftime directly
size_t my_strftime(char *s, size_t max, const char *fmt, const struct tm *tm)
{
	return strftime(s, max, fmt, tm);
}

// Return the number of hours to add to gmt time to get the local time in day JD
// taking the parameters from system. This takes into account the daylight saving
// time if there is. (positive for Est of GMT)
// TODO : %z in strftime only works on GNU compiler
// Fixed 31-05-2004 Now use the extern variables set by tzset()
float get_GMT_shift_from_system(double JD)
{
#if !defined(WIN32)
	struct tm * timeinfo;
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

	static char heure[20];
	heure[0] = '\0';

	my_strftime(heure, 19, "%z", timeinfo);
	
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

// Calculate time_t from julian day
time_t get_time_t_from_julian(double JD)
{
	return StelUtils::jdToQDateTime(JD).toTime_t();
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
	my_strftime(timez, 254, "%Z", timeinfo);
	return StelUtils::stringToWstring(timez);
}

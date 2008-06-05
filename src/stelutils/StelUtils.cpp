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
#include <iomanip>
#include <vector>
#include <algorithm>
#include <stdexcept>

#ifdef CYGWIN
 #include <malloc.h>
#endif

#include "StelUtils.hpp"
#include "vecmath.h"
#include "GLee.h"

#include "fixx11h.h"
#include <QString>
#include <QStringList>
#include <QTextStream>
#include <QFile>
#include <QDebug>
#include <QLocale>

namespace StelUtils
{

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

string underscoresToSpaces(char * c)
{
	string str = c;
	for (string::size_type i=0;i<str.length();++i)
	{
		if (str[i]=='_') str[i]=' ';
	}
	return str;
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
QString radToHmsStrAdapt(double angle)
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
	return QString::fromStdString(os.str());
}

/*************************************************************************
 Convert an angle in radian to a hms formatted string
*************************************************************************/
QString radToHmsStr(double angle, bool decimal)
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
	return QString::fromStdString(os.str());
}

/*************************************************************************
 Convert an angle in radian to a dms formatted wstring
 If the minute and second part are null are too small, don't print them
*************************************************************************/
QString radToDmsStrAdapt(double angle, bool useD)
{
	QChar degsign('d');
	if (!useD)
	{
		degsign = 0x00B0;
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
	return str;
}


/*************************************************************************
 Convert an angle in radian to a dms formatted wstring
*************************************************************************/
QString radToDmsStr(double angle, bool decimal, bool useD)
{
	QChar degsign('d');
	if (!useD)
	{
		degsign = 0x00B0;
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

	return str;
}

// Obtains a Vec3f from a string with the form x,y,z
Vec3f str_to_vec3f(const string& s)
{
	float x, y, z;
	if (s.empty() || (sscanf(s.c_str(),"%f,%f,%f",&x, &y, &z)!=3)) return Vec3f(0.f,0.f,0.f);
	return Vec3f(x,y,z);
}

// Obtains a Vec3f from a string with the form x,y,z
Vec3f str_to_vec3f(const QStringList& s)
{
	if (s.size()<3)
		 return Vec3f(0.f,0.f,0.f);
	return Vec3f(s[0].toFloat(),s[1].toFloat(),s[2].toFloat());
}

Vec3f str_to_vec3f(const QString& s)
{
	return str_to_vec3f(s.toStdString());
}

Vec3f str_to_vec3f(const char* s)
{
	return str_to_vec3f(string(s));
}


// Obtains a string from a Vec3f with the form x,y,z
string vec3f_to_str(const Vec3f& v)
{
	ostringstream os;
	os << v[0] << "," << v[1] << "," << v[2];
	return os.str();
}

// Converts a Vec3f to HTML color notation.
QString vec3fToHtmlColor(const Vec3f& v)
{
	return QString("#%1%2%3")
		.arg(MY_MIN(255, int(v[0] * 255)), 2, 16, QChar('0'))
		.arg(MY_MIN(255, int(v[1] * 255)), 2, 16, QChar('0'))
		.arg(MY_MIN(255, int(v[2] * 255)), 2, 16, QChar('0'));
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

	enum _type
	{
		HOURS, DEGREES, LAT, LONG
	}
	type;

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

double get_dec_angle(const QString& str)
{
	return get_dec_angle(str.toStdString());
}

double get_dec_angle(const char* str)
{
	return get_dec_angle(string(str));
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
bool isPowerOfTwo(int value)
{
	return (value & -value) == value;
}

// Return the first power of two bigger than the given value 
int getBiggerPowerOfTwo(int value)
{
	int p=1;
	while (p<value)
		p<<=1;
	return p;
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

QDateTime jdToQDateTime(const double& jd)
{
	int year, month, day;
	getDateFromJulianDay(jd, &year, &month, &day);
	QDateTime result = QDateTime::fromString(QString("%1.%2.%3").arg(year, 4, QChar('0')).arg(month).arg(day), "yyyy.m.d");
	result.setTime(jdFractionToQTime(jd));
	return result;
}

// based on QDateTime's original handling, but expanded to handle 0.0 and earlier.
void getDateFromJulianDay(double jd, int *year, int *month, int *day)
{
	int y, m, d;

	// put us in the right calendar day for the time of day.
	double fraction = jd - floor(jd);
	if (fraction >= .5)
	{
		jd += 1.0;
	}

	if (jd >= 2299161)
	{
		// Gregorian calendar starting from October 15, 1582
		// This algorithm is from Henry F. Fliegel and Thomas C. Van Flandern
		qulonglong ell, n, i, j;
		ell = qulonglong(floor(jd)) + 68569;
		n = (4 * ell) / 146097;
		ell = ell - (146097 * n + 3) / 4;
		i = (4000 * (ell + 1)) / 1461001;
		ell = ell - (1461 * i) / 4 + 31;
		j = (80 * ell) / 2447;
		d = ell - (2447 * j) / 80;
		ell = j / 11;
		m = j + 2 - (12 * ell);
		y = 100 * (n - 49) + i + ell;
	}
	else
	{
		// Julian calendar until October 4, 1582
		// Algorithm from Frequently Asked Questions about Calendars by Claus Toendering
		int julianDay = (int)floor(jd);
		julianDay += 32082;
		int dd = (4 * julianDay + 3) / 1461;
		int ee = julianDay - (1461 * dd) / 4;
		int mm = ((5 * ee) + 2) / 153;
		d = ee - (153 * mm + 2) / 5 + 1;
		m = mm + 3 - 12 * (mm / 10);
		y = dd - 4800 + (mm / 10);
	}
	*year = y;
	*month = m;
	*day = d;
}

void getTimeFromJulianDay(double julianDay, int *hour, int *minute, int *second)
{
	double frac = julianDay - (floor(julianDay));
	int s = (int)floor(frac * 24 * 60 * 60);

	*hour = ((s / (60 * 60))+12)%24;
	*minute = (s/(60))%60;
	*second = s % 60;
}

QString sixIntsToIsoString( int year, int month, int day, int hour, int minute, int second )
{
	// formatting a negative doesnt work the way i expect

	QString dt = QString("%1-%2-%3T%4:%5:%6")
	             .arg((year >= 0 ? year : -1* year),4,10,QLatin1Char('0'))
	             .arg(month,2,10,QLatin1Char('0'))
	             .arg(day,2,10,QLatin1Char('0'))
	             .arg(hour,2,10,QLatin1Char('0'))
	             .arg(minute,2,10,QLatin1Char('0'))
	             .arg(second,2,10,QLatin1Char('0'));

	if (year < 0)
	{
		dt.prepend("-");
	}
	return dt;
}

QString jdToIsoString(double jd)
{
	int year, month, day, hour, minute, second;
	getDateFromJulianDay(jd, &year, &month, &day);
	getTimeFromJulianDay(jd, &hour, &minute, &second);

	return sixIntsToIsoString(year, month, day, hour, minute, second);
}

// Format the date per the fmt.
QString localeDateString(int year, int month, int day, int dayOfWeek, QString fmt)
{
	/* we have to handle the year zero, and the years before qdatetime can represent. */
	const QLatin1Char quote('\'');
	QString out;
	int quotestartedat = -1;

	for (int i = 0; i < (int)fmt.length(); i++)
	{
		if (fmt.at(i) == quote)
		{
			if (quotestartedat >= 0)
			{
				if ((quotestartedat+1) == i)
				{
					out += quote;
					quotestartedat = -1;
				}
				else
				{
					quotestartedat = -1;
				}
			}
			else
			{
				quotestartedat = i;
			}
		}
		else if (quotestartedat > 0)
		{
			out += fmt.at(i);
		}
		else if (fmt.at(i) == QLatin1Char('d') ||
		         fmt.at(i) == QLatin1Char('M') ||
		         fmt.at(i) == QLatin1Char('y'))
		{
			int j = i+1;
			while ((j < fmt.length()) && (fmt.at(j) == fmt.at(i)) && (4 >= (j-i+1)))
			{
				j++;
			}

			QString frag = fmt.mid(i,(j-i));

			if (frag == "d")
			{
				out += QString("%1").arg(day);
			}
			else if (frag == "dd")
			{
				out += QString("%1").arg(day, 2, 10, QLatin1Char('0'));
			}
			else if (frag == "ddd")
			{
				out += QDate::shortDayName(dayOfWeek+1);
			}
			else if (frag == "dddd")
			{
				out += QDate::longDayName(dayOfWeek+1);
			}
			else if (frag == "M")
			{
				out += QString("%1").arg(month);
			}
			else if (frag == "MM")
			{
				out += QString("%1").arg(month, 2, 10, QLatin1Char('0'));
			}
			else if (frag == "MMM")
			{
				out += QDate::shortMonthName(month);
			}
			else if (frag == "MMMM")
			{
				out += QDate::longMonthName(month);
			}
			else if (frag == "y")
			{
				out += frag;
			}
			else if (frag == "yy")
			{
				int dispyear = year % 100;
				out += QString("%1").arg(dispyear,2,10,QLatin1Char('0'));
			}
			else if (frag == "yyy")
			{
				// assume greedy: understand yy before y.
				int dispyear = year % 100;
				out += QString("%1").arg(dispyear,2,10,QLatin1Char('0'));
				out += QLatin1Char('y');
			}
			else if (frag == "yyyy")
			{
				int dispyear = (year >= 0 ? year : -1 * year);
				if (year <  0)
				{
					out += QLatin1Char('-');
				}
				out += QString("%1").arg(dispyear,4,10,QLatin1Char('0'));
			}

			i = j-1;
		}
		else
		{
			out += fmt.at(i);
		}


	}

	return out;
}

//! try to get a reasonable locale date string from the system, trying to work around
//! limitations of qdatetime for large dates in the past.  see QDateTime::toString().
QString localeDateString(int year, int month, int day, int dayOfWeek)
{

	// try the QDateTime first
	QDate test(year, month, day);

	if (test.isValid() && !test.toString(Qt::LocaleDate).isEmpty())
	{
		return test.toString(Qt::LocaleDate);
	}
	else
	{
		return localeDateString(year,month,day,dayOfWeek,QLocale().dateFormat(QLocale::ShortFormat));
	}
}


//! use QDateTime to get a Julian Date from the system's current time.
//! this is an acceptable use of QDateTime because the system's current
//! time is more than likely always going to be expressible by QDateTime.
double getJDFromSystem(void)
{
	return qDateTimeToJd(QDateTime::currentDateTime().toUTC());
}

double qTimeToJDFraction(const QTime& time)
{
	return (double)1./(24*60*60*1000)*QTime().msecsTo(time)-0.5;
}

QTime jdFractionToQTime(const double jd)
{
	double decHours = fmod(jd+0.5, 1.0);
        int hours = decHours/0.041666666666666666666;
        int mins = (decHours-(hours*0.041666666666666666666))/0.00069444444444444444444;
        return QTime::fromString(QString("%1.%2").arg(hours).arg(mins), "h.m");
}

bool argsHaveOption(vector<string>& args, string shortOpt, string longOpt, bool modify)
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

// Use Qt's own sense of time and offset instead of platform specific code.
float get_GMT_shift_from_QT(double JD)
{
	int year, month, day, hour, minute, second;
	getDateFromJulianDay(JD, &year, &month, &day);
	getTimeFromJulianDay(JD, &hour, &minute, &second);
	QDateTime current(QDate(year, month, day), QTime(hour, minute, second));
	if (! current.isValid())
	{
		qWarning() << "JD " << QString("%1").arg(JD) << " out of bounds of QT help with GMT shift, using current datetime";
		current = QDateTime::currentDateTime();
	}
	QDateTime c1 = QDateTime::fromString(current.toString(Qt::ISODate),Qt::ISODate);
	QDateTime u1 = QDateTime::fromString(current.toUTC().toString(Qt::ISODate),Qt::ISODate);

	int secsto = u1.secsTo(c1);
	float hrsto = secsto / 3600.0f;
	return hrsto;
}
// UTC !
bool getJDFromDate(double* newjd, int y, int m, int d, int h, int min, int s)
{
  double delta_time = (h / 24.0) + (min / (24.0*60.0)) + (s / (24.0 * 60.0 * 60.0)) - 0.5;
  QDate test((y <= 0 ? y-1 : y), m, d);
  // if QDate will oblige, do so.
  if ( test.isValid() ) {
    double qdjd = (double)test.toJulianDay();
    qdjd += delta_time;
    *newjd = qdjd;
    return true;
  } else {
    double jd = (double)((1461 * (y + 4800 + (m - 14) / 12)) / 4 + (367 * (m - 2 - 12 * ((m - 14) / 12))) / 12 - (3 * ((y + 4900 + (m - 14) / 12) / 100)) / 4 + d - 32075) - 38;
    jd += delta_time;
    *newjd = jd;
    return true;
  }
  return false;
}
double getJDFromDate_alg2(int y, int m, int d, int h, int min, int s)
{
  
  double extra = (100.0* y) + m - 190002.5;
  double rjd = 367.0 * y;
  rjd -= floor(7.0*(y+floor((m+9.0)/12.0))/4.0);
  rjd += floor(275.0*m/9.0) ;
  rjd += d;
  rjd += (h + (min + s/60.0)/60.)/24.0;
  rjd += 1721013.5;
  rjd -= 0.5*extra/abs(extra);
  rjd += 0.5;
  return rjd;
}

  int numberOfDaysInMonthInYear(int month, int year)
{
  switch(month) {
  case 1:
  case 3:
  case 5:
  case 7:
  case 8:
  case 10:
  case 12:
    return 31;
    break;
  case 4:
  case 6:
  case 9:
  case 11:
    return 30;
    break;

  case 2:
    if ( year > 1582 ) {
      if ( year % 4 == 0 ) {
	if ( year % 100 == 0 ) {
	  if ( year % 400 == 0 ) {
	    return 29;
	  } else {
	    return 28;
	  }
	} else {
	  return 29;
	}
      } else {
	return 28;
      }
    } else {
      if ( year % 4 == 0 ) {
	return 29;
      } else {
	return 28;
      }
    }

    break;

  case 0:
    return numberOfDaysInMonthInYear(12, year-1);
    break;
  case 13:
    return numberOfDaysInMonthInYear(1, year+1);
    break;

  default:
    break;
  }

  return 0;
}

//! given the submitted year/month/day hour:minute:second, try to
//! normalize into an actual year/month/day.  values can be positive, 0,
//! or negative.  start assessing from seconds to larger increments.

bool changeDateTimeForRollover(int oy, int om, int od, int oh, int omin, int os,
			       int* ry, int* rm, int* rd, int* rh, int* rmin, int* rs)
{
  bool change = false;

  while ( os > 59 ) {
    os -= 60;
    omin += 1;
    change = true;
  }
  while ( os < 0 ) {
    os += 60;
    omin -= 1;
    change = true;
  }

  while (omin > 59 ) {
    omin -= 60;
    oh += 1;
    change = true;
  }
  while (omin < 0 ) {
    omin += 60;
    oh -= 1;
    change = true;
  }

  while ( oh > 23 ) {
    oh -= 24;
    od += 1;
    change = true;
  }
  while ( oh < 0 ) {
    oh += 24;
    od -= 1;
    change = true;
  }

  while ( od > numberOfDaysInMonthInYear(om, oy) ) {
    od -= numberOfDaysInMonthInYear(om, oy);
    om++;
    if ( om > 12 ) {
      om -= 12;
      oy += 1;
    }
    change = true;
  }
  while ( od < 1 ) {
    od += numberOfDaysInMonthInYear(om-1,oy);
    om--;
    if ( om < 1 ) {
      om += 12;
      oy -= 1;
    }
    change = true;
  }

  while ( om > 12 ) {
    om -= 12;
    oy += 1;
    change = true;
  }
  while ( om < 1 ) {
    om += 12;
    oy -= 1;
    change = true;
  }

  // and the julian-gregorian epoch hole: round up to the 15th
  if ( oy == 1582 && om == 10 && ( od > 4 && od < 15 ) ) {
    od = 15;
    change = true;
  }
  
  if ( change ) {
    *ry = oy;
    *rm = om;
    *rd = od;
    *rh = oh;
    *rmin = omin;
    *rs = os;
  }
  return change;
}


} // end of the StelUtils namespace


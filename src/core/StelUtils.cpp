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
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */

#ifdef CYGWIN
 #include <malloc.h>
#endif

#include "StelUtils.hpp"
#include "VecMath.hpp"
#include <QBuffer>
#include <QString>
#include <QStringList>
#include <QTextStream>
#include <QFile>
#include <QDebug>
#include <QLocale>
#include <QRegExp>
#include <QProcess>
#include <QSysInfo>
#include <cmath> // std::fmod
#include <zlib.h>

namespace StelUtils
{
//! Return the full name of stellarium, i.e. "stellarium 0.9.0"
QString getApplicationName()
{
	return QString("Stellarium")+" "+StelUtils::getApplicationVersion();
}

//! Return the version of stellarium, i.e. "0.9.0"
QString getApplicationVersion()
{
#ifdef BZR_REVISION
	return QString(PACKAGE_VERSION)+" (BZR r"+BZR_REVISION+")";
#elif DEV_VERSION
	return QString(DEV_VERSION);
#else
	return QString(PACKAGE_VERSION);
#endif
}

QString getOperatingSystemInfo()
{
	QString OS = "Unknown operating system";

	#ifdef Q_OS_WIN
	switch(QSysInfo::WindowsVersion)
	{
		case QSysInfo::WV_95:
			OS = "Windows 95";
			break;
		case QSysInfo::WV_98:
			OS = "Windows 98";
			break;
		case QSysInfo::WV_Me:
			OS = "Windows Me";
			break;
		case QSysInfo::WV_NT:
			OS = "Windows NT";
			break;
		case QSysInfo::WV_2000:
			OS = "Windows 2000";
			break;
		case QSysInfo::WV_XP:
			OS = "Windows XP";
			break;
		case QSysInfo::WV_2003:
			OS = "Windows Server 2003";
			break;
		case QSysInfo::WV_VISTA:
			OS = "Windows Vista";
			break;
		case QSysInfo::WV_WINDOWS7:
			OS = "Windows 7";
			break;
		case QSysInfo::WV_WINDOWS8:
			OS = "Windows 8";
			break;
		#if QT_VERSION >= 0x050200
		case QSysInfo::WV_WINDOWS8_1:
			OS = "Windows 8.1";
			break;
		#endif
		#if QT_VERSION >= 0x050500
		case QSysInfo::WV_WINDOWS10:
			OS = "Windows 10";
			break;
		#endif
		default:
			OS = "Unsupported Windows version";
			break;
	}

	// somebody writing something useful for Macs would be great here
	#elif defined Q_OS_MAC
	switch(QSysInfo::MacintoshVersion)
	{
		case QSysInfo::MV_PANTHER:
			OS = "Mac OS X 10.3 series";
			break;
		case QSysInfo::MV_TIGER:
			OS = "Mac OS X 10.4 series";
			break;
		case QSysInfo::MV_LEOPARD:
			OS = "Mac OS X 10.5 series";
			break;
		case QSysInfo::MV_SNOWLEOPARD:
			OS = "Mac OS X 10.6 series";
			break;
		case QSysInfo::MV_LION:
			OS = "Mac OS X 10.7 series";
			break;
		case QSysInfo::MV_MOUNTAINLION:
			OS = "Mac OS X 10.8 series";
			break;
		case QSysInfo::MV_MAVERICKS:
			OS = "Mac OS X 10.9 series";
			break;
		case QSysInfo::MV_YOSEMITE:
			OS = "Mac OS X 10.10 series";
			break;
		#if QT_VERSION >= 0x050500
		case QSysInfo::MV_ELCAPITAN:
			OS = "Mac OS X 10.11 series";
			break;
		#endif
		default:
			OS = "Unsupported Mac version";
			break;
	}

	#elif defined Q_OS_LINUX
	QFile procVersion("/proc/version");
	if(!procVersion.open(QIODevice::ReadOnly | QIODevice::Text))
		OS = "Unknown Linux version";
	else
	{
		QString version = procVersion.readAll();
		if(version.right(1) == "\n")
			version.chop(1);
		OS = version;
		procVersion.close();
	}
	#elif defined Q_OS_BSD4
	// Check FreeBSD, NetBSD, OpenBSD and DragonFly BSD
	QProcess uname;
	uname.start("/usr/bin/uname -srm");
	uname.waitForStarted();
	uname.waitForFinished();
	const QString BSDsystem = uname.readAllStandardOutput();
	OS = BSDsystem.trimmed();
	#endif

	return OS;
}

double hmsToRad(const unsigned int h, const unsigned int m, const double s )
{
	//return (double)M_PI/24.*h*2.+(double)M_PI/12.*m/60.+s*M_PI/43200.; // Wrong formula! --AW
	return (double)h*M_PI/12.+(double)m*M_PI/10800.+(double)s*M_PI/648000.;
}

double dmsToRad(const int d, const unsigned int m, const double s)
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
	angle = std::fmod(angle,2.0*M_PI);
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
	angle = std::fmod(angle,2.0*M_PI);
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
	// workaround for rounding numbers	
	if (s>59.9)
	{
		s = 0.;
		m += 1;
	}
	if (m==60)
	{
		m = 0.;
		d += 1;
	}	
}

void radToDecDeg(double rad, bool &sign, double &deg)
{
	rad = std::fmod(rad,2.0*M_PI);
	sign=true;
	if (rad<0)
	{
		rad *= -1;
		sign = false;
	}
	deg = rad*180./M_PI;
}

QString radToDecDegStr(const double angle, const int precision, const bool useD, const bool useC)
{
	QChar degsign('d');
	QString str;	
	if (!useD)
	{
		degsign = 0x00B0;		
	}
	bool sign;
	double deg;
	StelUtils::radToDecDeg(angle, sign, deg);
	str = QString("%1%2%3").arg((sign?"+":"-"), QString::number(deg, 'f', precision), degsign);
	if (useC)
	{
		if (!sign)
			deg = 360. - deg;

		str = QString("+%1%2").arg(QString::number(deg, 'f', precision), degsign);
	}

	return str;
}

/*************************************************************************
 Convert an angle in radian to a hms formatted string
 If the minute and second part are null are too small, don't print them
*************************************************************************/
QString radToHmsStrAdapt(const double angle)
{
	unsigned int h,m;
	double s;
	QString buf;
	QTextStream ts(&buf);
	StelUtils::radToHms(angle+0.005*M_PI/12/(60*60), h, m, s);
	ts << h << 'h';
	if (std::fabs(s*100-(int)s*100)>=1)
	{
		ts << m << 'm';
		ts.setRealNumberNotation(QTextStream::FixedNotation);
		ts.setPadChar('0');
		ts.setFieldWidth(4);
		ts.setRealNumberPrecision(1);
		ts << s;
		ts.reset();
		ts << 's';
	}
	else if ((int)s!=0)
	{
		ts << m << 'm' << (int)s << 's';
	}
	else if (m!=0)
	{
		ts << m << 'm';
	}
	return buf;
}

/*************************************************************************
 Convert an angle in radian to a hms formatted string
 If decimal is true,  output should be like this: "  16h29m55.3s"
 If decimal is true,  output should be like this: "  16h20m0.4s"
 If decimal is false, output should be like this: "0h26m5s"
*************************************************************************/
QString radToHmsStr(const double angle, const bool decimal)
{
	unsigned int h,m;
	double s;
	StelUtils::radToHms(angle+0.005*M_PI/12/(60*60), h, m, s);
	int width, precision;
	QString carry;
	if (decimal)
	{
		width=5;
		precision=2;
		carry="60.00";
	}
	else
	{
		width=4;
		precision=1;
		carry="60.0";
	}

	// handle carry case (when seconds are rounded up)
	if (QString("%1").arg(s, 0, 'f', precision) == carry)
	{
		s=0;
		m+=1;
	}
	if (m==60)
	{
		m=0;
		h+=1;
	}
	if (h==24 && m==0 && s==0)
		h=0;

	return QString("%1h%2m%3s").arg(h, width).arg(m,2,10,QLatin1Char('0')).arg(s, 0, 'f', precision);
}

/*************************************************************************
 Convert an angle in radian to a dms formatted string
 If the minute and second part are null are too small, don't print them
*************************************************************************/
QString radToDmsStrAdapt(const double angle, const bool useD)
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
	//qDebug() << "radToDmsStrAdapt(" << angle << ", " << useD << ") = " << str;
	return str;
}


/*************************************************************************
 Convert an angle in radian to a dms formatted string
*************************************************************************/
QString radToDmsStr(const double angle, const bool decimal, const bool useD)
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

	os << qSetFieldWidth(2) << qSetPadChar('0') << m << qSetFieldWidth(0) << '\'';
	int width;
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
	os << fixed << qSetFieldWidth(width) << qSetPadChar('0') << s << qSetFieldWidth(0) << '\"';

	return str;
}

void decDegToDms(double angle, bool &sign, unsigned int &d, unsigned int &m, double &s)
{
	sign = true;
	if (angle<0.)
	{
		sign = false;
		angle *= -1;
	}

	d = (unsigned int)angle;
	m = (unsigned int)((angle-d)*60);
	s = (angle-d)*3600.-60.*m;

	if (s==60.)
	{
		s = 0.;
		m += 1;
	}
	if (m==60)
	{
		m = 0;
		d += 1;
	}
}

// Convert an angle in decimal degrees to a dms formatted string
QString decDegToDmsStr(const double angle)
{
	bool sign;
	double s;
	unsigned int d, m;
	decDegToDms(angle, sign, d, m, s);
	return QString("%1%2%3%4\'%5\"").arg(sign?'+':'-').arg(d).arg(QChar(0x00B0)).arg(m,2,10,QLatin1Char('0')).arg((unsigned int)s,2,10,QLatin1Char('0'));
}

// Convert a dms formatted string to an angle in radian
double dmsStrToRad(const QString& s)
{
	QRegExp reg("([\\+\\-])(\\d+)d(\\d+)'(\\d+)\"");
	if (!reg.exactMatch(s))
		return 0;
	QStringList list = reg.capturedTexts();
	bool sign = (list[1] == "+");
	int deg = list[2].toInt();
	int min = list[3].toInt();
	int sec = list[4].toInt();

	return dmsToRad(sign ? deg : -deg, min, sec);
}

// Obtains a Vec3f from a string with the form x,y,z
Vec3f strToVec3f(const QStringList& s)
{
	if (s.size()<3)
		 return Vec3f(0.f,0.f,0.f);

	return Vec3f(s[0].toFloat(),s[1].toFloat(),s[2].toFloat());
}

Vec3f strToVec3f(const QString& s)
{
	return strToVec3f(s.split(","));
}

Vec4d strToVec4d(const QStringList &s)
{
	if(s.size()<4)
		return Vec4d(0.0,0.0,0.0,0.0);

	return Vec4d(s[0].toDouble(), s[1].toDouble(), s[2].toDouble(), s[3].toDouble());
}

Vec4d strToVec4d(const QString& str)
{
	return strToVec4d(str.split(","));
}

QString vec3fToStr(const Vec3f &v)
{
	return QString("%1,%2,%3")
		.arg(v[0],0,'f',6)
		.arg(v[1],0,'f',6)
		.arg(v[2],0,'f',6);
}

QString vec4dToStr(const Vec4d &v)
{
	return QString("%1,%2,%3,%4")
		.arg(v[0],0,'f',10)
		.arg(v[1],0,'f',10)
		.arg(v[2],0,'f',10)
		.arg(v[3],0,'f',10);
}

// Converts a Vec3f to HTML color notation.
QString vec3fToHtmlColor(const Vec3f& v)
{
	return QString("#%1%2%3")
		.arg(qMin(255, int(v[0] * 255)), 2, 16, QChar('0'))
		.arg(qMin(255, int(v[1] * 255)), 2, 16, QChar('0'))
		.arg(qMin(255, int(v[2] * 255)), 2, 16, QChar('0'));
}

Vec3f htmlColorToVec3f(const QString& c)
{
	Vec3f v;
	QRegExp re("^#([0-9a-fA-F]{2})([0-9a-fA-F]{2})([0-9a-fA-F]{2})$");
	if (re.exactMatch(c))
	{
		bool ok;
		int i = re.capturedTexts().at(1).toInt(&ok, 16);
		v[0] = (float)i / 255.;
		i = re.capturedTexts().at(2).toInt(&ok, 16);
		v[1] = (float)i / 255.;
		i = re.capturedTexts().at(3).toInt(&ok, 16);
		v[2] = (float)i / 255.;
	}
	else
	{
		v[0] = 0.;
		v[1] = 0.;
		v[2] = 0.;
	}
	return v;
}

void spheToRect(const double lng, const double lat, Vec3d& v)
{
	const double cosLat = cos(lat);
	v.set(cos(lng) * cosLat, sin(lng) * cosLat, sin(lat));
}

void spheToRect(const float lng, const float lat, Vec3f& v)
{	
	const double dlng = lng, dlat = lat, cosLat = cos(dlat);
	v.set(cos(dlng) * cosLat, sin(dlng) * cosLat, sin(dlat));
}

void rectToSphe(double *lng, double *lat, const Vec3d& v)
{
	double r = v.length();
	*lat = asin(v[2]/r);
	*lng = atan2(v[1],v[0]);
}

void rectToSphe(float *lng, float *lat, const Vec3d& v)
{
	double r = v.length();
	*lat = asin(v[2]/r);
	*lng = atan2(v[1],v[0]);
}

void rectToSphe(float *lng, float *lat, const Vec3f& v)
{
	float r = v.length();
	*lat = asin(v[2]/r);
	*lng = atan2(v[1],v[0]);
}

void rectToSphe(double *lng, double *lat, const Vec3f& v)
{
	double r = v.length();
	*lat = asin(v[2]/r);
	*lng = atan2(v[1],v[0]);
}

void equToEcl(const double raRad, const double decRad, const double eclRad, double *lambdaRad, double *betaRad)
{
	*lambdaRad=std::atan2(std::sin(raRad)*std::cos(eclRad)+std::tan(decRad)*std::sin(eclRad), std::cos(raRad));
	*betaRad=std::asin(std::sin(decRad)*std::cos(eclRad)-std::cos(decRad)*std::sin(eclRad)*std::sin(raRad));
}

void eclToEqu(const double lambdaRad, const double betaRad, const double eclRad, double *raRad, double *decRad)
{
	*raRad = std::atan2(std::sin(lambdaRad)*std::cos(eclRad)-std::tan(betaRad)*std::sin(eclRad), std::cos(lambdaRad));
	*decRad = std::asin(std::sin(betaRad)*std::cos(eclRad)+std::cos(betaRad)*std::sin(eclRad)*std::sin(lambdaRad));
}

double getDecAngle(const QString& str)
{
	QRegExp re1("^\\s*([\\+\\-])?\\s*(\\d+)\\s*([hHDd\xBA])\\s*(\\d+)\\s*['Mm]\\s*(\\d+(\\.\\d+)?)\\s*[\"Ss]\\s*([NSEWnsew])?\\s*$"); // DMS/HMS
	QRegExp re2("^\\s*([\\+\\-])?\\s*(\\d+(\\.\\d+)?).?([NSEWnsew])?\\s*$"); // Decimal
	QRegExp re3("([+-]?[\\d.]+)°(?:([\\d.]+)')?(?:([\\d.]+)\")?"); // DMS like +121°33'38.28"

	if (re1.exactMatch(str))
	{
		bool neg = (re1.capturedTexts().at(1) == "-");
		float d = re1.capturedTexts().at(2).toFloat();
		float m = re1.capturedTexts().at(4).toFloat();
		double s = re1.capturedTexts().at(5).toDouble();
		if (re1.capturedTexts().at(3).toUpper() == "H")
		{
			d *= 15;
			m *= 15;
			s *= 15;
		}
		QString cardinal = re1.capturedTexts().at(7);
		double deg = d + (m/60) + (s/3600);
		if (cardinal.toLower() == "s" || cardinal.toLower() == "w" || neg)
			deg *= -1.;
		return (deg * 2 * M_PI / 360.);
	}
	else if (re2.exactMatch(str))
	{
		bool neg = (re2.capturedTexts().at(1) == "-");
		double deg = re2.capturedTexts().at(2).toDouble();
		QString cardinal = re2.capturedTexts().at(4);
		if (cardinal.toLower() == "s" || cardinal.toLower() == "w" || neg)
			deg *= -1.;
		return (deg * 2 * M_PI / 360.);
	}
	else if (re3.exactMatch(str))
	{
		float deg = re3.capturedTexts()[1].toFloat();
		float min = re3.capturedTexts()[2].isEmpty()? 0 : re3.capturedTexts()[2].toFloat();
		float sec = re3.capturedTexts()[3].isEmpty()? 0 : re3.capturedTexts()[3].toFloat();
		float r = qAbs(deg) + min / 60 + sec / 3600;
		if (deg<0)
			r *= -1.;
		return (r * 2 * M_PI / 360.);
	}

	qDebug() << "getDecAngle failed to parse angle string:" << str;
	return -0.0;
}

// Check if a number is a power of 2
bool isPowerOfTwo(const int value)
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
double asinh(const double z)
{
	double returned;
	if(z>0)
	   returned = std::log(z + std::sqrt(z*z+1));
	else
	   returned = -std::log(-z + std::sqrt(z*z+1));

	return returned;
}

/*************************************************************************
 Convert a QT QDateTime class to julian day
*************************************************************************/
double qDateTimeToJd(const QDateTime& dateTime)
{
	return (double)(dateTime.date().toJulianDay())+(double)1./(24*60*60*1000)*QTime(0, 0, 0, 0).msecsTo(dateTime.time())-0.5;
}

QDateTime jdToQDateTime(const double& jd)
{
	int year, month, day;
	getDateFromJulianDay(jd, &year, &month, &day);
	QDateTime result = QDateTime::fromString(QString("%1.%2.%3").arg(year, 4, 10, QLatin1Char('0')).arg(month).arg(day), "yyyy.M.d");
	result.setTime(jdFractionToQTime(jd));
	return result;
}


void getDateFromJulianDay(const double jd, int *yy, int *mm, int *dd)
{
	/*
	 * This algorithm is taken from
	 * "Numerical Recipes in c, 2nd Ed." (1992), pp. 14-15
	 * and converted to integer math.
	 * The electronic version of the book is freely available
	 * at http://www.nr.com/ , under "Obsolete Versions - Older
	 * book and code versions.
	 */

	static const long JD_GREG_CAL = 2299161;
	static const int JB_MAX_WITHOUT_OVERFLOW = 107374182;
	long julian;

	julian = (long)floor(jd + 0.5);

	long ta, jalpha, tb, tc, td, te;

	if (julian >= JD_GREG_CAL)
	{
		jalpha = (4*(julian - 1867216) - 1) / 146097;
		ta = julian + 1 + jalpha - jalpha / 4;
	}
	else if (julian < 0)
	{
		ta = julian + 36525 * (1 - julian / 36525);
	}
	else
	{
		ta = julian;
	}

	tb = ta + 1524;
	if (tb <= JB_MAX_WITHOUT_OVERFLOW)
	{
		tc = (tb*20 - 2442) / 7305;
	}
	else
	{
		tc = (long)(((unsigned long long)tb*20 - 2442) / 7305);
	}
	td = 365 * tc + tc/4;
	te = ((tb - td) * 10000)/306001;

	*dd = tb - td - (306001 * te) / 10000;

	*mm = te - 1;
	if (*mm > 12)
	{
		*mm -= 12;
	}
	*yy = tc - 4715;
	if (*mm > 2)
	{
		--(*yy);
	}
	if (julian < 0)
	{
		*yy -= 100 * (1 - julian / 36525);
	}
}

void getTimeFromJulianDay(const double julianDay, int *hour, int *minute, int *second)
{
	double frac = julianDay - (floor(julianDay));
	int s = (int)floor((frac * 24.0 * 60.0 * 60.0) + 0.0001);  // add constant to fix floating-point truncation error

	*hour = ((s / (60 * 60))+12)%24;
	*minute = (s/(60))%60;
	*second = s % 60;
}

QString julianDayToISO8601String(const double jd)
{
	int year, month, day, hour, minute, second;
	getDateFromJulianDay(jd, &year, &month, &day);
	getTimeFromJulianDay(jd, &hour, &minute, &second);

	QString res = QString("%1-%2-%3T%4:%5:%6")
				 .arg((year >= 0 ? year : -1* year),4,10,QLatin1Char('0'))
				 .arg(month,2,10,QLatin1Char('0'))
				 .arg(day,2,10,QLatin1Char('0'))
				 .arg(hour,2,10,QLatin1Char('0'))
				 .arg(minute,2,10,QLatin1Char('0'))
				 .arg(second,2,10,QLatin1Char('0'));
	if (year < 0)
	{
		res.prepend("-");
	}
	return res;
}

// Format the date per the fmt.
QString localeDateString(const int year, const int month, const int day, const int dayOfWeek, const QString &fmt)
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
QString localeDateString(const int year, const int month, const int day, const int dayOfWeek)
{

	// try the QDateTime first
	QDate test(year, month, day);

	// try to avoid QDate's non-astronomical time here, don't do BCE or year 0.
	if (year > 0 && test.isValid() && !test.toString(Qt::LocaleDate).isEmpty())
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
double getJDFromSystem()
{
	return qDateTimeToJd(QDateTime::currentDateTime().toUTC());
}

double qTimeToJDFraction(const QTime& time)
{
	return (double)1./(24*60*60*1000)*QTime(0, 0, 0, 0).msecsTo(time)-0.5;
}

QTime jdFractionToQTime(const double jd)
{
	double decHours = std::fmod(jd+0.5, 1.0);
	int hours = (int)(decHours/0.041666666666666666666);
	int mins = (int)((decHours-(hours*0.041666666666666666666))/0.00069444444444444444444);
	return QTime::fromString(QString("%1.%2").arg(hours).arg(mins), "h.m");
}

// Use Qt's own sense of time and offset instead of platform specific code.
float getGMTShiftFromQT(const double JD)
{
	int year, month, day, hour, minute, second;
	getDateFromJulianDay(JD, &year, &month, &day);
	getTimeFromJulianDay(JD, &hour, &minute, &second);
	// as analogous to second statement in getJDFromDate, nkerr
	if ( year <= 0 )
	{
		year = year - 1;
	}
	//getTime/DateFromJulianDay returns UTC time, not local time
	QDateTime universal(QDate(year, month, day), QTime(hour, minute, second), Qt::UTC);
	if (! universal.isValid())
	{
		//qWarning() << "JD " << QString("%1").arg(JD) << " out of bounds of QT help with GMT shift, using current datetime";
		// Assumes the GMT shift was always the same before year -4710
		universal = QDateTime(QDate(-4710, month, day), QTime(hour, minute, second), Qt::UTC);
	}
	QDateTime local = universal.toLocalTime();
	//Both timezones should be interpreted as UTC because secsTo() converts both
	//times to UTC if their zones have different daylight saving time rules.
	local.setTimeSpec(Qt::UTC);

	int shiftInSeconds = universal.secsTo(local);
	float shiftInHours = shiftInSeconds / 3600.0f;
	return shiftInHours;
}

// UTC !
bool getJDFromDate(double* newjd, const int y, const int m, const int d, const int h, const int min, const int s)
{
	static const long IGREG2 = 15+31L*(10+12L*1582);
	double deltaTime = (h / 24.0) + (min / (24.0*60.0)) + (s / (24.0 * 60.0 * 60.0)) - 0.5;
	QDate test((y <= 0 ? y-1 : y), m, d);
	// if QDate will oblige, do so.
	// added hook for Julian calendar, because he has been removed from Qt5 --AW
	if ( test.isValid() && y>1582)
	{
		double qdjd = (double)test.toJulianDay();
		qdjd += deltaTime;
		*newjd = qdjd;
		return true;
	}
	else
	{
		/*
		 * Algorithm taken from "Numerical Recipes in c, 2nd Ed." (1992), pp. 11-12
		 */
		long ljul;
		long jy, jm;
		long laa, lbb, lcc, lee;

		jy = y;
		if (m > 2)
		{
			jm = m + 1;
		}
		else
		{
			--jy;
			jm = m + 13;
		}

		laa = 1461 * jy / 4;
		if (jy < 0 && jy % 4)
		{
			--laa;
		}
		lbb = 306001 * jm / 10000;
		ljul = laa + lbb + d + 1720995L;

		if (d + 31L*(m + 12L * y) >= IGREG2)
		{
			lcc = jy/100;
			if (jy < 0 && jy % 100)
			{
				--lcc;
			}
			lee = lcc/4;
			if (lcc < 0 && lcc % 4)
			{
				--lee;
			}
			ljul += 2 - lcc + lee;
		}
		double jd = (double)ljul;
		jd += deltaTime;
		*newjd = jd;
		return true;
	}
	return false;
}

double getJDFromDate_alg2(const int y, const int m, const int d, const int h, const int min, const int s)
{
	double extra = (100.0* y) + m - 190002.5;
	double rjd = 367.0 * y;
	rjd -= floor(7.0*(y+floor((m+9.0)/12.0))/4.0);
	rjd += floor(275.0*m/9.0) ;
	rjd += d;
	rjd += (h + (min + s/60.0)/60.)/24.0;
	rjd += 1721013.5;
	rjd -= 0.5*extra/std::fabs(extra);
	rjd += 0.5;
	return rjd;
}

int numberOfDaysInMonthInYear(const int month, const int year)
{
	switch(month)
	{
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
			if ( year > 1582 )
			{
				if ( year % 4 == 0 )
				{
					if ( year % 100 == 0 )
					{
						if ( year % 400 == 0 )
						{
							return 29;
						}
						else
						{
							return 28;
						}
					}
					else
					{
						return 29;
					}
				}
				else
				{
					return 28;
				}
			}
			else
			{
				if ( year % 4 == 0 )
				{
					return 29;
				}
				else
				{
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

void debugQVariantMap(const QVariant& m, const QString& indent, const QString& key)
{
	QVariant::Type t = m.type();
	if (t == QVariant::Map)
	{
		qDebug() << indent + key + "(map):";
		QList<QString> keys = m.toMap().keys();
		qSort(keys);
		foreach(QString k, keys)
		{
			debugQVariantMap(m.toMap()[k], indent + "    ", k);
		}
	}
	else if (t == QVariant::List)
	{
		qDebug() << indent + key + "(list):";
		foreach(QVariant item, m.toList())
		{
			debugQVariantMap(item, indent + "    ");
		}
	}
	else
		qDebug() << indent + key + " => " + m.toString();
}

double getJulianDayFromISO8601String(const QString& iso8601Date, bool* ok)
{
	int y, m, d, h, min;
	float s;
	*ok = getDateTimeFromISO8601String(iso8601Date, &y, &m, &d, &h, &min, &s);	
	if (*ok)
	{
		double jd;
		if (!StelUtils::getJDFromDate(&jd, y, m, d, h, min, s))
		{
			*ok = false;
			return 0.0;
		}
		return jd;
	}
	return 0.0;
}

bool getDateTimeFromISO8601String(const QString& iso8601Date, int* y, int* m, int* d, int* h, int* min, float* s)
{
	// Represents an ISO8601 complete date string.
	QRegExp finalRe("^([+\\-]?\\d+)[:\\-](\\d\\d)[:\\-](\\d\\d)T(\\d?\\d):(\\d\\d):(\\d\\d(?:\\.\\d*)?)$");
	if (finalRe.exactMatch(iso8601Date) && finalRe.captureCount()==6)
	{
		bool error = false;
		bool ok;
		*y = finalRe.capturedTexts().at(1).toInt(&ok);
		error = error || !ok;
		*m = finalRe.capturedTexts().at(2).toInt(&ok);
		error = error || !ok;
		*d = finalRe.capturedTexts().at(3).toInt(&ok);
		error = error || !ok;
		*h = finalRe.capturedTexts().at(4).toInt(&ok);
		error = error || !ok;
		*min = finalRe.capturedTexts().at(5).toInt(&ok);
		error = error || !ok;
		*s = finalRe.capturedTexts().at(6).toFloat(&ok);
		error = error || !ok;
		if (!error)
			return true;
	}
	return false;
}

// Calculate and getting sidereal period in days from semi-major axis
double calculateSiderealPeriod(const double SemiMajorAxis)
{
	// Calculate semi-major axis in meters
	double a = AU*1000*SemiMajorAxis;
	// Calculate orbital period in seconds
	// Here 1.32712440018e20 is heliocentric gravitational constant
	double period = 2*M_PI*std::sqrt(a*a*a/1.32712440018e20);
	return period/86400; // return period in days
}


QString hoursToHmsStr(const double hours)
{
	int h = (int)hours;
	int m = (int)((qAbs(hours)-qAbs(double(h)))*60);
	float s = (((qAbs(hours)-qAbs(double(h)))*60)-m)*60;

	return QString("%1h%2m%3s").arg(h).arg(m).arg(QString::number(s, 'f', 1));
}

/* /////////////////// DELTA T VARIANTS
// For the standard epochs for many formulae, we use
// J2000.0=2000-jan-1.5=2451545.0,
//  1900.0=1900-jan-0.5=2415020.0
//  1820.0=1820-jan-0.5=2385800.0
//  1810.0=1810-jan-0.5=2382148.0
//  1800.0=1800-jan-0.5=2378496.0
//  1735.0=1735-jan-0.5=2354755.0
//  1625.0=1625-jan-0.5=2314579.0
*/

// Implementation of algorithm by Espenak & Meeus (2006) for DeltaT computation
double getDeltaTByEspenakMeeus(const double jDay)
{
	int year, month, day;	
	getDateFromJulianDay(jDay, &year, &month, &day);

	// Note: the method here is adapted from
	// "Five Millennium Canon of Solar Eclipses" [Espenak and Meeus, 2006]
	// A summary is described here:
	// http://eclipse.gsfc.nasa.gov/SEhelp/deltatpoly2004.html

	double y = year+((month-1)*30.5+day/31.*30.5)/366;

	// set the default value for Delta T
	double u = (y-1820)/100.;
	double r = (-20 + 32 * u * u);

	if (y < -500)
	{
		// values are equal to defaults!
		// u = (y-1820)/100.;
		// r = (-20 + 32 * u * u);
	}
	else if (y < 500)
	{
		u = y/100;
		//r = (10583.6 - 1014.41 * u + 33.78311 * std::pow(u,2) - 5.952053 * std::pow(u,3)
		//       - 0.1798452 * std::pow(u,4) + 0.022174192 * std::pow(u,5) + 0.0090316521 * std::pow(u,6));
		r = (((((0.0090316521*u + 0.022174192)*u - 0.1798452)*u - 5.952053)*u + 33.78311)*u -1014.41)*u +10583.6;
	}
	else if (y < 1600)
	{
		u = (y-1000)/100;
		//r = (1574.2 - 556.01 * u + 71.23472 * std::pow(u,2) + 0.319781 * std::pow(u,3)
		//       - 0.8503463 * std::pow(u,4) - 0.005050998 * std::pow(u,5) + 0.0083572073 * std::pow(u,6));
		r = (((((0.0083572073*u - 0.005050998)*u -0.8503463)*u +0.319781)*u + 71.23472)*u -556.01)*u + 1574.2;
	}
	else if (y < 1700)
	{
		double t = y - 1600;
		//r = (120 - 0.9808 * t - 0.01532 * std::pow(t,2) + std::pow(t,3) / 7129);
		r = ((t/7129.0 - 0.01532)*t - 0.9808)*t +120.0;
	}
	else if (y < 1800)
	{
		double t = y - 1700;
		//r = (8.83 + 0.1603 * t - 0.0059285 * std::pow(t,2) + 0.00013336 * std::pow(t,3) - std::pow(t,4) / 1174000);
		r = (((-t/1174000.0 + 0.00013336)*t - 0.0059285)*t + 0.1603)*t +8.83;
	}
	else if (y < 1860)
	{
		double t = y - 1800;
		//r = (13.72 - 0.332447 * t + 0.0068612 * std::pow(t,2) + 0.0041116 * std::pow(t,3) - 0.00037436 * std::pow(t,4)
		//       + 0.0000121272 * std::pow(t,5) - 0.0000001699 * std::pow(t,6) + 0.000000000875 * std::pow(t,7));
		r = ((((((.000000000875*t -.0000001699)*t + 0.0000121272)*t - 0.00037436)*t + 0.0041116)*t + 0.0068612)*t - 0.332447)*t +13.72;
	}
	else if (y < 1900)
	{
		double t = y - 1860;
		//r = (7.62 + 0.5737 * t - 0.251754 * std::pow(t,2) + 0.01680668 * std::pow(t,3)
		//	-0.0004473624 * std::pow(t,4) + std::pow(t,5) / 233174);
		r = ((((t/233174.0 -0.0004473624)*t + 0.01680668)*t - 0.251754)*t + 0.5737)*t + 7.62;
	}
	else if (y < 1920)
	{
		double t = y - 1900;
		//r = (-2.79 + 1.494119 * t - 0.0598939 * std::pow(t,2) + 0.0061966 * std::pow(t,3) - 0.000197 * std::pow(t,4));
		r = (((-0.000197*t + 0.0061966)*t - 0.0598939)*t + 1.494119)*t -2.79;
	}
	else if (y < 1941)
	{
		double t = y - 1920;
		//r = (21.20 + 0.84493*t - 0.076100 * std::pow(t,2) + 0.0020936 * std::pow(t,3));
		r = ((0.0020936*t - 0.076100)*t+ 0.84493)*t +21.20;
	}
	else if (y < 1961)
	{
		double t = y - 1950;
		//r = (29.07 + 0.407*t - std::pow(t,2)/233 + std::pow(t,3) / 2547);
		r = ((t/2547.0 -1.0/233.0)*t + 0.407)*t +29.07;
	}
	else if (y < 1986)
	{
		double t = y - 1975;
		//r = (45.45 + 1.067*t - std::pow(t,2)/260 - std::pow(t,3) / 718);
		r = ((-t/718.0 -1/260.0)*t + 1.067)*t +45.45;
	}
	else if (y < 2005)
	{
		double t = y - 2000;
		//r = (63.86 + 0.3345 * t - 0.060374 * std::pow(t,2) + 0.0017275 * std::pow(t,3) + 0.000651814 * std::pow(t,4) + 0.00002373599 * std::pow(t,5));
		r = ((((0.00002373599*t + 0.000651814)*t + 0.0017275)*t - 0.060374)*t + 0.3345)*t +63.86;
	}
	else if (y < 2050)
	{
		double t = y - 2000;
		//r = (62.92 + 0.32217 * t + 0.005589 * std::pow(t,2));
		r = (0.005589*t +0.32217)*t + 62.92;
	}
	else if (y < 2150)
	{
		//r = (-20 + 32 * std::pow((y-1820)/100,2) - 0.5628 * (2150 - y));
		// r has been precomputed before, just add the term patching the discontinuity
		r -= 0.5628*(2150.0-y);
	}

	return r;
}

// Implementation of algorithm by Schoch (1931) for DeltaT computation
double getDeltaTBySchoch(const double jDay)
{
	double u=(jDay-2378496.0)/36525.0; // (1800-jan-0.5)
	return -36.28 + 36.28*u*u;
}

// Implementation of algorithm by Clemence (1948) for DeltaT computation
double getDeltaTByClemence(const double jDay)
{
	double u=(jDay-2415020.0)/36525.0; // (1900-jan-0.5)
	return +8.72 + 26.75*u + 11.22*u*u;
}

// Implementation of algorithm by IAU (1952) for DeltaT computation
double getDeltaTByIAU(const double jDay)
{
	double u=(jDay-2415020.0)/36525.0; // (1900-jan-0.5)
	// TODO: Calculate Moon's longitude fluctuation
	return (29.950*u +72.318)*u +24.349 /* + 1.82144*b */ ;
}

// Implementation of algorithm by Astronomical Ephemeris (1960) for DeltaT computation, also used by Mucke&Meeus, Canon of Solar Eclipses, Vienna 1983
double getDeltaTByAstronomicalEphemeris(const double jDay)
{
	double u=(jDay-2415020.0)/36525.0; // (1900-jan-0.5)
	// TODO: Calculate Moon's longitude fluctuation
	// Note: also Mucke&Meeus 1983 ignore b
	return (29.949*u +72.3165)*u +24.349 /* + 1.821*b*/ ;
}

// Implementation of algorithm by Tuckerman (1962, 1964) & Goldstine (1973) for DeltaT computation
double getDeltaTByTuckermanGoldstine(const double jDay)
{
	double u=(jDay-2415020.0)/36525.0; // (1900-jan-0.5)
	return (36.79*u +35.06)*u + 4.87;
}

// Implementation of algorithm by Muller & Stephenson (1975) for DeltaT computation
double getDeltaTByMullerStephenson(const double jDay)
{
	double u=(jDay-2415020.0)/36525.0; // (1900-jan-0.5)
	return (45.78*u +120.38)*u +66.0;
}

// Implementation of algorithm by Stephenson (1978) for DeltaT computation
double getDeltaTByStephenson1978(const double jDay)
{
	double u=(jDay-2415020.0)/36525.0; // (1900-jan-0.5)
	return (38.30*u +114.0)*u +20.0;
}

// Implementation of algorithm by Stephenson (1997) for DeltaT computation
double getDeltaTByStephenson1997(const double jDay)
{
	double u=(jDay-2354755.0)/36525.0; // (1735-jan-0.5)
	return -20.0 + 35.0*u*u;
}

// Implementation of algorithm by Schmadel & Zech (1979) for DeltaT computation
double getDeltaTBySchmadelZech1979(const double jDay)
{
	double u=(jDay-2415020.0)/36525.0; // (1900-jan-0.5)
	double deltaT=(((((((((((-0.089491*u -0.117389)*u + 0.185489)*u + 0.247433)*u - 0.159732)*u - 0.200097)*u + 0.075456)*u
			+ 0.076929)*u - 0.020446)*u - 0.013867)*u + 0.003081)*u + 0.001233)*u -0.000029;
	return deltaT * 86400.0;
}

// Implementation of algorithm by Morrison & Stephenson (1982) for DeltaT computation
double getDeltaTByMorrisonStephenson1982(const double jDay)
{
	double u=(jDay-2382148.0)/36525.0; // (1810-jan-0.5)
	return -15.0+32.50*u*u;
}

// Implementation of algorithm by Stephenson & Morrison (1984) for DeltaT computation
double getDeltaTByStephensonMorrison1984(const double jDay)
{
	int year, month, day;	
	double deltaT = 0.;
	getDateFromJulianDay(jDay, &year, &month, &day);

	double u = (getDecYear(year, month, day)-1800)/100;

	if (-391 < year && year <= 948)
		deltaT = (44.3*u +320.0)*u +1360.0;
	if (948 < year && year <= 1600)
		deltaT = 25.5*u*u;

	return deltaT;
}

// Implementation of algorithm by Stephenson & Morrison (1995) for DeltaT computation
double getDeltaTByStephensonMorrison1995(const double jDay)
{
	double u=(jDay-2385800.0)/36525.0; // (1820-jan-0.5)
	return -20.0 + 31.0*u*u;
}

// Implementation of algorithm by Stephenson & Houlden (1986) for DeltaT computation
double getDeltaTByStephensonHoulden(const double jDay)
{
	int year, month, day;
	double u;
	double deltaT = 0.;
	getDateFromJulianDay(jDay, &year, &month, &day);

	double yeardec=getDecYear(year, month, day);

	if (year <= 948)
	{
		u = (yeardec-948)/100;
		deltaT = (46.5*u -405.0)*u + 1830.0;
	}
	if (948 < year && year <= 1600)
	{
		u = (yeardec-1850)/100;
		deltaT = 22.5*u*u;
	}

	return deltaT;
}

// Implementation of algorithm by Espenak (1987, 1989) for DeltaT computation
double getDeltaTByEspenak(const double jDay)
{
	double u=(jDay-2451545.0)/36525.0; // (2000-jan-1.5)
	return (64.3*u +61.0)*u +67.0;
}

// Implementation of algorithm by Borkowski (1988) for DeltaT computation
// This is explicitly compatible with ELP2000-85.
double getDeltaTByBorkowski(const double jDay)
{
	double u=(jDay-2451545.0)/36525.0 + 3.75; // (2000-jan-1.5), deviation from 1625 as given in the paper.
	return 40.0 + 35.0*u*u;
}

// Implementation of algorithm by Schmadel & Zech (1988) for DeltaT computation
double getDeltaTBySchmadelZech1988(const double jDay)
{
	double u=(jDay-2415020.0)/36525.0; // (1900-jan-0.5)
	double deltaT = (((((((((((-0.058091*u -0.067471)*u +.145932)*u +.161416)*u -.149279)*u -.146960)*u +.079441)*u +.062971)*u -.022542)*u -.012462)*u +.003357)*u +.001148)*u-.000014;
	return deltaT * 86400.0;
}

// Implementation of algorithm by Chapront-Touzé & Chapront (1991) for DeltaT computation
double getDeltaTByChaprontTouze(const double jDay)
{
	int year, month, day;	
	double deltaT = 0.;
	getDateFromJulianDay(jDay, &year, &month, &day);

	double u=(jDay-2451545.0)/36525.0; // (2000-jan-1.5)

	if (-391 < year && year <= 948)
		deltaT = (42.4*u +495.0)*u + 2177.0;
	if (948 < year && year <= 1600)
		deltaT = (23.6*u +100.0)*u + 102.0;

	return deltaT;
}

// Implementation of algorithm by JPL Horizons for DeltaT computation
double getDeltaTByJPLHorizons(const double jDay)
{ // FIXME: It does not make sense to have zeros after 1620 in a JPL Horizons compatible implementation!
	int year, month, day;
	double u;
	double deltaT = 0.;
	getDateFromJulianDay(jDay, &year, &month, &day);

	if (-2999 < year && year < 948)
	{
		u=(jDay-2385800.0)/36525.0; // (1820-jan-1.5)
		deltaT = 31.0*u*u;
	}
	if (948 < year && year <= 1620)
	{
		u=(jDay-2451545.0)/36525.0; // (2000-jan-1.5)
		deltaT = (22.5*u +67.5)*u + 50.6;
	}

	return deltaT;
}

// Implementation of algorithm by Morrison & Stephenson (2004, 2005) for DeltaT computation
double getDeltaTByMorrisonStephenson2004(const double jDay)
{
	double u=(jDay-2385800.0)/36525.0; // (1820-jan-0.5)
	return -20.0 + 32.0 * u*u;
}

// Implementation of algorithm by Reijs (2006) for DeltaT computation
double getDeltaTByReijs(const double jDay)
{
	double OffSetYear = (2385800.0 - jDay)/365.25;

	return ((1.8 * OffSetYear*OffSetYear/200 + 1443*3.76/(2*M_PI)*(std::cos(2*M_PI*OffSetYear/1443)-1))*365.25)/1000;
}

// Implementation of algorithm by Chapront, Chapront-Touze & Francou (1997) & Meeus (1998) for DeltaT computation
// 191 values in tenths of second for interpolation table 1620..2000, every 2nd year.
static const int MeeusDeltaTTable[] = { 1210, 1120, 1030, 950, 880, 820, 770, 720, 680, 630, 600, 560, 530, 510, 480,
					460, 440, 420, 400, 380, 350, 330, 310, 290, 260, 240, 220, 200, 180, 160, 140, 120, 110, 100,  90,  80,  70,  70,  70,  70, // before 1700
					70,  70,  80,  80,  90,  90,  90,  90,  90, 100, 100, 100, 100, 100, 100, 100, 100, 110, 110, 110, 110, 110, 120, 120, 120, // before 1750
					120, 130, 130, 130, 140, 140, 140, 140, 150, 150, 150, 150, 150, 160, 160, 160, 160, 160, 160, 160, 160, 150, 150, 140, 130, // before 1800
					131, 125, 122, 120, 120, 120, 120, 120, 120, 119, 116, 110, 102,  92,  82,  71,  62,  56,  54,  53,  54,  56,  59,  62,  65, // before 1850
					68,  71,  73,  75,  76,  77,  73,  62,  52,  27,  14, -12, -28, -38, -48, -55, -53, -56, -57, -59, -60, -63, -65, -62, -47, // before 1900
					-28,  -1,  26,  53,  77, 104, 133, 160, 182, 202, 211, 224, 235, 238, 243, 240, 239, 239, 237, 240, 243, 253, 262, 273, 282, // before 1950
					291, 300, 307, 314, 322, 331, 340, 350, 365, 383, 402, 422, 445, 465, 485, 505, 522, 538, 549, 558, 569, 583, 600, 616, 630, 650}; //closing: 2000
double getDeltaTByChaprontMeeus(const double jDay)
{
	int year, month, day;
	double deltaT = 0.;
	getDateFromJulianDay(jDay, &year, &month, &day);

	//const double u19=(jDay-2415020.0)/36525.0; // (1900-jan-0.5) Only for approx form.
	const double u20=(jDay-2451545.0)/36525.0; // (2000-jan-1.5)

	if (year<948)
		deltaT=(44.1*u20 +497.0)*u20+2177.0;
	else if (year<1620)
		deltaT=(25.3*u20 + 102.0)*u20+102.0;
	// The next terms are the short approximative formulae. But Meeus gives exact values, table, etc. for 1620..2000.
	//else if (year<1900)
	//        deltaT= ((((((((( 123563.95*u19 + 727058.63)*u19 + 1818961.41)*u19 + 2513807.78)*u19 + 2087298.89)*u19 + 1061660.75)*u19 +
	//                     324011.78)*u19 + 56282.84)*u19 + 5218.61)*u19 + 228.95)*u19 - 2.50;
	//else if (year<1997)
	//        deltaT= (((((((( 58353.42*u19 -232424.66)*u19 +372919.88)*u19 - 303191.19)*u19 + 124906.15)*u19 - 18756.33)*u19 - 2637.80)*u19 + 815.20)*u19 + 87.24)*u19 - 2.44;
	else if (year <2000)
	{
		double yeardec=getDecYear(year, month, day);
		int pos=(year-1620)/2; // this is a deliberate integer division! 2->1, 3->1, 4->2, 5->2 etc.
		deltaT= MeeusDeltaTTable[pos]+ (yeardec-(2*pos+1620))*0.5  *(MeeusDeltaTTable[pos+1]-MeeusDeltaTTable[pos]);
		deltaT /= 10.0;
	}
	else if (year<2100)
		deltaT=(25.3*u20 + 102.0)*u20+102.0 + 0.37*(year-2100);
	else // year > 2100
		deltaT=(25.3*u20 + 102.0)*u20+102.0;
	return deltaT;
}

// Implementation of algorithm by Montenbruck & Pfleger (2000) for DeltaT computation
// Their implementation explicitly returns 0 for out-of-range data, so must ours!
// Note: the polynomes do not form a well-behaved continuous line.
// The source of their data is likely the data table from Meeus, Astr. Alg. 1991.
double getDeltaTByMontenbruckPfleger(const double jDay)
{
	double deltaT = 0.;

	const double T=(jDay-2451545)/36525;
	double t;
	if (jDay<2387627.5) // 1825-01-01 0:00 ...
		deltaT=0.0;
	else if (jDay < 2396758.5) { // 1850-01-01 0:00
		t=T+1.75;
		deltaT=(( -572.3*t+413.9)*t  -80.8)*t +10.4;
	} else if (jDay < 2405889.5) { // 1875-01-01 0:00
		t=T+1.50;
		deltaT=((   18.8*t-358.4)*t  +46.3)*t + 6.6;
	} else if (jDay < 2415020.5) { // 1900-01-01 0:00
		t=T+1.25;
		deltaT=((  867.4*t-166.2)*t  -10.8)*t - 3.9;
	} else if (jDay < 2424151.5) { // 1925-01-01 0:00
		t=T+1.00;
		deltaT=((-1467.4*t+327.5)*t +114.1)*t - 2.6;
	} else if (jDay < 2433282.5) { // 1950-01-01 0:00
		t=T+0.75;
		deltaT=((  483.4*t - 8.2)*t  - 6.3)*t +24.2;
	} else if (jDay < 2442413.5) { // 1975-01-01 0:00
		t=T+0.50;
		deltaT=((  550.7*t - 3.8)*t  +32.5)*t +29.3;
	} else if (jDay < 2453736.5) { // 2006-01-01 0:00
		t=T+0.25;
		deltaT=(( 1516.7*t-570.5)*t +130.5)*t +45.3;
	}

	return deltaT;
}

// Implementation of algorithm by Meeus & Simons (2000) for DeltaT computation
// Zero outside defined range 1620..2000!
double getDeltaTByMeeusSimons(const double jDay)
{
	int year, month, day;
	double u;	
	double deltaT = 0.;
	getDateFromJulianDay(jDay, &year, &month, &day);

	//double yeardec=year+((month-1)*30.5+day/31*30.5)/366;
	//double ub = (yeardec-2000)/100;
	const double ub=(jDay-2451545.0)/36525.0; // (2000-jan-1.5)

	if (year <1620)
		deltaT=0.0;
	else if (year < 1690)
	{
		u = 3.45 + ub;
		//deltaT = +40.3 - 107.0*u + 50.0*std::pow(u,2) - 454.0*std::pow(u,3) + 1244.0*std::pow(u,4);
		deltaT = (((1244.0*u -454.0)*u + 50.0)*u -107.0)*u +40.3;
	}
	else if (year < 1770)
	{
		u = 2.70 + ub;
		//deltaT = +10.2 + 11.3*u - std::pow(u,2) - 16.0*std::pow(u,3) + 70.0*std::pow(u,4);
		deltaT = (((70.0*u -16.0)*u -1.0)*u +11.3)*u +10.2;
	}
	else if (year < 1820)
	{
		u = 2.05 + ub;
		//deltaT = +14.7 - 18.8*u - 22.0*std::pow(u,2) + 173.0*std::pow(u,3) + 6.0*std::pow(u,4);
		deltaT = (((6.0*u +173.0)*u -22.0)*u -18.8)*u +14.7;
	}
	else if (year < 1870)
	{
		u = 1.55 + ub;
		//deltaT = +5.7 + 12.7*u + 111.0*std::pow(u,2) - 534.0*std::pow(u,3) - 1654.0*std::pow(u,4);
		deltaT = (((-1654.0*u -534.0)*u +111)*u +12.7)*u +5.7;
	}
	else if (year < 1900)
	{
		u = 1.15 + ub;
		//deltaT = -5.8 - 14.6*u + 27.0*std::pow(u,2) + 101.0*std::pow(u,3) + 8234.0*std::pow(u,4);
		deltaT = (((8234.0*u +101.0)*u +27.0)*u - 14.6)*u -5.8;
	}
	else if (year < 1940)
	{
		u = 0.80 + ub;
		//deltaT = +21.4 + 67.0*u - 443.0*std::pow(u,2) + 19.0*std::pow(u,3) + 4441.0*std::pow(u,4);
		deltaT = (((4441.0*u + 19.0)*u -443.0)*u +67.0)*u +21.4;
	}
	else if (year < 1990)
	{
		u = 0.35 + ub;
		//deltaT = +36.2 + 74.0*u + 189.0*std::pow(u,2) - 140.0*std::pow(u,3) - 1883.0*std::pow(u,4);
		deltaT = (((-1883.0*u -140.0)*u +189.0)*u +74.0)*u +36.2;
	}
	else if (year <= 2000)
	{
		u = 0.05 + ub;
		//deltaT = +60.8 + 82.0*u - 188.0*std::pow(u,2) - 5034.0*std::pow(u,3);
		deltaT = ((-5034.0*u -188.0)*u +82.0)*u +60.8;
	}

	return deltaT;
}

// Implementation of algorithm by Reingold & Dershowitz (Cal. Calc. 1997, 2001, 2007, Cal. Tab. 2002) for DeltaT computation.
// Created as yet another multi-segment polynomial fit through the table in Meeus: Astronomical Algorithms (1991).
// Note that only the Third edition (2007) adds the 1700-1799 term.
// More efficient reimplementation with stricter adherence to the source.
double getDeltaTByReingoldDershowitz(const double jDay)
{
	int year, month, day;	
	getDateFromJulianDay(jDay, &year, &month, &day);
	// R&D don't use a float-fraction year, but explicitly only the integer year! And R&D use a proleptic Gregorian year before 1582.
	// We cannot do that, but the difference is negligible.
	// FIXME: why are displayed values so far off the computed values? It seems currently broken!
	double deltaT=0.0; // If it returns 0, there is a bug!

	if ((year >= 2019) || (year < 1620))
	{
		double jdYear_0; getJDFromDate(&jdYear_0, year, 1, 1, 0, 0, 0);
		double jd1810_0; getJDFromDate(&jd1810_0, 1810, 1, 1, 0, 0, 0);
		double x = (jdYear_0-jd1810_0+0.5);
		deltaT = x*x/41048480.0 - 15.0;
	}
	else if (year >= 1988)
	{
		deltaT = year-1933.0;
	}
	else if (year >= 1800)
	{
		double jd1900_0; getJDFromDate(&jd1900_0, 1900, 1, 1, 0, 0, 0);
		double jdYear_5; getJDFromDate(&jdYear_5, year, 7, 1, 0, 0, 0);
		double c = (jdYear_5-jd1900_0) * (1/36525.0);
		if (year >= 1900)
		{
			deltaT = (((((((-0.212591*c +0.677066)*c -0.861938)*c +0.553040)*c -0.181133)*c +0.025184)*c +0.000297)*c -0.00002) * 86400.0;
		}
		else //if (year >= 1800)
		{
			deltaT = ((((((((((2.043794*c +11.636204)*c +28.316289)*c +38.291999)*c +31.332267)*c +15.845535)*c +4.867575)*c +0.865736)*c +0.083563)*c +0.003844)*c -0.000009) * 86400.0;
		}
	}
	else if (year >= 1700)
	{ // This term was added in the third edition (2007), its omission was a fault of the authors!
		double yDiff1700 = year-1700.0;
		deltaT = ((-0.0000266484*yDiff1700 +0.003336121)*yDiff1700 - 0.005092142)*yDiff1700 + 8.118780842;
	}
	else if (year >= 1620)
	{
		double yDiff1600 = year-1600.0;
		deltaT = (0.0219167*yDiff1600 -4.0675)*yDiff1600 +196.58333;
	}
	return deltaT;
}

// Implementation of algorithm by Banjevic (2006) for DeltaT computation.
double getDeltaTByBanjevic(const double jDay)
{
	int year, month, day;
	getDateFromJulianDay(jDay, &year, &month, &day);

	double u, c;

	if (year>=-2020 && year<=-700)
	{
		u = (jDay-2378496.0)/36525.0; // 1800.0=1800-jan-0.5=2378496.0
		c = 30.86;
	}
	else if (year>-700 && year<=1620)
	{
		u = (jDay-2385800.0)/36525.0; // 1820.0=1820-jan-0.5=2385800.0
		c = 31;
	}
	else
	{
		u = 0.;
		c = 0.;
	}

	return c*u*u;
}

// Implementation of algorithm by Islam, Sadiq & Qureshi (2008 + revisited 2013) for DeltaT computation.
double getDeltaTByIslamSadiqQureshi(const double jDay)
{
	int year, month, day;
	getDateFromJulianDay(jDay, &year, &month, &day);
	double deltaT = 0.0; // Return deltaT = 0 outside valid range.
	double u;
	const double ub=(jDay-2454101.0)/36525.0; // (2007-jan-0.5)
	if (year >= 1620 && year <= 1698)
	{
		u = 3.48 + ub;
		deltaT = (((1162.805 * u - 273.116) * u + 14.523) * u - 105.262) * u + 38.067;
	}
	else if (year <= 1806)
	{
		u = 2.545 + ub;
		deltaT = (((-71.724 * u - 39.048) * u + 7.591) * u + 13.893) * u + 13.759;
	}
	else if (year <= 1872)
	{
		u = 1.675 + ub;
		deltaT = (((-1612.55 * u - 157.977) * u + 161.524) * u - 3.654) * u + 5.859;
	}
	else if (year <= 1906)
	{
		u = 1.175 + ub;
		deltaT = (((6250.501 * u + 1006.463) * u + 139.921) * u - 2.732) * u - 6.203;
	}
	else if (year <= 1953)
	{
		// revised 2013 per email
		u = 0.77 + ub;
		deltaT = (((-390.785 * u + 901.514) * u - 88.044) * u + 8.997) * u + 24.006;
	}
	else if (year <= 2007)
	{
		// revised 2013 per email
		u = 0.265 + ub;
		deltaT = (((1314.759 * u - 296.018) * u - 101.898) * u + 88.659) * u + 49.997;
	}

	return deltaT;
}

// Implementation of polinomial approximation of time period 1620-2013 for DeltaT by M. Khalid, Mariam Sultana and Faheem Zaidi (2014).
double getDeltaTByKhalidSultanaZaidi(const double jDay)
{
	int year, month, day;
	getDateFromJulianDay(jDay, &year, &month, &day);
	double k, a0, a1, a2, a3, a4;
	if (year>=1620 && year<=1672)
	{
		k = 3.670; a0 = 76.541; a1 = -253.532; a2 = 695.901; a3 = -1256.982; a4 = 627.152;
	}
	else if (year>=1673 && year<=1729)
	{
		k = 3.120; a0 = 10.872; a1 = -40.744; a2 = 236.890; a3 = -351.537; a4 = 36.612;
	}
	else if (year>=1730 && year<=1797)
	{
		k = 2.495; a0 = 13.480; a1 = 13.075; a2 = 8.635; a3 = -3.307; a4 = -128.294;
	}
	else if (year>=1798 && year<=1843)
	{
		k = 1.925; a0 = 12.584; a1 = 1.929; a2 = 60.896; a3 = -1432.216; a4 = 3129.071;
	}
	else if (year>=1844 && year<=1877)
	{
		k = 1.525; a0 = 6.364; a1 = 11.004; a2 = 407.776; a3 = -4168.394; a4 = 7561.686;
	}
	else if (year>=1878 && year<=1904)
	{
		k = 1.220; a0 = -5.058; a1 = -1.701; a2 = -46.403; a3 = -866.171; a4 = 5917.585;
	}
	else if (year>=1905 && year<=1945)
	{
		k = 0.880; a0 = 13.392; a1 = 128.592; a2 = -279.165; a3 = -1282.050; a4 = 4039.490;
	}
	else if (year>=1946 && year<=1989)
	{
		k = 0.455; a0 = 30.782; a1 = 34.348; a2 = 46.452; a3 = 1295.550; a4 = -3210.913;
	}
	else if (year>=1990 && year<=2013)
	{
		k = 0.115; a0 = 55.281; a1 = 91.248; a2 = 87.202; a3 = -3092.565; a4 = 8255.422;
	}
	else
	{
		k = 0.0; a0 = 0.0; a1 = 0.0; a2 = 0.0; a3 = 0.0; a4 = 0.0;
	}

	double u = k + (year - 2000)/100;

	return (((a4*u + a3)*u + a2)*u + a1)*u + a0;
}

double getMoonSecularAcceleration(const double jDay, const double nd)
{
	int year, month, day;
	getDateFromJulianDay(jDay, &year, &month, &day);

	double t = (getDecYear(year, month, day)-1955.5)/100.0;
	// n.dot for secular acceleration of the Moon in ELP2000-82B
	// have value -23.8946 "/cy/cy
	return -0.91072 * (-23.8946 + qAbs(nd))*t*t;
}

double getDeltaTStandardError(const double jDay)
{
	int year, month, day;
	getDateFromJulianDay(jDay, &year, &month, &day);

	double sigma = -1.;

	if (-1000 <= year && year <= 1600)
	{
		double cDiff1820= (jDay-2385800.0)/36525.0; //    1820.0=1820-jan-0.5=2385800.0
		// sigma = std::pow((yeardec-1820.0)/100,2); // sigma(DeltaT) = 0.8*u^2
		sigma = 0.8 * cDiff1820 * cDiff1820;
	}
	return sigma;
}


// Arrays to keep cos/sin of angles and multiples of angles. rho and theta are delta angles, and these arrays
#define MAX_STACKS 4096
static float cos_sin_rho[2*(MAX_STACKS+1)];
#define MAX_SLICES 4096
static float cos_sin_theta[2*(MAX_SLICES+1)];

//! Compute cosines and sines around a circle which is split in "segments" parts.
//! Values are stored in the global static array cos_sin_theta.
//! Used for the sin/cos values along a latitude circle, equator, etc. for a spherical mesh.
//! @param slices number of partitions (elsewhere called "segments") for the circle
float* ComputeCosSinTheta(const int slices)
{
	Q_ASSERT(slices<=MAX_SLICES);
	
	// Difference angle between the stops. Always use 2*M_PI/slices!
	const float dTheta = 2.f * M_PI / slices;
	float *cos_sin = cos_sin_theta;
	float *cos_sin_rev = cos_sin + 2*(slices+1);
	const float c = std::cos(dTheta);
	const float s = std::sin(dTheta);
	*cos_sin++ = 1.f;
	*cos_sin++ = 0.f;
	*--cos_sin_rev = -cos_sin[-1];
	*--cos_sin_rev =  cos_sin[-2];
	*cos_sin++ = c;
	*cos_sin++ = s;
	*--cos_sin_rev = -cos_sin[-1];
	*--cos_sin_rev =  cos_sin[-2];
	while (cos_sin < cos_sin_rev)   // compares array address indices only!
	{
		// avoid expensive trig functions by use of the addition theorem.
		cos_sin[0] = cos_sin[-2]*c - cos_sin[-1]*s;
		cos_sin[1] = cos_sin[-2]*s + cos_sin[-1]*c;
		cos_sin += 2;
		*--cos_sin_rev = -cos_sin[-1];
		*--cos_sin_rev =  cos_sin[-2];
	}
	return cos_sin_theta;
}

//! Compute cosines and sines around a half-circle which is split in "segments" parts.
//! Values are stored in the global static array cos_sin_rho.
//! Used for the sin/cos values along a meridian for a spherical mesh.
//! @param segments number of partitions (elsewhere called "stacks") for the half-circle
float* ComputeCosSinRho(const int segments)
{
	Q_ASSERT(segments<=MAX_STACKS);
	
	// Difference angle between the stops. Always use M_PI/segments!
	const float dRho = M_PI / segments;
	float *cos_sin = cos_sin_rho;
	float *cos_sin_rev = cos_sin + 2*(segments+1);
	const float c = std::cos(dRho);
	const float s = std::sin(dRho);
	*cos_sin++ = 1.f;
	*cos_sin++ = 0.f;
	*--cos_sin_rev =  cos_sin[-1];
	*--cos_sin_rev = -cos_sin[-2];
	*cos_sin++ = c;
	*cos_sin++ = s;
	*--cos_sin_rev =  cos_sin[-1];
	*--cos_sin_rev = -cos_sin[-2];
	while (cos_sin < cos_sin_rev)    // compares array address indices only!
	{
		// avoid expensive trig functions by use of the addition theorem.
		cos_sin[0] = cos_sin[-2]*c - cos_sin[-1]*s;
		cos_sin[1] = cos_sin[-2]*s + cos_sin[-1]*c;
		cos_sin += 2;
		*--cos_sin_rev =  cos_sin[-1];
		*--cos_sin_rev = -cos_sin[-2];
	}
	
	return cos_sin_rho;
}

//! Compute cosines and sines around part of a circle (from top to bottom) which is split in "segments" parts.
//! Values are stored in the global static array cos_sin_rho.
//! Used for the sin/cos values along a meridian.
//! This allows leaving away pole caps. The array now contains values for the region minAngle+segments*phi
//! @param dRho a difference angle between the stops
//! @param segments number of segments
//! @param minAngle start angle inside the half-circle. maxAngle=minAngle+segments*phi
float *ComputeCosSinRhoZone(const float dRho, const int segments, const float minAngle)
{
	float *cos_sin = cos_sin_rho;
	const float c = cos(dRho);
	const float s = sin(dRho);
	*cos_sin++ = cos(minAngle);
	*cos_sin++ = sin(minAngle);
	for (int i=0; i<segments; ++i) // we cannot mirror this, it may be unequal.
	{   // efficient computation, avoid expensive trig functions by use of the addition theorem.
		cos_sin[0] = cos_sin[-2]*c - cos_sin[-1]*s;
		cos_sin[1] = cos_sin[-2]*s + cos_sin[-1]*c;
		cos_sin += 2;
	}
	return cos_sin_rho;
}

double getDecYear(const int year, const int month, const int day)
{
	return year+((month-1)*30.5+day/31.*30.5)/366;
}

//! Uncompress gzip or zlib compressed data.
QByteArray uncompress(const QByteArray& data)
{
	if (data.size() <= 4)
		return QByteArray();

	//needed for const-correctness, no deep copy performed
	QByteArray dataNonConst(data);
	QBuffer buf(&dataNonConst);
	buf.open(QIODevice::ReadOnly);

	return uncompress(buf);
}

//! Uncompress (gzip/zlib) data from this QIODevice, which must be open and readable.
//! @param device The device to read from, must already be opened with an OpenMode supporting reading
//! @param maxBytes The max. amount of bytes to read from the device, or -1 to read until EOF.  Note that it
//! always stops when inflate() returns Z_STREAM_END. Positive values can be used for interleaving compressed data
//! with other data.
QByteArray uncompress(QIODevice& device, qint64 maxBytes)
{
	// this is a basic zlib decompression routine, similar to:
	// http://zlib.net/zlib_how.html

	// buffer size 256k, zlib recommended size
	static const int CHUNK = 262144;
	QByteArray readBuffer(CHUNK, 0);
	QByteArray inflateBuffer(CHUNK, 0);
	QByteArray out;

	// zlib stream
	z_stream strm;
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	strm.avail_in = Z_NULL;
	strm.next_in = Z_NULL;

	// the amount of bytes already read from the device
	qint64 bytesRead = 0;

	// initialize zlib
	// 15 + 32 for gzip automatic header detection.
	int ret = inflateInit2(&strm, 15 +  32);
	if (ret != Z_OK)
	{
		qWarning()<<"zlib init error ("<<ret<<"), can't uncompress";
		if(strm.msg)
			qWarning()<<"zlib message: "<<QString(strm.msg);
		return QByteArray();
	}

	//zlib double loop - one for reading from file, one for inflating
	do
	{
		qint64 bytesToRead = CHUNK;
		if(maxBytes>=0)
		{
			//check if we reach the desired limit with the next read
			bytesToRead = qMin((qint64)CHUNK,maxBytes-bytesRead);
		}

		if(bytesToRead==0)
			break;

		//perform read from device
		qint64 read = device.read(readBuffer.data(), bytesToRead);
		if (read<0)
		{
			qWarning()<<"Error while reading from device";
			inflateEnd(&strm);
			return QByteArray();
		}

		bytesRead += read;
		strm.next_in = reinterpret_cast<Bytef*>(readBuffer.data());
		strm.avail_in = read;

		if(read==0)
			break;

		//inflate loop
		do
		{
			strm.avail_out = CHUNK;
			strm.next_out = reinterpret_cast<Bytef*>(inflateBuffer.data());
			ret = inflate(&strm,Z_NO_FLUSH);
			Q_ASSERT(ret != Z_STREAM_ERROR); // must never happen, indicates a programming error

			if(ret < 0 || ret == Z_NEED_DICT)
			{
				qWarning()<<"zlib inflate error ("<<ret<<"), can't uncompress";
				if(strm.msg)
					qWarning()<<"zlib message: "<<QString(strm.msg);
				inflateEnd(&strm);
				return QByteArray();
			}

			out.append(inflateBuffer.constData(), CHUNK - strm.avail_out);

		}while(strm.avail_out == 0); //if zlib has more data for us, repeat

	}while(ret!=Z_STREAM_END);

	// close zlib
	inflateEnd(&strm);

	if(ret!=Z_STREAM_END)
	{
		qWarning()<<"Premature end of compressed stream";
		if(strm.msg)
			qWarning()<<"zlib message: "<<QString(strm.msg);
		return QByteArray();
	}

	return out;
}

} // end of the StelUtils namespace


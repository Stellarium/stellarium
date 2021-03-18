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
#include "StelLocaleMgr.hpp"
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
//! Return the full name of stellarium, i.e. "stellarium 0.19.0"
QString getApplicationName()
{
	return QString("Stellarium")+" "+StelUtils::getApplicationVersion();
}

//! Return the version of stellarium, i.e. "0.19.0"
QString getApplicationVersion()
{
#if defined(STELLARIUM_VERSION)
	return QString(STELLARIUM_VERSION);
#elif defined(GIT_REVISION)
	return QString("%1-%2 [%3]").arg(PACKAGE_VERSION).arg(GIT_REVISION).arg(GIT_BRANCH);
#else
	return QString(PACKAGE_VERSION);
#endif
}

QString getUserAgentString()
{
	// Get info about operating system
	QString os = StelUtils::getOperatingSystemInfo();
	if (os.contains("FreeBSD"))
		os = "FreeBSD";
	else if (os.contains("NetBSD"))
		os = "NetBSD";
	else if (os.contains("OpenBSD"))
		os = "OpenBSD";

	// Set user agent as "Stellarium/$version$ ($operating system$; $CPU architecture$)"
	return QString("Stellarium/%1 (%2; %3)").arg(StelUtils::getApplicationVersion(), os, QSysInfo::currentCpuArchitecture());
}

QString getOperatingSystemInfo()
{
	QString OS = "Unknown operating system";

	#if defined(Q_OS_BSD4) || defined(Q_OS_SOLARIS)
	// Check FreeBSD, NetBSD, OpenBSD and DragonFly BSD
	QProcess uname;
	uname.start("/usr/bin/uname -srm");
	uname.waitForStarted();
	uname.waitForFinished();
	const QString BSDsystem = uname.readAllStandardOutput();
	OS = BSDsystem.trimmed();
	#else
	OS = QSysInfo::prettyProductName();
	#endif

	return OS;
}

double hmsStrToHours(const QString& s)
{
	QRegExp reg("(\\d+)h(\\d+)m(\\d+)s");
	if (!reg.exactMatch(s))
		return 0.;
	uint hrs = reg.cap(1).toUInt();
	uint min = reg.cap(2).toUInt();
	int sec = reg.cap(3).toInt();

	return hmsToHours(hrs, min, sec);
}

/*************************************************************************
 Convert a real duration in days to DHMS formatted string
*************************************************************************/
QString daysFloatToDHMS(float days)
{
	float remain = days;

	int d = static_cast<int> (remain); remain -= d;
	remain *= 24.0f;
	int h = static_cast<int> (remain); remain -= h;
	remain *= 60.0f;
	int m = static_cast<int> (remain); remain -= m; 
	remain *= 60.0f;

	auto r = QString("%1%2 %3%4 %5%6 %7%8")
	.arg(d)		.arg(qc_("d", "duration"))
	.arg(h)		.arg(qc_("h", "duration"))
	.arg(m)		.arg(qc_("m", "duration"))
	.arg(remain)	.arg(qc_("s", "duration"));

	return r;
}


/*************************************************************************
 Convert an angle in radian to hms
*************************************************************************/
void radToHms(double angle, unsigned int& h, unsigned int& m, double& s)
{
	angle = std::fmod(angle,2.0*M_PI);
	if (angle < 0.0) angle += 2.0*M_PI; // range: [0..2.0*M_PI)

	angle *= 12./M_PI;

	h = static_cast<unsigned int>(angle);
	m = static_cast<unsigned int>((angle-h)*60);
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
	angle *= M_180_PI;

	d = static_cast<unsigned int>(angle);
	m = static_cast<unsigned int>((angle - d)*60);
	s = (angle-d)*3600-60*m;
	// workaround for rounding numbers	
	if (s>59.9)
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

void radToDecDeg(double rad, bool &sign, double &deg)
{
	rad = std::fmod(rad,2.0*M_PI);
	sign=true;
	if (rad<0)
	{
		rad *= -1;
		sign = false;
	}
	deg = rad*M_180_PI;
}

QString radToDecDegStr(const double angle, const int precision, const bool useD, const bool useC)
{
	const QChar degsign = (useD ? 'd' : 0x00B0);
	bool sign;
	double deg;
	StelUtils::radToDecDeg(angle, sign, deg);
	QString str = QString("%1%2%3").arg((sign?"+":"-"), QString::number(deg, 'f', precision), degsign);
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
	if (std::fabs(s*100-static_cast<int>(s)*100)>=1)
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
	else if (static_cast<int>(s)!=0)
	{
		ts << m << 'm' << static_cast<int>(s) << 's';
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
 If decimal is true,  output should be like this: "  16h20m00.4s"
 If decimal is false, output should be like this: "   0h26m5s"
*************************************************************************/
QString radToHmsStr(const double angle, const bool decimal)
{
	unsigned int h,m;
	double s;
	StelUtils::radToHms(angle+0.005*M_PI/12/(60*60), h, m, s);
	int width, precision;
	QString carry, r;
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
		s=0.;
		m+=1;
	}
	if (m==60)
	{
		m=0;
		h+=1;
	}
	if (h==24 && m==0 && s==0.)
		h=0;

	return QString("%1h%2m%3s").arg(h, width).arg(m, 2, 10, QChar('0')).arg(s, 3+precision, 'f', precision, QChar('0'));
}

/*************************************************************************
 Convert an angle in radian to a dms formatted string
 If the minute and second part are null are too small, don't print them
*************************************************************************/
QString radToDmsStrAdapt(const double angle, const bool useD)
{
	const QChar degsign = (useD ? 'd' : 0x00B0);
	bool sign;
	unsigned int d,m;
	double s;
	StelUtils::radToDms(angle+0.005*M_PI/180/(60*60)*(angle<0?-1.:1.), sign, d, m, s); // NOTE: WTF???
	QString str;
	QTextStream os(&str);

	os << (sign?'+':'-') << d << degsign;
	if (std::fabs(s*100-static_cast<int>(s)*100)>=1)
	{
		os << m << '\'' << fixed << qSetRealNumberPrecision(2) << qSetFieldWidth(5) << qSetPadChar('0') << s << qSetFieldWidth(0) << '\"';
	}
	else if (static_cast<int>(s)!=0)
	{
		os << m << '\'' << static_cast<int>(s) << '\"';
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
	const int precision = decimal ? 1 : 0;
	return StelUtils::radToDmsPStr(angle, precision, useD);
}

/*************************************************************************
 Convert an angle in radian to a dms formatted string
*************************************************************************/
QString radToDmsPStr(const double angle, const int precision, const bool useD)
{
	const QChar degsign = (useD ? 'd' : 0x00B0);
	bool sign;
	unsigned int d,m;
	double s;
	StelUtils::radToDms(angle, sign, d, m, s);
	QString str;
	QTextStream os(&str);
	os << (sign?'+':'-') << d << degsign;

	os << qSetFieldWidth(2) << qSetPadChar('0') << m << qSetFieldWidth(0) << '\'';
	int width = 2;
	if (precision>0)
		width = 3 + precision;
	os << qSetRealNumberPrecision(precision);
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

	d = static_cast<unsigned int>(angle);
	m = static_cast<unsigned int>((angle-d)*60);
	s = (angle-d)*3600.-60.*m;

	if (s>59.9)
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
	bool sign = (reg.cap(1) == "-");
	int deg = reg.cap(2).toInt();
	uint min = reg.cap(3).toUInt();
	int sec = reg.cap(4).toInt();

	double rad = dmsToRad(qAbs(deg), min, sec);
	if (sign)
		rad *= -1;

	return rad;
}

double getDecAngle(const QString& str)
{
	QRegExp rex("([-+]?)\\s*"                         // [sign] (1)
		    "(?:"                                 // either
		    "(\\d+(?:\\.\\d+)?)\\s*"               // fract (2)
		    "([dhms°º]?)"                          // [dhms] (3) \u00B0\u00BA
		    "|"                                   // or
		    "(?:(\\d+)\\s*([hHdD°º])\\s*)?"         // [int degs] (4) (5)
		    "(?:"                                   // either
		    "(?:(\\d+)\\s*['mM]\\s*)?"              //  [int mins]  (6)
		    "(\\d+(?:\\.\\d+)?)\\s*[\"sS]"          //  fract secs  (7)
		    "|"                                     // or
		    "(\\d+(?:\\.\\d+)?)\\s*['mM]"           //  fract mins (8)
		    ")"                                     // end
		    ")"                                   // end
		    "\\s*([NSEW]?)",                      // [point] (9)
		    Qt::CaseInsensitive);
	if( rex.exactMatch(str) )
	{
		QStringList caps = rex.capturedTexts();
#if 0
		std::cout << "reg exp: ";
		for( int i = 1; i <= rex.captureCount() ; ++i ){
			std::cout << i << "=\"" << caps.at(i).toStdString() << "\" ";
		}
		std::cout << std::endl;
#endif
		double d = 0;
		double m = 0;
		double s = 0;
		ushort hd = caps.at(5).isEmpty() ? 'd' : caps.at(5).toLower().at(0).unicode();
		QString pointStr = caps.at(9).toUpper() + " ";
		if( caps.at(7) != "" )
		{
			// [dh, degs], [m] and s entries at 4, 5, 6, 7
			d = caps.at(4).toDouble();
			m = caps.at(6).toDouble();
			s = caps.at(7).toDouble();
		}
		else if( caps.at(8) != "" )
		{
			// [dh, degs] and m entries at 4, 5, 8
			d = caps.at(4).toDouble();
			m = caps.at(8).toDouble();
		}
		else if( caps.at(2) != "" )
		{
			// some value at 2, dh|m|s at 3
			double x = caps.at(2).toDouble();
			QString sS = caps.at(3) + caps.at(9);
			switch( sS.length() )
			{
				case 0:
					// degrees, no point
					hd = 'd';
					break;
				case 1:
					// NnSEeWw is point for degrees, "dhms°..." distinguish dhms
					if( QString("NnSEeWw").contains(sS.at(0)) )
					{
						pointStr = sS.toUpper();
						hd = 'd';
					}
					else
						hd = sS.toLower().at(0).unicode();
					break;
				case 2:
					// hdms selected by 1st char, NSEW by 2nd
					hd = sS.at(0).toLower().unicode();
					pointStr = sS.right(1).toUpper();
					break;
			}
			switch( hd )
			{
				case 'h':
				case 'd':
				case 0x00B0:
				case 0x00BA:
					d = x;
					break;
				case 'm':
				case '\'':
					m = x;
					break;
				case 's':
				case '"':
					s = x;
					break;
				default:
					qDebug() << "internal error, hd = " << hd;
			}
		}
		else
		{
			qDebug() << "getDecAngle failed to parse angle string: " << str;
			return -0.0;
		}

		// General sign handling: group 1 or overruled by point
		int sgn = caps.at(1) == "-" ? -1 : 1;
		bool isNS = false;
		switch( pointStr.at(0).unicode() )
		{
			case 'N':
				sgn = 1;
				isNS = 1;
				break;
			case 'S':
				sgn = -1;
				isNS = 1;
				break;
			case 'E':
				sgn = 1;
				break;
			case 'W':
				sgn = -1;
				break;
			default:  // OK, there is no NSEW.
				break;
		}

		int h2d = 1;
		if( hd == 'h' )
		{
			// Sanity check - h and N/S not accepted together
			if( isNS  )
			{
				qDebug() << "getDecAngle does not accept ...H...N/S: " << str;
				return -0.0;
			}
			h2d = 15;
		}
		double deg = (d + (m/60) + (s/3600))*h2d*sgn;
		return deg * 2 * M_PI / 360.;
	}

	qDebug() << "getDecAngle failed to parse angle string: " << str;
	return -0.0;
}

// Return the first power of two bigger than the given value
int getBiggerPowerOfTwo(int value)
{
	int p=1;
	while (p<value)
		p<<=1;
	return p;
}

/*************************************************************************
 Convert a QT QDateTime class to julian day
*************************************************************************/

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
	 * "Numerical Recipes in C, 2nd Ed." (1992), pp. 14-15
	 * and converted to integer math.
	 * The electronic version of the book is freely available
	 * at http://www.nr.com/ , under "Obsolete Versions - Older
	 * book and code versions".
	 */

	static const long JD_GREG_CAL = 2299161;
	static const int JB_MAX_WITHOUT_OVERFLOW = 107374182;
	const long julian = static_cast<long>(floor(jd + 0.5));

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
		tc = static_cast<long>((static_cast<unsigned long long>(tb)*20 - 2442) / 7305);
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

void getTimeFromJulianDay(const double julianDay, int *hour, int *minute, int *second, int *millis)
{
	double frac = julianDay - (floor(julianDay));
	double secs = frac * 24.0 * 60.0 * 60.0 + 0.0001; // add constant to fix floating-point truncation error
	int s = static_cast<int>(floor(secs));

	*hour = ((s / (60 * 60))+12)%24;
	*minute = (s/(60))%60;
	*second = s % 60;
	if(millis)
	{
		*millis = static_cast<int>(floor((secs - floor(secs)) * 1000.0));
	}
}

QString julianDayToISO8601String(const double jd, bool addMS)
{
	int year, month, day, hour, minute, second, millis=0;
	getDateFromJulianDay(jd, &year, &month, &day);
	getTimeFromJulianDay(jd, &hour, &minute, &second, addMS ? &millis : Q_NULLPTR );

	QString res = QString("%1-%2-%3T%4:%5:%6")
				 .arg((year >= 0 ? year : -1* year),4,10,QLatin1Char('0'))
				 .arg(month,2,10,QLatin1Char('0'))
				 .arg(day,2,10,QLatin1Char('0'))
				 .arg(hour,2,10,QLatin1Char('0'))
				 .arg(minute,2,10,QLatin1Char('0'))
				 .arg(second,2,10,QLatin1Char('0'));

	if(addMS)
	{
		res.append(".%1").arg(millis,3,10,QLatin1Char('0'));
	}
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

	for (int i = 0; i < fmt.length(); i++)
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
				out += StelLocaleMgr::shortDayName(dayOfWeek+1);
			}
			else if (frag == "dddd")
			{
				out += StelLocaleMgr::longDayName(dayOfWeek+1);
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
				out += StelLocaleMgr::shortMonthName(month);
			}
			else if (frag == "MMMM")
			{
				out += StelLocaleMgr::longMonthName(month);
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
				int dispyear = abs(year);
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
	if (year > 0 && test.isValid() && !test.toString(Qt::DefaultLocaleShortDate).isEmpty())
	{
		return test.toString(Qt::DefaultLocaleShortDate);
	}
	else
	{
		return localeDateString(year,month,day,dayOfWeek,QLocale().dateFormat(QLocale::ShortFormat));
	}
}

int getDayOfWeek(int year, int month, int day)
{
	double JD;
	getJDFromDate(&JD, year, month, day, 0, 0, 0);
	return getDayOfWeek(JD);
}

//! use QDateTime to get a Julian Date from the system's current time.
//! this is an acceptable use of QDateTime because the system's current
//! time is more than likely always going to be expressible by QDateTime.
double getJDFromSystem()
{
	return qDateTimeToJd(QDateTime::currentDateTime().toUTC());
}

double getJDFromBesselianEpoch(const double epoch)
{
	return 2400000.5 + (15019.81352 + (epoch - 1900.0) * 365.242198781);
}

double qTimeToJDFraction(const QTime& time)
{
	return static_cast<double>(1./(24*60*60*1000)*QTime(0, 0, 0, 0).msecsTo(time))-0.5;
}

QTime jdFractionToQTime(const double jd)
{
	double decHours = std::fmod(jd+0.5, 1.0);
	int hours = static_cast<int>(decHours/0.041666666666666666666);
	int mins = static_cast<int>((decHours-(hours*0.041666666666666666666))/0.00069444444444444444444);
	return QTime::fromString(QString("%1.%2").arg(hours).arg(mins), "h.m");
}

// UTC !
bool getJDFromDate(double* newjd, const int y, const int m, const int d, const int h, const int min, const float s)
{
	static const long IGREG2 = 15+31L*(10+12L*1582);
	double deltaTime = (h / 24.0) + (min / (24.0*60.0)) + (static_cast<double>(s) / (24.0 * 60.0 * 60.0)) - 0.5;
	QDate test((y <= 0 ? y-1 : y), m, d);
	// if QDate will oblige, do so.
	// added hook for Julian calendar, because it has been removed from Qt5 --AW
	if ( test.isValid() && y>1582)
	{
		double qdjd = static_cast<double>(test.toJulianDay());
		qdjd += deltaTime;
		*newjd = qdjd;
		return true;
	}
	else
	{
		/*
		 * Algorithm taken from "Numerical Recipes in C, 2nd Ed." (1992), pp. 11-12
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
		double jd = static_cast<double>(ljul);
		jd += deltaTime;
		*newjd = jd;
		return true;
	}
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
		case 4:
		case 6:
		case 9:
		case 11:
			return 30;
		case 2:
			if ( year > 1582 )
			{
				if ( year % 4 == 0 )
				{
					if ( year % 100 == 0 )
					{
						return ( year % 400 == 0 ? 29 : 28 );
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
				return ( year % 4 == 0 ? 29 : 28 );
			}
		case 0:
			return numberOfDaysInMonthInYear(12, year-1);
		case 13:
			return numberOfDaysInMonthInYear(1, year+1);
		default:
			break;
	}

	return 0;
}

// return true if year is a leap year. Observes 1582 switch from Julian to Gregorian Calendar.
bool isLeapYear(const int year)
{
	if (year>1582)
	{
		if (year % 100 == 0)
			return (year % 400 == 0);
		else
			return (year % 4 == 0);
	}
	else
		return (imod(year, 4) == (year > 0 ? 0 : 3) );
}

// Find day number for date in year.
// Meeus, AA 2nd, 1998, ch.7 p.65
int dayInYear(const int year, const int month, const int day)
{
	int k=(isLeapYear(year) ? 1:2);
	return static_cast<int>(275*month/9) - k*static_cast<int>((month+9)/12) + day -30;
}

// Return a fractional year like YYYY.ddddd. For negative years, the year number is of course decrease. E.g. -500.5 occurs in -501.
double yearFraction(const int year, const int month, const double day)
{
	double d=dayInYear(year, month, 0)+day;
	double daysInYear=( isLeapYear(year) ? 366.0 : 365.0);
	return year+d/daysInYear;
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
		std::sort(keys.begin(), keys.end());
		for (auto k : keys)
		{
			debugQVariantMap(m.toMap()[k], indent + "    ", k);
		}
	}
	else if (t == QVariant::List)
	{
		qDebug() << indent + key + "(list):";
		for (const auto& item : m.toList())
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
		*y = finalRe.cap(1).toInt(&ok);
		error = error || !ok;
		*m = finalRe.cap(2).toInt(&ok);
		error = error || !ok;
		*d = finalRe.cap(3).toInt(&ok);
		error = error || !ok;
		*h = finalRe.cap(4).toInt(&ok);
		error = error || !ok;
		*min = finalRe.cap(5).toInt(&ok);
		error = error || !ok;
		*s = finalRe.cap(6).toFloat(&ok);
		error = error || !ok;
		if (!error)
			return true;
	}
	return false;
}

QString hoursToHmsStr(const double hours, const bool lowprecision)
{
	int h = static_cast<int>(hours);
	double minutes = (qAbs(hours)-qAbs(double(h)))*60.;
	if (lowprecision)
	{
		int m = qRound(minutes);
		if (m==60)
		{
			h += 1;
			m = 0;
		}
		return QString("%1h%2m").arg(h).arg(m, 2, 10, QChar('0'));
	}
	else
	{
		int m = static_cast<int>(minutes);
		float s = static_cast<float>((((qAbs(hours)-qAbs(double(h)))*60.)-m)*60.);
		if (s>59.9f)
		{
			m += 1;
			s = 0.f;
		}
		if (m==60)
		{
			h += 1;
			m = 0;
		}
		return QString("%1h%2m%3s").arg(h).arg(m, 2, 10, QChar('0')).arg(s, 4, 'f', 1, QChar('0'));
	}
}

QString hoursToHmsStr(const float hours, const bool lowprecision)
{
	return hoursToHmsStr(static_cast<double>(hours), lowprecision);
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
//
// Up to V0.15.1, if the requested year was outside validity range, we returned zero or some useless value.
// Starting with V0.15.2 the value from the edge of the range is returned instead.
*/

double getDeltaTwithoutCorrection(const double jDay)
{
	Q_UNUSED(jDay)
	return 0.;
}

// Implementation of algorithm by Espenak & Meeus (2006) for DeltaT computation
double getDeltaTByEspenakMeeus(const double jDay)
{
	int year, month, day;	
	getDateFromJulianDay(jDay, &year, &month, &day);

	// Note: the method here is adapted from
	// "Five Millennium Canon of Solar Eclipses" [Espenak and Meeus, 2006]
	// A summary is described here:
	// http://eclipse.gsfc.nasa.gov/SEhelp/deltatpoly2004.html

	double y = yearFraction(year, month, day);

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
	return (29.950*u +72.318)*u +24.349 + 1.82144*getMoonFluctuation(jDay)  ;
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

// Implementation of algorithm by Schmadel & Zech (1979) for DeltaT computation. STRICTLY 1800...1975 ONLY!! Now delivers values for the edges.
double getDeltaTBySchmadelZech1979(const double jDay)
{
	double u=(jDay-2415020.0)/36525.0; // (1900-jan-0.5)
	u=qBound(-1.0, u, 0.76);  // Limit range to 1800...1975. Else we have crazy values which cause strange artefacts.
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

	// Limited years!
	year=qBound(-391, year, 1600);

	double u = (yearFraction(year, month, day)-1800)/100;

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
	// This formula found in the cited book, page (ii), formula (1).
	double T=(jDay-2415020.0)/36525; // centuries from J1900.0
	return (36.79*T+35.06)*T+4.87;
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

// Implementation of algorithm by Schmadel & Zech (1988) for DeltaT computation. STRICTLY 1800...1988 ONLY!! Now delivers values for the edges.
double getDeltaTBySchmadelZech1988(const double jDay)
{
	double u=(jDay-2415020.0)/36525.0; // (1900-jan-0.5)
	u=qBound(-1.0, u, 0.89);  // Limit range to 1800...1988. Else we have crazy values which cause strange artefacts.
	double deltaT = (((((((((((-0.058091*u -0.067471)*u +.145932)*u +.161416)*u -.149279)*u -.146960)*u +.079441)*u +.062971)*u -.022542)*u -.012462)*u +.003357)*u +.001148)*u-.000014;
	return deltaT * 86400.0;
}

// Implementation of algorithm by Chapront-Touzé & Chapront (1991) for DeltaT computation
double getDeltaTByChaprontTouze(const double jDay)
{
	int year, month, day;	
	double deltaT = 0.;
	getDateFromJulianDay(jDay, &year, &month, &day);

	// Limited years!
	year=qBound(-391, year, 1600);

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

	// Limited years!
	year=qBound(-2999, year, 1620);

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
		double yeardec=yearFraction(year, month, day);
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
	const double T=(jDay-2451545.)/36525.;
	double t;
	if (jDay<2387627.5 || jDay >=2453736.5) // ...1825-01-01 0:00 or 2006-01-01 0:00...
		deltaT=0.0;
	else if (jDay < 2396758.5) // 1850-01-01 0:00
	{
		t=T+1.75;
		deltaT=(( -572.3*t+413.9)*t  -80.8)*t +10.4;
	}
	else if (jDay < 2405889.5) // 1875-01-01 0:00
	{
		t=T+1.50;
		deltaT=((   18.8*t-358.4)*t  +46.3)*t + 6.6;
	}
	else if (jDay < 2415020.5) // 1900-01-01 0:00
	{
		t=T+1.25;
		deltaT=((  867.4*t-166.2)*t  -10.8)*t - 3.9;
	}
	else if (jDay < 2424151.5) // 1925-01-01 0:00
	{
		t=T+1.00;
		deltaT=((-1467.4*t+327.5)*t +114.1)*t - 2.6;
	}
	else if (jDay < 2433282.5) // 1950-01-01 0:00
	{
		t=T+0.75;
		deltaT=((  483.4*t - 8.2)*t  - 6.3)*t +24.2;
	}
	else if (jDay < 2442413.5) // 1975-01-01 0:00
	{
		t=T+0.50;
		deltaT=((  550.7*t - 3.8)*t  +32.5)*t +29.3;
	}
	else if (jDay < 2451545.5) // 2000-01-01 0:00
	{
		t=T+0.25;
		deltaT=(( 1516.7*t-570.5)*t +130.5)*t +45.3;
	}
	else if (jDay < 2453736.5) // 2006-01-01 0:00 [extrapolation from 2000]
	{
		t=T+0.5;
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
	double deltaT = 0.0;
	getDateFromJulianDay(jDay, &year, &month, &day);
	const double ub=(jDay-2451545.0)/36525.0; // (2000-jan-1.5)

	if (year<1620)
		deltaT = 0.0;
	else if (year < 1690)
	{
		u = 3.45 + ub;
		deltaT = (((1244.0*u -454.0)*u + 50.0)*u -107.0)*u +40.3;
	}
	else if (year < 1770)
	{
		u = 2.70 + ub;
		deltaT = (((70.0*u -16.0)*u -1.0)*u +11.3)*u +10.2;
	}
	else if (year < 1820)
	{
		u = 2.05 + ub;
		deltaT = (((6.0*u +173.0)*u -22.0)*u -18.8)*u +14.7;
	}
	else if (year < 1870)
	{
		u = 1.55 + ub;
		deltaT = (((-1654.0*u -534.0)*u +111)*u +12.7)*u +5.7;
	}
	else if (year < 1900)
	{
		u = 1.15 + ub;
		deltaT = (((8234.0*u +101.0)*u +27.0)*u - 14.6)*u -5.8;
	}
	else if (year < 1940)
	{
		u = 0.80 + ub;
		deltaT = (((4441.0*u + 19.0)*u -443.0)*u +67.0)*u +21.4;
	}
	else if (year < 1990)
	{
		u = 0.35 + ub;
		deltaT = (((-1883.0*u -140.0)*u +189.0)*u +74.0)*u +36.2;
	}
	else if (year <= 2000)
	{
		u = 0.05 + ub;
		deltaT = ((-5034.0*u -188.0)*u +82.0)*u +60.8;
	}

	return deltaT;
}

// Implementation of algorithm by Reingold & Dershowitz (Cal. Calc. 1997, 2001, 2007, 2018, Cal. Tab. 2002) for DeltaT computation.
// Created as yet another multi-segment polynomial fit through the table in Meeus: Astronomical Algorithms (1991).
// Note that only the Third edition (2007) adds the 1700-1799 term.
// Note that only the ultimate edition (2018) adds the -500..1699 and 2006..2150 terms.
// More efficient reimplementation with stricter adherence to the source.
double getDeltaTByReingoldDershowitz(const double jDay)
{
	int year, month, day;	
	getDateFromJulianDay(jDay, &year, &month, &day);
	// R&D don't use a float-fraction year, but explicitly only the integer year! And R&D use a proleptic Gregorian year before 1582.
	// We cannot do that, but the difference is negligible.
	double deltaT=0.0; // If it returns 0, there is a bug!

	if ((year>= 2051) && (year <= 2150))
	{
		// [2051..2150]
		double x = (year-1820)/100.;
		deltaT = (- 20 + 32*x*x + 0.5628*(2150-year));
	}
	else if ((year >= 1987) && (year <= 2050))
	{
		int y2000 = year-2000;
		if (year>=2006) // [2006..2050]
		{
			deltaT = ((0.005589*y2000 + 0.32217)*y2000 + 62.92);
		}
		else  // [1987..2005]
		{
			deltaT = (((((0.00002373599*y2000 + 0.000651814)*y2000 + 0.0017275)*y2000 - 0.060374)*y2000 + 0.3345)*y2000 + 63.86);
		}
	}
	else if ((year >= 1800) && (year <= 1986))
	{
		// FIXME: This part should be check because this part gives the strange values of DeltaT (see unit tests)
		double c = (getFixedFromGregorian(1900, 1, 1)-getFixedFromGregorian(year, 7, 1))/36525.;

		if (year >= 1900) // [1900..1986]
		{
			deltaT = ((((((-0.212591*c + 0.677066)*c - 0.861938)*c + 0.553040)*c - 0.181133)*c + 0.025184)*c + 0.000297)*c - 0.00002;
		}
		else    // [1800..1899]
		{
			deltaT = (((((((((2.043794*c + 11.636204)*c + 28.316289)*c + 38.291999)*c + 31.332267)*c + 15.845535)*c + 4.867575)*c + 0.865736)*c + 0.083563)*c + 0.003844)*c - 0.000009;
		}
		deltaT *= 86400.; // convert to seconds
	}
	else if ((year>=1700) && (year<=1799))
	{
		// [1700..1799]
		int y1700 = year-1700;
		deltaT = (((-0.0000266484*y1700 + 0.003336121)*y1700 - 0.005092142)*y1700 + 8.118780842);
	}
	else if ((year>=1600) && (year<=1699))
	{
		// [1600..1699]
		int y1600 = year-1600;
		deltaT = (((0.000140272128*y1600 - 0.01532)*y1600 - 0.9808)*y1600 + 120);
	}
	else if ((year>=500) && (year<=1599))
	{
		// [500..1599]
		double y1000 = (year-1000)/100.;
		deltaT = ((((((0.0083572073*y1000 - 0.005050998)*y1000 - 0.8503463)*y1000 + 0.319781)*y1000 + 71.23472)*y1000 - 556.01)*y1000 + 1574.2);
	}
	else if ((year>-500) && (year<500))
	{
		// (-500..500)
		double y0 = year/100.;
		deltaT = ((((((0.0090316521*y0 + 0.022174192)*y0 - 0.1798452)*y0 - 5.952053)*y0 + 33.78311)*y0 - 1014.41)*y0 + 10583.6);
	}
	else
	{
		// otherwise
		double x = (year-1820)/100.;
		deltaT = (-20 + 32*x*x);
	}

	return deltaT;
}

// Implementation of algorithm by Banjevic (2006) for DeltaT computation.
double getDeltaTByBanjevic(const double jDay)
{
	int year, month, day;
	getDateFromJulianDay(jDay, &year, &month, &day);
	double u, c;

	// Limited years!
	year=qBound(-2020, year, 1620);

	if (year<=-700)
	{
		u = (jDay-2378496.0)/36525.0; // 1800.0=1800-jan-0.5=2378496.0
		c = 30.86;
	}
	else //  if (year>-700 && year<=1620)
	{
		u = (jDay-2385800.0)/36525.0; // 1820.0=1820-jan-0.5=2385800.0
		c = 31;
	}

	return c*u*u;
}

// Implementation of algorithm by Islam, Sadiq & Qureshi (2008 + revisited 2013) for DeltaT computation.
double getDeltaTByIslamSadiqQureshi(const double jDay)
{
	int year, month, day;
	getDateFromJulianDay(jDay, &year, &month, &day);
	double deltaT; // Return deltaT for the edge year outside valid range.
	double u, ub;

	// Limited years: Apply border values!
	//year=qBound(1620, year, 2007);
	if (year<1620)
	{
		const double j1620=qDateTimeToJd(QDateTime(QDate(1620, 1, 1), QTime(0, 0, 0), Qt::UTC));
		ub=(j1620-2454101.0)/36525.0;
	}
	else if (year>2007)
	{
		const double j2008=qDateTimeToJd(QDateTime(QDate(2008, 1, 1), QTime(0, 0, 0), Qt::UTC));
		ub=(j2008-2454101.0)/36525.0;
	}
	else
		ub=(jDay-2454101.0)/36525.0; // (2007-jan-0.5)


	if (year <= 1698)
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
	else //if (year <= 2007)
	{
		// revised 2013 per email
		u = 0.265 + ub;
		deltaT = (((1314.759 * u - 296.018) * u - 101.898) * u + 88.659) * u + 49.997;
	}

	return deltaT;
}

// Implementation of polynomial approximation of time period 1620-2013 for DeltaT by M. Khalid, Mariam Sultana and Faheem Zaidi (2014).
double getDeltaTByKhalidSultanaZaidi(const double jDay)
{
	int year, month, day;
	getDateFromJulianDay(jDay, &year, &month, &day);
	//double a0, a1, a2, a3, a4;
	const double k[9] ={     3.670,    3.120,    2.495,     1.925,     1.525,    1.220,     0.880,     0.455,      0.115};
	const double a0[9]={    76.541,   10.872,   13.480,    12.584,     6.364,   -5.058,    13.392,    30.782,     55.281};
	const double a1[9]={  -253.532,  -40.744,   13.075,     1.929,    11.004,   -1.701,   128.592,    34.348,     91.248};
	const double a2[9]={   695.901,  236.890,    8.635,    60.896,   407.776,  -46.403,  -279.165,    46.452,     87.202};
	const double a3[9]={ -1256.982, -351.537,   -3.307, -1432.216, -4168.394, -866.171, -1282.050,  1295.550,  -3092.565};
	const double a4[9]={   627.152,   36.612, -128.294,  3129.071,  7561.686, 5917.585,  4039.490, -3210.913,   8255.422};
	int i;
	// Limited years! Deliver border values.
	year=qBound(1620, year, 2013);

	if (year<=1672)
		i=0;
	else if (year<=1729)
		i=1;
	else if (year<=1797)
		i=2;
	else if (year<=1843)
		i=3;
	else if (year<=1877)
		i=4;
	else if (year<=1904)
		i=5;
	else if (year<=1945)
		i=6;
	else if (year<=1989)
		i=7;
	else // if (year<=2013)
		i=8;

	double u = k[i] + (year - 2000.)/100.; // Avoid possible wrong calculations!

	return (((a4[i]*u + a3[i])*u + a2[i])*u + a1[i])*u + a0[i];
}

static const double StephensonMorrisonHohenkerk2016DeltaTtableS15[54][6]={
//	Row         Years                  Polynomial Coefficients
//	  i      K_i    K_{i+1}        a_0         a_1         a_2         a_3
//	- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
/*	  1 */  {-720.0,     400.0,   20550.593,  -21268.478,   11863.418,   -4541.129},
/*	  2 */  { 400.0,    1000.0,    6604.404,   -5981.266,    -505.093,    1349.609},
/*	  3 */  {1000.0,    1500.0,    1467.654,   -2452.187,    2460.927,   -1183.759},
/*	  4 */  {1500.0,    1600.0,     292.635,    -216.322,     -43.614,      56.681},
/*	  5 */  {1600.0,    1650.0,      89.380,     -66.754,      31.607,     -10.497},
/*	  6 */  {1650.0,    1720.0,      43.736,     -49.043,       0.227,      15.811},
/*	  7 */  {1720.0,    1800.0,      10.730,      -1.321,      62.250,     -52.946},
/*	  8 */  {1800.0,    1810.0,      18.714,      -4.457,      -1.509,       2.507},
/*	  9 */  {1810.0,    1820.0,      15.255,       0.046,       6.012,      -4.634},
/*	 10 */  {1820.0,    1830.0,      16.679,      -1.831,      -7.889,       3.799},
/*	 11 */  {1830.0,    1840.0,      10.758,      -6.211,       3.509,      -0.388},
/*	 12 */  {1840.0,    1850.0,       7.668,      -0.357,       2.345,      -0.338},
/*	 13 */  {1850.0,    1855.0,       9.317,       1.659,       0.332,      -0.932},
/*	 14 */  {1855.0,    1860.0,      10.376,      -0.472,      -2.463,       1.596},
/*	 15 */  {1860.0,    1865.0,       9.038,      -0.610,       2.325,      -2.497},
/*	 16 */  {1865.0,    1870.0,       8.256,      -3.450,      -5.166,       2.729},
/*	 17 */  {1870.0,    1875.0,       2.369,      -5.596,       3.020,      -0.919},
/*	 18 */  {1875.0,    1880.0,      -1.126,      -2.312,       0.264,      -0.037},
/*	 19 */  {1880.0,    1885.0,      -3.211,      -1.894,       0.154,       0.562},
/*	 20 */  {1885.0,    1890.0,      -4.388,       0.101,       1.841,      -1.438},
/*	 21 */  {1890.0,    1895.0,      -3.884,      -0.531,      -2.473,       1.870},
/*	 22 */  {1895.0,    1900.0,      -5.017,       0.134,       3.138,      -0.232},
/*	 23 */  {1900.0,    1905.0,      -1.977,       5.715,       2.443,      -1.257},
/*	 24 */  {1905.0,    1910.0,       4.923,       6.828,      -1.329,       0.720},
/*	 25 */  {1910.0,    1915.0,      11.142,       6.330,       0.831,      -0.825},
/*	 26 */  {1915.0,    1920.0,      17.479,       5.518,      -1.643,       0.262},
/*	 27 */  {1920.0,    1925.0,      21.617,       3.020,      -0.856,       0.008},
/*	 28 */  {1925.0,    1930.0,      23.789,       1.333,      -0.831,       0.127},
/*	 29 */  {1930.0,    1935.0,      24.418,       0.052,      -0.449,       0.142},
/*	 30 */  {1935.0,    1940.0,      24.164,      -0.419,      -0.022,       0.702},
/*	 31 */  {1940.0,    1945.0,      24.426,       1.645,       2.086,      -1.106},
/*	 32 */  {1945.0,    1950.0,      27.050,       2.499,      -1.232,       0.614},
/*	 33 */  {1950.0,    1953.0,      28.932,       1.127,       0.220,      -0.277},
/*	 34 */  {1953.0,    1956.0,      30.002,       0.737,      -0.610,       0.631},
/*	 35 */  {1956.0,    1959.0,      30.760,       1.409,       1.282,      -0.799},
/*	 36 */  {1959.0,    1962.0,      32.652,       1.577,      -1.115,       0.507},
/*	 37 */  {1962.0,    1965.0,      33.621,       0.868,       0.406,       0.199},
/*	 38 */  {1965.0,    1968.0,      35.093,       2.275,       1.002,      -0.414},
/*	 39 */  {1968.0,    1971.0,      37.956,       3.035,      -0.242,       0.202},
/*	 40 */  {1971.0,    1974.0,      40.951,       3.157,       0.364,      -0.229},
/*	 41 */  {1974.0,    1977.0,      44.244,       3.198,      -0.323,       0.172},
/*	 42 */  {1977.0,    1980.0,      47.291,       3.069,       0.193,      -0.192},
/*	 43 */  {1980.0,    1983.0,      50.361,       2.878,      -0.384,       0.081},
/*	 44 */  {1983.0,    1986.0,      52.936,       2.354,      -0.140,      -0.166},
/*	 45 */  {1986.0,    1989.0,      54.984,       1.577,      -0.637,       0.448},
/*	 46 */  {1989.0,    1992.0,      56.373,       1.649,       0.709,      -0.277},
/*	 47 */  {1992.0,    1995.0,      58.453,       2.235,      -0.122,       0.111},
/*	 48 */  {1995.0,    1998.0,      60.677,       2.324,       0.212,      -0.315},
/*	 49 */  {1998.0,    2001.0,      62.899,       1.804,      -0.732,       0.112},
/*	 50 */  {2001.0,    2004.0,      64.082,       0.675,      -0.396,       0.193},
/*	 51 */  {2004.0,    2007.0,      64.555,       0.463,       0.184,      -0.008},
/*	 52 */  {2007.0,    2010.0,      65.194,       0.809,       0.161,      -0.101},
/*	 53 */  {2010.0,    2013.0,      66.063,       0.828,      -0.142,       0.168},
/*	 54 */  {2013.0,    2016.0,      66.917,       1.046,       0.360,      -0.282}
};
double getDeltaTByStephensonMorrisonHohenkerk2016(const double jDay)
{
	int year, month, day;
	getDateFromJulianDay(jDay, &year, &month, &day);
	double y=yearFraction(year, month, day);
	if ((y<-720.) || (y>2016.))
	{
		double fact=(y-1825.0)/100.;
		return -320.0+32.5*fact*fact;
	}
	int i=0;
	while (StephensonMorrisonHohenkerk2016DeltaTtableS15[i][1]<y) i++;
	Q_ASSERT(i<54);
	double t=(y-StephensonMorrisonHohenkerk2016DeltaTtableS15[i][0]) / (StephensonMorrisonHohenkerk2016DeltaTtableS15[i][1]-StephensonMorrisonHohenkerk2016DeltaTtableS15[i][0]);
	return ((StephensonMorrisonHohenkerk2016DeltaTtableS15[i][5]*t + StephensonMorrisonHohenkerk2016DeltaTtableS15[i][4])*t
		+ StephensonMorrisonHohenkerk2016DeltaTtableS15[i][3])*t + StephensonMorrisonHohenkerk2016DeltaTtableS15[i][2];
}

double getMoonSecularAcceleration(const double jDay, const double nd, const bool useDE43x)
{
	int year, month, day;
	getDateFromJulianDay(jDay, &year, &month, &day);

	double t = (yearFraction(year, month, day)-1955.5)/100.0;
	// n.dot for secular acceleration of the Moon in ELP2000-82B
	// have value -23.8946 "/cy/cy (or -25.8 for DE43x usage)
	double ephND = -23.8946;
	if (useDE43x)
		ephND = -25.8;

	return -0.91072 * (ephND + qAbs(nd))*t*t;
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

// Current table contains interpolated data of original table by Spencer Jones, H., "The Rotation of the Earth, and the Secular
// Accelerations of the Sun, Moon and Planets", Monthly Notices of the Royal Astronomical Society, 99 (1939), 541-558
// [http://adsabs.harvard.edu/abs/1939MNRAS..99..541S] see Table I.
static const double MoonFluctuationTable[2555] = {
	-12.720, -12.691, -12.662, -12.632, -12.603, -12.574, -12.545, -12.516, -12.486, -12.457, -12.428, -12.399, -12.369, -12.340,
	-12.311, -12.282, -12.253, -12.223, -12.194, -12.165, -12.136, -12.107, -12.077, -12.048, -12.019, -11.990, -11.960, -11.931,
	-11.902, -11.873, -11.843, -11.814, -11.785, -11.756, -11.726, -11.697, -11.668, -11.639, -11.609, -11.580, -11.551, -11.522,
	-11.492, -11.463, -11.434, -11.404, -11.375, -11.346, -11.317, -11.287, -11.258, -11.229, -11.199, -11.170, -11.141, -11.111,
	-11.082, -11.053, -11.023, -10.994, -10.965, -10.935, -10.906, -10.877, -10.847, -10.818, -10.788, -10.759, -10.730, -10.700,
	-10.671, -10.641, -10.612, -10.583, -10.553, -10.524, -10.494, -10.465, -10.435, -10.406, -10.376, -10.347, -10.317, -10.288,
	-10.258, -10.229, -10.199, -10.170, -10.140, -10.111, -10.081, -10.052, -10.022, -09.993, -09.963, -09.934, -09.904, -09.874,
	-09.845, -09.815, -09.786, -09.756, -09.726, -09.697, -09.667, -09.637, -09.608, -09.578, -09.548, -09.519, -09.489, -09.459,
	-09.430, -09.400, -09.370, -09.340, -09.311, -09.281, -09.251, -09.221, -09.191, -09.162, -09.132, -09.102, -09.072, -09.042,
	-09.013, -08.983, -08.953, -08.923, -08.893, -08.863, -08.833, -08.803, -08.773, -08.743, -08.713, -08.683, -08.653, -08.623,
	-08.593, -08.563, -08.533, -08.503, -08.473, -08.443, -08.413, -08.383, -08.353, -08.323, -08.293, -08.263, -08.232, -08.202,
	-08.172, -08.142, -08.112, -08.082, -08.051, -08.021, -07.991, -07.961, -07.930, -07.900, -07.870, -07.839, -07.809, -07.779,
	-07.748, -07.718, -07.688, -07.657, -07.627, -07.596, -07.566, -07.535, -07.505, -07.474, -07.444, -07.413, -07.383, -07.352,
	-07.322, -07.291, -07.261, -07.230, -07.199, -07.169, -07.138, -07.107, -07.077, -07.046, -07.015, -06.985, -06.954, -06.923,
	-06.892, -06.862, -06.831, -06.800, -06.769, -06.738, -06.707, -06.676, -06.646, -06.615, -06.584, -06.553, -06.522, -06.491,
	-06.460, -06.429, -06.398, -06.367, -06.336, -06.304, -06.273, -06.242, -06.211, -06.180, -06.149, -06.118, -06.086, -06.055,
	-06.024, -05.993, -05.961, -05.930, -05.899, -05.867, -05.836, -05.805, -05.773, -05.742, -05.710, -05.679, -05.647, -05.616,
	-05.584, -05.553, -05.521, -05.490, -05.458, -05.426, -05.395, -05.363, -05.331, -05.300, -05.268, -05.236, -05.204, -05.173,
	-05.141, -05.109, -05.077, -05.045, -05.013, -04.982, -04.950, -04.918, -04.886, -04.854, -04.822, -04.790, -04.758, -04.726,
	-04.693, -04.661, -04.629, -04.597, -04.565, -04.533, -04.500, -04.468, -04.436, -04.404, -04.371, -04.339, -04.307, -04.274,
	-04.242, -04.209, -04.177, -04.144, -04.112, -04.079, -04.047, -04.014, -03.982, -03.949, -03.916, -03.884, -03.851, -03.818,
	-03.785, -03.753, -03.720, -03.687, -03.654, -03.621, -03.588, -03.556, -03.523, -03.490, -03.457, -03.424, -03.391, -03.357,
	-03.324, -03.291, -03.258, -03.225, -03.192, -03.158, -03.125, -03.092, -03.058, -03.025, -02.992, -02.958, -02.925, -02.891,
	-02.858, -02.824, -02.791, -02.757, -02.723, -02.690, -02.656, -02.622, -02.589, -02.555, -02.521, -02.487, -02.453, -02.419,
	-02.385, -02.351, -02.317, -02.283, -02.249, -02.215, -02.181, -02.147, -02.112, -02.078, -02.044, -02.009, -01.975, -01.941,
	-01.906, -01.872, -01.837, -01.803, -01.768, -01.733, -01.699, -01.664, -01.629, -01.594, -01.559, -01.525, -01.490, -01.455,
	-01.420, -01.385, -01.350, -01.315, -01.279, -01.244, -01.209, -01.174, -01.138, -01.103, -01.068, -01.032, -00.997, -00.961,
	-00.926, -00.890, -00.854, -00.819, -00.783, -00.747, -00.711, -00.675, -00.639, -00.603, -00.567, -00.531, -00.495, -00.459,
	-00.423, -00.387, -00.350, -00.314, -00.278, -00.241, -00.205, -00.168, -00.132, -00.095, -00.058, -00.022,  00.015,  00.052,
	 00.089,  00.126,  00.163,  00.200,  00.237,  00.274,  00.311,  00.348,  00.385,  00.423,  00.460,  00.497,  00.535,  00.572,
	 00.610,  00.647,  00.685,  00.723,  00.761,  00.798,  00.836,  00.874,  00.912,  00.950,  00.988,  01.026,  01.065,  01.103,
	 01.141,  01.180,  01.218,  01.257,  01.295,  01.334,  01.372,  01.411,  01.450,  01.489,  01.527,  01.566,  01.605,  01.644,
	 01.683,  01.723,  01.762,  01.801,  01.840,  01.880,  01.919,  01.959,  01.998,  02.038,  02.078,  02.117,  02.157,  02.197,
	 02.237,  02.277,  02.317,  02.357,  02.397,  02.437,  02.478,  02.518,  02.558,  02.598,  02.639,  02.679,  02.720,  02.760,
	 02.800,  02.841,  02.881,  02.922,  02.962,  03.003,  03.043,  03.084,  03.124,  03.165,  03.205,  03.246,  03.286,  03.326,
	 03.367,  03.407,  03.448,  03.488,  03.528,  03.568,  03.609,  03.649,  03.689,  03.729,  03.769,  03.809,  03.849,  03.889,
	 03.929,  03.968,  04.008,  04.048,  04.087,  04.127,  04.166,  04.205,  04.245,  04.284,  04.323,  04.362,  04.401,  04.440,
	 04.478,  04.517,  04.555,  04.594,  04.632,  04.670,  04.708,  04.746,  04.784,  04.822,  04.859,  04.896,  04.934,  04.971,
	 05.008,  05.045,  05.082,  05.118,  05.155,  05.191,  05.227,  05.263,  05.299,  05.334,  05.370,  05.405,  05.440,  05.475,
	 05.510,  05.545,  05.579,  05.613,  05.647,  05.681,  05.715,  05.748,  05.782,  05.815,  05.848,  05.880,  05.913,  05.945,
	 05.977,  06.009,  06.040,  06.072,  06.103,  06.134,  06.165,  06.195,	 06.226,  06.256,  06.286,  06.315,  06.345,  06.374,
	 06.404,  06.433,  06.461,  06.490,  06.519,  06.547,  06.575,  06.603,  06.631,  06.659,  06.686,  06.713,  06.741,  06.768,
	 06.794,  06.821,  06.848,  06.874,  06.900,  06.927,  06.953,  06.979,  07.004,  07.030,  07.055,  07.081,  07.106,  07.131,
	 07.156,  07.181,  07.206,  07.231,  07.255,  07.280,  07.304,  07.329,  07.353,  07.377,  07.401,  07.425,  07.449,  07.472,
	 07.496,  07.520,  07.543,  07.567,  07.590,  07.613,  07.637,  07.660,  07.683,  07.706,  07.729,  07.752,  07.775,  07.798,
	 07.820,  07.843,  07.866,  07.889,  07.911,  07.934,  07.956,  07.979,  08.002,  08.024,  08.047,  08.069,  08.092,  08.114,
	 08.136,  08.159,  08.181,  08.204,  08.226,  08.248,  08.271,  08.293,  08.316,  08.338,  08.361,  08.383,  08.406,  08.428,
	 08.451,  08.473,  08.496,  08.518,  08.541,  08.564,  08.587,  08.609,  08.632,  08.655,  08.678,  08.700,  08.723,  08.746,
	 08.769,  08.792,  08.815,  08.838,  08.861,  08.884,  08.907,  08.930,  08.953,  08.976,  09.000,  09.023,  09.046,  09.069,
	 09.092,  09.115,  09.139,  09.162,  09.185,  09.208,  09.231,  09.255,  09.278,  09.301,  09.325,  09.348,  09.371,  09.394,
	 09.418,  09.441,  09.464,  09.488,  09.511,  09.534,  09.558,  09.581,  09.604,  09.628,  09.651,  09.674,  09.698,  09.721,
	 09.744,  09.768,  09.791,  09.814,  09.837,  09.861,  09.884,  09.907,  09.930,  09.954,  09.977,  10.000,  10.023,  10.047,
	 10.070,  10.093,  10.116,  10.139,  10.162,  10.185,  10.209,  10.232,  10.255,  10.278,  10.301,  10.324,  10.347,  10.370,
	 10.393,  10.415,  10.438,  10.461,  10.484,  10.507,  10.530,  10.552,  10.575,  10.598,  10.621,  10.643,  10.666,  10.688,
	 10.711,  10.734,  10.756,  10.779,  10.801,  10.824,  10.846,  10.868,  10.891,  10.913,  10.935,  10.958,  10.980,  11.002,
	 11.024,  11.047,  11.069,  11.091,  11.113,  11.135,  11.157,  11.179,	 11.201,  11.223,  11.245,  11.266,  11.288,  11.310,
	 11.332,  11.353,  11.375,  11.397,  11.418,  11.440,  11.462,  11.483,  11.504,  11.526,  11.547,  11.569,  11.590,  11.611,
	 11.632,  11.654,  11.675,  11.696,  11.717,  11.738,  11.759,  11.780,  11.801,  11.822,  11.843,  11.863,  11.884,  11.905,
	 11.926,  11.946,  11.967,  11.987,  12.008,  12.028,  12.049,  12.069,  12.089,  12.110,  12.130,  12.150,  12.170,  12.190,
	 12.210,  12.230,  12.250,  12.270,  12.290,  12.310,  12.330,  12.349,  12.369,  12.389,  12.408,  12.428,  12.447,  12.467,
	 12.486,  12.505,  12.525,  12.544,  12.563,  12.582,  12.601,  12.620,  12.639,  12.658,  12.677,  12.696,  12.714,  12.733,
	 12.752,  12.770,  12.789,  12.807,  12.826,  12.844,  12.862,  12.881,  12.899,  12.917,  12.935,  12.953,  12.971,  12.989,
	 13.007,  13.025,  13.042,  13.060,  13.078,  13.095,  13.113,  13.130,  13.148,  13.165,  13.182,  13.199,  13.216,  13.234,
	 13.251,  13.267,  13.284,  13.301,  13.318,  13.335,  13.351,  13.368,  13.384,  13.401,  13.417,  13.433,  13.450,  13.466,
	 13.482,  13.498,  13.514,  13.530,  13.546,  13.561,  13.577,  13.593,  13.608,  13.624,  13.639,  13.654,  13.670,  13.685,
	 13.700,  13.715,  13.730,  13.745,  13.760,  13.775,  13.789,  13.804,  13.818,  13.833,  13.847,  13.861,  13.876,  13.890,
	 13.904,  13.918,  13.932,  13.946,  13.959,  13.973,  13.986,  14.000,  14.013,  14.027,  14.040,  14.053,  14.066,  14.079,
	 14.092,  14.105,  14.117,  14.130,  14.142,  14.155,  14.167,  14.179,  14.192,  14.204,  14.216,  14.227,  14.239,  14.251,
	 14.262,  14.274,  14.285,  14.296,  14.308,  14.319,  14.330,  14.341,  14.351,  14.362,  14.373,  14.383,  14.394,  14.404,
	 14.414,  14.424,  14.434,  14.444,  14.454,  14.463,  14.473,  14.482,  14.492,  14.501,  14.510,  14.519,  14.528,  14.537,
	 14.545,  14.554,  14.562,  14.571,  14.579,  14.587,  14.595,  14.603,	 14.610,  14.618,  14.626,  14.633,  14.640,  14.647,
	 14.654,  14.661,  14.668,  14.675,  14.681,  14.688,  14.694,  14.700,  14.706,  14.712,  14.718,  14.724,  14.730,  14.735,
	 14.740,  14.745,  14.751,  14.755,  14.760,  14.765,  14.770,  14.774,  14.778,  14.782,  14.786,  14.790,  14.794,  14.798,
	 14.801,  14.805,  14.808,  14.811,  14.814,  14.817,  14.819,  14.822,  14.824,  14.826,  14.829,  14.831,  14.832,  14.834,
	 14.836,  14.837,  14.838,  14.839,  14.840,  14.841,  14.842,  14.842,  14.843,  14.843,  14.843,  14.843,  14.843,  14.843,
	 14.842,  14.841,  14.841,  14.840,  14.839,  14.837,  14.836,  14.834,  14.833,  14.831,  14.829,  14.827,  14.824,  14.822,
	 14.819,  14.817,  14.814,  14.811,  14.807,  14.804,  14.800,  14.797,  14.793,  14.789,  14.785,  14.780,  14.776,  14.771,
	 14.766,  14.761,  14.756,  14.751,  14.746,  14.740,  14.734,  14.728,  14.722,  14.716,  14.709,  14.703,  14.696,  14.689,
	 14.682,  14.675,  14.667,  14.660,  14.652,  14.644,  14.636,  14.628,  14.619,  14.611,  14.602,  14.593,  14.584,  14.575,
	 14.565,  14.555,  14.546,  14.536,  14.526,  14.515,  14.505,  14.494,  14.483,  14.472,  14.461,  14.450,  14.438,  14.427,
	 14.415,  14.403,  14.391,  14.379,  14.366,  14.354,  14.341,  14.329,  14.316,  14.303,  14.289,  14.276,  14.263,  14.249,
	 14.235,  14.222,  14.208,  14.194,  14.179,  14.165,  14.151,  14.136,  14.122,  14.107,  14.092,  14.077,  14.062,  14.047,
	 14.032,  14.017,  14.001,  13.986,  13.970,  13.955,  13.939,  13.923,  13.908,  13.892,  13.876,  13.860,  13.843,  13.827,
	 13.811,  13.795,  13.778,  13.762,  13.745,  13.729,  13.712,  13.695,  13.679,  13.662,  13.645,  13.628,  13.612,  13.595,
	 13.578,  13.561,  13.544,  13.527,  13.510,  13.493,  13.476,  13.459,  13.441,  13.424,  13.407,  13.390,  13.373,  13.356,
	 13.338,  13.321,  13.304,  13.287,  13.270,  13.253,  13.236,  13.218,	 13.201,  13.184,  13.167,  13.150,  13.133,  13.116,
	 13.099,  13.082,  13.065,  13.048,  13.031,  13.014,  12.998,  12.981,  12.964,  12.947,  12.931,  12.914,  12.897,  12.881,
	 12.864,  12.847,  12.831,  12.814,  12.798,  12.781,  12.765,  12.748,  12.732,  12.716,  12.699,  12.683,  12.667,  12.650,
	 12.634,  12.618,  12.602,  12.585,  12.569,  12.553,  12.537,  12.521,  12.504,  12.488,  12.472,  12.456,  12.440,  12.424,
	 12.408,  12.392,  12.376,  12.360,  12.344,  12.328,  12.312,  12.296,  12.280,  12.265,  12.249,  12.233,  12.217,  12.201,
	 12.185,  12.169,  12.154,  12.138,  12.122,  12.106,  12.090,  12.075,  12.059,  12.043,  12.027,  12.012,  11.996,  11.980,
	 11.965,  11.949,  11.933,  11.917,  11.902,  11.886,  11.870,  11.855,  11.839,  11.823,  11.808,  11.792,  11.776,  11.761,
	 11.745,  11.730,  11.714,  11.698,  11.683,  11.667,  11.652,  11.636,  11.621,  11.606,  11.590,  11.575,  11.560,  11.545,
	 11.530,  11.515,  11.500,  11.485,  11.470,  11.455,  11.441,  11.426,  11.412,  11.397,  11.383,  11.369,  11.355,  11.341,
	 11.327,  11.314,  11.300,  11.286,  11.273,  11.260,  11.247,  11.234,  11.221,  11.208,  11.196,  11.183,  11.171,  11.158,
	 11.146,  11.134,  11.122,  11.109,  11.097,  11.085,  11.074,  11.062,  11.050,  11.038,  11.026,  11.014,  11.003,  10.991,
	 10.979,  10.968,  10.956,  10.944,  10.932,  10.921,  10.909,  10.897,  10.885,  10.873,  10.861,  10.849,  10.837,  10.825,
	 10.813,  10.801,  10.789,  10.776,  10.764,  10.751,  10.738,  10.726,  10.713,  10.700,  10.687,  10.674,  10.660,  10.647,
	 10.633,  10.619,  10.605,  10.591,  10.577,  10.563,  10.548,  10.534,  10.519,  10.504,  10.488,  10.473,  10.457,  10.441,
	 10.425,  10.409,  10.392,  10.376,  10.359,  10.342,  10.324,  10.306,  10.288,  10.270,  10.252,  10.233,  10.214,  10.195,
	 10.175,  10.155,  10.135,  10.115,  10.094,  10.073,  10.052,  10.030,	 10.008,  09.985,  09.963,  09.940,  09.917,  09.893,
	 09.869,  09.845,  09.820,  09.795,  09.770,  09.745,  09.719,  09.693,  09.667,  09.641,  09.614,  09.587,  09.559,  09.532,
	 09.504,  09.476,  09.447,  09.419,  09.390,  09.361,  09.331,  09.301,  09.272,  09.242,  09.211,  09.181,  09.150,  09.119,
	 09.088,  09.056,  09.025,  08.993,  08.961,  08.928,  08.896,  08.863,  08.831,  08.798,  08.764,  08.731,  08.697,  08.664,
	 08.630,  08.596,  08.561,  08.527,  08.492,  08.458,  08.423,  08.388,  08.353,  08.317,  08.282,  08.246,  08.211,  08.175,
	 08.139,  08.103,  08.067,  08.030,  07.994,  07.957,  07.921,  07.884,  07.847,  07.810,  07.773,  07.736,  07.699,  07.662,
	 07.624,  07.587,  07.549,  07.512,  07.474,  07.437,  07.399,  07.361,  07.323,  07.285,  07.247,  07.209,  07.171,  07.133,
	 07.095,  07.057,  07.019,  06.980,  06.942,  06.904,  06.866,  06.827,  06.789,  06.751,  06.713,  06.674,  06.636,  06.598,
	 06.560,  06.522,  06.484,  06.446,  06.409,  06.371,  06.333,  06.296,  06.259,  06.221,  06.184,  06.148,  06.111,  06.074,
	 06.038,  06.002,  05.966,  05.931,  05.895,  05.860,  05.825,  05.790,  05.756,  05.722,  05.688,  05.654,  05.621,  05.588,
	 05.556,  05.523,  05.491,  05.460,  05.429,  05.398,  05.367,  05.337,  05.308,  05.279,  05.250,  05.221,  05.194,  05.166,
	 05.139,  05.113,  05.087,  05.061,  05.036,  05.011,  04.987,  04.964,  04.941,  04.919,  04.897,  04.875,  04.855,  04.835,
	 04.815,  04.796,  04.777,  04.759,  04.741,  04.724,  04.708,  04.691,  04.676,  04.661,  04.646,  04.631,  04.617,  04.604,
	 04.591,  04.578,  04.566,  04.554,  04.542,  04.531,  04.520,  04.510,  04.500,  04.490,  04.480,  04.471,  04.462,  04.454,
	 04.446,  04.438,  04.430,  04.422,  04.415,  04.408,  04.402,  04.395,  04.389,  04.383,  04.377,  04.371,  04.366,  04.361,
	 04.355,  04.350,  04.346,  04.341,  04.336,  04.332,  04.328,  04.323,	 04.319,  04.315,  04.311,  04.308,  04.304,  04.300,
	 04.296,  04.293,  04.289,  04.285,  04.282,  04.278,  04.275,  04.271,  04.268,  04.264,  04.261,  04.257,  04.253,  04.249,
	 04.246,  04.242,  04.238,  04.234,  04.230,  04.226,  04.221,  04.217,  04.212,  04.208,  04.203,  04.198,  04.193,  04.188,
	 04.183,  04.177,  04.171,  04.165,  04.159,  04.153,  04.147,  04.140,  04.133,  04.126,  04.118,  04.111,  04.103,  04.095,
	 04.086,  04.078,  04.069,  04.059,  04.050,  04.040,  04.030,  04.019,  04.009,  03.997,  03.986,  03.974,  03.962,  03.949,
	 03.937,  03.923,  03.910,  03.896,  03.882,  03.867,  03.853,  03.838,  03.823,  03.807,  03.791,  03.776,  03.760,  03.743,
	 03.727,  03.710,  03.693,  03.676,  03.659,  03.642,  03.625,  03.607,  03.590,  03.572,  03.554,  03.537,  03.519,  03.501,
	 03.483,  03.465,  03.447,  03.429,  03.412,  03.394,  03.376,  03.358,  03.340,  03.323,  03.305,  03.287,  03.270,  03.252,
	 03.235,  03.217,  03.200,  03.182,  03.164,  03.147,  03.129,  03.111,  03.094,  03.076,  03.058,  03.040,  03.022,  03.004,
	 02.985,  02.967,  02.949,  02.930,  02.911,  02.893,  02.874,  02.855,  02.835,  02.816,  02.796,  02.777,  02.757,  02.737,
	 02.716,  02.696,  02.675,  02.654,  02.633,  02.612,  02.590,  02.568,  02.546,  02.524,  02.501,  02.478,  02.455,  02.431,
	 02.407,  02.383,  02.359,  02.334,  02.309,  02.284,  02.258,  02.233,  02.206,  02.180,  02.153,  02.126,  02.099,  02.072,
	 02.044,  02.016,  01.988,  01.960,  01.931,  01.902,  01.873,  01.844,  01.814,  01.785,  01.755,  01.725,  01.694,  01.664,
	 01.633,  01.602,  01.571,  01.540,  01.509,  01.477,  01.445,  01.413,  01.381,  01.349,  01.317,  01.284,  01.252,  01.219,
	 01.186,  01.153,  01.120,  01.087,  01.054,  01.020,  00.987,  00.953,  00.920,  00.886,  00.852,  00.818,  00.784,  00.750,
	 00.715,  00.680,  00.645,  00.609,  00.574,  00.537,  00.500,  00.463,	 00.426,  00.387,  00.348,  00.309,  00.269,  00.228,
	 00.187,  00.145,  00.102,  00.058,  00.013, -00.032, -00.079, -00.126,	-00.174, -00.223, -00.274, -00.325, -00.378, -00.431,
	-00.486, -00.542, -00.599, -00.658, -00.718, -00.779, -00.841, -00.905,	-00.970, -01.037, -01.105, -01.175, -01.247, -01.320,
	-01.394, -01.471, -01.549, -01.628, -01.710, -01.793, -01.877, -01.964,	-02.051, -02.140, -02.230, -02.322, -02.415, -02.508,
	-02.603, -02.699, -02.796, -02.893, -02.992, -03.091, -03.191, -03.291,	-03.392, -03.494, -03.596, -03.698, -03.801, -03.903,
	-04.006, -04.109, -04.212, -04.315, -04.418, -04.521, -04.624, -04.726,	-04.828, -04.929, -05.030, -05.131, -05.231, -05.330,
	-05.428, -05.526, -05.623, -05.719, -05.813, -05.907, -06.000, -06.091,	-06.182, -06.271, -06.358, -06.444, -06.529, -06.612,
	-06.694, -06.775, -06.854, -06.932, -07.009, -07.084, -07.158, -07.231,	-07.302, -07.373, -07.442, -07.510, -07.578, -07.644,
	-07.708, -07.772, -07.835, -07.897, -07.958, -08.018, -08.077, -08.135,	-08.193, -08.249, -08.305, -08.360, -08.414, -08.467,
	-08.520, -08.572, -08.623, -08.674, -08.724, -08.773, -08.822, -08.870,	-08.918, -08.965, -09.011, -09.058, -09.103, -09.149,
	-09.194, -09.238, -09.282, -09.326, -09.370, -09.413, -09.456, -09.499,	-09.542, -09.584, -09.626, -09.668, -09.709, -09.751,
	-09.792, -09.833, -09.873, -09.914, -09.954, -09.994, -10.034, -10.073,	-10.113, -10.152, -10.191, -10.230, -10.269, -10.307,
	-10.346, -10.384, -10.422, -10.460, -10.498, -10.535, -10.573, -10.610,	-10.648, -10.685, -10.722, -10.759, -10.796, -10.832,
	-10.869, -10.905, -10.942, -10.978, -11.014, -11.051, -11.087, -11.123,	-11.159, -11.195, -11.231, -11.267, -11.302, -11.338,
	-11.374, -11.410, -11.445, -11.481, -11.517, -11.552, -11.588, -11.623,	-11.659, -11.694, -11.730, -11.765, -11.800, -11.836,
	-11.871, -11.906, -11.941, -11.976, -12.011, -12.047, -12.082, -12.117,	-12.151, -12.186, -12.221, -12.256, -12.291, -12.325,
	-12.360, -12.395, -12.429, -12.464, -12.498, -12.533, -12.567, -12.601,	-12.636, -12.670, -12.704, -12.738, -12.772, -12.807,
	-12.841, -12.875, -12.908, -12.942, -12.976, -13.010, -13.044, -13.077,	-13.111, -13.144, -13.178, -13.211, -13.245, -13.278,
	-13.311, -13.344, -13.378, -13.411, -13.444, -13.476, -13.509, -13.542,	-13.575, -13.607, -13.640, -13.673, -13.705, -13.737,
	-13.770, -13.802, -13.834, -13.866, -13.898, -13.930, -13.961, -13.993,	-14.024, -14.056, -14.087, -14.119, -14.150, -14.181,
	-14.212, -14.243, -14.273, -14.304, -14.335, -14.365, -14.395, -14.426,	-14.456, -14.486, -14.516, -14.546, -14.576, -14.605,
	-14.635, -14.665, -14.694, -14.724, -14.754, -14.783, -14.813, -14.842,	-14.871, -14.901, -14.930, -14.960, -14.989, -15.018,
	-15.048, -15.077, -15.107, -15.136, -15.166, -15.195, -15.225, -15.255,	-15.285, -15.314, -15.344, -15.373, -15.403, -15.432,
	-15.461, -15.490, -15.519, -15.547, -15.575, -15.603, -15.630, -15.657,	-15.684, -15.710, -15.735, -15.760, -15.784, -15.808,
	-15.831, -15.853, -15.875, -15.896, -15.916, -15.935, -15.954, -15.971,	-15.988, -16.003, -16.018, -16.031, -16.044, -16.055,
	-16.065, -16.074, -16.082, -16.089, -16.094, -16.098, -16.101, -16.102,	-16.102, -16.100, -16.097, -16.092, -16.086, -16.078,
	-16.068, -16.057, -16.044, -16.029, -16.013, -15.995, -15.974, -15.952,	-15.928, -15.902, -15.874, -15.844, -15.812, -15.778,
	-15.742, -15.705, -15.666, -15.626, -15.584, -15.541, -15.496, -15.451,	-15.404, -15.357, -15.309, -15.260, -15.211, -15.161,
	-15.110, -15.059, -15.008, -14.957, -14.906, -14.855, -14.804, -14.753,	-14.703, -14.653, -14.603, -14.554, -14.506, -14.459,
	-14.412, -14.367, -14.322, -14.278, -14.235, -14.192, -14.151, -14.110,	-14.070, -14.031, -13.993, -13.955, -13.918, -13.882,
	-13.847, -13.813, -13.779, -13.746, -13.714, -13.682, -13.652, -13.622,	-13.593, -13.564, -13.536, -13.509, -13.483, -13.458,
	-13.433, -13.409, -13.385, -13.363, -13.340, -13.319, -13.297, -13.276,	-13.256, -13.236, -13.216, -13.196, -13.176, -13.156,
	-13.137, -13.117, -13.097, -13.078, -13.058, -13.037, -13.017, -12.996,	-12.974, -12.953, -12.930, -12.907, -12.884, -12.860,
	-12.835, -12.809, -12.783, -12.755, -12.727, -12.698, -12.668, -12.637,	-12.606, -12.573, -12.540, -12.506, -12.471, -12.435,
	-12.399, -12.362, -12.324, -12.285, -12.246, -12.205, -12.165, -12.123,	-12.081, -12.038, -11.995, -11.951, -11.906, -11.860,
	-11.814, -11.768, -11.721, -11.673, -11.625, -11.576, -11.526, -11.477,	-11.427, -11.377, -11.326, -11.276, -11.226, -11.176,
	-11.127, -11.078, -11.029, -10.981, -10.934, -10.887, -10.842, -10.797,	-10.754, -10.711, -10.670, -10.631, -10.592, -10.556,
	-10.521, -10.488, -10.456, -10.427, -10.400, -10.375, -10.352, -10.331,	-10.313, -10.297, -10.283, -10.270, -10.260, -10.251,
	-10.243, -10.237, -10.232, -10.229, -10.226, -10.224, -10.223, -10.222,	-10.222, -10.222, -10.223, -10.223, -10.224, -10.224,
	-10.225, -10.224, -10.223, -10.222, -10.220, -10.217, -10.212, -10.207,	-10.201, -10.193, -10.183, -10.173, -10.162, -10.150,
	-10.138, -10.125, -10.112, -10.099, -10.087, -10.075, -10.063, -10.053,	-10.043, -10.035, -10.028, -10.022, -10.019, -10.017,
	-10.017, -10.020, -10.025, -10.033, -10.043, -10.057, -10.074, -10.094,	-10.118, -10.146, -10.178, -10.214, -10.254, -10.297,
	-10.344, -10.394, -10.447, -10.502, -10.560, -10.619, -10.680, -10.743,	-10.806, -10.871, -10.936, -11.001, -11.066, -11.131,
	-11.195, -11.259, -11.321, -11.382, -11.441, -11.498, -11.553, -11.605,	-11.655, -11.701, -11.744, -11.783, -11.818, -11.850,
	-11.877, -11.900, -11.921, -11.938, -11.953, -11.966, -11.978, -11.988,	-11.997, -12.005, -12.013, -12.021, -12.029, -12.038,
	-12.049, -12.061, -12.074, -12.091, -12.109, -12.131, -12.155, -12.182,	-12.212, -12.244, -12.278, -12.314, -12.352, -12.392,
	-12.434, -12.476, -12.520, -12.565, -12.612, -12.658, -12.706, -12.754,	-12.802, -12.850, -12.898, -12.947, -12.995, -13.042,
	-13.090, -13.137, -13.185, -13.232, -13.278, -13.325, -13.372, -13.418,	-13.464, -13.510, -13.556, -13.602, -13.648, -13.693,
	-13.739, -13.784, -13.829, -13.874, -13.919, -13.964, -14.009, -14.054,	-14.099, -14.145, -14.192, -14.238, -14.286, -14.334,
	-14.383, -14.432, -14.483, -14.534, -14.587, -14.640, -14.695, -14.751,	-14.809, -14.868, -14.928, -14.990, -15.052, -15.115,
	-15.178, -15.241, -15.304, -15.367, -15.430, -15.491, -15.552, -15.612,	-15.670, -15.727, -15.782, -15.835, -15.886, -15.934,
	-15.980, -16.022, -16.063, -16.100, -16.135, -16.168, -16.198, -16.227,	-16.254, -16.279, -16.302, -16.324, -16.344, -16.364,
	-16.382, -16.400, -16.417, -16.433, -16.449, -16.465, -16.480
};

double getMoonFluctuation(const double jDay)
{
	double f = 0.;
	int year, month, day, index;
	getDateFromJulianDay(jDay, &year, &month, &day);

	double t = yearFraction(year, month, day);
	if (t>=1681.0 && t<=1936.5) {
		index = qRound(std::floor((t - 1681.0)*10));
		f = MoonFluctuationTable[index]*0.07; // Get interpolated data and convert to seconds of time
	}

	return f;
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
float* ComputeCosSinTheta(const unsigned int slices)
{
	Q_ASSERT(slices<=MAX_SLICES);
	
	// Difference angle between the stops. Always use 2*M_PI/slices!
	const float dTheta = 2.f * static_cast<float>(M_PI) / slices;
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
float* ComputeCosSinRho(const unsigned int segments)
{
	Q_ASSERT(segments<=MAX_STACKS);
	
	// Difference angle between the stops. Always use M_PI/segments!
	const float dRho = static_cast<float>(M_PI) / segments;
	float *cos_sin = cos_sin_rho;
	float *cos_sin_rev = cos_sin + 2*(segments+1);
	const float c = cosf(dRho);
	const float s = sinf(dRho);
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
float *ComputeCosSinRhoZone(const float dRho, const unsigned int segments, const float minAngle)
{
	float *cos_sin = cos_sin_rho;
	const float c = cosf(dRho);
	const float s = sinf(dRho);
	*cos_sin++ = cosf(minAngle);
	*cos_sin++ = sinf(minAngle);
	for (unsigned int i=0; i<segments; ++i) // we cannot mirror this, it may be unequal.
	{   // efficient computation, avoid expensive trig functions by use of the addition theorem.
		cos_sin[0] = cos_sin[-2]*c - cos_sin[-1]*s;
		cos_sin[1] = cos_sin[-2]*s + cos_sin[-1]*c;
		cos_sin += 2;
	}
	return cos_sin_rho;
}

int getFixedFromGregorian(const int year, const int month, const int day)
{
	int y = year - 1;
	int r = 365*y + intFloorDiv(y, 4) - intFloorDiv(y, 100) + intFloorDiv(y, 400) + (367*month-362)/12 + day;
	if (month>2)
		r += (isLeapYear(year) ? -1 : -2);

	return r;
}

int compareVersions(const QString v1, const QString v2)
{
	// result (-1: v1<v2; 0: v1==v2; 1: v1>v2)
	int ver1, ver2, result = 0;
	QStringList v1s = v1.split(".");	
	QStringList v2s = v2.split(".");	
	if (v1s.count()==3) // Full format: X.Y.Z
		ver1 = v1s.at(0).toInt()*1000 + v1s.at(1).toInt()*100 + v1s.at(2).toInt();
	else // Short format: X.Y
		ver1 = v1s.at(0).toInt()*1000 + v1s.at(1).toInt()*100;
	if (v2s.count()==3)
		ver2 = v2s.at(0).toInt()*1000 + v2s.at(1).toInt()*100 + v2s.at(2).toInt();
	else
		ver2 = v2s.at(0).toInt()*1000 + v2s.at(1).toInt()*100;

	if (ver1<ver2)
		result = -1;
	else if (ver1 == ver2)
		result = 0;
	else if (ver1 > ver2)
		result = 1;

	return result;
}

int gcd(int a, int b)
{
	return b ? gcd(b, a % b) : a;
}

int lcm(int a, int b)
{
	return (a*b/gcd(a, b));
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
	strm.zalloc = Q_NULLPTR;
	strm.zfree = Q_NULLPTR;
	strm.opaque = Q_NULLPTR;
	strm.avail_in = Z_NULL;
	strm.next_in = Q_NULLPTR;

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
			bytesToRead = qMin(static_cast<qint64>(CHUNK),maxBytes-bytesRead);
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
		strm.avail_in = static_cast<unsigned int>(read);

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

			out.append(inflateBuffer.constData(), CHUNK - static_cast<int>(strm.avail_out));
		} while(strm.avail_out == 0); //if zlib has more data for us, repeat
	} while(ret!=Z_STREAM_END);

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


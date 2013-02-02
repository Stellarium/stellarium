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

#include <cmath> // std::fmod

#ifdef CYGWIN
 #include <malloc.h>
#endif

#include "StelUtils.hpp"
#include "VecMath.hpp"
#include <QString>
#include <QStringList>
#include <QTextStream>
#include <QFile>
#include <QDebug>
#include <QLocale>
#include <QRegExp>
#include <QtGlobal>

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
#elif SVN_REVISION
	return QString(PACKAGE_VERSION)+" (SVN r"+SVN_REVISION+")";
#else
	return QString(PACKAGE_VERSION);
#endif
}

double hmsToRad(unsigned int h, unsigned int m, double s )
{
	//return (double)M_PI/24.*h*2.+(double)M_PI/12.*m/60.+s*M_PI/43200.; // Wrong formula! --AW
	return (double)h*M_PI/12.+(double)m*M_PI/10800.+(double)s*M_PI/648000.;
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
		if (sign)
			m += 1;
		else
			m -= 1;
	}
	if (m==60)
	{
		m = 0.;
		if (sign)
			d += 1;
		else
			d -= 1;
	}
}

/*************************************************************************
 Convert an angle in radian to a hms formatted string
 If the minute and second part are null are too small, don't print them
*************************************************************************/
QString radToHmsStrAdapt(double angle)
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
QString radToHmsStr(double angle, bool decimal)
{
	unsigned int h,m;
	double s;
	StelUtils::radToHms(angle+0.005*M_PI/12/(60*60), h, m, s);
	int width, precision;
	QString carry;
	if (decimal)
	{
		width=4;
		precision=1;
		carry="60.0";
	}
	else
	{
		width=2;
		precision=0;
		carry="60";
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
	//qDebug() << "radToDmsStrAdapt(" << angle << ", " << useD << ") = " << str;
	return str;
}


/*************************************************************************
 Convert an angle in radian to a dms formatted string
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

void spheToRect(double lng, double lat, Vec3d& v)
{
	const double cosLat = cos(lat);
	v.set(cos(lng) * cosLat, sin(lng) * cosLat, sin(lat));
}

void spheToRect(float lng, float lat, Vec3f& v)
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

// GZ: some additions. I need those just for quick conversions for text display.
void ctRadec2Ecl(const double raRad, const double decRad, const double eclRad, double *lambdaRad, double *betaRad)
{
  *lambdaRad=std::atan2(std::sin(raRad)*std::cos(eclRad)+std::tan(decRad)*std::sin(eclRad), std::cos(raRad));
  *betaRad=std::asin(std::sin(decRad)*std::cos(eclRad)-std::cos(decRad)*std::sin(eclRad)*std::sin(raRad));
}
// GZ: done

double getDecAngle(const QString& str)
{
	QRegExp re1("^\\s*([\\+\\-])?\\s*(\\d+)\\s*([hHDd\xBA])\\s*(\\d+)\\s*['Mm]\\s*(\\d+(\\.\\d+)?)\\s*[\"Ss]\\s*([NSEWnsew])?\\s*$"); // DMS/HMS
	QRegExp re2("^\\s*([\\+\\-])?\\s*(\\d+(\\.\\d+)?).?([NSEWnsew])?\\s*$"); // Decimal

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

	qDebug() << "getDecAngle failed to parse angle string:" << str;
	return -0.0;
}

// Check if a number is a power of 2
bool isPowerOfTwo(int value)
{
	return (value & -value) == value;
}

// Return the first power of two greater or equal to the given value
int smallestPowerOfTwoGreaterOrEqualTo(int value)
{
#ifndef NDEBUG
	const int twoTo30 = 1073741824;
	Q_ASSERT_X(value <= twoTo30, Q_FUNC_INFO,
	           "Value too large - smallest greater/equal power-of-2 is out of range");
#endif

	if(value == 0){return 0;}

	int pot=1;
	while (pot<value){pot<<=1;}
	return pot;
}

QSize smallestPowerOfTwoSizeGreaterOrEqualTo(const QSize base)
{
	return QSize(smallestPowerOfTwoGreaterOrEqualTo(base.width()), 
	             smallestPowerOfTwoGreaterOrEqualTo(base.height()));
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
	QDateTime result = QDateTime::fromString(QString("%1.%2.%3").arg(year, 4, 10, QLatin1Char('0')).arg(month).arg(day), "yyyy.M.d");
	result.setTime(jdFractionToQTime(jd));
	return result;
}


void getDateFromJulianDay(double jd, int *yy, int *mm, int *dd)
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

void getTimeFromJulianDay(double julianDay, int *hour, int *minute, int *second)
{
	double frac = julianDay - (floor(julianDay));
	int s = (int)floor((frac * 24.0 * 60.0 * 60.0) + 0.0001);  // add constant to fix floating-point truncation error

	*hour = ((s / (60 * 60))+12)%24;
	*minute = (s/(60))%60;
	*second = s % 60;
}

QString julianDayToISO8601String(double jd)
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
	return (double)1./(24*60*60*1000)*QTime().msecsTo(time)-0.5;
}

QTime jdFractionToQTime(double jd)
{
	double decHours = std::fmod(jd+0.5, 1.0);
	int hours = (int)(decHours/0.041666666666666666666);
	int mins = (int)((decHours-(hours*0.041666666666666666666))/0.00069444444444444444444);
	return QTime::fromString(QString("%1.%2").arg(hours).arg(mins), "h.m");
}

// Use Qt's own sense of time and offset instead of platform specific code.
float getGMTShiftFromQT(double JD)
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
bool getJDFromDate(double* newjd, int y, int m, int d, int h, int min, int s)
{
	static const long IGREG2 = 15+31L*(10+12L*1582);
	double deltaTime = (h / 24.0) + (min / (24.0*60.0)) + (s / (24.0 * 60.0 * 60.0)) - 0.5;
	QDate test((y <= 0 ? y-1 : y), m, d);
	// if QDate will oblige, do so.
	if ( test.isValid() )
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

double getJDFromDate_alg2(int y, int m, int d, int h, int min, int s)
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

int numberOfDaysInMonthInYear(int month, int year)
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
double calculateSiderealPeriod(double SemiMajorAxis)
{
	// Calculate semi-major axis in meters
	double a = AU*1000*SemiMajorAxis;
	// Calculate orbital period in seconds
	// Here 1.32712440018e20 is heliocentric gravitational constant
	double period = 2*M_PI*std::sqrt(a*a*a/1.32712440018e20);
	return period/86400; // return period in days
}

// Calculate duration of mean solar day
double calculateSolarDay(double siderealPeriod, double siderealDay, bool forwardDirection)
{
	double coeff, solarDay;
	if (forwardDirection)
		coeff = 1 - siderealDay/siderealPeriod;
	else
		coeff = 1 + std::abs(siderealDay)/siderealPeriod;

	solarDay = siderealDay/coeff;

	return solarDay;
}

QString hoursToHmsStr(double hours)
{
	int h = (int)hours;
	int m = (int)((hours-h)*60);
	float s = (((hours-h)*60)-m)*60;

	return QString("%1h%2m%3s").arg(h).arg(m).arg(QString::number(s, 'f', 1));
}

#if defined(Q_OS_MAC) || defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD)
#include <sys/time.h>
#else
#include <time.h>
#endif

//! Get current time in seconds (relative to some arbitrary beginning in the past)
//!
//! Currently we only have a high-precision implementation for Mac, Linux and 
//! FreeBSD. A Windows implementation would be good as well (clock_t, 
//! which we use right now, might be faster/slower than wall clock time 
//! and usually supports milliseconds at best).
static long double getTime()
{
#if defined(Q_OS_MAC) || defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD)
	struct timeval timeVal;
	if(gettimeofday(&timeVal, NULL) != 0)
	{
		Q_ASSERT_X(false, Q_FUNC_INFO, "Failed to get time");
	}
	return static_cast<long double>(timeVal.tv_sec) + 0.000001L * timeVal.tv_usec;
#else
	clock_t cpuTime = clock();
	return static_cast<long double>(cpuTime) / CLOCKS_PER_SEC;
#endif
}

//! Time when the program execution started.
long double startTime = getTime();

long double secondsSinceStart()
{
	return getTime() - startTime;
}

double decYear2DeltaT(double y)
{
	// Note: the method here is adapted from Adapted from
	// "Five Millennium Canon of Solar Eclipses" [Espenak and Meeus]
	// A summary is described here:
	// http://eclipse.gsfc.nasa.gov/SEhelp/deltatpoly2004.html

	// set the default value for Delta T
	double u = (y-1820)/100.;
	double r = (-20 + 32 * std::pow(u,2.0));

	if (y < -500)
	{
		u = (y-1820)/100.;
		r = (-20 + 32 * std::pow(u,2.0));
	}
	else if (y < 500)
	{
		u = y/100;
		r = (10583.6 - 1014.41 * u + 33.78311 * std::pow(u,2) - 5.952053 * std::pow(u,3)
		       - 0.1798452 * std::pow(u,4) + 0.022174192 * std::pow(u,5) + 0.0090316521 * std::pow(u,6));
	}
	else if (y < 1600)
	{
		u = (y-1000)/100;
		r = (1574.2 - 556.01 * u + 71.23472 * std::pow(u,2) + 0.319781 * std::pow(u,3)
		       - 0.8503463 * std::pow(u,4) - 0.005050998 * std::pow(u,5) + 0.0083572073 * std::pow(u,6));
	}
	else if (y < 1700)
	{
		double t = y - 1600;
		r = (120 - 0.9808 * t - 0.01532 * std::pow(t,2) + std::pow(t,3) / 7129);
	}
	else if (y < 1800)
	{
		double t = y - 1700;
		r = (8.83 + 0.1603 * t - 0.0059285 * std::pow(t,2) + 0.00013336 * std::pow(t,3) - std::pow(t,4) / 1174000);
	}
	else if (y < 1860)
	{
		double t = y - 1800;
		r = (13.72 - 0.332447 * t + 0.0068612 * std::pow(t,2) + 0.0041116 * std::pow(t,3) - 0.00037436 * std::pow(t,4)
		       + 0.0000121272 * std::pow(t,5) - 0.0000001699 * std::pow(t,6) + 0.000000000875 * std::pow(t,7));
	}
	else if (y < 1900)
	{
		double t = y - 1860;
		r = (7.62 + 0.5737 * t - 0.251754 * std::pow(t,2) + 0.01680668 * std::pow(t,3)
			-0.0004473624 * std::pow(t,4) + std::pow(t,5) / 233174);
	}
	else if (y < 1920)
	{
		double t = y - 1900;
		r = (-2.79 + 1.494119 * t - 0.0598939 * std::pow(t,2) + 0.0061966 * std::pow(t,3) - 0.000197 * std::pow(t,4));
	}
	else if (y < 1941)
	{
		double t = y - 1920;
		r = (21.20 + 0.84493*t - 0.076100 * std::pow(t,2) + 0.0020936 * std::pow(t,3));
	}
	else if (y < 1961)
	{
		double t = y - 1950;
		r = (29.07 + 0.407*t - std::pow(t,2)/233 + std::pow(t,3) / 2547);
	}
	else if (y < 1986)
	{
		double t = y - 1975;
		r = (45.45 + 1.067*t - std::pow(t,2)/260 - std::pow(t,3) / 718);
	}
	else if (y < 2005)
	{
		double t = y - 2000;
		r = (63.86 + 0.3345 * t - 0.060374 * std::pow(t,2) + 0.0017275 * std::pow(t,3) + 0.000651814 * std::pow(t,4)
			+ 0.00002373599 * std::pow(t,5));
	}
	else if (y < 2050)
	{
		double t = y - 2000;
		r = (62.92 + 0.32217 * t + 0.005589 * std::pow(t,2));
	}
	else if (y < 2150)
	{
		r = (-20 + 32 * std::pow((y-1820)/100,2) - 0.5628 * (2150 - y));
	}	

	return r;
}

double getDeltaT(double jDay)
{
	int year, month, day;
	double moon = 0.;
	getDateFromJulianDay(jDay, &year, &month, &day);
	if (year<1955 or year>2005)
		moon = getMoonSecularAcceleration(jDay);
	// approximate "decimal year" = year + (month - 0.5)/12
	return decYear2DeltaT(year + (month - 0.5)/12)+moon;
}

double getMoonSecularAcceleration(double jDay)
{
	// Method described is here: http://eclipse.gsfc.nasa.gov/SEcat5/secular.html
	// For adapting from -26 to -25.858, use -0.91072 * (-25.858 + 26.0) = -0.12932224
	// For adapting from -26 to -23.895, use -0.91072 * (-23.895 + 26.0) = -1.9170656
	int year, month, day;
	getDateFromJulianDay(jDay, &year, &month, &day);
	double t = (year-1955)/100;
	return -0.12932224 * t * t;
}

} // end of the StelUtils namespace


/*
 * Copyright (C) 2020 Georg Zotti
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

#include <QDebug>
#include <QTimeZone>

#include "Calendar.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"


QString Calendar::getFormattedDateString(QVector<int> date, QString sep)
{
	QString res;
	QVector<int>::const_iterator it;
	for (it=date.constBegin(); it!=date.constEnd(); ++it)
	{
		res.append(QString::number(*it));
		res.append(sep);
	}
	return res;
}

QString Calendar::getFormattedDateString() const
{
	return getDateStrings().join(' ');
}

// CC.UE 1.24: This EXCLUDES the upper limit!
double Calendar::modInterval(double x, double a, double b)
{
	if (fuzzyEquals(a,b)) return x;
	return a + StelUtils::fmodpos(x-a, b-a);
}

// CC.UE 1.24: This EXCLUDES the upper limit! Use StelUtils::amod(x, b) for CC's (x)mod[1..b]
int Calendar::modInterval(int x, int a, int b)
{
	if (a==b) return x;
	return a + StelUtils::imod(x-a, b-a);
}

double Calendar::momentFromJD(double jd, bool respectUTCoffset)
{
	double ret=jd+jdEpoch;
	if (respectUTCoffset)
	{
		ret+=StelApp::getInstance().getCore()->getUTCOffset(jd)/24.;
	}
	return ret;
}

double Calendar::jdFromMoment(double rd, bool respectUTCoffset)
{
	double ret=rd-jdEpoch;
	if (respectUTCoffset)
	{
		ret-=StelApp::getInstance().getCore()->getUTCOffset(ret)/24.;
	}
	return ret;
}

// Reingold-Dershowitz CC.UE 1.48
int Calendar::rdCorrSum(QVector<int>parts, QVector<int>factors, int corr)
{
	Q_ASSERT(parts.length()==factors.length());

	int sum=0;
	for (int i=0; i<parts.length(); ++i)
		sum+=(parts.at(i)+corr)*factors.at(i);
	return sum;
}

// Split integer to mixed-radix vector. Reingold-Dershowitz CC.UE 1.42
QVector<int> Calendar::toRadix(int num, QVector<int>radix)
{
	int div=1;
	int rest=num;
	QVector<int> res;
	QVector<int>::const_reverse_iterator i;
	for (i = radix.rbegin(); i != radix.rend(); ++i)
	{
		div = *i;

		int lsb=StelUtils::imod(rest, div);
		res.prepend(lsb);
		rest -= lsb;
		rest /= div;
	}
	res.prepend(rest);
	return res;
}

// DEFINITIONS FROM CHAPTER 14: ASTRONOMICAL CALENDARS
const StelLocation Calendar::urbana   = StelLocation::createFromLine("Urbana	Il	09	X	250	40.1N	88.2W	225	7	UT-6");
const StelLocation Calendar::greenwich= StelLocation::createFromLine("Greenwich	London	18	X	100	51.4777815N	0E	46.9	7	UT");
const StelLocation Calendar::mecca    = StelLocation::createFromLine("Mecca	Mecca	16	X	200	21.42333333N	39.82333333E	298	9	UT+3");
const StelLocation Calendar::jerusalem= StelLocation::createFromLine("Jerusalem	Israel	16	X	2000	31.78N	35.24E	740	7	UT+2");
const StelLocation Calendar::acre     = StelLocation::createFromLine("Acre	Israel	16	X	20	32.94N	35.09E	22	6	UT+2");
double Calendar::direction(StelLocation loc1, StelLocation loc2)
{
	// We could do that, but south azimuth may cause problems.
	//return StelLocation::getAzimuthForLocation(static_cast<double>(loc1.longitude), static_cast<double>(loc1.latitude),
	//					   static_cast<double>(loc2.longitude), static_cast<double>(loc2.latitude));
	const double longObs    = static_cast<double>(loc1.longitude) * M_PI_180;
	const double latObs     = static_cast<double>(loc1.latitude ) * M_PI_180;
	const double longTarget = static_cast<double>(loc2.longitude) * M_PI_180;
	const double latTarget  = static_cast<double>(loc2.latitude ) * M_PI_180;

	double az = atan2(sin(longTarget-longObs), cos(latObs)*tan(latTarget)-sin(latObs)*cos(longTarget-longObs));
	return StelUtils::fmodpos(M_180_PI * az, 360.0);
}
double Calendar::universalFromLocal(double fractionalDay, StelLocation location)
{
	return fractionalDay-zoneFromLongitude(static_cast<double>(location.longitude));
}
double Calendar::localFromUniversal(double fractionalDay, StelLocation location)
{
	return fractionalDay+zoneFromLongitude(static_cast<double>(location.longitude));

}
double Calendar::standardFromUniversal(double fractionalDay, StelLocation location)
{
	static const QDateTime j2000(QDate(2000, 1, 1), QTime(0, 0, 0), Qt::UTC);
	QTimeZone tz(location.ianaTimeZone.toUtf8());
	return fractionalDay+tz.standardTimeOffset(j2000)/86400;
}
double Calendar::universalFromStandard(double fractionalDay, StelLocation location)
{
	static const QDateTime j2000(QDate(2000, 1, 1), QTime(0, 0, 0), Qt::UTC);
	QTimeZone tz(location.ianaTimeZone.toUtf8());
	return fractionalDay-tz.standardTimeOffset(j2000)/86400;
}
double Calendar::standardFromLocal(double fractionalDay, StelLocation location)
{
	return standardFromUniversal(universalFromLocal(fractionalDay, location), location);
}
double Calendar::localFromStandard(double fractionalDay, StelLocation location)
{
	return localFromUniversal(universalFromStandard(fractionalDay, location), location);
}
double Calendar::ephemerisCorrection(double rd)
{
	double jd=jdFromMoment(rd, false);
	double deltaT=StelApp::getInstance().getCore()->computeDeltaT(jd);
	return deltaT/86400.;
}
double Calendar::dynamicalFromUniversal(double rd_ut)
{
	return rd_ut+ephemerisCorrection(rd_ut);
}
double Calendar::universalFromDynamical(double rd_dt)
{
	return rd_dt-ephemerisCorrection(rd_dt);
}
double Calendar::julianCenturies(double rd_ut)
{
	return (dynamicalFromUniversal(rd_ut)-j2000)/36525.;
}

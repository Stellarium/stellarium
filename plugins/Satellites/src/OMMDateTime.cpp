/*
 * Copyright (C) 2023 Andy Kirkham
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

#include <QChar>
#include <QDate>
#include <QDebug>

#include "OMMDateTime.hpp"

OMMDateTime::OMMDateTime() 
{}

OMMDateTime::~OMMDateTime() 
{}

OMMDateTime::OMMDateTime(QString & s, Type t) 
{
	switch(t) {
		case STR_TLE:
			ctorTle(s);
			break;
		case STR_ISO8601:
			ctorISO(s);
			break;
		default:
			break;
	}
}

// From SGP4.cpp
static void jday_SGP4(int year, int mon, int day, int hr, int minute, double sec, double & jd, double & jdFrac)
{
	jd = 367.0 * year - floor((7 * (year + floor((mon + 9) / 12.0))) * 0.25) + floor(275 * mon / 9.0) + day +
	            1721013.5; // use - 678987.0 to go to mjd directly
	jdFrac = (sec + minute * 60.0 + hr * 3600.0) / 86400.0;
	if (fabs(jdFrac) > 1.0) {
		double dtt = floor(jdFrac);
		jd = jd + dtt;
		jdFrac = jdFrac - dtt;
	}
}

void OMMDateTime::ctorTle(const QString & s) 
{
	int year = s.mid(0, 2).toInt();
	double day = s.mid(2).toDouble();
	int whole_day = std::floor(day);
	double frac_day = day - whole_day;

	// Create a QDate.
	year += (year < 57) ? 2000 : 1900;
	QDate d(year, 1, 1);
	d = d.addDays(whole_day - 1); // Minus 1 because we start on 1st Jan.

	// Create the time.
	double seconds = (24 * 60 * 60) * frac_day;
	int whole_hours = std::floor(seconds / 3600);
	seconds -= whole_hours * 3600;
	int whole_mins = std::floor(seconds / 60);
	seconds -= whole_mins * 60;

	jday_SGP4(d.year(), d.month(), d.day(), whole_hours, whole_mins, seconds, m_epoch_jd, m_epoch_jd_frac);
}

void OMMDateTime::ctorISO(const QString & s)
{
	QDateTime d = QDateTime::fromString(s, Qt::ISODate);
	int year = d.date().year();
	int mon = d.date().month();
	int day = d.date().day();
	int hour = d.time().hour();
	int min = d.time().minute();
	double sec = d.time().second();
	auto decimal = s.indexOf(QChar('.'));
	if(decimal > 0) {
		auto frac_s = s.mid(decimal);
		double frac_d = frac_s.toDouble();
		sec += frac_d;
	}
	jday_SGP4(year, mon, day, hour, min, sec, m_epoch_jd, m_epoch_jd_frac);
}


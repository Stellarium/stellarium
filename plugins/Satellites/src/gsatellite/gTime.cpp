/***************************************************************************
 * Name: gTime.cpp
 *
 * Description:
 ***************************************************************************/

/***************************************************************************
 *   Copyright (C) 2004 by JL Trabajo                                      *
 *   ph03696@homeserver                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.             *
 ***************************************************************************/


#include <cassert>
// gtime
#include "gTime.hpp"
// GExcpt
#include "gException.hpp"
#include "stdsat.h"
#include <math.h>

#include <stdio.h>
#include <stdlib.h>

// Class GTimeSpan

//////////////////////////////////////////////////////////////////////////////
// Referencies:
// Astronomical Formulae for Calculators.
//	Jean Meeus
//	Chapter 3: Julian Day and Calendar Date. PG's 23, 24, 25
//
void gTime::setTime(int year, double day)
{
	assert((day >= 0.0) && (day < 367.0));

	// Now calculate Julian date

	year--;

	int A = (year / 100);
	int B = 2 - A + (A / 4);

	double JDforYear = (int)(365.25 * year) +
	                   (int)(30.6001 * 14)  +  //MM = 1 then MM=12 + 1 for the expresion (30.6001 * (mm +1))
	                   1720994.5 + B;

	m_time = JDforYear + day;
}


// Constructors

gTime gTime::getCurrentTime()
{

	time_t rawtime;
	struct tm * timeinfo;

	time(&rawtime);
	timeinfo = gmtime(&rawtime);

	return gTime(*timeinfo);
}



gTime::gTime(int year, double day)
{
	setTime(year, day);
}

gTime::gTime(double ai_jDays)
{
	m_time= ai_jDays;
}

gTime::gTime(int nYear, int nMonth, int nDay, int nHour, int nMin, double nSec)
{
	// Calculate N, the day of the year (1..366)
	int N;
	int F1 = (int)((275.0 * nMonth) / 9.0);
	int F2 = (int)((nMonth + 9.0) / 12.0);

	if(isLeapYear(nYear))
	{
		// Leap year
		N = F1 - F2 + nDay - 30;
	}
	else
	{
		// Common year
		N = F1 - (2 * F2) + nDay - 30;
	}

	double dDay = N + (nHour + (nMin + (nSec / 60.0)) / 60.0) / 24.0;


	setTime(nYear, dDay);

}


gTime::gTime(const gTime& timeSrc):m_time(timeSrc.m_time)
{

}

gTime::gTime(struct tm ai_timestruct)
{

	int    year = ai_timestruct.tm_year + 1900;

	double day  = ai_timestruct.tm_yday + 1;
	day += (ai_timestruct.tm_hour + (ai_timestruct.tm_min + (ai_timestruct.tm_sec / 60.0)) / 60.0) / 24.0;

	setTime(year, day);
}

gTimeSpan gTime::getTimeToUTC()
{

	//Time to utc calculation.
	time_t when   = time(NULL);
	struct tm utc = *gmtime(&when);
	struct tm lcl = *localtime(&when);
	gTimeSpan tUTCDiff;


	int delta_h = lcl.tm_hour - utc.tm_hour;
	tUTCDiff = (time_t) delta_h*3600;

	return(tUTCDiff);
}


const gTime& gTime::operator=(const gTime& timeSrc)
{
	m_time = timeSrc.m_time;
	return (*this);
}

const gTime& gTime::operator=(time_t t)
{
	struct tm *ptm = gmtime(&t);
	assert(ptm);

	int    year = ptm->tm_year + 1900;

	double day  = ptm->tm_yday + 1;
	day += (ptm->tm_hour + (ptm->tm_min + (ptm->tm_sec / 60.0)) / 60.0) / 24.0;

	setTime(year, day);

	return (*this);

}


// Attributes
double gTime::getGmtTm() const
{
	return m_time;
}

double gTime::getLocalTm() const
{
	return (m_time + getTimeToUTC().getDblSeconds());
}


time_t gTime::toTime() const
{
	return ((m_time - JDAY_JAN1_00H_1970)*KSEC_PER_DAY);


}

void gTime::toCalendarDate(int *pYear, int *pMonth , double *pDom) const
{

	assert(pYear != NULL);
	assert(pMonth != NULL);
	assert(pDom != NULL);

	double jdAdj, F, alpha, A, B, DOM;
	int Z, C, D, E, month, year;

	jdAdj = m_time + 0.5;
	Z     = (int)jdAdj;  // integer part
	F     = jdAdj - Z;   // fractional part

	if(Z < 2299161)
	{
		A = Z;
	}
	else
	{
		alpha = (int)((Z - 1867216.25) / 36524.25);
		A     = Z + 1 + alpha - (int)(alpha / 4.0);
	}

	B     = A + 1524.0;
	C     = (int)((B - 122.1) / 365.25);
	D     = (int)(C * 365.25);
	E     = (int)((B - D) / 30.6001);

	DOM   = B - D - (int)(E * 30.6001) + F;
	month = (E < 13.5) ? (E - 1) : (E - 13);
	year  = (month > 2.5) ? (C - 4716) : (C - 4715);

	*pYear = year;
	*pMonth = month;
	*pDom = DOM;
}

double gTime::toJCenturies() const
{

	double jd;
	double UT = fmod((m_time + 0.5), 1.0);
	jd = m_time - UT;
	double TU = (jd- JDAY_JAN1_12H_2000) / 36525.0;

	return TU;
}

// @method  toThetaGMST();
// Definition: Calculate Theta Angle at Greenwich Mean Time for the Julian date. The return value
// is the angle, in radians, measuring eastward from the Vernal Equinox to the
// prime meridian.
double gTime::toThetaGMST() const
{

	double jd, Theta_JD;
	double UT = fmod((m_time + 0.5), 1.0);
	jd = m_time - UT;
	double TU = (jd- JDAY_JAN1_12H_2000) / 36525.0;

	double GMST = 24110.54841 + TU *
	              (8640184.812866 + TU * (0.093104 - TU * 6.2e-06));

	GMST = fmod((GMST + KSEC_PER_DAY * OMEGA_E * UT),KSEC_PER_DAY);
	Theta_JD=(K2PI * (GMST / KSEC_PER_DAY));

	if(Theta_JD <0.0)
		Theta_JD+=K2PI;

	return Theta_JD;
}

// @method  toThetaLMST();
// Definition: Calculate Theta Angle at Local Mean Time for the Julian date.
double gTime::toThetaLMST(double longitude) const
{
	return fmod(toThetaGMST() + longitude,  K2PI);
}



// Operations
// time math
gTimeSpan gTime::operator-(gTime ai_time) const
{
	return (gTimeSpan((m_time - ai_time.m_time)));
}

gTime gTime::operator-(gTimeSpan ai_timeSpan) const
{
	return (gTime((m_time - ai_timeSpan.getDblDays())));
}

gTime gTime::operator+(gTimeSpan ai_timeSpan) const
{
	return (gTime((m_time + ai_timeSpan.getDblDays())));
}

const gTime& gTime::operator+=(gTimeSpan ai_timeSpan)
{
	m_time += ai_timeSpan.getDblDays();
	return (*this);

}

const gTime& gTime::operator-=(gTimeSpan ai_timeSpan)
{
	m_time -= ai_timeSpan.getDblDays();
	return (*this);
}

bool gTime::operator==(gTime ai_time) const
{

	if(m_time == ai_time.m_time)
		return true;
	else
		return false;
}

bool gTime::operator!=(gTime ai_time) const
{

	if(m_time != ai_time.m_time)
		return true;
	else
		return false;
}

bool gTime::operator<(gTime ai_time) const
{

	if(m_time < ai_time.m_time)
		return true;
	else
		return false;
}

bool gTime::operator>(gTime ai_time) const
{

	if(m_time > ai_time.m_time)
		return true;
	else
		return false;
}

bool gTime::operator<=(gTime ai_time) const
{

	if(m_time <= ai_time.m_time)
		return true;
	else
		return false;
}

bool gTime::operator>=(gTime ai_time) const
{

	if(m_time >= ai_time.m_time)
		return true;
	else
		return false;
}

/****************************************************************************
 * Name: gTime.hpp
 *
 * Description: gTime y gTimeSpan classes declaration.
 *		 This classes implement the method and operators to manage
 *		 calculation over dates and timestamps.
 *
 *
 ***************************************************************************/

/***************************************************************************
 *   Copyright (C) 2006 by J. L. Canales                                   *
 *   jlcanales@users.sourceforge.net                                       *
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

/////////////////////////////////////////////////////////////////////////////
// gTimeSpan and gTime
#ifndef GTIME_HPP
#define GTIME_HPP

#include <time.h>
#include <iostream> // for operator<<(), see below

static const double JDAY_JAN1_00H_1900 = 2415019.5; // Jan 1.0 1900 = Jan 1 1900 00h UTC
static const double JDAY_JAN1_12H_1900 = 2415020.0; // Jan 1.5 1900 = Jan 1 1900 12h UTC
static const double JDAY_JAN1_12H_2000 = 2451545.0; // Jan 1.5 2000 = Jan 1 2000 12h UTC
static const double JDAY_JAN1_00H_1970 = 2440587.5;

static const double OMEGA_E = 1.002737909350795; // earth rotation per sideral day

static const int KSEC_PER_MIN = 60;
static const int KSEC_PER_HR  = 3600;
static const int KSEC_PER_DAY = 86400;
static const int KMIN_PER_HR  = 60;
static const int KMIN_PER_DAY = 1440;
static const int KHR_PER_DAY  = 60;

//! @ingroup satellites
class gTimeSpan
{
public:
	// Constructors
	gTimeSpan(double timeSpanSrc = 0); // timeSpanSrc is mesured in days and fraction of day
	gTimeSpan(long lDays, int nHours, int nMins, double nSecs);

	gTimeSpan(const gTimeSpan& timeSpanSrc);
	const gTimeSpan& operator=(const gTimeSpan& timeSpanSrc);

	// Equal to time in Julian Days
	const gTimeSpan& operator=(const double& timeSpanSrc)
	{
		m_timeSpan=timeSpanSrc*KSEC_PER_DAY;
		return *this;
	}

	// Attributes
	// extract parts

	//! Operation: getDays()
	//!	This method returns the integer days number stored in the gTimeSpan object.
	//! @return
	//!    long  Total number of days
	long  getDays() const;

	//! Operation: getHours()
	//!	This method returns the integer hours number stored in the gTimeSpan object.
	//! @return
	//!    int This is a value between 0 and 23 hours
	int   getHours() const;

	//! Operation: getMinutes()
	//! This method returns the integer Minutes number stored in the gTimeSpan object.
	//! @return
	//!	int This is a value between 0 and 59 minutes.
	int   getMinutes() const;

	//! Operation: getSeconds()
	//!  	This method returns the integer seconds number stored in the gTimeSpan object.
	//! @return
	//!	int This is a value between 0 and 59 seconds
	int   getSeconds() const;

	//! Operation: getDblSeconds()
	//!	This method returns the total seconds number stored in the gTimeSpan
	//!	object.
	//! @return
	//!	double Total number of seconds in seconds and fraction of second.
	double getDblSeconds() const;


	//! Operation: getDblDays()
	//!	This method returns the total days number stored in the gTimeSpan
	//!	object.
	//! @return
	//!	double Total number of days in days and fraction of day.
	double getDblDays() const;
	// Operations

	//////////////////////////////////////
	// TimeSpan Object Math operations
	//////////////////////////////////////
	gTimeSpan operator-(gTimeSpan timeSpan) const;
	gTimeSpan operator+(gTimeSpan timeSpan) const;
	const gTimeSpan& operator+=(gTimeSpan timeSpan);
	const gTimeSpan& operator-=(gTimeSpan timeSpan);
	bool operator==(gTimeSpan timeSpan) const;
	bool operator!=(gTimeSpan timeSpan) const;
	bool operator<(gTimeSpan timeSpan) const;
	bool operator>(gTimeSpan timeSpan) const;
	bool operator<=(gTimeSpan timeSpan) const;
	bool operator>=(gTimeSpan timeSpan) const;

private:
	double m_timeSpan; //time span in julian days
};

//! @class gTime
//! This class implements time calculations.
//! Time is stored in julian days
//! - Time getting in GMT, Local and Sidereal (earth rotational angle)
//! - Time Math operations.

class gTime
{
public:
	// Constructors
	gTime(double ai_jDays = 0);
	gTime(int year, double day);
	gTime(int nYear, int nMonth, int nDay, int nHour, int nMin, double nSec);
	gTime(struct tm ai_timestruct);

	// copy constructor
	gTime(const gTime& timeSrc);

	//////////////////////////////////////
	// Time Object setting operations
	//////////////////////////////////////

	// Operation setTime
	//! @brief Set the time value of the time object to the julian day of
	//!      year,  day.
	//! @details
	//!    References:\n
	//!      Astronomical Formulae for Calculators.\n
	//!	     Jean Meeus\n
	//!	     Chapter 3: Julian Day and Calendar Date. PG's 23, 24, 25\n
	//! @param[in] year date year
	//!	@param[in] day  date day and day fraction. Notice: 1 Jan is day=0 (not 1)
	void setTime(int year, double day);

	// Operation operator=
	//! @brief overload de = operator to assign time values to he object.
	//! @param timeSrc Time to be assigned
	//!	@return const gTime&  Reference to *this object modified after operation
	const gTime& operator=(const gTime& timeSrc);

	// Operation operator=
	//! @brief overload de = operator to assign time values to he object in
	//!   time_t (operating system) format.
	//! @param[in] t Time to be assigned
	//! @return
	//!   const gTime&  Reference to *this object modified after operation
	const gTime& operator=(time_t t);


	// Operation operator=
	//! @brief overload the = operator to assign time values to the object in
	//!   julian days.
	//! @param[in] t Time (JD) to be assigned
	//! @return
	//!   const gTime&  Reference to *this object modified after operation
	const gTime& operator=(double t)
	{
		m_time = t;
		return *this;
	}

	//////////////////////////////////////
	// Time Machine getting operations
	//////////////////////////////////////

	// Operation  getCurrentTime();
	//! @brief Returns a gTime object setted with the actual machine time.
	//! @warning This method works with the operating system function time. So it doesn't support seconds fractions
	//! @return
	//!    gTime Object assigned with real machine time.
	static gTime     getCurrentTime();

	// Operation  getTimeToUTC();
	//! @brief Returns a gTimeSpan object setted with the Local Time Span setted in the machine.
	//! @return
	//!    gTimeSpan Object assigned with the real machine GMT Diff.
	static gTimeSpan getTimeToUTC();

	// Operation: isLeapYear
	//! @brief Leap Year Calculation.
	//! @details  This Operation use the standart algorithm to
	//!     calculate Leap Years. Reference: http://en.wikipedia.org/wiki/Leap_year
	//! @param ai_year Year number
	//! @return true if ai_year is a Leap Year
	static bool isLeapYear(int ai_year)
	{
		return (ai_year % 4 == 0 && ai_year % 100 != 0) || (ai_year % 400 == 0);
	}


	//////////////////////////////////////
	// Time Object getting operations
	//////////////////////////////////////

	// Operation:  getGmtTm();
	//! @brief Returns the time GMT value in Julian days.
	//! @return:
	//!    double Julian Days represented in the gTime Object
	double getGmtTm() const;

	// Operation:  getLocalTm();
	//! @brief Returns the time Local value in Julian days.
	//! @return:
	//!    double Julian Days represented in the gTime Object
	double getLocalTm() const;


	//////////////////////////////////////
	// Time Object Converting operations
	//////////////////////////////////////

	// Operation:  toTime();
	//! @brief Returns the time_t value of the gTime Object.
	//! @warning time_t object doesn't support seconds fractions. This method
	//!            must be avoid for astronomical calculation.
	//! @return:
	//!    time_t machine time represented in the gTime Object
	time_t toTime() const;


	void toCalendarDate(int *pYear, int *pMonth , double *pDom) const;

	double toJCenturies() const;

	// Operation:  toThetaGMST();
	//! @brief Calculate Theta Angle at Greenwich Mean Time for the Julian date.
	//! @details The return value is the angle, in radians, measuring eastward from
	//! the Vernal Equinox to the  prime meridian.\n
	//!   References:\n
	//!     Explanatory Supplement to the Astronomical Almanac, page 50.
	//!     http://books.google.com/books?id=uJ4JhGJANb4C&lpg=PA52&hl=es&pg=PA50#v=onepage&q&f=false
	//!     "Orbital Coordinate Systems, Part II."  Dr. T.Kelzo
	//!	     Satellite Times, 2, no. 2 (November/December 1995): 78-79.
	//!       http://www.celestrak.com/columns/v02n02/
	//! @return
	//!    Theta Angle in radians, measuring eastward from the Vernal Equinox to the
	//!    prime meridian
	double toThetaGMST() const;

	// Operation:  toThetaLMST();
	//! @brief Calculate Theta Angle at Local Mean Time for the Julian date.
	//! @param[in] longitude Geographical longitude for the local meridian.\n
	//!     Positive longitude = east longitude\n
	//!     Negative longitude = west longitude\n
	//! @return Theta Angle in radians, measuring eastward from the Vernal Equinox to the
	//!    prime meridian
	double toThetaLMST(double longitude) const;


	//////////////////////////////////////
	// Time Object Math operations
	//////////////////////////////////////

	gTimeSpan operator-(gTime time)         const;
	gTime operator- (gTimeSpan timeSpan) const;
	gTime operator+ (gTimeSpan timeSpan) const;
	const gTime& operator+= (gTimeSpan timeSpan);
	const gTime& operator-= (gTimeSpan timeSpan);
	bool operator== (gTime time)         const;
	bool operator!= (gTime time)         const;
	bool operator< (gTime time)         const;
	bool operator> (gTime time)         const;
	bool operator<= (gTime time)         const;
	bool operator>= (gTime time)         const;

private:
	double m_time; //Time in Julian Days
};



inline std::ostream& operator<<(std::ostream& s, gTime& ai_gTime)
{
	int    year, month;
	double Dom;

	ai_gTime.toCalendarDate(&year, &month , &Dom);

	s << "GMT " << year <<" " << month <<":"<<Dom;
	return s;
}

inline std::ostream& operator<<(std::ostream& s, gTimeSpan& ai_gTimeSpan)
{
	s << "D " <<ai_gTimeSpan.getDays()<<" "<<ai_gTimeSpan.getHours()<<":"<<ai_gTimeSpan.getMinutes()<<":"<<ai_gTimeSpan.getSeconds()<<std::endl;
	return s;
}


#endif // GTIME_HPP

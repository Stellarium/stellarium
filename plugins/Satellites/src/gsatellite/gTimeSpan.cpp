/***************************************************************************
 * Name:GTimeSpan.cpp
 *
 * Description: GTimeSpan class definition.
 *              This class has the needed functionality to manage time interval
 *              operations.
 ***************************************************************************/

/***************************************************************************
 *   Copyright (C) 2004 by J.L. Canales                                    *
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

// gtime
#include "gTime.hpp"

// Class GTimeSpan

// Constructors
gTimeSpan::gTimeSpan(double timeSpanSrc)  //timeSpanScr meassured in days and day fractions
{
	m_timeSpan=timeSpanSrc*KSEC_PER_DAY;
}

gTimeSpan::gTimeSpan(long lDays, int nHours, int nMins, double nSecs)
{
	m_timeSpan= (lDays     * KSEC_PER_DAY)
	            + (nHours * KSEC_PER_HR)
	            + (nMins  * KSEC_PER_MIN)
	            +  nSecs;
}

gTimeSpan::gTimeSpan(const gTimeSpan& timeSpanSrc):m_timeSpan(timeSpanSrc.m_timeSpan)
{
}

////////////////////////////////////////////////////////////////////////////
//## Operation: operator=
//	Equal operator overload.
//

const gTimeSpan& gTimeSpan::operator=(const gTimeSpan& timeSpanSrc)
{
	m_timeSpan = timeSpanSrc.m_timeSpan;
	return (*this);
}

////////////////////////////////////////////////////////////////////////////
//## Operation: getDays()
//	This method returns the integer days number stored in the gTimeSpan object.


long  gTimeSpan::getDays() const
{
	return (long)(m_timeSpan / KSEC_PER_DAY);
}

////////////////////////////////////////////////////////////////////////////
//## Operation: getHours()
//	This method returns the integer hours number stored in the gTimeSpan object.
//	This is a value between 0 and 23 hours

int gTimeSpan::getHours() const
{
	double AuxValue = m_timeSpan - (getDays() * KSEC_PER_DAY);
	return static_cast<int>(AuxValue / KSEC_PER_HR);
}

////////////////////////////////////////////////////////////////////////////
//## Operation: getMinutes()
//  This method returns the integer Minutes number stored in the gTimeSpan object.
//	This is a value between 0 and 59 minutes.

int gTimeSpan::getMinutes() const
{
	double AuxValue = m_timeSpan - (getDays()  * KSEC_PER_DAY)
	                  - (getHours() * KSEC_PER_HR);
	return static_cast<int>(AuxValue / KSEC_PER_MIN);
}

////////////////////////////////////////////////////////////////////////////
//## Operation: getSeconds()
//  This method returns the integer seconds number stored in the gTimeSpan object.
//	This is a value between 0 and 59 seconds

int gTimeSpan::getSeconds() const
{
	double AuxValue = m_timeSpan - (getDays()  * KSEC_PER_DAY)
	                  - (getHours() * KSEC_PER_HR)
	                  - (getMinutes() * KSEC_PER_MIN);
	return static_cast<int>(AuxValue);
}

////////////////////////////////////////////////////////////////////////////
//## Operation: getDblSeconds()
//	This metrod returns the total seconds number stored in the gTimeSpan
//	object.

double gTimeSpan::getDblSeconds() const
{
	return m_timeSpan;
}

////////////////////////////////////////////////////////////////////////////
//## Operation: getDblDays()
//	This metrod returns the total seconds number stored in the gTimeSpan
//	object.

double gTimeSpan::getDblDays() const
{
	return m_timeSpan/KSEC_PER_DAY;
}

// time math Operations
////////////////////////////////////////////////////////////////////////////
//## Operation: operator
//	Operators overload.

gTimeSpan gTimeSpan::operator-(gTimeSpan ai_timeSpan) const
{
	return (gTimeSpan(m_timeSpan - ai_timeSpan.getDblSeconds()));
}

gTimeSpan gTimeSpan::operator+(gTimeSpan ai_timeSpan) const
{
	return (gTimeSpan(m_timeSpan + ai_timeSpan.getDblSeconds()));
}

const gTimeSpan& gTimeSpan::operator+=(gTimeSpan ai_timeSpan)
{
	m_timeSpan += ai_timeSpan.getDblSeconds();
	return (*this);
}

const gTimeSpan& gTimeSpan::operator-=(gTimeSpan ai_timeSpan)
{
	m_timeSpan -= ai_timeSpan.getDblSeconds();
	return (*this);
}

bool gTimeSpan::operator==(gTimeSpan ai_timeSpan) const
{
	if(m_timeSpan == ai_timeSpan.getDblSeconds())
		return true;

	return false;
}

bool gTimeSpan::operator!=(gTimeSpan ai_timeSpan) const
{
	if(m_timeSpan != ai_timeSpan.getDblSeconds())
		return true;

	return false;
}

bool gTimeSpan::operator<(gTimeSpan ai_timeSpan) const
{
	if(m_timeSpan < ai_timeSpan.getDblSeconds())
		return true;

	return false;
}

bool gTimeSpan::operator>(gTimeSpan ai_timeSpan) const
{
	if(m_timeSpan > ai_timeSpan.getDblSeconds())
		return true;

	return false;
}

bool gTimeSpan::operator<=(gTimeSpan ai_timeSpan) const
{
	if(m_timeSpan <= ai_timeSpan.getDblSeconds())
		return true;

	return false;
}

bool gTimeSpan::operator>=(gTimeSpan ai_timeSpan) const
{
	if(m_timeSpan >= ai_timeSpan.getDblSeconds())
		return true;

	return false;
}

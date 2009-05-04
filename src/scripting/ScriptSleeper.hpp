/*
 * Stellarium
 * Copyright (C) 2009 Matthew Gates
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

#ifndef _SCRIPTSLEEPER_HPP_
#define _SCRIPTSLEEPER_HPP_

#include <QThread>
#include <QTime>
#include "VecMath.hpp"

//! @class ScriptSleeper
//! provides a blocking sleep function which can receive a signal
//! changing the rate at which the sleep timer counts down.
class ScriptSleeper : public QThread
{
public:
	ScriptSleeper();
	~ScriptSleeper();
	void sleep(int ms);
	void setRate(double newRate);
	double getRate(void) const;
	
private:
	QTime sleptTime;
	int sleepForTime;
	double scriptRateOnSleep;
	double scriptRate;

};
		
#endif // _SCRIPTSLEEPER_HPP_


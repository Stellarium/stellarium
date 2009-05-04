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

#include "ScriptSleeper.hpp"

ScriptSleeper::ScriptSleeper()
	: scriptRate(1.0)
{
}

ScriptSleeper::~ScriptSleeper()
{
}

void ScriptSleeper::setRate(double newRate)
{
	scriptRate = newRate;
}

double ScriptSleeper::getRate(void) const
{
	return scriptRate;
}

void ScriptSleeper::sleep(int ms)
{
	scriptRateOnSleep = scriptRate;
	sleptTime.start();
	int sleepForTime = ms/scriptRate;
	while(sleepForTime > sleptTime.elapsed())
	{
		msleep(10);
		if (scriptRateOnSleep!=scriptRate)
		{
			int sleepLeft = sleepForTime - sleptTime.elapsed();
			sleepForTime = sleptTime.elapsed() + (sleepLeft / scriptRate);
			scriptRateOnSleep = scriptRate;
		}
	}
}


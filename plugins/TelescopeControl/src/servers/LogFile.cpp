/*
 * Author and Copyright of this file and of the stellarium telescope feature:
 * Johannes Gajdosik, 2006
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

#include "LogFile.hpp"

#include <QTextStream>

QTextStream &operator<<(QTextStream &o, const Now &now)
{
	qlonglong x = now.time;
	const int micros = x % 1000000; x /= 1000000;
	const int secs = x % 60; x /= 60;
	const int mins = x % 60; x /= 60;
	const int hours = x % 24; x /= 24;
	o << qSetPadChar('0')
	  << x << ", " //Days since the Unix epoch (1970-01-01)
	  //ISO 8601 UTC time with decimal seconds:
	  << qSetFieldWidth(2) << hours  << qSetFieldWidth(0) << ':'
	  << qSetFieldWidth(2) << mins   << qSetFieldWidth(0) << ':'
	  << qSetFieldWidth(2) << secs   << qSetFieldWidth(0) << '.'
	  << qSetFieldWidth(6) << micros << qSetFieldWidth(0)
	  << qSetPadChar(' ') << "Z: ";//'Z' is for UTC
	return o;
}

QTextStream * log_file = NULL;

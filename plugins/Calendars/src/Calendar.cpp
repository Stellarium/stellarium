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

#include "Calendar.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"

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

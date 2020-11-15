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

#include "StelTranslator.hpp"
#include "MayaLongCountCalendar.hpp"
#include "StelUtils.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"

const long int MayaLongCountCalendar::mayanEpoch=fixedFromJD(584283);

MayaLongCountCalendar::MayaLongCountCalendar(double jd): Calendar(jd)
{
	parts={0, 0, 0, 0, 0};
	retranslate();
}

void MayaLongCountCalendar::retranslate()
{
}

// Set a calendar date from the Julian day number
void MayaLongCountCalendar::setJD(double JD)
{
	this->JD=JD;

	const int rd=fixedFromJD(JD, true);
	const int longCount=rd-mayanEpoch;
	const int baktun     =longCount / 144000;
	const int dayOfBaktun=longCount % 144000;
	const int katun      =dayOfBaktun / 7200;
	const int dayOfKatun =dayOfBaktun % 7200;
	const int tun        =dayOfKatun / 360;
	const int dayOfTun   =dayOfKatun % 360;
	const int uinal      =dayOfTun / 20;
	const int kin        =dayOfTun % 20;

	parts={ baktun, katun, tun, uinal, kin };
	emit partsChanged(parts);
}

// get a stringlist of calendar date elements sorted from the largest to the smallest.
// baktun-katun-tun-uinal-kin
QStringList MayaLongCountCalendar::getDateStrings()
{
	QStringList list;
	list << QString::number(std::lround(parts.at(0)));
	list << QString::number(std::lround(parts.at(1)));
	list << QString::number(std::lround(parts.at(2)));
	list << QString::number(std::lround(parts.at(3)));
	list << QString::number(std::lround(parts.at(4)));

	return list;
}


// get a formatted complete string for a date
QString MayaLongCountCalendar::getFormattedDateString()
{
	QStringList str=getDateStrings();
	return QString("%1.%2.%3.%4.%5")
			.arg(str.at(0)) // baktun
			.arg(str.at(1)) // katun
			.arg(str.at(2)) // tun
			.arg(str.at(3)) // uinal
			.arg(str.at(4)); // kin
}



// set date from a vector of calendar date elements sorted from the largest to the smallest.
// baktun-katun-tun-uinal-kin
void MayaLongCountCalendar::setDate(QVector<int> parts)
{
	// Problem: This sets time to midnight. We need to keep and reset the fractional day.
	//const double dayFraction=JD-std::floor(JD-.5);
	//const double dayFraction=JD+0.5 -std::floor(JD);

	this->parts=parts;

	const int baktun=parts.at(0);
	const int katun =parts.at(1);
	const int tun   =parts.at(2);
	const int uinal =parts.at(3);
	const int kin   =parts.at(4);

	const int fixedFromMayanLongCount=mayanEpoch+baktun*144000+katun*7200+tun*360+uinal*20+kin;

	//JD=jdFromFixed(fixedFromMayanLongCount)+dayFraction;

	// restore time from JD!
	double frac=StelUtils::fmodpos(JD+0.5+StelApp::getInstance().getCore()->getUTCOffset(JD)/24., 1.);
	JD=jdFromFixed(fixedFromMayanLongCount+frac, true);

	emit jdChanged(JD);
}

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
#include "AztecTonalpohualliCalendar.hpp"
#include "AztecXihuitlCalendar.hpp"
#include "StelUtils.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"

const int AztecTonalpohualliCalendar::aztecTonalpohualliCorrelation=AztecXihuitlCalendar::aztecCorrelation-aztecTonalpohualliOrdinal({1, 5});

AztecTonalpohualliCalendar::AztecTonalpohualliCalendar(double jd) : MayaTzolkinCalendar(jd)
{
	AztecTonalpohualliCalendar::retranslate();
}

QMap<int, QString> AztecTonalpohualliCalendar::tonalpohualliNames;

void AztecTonalpohualliCalendar::retranslate()
{
	// fill the name lists with translated/transcribed day names
	tonalpohualliNames={
		{ 1, qc_("Cipactli (Alligator)"    , "Aztec Tonalpohualli name")},
		{ 2, qc_("Ehecatl (Wind)"          , "Aztec Tonalpohualli name")},
		{ 3, qc_("Calli (House)"           , "Aztec Tonalpohualli name")},
		{ 4, qc_("Cuetzpallin (Iguana)"    , "Aztec Tonalpohualli name")},
		{ 5, qc_("Coatl (Serpent)"         , "Aztec Tonalpohualli name")},
		{ 6, qc_("Miquiztli (Death)"       , "Aztec Tonalpohualli name")},
		{ 7, qc_("Mazatl (Deer)"           , "Aztec Tonalpohualli name")},
		{ 8, qc_("Tochtli (Rabbit)"        , "Aztec Tonalpohualli name")},
		{ 9, qc_("Atl (Water)"             , "Aztec Tonalpohualli name")},
		{10, qc_("Itzcuintli (Dog)"        , "Aztec Tonalpohualli name")},
		{11, qc_("Ozomatli (Monkey)"       , "Aztec Tonalpohualli name")},
		{12, qc_("Malinalli (Grass)"       , "Aztec Tonalpohualli name")},
		{13, qc_("Acatl (Cane)"            , "Aztec Tonalpohualli name")},
		{14, qc_("Ocelotl (Jaguar)"        , "Aztec Tonalpohualli name")},
		{15, qc_("Quauhtli (Eagle)"        , "Aztec Tonalpohualli name")},
		{16, qc_("Cozcaquauhtli (Buzzard)" , "Aztec Tonalpohualli name")},
		{17, qc_("Ollin (Quake)"           , "Aztec Tonalpohualli name")},
		{18, qc_("Tecpatl (Flint)"         , "Aztec Tonalpohualli name")},
		{19, qc_("Quiahuitl (Rain)"        , "Aztec Tonalpohualli name")},
		{20, qc_("Xochitl (Flower)"        , "Aztec Tonalpohualli name")}};
}

// Set a calendar date from the Julian day number
void AztecTonalpohualliCalendar::setJD(double JD)
{
	this->JD=JD;

	const int rd=fixedFromJD(JD, true);
	const int count=rd-aztecTonalpohualliCorrelation+1;
	const int number = StelUtils::amod(count, 13);
	const int name = StelUtils::amod(count, 20);

	parts = { number, name };

	emit partsChanged(parts);
}

// get a stringlist of calendar date elements sorted from the largest to the smallest.
// baktun-katun-tun-uinal-kin
QStringList AztecTonalpohualliCalendar::getDateStrings() const
{
	QStringList list;
	list << QString::number(parts.at(0));
	list << tonalpohualliNames.value(parts.at(1));

	return list;
}

// set date from a vector of calendar date elements sorted from the largest to the smallest.
// month-day
// We face a problem as the year is not unique. We can only find the date before current JD which matches the parts.
void AztecTonalpohualliCalendar::setDate(QVector<int> parts)
{
	this->parts=parts;

	const int rdOnOrBefore=aztecTonalpohualliOnOrBefore(parts, fixedFromJD(JD));
	// restore time from JD!
	double frac=StelUtils::fmodpos(JD+0.5+StelApp::getInstance().getCore()->getUTCOffset(JD)/24., 1.);
	JD=jdFromFixed(rdOnOrBefore+frac, true);

	emit jdChanged(JD);
}

int AztecTonalpohualliCalendar::aztecTonalpohualliOnOrBefore(QVector<int> tonalpohualli, int rd)
{
	return modInterval(aztecTonalpohualliCorrelation+aztecTonalpohualliOrdinal(tonalpohualli), rd, rd-260);
}

// get 2-part vector of Tonalpohualli date from RD
QVector<int> AztecTonalpohualliCalendar::aztecTonalpohualliFromFixed(int rd)
{
	const int count=rd-aztecTonalpohualliCorrelation+1;
	const int number = StelUtils::amod(count, 13);
	const int name = StelUtils::amod(count, 20);

	return { number, name };
}

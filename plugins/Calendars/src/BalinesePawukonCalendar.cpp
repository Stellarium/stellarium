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

#include "BalinesePawukonCalendar.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"


BalinesePawukonCalendar::BalinesePawukonCalendar(double jd): Calendar(jd)
{
	parts={0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
	BalinesePawukonCalendar::retranslate();
}

// Set a calendar date from the Julian day number
void BalinesePawukonCalendar::setJD(double JD)
{
	this->JD=JD;
	const int rd=fixedFromJD(JD, true);

	parts=baliPawukonFromFixed(rd);
	emit partsChanged(parts);
}

// set date from a vector of calendar date elements in canonical order.
// Given that we cannot set a date directly, this sets the date of these elements on or before the current JD
void BalinesePawukonCalendar::setDate(QVector<int> parts)
{
	// Problem: This sets time to midnight. We need to keep and reset the fractional day.
	//const double dayFraction=JD-std::floor(JD-.5);
	//const double dayFraction=JD+0.5 -std::floor(JD);

	this->parts=parts;
	int fixedFromBP=baliOnOrBefore(parts, fixedFromJD(JD));

	// restore time from JD!
	double frac=StelUtils::fmodpos(JD+0.5+StelApp::getInstance().getCore()->getUTCOffset(JD)/24., 1.);
	JD=jdFromFixed(fixedFromBP+frac, true);

	emit jdChanged(JD);
}

//! get a stringlist of calendar date elements sorted from the largest to the smallest.
//! The order depends on the actual calendar
QStringList BalinesePawukonCalendar::getDateStrings() const
{
	QStringList list;
	list << ekawaraNames.value(parts.at(0));
	list << dwiwaraNames.value(parts.at(1));
	list << triwaraNames.value(parts.at(2));
	list << caturwaraNames.value(parts.at(3));
	list << pancawaraNames.value(parts.at(4));
	list << sadwaraNames.value(parts.at(5));
	list << saptawaraNames.value(parts.at(6));
	list << asatawaraNames.value(parts.at(7));
	list << sangawaraNames.value(parts.at(8));
	list << dasawaraNames.value(parts.at(9));
	return list;
}

QString BalinesePawukonCalendar::getFormattedDateString() const
{
	QStringList dateStrings=getDateStrings();
	QString str;
	for (int i=1; i<=10; i++)
	{
		str.append(QString("%1:%2 (%3) ").arg(QString::number(i)).arg(QString::number(parts.at(i-1))).arg(dateStrings.at(i-1)));
	}
	return str;
}

// get a formatted string for the 5 first components of a date.
QString BalinesePawukonCalendar::getFormattedDateString1to5() const
{
	QStringList dateStrings=getDateStrings();
	QString str;
	str.append(dateStrings.at(0)); // "Luang" or nothing.
	if (str.length()>0) str.append(" ");
	for (int i=2; i<=5; i++)
	{
		str.append(QString("%1:%2 (%3) ").arg(QString::number(i)).arg(QString::number(parts.at(i-1))).arg(dateStrings.at(i-1)));
	}
	return str;
}

// get a formatted string for the 5 second components of a date.
QString BalinesePawukonCalendar::getFormattedDateString6to10() const
{
	QStringList dateStrings=getDateStrings();
	QString str;
	for (int i=6; i<=10; i++)
	{
		str.append(QString("%1:%2 (%3) ").arg(QString::number(i)).arg(QString::number(parts.at(i-1))).arg(dateStrings.at(i-1)));
	}
	return str;
}

const int BalinesePawukonCalendar::baliEpoch=fixedFromJD(146, false);

QVector<int> BalinesePawukonCalendar::baliPawukonFromFixed(const int rd)
{
	return {
		baliLuangFromFixed(rd),
		baliDwiwaraFromFixed(rd),
		baliTriwaraFromFixed(rd),
		baliCaturwaraFromFixed(rd),
		baliPancawaraFromFixed(rd),
		baliSadwaraFromFixed(rd),
		baliSaptawaraFromFixed(rd),
		baliAsatawaraFromFixed(rd),
		baliSangawaraFromFixed(rd),
		baliDasawaraFromFixed(rd) };
}

int BalinesePawukonCalendar::baliDayFromFixed(const int rd)
{
	return StelUtils::imod(rd-baliEpoch, 210);
}

int BalinesePawukonCalendar::baliTriwaraFromFixed(const int rd)
{
	return StelUtils::imod(baliDayFromFixed(rd), 3) +1;
}

int BalinesePawukonCalendar::baliSadwaraFromFixed(const int rd)
{
	return StelUtils::imod(baliDayFromFixed(rd), 6) +1;
}

int BalinesePawukonCalendar::baliSaptawaraFromFixed(const int rd)
{
	return StelUtils::imod(baliDayFromFixed(rd), 7) +1;
}

int BalinesePawukonCalendar::baliPancawaraFromFixed(const int rd)
{
	return StelUtils::amod(baliDayFromFixed(rd)+2, 5);
}

int BalinesePawukonCalendar::baliWeekFromFixed(const int rd)
{
	return StelUtils::intFloorDiv(baliDayFromFixed(rd), 7) +1;
}

int BalinesePawukonCalendar::baliDasawaraFromFixed(const int rd)
{
	const int i=baliPancawaraFromFixed(rd)-1;
	const int j=baliSaptawaraFromFixed(rd)-1;

	return StelUtils::imod(1+ QVector<int>({5, 9, 7, 4, 8}).at(i) + QVector<int>({5, 4, 3, 7, 8, 6, 9}).at(j), 10);
}

int BalinesePawukonCalendar::baliDwiwaraFromFixed(const int rd)
{
	return StelUtils::amod(baliDasawaraFromFixed(rd), 2);
}

int BalinesePawukonCalendar::baliLuangFromFixed(const int rd)
{
	// CC uses a boolean for (.)mod2==0; We return 1 in that case, but must inverse logic...
	return StelUtils::imod(baliDasawaraFromFixed(rd)+1, 2);
}

int BalinesePawukonCalendar::baliSangawaraFromFixed(const int rd)
{
	return StelUtils::imod(qMax(0, baliDayFromFixed(rd)-3), 9)+1;
}

int BalinesePawukonCalendar::baliAsatawaraFromFixed(const int rd)
{
	return StelUtils::imod(qMax(6, 4 + StelUtils::imod(baliDayFromFixed(rd)-70, 210)), 8)+1;
}

int BalinesePawukonCalendar::baliCaturwaraFromFixed(const int rd)
{
	return StelUtils::amod(baliAsatawaraFromFixed(rd), 4);
}

int BalinesePawukonCalendar::baliOnOrBefore(const QVector<int>baliDate, const int rd)
{
	const int a5=baliDate.at(4)-1;
	const int a6=baliDate.at(5)-1;
	const int b7=baliDate.at(6)-1;
	const int b35=StelUtils::imod(a5+14+15*(b7-a5), 35);
	const int days=a6+36*(b35-a6);
	static const int Delta=baliDayFromFixed(0);
	return rd-StelUtils::imod(rd+Delta-days, 210);
}

//! Translate e.g. stringlists of part names
void BalinesePawukonCalendar::retranslate()
{
	ekawaraNames   = {{ 1, qc_("Luang"    , "Balinese Pawukon date name")}};
	dwiwaraNames   = {{ 1, qc_("Menga"    , "Balinese Pawukon date name")},
			  { 2, qc_("Pepet"    , "Balinese Pawukon date name")}};
	triwaraNames   = {{ 1, qc_("Pasah"    , "Balinese Pawukon date name")},
			  { 2, qc_("Beteng"   , "Balinese Pawukon date name")},
			  { 3, qc_("Kajeng"   , "Balinese Pawukon date name")}};
	caturwaraNames = {{ 1, qc_("Sri"      , "Balinese Pawukon date name")},
			  { 2, qc_("Laba"     , "Balinese Pawukon date name")},
			  { 3, qc_("Jaya"     , "Balinese Pawukon date name")},
			  { 4, qc_("Menala"   , "Balinese Pawukon date name")}};
	pancawaraNames = {{ 1, qc_("Umanis"   , "Balinese Pawukon date name")},
			  { 2, qc_("Paing"    , "Balinese Pawukon date name")},
			  { 3, qc_("Pon"      , "Balinese Pawukon date name")},
			  { 4, qc_("Wage"     , "Balinese Pawukon date name")},
			  { 5, qc_("Keliwon"  , "Balinese Pawukon date name")}};
	sadwaraNames   = {{ 1, qc_("Tungleh"  , "Balinese Pawukon date name")},
			  { 2, qc_("Aryang"   , "Balinese Pawukon date name")},
			  { 3, qc_("Urukung"  , "Balinese Pawukon date name")},
			  { 4, qc_("Paniron"  , "Balinese Pawukon date name")},
			  { 5, qc_("Was"      , "Balinese Pawukon date name")},
			  { 6, qc_("Maulu"    , "Balinese Pawukon date name")}};
	saptawaraNames = {{ 1, qc_("Redite"   , "Balinese Pawukon date name")},
			  { 2, qc_("Coma"     , "Balinese Pawukon date name")},
			  { 3, qc_("Anggara"  , "Balinese Pawukon date name")},
			  { 4, qc_("Buda"     , "Balinese Pawukon date name")},
			  { 5, qc_("Wraspati" , "Balinese Pawukon date name")},
			  { 6, qc_("Sukra"    , "Balinese Pawukon date name")},
			  { 7, qc_("Saniscara", "Balinese Pawukon date name")}};
	asatawaraNames = {{ 1, qc_("Sri"      , "Balinese Pawukon date name")},
			  { 2, qc_("Indra"    , "Balinese Pawukon date name")},
			  { 3, qc_("Guru"     , "Balinese Pawukon date name")},
			  { 4, qc_("Yama"     , "Balinese Pawukon date name")},
			  { 5, qc_("Ludra"    , "Balinese Pawukon date name")},
			  { 6, qc_("Brahma"   , "Balinese Pawukon date name")},
			  { 7, qc_("Kala"     , "Balinese Pawukon date name")},
			  { 8, qc_("Uma"      , "Balinese Pawukon date name")}};
	sangawaraNames = {{ 1, qc_("Dangu"    , "Balinese Pawukon date name")},
			  { 2, qc_("Jangur"   , "Balinese Pawukon date name")},
			  { 3, qc_("Gigis"    , "Balinese Pawukon date name")},
			  { 4, qc_("Nohan"    , "Balinese Pawukon date name")},
			  { 5, qc_("Ogan"     , "Balinese Pawukon date name")},
			  { 6, qc_("Erangan"  , "Balinese Pawukon date name")},
			  { 7, qc_("Urungan"  , "Balinese Pawukon date name")},
			  { 8, qc_("Tulus"    , "Balinese Pawukon date name")},
			  { 9, qc_("Dadi"     , "Balinese Pawukon date name")}};
	dasawaraNames  = {{ 1, qc_("Pandita"  , "Balinese Pawukon date name")},
			  { 2, qc_("Pati"     , "Balinese Pawukon date name")},
			  { 3, qc_("Suka"     , "Balinese Pawukon date name")},
			  { 4, qc_("Duka"     , "Balinese Pawukon date name")},
			  { 5, qc_("Sri"      , "Balinese Pawukon date name")},
			  { 6, qc_("Manuh"    , "Balinese Pawukon date name")},
			  { 7, qc_("Manusa"   , "Balinese Pawukon date name")},
			  { 8, qc_("Raja"     , "Balinese Pawukon date name")},
			  { 9, qc_("Dewa"     , "Balinese Pawukon date name")},
			  { 0, qc_("Raksasa"  , "Balinese Pawukon date name")}};
	weekNames      = {{ 1, qc_("Sinta"       , "Balinese Pawukon date name")},
			  { 2, qc_("Landep"      , "Balinese Pawukon date name")},
			  { 3, qc_("Ukir"        , "Balinese Pawukon date name")},
			  { 4, qc_("Kulantir"    , "Balinese Pawukon date name")},
			  { 5, qc_("Taulu"       , "Balinese Pawukon date name")},
			  { 6, qc_("Gumbreg"     , "Balinese Pawukon date name")},
			  { 7, qc_("Wariga"      , "Balinese Pawukon date name")},
			  { 8, qc_("Warigadian"  , "Balinese Pawukon date name")},
			  { 9, qc_("Jukungwangi" , "Balinese Pawukon date name")},
			  {10, qc_("Sungsang"    , "Balinese Pawukon date name")},
			  {11, qc_("Dunggulan"   , "Balinese Pawukon date name")},
			  {12, qc_("Kuningan"    , "Balinese Pawukon date name")},
			  {13, qc_("Langkir"     , "Balinese Pawukon date name")},
			  {14, qc_("Medangsia"   , "Balinese Pawukon date name")},
			  {15, qc_("Pujut"       , "Balinese Pawukon date name")},
			  {16, qc_("Pahang"      , "Balinese Pawukon date name")},
			  {17, qc_("Krulut"      , "Balinese Pawukon date name")},
			  {18, qc_("Merakih"     , "Balinese Pawukon date name")},
			  {19, qc_("Tambir"      , "Balinese Pawukon date name")},
			  {20, qc_("Medangkungan", "Balinese Pawukon date name")},
			  {21, qc_("Matal"       , "Balinese Pawukon date name")},
			  {22, qc_("Uye"         , "Balinese Pawukon date name")},
			  {23, qc_("Menail"      , "Balinese Pawukon date name")},
			  {24, qc_("Parangbakat" , "Balinese Pawukon date name")},
			  {25, qc_("Bala"        , "Balinese Pawukon date name")},
			  {26, qc_("Ugu"         , "Balinese Pawukon date name")},
			  {27, qc_("Wayang"      , "Balinese Pawukon date name")},
			  {28, qc_("Kelawu"      , "Balinese Pawukon date name")},
			  {29, qc_("Dukut"       , "Balinese Pawukon date name")},
			  {30, qc_("Watugunung"  , "Balinese Pawukon date name")}};
}

QMap<int, QString>BalinesePawukonCalendar::ekawaraNames;
QMap<int, QString>BalinesePawukonCalendar::dwiwaraNames;
QMap<int, QString>BalinesePawukonCalendar::triwaraNames;
QMap<int, QString>BalinesePawukonCalendar::caturwaraNames;
QMap<int, QString>BalinesePawukonCalendar::pancawaraNames;
QMap<int, QString>BalinesePawukonCalendar::sadwaraNames;
QMap<int, QString>BalinesePawukonCalendar::saptawaraNames;
QMap<int, QString>BalinesePawukonCalendar::asatawaraNames;
QMap<int, QString>BalinesePawukonCalendar::sangawaraNames;
QMap<int, QString>BalinesePawukonCalendar::dasawaraNames;
QMap<int, QString>BalinesePawukonCalendar::weekNames;

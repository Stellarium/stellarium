/*
 * Stellarium
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

#include "test/testCalendars.hpp"

#include <QString>
#include <QDebug>
#include <QtGlobal>
#include <QVariantList>
#include <QMap>

#include "StelUtils.hpp"
#include "../Calendar.hpp"
#include "../JulianCalendar.hpp"
#include "../GregorianCalendar.hpp"
#include "../ISOCalendar.hpp"
#include "../EgyptianCalendar.hpp"
#include "../ArmenianCalendar.hpp"
#include "../ZoroastrianCalendar.hpp"
#include "../CopticCalendar.hpp"
#include "../EthiopicCalendar.hpp"
#include "../IslamicCalendar.hpp"
#include "../HebrewCalendar.hpp"
#include "../OldHinduSolarCalendar.hpp"
#include "../OldHinduLuniSolarCalendar.hpp"
#include "../MayaLongCountCalendar.hpp"
#include "../MayaHaabCalendar.hpp"
#include "../MayaTzolkinCalendar.hpp"
#include "../AztecXihuitlCalendar.hpp"
#include "../AztecTonalpohualliCalendar.hpp"
#include "../BalinesePawukonCalendar.hpp"

QTEST_GUILESS_MAIN(TestCalendars)

QString printVec(QVector<int>vec)
{
	QString ret="{ ";
	for (int i = 0; i < vec.size(); ++i) {
		ret.append(QString::number(vec.at(i)));
		if (i<(vec.size()-1)) ret.append(", ");
	  }
	ret.append(" }");
	return ret;
}

void TestCalendars::testBasics()
{
	int h=100;
	QVector<int>hsplit=Calendar::toRadix(h, {4});
	QVERIFY2(hsplit==QVector<int>({25, 0}), qPrintable(QString("radix of %1 in {4} is %2")
							   .arg(h)
							   .arg(printVec(hsplit))));
	QVector<int>hsplit2=Calendar::toRadix(h, {5, 4});
	QVERIFY2(hsplit2==QVector<int>({5, 0, 0}), qPrintable(QString("radix of %1 in {5, 4} is %2")
							   .arg(h)
							   .arg(printVec(hsplit2))));
	QVERIFY2(Calendar::modInterval(5, 1, 5)==1, qPrintable(QString("modInterval(5, 1, 5)=%1").arg(QString::number(Calendar::modInterval(5, 1, 5)))));
	QVERIFY2(Calendar::modInterval(5, 0, 4)==1, qPrintable(QString("modInterval(5, 0, 4)=%1").arg(QString::number(Calendar::modInterval(5, 0, 4)))));
	QVERIFY2(Calendar::modInterval(5, 1, 3)==1, qPrintable(QString("modInterval(5, 1, 3)=%1").arg(QString::number(Calendar::modInterval(5, 1, 3)))));
	QVERIFY2(Calendar::modInterval(5, 0, 3)==2, qPrintable(QString("modInterval(5, 0, 3)=%1").arg(QString::number(Calendar::modInterval(5, 0, 3)))));
	QVERIFY2(Calendar::modInterval(5, 1, 2)==1, qPrintable(QString("modInterval(5, 1, 2)=%1").arg(QString::number(Calendar::modInterval(5, 1, 2)))));
	QVERIFY2(Calendar::modInterval(6, 1, 2)==1, qPrintable(QString("modInterval(6, 1, 2)=%1").arg(QString::number(Calendar::modInterval(6, 1, 2)))));
	// Critically important: modinterval(., 1, n)=amod(., n-1). n is NOT the maximum possible return value!
	QVERIFY2(Calendar::modInterval(42, 1, 7)==StelUtils::amod(42, 6), qPrintable(QString("modInterval(42, 1, 6)=%1 vs amod(42, 6)=%2").arg(QString::number(Calendar::modInterval(5, 1, 2))).arg(QString::number(StelUtils::amod(42, 6)))));
	QVERIFY2(Calendar::modInterval(43, 1, 7)==StelUtils::amod(43, 6), qPrintable(QString("modInterval(43, 1, 6)=%1 vs amod(43, 6)=%2").arg(QString::number(Calendar::modInterval(6, 1, 2))).arg(QString::number(StelUtils::amod(43, 6)))));
}

void TestCalendars::testEuropean()
{
	QVERIFY(GregorianCalendar::fixedFromGregorian({1582, 10, 14})==JulianCalendar::fixedFromJulian({1582, 10, 4}));
	QVERIFY(GregorianCalendar::fixedFromGregorian({1582, 10, 15})==JulianCalendar::fixedFromJulian({1582, 10, 5}));
	QVERIFY(GregorianCalendar::fixedFromGregorian({1, 1, 1})==GregorianCalendar::gregorianEpoch);
	QVERIFY(JulianCalendar::fixedFromJulian({1, 1, 1})==JulianCalendar::julianEpoch);
	QVERIFY(JulianCalendar::fixedFromJulian({1, 1, 1})==GregorianCalendar::fixedFromGregorian({0, JulianCalendar::december, 30}));
}

void TestCalendars::testNearEastern()
{
	QVERIFY2(EgyptianCalendar::egyptianEpoch==-272787,
		 qPrintable(QString("egyptianEpoch %1 vs %2")
			    .arg(EgyptianCalendar::egyptianEpoch)
			    .arg(-272787)));
	QVERIFY2(ArmenianCalendar::armenianEpoch==201443,
		 qPrintable(QString("armenianEpoch %1 vs %2")
			    .arg(ArmenianCalendar::armenianEpoch)
			    .arg(201443)));
	QVERIFY2(ZoroastrianCalendar::zoroastrianEpoch==230638,
		 qPrintable(QString("zoroastrianEpoch %1 vs %2")
			    .arg(ZoroastrianCalendar::zoroastrianEpoch)
			    .arg(230638)));
	QVERIFY2(CopticCalendar::copticEpoch==103605,
		 qPrintable(QString("copticEpoch %1 vs %2")
			    .arg(CopticCalendar::copticEpoch)
			    .arg(103605)));
	QVERIFY2(EthiopicCalendar::ethiopicEpoch==2796,
		 qPrintable(QString("ethiopicEpoch %1 vs %2")
			    .arg(EthiopicCalendar::ethiopicEpoch)
			    .arg(2796)));
}

void TestCalendars::testMesoamerican()
{
	QVERIFY2(GregorianCalendar::fixedFromGregorian({-3113, 8, 11}) == JulianCalendar::fixedFromJulian({-3114, 9, 6}),
		 qPrintable(QString("fixedFromGregorian %1 vs fixedFromJulian %2")
			    .arg(GregorianCalendar::fixedFromGregorian({-3113, 8, 11}))
			    .arg(JulianCalendar::fixedFromJulian({-3114, 9, 6}))  ));
	QVERIFY2(GregorianCalendar::fixedFromGregorian({-3113, 8, 11}) == -1137142,
		 qPrintable(QString("fixedFromGegorian %1 vs constant %2").arg(GregorianCalendar::fixedFromGregorian({-3113, 8, 11})).arg(-1137142)) );
	QVERIFY2(Calendar::fixedFromJD(584282.5, false) == JulianCalendar::fixedFromJulian({-3114, 9, 6}),
		 qPrintable(QString("fixedFromJD %1 vs fixedFromJulian %2")
			    .arg(Calendar::fixedFromJD(584282.5, false))
			    .arg(JulianCalendar::fixedFromJulian({-3114, 9, 6}))  ));
	const int mlc0=MayaLongCountCalendar::fixedFromMayanLongCount({0, 0, 0, 0, 0});
	QVERIFY2(mlc0 == GregorianCalendar::fixedFromGregorian({-3113, 8, 11}),
		 qPrintable(QString("MayaLongCount{0, 0, 0, 0, 0}=%1 vs fixedFromGregorian{-3113,8,11}=%2")
			    .arg(MayaLongCountCalendar::fixedFromMayanLongCount({0, 0, 0, 0, 0}))
			    .arg(GregorianCalendar::fixedFromGregorian({-3113, 8, 11}))  ));
	QVERIFY2(MayaLongCountCalendar::fixedFromMayanLongCount({7, 17, 18, 13, 2}) == 0,
		 qPrintable(QString("MayaLongCount %1 vs R.D. 0 = %2").arg("{7, 17, 18, 13, 2}").arg(0)  ));
	QVERIFY(MayaHaabCalendar::mayanHaabFromFixed(mlc0) == QVector<int>({18, 8}));
	QVERIFY(MayaTzolkinCalendar::mayanTzolkinFromFixed(mlc0) == QVector<int>({4, 20}));
	QVERIFY2(AztecXihuitlCalendar::aztecCorrelation==555403,
		 qPrintable(QString("aztecCorrelation %1 vs %2")
			    .arg(AztecXihuitlCalendar::aztecCorrelation)
			    .arg(555403)));
	QVERIFY2(AztecXihuitlCalendar::aztecXihuitlCorrelation==555202,
		 qPrintable(QString("aztecXihuitlCorrelation %1 vs %2")
			    .arg(AztecXihuitlCalendar::aztecXihuitlCorrelation)
			    .arg(555202)));
	QVERIFY2(AztecTonalpohualliCalendar::aztecTonalpohualliCorrelation==555299,
		 qPrintable(QString("aztecTonalpohualliCorrelation %1 vs %2")
			    .arg(AztecTonalpohualliCalendar::aztecTonalpohualliCorrelation)
			    .arg(555299)));
}

void TestCalendars::testIslamic()
{
	QVERIFY(-214193==IslamicCalendar::fixedFromIslamic({-1245, 12, 9}));
	QVERIFY( -61387==IslamicCalendar::fixedFromIslamic({-813,  2, 23}));
	QVERIFY(  25469==IslamicCalendar::fixedFromIslamic({-568,  4,  1}));
	QVERIFY(  49217==IslamicCalendar::fixedFromIslamic({-501,  4,  6}));
	QVERIFY( 171307==IslamicCalendar::fixedFromIslamic({-157, 10, 17}));
	QVERIFY( 210155==IslamicCalendar::fixedFromIslamic({ -47,  6,  3}));
	QVERIFY( 253427==IslamicCalendar::fixedFromIslamic({  75,  7, 13}));
	QVERIFY( 369740==IslamicCalendar::fixedFromIslamic({ 403, 10,  5}));
	QVERIFY( 400085==IslamicCalendar::fixedFromIslamic({ 489,  5, 22}));
	QVERIFY( 434355==IslamicCalendar::fixedFromIslamic({ 586,  2,  7}));
	QVERIFY( 452605==IslamicCalendar::fixedFromIslamic({ 637,  8,  7}));
	QVERIFY( 470160==IslamicCalendar::fixedFromIslamic({ 687,  2, 20}));
	QVERIFY( 473837==IslamicCalendar::fixedFromIslamic({ 697,  7,  7}));
	QVERIFY( 507850==IslamicCalendar::fixedFromIslamic({ 793,  7,  1}));
	QVERIFY( 524156==IslamicCalendar::fixedFromIslamic({ 839,  7,  6}));
	QVERIFY( 544676==IslamicCalendar::fixedFromIslamic({ 897,  6,  1}));
	QVERIFY( 567118==IslamicCalendar::fixedFromIslamic({ 960,  9, 30}));
	QVERIFY( 569477==IslamicCalendar::fixedFromIslamic({ 967,  5, 27}));
	QVERIFY( 601716==IslamicCalendar::fixedFromIslamic({1058,  5, 18}));
	QVERIFY( 613424==IslamicCalendar::fixedFromIslamic({1091,  6,  2}));
	QVERIFY( 626596==IslamicCalendar::fixedFromIslamic({1128,  8,  4}));
	QVERIFY( 645554==IslamicCalendar::fixedFromIslamic({1182,  2,  3}));
	QVERIFY( 664224==IslamicCalendar::fixedFromIslamic({1234, 10, 10}));
	QVERIFY( 671401==IslamicCalendar::fixedFromIslamic({1255,  1, 11}));
	QVERIFY( 694799==IslamicCalendar::fixedFromIslamic({1321,  1, 21}));
	QVERIFY( 704424==IslamicCalendar::fixedFromIslamic({1348,  3, 19}));
	QVERIFY( 708842==IslamicCalendar::fixedFromIslamic({1360,  9,  8}));
	QVERIFY( 709409==IslamicCalendar::fixedFromIslamic({1362,  4, 13}));
	QVERIFY( 709580==IslamicCalendar::fixedFromIslamic({1362, 10,  7}));
	QVERIFY( 727274==IslamicCalendar::fixedFromIslamic({1412,  9, 13}));
	QVERIFY( 728714==IslamicCalendar::fixedFromIslamic({1416, 10,  5}));
	QVERIFY( 744313==IslamicCalendar::fixedFromIslamic({1460, 10, 12}));
	QVERIFY( 764652==IslamicCalendar::fixedFromIslamic({1518,  3,  5}));

	QVERIFY(IslamicCalendar::islamicFromFixed(-214193)==QVector<int>({-1245, 12, 9}));
	QVERIFY(IslamicCalendar::islamicFromFixed( -61387)==QVector<int>({-813,  2, 23}));
	QVERIFY(IslamicCalendar::islamicFromFixed(  25469)==QVector<int>({-568,  4,  1}));
	QVERIFY(IslamicCalendar::islamicFromFixed(  49217)==QVector<int>({-501,  4,  6}));
	QVERIFY(IslamicCalendar::islamicFromFixed( 171307)==QVector<int>({-157, 10, 17}));
	QVERIFY(IslamicCalendar::islamicFromFixed( 210155)==QVector<int>({ -47,  6,  3}));
	QVERIFY(IslamicCalendar::islamicFromFixed( 253427)==QVector<int>({  75,  7, 13}));
	QVERIFY(IslamicCalendar::islamicFromFixed( 369740)==QVector<int>({ 403, 10,  5}));
	QVERIFY(IslamicCalendar::islamicFromFixed( 400085)==QVector<int>({ 489,  5, 22}));
	QVERIFY(IslamicCalendar::islamicFromFixed( 434355)==QVector<int>({ 586,  2,  7}));
	QVERIFY(IslamicCalendar::islamicFromFixed( 452605)==QVector<int>({ 637,  8,  7}));
	QVERIFY(IslamicCalendar::islamicFromFixed( 470160)==QVector<int>({ 687,  2, 20}));
	QVERIFY(IslamicCalendar::islamicFromFixed( 473837)==QVector<int>({ 697,  7,  7}));
	QVERIFY(IslamicCalendar::islamicFromFixed( 507850)==QVector<int>({ 793,  7,  1}));
	QVERIFY(IslamicCalendar::islamicFromFixed( 524156)==QVector<int>({ 839,  7,  6}));
	QVERIFY(IslamicCalendar::islamicFromFixed( 544676)==QVector<int>({ 897,  6,  1}));
	QVERIFY(IslamicCalendar::islamicFromFixed( 567118)==QVector<int>({ 960,  9, 30}));
	QVERIFY(IslamicCalendar::islamicFromFixed( 569477)==QVector<int>({ 967,  5, 27}));
	QVERIFY(IslamicCalendar::islamicFromFixed( 601716)==QVector<int>({1058,  5, 18}));
	QVERIFY(IslamicCalendar::islamicFromFixed( 613424)==QVector<int>({1091,  6,  2}));
	QVERIFY(IslamicCalendar::islamicFromFixed( 626596)==QVector<int>({1128,  8,  4}));
	QVERIFY(IslamicCalendar::islamicFromFixed( 645554)==QVector<int>({1182,  2,  3}));
	QVERIFY(IslamicCalendar::islamicFromFixed( 664224)==QVector<int>({1234, 10, 10}));
	QVERIFY(IslamicCalendar::islamicFromFixed( 671401)==QVector<int>({1255,  1, 11}));
	QVERIFY(IslamicCalendar::islamicFromFixed( 694799)==QVector<int>({1321,  1, 21}));
	QVERIFY(IslamicCalendar::islamicFromFixed( 704424)==QVector<int>({1348,  3, 19}));
	QVERIFY(IslamicCalendar::islamicFromFixed( 708842)==QVector<int>({1360,  9,  8}));
	QVERIFY(IslamicCalendar::islamicFromFixed( 709409)==QVector<int>({1362,  4, 13}));
	QVERIFY(IslamicCalendar::islamicFromFixed( 709580)==QVector<int>({1362, 10,  7}));
	QVERIFY(IslamicCalendar::islamicFromFixed( 727274)==QVector<int>({1412,  9, 13}));
	QVERIFY(IslamicCalendar::islamicFromFixed( 728714)==QVector<int>({1416, 10,  5}));
	QVERIFY(IslamicCalendar::islamicFromFixed( 744313)==QVector<int>({1460, 10, 12}));
	QVERIFY(IslamicCalendar::islamicFromFixed( 764652)==QVector<int>({1518,  3,  5}));
}

void TestCalendars::testHebrew()
{
	QVERIFY(-214193==HebrewCalendar::fixedFromHebrew({3174,  5, 10}));
	QVERIFY( -61387==HebrewCalendar::fixedFromHebrew({3593,  9, 25}));
	QVERIFY(  25469==HebrewCalendar::fixedFromHebrew({3831,  7,  3}));
	QVERIFY(  49217==HebrewCalendar::fixedFromHebrew({3896,  7,  9}));
	QVERIFY( 171307==HebrewCalendar::fixedFromHebrew({4230, 10, 18}));
	QVERIFY( 210155==HebrewCalendar::fixedFromHebrew({4336,  3,  4}));
	QVERIFY( 253427==HebrewCalendar::fixedFromHebrew({4455,  8, 13}));
	QVERIFY( 369740==HebrewCalendar::fixedFromHebrew({4773,  2,  6}));
	QVERIFY( 400085==HebrewCalendar::fixedFromHebrew({4856,  2, 23}));
	QVERIFY( 434355==HebrewCalendar::fixedFromHebrew({4950,  1,  7}));
	QVERIFY( 452605==HebrewCalendar::fixedFromHebrew({5000, 13,  8}));
	QVERIFY( 470160==HebrewCalendar::fixedFromHebrew({5048,  1, 21}));
	QVERIFY( 473837==HebrewCalendar::fixedFromHebrew({5058,  2,  7}));
	QVERIFY( 507850==HebrewCalendar::fixedFromHebrew({5151,  4,  1}));
	QVERIFY( 524156==HebrewCalendar::fixedFromHebrew({5196, 11,  7}));
	QVERIFY( 544676==HebrewCalendar::fixedFromHebrew({5252,  1,  3}));
	QVERIFY( 567118==HebrewCalendar::fixedFromHebrew({5314,  7,  1}));
	QVERIFY( 569477==HebrewCalendar::fixedFromHebrew({5320, 12, 27}));
	QVERIFY( 601716==HebrewCalendar::fixedFromHebrew({5408,  3, 20}));
	QVERIFY( 613424==HebrewCalendar::fixedFromHebrew({5440,  4,  3}));
	QVERIFY( 626596==HebrewCalendar::fixedFromHebrew({5476,  5,  5}));
	QVERIFY( 645554==HebrewCalendar::fixedFromHebrew({5528,  4,  4}));
	QVERIFY( 664224==HebrewCalendar::fixedFromHebrew({5579,  5, 11}));
	QVERIFY( 671401==HebrewCalendar::fixedFromHebrew({5599,  1, 12}));
	QVERIFY( 694799==HebrewCalendar::fixedFromHebrew({5663,  1, 22}));
	QVERIFY( 704424==HebrewCalendar::fixedFromHebrew({5689,  5, 19}));
	QVERIFY( 708842==HebrewCalendar::fixedFromHebrew({5702,  7,  8}));
	QVERIFY( 709409==HebrewCalendar::fixedFromHebrew({5703,  1, 14}));
	QVERIFY( 709580==HebrewCalendar::fixedFromHebrew({5704,  7,  8}));
	QVERIFY( 727274==HebrewCalendar::fixedFromHebrew({5752, 13, 12}));
	QVERIFY( 728714==HebrewCalendar::fixedFromHebrew({5756, 12,  5}));
	QVERIFY( 744313==HebrewCalendar::fixedFromHebrew({5799,  8, 12}));
	QVERIFY( 764652==HebrewCalendar::fixedFromHebrew({5854,  5,  5}));

	QVERIFY2(HebrewCalendar::hebrewFromFixed(-214193)==QVector<int>({3174,  5, 10}),
		 qPrintable(QString("hebrewFromFixed %1 vs constant %2").arg(printVec(HebrewCalendar::hebrewFromFixed(-214193))).arg("{3174,  5, 10}")) );
	QVERIFY(HebrewCalendar::hebrewFromFixed(-214193)==QVector<int>({3174,  5, 10}));
	QVERIFY(HebrewCalendar::hebrewFromFixed( -61387)==QVector<int>({3593,  9, 25}));
	QVERIFY(HebrewCalendar::hebrewFromFixed(  25469)==QVector<int>({3831,  7,  3}));
	QVERIFY(HebrewCalendar::hebrewFromFixed(  49217)==QVector<int>({3896,  7,  9}));
	QVERIFY(HebrewCalendar::hebrewFromFixed( 171307)==QVector<int>({4230, 10, 18}));
	QVERIFY(HebrewCalendar::hebrewFromFixed( 210155)==QVector<int>({4336,  3,  4}));
	QVERIFY(HebrewCalendar::hebrewFromFixed( 253427)==QVector<int>({4455,  8, 13}));
	QVERIFY(HebrewCalendar::hebrewFromFixed( 369740)==QVector<int>({4773,  2,  6}));
	QVERIFY(HebrewCalendar::hebrewFromFixed( 400085)==QVector<int>({4856,  2, 23}));
	QVERIFY(HebrewCalendar::hebrewFromFixed( 434355)==QVector<int>({4950,  1,  7}));
	QVERIFY(HebrewCalendar::hebrewFromFixed( 452605)==QVector<int>({5000, 13,  8}));
	QVERIFY(HebrewCalendar::hebrewFromFixed( 470160)==QVector<int>({5048,  1, 21}));
	QVERIFY(HebrewCalendar::hebrewFromFixed( 473837)==QVector<int>({5058,  2,  7}));
	QVERIFY(HebrewCalendar::hebrewFromFixed( 507850)==QVector<int>({5151,  4,  1}));
	QVERIFY(HebrewCalendar::hebrewFromFixed( 524156)==QVector<int>({5196, 11,  7}));
	QVERIFY(HebrewCalendar::hebrewFromFixed( 544676)==QVector<int>({5252,  1,  3}));
	QVERIFY(HebrewCalendar::hebrewFromFixed( 567118)==QVector<int>({5314,  7,  1}));
	QVERIFY(HebrewCalendar::hebrewFromFixed( 569477)==QVector<int>({5320, 12, 27}));
	QVERIFY(HebrewCalendar::hebrewFromFixed( 601716)==QVector<int>({5408,  3, 20}));
	QVERIFY(HebrewCalendar::hebrewFromFixed( 613424)==QVector<int>({5440,  4,  3}));
	QVERIFY(HebrewCalendar::hebrewFromFixed( 626596)==QVector<int>({5476,  5,  5}));
	QVERIFY(HebrewCalendar::hebrewFromFixed( 645554)==QVector<int>({5528,  4,  4}));
	QVERIFY(HebrewCalendar::hebrewFromFixed( 664224)==QVector<int>({5579,  5, 11}));
	QVERIFY(HebrewCalendar::hebrewFromFixed( 671401)==QVector<int>({5599,  1, 12}));
	QVERIFY(HebrewCalendar::hebrewFromFixed( 694799)==QVector<int>({5663,  1, 22}));
	QVERIFY(HebrewCalendar::hebrewFromFixed( 704424)==QVector<int>({5689,  5, 19}));
	QVERIFY(HebrewCalendar::hebrewFromFixed( 708842)==QVector<int>({5702,  7,  8}));
	QVERIFY(HebrewCalendar::hebrewFromFixed( 709409)==QVector<int>({5703,  1, 14}));
	QVERIFY(HebrewCalendar::hebrewFromFixed( 709580)==QVector<int>({5704,  7,  8}));
	QVERIFY(HebrewCalendar::hebrewFromFixed( 727274)==QVector<int>({5752, 13, 12}));
	QVERIFY(HebrewCalendar::hebrewFromFixed( 728714)==QVector<int>({5756, 12,  5}));
	QVERIFY(HebrewCalendar::hebrewFromFixed( 744313)==QVector<int>({5799,  8, 12}));
	QVERIFY(HebrewCalendar::hebrewFromFixed( 764652)==QVector<int>({5854,  5,  5}));
}

void TestCalendars::testBalinesePawukon()
{
	QVERIFY2(BalinesePawukonCalendar::baliEpoch==Calendar::fixedFromJD(146, false),
		 qPrintable(QString("BaliEpoch %1 vs %2")
			    .arg(BalinesePawukonCalendar::baliEpoch)
			    .arg(Calendar::fixedFromJD(146, false))));

	const int sample2038 = GregorianCalendar::fixedFromGregorian({2038, 11, 10});
	const QVector<int>expect2038({1, 2, 1, 3, 4, 1, 4, 7, 1, 2});

	QVERIFY2(BalinesePawukonCalendar::baliPawukonFromFixed(sample2038) == expect2038,
		 qPrintable(QString("Bali for %1 = %2").arg(QString::number(sample2038))
			    .arg(printVec(BalinesePawukonCalendar::baliPawukonFromFixed(sample2038)))));

	QVERIFY(-214193==BalinesePawukonCalendar::baliOnOrBefore({0, 1, 1, 1, 3, 1, 1, 5, 7, 3}, -214193));
	QVERIFY( -61387==BalinesePawukonCalendar::baliOnOrBefore({1, 2, 2, 1, 4, 5, 4, 5, 5, 2},  -61387));
	QVERIFY(  25469==BalinesePawukonCalendar::baliOnOrBefore({1, 2, 2, 1, 5, 5, 4, 1, 5, 6},   25469));
	QVERIFY(  49217==BalinesePawukonCalendar::baliOnOrBefore({0, 1, 2, 3, 3, 5, 1, 3, 5, 3},   49217));
	QVERIFY( 171307==BalinesePawukonCalendar::baliOnOrBefore({0, 1, 1, 3, 3, 1, 4, 3, 1, 5},  171307));
	QVERIFY( 210155==BalinesePawukonCalendar::baliOnOrBefore({1, 2, 2, 1, 1, 5, 2, 1, 8, 0},  210155));
	QVERIFY( 253427==BalinesePawukonCalendar::baliOnOrBefore({0, 1, 2, 3, 3, 5, 7, 3, 2, 7},  253427));
	QVERIFY( 369740==BalinesePawukonCalendar::baliOnOrBefore({0, 1, 2, 2, 1, 2, 1, 2, 2, 1},  369740));
	QVERIFY( 400085==BalinesePawukonCalendar::baliOnOrBefore({0, 1, 2, 1, 1, 5, 1, 1, 8, 1},  400085));
	QVERIFY( 434355==BalinesePawukonCalendar::baliOnOrBefore({1, 2, 3, 1, 1, 3, 6, 1, 3, 2},  434355));
	QVERIFY( 452605==BalinesePawukonCalendar::baliOnOrBefore({0, 1, 1, 1, 1, 1, 7, 5, 1, 5},  452605));
	QVERIFY( 470160==BalinesePawukonCalendar::baliOnOrBefore({1, 2, 3, 4, 1, 6, 6, 8, 6, 2},  470160));
	QVERIFY( 473837==BalinesePawukonCalendar::baliOnOrBefore({0, 1, 2, 3, 3, 5, 1, 3, 5, 3},  473837));
	QVERIFY( 507850==BalinesePawukonCalendar::baliOnOrBefore({0, 1, 1, 4, 1, 4, 1, 4, 7, 1},  507850));
	QVERIFY( 524156==BalinesePawukonCalendar::baliOnOrBefore({0, 1, 2, 2, 2, 2, 4, 2, 5, 7},  524156));
	QVERIFY( 544676==BalinesePawukonCalendar::baliOnOrBefore({0, 1, 2, 4, 2, 2, 7, 8, 8, 9},  544676));
	QVERIFY( 567118==BalinesePawukonCalendar::baliOnOrBefore({1, 2, 1, 4, 4, 4, 7, 4, 7, 4},  567118));
	QVERIFY( 569477==BalinesePawukonCalendar::baliOnOrBefore({0, 1, 2, 3, 3, 5, 7, 3, 2, 7},  569477));
	QVERIFY( 601716==BalinesePawukonCalendar::baliOnOrBefore({0, 1, 3, 4, 2, 6, 4, 8, 3, 7},  601716));
	QVERIFY( 613424==BalinesePawukonCalendar::baliOnOrBefore({1, 2, 2, 4, 5, 2, 1, 4, 5, 4},  613424));
	QVERIFY( 626596==BalinesePawukonCalendar::baliOnOrBefore({1, 2, 1, 2, 2, 4, 6, 2, 1, 6},  626596));
	QVERIFY( 645554==BalinesePawukonCalendar::baliOnOrBefore({1, 2, 2, 4, 5, 2, 1, 4, 5, 4},  645554));
	QVERIFY( 664224==BalinesePawukonCalendar::baliOnOrBefore({0, 1, 3, 4, 5, 6, 2, 8, 3, 3},  664224));
	QVERIFY( 671401==BalinesePawukonCalendar::baliOnOrBefore({0, 1, 1, 1, 2, 1, 4, 5, 4, 7},  671401));
	QVERIFY( 694799==BalinesePawukonCalendar::baliOnOrBefore({1, 2, 2, 1, 5, 5, 1, 5, 8, 4},  694799));
	QVERIFY( 704424==BalinesePawukonCalendar::baliOnOrBefore({1, 2, 3, 2, 5, 6, 1, 2, 3, 4},  704424));
	QVERIFY( 708842==BalinesePawukonCalendar::baliOnOrBefore({1, 2, 2, 2, 3, 2, 2, 2, 1, 2},  708842));
	QVERIFY( 709409==BalinesePawukonCalendar::baliOnOrBefore({0, 1, 2, 3, 5, 5, 2, 3, 2, 3},  709409));
	QVERIFY( 709580==BalinesePawukonCalendar::baliOnOrBefore({1, 2, 2, 4, 1, 2, 5, 4, 8, 4},  709580));
	QVERIFY( 727274==BalinesePawukonCalendar::baliOnOrBefore({1, 2, 2, 2, 5, 2, 3, 2, 8, 2},  727274));
	QVERIFY( 728714==BalinesePawukonCalendar::baliOnOrBefore({1, 2, 2, 4, 5, 2, 1, 4, 5, 4},  728714));
	QVERIFY( 744313==BalinesePawukonCalendar::baliOnOrBefore({1, 2, 1, 3, 4, 1, 4, 7, 1, 2},  744313));
	QVERIFY( 764652==BalinesePawukonCalendar::baliOnOrBefore({0, 1, 3, 4, 3, 6, 1, 8, 6, 3},  764652));

	QVERIFY(BalinesePawukonCalendar::baliPawukonFromFixed(-214193)==QVector<int>({0, 1, 1, 1, 3, 1, 1, 5, 7, 3}));
	QVERIFY(BalinesePawukonCalendar::baliPawukonFromFixed( -61387)==QVector<int>({1, 2, 2, 1, 4, 5, 4, 5, 5, 2}));
	QVERIFY(BalinesePawukonCalendar::baliPawukonFromFixed(  25469)==QVector<int>({1, 2, 2, 1, 5, 5, 4, 1, 5, 6}));
	QVERIFY(BalinesePawukonCalendar::baliPawukonFromFixed(  49217)==QVector<int>({0, 1, 2, 3, 3, 5, 1, 3, 5, 3}));
	QVERIFY(BalinesePawukonCalendar::baliPawukonFromFixed( 171307)==QVector<int>({0, 1, 1, 3, 3, 1, 4, 3, 1, 5}));
	QVERIFY(BalinesePawukonCalendar::baliPawukonFromFixed( 210155)==QVector<int>({1, 2, 2, 1, 1, 5, 2, 1, 8, 0}));
	QVERIFY(BalinesePawukonCalendar::baliPawukonFromFixed( 253427)==QVector<int>({0, 1, 2, 3, 3, 5, 7, 3, 2, 7}));
	QVERIFY(BalinesePawukonCalendar::baliPawukonFromFixed( 369740)==QVector<int>({0, 1, 2, 2, 1, 2, 1, 2, 2, 1}));
	QVERIFY(BalinesePawukonCalendar::baliPawukonFromFixed( 400085)==QVector<int>({0, 1, 2, 1, 1, 5, 1, 1, 8, 1}));
	QVERIFY(BalinesePawukonCalendar::baliPawukonFromFixed( 434355)==QVector<int>({1, 2, 3, 1, 1, 3, 6, 1, 3, 2}));
	QVERIFY(BalinesePawukonCalendar::baliPawukonFromFixed( 452605)==QVector<int>({0, 1, 1, 1, 1, 1, 7, 5, 1, 5}));
	QVERIFY(BalinesePawukonCalendar::baliPawukonFromFixed( 470160)==QVector<int>({1, 2, 3, 4, 1, 6, 6, 8, 6, 2}));
	QVERIFY(BalinesePawukonCalendar::baliPawukonFromFixed( 473837)==QVector<int>({0, 1, 2, 3, 3, 5, 1, 3, 5, 3}));
	QVERIFY(BalinesePawukonCalendar::baliPawukonFromFixed( 507850)==QVector<int>({0, 1, 1, 4, 1, 4, 1, 4, 7, 1}));
	QVERIFY(BalinesePawukonCalendar::baliPawukonFromFixed( 524156)==QVector<int>({0, 1, 2, 2, 2, 2, 4, 2, 5, 7}));
	QVERIFY(BalinesePawukonCalendar::baliPawukonFromFixed( 544676)==QVector<int>({0, 1, 2, 4, 2, 2, 7, 8, 8, 9}));
	QVERIFY(BalinesePawukonCalendar::baliPawukonFromFixed( 567118)==QVector<int>({1, 2, 1, 4, 4, 4, 7, 4, 7, 4}));
	QVERIFY(BalinesePawukonCalendar::baliPawukonFromFixed( 569477)==QVector<int>({0, 1, 2, 3, 3, 5, 7, 3, 2, 7}));
	QVERIFY(BalinesePawukonCalendar::baliPawukonFromFixed( 601716)==QVector<int>({0, 1, 3, 4, 2, 6, 4, 8, 3, 7}));
	QVERIFY(BalinesePawukonCalendar::baliPawukonFromFixed( 613424)==QVector<int>({1, 2, 2, 4, 5, 2, 1, 4, 5, 4}));
	QVERIFY(BalinesePawukonCalendar::baliPawukonFromFixed( 626596)==QVector<int>({1, 2, 1, 2, 2, 4, 6, 2, 1, 6}));
	QVERIFY(BalinesePawukonCalendar::baliPawukonFromFixed( 645554)==QVector<int>({1, 2, 2, 4, 5, 2, 1, 4, 5, 4}));
	QVERIFY(BalinesePawukonCalendar::baliPawukonFromFixed( 664224)==QVector<int>({0, 1, 3, 4, 5, 6, 2, 8, 3, 3}));
	QVERIFY(BalinesePawukonCalendar::baliPawukonFromFixed( 671401)==QVector<int>({0, 1, 1, 1, 2, 1, 4, 5, 4, 7}));
	QVERIFY(BalinesePawukonCalendar::baliPawukonFromFixed( 694799)==QVector<int>({1, 2, 2, 1, 5, 5, 1, 5, 8, 4}));
	QVERIFY(BalinesePawukonCalendar::baliPawukonFromFixed( 704424)==QVector<int>({1, 2, 3, 2, 5, 6, 1, 2, 3, 4}));
	QVERIFY(BalinesePawukonCalendar::baliPawukonFromFixed( 708842)==QVector<int>({1, 2, 2, 2, 3, 2, 2, 2, 1, 2}));
	QVERIFY(BalinesePawukonCalendar::baliPawukonFromFixed( 709409)==QVector<int>({0, 1, 2, 3, 5, 5, 2, 3, 2, 3}));
	QVERIFY(BalinesePawukonCalendar::baliPawukonFromFixed( 709580)==QVector<int>({1, 2, 2, 4, 1, 2, 5, 4, 8, 4}));
	QVERIFY(BalinesePawukonCalendar::baliPawukonFromFixed( 727274)==QVector<int>({1, 2, 2, 2, 5, 2, 3, 2, 8, 2}));
	QVERIFY(BalinesePawukonCalendar::baliPawukonFromFixed( 728714)==QVector<int>({1, 2, 2, 4, 5, 2, 1, 4, 5, 4}));
	QVERIFY(BalinesePawukonCalendar::baliPawukonFromFixed( 744313)==QVector<int>({1, 2, 1, 3, 4, 1, 4, 7, 1, 2}));
	QVERIFY(BalinesePawukonCalendar::baliPawukonFromFixed( 764652)==QVector<int>({0, 1, 3, 4, 3, 6, 1, 8, 6, 3}));
}

void TestCalendars::testOldHindu()
{
	QVERIFY(-214193==OldHinduSolarCalendar::fixedFromOldHinduSolar({2515,  5, 19}));
	QVERIFY( -61387==OldHinduSolarCalendar::fixedFromOldHinduSolar({2933,  9, 26}));
	QVERIFY(  25469==OldHinduSolarCalendar::fixedFromOldHinduSolar({3171,  7, 11}));
	QVERIFY(  49217==OldHinduSolarCalendar::fixedFromOldHinduSolar({3236,  7, 17}));
	QVERIFY( 171307==OldHinduSolarCalendar::fixedFromOldHinduSolar({3570, 10, 19}));
	QVERIFY( 210155==OldHinduSolarCalendar::fixedFromOldHinduSolar({3677,  2, 28}));
	QVERIFY( 253427==OldHinduSolarCalendar::fixedFromOldHinduSolar({3795,  8, 17}));
	QVERIFY( 369740==OldHinduSolarCalendar::fixedFromOldHinduSolar({4114,  1, 26}));
	QVERIFY( 400085==OldHinduSolarCalendar::fixedFromOldHinduSolar({4197,  2, 24}));
	QVERIFY( 434355==OldHinduSolarCalendar::fixedFromOldHinduSolar({4290, 12, 20}));
	QVERIFY( 452605==OldHinduSolarCalendar::fixedFromOldHinduSolar({4340, 12,  7}));
	QVERIFY( 470160==OldHinduSolarCalendar::fixedFromOldHinduSolar({4388, 12, 30}));
	QVERIFY( 473837==OldHinduSolarCalendar::fixedFromOldHinduSolar({4399,  1, 24}));
	QVERIFY( 507850==OldHinduSolarCalendar::fixedFromOldHinduSolar({4492,  3,  7}));
	QVERIFY( 524156==OldHinduSolarCalendar::fixedFromOldHinduSolar({4536, 10, 28}));
	QVERIFY( 544676==OldHinduSolarCalendar::fixedFromOldHinduSolar({4593,  1,  3}));
	QVERIFY( 567118==OldHinduSolarCalendar::fixedFromOldHinduSolar({4654,  6, 12}));
	QVERIFY( 569477==OldHinduSolarCalendar::fixedFromOldHinduSolar({4660, 11, 27}));
	QVERIFY( 601716==OldHinduSolarCalendar::fixedFromOldHinduSolar({4749,  3,  1}));
	QVERIFY( 613424==OldHinduSolarCalendar::fixedFromOldHinduSolar({4781,  3, 21}));
	QVERIFY( 626596==OldHinduSolarCalendar::fixedFromOldHinduSolar({4817,  4, 13}));
	QVERIFY( 645554==OldHinduSolarCalendar::fixedFromOldHinduSolar({4869,  3,  8}));
	QVERIFY( 664224==OldHinduSolarCalendar::fixedFromOldHinduSolar({4920,  4, 20}));
	QVERIFY( 671401==OldHinduSolarCalendar::fixedFromOldHinduSolar({4939, 12, 13}));
	QVERIFY( 694799==OldHinduSolarCalendar::fixedFromOldHinduSolar({5004,  1,  4}));
	QVERIFY( 704424==OldHinduSolarCalendar::fixedFromOldHinduSolar({5030,  5, 11}));
	QVERIFY( 708842==OldHinduSolarCalendar::fixedFromOldHinduSolar({5042,  6, 15}));
	QVERIFY( 709409==OldHinduSolarCalendar::fixedFromOldHinduSolar({5044,  1,  4}));
	QVERIFY( 709580==OldHinduSolarCalendar::fixedFromOldHinduSolar({5044,  6, 23}));
	QVERIFY( 727274==OldHinduSolarCalendar::fixedFromOldHinduSolar({5092, 12,  2}));
	QVERIFY( 728714==OldHinduSolarCalendar::fixedFromOldHinduSolar({5096, 11, 11}));
	QVERIFY( 744313==OldHinduSolarCalendar::fixedFromOldHinduSolar({5139,  7, 26}));
	QVERIFY( 764652==OldHinduSolarCalendar::fixedFromOldHinduSolar({5195,  4,  2}));

	QVERIFY(OldHinduSolarCalendar::oldHinduSolarFromFixed(-214193)==QVector<int>({2515,  5, 19}));
	QVERIFY(OldHinduSolarCalendar::oldHinduSolarFromFixed( -61387)==QVector<int>({2933,  9, 26}));
	QVERIFY(OldHinduSolarCalendar::oldHinduSolarFromFixed(  25469)==QVector<int>({3171,  7, 11}));
	QVERIFY(OldHinduSolarCalendar::oldHinduSolarFromFixed(  49217)==QVector<int>({3236,  7, 17}));
	QVERIFY(OldHinduSolarCalendar::oldHinduSolarFromFixed( 171307)==QVector<int>({3570, 10, 19}));
	QVERIFY(OldHinduSolarCalendar::oldHinduSolarFromFixed( 210155)==QVector<int>({3677,  2, 28}));
	QVERIFY(OldHinduSolarCalendar::oldHinduSolarFromFixed( 253427)==QVector<int>({3795,  8, 17}));
	QVERIFY(OldHinduSolarCalendar::oldHinduSolarFromFixed( 369740)==QVector<int>({4114,  1, 26}));
	QVERIFY(OldHinduSolarCalendar::oldHinduSolarFromFixed( 400085)==QVector<int>({4197,  2, 24}));
	QVERIFY(OldHinduSolarCalendar::oldHinduSolarFromFixed( 434355)==QVector<int>({4290, 12, 20}));
	QVERIFY(OldHinduSolarCalendar::oldHinduSolarFromFixed( 452605)==QVector<int>({4340, 12,  7}));
	QVERIFY(OldHinduSolarCalendar::oldHinduSolarFromFixed( 470160)==QVector<int>({4388, 12, 30}));
	QVERIFY(OldHinduSolarCalendar::oldHinduSolarFromFixed( 473837)==QVector<int>({4399,  1, 24}));
	QVERIFY(OldHinduSolarCalendar::oldHinduSolarFromFixed( 507850)==QVector<int>({4492,  3,  7}));
	QVERIFY(OldHinduSolarCalendar::oldHinduSolarFromFixed( 524156)==QVector<int>({4536, 10, 28}));
	QVERIFY(OldHinduSolarCalendar::oldHinduSolarFromFixed( 544676)==QVector<int>({4593,  1,  3}));
	QVERIFY(OldHinduSolarCalendar::oldHinduSolarFromFixed( 567118)==QVector<int>({4654,  6, 12}));
	QVERIFY(OldHinduSolarCalendar::oldHinduSolarFromFixed( 569477)==QVector<int>({4660, 11, 27}));
	QVERIFY(OldHinduSolarCalendar::oldHinduSolarFromFixed( 601716)==QVector<int>({4749,  3,  1}));
	QVERIFY(OldHinduSolarCalendar::oldHinduSolarFromFixed( 613424)==QVector<int>({4781,  3, 21}));
	QVERIFY(OldHinduSolarCalendar::oldHinduSolarFromFixed( 626596)==QVector<int>({4817,  4, 13}));
	QVERIFY(OldHinduSolarCalendar::oldHinduSolarFromFixed( 645554)==QVector<int>({4869,  3,  8}));
	QVERIFY(OldHinduSolarCalendar::oldHinduSolarFromFixed( 664224)==QVector<int>({4920,  4, 20}));
	QVERIFY(OldHinduSolarCalendar::oldHinduSolarFromFixed( 671401)==QVector<int>({4939, 12, 13}));
	QVERIFY(OldHinduSolarCalendar::oldHinduSolarFromFixed( 694799)==QVector<int>({5004,  1,  4}));
	QVERIFY(OldHinduSolarCalendar::oldHinduSolarFromFixed( 704424)==QVector<int>({5030,  5, 11}));
	QVERIFY(OldHinduSolarCalendar::oldHinduSolarFromFixed( 708842)==QVector<int>({5042,  6, 15}));
	QVERIFY(OldHinduSolarCalendar::oldHinduSolarFromFixed( 709409)==QVector<int>({5044,  1,  4}));
	QVERIFY(OldHinduSolarCalendar::oldHinduSolarFromFixed( 709580)==QVector<int>({5044,  6, 23}));
	QVERIFY(OldHinduSolarCalendar::oldHinduSolarFromFixed( 727274)==QVector<int>({5092, 12,  2}));
	QVERIFY(OldHinduSolarCalendar::oldHinduSolarFromFixed( 728714)==QVector<int>({5096, 11, 11}));
	QVERIFY(OldHinduSolarCalendar::oldHinduSolarFromFixed( 744313)==QVector<int>({5139,  7, 26}));
	QVERIFY(OldHinduSolarCalendar::oldHinduSolarFromFixed( 764652)==QVector<int>({5195,  4,  2}));

	QVERIFY(-214193==OldHinduLuniSolarCalendar::fixedFromOldHinduLunar({2515,  6, 0, 11}));
	QVERIFY( -61387==OldHinduLuniSolarCalendar::fixedFromOldHinduLunar({2933,  9, 0, 26}));
	QVERIFY(  25469==OldHinduLuniSolarCalendar::fixedFromOldHinduLunar({3171,  8, 0,  3}));
	QVERIFY(  49217==OldHinduLuniSolarCalendar::fixedFromOldHinduLunar({3236,  8, 0,  9}));
	QVERIFY( 171307==OldHinduLuniSolarCalendar::fixedFromOldHinduLunar({3570, 11, 1, 19}));
	QVERIFY( 210155==OldHinduLuniSolarCalendar::fixedFromOldHinduLunar({3677,  3, 0,  5}));
	QVERIFY( 253427==OldHinduLuniSolarCalendar::fixedFromOldHinduLunar({3795,  9, 0, 15}));
	QVERIFY( 369740==OldHinduLuniSolarCalendar::fixedFromOldHinduLunar({4114,  2, 0,  7}));
	QVERIFY( 400085==OldHinduLuniSolarCalendar::fixedFromOldHinduLunar({4197,  2, 0, 24}));
	QVERIFY( 434355==OldHinduLuniSolarCalendar::fixedFromOldHinduLunar({4291,  1, 0,  9}));
	QVERIFY( 452605==OldHinduLuniSolarCalendar::fixedFromOldHinduLunar({4340, 12, 0,  9}));
	QVERIFY( 470160==OldHinduLuniSolarCalendar::fixedFromOldHinduLunar({4389,  1, 0, 23}));
	QVERIFY( 473837==OldHinduLuniSolarCalendar::fixedFromOldHinduLunar({4399,  2, 0,  8}));
	QVERIFY( 507850==OldHinduLuniSolarCalendar::fixedFromOldHinduLunar({4492,  4, 0,  2}));
	QVERIFY( 524156==OldHinduLuniSolarCalendar::fixedFromOldHinduLunar({4536, 11, 0,  7}));
	QVERIFY( 544676==OldHinduLuniSolarCalendar::fixedFromOldHinduLunar({4593,  1, 0,  3}));
	QVERIFY( 567118==OldHinduLuniSolarCalendar::fixedFromOldHinduLunar({4654,  7, 0,  2}));
	QVERIFY( 569477==OldHinduLuniSolarCalendar::fixedFromOldHinduLunar({4660, 11, 0, 29}));
	QVERIFY( 601716==OldHinduLuniSolarCalendar::fixedFromOldHinduLunar({4749,  3, 0, 20}));
	QVERIFY( 613424==OldHinduLuniSolarCalendar::fixedFromOldHinduLunar({4781,  4, 0,  4}));
	QVERIFY( 626596==OldHinduLuniSolarCalendar::fixedFromOldHinduLunar({4817,  5, 0,  6}));
	QVERIFY( 645554==OldHinduLuniSolarCalendar::fixedFromOldHinduLunar({4869,  4, 0,  5}));
	QVERIFY( 664224==OldHinduLuniSolarCalendar::fixedFromOldHinduLunar({4920,  5, 0, 12}));
	QVERIFY( 671401==OldHinduLuniSolarCalendar::fixedFromOldHinduLunar({4940,  1, 1, 13}));
	QVERIFY( 694799==OldHinduLuniSolarCalendar::fixedFromOldHinduLunar({5004,  1, 0, 23}));
	QVERIFY( 704424==OldHinduLuniSolarCalendar::fixedFromOldHinduLunar({5030,  5, 0, 21}));
	QVERIFY( 708842==OldHinduLuniSolarCalendar::fixedFromOldHinduLunar({5042,  7, 0,  9}));
	QVERIFY( 709409==OldHinduLuniSolarCalendar::fixedFromOldHinduLunar({5044,  1, 0, 15}));
	QVERIFY( 709580==OldHinduLuniSolarCalendar::fixedFromOldHinduLunar({5044,  7, 0,  9}));
	QVERIFY( 727274==OldHinduLuniSolarCalendar::fixedFromOldHinduLunar({5092, 12, 0, 14}));
	QVERIFY( 728714==OldHinduLuniSolarCalendar::fixedFromOldHinduLunar({5096, 12, 0,  7}));
	QVERIFY( 744313==OldHinduLuniSolarCalendar::fixedFromOldHinduLunar({5139,  8, 0, 14}));
	QVERIFY( 764652==OldHinduLuniSolarCalendar::fixedFromOldHinduLunar({5195,  4, 0,  6}));

	QVERIFY(OldHinduLuniSolarCalendar::oldHinduLunarFromFixed(-214193)==QVector<int>({2515,  6, 0, 11}));
	QVERIFY(OldHinduLuniSolarCalendar::oldHinduLunarFromFixed( -61387)==QVector<int>({2933,  9, 0, 26}));
	QVERIFY(OldHinduLuniSolarCalendar::oldHinduLunarFromFixed(  25469)==QVector<int>({3171,  8, 0,  3}));
	QVERIFY(OldHinduLuniSolarCalendar::oldHinduLunarFromFixed(  49217)==QVector<int>({3236,  8, 0,  9}));
	QVERIFY(OldHinduLuniSolarCalendar::oldHinduLunarFromFixed( 171307)==QVector<int>({3570, 11, 1, 19}));
	QVERIFY(OldHinduLuniSolarCalendar::oldHinduLunarFromFixed( 210155)==QVector<int>({3677,  3, 0,  5}));
	QVERIFY(OldHinduLuniSolarCalendar::oldHinduLunarFromFixed( 253427)==QVector<int>({3795,  9, 0, 15}));
	QVERIFY(OldHinduLuniSolarCalendar::oldHinduLunarFromFixed( 369740)==QVector<int>({4114,  2, 0,  7}));
	QVERIFY(OldHinduLuniSolarCalendar::oldHinduLunarFromFixed( 400085)==QVector<int>({4197,  2, 0, 24}));
	QVERIFY(OldHinduLuniSolarCalendar::oldHinduLunarFromFixed( 434355)==QVector<int>({4291,  1, 0,  9}));
	QVERIFY(OldHinduLuniSolarCalendar::oldHinduLunarFromFixed( 452605)==QVector<int>({4340, 12, 0,  9}));
	QVERIFY(OldHinduLuniSolarCalendar::oldHinduLunarFromFixed( 470160)==QVector<int>({4389,  1, 0, 23}));
	QVERIFY(OldHinduLuniSolarCalendar::oldHinduLunarFromFixed( 473837)==QVector<int>({4399,  2, 0,  8}));
	QVERIFY(OldHinduLuniSolarCalendar::oldHinduLunarFromFixed( 507850)==QVector<int>({4492,  4, 0,  2}));
	QVERIFY(OldHinduLuniSolarCalendar::oldHinduLunarFromFixed( 524156)==QVector<int>({4536, 11, 0,  7}));
	QVERIFY(OldHinduLuniSolarCalendar::oldHinduLunarFromFixed( 544676)==QVector<int>({4593,  1, 0,  3}));
	QVERIFY(OldHinduLuniSolarCalendar::oldHinduLunarFromFixed( 567118)==QVector<int>({4654,  7, 0,  2}));
	QVERIFY(OldHinduLuniSolarCalendar::oldHinduLunarFromFixed( 569477)==QVector<int>({4660, 11, 0, 29}));
	QVERIFY(OldHinduLuniSolarCalendar::oldHinduLunarFromFixed( 601716)==QVector<int>({4749,  3, 0, 20}));
	QVERIFY(OldHinduLuniSolarCalendar::oldHinduLunarFromFixed( 613424)==QVector<int>({4781,  4, 0,  4}));
	QVERIFY(OldHinduLuniSolarCalendar::oldHinduLunarFromFixed( 626596)==QVector<int>({4817,  5, 0,  6}));
	QVERIFY(OldHinduLuniSolarCalendar::oldHinduLunarFromFixed( 645554)==QVector<int>({4869,  4, 0,  5}));
	QVERIFY(OldHinduLuniSolarCalendar::oldHinduLunarFromFixed( 664224)==QVector<int>({4920,  5, 0, 12}));
	QVERIFY(OldHinduLuniSolarCalendar::oldHinduLunarFromFixed( 671401)==QVector<int>({4940,  1, 1, 13}));
	QVERIFY(OldHinduLuniSolarCalendar::oldHinduLunarFromFixed( 694799)==QVector<int>({5004,  1, 0, 23}));
	QVERIFY(OldHinduLuniSolarCalendar::oldHinduLunarFromFixed( 704424)==QVector<int>({5030,  5, 0, 21}));
	QVERIFY(OldHinduLuniSolarCalendar::oldHinduLunarFromFixed( 708842)==QVector<int>({5042,  7, 0,  9}));
	QVERIFY(OldHinduLuniSolarCalendar::oldHinduLunarFromFixed( 709409)==QVector<int>({5044,  1, 0, 15}));
	QVERIFY(OldHinduLuniSolarCalendar::oldHinduLunarFromFixed( 709580)==QVector<int>({5044,  7, 0,  9}));
	QVERIFY(OldHinduLuniSolarCalendar::oldHinduLunarFromFixed( 727274)==QVector<int>({5092, 12, 0, 14}));
	QVERIFY(OldHinduLuniSolarCalendar::oldHinduLunarFromFixed( 728714)==QVector<int>({5096, 12, 0,  7}));
	QVERIFY(OldHinduLuniSolarCalendar::oldHinduLunarFromFixed( 744313)==QVector<int>({5139,  8, 0, 14}));
	QVERIFY(OldHinduLuniSolarCalendar::oldHinduLunarFromFixed( 764652)==QVector<int>({5195,  4, 0,  6}));
}

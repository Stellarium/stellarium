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
#include "../IcelandicCalendar.hpp"
#include "../EgyptianCalendar.hpp"
#include "../ArmenianCalendar.hpp"
#include "../ZoroastrianCalendar.hpp"
#include "../CopticCalendar.hpp"
#include "../EthiopicCalendar.hpp"
#include "../RomanCalendar.hpp"
#include "../FrenchArithmeticCalendar.hpp"
#include "../IslamicCalendar.hpp"
#include "../HebrewCalendar.hpp"
#include "../PersianArithmeticCalendar.hpp"
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
	QVERIFY(StelUtils::intFloorDiv(8, 2)==4);
	QVERIFY(StelUtils::intFloorDiv(8, 3)==2);
	QVERIFY(StelUtils::intFloorDiv(-8, 3)==-3);
	QVERIFY(StelUtils::intFloorDiv(-8, -2)==4);
	QVERIFY(StelUtils::intFloorDiv(-8, 2)==-4);
	QVERIFY(StelUtils::intFloorDiv(8, -2)==-4);
}

void TestCalendars::testEuropean()
{
	QVERIFY(GregorianCalendar::fixedFromGregorian({1582, 10, 14})==JulianCalendar::fixedFromJulian({1582, 10, 4}));
	QVERIFY(GregorianCalendar::fixedFromGregorian({1582, 10, 15})==JulianCalendar::fixedFromJulian({1582, 10, 5}));
	QVERIFY(GregorianCalendar::fixedFromGregorian({1, 1, 1})==GregorianCalendar::gregorianEpoch);
	QVERIFY(JulianCalendar::fixedFromJulian({1, 1, 1})==JulianCalendar::julianEpoch);
	QVERIFY(JulianCalendar::fixedFromJulian({1, 1, 1})==GregorianCalendar::fixedFromGregorian({0, JulianCalendar::december, 30}));

	QVERIFY(-214193==ISOCalendar::fixedFromISO({-586, 29, 7}));
	QVERIFY( -61387==ISOCalendar::fixedFromISO({-168, 49, 3}));
	QVERIFY(  25469==ISOCalendar::fixedFromISO({  70, 39, 3}));
	QVERIFY(  49217==ISOCalendar::fixedFromISO({ 135, 39, 7}));
	QVERIFY( 171307==ISOCalendar::fixedFromISO({ 470,  2, 3}));
	QVERIFY( 210155==ISOCalendar::fixedFromISO({ 576, 21, 1}));
	QVERIFY( 253427==ISOCalendar::fixedFromISO({ 694, 45, 6}));
	QVERIFY( 369740==ISOCalendar::fixedFromISO({1013, 16, 7}));
	QVERIFY( 400085==ISOCalendar::fixedFromISO({1096, 21, 7}));
	QVERIFY( 434355==ISOCalendar::fixedFromISO({1190, 12, 5}));
	QVERIFY( 452605==ISOCalendar::fixedFromISO({1240, 10, 6}));
	QVERIFY( 470160==ISOCalendar::fixedFromISO({1288, 14, 5}));
	QVERIFY( 473837==ISOCalendar::fixedFromISO({1298, 17, 7}));
	QVERIFY( 507850==ISOCalendar::fixedFromISO({1391, 23, 7}));
	QVERIFY( 524156==ISOCalendar::fixedFromISO({1436,  5, 3}));
	QVERIFY( 544676==ISOCalendar::fixedFromISO({1492, 14, 6}));
	QVERIFY( 567118==ISOCalendar::fixedFromISO({1553, 38, 6}));
	QVERIFY( 569477==ISOCalendar::fixedFromISO({1560,  9, 6}));
	QVERIFY( 601716==ISOCalendar::fixedFromISO({1648, 24, 3}));
	QVERIFY( 613424==ISOCalendar::fixedFromISO({1680, 26, 7}));
	QVERIFY( 626596==ISOCalendar::fixedFromISO({1716, 30, 5}));
	QVERIFY( 645554==ISOCalendar::fixedFromISO({1768, 24, 7}));
	QVERIFY( 664224==ISOCalendar::fixedFromISO({1819, 31, 1}));
	QVERIFY( 671401==ISOCalendar::fixedFromISO({1839, 13, 3}));
	QVERIFY( 694799==ISOCalendar::fixedFromISO({1903, 16, 7}));
	QVERIFY( 704424==ISOCalendar::fixedFromISO({1929, 34, 7}));
	QVERIFY( 708842==ISOCalendar::fixedFromISO({1941, 40, 1}));
	QVERIFY( 709409==ISOCalendar::fixedFromISO({1943, 16, 1}));
	QVERIFY( 709580==ISOCalendar::fixedFromISO({1943, 40, 4}));
	QVERIFY( 727274==ISOCalendar::fixedFromISO({1992, 12, 2}));
	QVERIFY( 728714==ISOCalendar::fixedFromISO({1996,  8, 7}));
	QVERIFY( 744313==ISOCalendar::fixedFromISO({2038, 45, 3}));
	QVERIFY( 764652==ISOCalendar::fixedFromISO({2094, 28, 7}));

	QVERIFY(ISOCalendar::isoFromFixed(-214193)==QVector<int>({-586, 29, 7}));
	QVERIFY(ISOCalendar::isoFromFixed( -61387)==QVector<int>({-168, 49, 3}));
	QVERIFY(ISOCalendar::isoFromFixed(  25469)==QVector<int>({  70, 39, 3}));
	QVERIFY(ISOCalendar::isoFromFixed(  49217)==QVector<int>({ 135, 39, 7}));
	QVERIFY(ISOCalendar::isoFromFixed( 171307)==QVector<int>({ 470,  2, 3}));
	QVERIFY(ISOCalendar::isoFromFixed( 210155)==QVector<int>({ 576, 21, 1}));
	QVERIFY(ISOCalendar::isoFromFixed( 253427)==QVector<int>({ 694, 45, 6}));
	QVERIFY(ISOCalendar::isoFromFixed( 369740)==QVector<int>({1013, 16, 7}));
	QVERIFY(ISOCalendar::isoFromFixed( 400085)==QVector<int>({1096, 21, 7}));
	QVERIFY(ISOCalendar::isoFromFixed( 434355)==QVector<int>({1190, 12, 5}));
	QVERIFY(ISOCalendar::isoFromFixed( 452605)==QVector<int>({1240, 10, 6}));
	QVERIFY(ISOCalendar::isoFromFixed( 470160)==QVector<int>({1288, 14, 5}));
	QVERIFY(ISOCalendar::isoFromFixed( 473837)==QVector<int>({1298, 17, 7}));
	QVERIFY(ISOCalendar::isoFromFixed( 507850)==QVector<int>({1391, 23, 7}));
	QVERIFY(ISOCalendar::isoFromFixed( 524156)==QVector<int>({1436,  5, 3}));
	QVERIFY(ISOCalendar::isoFromFixed( 544676)==QVector<int>({1492, 14, 6}));
	QVERIFY(ISOCalendar::isoFromFixed( 567118)==QVector<int>({1553, 38, 6}));
	QVERIFY(ISOCalendar::isoFromFixed( 569477)==QVector<int>({1560,  9, 6}));
	QVERIFY(ISOCalendar::isoFromFixed( 601716)==QVector<int>({1648, 24, 3}));
	QVERIFY(ISOCalendar::isoFromFixed( 613424)==QVector<int>({1680, 26, 7}));
	QVERIFY(ISOCalendar::isoFromFixed( 626596)==QVector<int>({1716, 30, 5}));
	QVERIFY(ISOCalendar::isoFromFixed( 645554)==QVector<int>({1768, 24, 7}));
	QVERIFY(ISOCalendar::isoFromFixed( 664224)==QVector<int>({1819, 31, 1}));
	QVERIFY(ISOCalendar::isoFromFixed( 671401)==QVector<int>({1839, 13, 3}));
	QVERIFY(ISOCalendar::isoFromFixed( 694799)==QVector<int>({1903, 16, 7}));
	QVERIFY(ISOCalendar::isoFromFixed( 704424)==QVector<int>({1929, 34, 7}));
	QVERIFY(ISOCalendar::isoFromFixed( 708842)==QVector<int>({1941, 40, 1}));
	QVERIFY(ISOCalendar::isoFromFixed( 709409)==QVector<int>({1943, 16, 1}));
	QVERIFY(ISOCalendar::isoFromFixed( 709580)==QVector<int>({1943, 40, 4}));
	QVERIFY(ISOCalendar::isoFromFixed( 727274)==QVector<int>({1992, 12, 2}));
	QVERIFY(ISOCalendar::isoFromFixed( 728714)==QVector<int>({1996,  8, 7}));
	QVERIFY(ISOCalendar::isoFromFixed( 744313)==QVector<int>({2038, 45, 3}));
	QVERIFY(ISOCalendar::isoFromFixed( 764652)==QVector<int>({2094, 28, 7}));

	QVERIFY(-214193==IcelandicCalendar::fixedFromIcelandic({-586,  90, 14, 0}));
	QVERIFY( -61387==IcelandicCalendar::fixedFromIcelandic({-168, 270,  6, 3}));
	QVERIFY(  25469==IcelandicCalendar::fixedFromIcelandic({  70,  90, 22, 3}));
	QVERIFY(  49217==IcelandicCalendar::fixedFromIcelandic({ 135,  90, 24, 0}));
	QVERIFY( 171307==IcelandicCalendar::fixedFromIcelandic({ 469, 270, 11, 3}));
	QVERIFY( 210155==IcelandicCalendar::fixedFromIcelandic({ 576,  90,  4, 1}));
	QVERIFY( 253427==IcelandicCalendar::fixedFromIcelandic({ 694, 270,  3, 6}));
	QVERIFY( 369740==IcelandicCalendar::fixedFromIcelandic({1013,  90,  1, 0}));
	QVERIFY( 400085==IcelandicCalendar::fixedFromIcelandic({1096,  90,  5, 0}));
	QVERIFY( 434355==IcelandicCalendar::fixedFromIcelandic({1189, 270, 22, 5}));
	QVERIFY( 452605==IcelandicCalendar::fixedFromIcelandic({1239, 270, 21, 6}));
	QVERIFY( 470160==IcelandicCalendar::fixedFromIcelandic({1287, 270, 23, 5}));
	QVERIFY( 473837==IcelandicCalendar::fixedFromIcelandic({1298,  90,  1, 0}));
	QVERIFY( 507850==IcelandicCalendar::fixedFromIcelandic({1391,  90,  8, 0}));
	QVERIFY( 524156==IcelandicCalendar::fixedFromIcelandic({1435, 270, 15, 3}));
	QVERIFY( 544676==IcelandicCalendar::fixedFromIcelandic({1491, 270, 25, 6}));
	QVERIFY( 567118==IcelandicCalendar::fixedFromIcelandic({1553,  90, 22, 6}));
	QVERIFY( 569477==IcelandicCalendar::fixedFromIcelandic({1559, 270, 20, 6}));
	QVERIFY( 601716==IcelandicCalendar::fixedFromIcelandic({1648,  90,  7, 3}));
	QVERIFY( 613424==IcelandicCalendar::fixedFromIcelandic({1680,  90, 10, 0}));
	QVERIFY( 626596==IcelandicCalendar::fixedFromIcelandic({1716,  90, 14, 5}));
	QVERIFY( 645554==IcelandicCalendar::fixedFromIcelandic({1768,  90,  9, 0}));
	QVERIFY( 664224==IcelandicCalendar::fixedFromIcelandic({1819,  90, 15, 1}));
	QVERIFY( 671401==IcelandicCalendar::fixedFromIcelandic({1838, 270, 22, 3}));
	QVERIFY( 694799==IcelandicCalendar::fixedFromIcelandic({1902, 270, 26, 0}));
	QVERIFY( 704424==IcelandicCalendar::fixedFromIcelandic({1929,  90, 18, 0}));
	QVERIFY( 708842==IcelandicCalendar::fixedFromIcelandic({1941,  90, 23, 1}));
	QVERIFY( 709409==IcelandicCalendar::fixedFromIcelandic({1942, 270, 26, 1}));
	QVERIFY( 709580==IcelandicCalendar::fixedFromIcelandic({1943,  90, 25, 4}));
	QVERIFY( 727274==IcelandicCalendar::fixedFromIcelandic({1991, 270, 21, 2}));
	QVERIFY( 728714==IcelandicCalendar::fixedFromIcelandic({1995, 270, 18, 0}));
	QVERIFY( 744313==IcelandicCalendar::fixedFromIcelandic({2038, 270,  3, 3}));
	QVERIFY( 764652==IcelandicCalendar::fixedFromIcelandic({2094,  90, 13, 0}));

	QVERIFY(IcelandicCalendar::icelandicFromFixed(-214193)==QVector<int>({-586,  90, 14, 0}));
	QVERIFY(IcelandicCalendar::icelandicFromFixed( -61387)==QVector<int>({-168, 270,  6, 3}));
	QVERIFY(IcelandicCalendar::icelandicFromFixed(  25469)==QVector<int>({  70,  90, 22, 3}));
	QVERIFY(IcelandicCalendar::icelandicFromFixed(  49217)==QVector<int>({ 135,  90, 24, 0}));
	QVERIFY(IcelandicCalendar::icelandicFromFixed( 171307)==QVector<int>({ 469, 270, 11, 3}));
	QVERIFY(IcelandicCalendar::icelandicFromFixed( 210155)==QVector<int>({ 576,  90,  4, 1}));
	QVERIFY(IcelandicCalendar::icelandicFromFixed( 253427)==QVector<int>({ 694, 270,  3, 6}));
	QVERIFY(IcelandicCalendar::icelandicFromFixed( 369740)==QVector<int>({1013,  90,  1, 0}));
	QVERIFY(IcelandicCalendar::icelandicFromFixed( 400085)==QVector<int>({1096,  90,  5, 0}));
	QVERIFY(IcelandicCalendar::icelandicFromFixed( 434355)==QVector<int>({1189, 270, 22, 5}));
	QVERIFY(IcelandicCalendar::icelandicFromFixed( 452605)==QVector<int>({1239, 270, 21, 6}));
	QVERIFY(IcelandicCalendar::icelandicFromFixed( 470160)==QVector<int>({1287, 270, 23, 5}));
	QVERIFY(IcelandicCalendar::icelandicFromFixed( 473837)==QVector<int>({1298,  90,  1, 0}));
	QVERIFY(IcelandicCalendar::icelandicFromFixed( 507850)==QVector<int>({1391,  90,  8, 0}));
	QVERIFY(IcelandicCalendar::icelandicFromFixed( 524156)==QVector<int>({1435, 270, 15, 3}));
	QVERIFY(IcelandicCalendar::icelandicFromFixed( 544676)==QVector<int>({1491, 270, 25, 6}));
	QVERIFY(IcelandicCalendar::icelandicFromFixed( 567118)==QVector<int>({1553,  90, 22, 6}));
	QVERIFY(IcelandicCalendar::icelandicFromFixed( 569477)==QVector<int>({1559, 270, 20, 6}));
	QVERIFY(IcelandicCalendar::icelandicFromFixed( 601716)==QVector<int>({1648,  90,  7, 3}));
	QVERIFY(IcelandicCalendar::icelandicFromFixed( 613424)==QVector<int>({1680,  90, 10, 0}));
	QVERIFY(IcelandicCalendar::icelandicFromFixed( 626596)==QVector<int>({1716,  90, 14, 5}));
	QVERIFY(IcelandicCalendar::icelandicFromFixed( 645554)==QVector<int>({1768,  90,  9, 0}));
	QVERIFY(IcelandicCalendar::icelandicFromFixed( 664224)==QVector<int>({1819,  90, 15, 1}));
	QVERIFY(IcelandicCalendar::icelandicFromFixed( 671401)==QVector<int>({1838, 270, 22, 3}));
	QVERIFY(IcelandicCalendar::icelandicFromFixed( 694799)==QVector<int>({1902, 270, 26, 0}));
	QVERIFY(IcelandicCalendar::icelandicFromFixed( 704424)==QVector<int>({1929,  90, 18, 0}));
	QVERIFY(IcelandicCalendar::icelandicFromFixed( 708842)==QVector<int>({1941,  90, 23, 1}));
	QVERIFY(IcelandicCalendar::icelandicFromFixed( 709409)==QVector<int>({1942, 270, 26, 1}));
	QVERIFY(IcelandicCalendar::icelandicFromFixed( 709580)==QVector<int>({1943,  90, 25, 4}));
	QVERIFY(IcelandicCalendar::icelandicFromFixed( 727274)==QVector<int>({1991, 270, 21, 2}));
	QVERIFY(IcelandicCalendar::icelandicFromFixed( 728714)==QVector<int>({1995, 270, 18, 0}));
	QVERIFY(IcelandicCalendar::icelandicFromFixed( 744313)==QVector<int>({2038, 270,  3, 3}));
	QVERIFY(IcelandicCalendar::icelandicFromFixed( 764652)==QVector<int>({2094,  90, 13, 0}));
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
	QVERIFY(-214193==EgyptianCalendar::fixedFromEgyptian({ 161,  7, 15}));
	QVERIFY( -61387==EgyptianCalendar::fixedFromEgyptian({ 580,  3,  6}));
	QVERIFY(  25469==EgyptianCalendar::fixedFromEgyptian({ 818,  2, 22}));
	QVERIFY(  49217==EgyptianCalendar::fixedFromEgyptian({ 883,  3, 15}));
	QVERIFY( 171307==EgyptianCalendar::fixedFromEgyptian({1217,  9, 15}));
	QVERIFY( 210155==EgyptianCalendar::fixedFromEgyptian({1324,  2, 18}));
	QVERIFY( 253427==EgyptianCalendar::fixedFromEgyptian({1442,  9, 10}));
	QVERIFY( 369740==EgyptianCalendar::fixedFromEgyptian({1761,  5,  8}));
	QVERIFY( 400085==EgyptianCalendar::fixedFromEgyptian({1844,  6, 28}));
	QVERIFY( 434355==EgyptianCalendar::fixedFromEgyptian({1938,  5, 18}));
	QVERIFY( 452605==EgyptianCalendar::fixedFromEgyptian({1988,  5, 18}));
	QVERIFY( 470160==EgyptianCalendar::fixedFromEgyptian({2036,  6, 23}));
	QVERIFY( 473837==EgyptianCalendar::fixedFromEgyptian({2046,  7, 20}));
	QVERIFY( 507850==EgyptianCalendar::fixedFromEgyptian({2139,  9, 28}));
	QVERIFY( 524156==EgyptianCalendar::fixedFromEgyptian({2184,  5, 29}));
	QVERIFY( 544676==EgyptianCalendar::fixedFromEgyptian({2240,  8, 19}));
	QVERIFY( 567118==EgyptianCalendar::fixedFromEgyptian({2302,  2, 11}));
	QVERIFY( 569477==EgyptianCalendar::fixedFromEgyptian({2308,  7, 30}));
	QVERIFY( 601716==EgyptianCalendar::fixedFromEgyptian({2396, 11, 29}));
	QVERIFY( 613424==EgyptianCalendar::fixedFromEgyptian({2428, 12, 27}));
	QVERIFY( 626596==EgyptianCalendar::fixedFromEgyptian({2465,  1, 24}));
	QVERIFY( 645554==EgyptianCalendar::fixedFromEgyptian({2517,  1,  2}));
	QVERIFY( 664224==EgyptianCalendar::fixedFromEgyptian({2568,  2, 27}));
	QVERIFY( 671401==EgyptianCalendar::fixedFromEgyptian({2587, 10, 29}));
	QVERIFY( 694799==EgyptianCalendar::fixedFromEgyptian({2651, 12,  7}));
	QVERIFY( 704424==EgyptianCalendar::fixedFromEgyptian({2678,  4, 17}));
	QVERIFY( 708842==EgyptianCalendar::fixedFromEgyptian({2690,  5, 25}));
	QVERIFY( 709409==EgyptianCalendar::fixedFromEgyptian({2691, 12, 17}));
	QVERIFY( 709580==EgyptianCalendar::fixedFromEgyptian({2692,  6,  3}));
	QVERIFY( 727274==EgyptianCalendar::fixedFromEgyptian({2740, 11, 27}));
	QVERIFY( 728714==EgyptianCalendar::fixedFromEgyptian({2744, 11,  7}));
	QVERIFY( 744313==EgyptianCalendar::fixedFromEgyptian({2787,  8,  1}));
	QVERIFY( 764652==EgyptianCalendar::fixedFromEgyptian({2843,  4, 20}));

	QVERIFY(EgyptianCalendar::egyptianFromFixed(-214193)==QVector<int>({ 161,  7, 15}));
	QVERIFY(EgyptianCalendar::egyptianFromFixed( -61387)==QVector<int>({ 580,  3,  6}));
	QVERIFY(EgyptianCalendar::egyptianFromFixed(  25469)==QVector<int>({ 818,  2, 22}));
	QVERIFY(EgyptianCalendar::egyptianFromFixed(  49217)==QVector<int>({ 883,  3, 15}));
	QVERIFY(EgyptianCalendar::egyptianFromFixed( 171307)==QVector<int>({1217,  9, 15}));
	QVERIFY(EgyptianCalendar::egyptianFromFixed( 210155)==QVector<int>({1324,  2, 18}));
	QVERIFY(EgyptianCalendar::egyptianFromFixed( 253427)==QVector<int>({1442,  9, 10}));
	QVERIFY(EgyptianCalendar::egyptianFromFixed( 369740)==QVector<int>({1761,  5,  8}));
	QVERIFY(EgyptianCalendar::egyptianFromFixed( 400085)==QVector<int>({1844,  6, 28}));
	QVERIFY(EgyptianCalendar::egyptianFromFixed( 434355)==QVector<int>({1938,  5, 18}));
	QVERIFY(EgyptianCalendar::egyptianFromFixed( 452605)==QVector<int>({1988,  5, 18}));
	QVERIFY(EgyptianCalendar::egyptianFromFixed( 470160)==QVector<int>({2036,  6, 23}));
	QVERIFY(EgyptianCalendar::egyptianFromFixed( 473837)==QVector<int>({2046,  7, 20}));
	QVERIFY(EgyptianCalendar::egyptianFromFixed( 507850)==QVector<int>({2139,  9, 28}));
	QVERIFY(EgyptianCalendar::egyptianFromFixed( 524156)==QVector<int>({2184,  5, 29}));
	QVERIFY(EgyptianCalendar::egyptianFromFixed( 544676)==QVector<int>({2240,  8, 19}));
	QVERIFY(EgyptianCalendar::egyptianFromFixed( 567118)==QVector<int>({2302,  2, 11}));
	QVERIFY(EgyptianCalendar::egyptianFromFixed( 569477)==QVector<int>({2308,  7, 30}));
	QVERIFY(EgyptianCalendar::egyptianFromFixed( 601716)==QVector<int>({2396, 11, 29}));
	QVERIFY(EgyptianCalendar::egyptianFromFixed( 613424)==QVector<int>({2428, 12, 27}));
	QVERIFY(EgyptianCalendar::egyptianFromFixed( 626596)==QVector<int>({2465,  1, 24}));
	QVERIFY(EgyptianCalendar::egyptianFromFixed( 645554)==QVector<int>({2517,  1,  2}));
	QVERIFY(EgyptianCalendar::egyptianFromFixed( 664224)==QVector<int>({2568,  2, 27}));
	QVERIFY(EgyptianCalendar::egyptianFromFixed( 671401)==QVector<int>({2587, 10, 29}));
	QVERIFY(EgyptianCalendar::egyptianFromFixed( 694799)==QVector<int>({2651, 12,  7}));
	QVERIFY(EgyptianCalendar::egyptianFromFixed( 704424)==QVector<int>({2678,  4, 17}));
	QVERIFY(EgyptianCalendar::egyptianFromFixed( 708842)==QVector<int>({2690,  5, 25}));
	QVERIFY(EgyptianCalendar::egyptianFromFixed( 709409)==QVector<int>({2691, 12, 17}));
	QVERIFY(EgyptianCalendar::egyptianFromFixed( 709580)==QVector<int>({2692,  6,  3}));
	QVERIFY(EgyptianCalendar::egyptianFromFixed( 727274)==QVector<int>({2740, 11, 27}));
	QVERIFY(EgyptianCalendar::egyptianFromFixed( 728714)==QVector<int>({2744, 11,  7}));
	QVERIFY(EgyptianCalendar::egyptianFromFixed( 744313)==QVector<int>({2787,  8,  1}));
	QVERIFY(EgyptianCalendar::egyptianFromFixed( 764652)==QVector<int>({2843,  4, 20}));

	QVERIFY(-214193==ArmenianCalendar::fixedFromArmenian({-1138, 4, 10}));
	QVERIFY( -61387==ArmenianCalendar::fixedFromArmenian({-720, 12,  6}));
	QVERIFY(  25469==ArmenianCalendar::fixedFromArmenian({-482, 11, 22}));
	QVERIFY(  49217==ArmenianCalendar::fixedFromArmenian({-417, 12, 15}));
	QVERIFY( 171307==ArmenianCalendar::fixedFromArmenian({ -82,  6, 10}));
	QVERIFY( 210155==ArmenianCalendar::fixedFromArmenian({  24, 11, 18}));
	QVERIFY( 253427==ArmenianCalendar::fixedFromArmenian({ 143,  6,  5}));
	QVERIFY( 369740==ArmenianCalendar::fixedFromArmenian({ 462,  2,  3}));
	QVERIFY( 400085==ArmenianCalendar::fixedFromArmenian({ 545,  3, 23}));
	QVERIFY( 434355==ArmenianCalendar::fixedFromArmenian({ 639,  2, 13}));
	QVERIFY( 452605==ArmenianCalendar::fixedFromArmenian({ 689,  2, 13}));
	QVERIFY( 470160==ArmenianCalendar::fixedFromArmenian({ 737,  3, 18}));
	QVERIFY( 473837==ArmenianCalendar::fixedFromArmenian({ 747,  4, 15}));
	QVERIFY( 507850==ArmenianCalendar::fixedFromArmenian({ 840,  6, 23}));
	QVERIFY( 524156==ArmenianCalendar::fixedFromArmenian({ 885,  2, 24}));
	QVERIFY( 544676==ArmenianCalendar::fixedFromArmenian({ 941,  5, 14}));
	QVERIFY( 567118==ArmenianCalendar::fixedFromArmenian({1002, 11, 11}));
	QVERIFY( 569477==ArmenianCalendar::fixedFromArmenian({1009,  4, 25}));
	QVERIFY( 601716==ArmenianCalendar::fixedFromArmenian({1097,  8, 24}));
	QVERIFY( 613424==ArmenianCalendar::fixedFromArmenian({1129,  9, 22}));
	QVERIFY( 626596==ArmenianCalendar::fixedFromArmenian({1165, 10, 24}));
	QVERIFY( 645554==ArmenianCalendar::fixedFromArmenian({1217, 10,  2}));
	QVERIFY( 664224==ArmenianCalendar::fixedFromArmenian({1268, 11, 27}));
	QVERIFY( 671401==ArmenianCalendar::fixedFromArmenian({1288,  7, 24}));
	QVERIFY( 694799==ArmenianCalendar::fixedFromArmenian({1352,  9,  2}));
	QVERIFY( 704424==ArmenianCalendar::fixedFromArmenian({1379,  1, 12}));
	QVERIFY( 708842==ArmenianCalendar::fixedFromArmenian({1391,  2, 20}));
	QVERIFY( 709409==ArmenianCalendar::fixedFromArmenian({1392,  9, 12}));
	QVERIFY( 709580==ArmenianCalendar::fixedFromArmenian({1393,  2, 28}));
	QVERIFY( 727274==ArmenianCalendar::fixedFromArmenian({1441,  8, 22}));
	QVERIFY( 728714==ArmenianCalendar::fixedFromArmenian({1445,  8,  2}));
	QVERIFY( 744313==ArmenianCalendar::fixedFromArmenian({1488,  4, 26}));
	QVERIFY( 764652==ArmenianCalendar::fixedFromArmenian({1544,  1, 15}));

	QVERIFY(ArmenianCalendar::armenianFromFixed(-214193)==QVector<int>({-1138, 4, 10}));
	QVERIFY(ArmenianCalendar::armenianFromFixed( -61387)==QVector<int>({-720, 12,  6}));
	QVERIFY(ArmenianCalendar::armenianFromFixed(  25469)==QVector<int>({-482, 11, 22}));
	QVERIFY(ArmenianCalendar::armenianFromFixed(  49217)==QVector<int>({-417, 12, 15}));
	QVERIFY(ArmenianCalendar::armenianFromFixed( 171307)==QVector<int>({ -82,  6, 10}));
	QVERIFY(ArmenianCalendar::armenianFromFixed( 210155)==QVector<int>({  24, 11, 18}));
	QVERIFY(ArmenianCalendar::armenianFromFixed( 253427)==QVector<int>({ 143,  6,  5}));
	QVERIFY(ArmenianCalendar::armenianFromFixed( 369740)==QVector<int>({ 462,  2,  3}));
	QVERIFY(ArmenianCalendar::armenianFromFixed( 400085)==QVector<int>({ 545,  3, 23}));
	QVERIFY(ArmenianCalendar::armenianFromFixed( 434355)==QVector<int>({ 639,  2, 13}));
	QVERIFY(ArmenianCalendar::armenianFromFixed( 452605)==QVector<int>({ 689,  2, 13}));
	QVERIFY(ArmenianCalendar::armenianFromFixed( 470160)==QVector<int>({ 737,  3, 18}));
	QVERIFY(ArmenianCalendar::armenianFromFixed( 473837)==QVector<int>({ 747,  4, 15}));
	QVERIFY(ArmenianCalendar::armenianFromFixed( 507850)==QVector<int>({ 840,  6, 23}));
	QVERIFY(ArmenianCalendar::armenianFromFixed( 524156)==QVector<int>({ 885,  2, 24}));
	QVERIFY(ArmenianCalendar::armenianFromFixed( 544676)==QVector<int>({ 941,  5, 14}));
	QVERIFY(ArmenianCalendar::armenianFromFixed( 567118)==QVector<int>({1002, 11, 11}));
	QVERIFY(ArmenianCalendar::armenianFromFixed( 569477)==QVector<int>({1009,  4, 25}));
	QVERIFY(ArmenianCalendar::armenianFromFixed( 601716)==QVector<int>({1097,  8, 24}));
	QVERIFY(ArmenianCalendar::armenianFromFixed( 613424)==QVector<int>({1129,  9, 22}));
	QVERIFY(ArmenianCalendar::armenianFromFixed( 626596)==QVector<int>({1165, 10, 24}));
	QVERIFY(ArmenianCalendar::armenianFromFixed( 645554)==QVector<int>({1217, 10,  2}));
	QVERIFY(ArmenianCalendar::armenianFromFixed( 664224)==QVector<int>({1268, 11, 27}));
	QVERIFY(ArmenianCalendar::armenianFromFixed( 671401)==QVector<int>({1288,  7, 24}));
	QVERIFY(ArmenianCalendar::armenianFromFixed( 694799)==QVector<int>({1352,  9,  2}));
	QVERIFY(ArmenianCalendar::armenianFromFixed( 704424)==QVector<int>({1379,  1, 12}));
	QVERIFY(ArmenianCalendar::armenianFromFixed( 708842)==QVector<int>({1391,  2, 20}));
	QVERIFY(ArmenianCalendar::armenianFromFixed( 709409)==QVector<int>({1392,  9, 12}));
	QVERIFY(ArmenianCalendar::armenianFromFixed( 709580)==QVector<int>({1393,  2, 28}));
	QVERIFY(ArmenianCalendar::armenianFromFixed( 727274)==QVector<int>({1441,  8, 22}));
	QVERIFY(ArmenianCalendar::armenianFromFixed( 728714)==QVector<int>({1445,  8,  2}));
	QVERIFY(ArmenianCalendar::armenianFromFixed( 744313)==QVector<int>({1488,  4, 26}));
	QVERIFY(ArmenianCalendar::armenianFromFixed( 764652)==QVector<int>({1544,  1, 15}));

	QVERIFY(-214193==CopticCalendar::fixedFromCoptic({-870, 12,  6}));
	QVERIFY( -61387==CopticCalendar::fixedFromCoptic({-451,  4, 12}));
	QVERIFY(  25469==CopticCalendar::fixedFromCoptic({-213,  1, 29}));
	QVERIFY(  49217==CopticCalendar::fixedFromCoptic({-148,  2,  5}));
	QVERIFY( 171307==CopticCalendar::fixedFromCoptic({ 186,  5, 12}));
	QVERIFY( 210155==CopticCalendar::fixedFromCoptic({ 292,  9, 23}));
	QVERIFY( 253427==CopticCalendar::fixedFromCoptic({ 411,  3, 11}));
	QVERIFY( 369740==CopticCalendar::fixedFromCoptic({ 729,  8, 24}));
	QVERIFY( 400085==CopticCalendar::fixedFromCoptic({ 812,  9, 23}));
	QVERIFY( 434355==CopticCalendar::fixedFromCoptic({ 906,  7, 20}));
	QVERIFY( 452605==CopticCalendar::fixedFromCoptic({ 956,  7,  7}));
	QVERIFY( 470160==CopticCalendar::fixedFromCoptic({1004,  7, 30}));
	QVERIFY( 473837==CopticCalendar::fixedFromCoptic({1014,  8, 25}));
	QVERIFY( 507850==CopticCalendar::fixedFromCoptic({1107, 10, 10}));
	QVERIFY( 524156==CopticCalendar::fixedFromCoptic({1152,  5, 29}));
	QVERIFY( 544676==CopticCalendar::fixedFromCoptic({1208,  8,  5}));
	QVERIFY( 567118==CopticCalendar::fixedFromCoptic({1270,  1, 12}));
	QVERIFY( 569477==CopticCalendar::fixedFromCoptic({1276,  6, 29}));
	QVERIFY( 601716==CopticCalendar::fixedFromCoptic({1364, 10,  6}));
	QVERIFY( 613424==CopticCalendar::fixedFromCoptic({1396, 10, 26}));
	QVERIFY( 626596==CopticCalendar::fixedFromCoptic({1432, 11, 19}));
	QVERIFY( 645554==CopticCalendar::fixedFromCoptic({1484, 10, 14}));
	QVERIFY( 664224==CopticCalendar::fixedFromCoptic({1535, 11, 27}));
	QVERIFY( 671401==CopticCalendar::fixedFromCoptic({1555,  7, 19}));
	QVERIFY( 694799==CopticCalendar::fixedFromCoptic({1619,  8, 11}));
	QVERIFY( 704424==CopticCalendar::fixedFromCoptic({1645, 12, 19}));
	QVERIFY( 708842==CopticCalendar::fixedFromCoptic({1658,  1, 19}));
	QVERIFY( 709409==CopticCalendar::fixedFromCoptic({1659,  8, 11}));
	QVERIFY( 709580==CopticCalendar::fixedFromCoptic({1660,  1, 26}));
	QVERIFY( 727274==CopticCalendar::fixedFromCoptic({1708,  7,  8}));
	QVERIFY( 728714==CopticCalendar::fixedFromCoptic({1712,  6, 17}));
	QVERIFY( 744313==CopticCalendar::fixedFromCoptic({1755,  3,  1}));
	QVERIFY( 764652==CopticCalendar::fixedFromCoptic({1810, 11, 11}));

	QVERIFY(CopticCalendar::copticFromFixed(-214193)==QVector<int>({-870, 12,  6}));
	QVERIFY(CopticCalendar::copticFromFixed( -61387)==QVector<int>({-451,  4, 12}));
	QVERIFY(CopticCalendar::copticFromFixed(  25469)==QVector<int>({-213,  1, 29}));
	QVERIFY(CopticCalendar::copticFromFixed(  49217)==QVector<int>({-148,  2,  5}));
	QVERIFY(CopticCalendar::copticFromFixed( 171307)==QVector<int>({ 186,  5, 12}));
	QVERIFY(CopticCalendar::copticFromFixed( 210155)==QVector<int>({ 292,  9, 23}));
	QVERIFY(CopticCalendar::copticFromFixed( 253427)==QVector<int>({ 411,  3, 11}));
	QVERIFY(CopticCalendar::copticFromFixed( 369740)==QVector<int>({ 729,  8, 24}));
	QVERIFY(CopticCalendar::copticFromFixed( 400085)==QVector<int>({ 812,  9, 23}));
	QVERIFY(CopticCalendar::copticFromFixed( 434355)==QVector<int>({ 906,  7, 20}));
	QVERIFY(CopticCalendar::copticFromFixed( 452605)==QVector<int>({ 956,  7,  7}));
	QVERIFY(CopticCalendar::copticFromFixed( 470160)==QVector<int>({1004,  7, 30}));
	QVERIFY(CopticCalendar::copticFromFixed( 473837)==QVector<int>({1014,  8, 25}));
	QVERIFY(CopticCalendar::copticFromFixed( 507850)==QVector<int>({1107, 10, 10}));
	QVERIFY(CopticCalendar::copticFromFixed( 524156)==QVector<int>({1152,  5, 29}));
	QVERIFY(CopticCalendar::copticFromFixed( 544676)==QVector<int>({1208,  8,  5}));
	QVERIFY(CopticCalendar::copticFromFixed( 567118)==QVector<int>({1270,  1, 12}));
	QVERIFY(CopticCalendar::copticFromFixed( 569477)==QVector<int>({1276,  6, 29}));
	QVERIFY(CopticCalendar::copticFromFixed( 601716)==QVector<int>({1364, 10,  6}));
	QVERIFY(CopticCalendar::copticFromFixed( 613424)==QVector<int>({1396, 10, 26}));
	QVERIFY(CopticCalendar::copticFromFixed( 626596)==QVector<int>({1432, 11, 19}));
	QVERIFY(CopticCalendar::copticFromFixed( 645554)==QVector<int>({1484, 10, 14}));
	QVERIFY(CopticCalendar::copticFromFixed( 664224)==QVector<int>({1535, 11, 27}));
	QVERIFY(CopticCalendar::copticFromFixed( 671401)==QVector<int>({1555,  7, 19}));
	QVERIFY(CopticCalendar::copticFromFixed( 694799)==QVector<int>({1619,  8, 11}));
	QVERIFY(CopticCalendar::copticFromFixed( 704424)==QVector<int>({1645, 12, 19}));
	QVERIFY(CopticCalendar::copticFromFixed( 708842)==QVector<int>({1658,  1, 19}));
	QVERIFY(CopticCalendar::copticFromFixed( 709409)==QVector<int>({1659,  8, 11}));
	QVERIFY(CopticCalendar::copticFromFixed( 709580)==QVector<int>({1660,  1, 26}));
	QVERIFY(CopticCalendar::copticFromFixed( 727274)==QVector<int>({1708,  7,  8}));
	QVERIFY(CopticCalendar::copticFromFixed( 728714)==QVector<int>({1712,  6, 17}));
	QVERIFY(CopticCalendar::copticFromFixed( 744313)==QVector<int>({1755,  3,  1}));
	QVERIFY(CopticCalendar::copticFromFixed( 764652)==QVector<int>({1810, 11, 11}));

	QVERIFY(-214193==EthiopicCalendar::fixedFromEthiopic({-594, 12,  6}));
	QVERIFY( -61387==EthiopicCalendar::fixedFromEthiopic({-175,  4, 12}));
	QVERIFY(  25469==EthiopicCalendar::fixedFromEthiopic({  63,  1, 29}));
	QVERIFY(  49217==EthiopicCalendar::fixedFromEthiopic({ 128,  2,  5}));
	QVERIFY( 171307==EthiopicCalendar::fixedFromEthiopic({ 462,  5, 12}));
	QVERIFY( 210155==EthiopicCalendar::fixedFromEthiopic({ 568,  9, 23}));
	QVERIFY( 253427==EthiopicCalendar::fixedFromEthiopic({ 687,  3, 11}));
	QVERIFY( 369740==EthiopicCalendar::fixedFromEthiopic({1005,  8, 24}));
	QVERIFY( 400085==EthiopicCalendar::fixedFromEthiopic({1088,  9, 23}));
	QVERIFY( 434355==EthiopicCalendar::fixedFromEthiopic({1182,  7, 20}));
	QVERIFY( 452605==EthiopicCalendar::fixedFromEthiopic({1232,  7,  7}));
	QVERIFY( 470160==EthiopicCalendar::fixedFromEthiopic({1280,  7, 30}));
	QVERIFY( 473837==EthiopicCalendar::fixedFromEthiopic({1290,  8, 25}));
	QVERIFY( 507850==EthiopicCalendar::fixedFromEthiopic({1383, 10, 10}));
	QVERIFY( 524156==EthiopicCalendar::fixedFromEthiopic({1428,  5, 29}));
	QVERIFY( 544676==EthiopicCalendar::fixedFromEthiopic({1484,  8,  5}));
	QVERIFY( 567118==EthiopicCalendar::fixedFromEthiopic({1546,  1, 12}));
	QVERIFY( 569477==EthiopicCalendar::fixedFromEthiopic({1552,  6, 29}));
	QVERIFY( 601716==EthiopicCalendar::fixedFromEthiopic({1640, 10,  6}));
	QVERIFY( 613424==EthiopicCalendar::fixedFromEthiopic({1672, 10, 26}));
	QVERIFY( 626596==EthiopicCalendar::fixedFromEthiopic({1708, 11, 19}));
	QVERIFY( 645554==EthiopicCalendar::fixedFromEthiopic({1760, 10, 14}));
	QVERIFY( 664224==EthiopicCalendar::fixedFromEthiopic({1811, 11, 27}));
	QVERIFY( 671401==EthiopicCalendar::fixedFromEthiopic({1831,  7, 19}));
	QVERIFY( 694799==EthiopicCalendar::fixedFromEthiopic({1895,  8, 11}));
	QVERIFY( 704424==EthiopicCalendar::fixedFromEthiopic({1921, 12, 19}));
	QVERIFY( 708842==EthiopicCalendar::fixedFromEthiopic({1934,  1, 19}));
	QVERIFY( 709409==EthiopicCalendar::fixedFromEthiopic({1935,  8, 11}));
	QVERIFY( 709580==EthiopicCalendar::fixedFromEthiopic({1936,  1, 26}));
	QVERIFY( 727274==EthiopicCalendar::fixedFromEthiopic({1984,  7,  8}));
	QVERIFY( 728714==EthiopicCalendar::fixedFromEthiopic({1988,  6, 17}));
	QVERIFY( 744313==EthiopicCalendar::fixedFromEthiopic({2031,  3,  1}));
	QVERIFY( 764652==EthiopicCalendar::fixedFromEthiopic({2086, 11, 11}));

	QVERIFY(EthiopicCalendar::ethiopicFromFixed(-214193)==QVector<int>({-594, 12,  6}));
	QVERIFY(EthiopicCalendar::ethiopicFromFixed( -61387)==QVector<int>({-175,  4, 12}));
	QVERIFY(EthiopicCalendar::ethiopicFromFixed(  25469)==QVector<int>({  63,  1, 29}));
	QVERIFY(EthiopicCalendar::ethiopicFromFixed(  49217)==QVector<int>({ 128,  2,  5}));
	QVERIFY(EthiopicCalendar::ethiopicFromFixed( 171307)==QVector<int>({ 462,  5, 12}));
	QVERIFY(EthiopicCalendar::ethiopicFromFixed( 210155)==QVector<int>({ 568,  9, 23}));
	QVERIFY(EthiopicCalendar::ethiopicFromFixed( 253427)==QVector<int>({ 687,  3, 11}));
	QVERIFY(EthiopicCalendar::ethiopicFromFixed( 369740)==QVector<int>({1005,  8, 24}));
	QVERIFY(EthiopicCalendar::ethiopicFromFixed( 400085)==QVector<int>({1088,  9, 23}));
	QVERIFY(EthiopicCalendar::ethiopicFromFixed( 434355)==QVector<int>({1182,  7, 20}));
	QVERIFY(EthiopicCalendar::ethiopicFromFixed( 452605)==QVector<int>({1232,  7,  7}));
	QVERIFY(EthiopicCalendar::ethiopicFromFixed( 470160)==QVector<int>({1280,  7, 30}));
	QVERIFY(EthiopicCalendar::ethiopicFromFixed( 473837)==QVector<int>({1290,  8, 25}));
	QVERIFY(EthiopicCalendar::ethiopicFromFixed( 507850)==QVector<int>({1383, 10, 10}));
	QVERIFY(EthiopicCalendar::ethiopicFromFixed( 524156)==QVector<int>({1428,  5, 29}));
	QVERIFY(EthiopicCalendar::ethiopicFromFixed( 544676)==QVector<int>({1484,  8,  5}));
	QVERIFY(EthiopicCalendar::ethiopicFromFixed( 567118)==QVector<int>({1546,  1, 12}));
	QVERIFY(EthiopicCalendar::ethiopicFromFixed( 569477)==QVector<int>({1552,  6, 29}));
	QVERIFY(EthiopicCalendar::ethiopicFromFixed( 601716)==QVector<int>({1640, 10,  6}));
	QVERIFY(EthiopicCalendar::ethiopicFromFixed( 613424)==QVector<int>({1672, 10, 26}));
	QVERIFY(EthiopicCalendar::ethiopicFromFixed( 626596)==QVector<int>({1708, 11, 19}));
	QVERIFY(EthiopicCalendar::ethiopicFromFixed( 645554)==QVector<int>({1760, 10, 14}));
	QVERIFY(EthiopicCalendar::ethiopicFromFixed( 664224)==QVector<int>({1811, 11, 27}));
	QVERIFY(EthiopicCalendar::ethiopicFromFixed( 671401)==QVector<int>({1831,  7, 19}));
	QVERIFY(EthiopicCalendar::ethiopicFromFixed( 694799)==QVector<int>({1895,  8, 11}));
	QVERIFY(EthiopicCalendar::ethiopicFromFixed( 704424)==QVector<int>({1921, 12, 19}));
	QVERIFY(EthiopicCalendar::ethiopicFromFixed( 708842)==QVector<int>({1934,  1, 19}));
	QVERIFY(EthiopicCalendar::ethiopicFromFixed( 709409)==QVector<int>({1935,  8, 11}));
	QVERIFY(EthiopicCalendar::ethiopicFromFixed( 709580)==QVector<int>({1936,  1, 26}));
	QVERIFY(EthiopicCalendar::ethiopicFromFixed( 727274)==QVector<int>({1984,  7,  8}));
	QVERIFY(EthiopicCalendar::ethiopicFromFixed( 728714)==QVector<int>({1988,  6, 17}));
	QVERIFY(EthiopicCalendar::ethiopicFromFixed( 744313)==QVector<int>({2031,  3,  1}));
	QVERIFY(EthiopicCalendar::ethiopicFromFixed( 764652)==QVector<int>({2086, 11, 11}));
}

void TestCalendars::testRoman()
{
	QVERIFY(-214193==RomanCalendar::fixedFromRoman({-587,  8, 1,  3, 0}));
	QVERIFY( -61387==RomanCalendar::fixedFromRoman({-169, 12, 3,  6, 0}));
	QVERIFY(  25469==RomanCalendar::fixedFromRoman({  70, 10, 1,  6, 0}));
	QVERIFY(  49217==RomanCalendar::fixedFromRoman({ 135, 10, 2,  5, 0}));
	QVERIFY( 171307==RomanCalendar::fixedFromRoman({ 470,  1, 3,  7, 0}));
	QVERIFY( 210155==RomanCalendar::fixedFromRoman({ 576,  6, 1, 15, 0}));
	QVERIFY( 253427==RomanCalendar::fixedFromRoman({ 694, 11, 3,  7, 0}));
	QVERIFY( 369740==RomanCalendar::fixedFromRoman({1013,  5, 1, 13, 0}));
	QVERIFY( 400085==RomanCalendar::fixedFromRoman({1096,  6, 1, 15, 0}));
	QVERIFY( 434355==RomanCalendar::fixedFromRoman({1190,  4, 1, 17, 0}));
	QVERIFY( 452605==RomanCalendar::fixedFromRoman({1240,  3, 2,  5, 0}));
	QVERIFY( 470160==RomanCalendar::fixedFromRoman({1288,  4, 1,  7, 0}));
	QVERIFY( 473837==RomanCalendar::fixedFromRoman({1298,  5, 1, 12, 0}));
	QVERIFY( 507850==RomanCalendar::fixedFromRoman({1391,  6, 2,  2, 0}));
	QVERIFY( 524156==RomanCalendar::fixedFromRoman({1436,  2, 1,  8, 0}));
	QVERIFY( 544676==RomanCalendar::fixedFromRoman({1492,  4, 1,  2, 0}));
	QVERIFY( 567118==RomanCalendar::fixedFromRoman({1553,  9, 3,  5, 0}));
	QVERIFY( 569477==RomanCalendar::fixedFromRoman({1560,  3, 1,  6, 0}));
	QVERIFY( 601716==RomanCalendar::fixedFromRoman({1648,  6, 1,  2, 0}));
	QVERIFY( 613424==RomanCalendar::fixedFromRoman({1680,  7, 1, 12, 0}));
	QVERIFY( 626596==RomanCalendar::fixedFromRoman({1716,  7, 3,  3, 0}));
	QVERIFY( 645554==RomanCalendar::fixedFromRoman({1768,  6, 3,  6, 0}));
	QVERIFY( 664224==RomanCalendar::fixedFromRoman({1819,  8, 1, 12, 0}));
	QVERIFY( 671401==RomanCalendar::fixedFromRoman({1839,  3, 3,  1, 0}));
	QVERIFY( 694799==RomanCalendar::fixedFromRoman({1903,  4, 3,  8, 0}));
	QVERIFY( 704424==RomanCalendar::fixedFromRoman({1929,  8, 3,  2, 0}));
	QVERIFY( 708842==RomanCalendar::fixedFromRoman({1941, 10, 1, 16, 0}));
	QVERIFY( 709409==RomanCalendar::fixedFromRoman({1943,  4, 3,  8, 0}));
	QVERIFY( 709580==RomanCalendar::fixedFromRoman({1943, 10, 1,  8, 0}));
	QVERIFY( 727274==RomanCalendar::fixedFromRoman({1992,  3, 2,  4, 0}));
	QVERIFY( 728714==RomanCalendar::fixedFromRoman({1996,  2, 3,  2, 0}));
	QVERIFY( 744313==RomanCalendar::fixedFromRoman({2038, 11, 1,  5, 0}));
	QVERIFY( 764652==RomanCalendar::fixedFromRoman({2094,  7, 2,  3, 0}));

	QVERIFY(RomanCalendar::romanFromFixed(-214193)==QVector<int>({-587,  8, 1,  3, 0}));
	QVERIFY(RomanCalendar::romanFromFixed( -61387)==QVector<int>({-169, 12, 3,  6, 0}));
	QVERIFY(RomanCalendar::romanFromFixed(  25469)==QVector<int>({  70, 10, 1,  6, 0}));
	QVERIFY(RomanCalendar::romanFromFixed(  49217)==QVector<int>({ 135, 10, 2,  5, 0}));
	QVERIFY(RomanCalendar::romanFromFixed( 171307)==QVector<int>({ 470,  1, 3,  7, 0}));
	QVERIFY(RomanCalendar::romanFromFixed( 210155)==QVector<int>({ 576,  6, 1, 15, 0}));
	QVERIFY(RomanCalendar::romanFromFixed( 253427)==QVector<int>({ 694, 11, 3,  7, 0}));
	QVERIFY(RomanCalendar::romanFromFixed( 369740)==QVector<int>({1013,  5, 1, 13, 0}));
	QVERIFY(RomanCalendar::romanFromFixed( 400085)==QVector<int>({1096,  6, 1, 15, 0}));
	QVERIFY(RomanCalendar::romanFromFixed( 434355)==QVector<int>({1190,  4, 1, 17, 0}));
	QVERIFY(RomanCalendar::romanFromFixed( 452605)==QVector<int>({1240,  3, 2,  5, 0}));
	QVERIFY(RomanCalendar::romanFromFixed( 470160)==QVector<int>({1288,  4, 1,  7, 0}));
	QVERIFY(RomanCalendar::romanFromFixed( 473837)==QVector<int>({1298,  5, 1, 12, 0}));
	QVERIFY(RomanCalendar::romanFromFixed( 507850)==QVector<int>({1391,  6, 2,  2, 0}));
	QVERIFY(RomanCalendar::romanFromFixed( 524156)==QVector<int>({1436,  2, 1,  8, 0}));
	QVERIFY(RomanCalendar::romanFromFixed( 544676)==QVector<int>({1492,  4, 1,  2, 0}));
	QVERIFY(RomanCalendar::romanFromFixed( 567118)==QVector<int>({1553,  9, 3,  5, 0}));
	QVERIFY(RomanCalendar::romanFromFixed( 569477)==QVector<int>({1560,  3, 1,  6, 0}));
	QVERIFY(RomanCalendar::romanFromFixed( 601716)==QVector<int>({1648,  6, 1,  2, 0}));
	QVERIFY(RomanCalendar::romanFromFixed( 613424)==QVector<int>({1680,  7, 1, 12, 0}));
	QVERIFY(RomanCalendar::romanFromFixed( 626596)==QVector<int>({1716,  7, 3,  3, 0}));
	QVERIFY(RomanCalendar::romanFromFixed( 645554)==QVector<int>({1768,  6, 3,  6, 0}));
	QVERIFY(RomanCalendar::romanFromFixed( 664224)==QVector<int>({1819,  8, 1, 12, 0}));
	QVERIFY(RomanCalendar::romanFromFixed( 671401)==QVector<int>({1839,  3, 3,  1, 0}));
	QVERIFY(RomanCalendar::romanFromFixed( 694799)==QVector<int>({1903,  4, 3,  8, 0}));
	QVERIFY(RomanCalendar::romanFromFixed( 704424)==QVector<int>({1929,  8, 3,  2, 0}));
	QVERIFY(RomanCalendar::romanFromFixed( 708842)==QVector<int>({1941, 10, 1, 16, 0}));
	QVERIFY(RomanCalendar::romanFromFixed( 709409)==QVector<int>({1943,  4, 3,  8, 0}));
	QVERIFY(RomanCalendar::romanFromFixed( 709580)==QVector<int>({1943, 10, 1,  8, 0}));
	QVERIFY(RomanCalendar::romanFromFixed( 727274)==QVector<int>({1992,  3, 2,  4, 0}));
	QVERIFY(RomanCalendar::romanFromFixed( 728714)==QVector<int>({1996,  2, 3,  2, 0}));
	QVERIFY(RomanCalendar::romanFromFixed( 744313)==QVector<int>({2038, 11, 1,  5, 0}));
	QVERIFY(RomanCalendar::romanFromFixed( 764652)==QVector<int>({2094,  7, 2,  3, 0}));
}

void TestCalendars::testFrenchRevolution()
{
	QVERIFY(-214193==FrenchArithmeticCalendar::fixedFromFrenchArithmetic({-2378, 11,  4}));
	QVERIFY( -61387==FrenchArithmeticCalendar::fixedFromFrenchArithmetic({-1959,  3, 13}));
	QVERIFY(  25469==FrenchArithmeticCalendar::fixedFromFrenchArithmetic({-1721,  1,  2}));
	QVERIFY(  49217==FrenchArithmeticCalendar::fixedFromFrenchArithmetic({-1656,  1, 10}));
	QVERIFY( 171307==FrenchArithmeticCalendar::fixedFromFrenchArithmetic({-1322,  4, 18}));
	QVERIFY( 210155==FrenchArithmeticCalendar::fixedFromFrenchArithmetic({-1216,  9,  1}));
	QVERIFY( 253427==FrenchArithmeticCalendar::fixedFromFrenchArithmetic({-1097,  2, 19}));
	QVERIFY( 369740==FrenchArithmeticCalendar::fixedFromFrenchArithmetic({ -779,  8,  4}));
	QVERIFY( 400085==FrenchArithmeticCalendar::fixedFromFrenchArithmetic({ -696,  9,  5}));
	QVERIFY( 434355==FrenchArithmeticCalendar::fixedFromFrenchArithmetic({ -602,  7,  1}));
	QVERIFY( 452605==FrenchArithmeticCalendar::fixedFromFrenchArithmetic({ -552,  6, 20}));
	QVERIFY( 470160==FrenchArithmeticCalendar::fixedFromFrenchArithmetic({ -504,  7, 13}));
	QVERIFY( 473837==FrenchArithmeticCalendar::fixedFromFrenchArithmetic({ -494,  8,  8}));
	QVERIFY( 507850==FrenchArithmeticCalendar::fixedFromFrenchArithmetic({ -401,  9, 23}));
	QVERIFY( 524156==FrenchArithmeticCalendar::fixedFromFrenchArithmetic({ -356,  5, 13}));
	QVERIFY( 544676==FrenchArithmeticCalendar::fixedFromFrenchArithmetic({ -300,  7, 19}));
	QVERIFY( 567118==FrenchArithmeticCalendar::fixedFromFrenchArithmetic({ -239, 13,  1}));
	QVERIFY( 569477==FrenchArithmeticCalendar::fixedFromFrenchArithmetic({ -232,  6, 14}));
	QVERIFY( 601716==FrenchArithmeticCalendar::fixedFromFrenchArithmetic({ -144,  9, 22}));
	QVERIFY( 613424==FrenchArithmeticCalendar::fixedFromFrenchArithmetic({ -112, 10, 12}));
	QVERIFY( 626596==FrenchArithmeticCalendar::fixedFromFrenchArithmetic({  -76, 11,  6}));
	QVERIFY( 645554==FrenchArithmeticCalendar::fixedFromFrenchArithmetic({  -24, 10,  1}));
	QVERIFY( 664224==FrenchArithmeticCalendar::fixedFromFrenchArithmetic({   27, 11, 14}));
	QVERIFY( 671401==FrenchArithmeticCalendar::fixedFromFrenchArithmetic({   47,  7,  6}));
	QVERIFY( 694799==FrenchArithmeticCalendar::fixedFromFrenchArithmetic({  111,  7, 29}));
	QVERIFY( 704424==FrenchArithmeticCalendar::fixedFromFrenchArithmetic({  137, 12,  7}));
	QVERIFY( 708842==FrenchArithmeticCalendar::fixedFromFrenchArithmetic({  150,  1,  7}));
	QVERIFY( 709409==FrenchArithmeticCalendar::fixedFromFrenchArithmetic({  151,  7, 29}));
	QVERIFY( 709580==FrenchArithmeticCalendar::fixedFromFrenchArithmetic({  152,  1, 15}));
	QVERIFY( 727274==FrenchArithmeticCalendar::fixedFromFrenchArithmetic({  200,  6, 27}));
	QVERIFY( 728714==FrenchArithmeticCalendar::fixedFromFrenchArithmetic({  204,  6,  7}));
	QVERIFY( 744313==FrenchArithmeticCalendar::fixedFromFrenchArithmetic({  247,  2, 20}));
	QVERIFY( 764652==FrenchArithmeticCalendar::fixedFromFrenchArithmetic({  302, 11,  1}));

	QVERIFY(FrenchArithmeticCalendar::frenchArithmeticFromFixed(-214193)==QVector<int>({-2378, 11,  4}));
	QVERIFY(FrenchArithmeticCalendar::frenchArithmeticFromFixed( -61387)==QVector<int>({-1959,  3, 13}));
	QVERIFY(FrenchArithmeticCalendar::frenchArithmeticFromFixed(  25469)==QVector<int>({-1721,  1,  2}));
	QVERIFY(FrenchArithmeticCalendar::frenchArithmeticFromFixed(  49217)==QVector<int>({-1656,  1, 10}));
	QVERIFY(FrenchArithmeticCalendar::frenchArithmeticFromFixed( 171307)==QVector<int>({-1322,  4, 18}));
	QVERIFY(FrenchArithmeticCalendar::frenchArithmeticFromFixed( 210155)==QVector<int>({-1216,  9,  1}));
	QVERIFY(FrenchArithmeticCalendar::frenchArithmeticFromFixed( 253427)==QVector<int>({-1097,  2, 19}));
	QVERIFY(FrenchArithmeticCalendar::frenchArithmeticFromFixed( 369740)==QVector<int>({ -779,  8,  4}));
	QVERIFY(FrenchArithmeticCalendar::frenchArithmeticFromFixed( 400085)==QVector<int>({ -696,  9,  5}));
	QVERIFY(FrenchArithmeticCalendar::frenchArithmeticFromFixed( 434355)==QVector<int>({ -602,  7,  1}));
	QVERIFY(FrenchArithmeticCalendar::frenchArithmeticFromFixed( 452605)==QVector<int>({ -552,  6, 20}));
	QVERIFY(FrenchArithmeticCalendar::frenchArithmeticFromFixed( 470160)==QVector<int>({ -504,  7, 13}));
	QVERIFY(FrenchArithmeticCalendar::frenchArithmeticFromFixed( 473837)==QVector<int>({ -494,  8,  8}));
	QVERIFY(FrenchArithmeticCalendar::frenchArithmeticFromFixed( 507850)==QVector<int>({ -401,  9, 23}));
	QVERIFY(FrenchArithmeticCalendar::frenchArithmeticFromFixed( 524156)==QVector<int>({ -356,  5, 13}));
	QVERIFY(FrenchArithmeticCalendar::frenchArithmeticFromFixed( 544676)==QVector<int>({ -300,  7, 19}));
	QVERIFY(FrenchArithmeticCalendar::frenchArithmeticFromFixed( 567118)==QVector<int>({ -239, 13,  1}));
	QVERIFY(FrenchArithmeticCalendar::frenchArithmeticFromFixed( 569477)==QVector<int>({ -232,  6, 14}));
	QVERIFY(FrenchArithmeticCalendar::frenchArithmeticFromFixed( 601716)==QVector<int>({ -144,  9, 22}));
	QVERIFY(FrenchArithmeticCalendar::frenchArithmeticFromFixed( 613424)==QVector<int>({ -112, 10, 12}));
	QVERIFY(FrenchArithmeticCalendar::frenchArithmeticFromFixed( 626596)==QVector<int>({  -76, 11,  6}));
	QVERIFY(FrenchArithmeticCalendar::frenchArithmeticFromFixed( 645554)==QVector<int>({  -24, 10,  1}));
	QVERIFY(FrenchArithmeticCalendar::frenchArithmeticFromFixed( 664224)==QVector<int>({   27, 11, 14}));
	QVERIFY(FrenchArithmeticCalendar::frenchArithmeticFromFixed( 671401)==QVector<int>({   47,  7,  6}));
	QVERIFY(FrenchArithmeticCalendar::frenchArithmeticFromFixed( 694799)==QVector<int>({  111,  7, 29}));
	QVERIFY(FrenchArithmeticCalendar::frenchArithmeticFromFixed( 704424)==QVector<int>({  137, 12,  7}));
	QVERIFY(FrenchArithmeticCalendar::frenchArithmeticFromFixed( 708842)==QVector<int>({  150,  1,  7}));
	QVERIFY(FrenchArithmeticCalendar::frenchArithmeticFromFixed( 709409)==QVector<int>({  151,  7, 29}));
	QVERIFY(FrenchArithmeticCalendar::frenchArithmeticFromFixed( 709580)==QVector<int>({  152,  1, 15}));
	QVERIFY(FrenchArithmeticCalendar::frenchArithmeticFromFixed( 727274)==QVector<int>({  200,  6, 27}));
	QVERIFY(FrenchArithmeticCalendar::frenchArithmeticFromFixed( 728714)==QVector<int>({  204,  6,  7}));
	QVERIFY(FrenchArithmeticCalendar::frenchArithmeticFromFixed( 744313)==QVector<int>({  247,  2, 20}));
	QVERIFY(FrenchArithmeticCalendar::frenchArithmeticFromFixed( 764652)==QVector<int>({  302, 11,  1}));
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

	QVERIFY(-214193==MayaLongCountCalendar::fixedFromMayanLongCount({ 6,  8,  3, 13,  9}));
	QVERIFY( -61387==MayaLongCountCalendar::fixedFromMayanLongCount({ 7,  9,  8,  3, 15}));
	QVERIFY(  25469==MayaLongCountCalendar::fixedFromMayanLongCount({ 8,  1,  9,  8, 11}));
	QVERIFY(  49217==MayaLongCountCalendar::fixedFromMayanLongCount({ 8,  4, 15,  7, 19}));
	QVERIFY( 171307==MayaLongCountCalendar::fixedFromMayanLongCount({ 9,  1, 14, 10,  9}));
	QVERIFY( 210155==MayaLongCountCalendar::fixedFromMayanLongCount({ 9,  7,  2,  8, 17}));
	QVERIFY( 253427==MayaLongCountCalendar::fixedFromMayanLongCount({ 9, 13,  2, 12,  9}));
	QVERIFY( 369740==MayaLongCountCalendar::fixedFromMayanLongCount({10,  9,  5, 14,  2}));
	QVERIFY( 400085==MayaLongCountCalendar::fixedFromMayanLongCount({10, 13, 10,  1,  7}));
	QVERIFY( 434355==MayaLongCountCalendar::fixedFromMayanLongCount({10, 18,  5,  4, 17}));
	QVERIFY( 452605==MayaLongCountCalendar::fixedFromMayanLongCount({11,  0, 15, 17,  7}));
	QVERIFY( 470160==MayaLongCountCalendar::fixedFromMayanLongCount({11,  3,  4, 13,  2}));
	QVERIFY( 473837==MayaLongCountCalendar::fixedFromMayanLongCount({11,  3, 14, 16, 19}));
	QVERIFY( 507850==MayaLongCountCalendar::fixedFromMayanLongCount({11,  8,  9,  7, 12}));
	QVERIFY( 524156==MayaLongCountCalendar::fixedFromMayanLongCount({11, 10, 14, 12, 18}));
	QVERIFY( 544676==MayaLongCountCalendar::fixedFromMayanLongCount({11, 13, 11, 12, 18}));
	QVERIFY( 567118==MayaLongCountCalendar::fixedFromMayanLongCount({11, 16, 14,  1,  0}));
	QVERIFY( 569477==MayaLongCountCalendar::fixedFromMayanLongCount({11, 17,  0, 10, 19}));
	QVERIFY( 601716==MayaLongCountCalendar::fixedFromMayanLongCount({12,  1, 10,  2, 18}));
	QVERIFY( 613424==MayaLongCountCalendar::fixedFromMayanLongCount({12,  3,  2, 12,  6}));
	QVERIFY( 626596==MayaLongCountCalendar::fixedFromMayanLongCount({12,  4, 19,  4, 18}));
	QVERIFY( 645554==MayaLongCountCalendar::fixedFromMayanLongCount({12,  7, 11, 16, 16}));
	QVERIFY( 664224==MayaLongCountCalendar::fixedFromMayanLongCount({12, 10,  3, 14,  6}));
	QVERIFY( 671401==MayaLongCountCalendar::fixedFromMayanLongCount({12, 11,  3, 13,  3}));
	QVERIFY( 694799==MayaLongCountCalendar::fixedFromMayanLongCount({12, 14,  8, 13,  1}));
	QVERIFY( 704424==MayaLongCountCalendar::fixedFromMayanLongCount({12, 15, 15,  8,  6}));
	QVERIFY( 708842==MayaLongCountCalendar::fixedFromMayanLongCount({12, 16,  7, 13,  4}));
	QVERIFY( 709409==MayaLongCountCalendar::fixedFromMayanLongCount({12, 16,  9,  5, 11}));
	QVERIFY( 709580==MayaLongCountCalendar::fixedFromMayanLongCount({12, 16,  9, 14,  2}));
	QVERIFY( 727274==MayaLongCountCalendar::fixedFromMayanLongCount({12, 18, 18, 16, 16}));
	QVERIFY( 728714==MayaLongCountCalendar::fixedFromMayanLongCount({12, 19,  2, 16, 16}));
	QVERIFY( 744313==MayaLongCountCalendar::fixedFromMayanLongCount({13,  1,  6,  4, 15}));
	QVERIFY( 764652==MayaLongCountCalendar::fixedFromMayanLongCount({13,  4,  2, 13, 14}));

	QVERIFY(MayaLongCountCalendar::mayaLongCountFromFixed(-214193)==QVector<int>({ 6,  8,  3, 13,  9}));
	QVERIFY(MayaLongCountCalendar::mayaLongCountFromFixed( -61387)==QVector<int>({ 7,  9,  8,  3, 15}));
	QVERIFY(MayaLongCountCalendar::mayaLongCountFromFixed(  25469)==QVector<int>({ 8,  1,  9,  8, 11}));
	QVERIFY(MayaLongCountCalendar::mayaLongCountFromFixed(  49217)==QVector<int>({ 8,  4, 15,  7, 19}));
	QVERIFY(MayaLongCountCalendar::mayaLongCountFromFixed( 171307)==QVector<int>({ 9,  1, 14, 10,  9}));
	QVERIFY(MayaLongCountCalendar::mayaLongCountFromFixed( 210155)==QVector<int>({ 9,  7,  2,  8, 17}));
	QVERIFY(MayaLongCountCalendar::mayaLongCountFromFixed( 253427)==QVector<int>({ 9, 13,  2, 12,  9}));
	QVERIFY(MayaLongCountCalendar::mayaLongCountFromFixed( 369740)==QVector<int>({10,  9,  5, 14,  2}));
	QVERIFY(MayaLongCountCalendar::mayaLongCountFromFixed( 400085)==QVector<int>({10, 13, 10,  1,  7}));
	QVERIFY(MayaLongCountCalendar::mayaLongCountFromFixed( 434355)==QVector<int>({10, 18,  5,  4, 17}));
	QVERIFY(MayaLongCountCalendar::mayaLongCountFromFixed( 452605)==QVector<int>({11,  0, 15, 17,  7}));
	QVERIFY(MayaLongCountCalendar::mayaLongCountFromFixed( 470160)==QVector<int>({11,  3,  4, 13,  2}));
	QVERIFY(MayaLongCountCalendar::mayaLongCountFromFixed( 473837)==QVector<int>({11,  3, 14, 16, 19}));
	QVERIFY(MayaLongCountCalendar::mayaLongCountFromFixed( 507850)==QVector<int>({11,  8,  9,  7, 12}));
	QVERIFY(MayaLongCountCalendar::mayaLongCountFromFixed( 524156)==QVector<int>({11, 10, 14, 12, 18}));
	QVERIFY(MayaLongCountCalendar::mayaLongCountFromFixed( 544676)==QVector<int>({11, 13, 11, 12, 18}));
	QVERIFY(MayaLongCountCalendar::mayaLongCountFromFixed( 567118)==QVector<int>({11, 16, 14,  1,  0}));
	QVERIFY(MayaLongCountCalendar::mayaLongCountFromFixed( 569477)==QVector<int>({11, 17,  0, 10, 19}));
	QVERIFY(MayaLongCountCalendar::mayaLongCountFromFixed( 601716)==QVector<int>({12,  1, 10,  2, 18}));
	QVERIFY(MayaLongCountCalendar::mayaLongCountFromFixed( 613424)==QVector<int>({12,  3,  2, 12,  6}));
	QVERIFY(MayaLongCountCalendar::mayaLongCountFromFixed( 626596)==QVector<int>({12,  4, 19,  4, 18}));
	QVERIFY(MayaLongCountCalendar::mayaLongCountFromFixed( 645554)==QVector<int>({12,  7, 11, 16, 16}));
	QVERIFY(MayaLongCountCalendar::mayaLongCountFromFixed( 664224)==QVector<int>({12, 10,  3, 14,  6}));
	QVERIFY(MayaLongCountCalendar::mayaLongCountFromFixed( 671401)==QVector<int>({12, 11,  3, 13,  3}));
	QVERIFY(MayaLongCountCalendar::mayaLongCountFromFixed( 694799)==QVector<int>({12, 14,  8, 13,  1}));
	QVERIFY(MayaLongCountCalendar::mayaLongCountFromFixed( 704424)==QVector<int>({12, 15, 15,  8,  6}));
	QVERIFY(MayaLongCountCalendar::mayaLongCountFromFixed( 708842)==QVector<int>({12, 16,  7, 13,  4}));
	QVERIFY(MayaLongCountCalendar::mayaLongCountFromFixed( 709409)==QVector<int>({12, 16,  9,  5, 11}));
	QVERIFY(MayaLongCountCalendar::mayaLongCountFromFixed( 709580)==QVector<int>({12, 16,  9, 14,  2}));
	QVERIFY(MayaLongCountCalendar::mayaLongCountFromFixed( 727274)==QVector<int>({12, 18, 18, 16, 16}));
	QVERIFY(MayaLongCountCalendar::mayaLongCountFromFixed( 728714)==QVector<int>({12, 19,  2, 16, 16}));
	QVERIFY(MayaLongCountCalendar::mayaLongCountFromFixed( 744313)==QVector<int>({13,  1,  6,  4, 15}));
	QVERIFY(MayaLongCountCalendar::mayaLongCountFromFixed( 764652)==QVector<int>({13,  4,  2, 13, 14}));

	QVERIFY(MayaHaabCalendar::mayanHaabFromFixed(-214193)==QVector<int>({11, 12}));
	QVERIFY(MayaHaabCalendar::mayanHaabFromFixed( -61387)==QVector<int>({ 5,  3}));
	QVERIFY(MayaHaabCalendar::mayanHaabFromFixed(  25469)==QVector<int>({ 4,  9}));
	QVERIFY(MayaHaabCalendar::mayanHaabFromFixed(  49217)==QVector<int>({ 5, 12}));
	QVERIFY(MayaHaabCalendar::mayanHaabFromFixed( 171307)==QVector<int>({14, 12}));
	QVERIFY(MayaHaabCalendar::mayanHaabFromFixed( 210155)==QVector<int>({ 4,  5}));
	QVERIFY(MayaHaabCalendar::mayanHaabFromFixed( 253427)==QVector<int>({14,  7}));
	QVERIFY(MayaHaabCalendar::mayanHaabFromFixed( 369740)==QVector<int>({ 8,  5}));
	QVERIFY(MayaHaabCalendar::mayanHaabFromFixed( 400085)==QVector<int>({10, 15}));
	QVERIFY(MayaHaabCalendar::mayanHaabFromFixed( 434355)==QVector<int>({ 8, 15}));
	QVERIFY(MayaHaabCalendar::mayanHaabFromFixed( 452605)==QVector<int>({ 8, 15}));
	QVERIFY(MayaHaabCalendar::mayanHaabFromFixed( 470160)==QVector<int>({10, 10}));
	QVERIFY(MayaHaabCalendar::mayanHaabFromFixed( 473837)==QVector<int>({11, 17}));
	QVERIFY(MayaHaabCalendar::mayanHaabFromFixed( 507850)==QVector<int>({15,  5}));
	QVERIFY(MayaHaabCalendar::mayanHaabFromFixed( 524156)==QVector<int>({ 9,  6}));
	QVERIFY(MayaHaabCalendar::mayanHaabFromFixed( 544676)==QVector<int>({13,  6}));
	QVERIFY(MayaHaabCalendar::mayanHaabFromFixed( 567118)==QVector<int>({ 3, 18}));
	QVERIFY(MayaHaabCalendar::mayanHaabFromFixed( 569477)==QVector<int>({12,  7}));
	QVERIFY(MayaHaabCalendar::mayanHaabFromFixed( 601716)==QVector<int>({18,  6}));
	QVERIFY(MayaHaabCalendar::mayanHaabFromFixed( 613424)==QVector<int>({ 1,  9}));
	QVERIFY(MayaHaabCalendar::mayanHaabFromFixed( 626596)==QVector<int>({ 3,  1}));
	QVERIFY(MayaHaabCalendar::mayanHaabFromFixed( 645554)==QVector<int>({ 1, 19}));
	QVERIFY(MayaHaabCalendar::mayanHaabFromFixed( 664224)==QVector<int>({ 4, 14}));
	QVERIFY(MayaHaabCalendar::mayanHaabFromFixed( 671401)==QVector<int>({16, 16}));
	QVERIFY(MayaHaabCalendar::mayanHaabFromFixed( 694799)==QVector<int>({18, 14}));
	QVERIFY(MayaHaabCalendar::mayanHaabFromFixed( 704424)==QVector<int>({ 7,  4}));
	QVERIFY(MayaHaabCalendar::mayanHaabFromFixed( 708842)==QVector<int>({ 9,  2}));
	QVERIFY(MayaHaabCalendar::mayanHaabFromFixed( 709409)==QVector<int>({19,  4}));
	QVERIFY(MayaHaabCalendar::mayanHaabFromFixed( 709580)==QVector<int>({ 9, 10}));
	QVERIFY(MayaHaabCalendar::mayanHaabFromFixed( 727274)==QVector<int>({18,  4}));
	QVERIFY(MayaHaabCalendar::mayanHaabFromFixed( 728714)==QVector<int>({17,  4}));
	QVERIFY(MayaHaabCalendar::mayanHaabFromFixed( 744313)==QVector<int>({12,  8}));
	QVERIFY(MayaHaabCalendar::mayanHaabFromFixed( 764652)==QVector<int>({ 7,  7}));

	QVERIFY(MayaTzolkinCalendar::mayanTzolkinFromFixed(-214193)==QVector<int>({ 5,  9}));
	QVERIFY(MayaTzolkinCalendar::mayanTzolkinFromFixed( -61387)==QVector<int>({ 9, 15}));
	QVERIFY(MayaTzolkinCalendar::mayanTzolkinFromFixed(  25469)==QVector<int>({12, 11}));
	QVERIFY(MayaTzolkinCalendar::mayanTzolkinFromFixed(  49217)==QVector<int>({ 9, 19}));
	QVERIFY(MayaTzolkinCalendar::mayanTzolkinFromFixed( 171307)==QVector<int>({ 3,  9}));
	QVERIFY(MayaTzolkinCalendar::mayanTzolkinFromFixed( 210155)==QVector<int>({ 7, 17}));
	QVERIFY(MayaTzolkinCalendar::mayanTzolkinFromFixed( 253427)==QVector<int>({ 2,  9}));
	QVERIFY(MayaTzolkinCalendar::mayanTzolkinFromFixed( 369740)==QVector<int>({ 4,  2}));
	QVERIFY(MayaTzolkinCalendar::mayanTzolkinFromFixed( 400085)==QVector<int>({ 7,  7}));
	QVERIFY(MayaTzolkinCalendar::mayanTzolkinFromFixed( 434355)==QVector<int>({ 9, 17}));
	QVERIFY(MayaTzolkinCalendar::mayanTzolkinFromFixed( 452605)==QVector<int>({ 7,  7}));
	QVERIFY(MayaTzolkinCalendar::mayanTzolkinFromFixed( 470160)==QVector<int>({12,  2}));
	QVERIFY(MayaTzolkinCalendar::mayanTzolkinFromFixed( 473837)==QVector<int>({10, 19}));
	QVERIFY(MayaTzolkinCalendar::mayanTzolkinFromFixed( 507850)==QVector<int>({ 2, 12}));
	QVERIFY(MayaTzolkinCalendar::mayanTzolkinFromFixed( 524156)==QVector<int>({ 6, 18}));
	QVERIFY(MayaTzolkinCalendar::mayanTzolkinFromFixed( 544676)==QVector<int>({12, 18}));
	QVERIFY(MayaTzolkinCalendar::mayanTzolkinFromFixed( 567118)==QVector<int>({ 3, 20}));
	QVERIFY(MayaTzolkinCalendar::mayanTzolkinFromFixed( 569477)==QVector<int>({ 9, 19}));
	QVERIFY(MayaTzolkinCalendar::mayanTzolkinFromFixed( 601716)==QVector<int>({ 8, 18}));
	QVERIFY(MayaTzolkinCalendar::mayanTzolkinFromFixed( 613424)==QVector<int>({ 3,  6}));
	QVERIFY(MayaTzolkinCalendar::mayanTzolkinFromFixed( 626596)==QVector<int>({ 6, 18}));
	QVERIFY(MayaTzolkinCalendar::mayanTzolkinFromFixed( 645554)==QVector<int>({10, 16}));
	QVERIFY(MayaTzolkinCalendar::mayanTzolkinFromFixed( 664224)==QVector<int>({12,  6}));
	QVERIFY(MayaTzolkinCalendar::mayanTzolkinFromFixed( 671401)==QVector<int>({13,  3}));
	QVERIFY(MayaTzolkinCalendar::mayanTzolkinFromFixed( 694799)==QVector<int>({11,  1}));
	QVERIFY(MayaTzolkinCalendar::mayanTzolkinFromFixed( 704424)==QVector<int>({ 3,  6}));
	QVERIFY(MayaTzolkinCalendar::mayanTzolkinFromFixed( 708842)==QVector<int>({ 1,  4}));
	QVERIFY(MayaTzolkinCalendar::mayanTzolkinFromFixed( 709409)==QVector<int>({ 9, 11}));
	QVERIFY(MayaTzolkinCalendar::mayanTzolkinFromFixed( 709580)==QVector<int>({11,  2}));
	QVERIFY(MayaTzolkinCalendar::mayanTzolkinFromFixed( 727274)==QVector<int>({12, 16}));
	QVERIFY(MayaTzolkinCalendar::mayanTzolkinFromFixed( 728714)==QVector<int>({ 9, 16}));
	QVERIFY(MayaTzolkinCalendar::mayanTzolkinFromFixed( 744313)==QVector<int>({ 8, 15}));
	QVERIFY(MayaTzolkinCalendar::mayanTzolkinFromFixed( 764652)==QVector<int>({ 2, 14}));

	QVERIFY(AztecXihuitlCalendar::aztecXihuitlFromFixed(-214193)==QVector<int>({ 2,  6}));
	QVERIFY(AztecXihuitlCalendar::aztecXihuitlFromFixed( -61387)==QVector<int>({14,  2}));
	QVERIFY(AztecXihuitlCalendar::aztecXihuitlFromFixed(  25469)==QVector<int>({13,  8}));
	QVERIFY(AztecXihuitlCalendar::aztecXihuitlFromFixed(  49217)==QVector<int>({14, 11}));
	QVERIFY(AztecXihuitlCalendar::aztecXihuitlFromFixed( 171307)==QVector<int>({ 5,  6}));
	QVERIFY(AztecXihuitlCalendar::aztecXihuitlFromFixed( 210155)==QVector<int>({13,  4}));
	QVERIFY(AztecXihuitlCalendar::aztecXihuitlFromFixed( 253427)==QVector<int>({ 5,  1}));
	QVERIFY(AztecXihuitlCalendar::aztecXihuitlFromFixed( 369740)==QVector<int>({17,  4}));
	QVERIFY(AztecXihuitlCalendar::aztecXihuitlFromFixed( 400085)==QVector<int>({ 1,  9}));
	QVERIFY(AztecXihuitlCalendar::aztecXihuitlFromFixed( 434355)==QVector<int>({17, 14}));
	QVERIFY(AztecXihuitlCalendar::aztecXihuitlFromFixed( 452605)==QVector<int>({17, 14}));
	QVERIFY(AztecXihuitlCalendar::aztecXihuitlFromFixed( 470160)==QVector<int>({ 1,  4}));
	QVERIFY(AztecXihuitlCalendar::aztecXihuitlFromFixed( 473837)==QVector<int>({ 2, 11}));
	QVERIFY(AztecXihuitlCalendar::aztecXihuitlFromFixed( 507850)==QVector<int>({ 5, 19}));
	QVERIFY(AztecXihuitlCalendar::aztecXihuitlFromFixed( 524156)==QVector<int>({18,  5}));
	QVERIFY(AztecXihuitlCalendar::aztecXihuitlFromFixed( 544676)==QVector<int>({ 3, 20}));
	QVERIFY(AztecXihuitlCalendar::aztecXihuitlFromFixed( 567118)==QVector<int>({12, 17}));
	QVERIFY(AztecXihuitlCalendar::aztecXihuitlFromFixed( 569477)==QVector<int>({ 3,  1}));
	QVERIFY(AztecXihuitlCalendar::aztecXihuitlFromFixed( 601716)==QVector<int>({ 8, 20}));
	QVERIFY(AztecXihuitlCalendar::aztecXihuitlFromFixed( 613424)==QVector<int>({10,  8}));
	QVERIFY(AztecXihuitlCalendar::aztecXihuitlFromFixed( 626596)==QVector<int>({11, 20}));
	QVERIFY(AztecXihuitlCalendar::aztecXihuitlFromFixed( 645554)==QVector<int>({10, 18}));
	QVERIFY(AztecXihuitlCalendar::aztecXihuitlFromFixed( 664224)==QVector<int>({13, 13}));
	QVERIFY(AztecXihuitlCalendar::aztecXihuitlFromFixed( 671401)==QVector<int>({ 7, 10}));
	QVERIFY(AztecXihuitlCalendar::aztecXihuitlFromFixed( 694799)==QVector<int>({ 9,  8}));
	QVERIFY(AztecXihuitlCalendar::aztecXihuitlFromFixed( 704424)==QVector<int>({16,  3}));
	QVERIFY(AztecXihuitlCalendar::aztecXihuitlFromFixed( 708842)==QVector<int>({18,  1}));
	QVERIFY(AztecXihuitlCalendar::aztecXihuitlFromFixed( 709409)==QVector<int>({ 9, 18}));
	QVERIFY(AztecXihuitlCalendar::aztecXihuitlFromFixed( 709580)==QVector<int>({18,  9}));
	QVERIFY(AztecXihuitlCalendar::aztecXihuitlFromFixed( 727274)==QVector<int>({ 8, 18}));
	QVERIFY(AztecXihuitlCalendar::aztecXihuitlFromFixed( 728714)==QVector<int>({ 7, 18}));
	QVERIFY(AztecXihuitlCalendar::aztecXihuitlFromFixed( 744313)==QVector<int>({ 3,  2}));
	QVERIFY(AztecXihuitlCalendar::aztecXihuitlFromFixed( 764652)==QVector<int>({16,  6}));

	QVERIFY(AztecTonalpohualliCalendar::aztecTonalpohualliFromFixed(-214193)==QVector<int>({ 5,  9}));
	QVERIFY(AztecTonalpohualliCalendar::aztecTonalpohualliFromFixed( -61387)==QVector<int>({ 9, 15}));
	QVERIFY(AztecTonalpohualliCalendar::aztecTonalpohualliFromFixed(  25469)==QVector<int>({12, 11}));
	QVERIFY(AztecTonalpohualliCalendar::aztecTonalpohualliFromFixed(  49217)==QVector<int>({ 9, 19}));
	QVERIFY(AztecTonalpohualliCalendar::aztecTonalpohualliFromFixed( 171307)==QVector<int>({ 3,  9}));
	QVERIFY(AztecTonalpohualliCalendar::aztecTonalpohualliFromFixed( 210155)==QVector<int>({ 7, 17}));
	QVERIFY(AztecTonalpohualliCalendar::aztecTonalpohualliFromFixed( 253427)==QVector<int>({ 2,  9}));
	QVERIFY(AztecTonalpohualliCalendar::aztecTonalpohualliFromFixed( 369740)==QVector<int>({ 4,  2}));
	QVERIFY(AztecTonalpohualliCalendar::aztecTonalpohualliFromFixed( 400085)==QVector<int>({ 7,  7}));
	QVERIFY(AztecTonalpohualliCalendar::aztecTonalpohualliFromFixed( 434355)==QVector<int>({ 9, 17}));
	QVERIFY(AztecTonalpohualliCalendar::aztecTonalpohualliFromFixed( 452605)==QVector<int>({ 7,  7}));
	QVERIFY(AztecTonalpohualliCalendar::aztecTonalpohualliFromFixed( 470160)==QVector<int>({12,  2}));
	QVERIFY(AztecTonalpohualliCalendar::aztecTonalpohualliFromFixed( 473837)==QVector<int>({10, 19}));
	QVERIFY(AztecTonalpohualliCalendar::aztecTonalpohualliFromFixed( 507850)==QVector<int>({ 2, 12}));
	QVERIFY(AztecTonalpohualliCalendar::aztecTonalpohualliFromFixed( 524156)==QVector<int>({ 6, 18}));
	QVERIFY(AztecTonalpohualliCalendar::aztecTonalpohualliFromFixed( 544676)==QVector<int>({12, 18}));
	QVERIFY(AztecTonalpohualliCalendar::aztecTonalpohualliFromFixed( 567118)==QVector<int>({ 3, 20}));
	QVERIFY(AztecTonalpohualliCalendar::aztecTonalpohualliFromFixed( 569477)==QVector<int>({ 9, 19}));
	QVERIFY(AztecTonalpohualliCalendar::aztecTonalpohualliFromFixed( 601716)==QVector<int>({ 8, 18}));
	QVERIFY(AztecTonalpohualliCalendar::aztecTonalpohualliFromFixed( 613424)==QVector<int>({ 3,  6}));
	QVERIFY(AztecTonalpohualliCalendar::aztecTonalpohualliFromFixed( 626596)==QVector<int>({ 6, 18}));
	QVERIFY(AztecTonalpohualliCalendar::aztecTonalpohualliFromFixed( 645554)==QVector<int>({10, 16}));
	QVERIFY(AztecTonalpohualliCalendar::aztecTonalpohualliFromFixed( 664224)==QVector<int>({12,  6}));
	QVERIFY(AztecTonalpohualliCalendar::aztecTonalpohualliFromFixed( 671401)==QVector<int>({13,  3}));
	QVERIFY(AztecTonalpohualliCalendar::aztecTonalpohualliFromFixed( 694799)==QVector<int>({11,  1}));
	QVERIFY(AztecTonalpohualliCalendar::aztecTonalpohualliFromFixed( 704424)==QVector<int>({ 3,  6}));
	QVERIFY(AztecTonalpohualliCalendar::aztecTonalpohualliFromFixed( 708842)==QVector<int>({ 1,  4}));
	QVERIFY(AztecTonalpohualliCalendar::aztecTonalpohualliFromFixed( 709409)==QVector<int>({ 9, 11}));
	QVERIFY(AztecTonalpohualliCalendar::aztecTonalpohualliFromFixed( 709580)==QVector<int>({11,  2}));
	QVERIFY(AztecTonalpohualliCalendar::aztecTonalpohualliFromFixed( 727274)==QVector<int>({12, 16}));
	QVERIFY(AztecTonalpohualliCalendar::aztecTonalpohualliFromFixed( 728714)==QVector<int>({ 9, 16}));
	QVERIFY(AztecTonalpohualliCalendar::aztecTonalpohualliFromFixed( 744313)==QVector<int>({ 8, 15}));
	QVERIFY(AztecTonalpohualliCalendar::aztecTonalpohualliFromFixed( 764652)==QVector<int>({ 2, 14}));
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

void TestCalendars::testPersian()
{
	QVERIFY(-214193==PersianArithmeticCalendar::fixedFromPersianArithmetic({-1208,  5,  1}));
	QVERIFY( -61387==PersianArithmeticCalendar::fixedFromPersianArithmetic({ -790,  9, 14}));
	QVERIFY(  25469==PersianArithmeticCalendar::fixedFromPersianArithmetic({ -552,  7,  2}));
	QVERIFY(  49217==PersianArithmeticCalendar::fixedFromPersianArithmetic({ -487,  7,  9}));
	QVERIFY( 171307==PersianArithmeticCalendar::fixedFromPersianArithmetic({ -153, 10, 18}));
	QVERIFY( 210155==PersianArithmeticCalendar::fixedFromPersianArithmetic({  -46,  2, 30}));
	QVERIFY( 253427==PersianArithmeticCalendar::fixedFromPersianArithmetic({   73,  8, 19}));
	QVERIFY( 369740==PersianArithmeticCalendar::fixedFromPersianArithmetic({  392,  2,  5}));
	QVERIFY( 400085==PersianArithmeticCalendar::fixedFromPersianArithmetic({  475,  3,  3}));
	QVERIFY( 434355==PersianArithmeticCalendar::fixedFromPersianArithmetic({  569,  1,  3}));
	QVERIFY( 452605==PersianArithmeticCalendar::fixedFromPersianArithmetic({  618, 12, 20}));
	QVERIFY( 470160==PersianArithmeticCalendar::fixedFromPersianArithmetic({  667,  1, 14}));
	QVERIFY( 473837==PersianArithmeticCalendar::fixedFromPersianArithmetic({  677,  2,  8}));
	QVERIFY( 507850==PersianArithmeticCalendar::fixedFromPersianArithmetic({  770,  3, 22}));
	QVERIFY( 524156==PersianArithmeticCalendar::fixedFromPersianArithmetic({  814, 11, 13}));
	QVERIFY( 544676==PersianArithmeticCalendar::fixedFromPersianArithmetic({  871,  1, 21}));
	QVERIFY( 567118==PersianArithmeticCalendar::fixedFromPersianArithmetic({  932,  6, 28}));
	QVERIFY( 569477==PersianArithmeticCalendar::fixedFromPersianArithmetic({  938, 12, 14}));
	QVERIFY( 601716==PersianArithmeticCalendar::fixedFromPersianArithmetic({ 1027,  3, 21}));
	QVERIFY( 613424==PersianArithmeticCalendar::fixedFromPersianArithmetic({ 1059,  4, 10}));
	QVERIFY( 626596==PersianArithmeticCalendar::fixedFromPersianArithmetic({ 1095,  5,  2}));
	QVERIFY( 645554==PersianArithmeticCalendar::fixedFromPersianArithmetic({ 1147,  3, 30}));
	QVERIFY( 664224==PersianArithmeticCalendar::fixedFromPersianArithmetic({ 1198,  5, 10}));
	QVERIFY( 671401==PersianArithmeticCalendar::fixedFromPersianArithmetic({ 1218,  1,  7}));
	QVERIFY( 694799==PersianArithmeticCalendar::fixedFromPersianArithmetic({ 1282,  1, 29}));
	QVERIFY( 704424==PersianArithmeticCalendar::fixedFromPersianArithmetic({ 1308,  6,  3}));
	QVERIFY( 708842==PersianArithmeticCalendar::fixedFromPersianArithmetic({ 1320,  7,  7}));
	QVERIFY( 709409==PersianArithmeticCalendar::fixedFromPersianArithmetic({ 1322,  1, 29}));
	QVERIFY( 709580==PersianArithmeticCalendar::fixedFromPersianArithmetic({ 1322,  7, 14}));
	QVERIFY( 727274==PersianArithmeticCalendar::fixedFromPersianArithmetic({ 1370, 12, 27}));
	QVERIFY( 728714==PersianArithmeticCalendar::fixedFromPersianArithmetic({ 1374, 12,  6}));
	QVERIFY( 744313==PersianArithmeticCalendar::fixedFromPersianArithmetic({ 1417,  8, 19}));
	QVERIFY( 764652==PersianArithmeticCalendar::fixedFromPersianArithmetic({ 1473,  4, 28}));

	QVERIFY(PersianArithmeticCalendar::persianArithmeticFromFixed(-214193)==QVector<int>({-1208,  5,  1}));
	QVERIFY(PersianArithmeticCalendar::persianArithmeticFromFixed( -61387)==QVector<int>({ -790,  9, 14}));
	QVERIFY(PersianArithmeticCalendar::persianArithmeticFromFixed(  25469)==QVector<int>({ -552,  7,  2}));
	QVERIFY(PersianArithmeticCalendar::persianArithmeticFromFixed(  49217)==QVector<int>({ -487,  7,  9}));
	QVERIFY(PersianArithmeticCalendar::persianArithmeticFromFixed( 171307)==QVector<int>({ -153, 10, 18}));
	QVERIFY(PersianArithmeticCalendar::persianArithmeticFromFixed( 210155)==QVector<int>({  -46,  2, 30}));
	QVERIFY(PersianArithmeticCalendar::persianArithmeticFromFixed( 253427)==QVector<int>({   73,  8, 19}));
	QVERIFY(PersianArithmeticCalendar::persianArithmeticFromFixed( 369740)==QVector<int>({  392,  2,  5}));
	QVERIFY(PersianArithmeticCalendar::persianArithmeticFromFixed( 400085)==QVector<int>({  475,  3,  3}));
	QVERIFY(PersianArithmeticCalendar::persianArithmeticFromFixed( 434355)==QVector<int>({  569,  1,  3}));
	QVERIFY(PersianArithmeticCalendar::persianArithmeticFromFixed( 452605)==QVector<int>({  618, 12, 20}));
	QVERIFY(PersianArithmeticCalendar::persianArithmeticFromFixed( 470160)==QVector<int>({  667,  1, 14}));
	QVERIFY(PersianArithmeticCalendar::persianArithmeticFromFixed( 473837)==QVector<int>({  677,  2,  8}));
	QVERIFY(PersianArithmeticCalendar::persianArithmeticFromFixed( 507850)==QVector<int>({  770,  3, 22}));
	QVERIFY(PersianArithmeticCalendar::persianArithmeticFromFixed( 524156)==QVector<int>({  814, 11, 13}));
	QVERIFY(PersianArithmeticCalendar::persianArithmeticFromFixed( 544676)==QVector<int>({  871,  1, 21}));
	QVERIFY(PersianArithmeticCalendar::persianArithmeticFromFixed( 567118)==QVector<int>({  932,  6, 28}));
	QVERIFY(PersianArithmeticCalendar::persianArithmeticFromFixed( 569477)==QVector<int>({  938, 12, 14}));
	QVERIFY(PersianArithmeticCalendar::persianArithmeticFromFixed( 601716)==QVector<int>({ 1027,  3, 21}));
	QVERIFY(PersianArithmeticCalendar::persianArithmeticFromFixed( 613424)==QVector<int>({ 1059,  4, 10}));
	QVERIFY(PersianArithmeticCalendar::persianArithmeticFromFixed( 626596)==QVector<int>({ 1095,  5,  2}));
	QVERIFY(PersianArithmeticCalendar::persianArithmeticFromFixed( 645554)==QVector<int>({ 1147,  3, 30}));
	QVERIFY(PersianArithmeticCalendar::persianArithmeticFromFixed( 664224)==QVector<int>({ 1198,  5, 10}));
	QVERIFY(PersianArithmeticCalendar::persianArithmeticFromFixed( 671401)==QVector<int>({ 1218,  1,  7}));
	QVERIFY(PersianArithmeticCalendar::persianArithmeticFromFixed( 694799)==QVector<int>({ 1282,  1, 29}));
	QVERIFY(PersianArithmeticCalendar::persianArithmeticFromFixed( 704424)==QVector<int>({ 1308,  6,  3}));
	QVERIFY(PersianArithmeticCalendar::persianArithmeticFromFixed( 708842)==QVector<int>({ 1320,  7,  7}));
	QVERIFY(PersianArithmeticCalendar::persianArithmeticFromFixed( 709409)==QVector<int>({ 1322,  1, 29}));
	QVERIFY(PersianArithmeticCalendar::persianArithmeticFromFixed( 709580)==QVector<int>({ 1322,  7, 14}));
	QVERIFY(PersianArithmeticCalendar::persianArithmeticFromFixed( 727274)==QVector<int>({ 1370, 12, 27}));
	QVERIFY(PersianArithmeticCalendar::persianArithmeticFromFixed( 728714)==QVector<int>({ 1374, 12,  6}));
	QVERIFY(PersianArithmeticCalendar::persianArithmeticFromFixed( 744313)==QVector<int>({ 1417,  8, 19}));
	QVERIFY(PersianArithmeticCalendar::persianArithmeticFromFixed( 764652)==QVector<int>({ 1473,  4, 28}));
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

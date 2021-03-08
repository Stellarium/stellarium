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
}

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

#include "tests/testCalendars.hpp"

#include <QString>
#include <QDebug>
#include <QtGlobal>
#include <QVariantList>
#include <QMap>

#include "StelUtils.hpp"
#include "../plugins/Calendars/src/Calendar.hpp"
#include "../plugins/Calendars/src/JulianCalendar.hpp"
#include "../plugins/Calendars/src/GregorianCalendar.hpp"
#include "../plugins/Calendars/src/ISOCalendar.hpp"
#include "../plugins/Calendars/src/MayaLongCountCalendar.hpp"
#include "../plugins/Calendars/src/MayaHaabCalendar.hpp"
#include "../plugins/Calendars/src/MayaTzolkinCalendar.hpp"
#include "../plugins/Calendars/src/AztecXihuitlCalendar.hpp"
#include "../plugins/Calendars/src/AztecTonalpohualliCalendar.hpp"

//#define ERROR_LIMIT 1e-6

QTEST_GUILESS_MAIN(TestCalendars)

void TestCalendars::testEuropean()
{
	QVERIFY(GregorianCalendar::fixedFromGregorian({1582, 10, 14})==JulianCalendar::fixedFromJulian({1582, 10, 4}));
	QVERIFY(GregorianCalendar::fixedFromGregorian({1582, 10, 15})==JulianCalendar::fixedFromJulian({1582, 10, 5}));
	QVERIFY(GregorianCalendar::fixedFromGregorian({1, 1, 1})==GregorianCalendar::gregorianEpoch);
	QVERIFY(JulianCalendar::fixedFromJulian({1, 1, 1})==JulianCalendar::julianEpoch);
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
	
	

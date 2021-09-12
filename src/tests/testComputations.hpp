/*
 * Stellarium
 * Copyright (C) 2019 Alexander Wolf
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

#ifndef TESTCOMPUTATIONS_HPP
#define TESTCOMPUTATIONS_HPP

#include <QObject>
#include <QtTest>

class TestComputations : public QObject
{
	Q_OBJECT

private slots:
	void testJDFromBesselianEpoch();
	void testIsPowerOfTwo();
	void testGetBiggerPowerOfTwo();
	void testDayInYear();
	void testYearFraction();
	void testEquToEqlTransformations();
	void testEclToEquTransformations();
	void testSpheToRectTransformations();
	void testRectToSpheTransformations();
	void testVector2Operators();
	void testVector3Operators();
	void testIntMod();
	void testIntDiv();
	void testIntFloorDiv();
	void testFloatMod();
	void testDoubleMod();
	void testExp();
	void testACos();
	void testSign();
	void testInterpolation();
};

#endif // _TESTCOMPUTATIONS_HPP


/*
 * Stellarium
 * Copyright (C) 2026 Sylvain Simard
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

#ifndef TESTHORIZONSSITE_HPP
#define TESTHORIZONSSITE_HPP

#include <QObject>
#include <QtTest>

//! (SS) 2026-11-09 Self-consistency tests of the site of a topocentric observer (HorizonsSite). No StelCore, no StelApp.
class TestHorizonsSite : public QObject
{
	Q_OBJECT

private slots:
	void testWgs84Site();
	void testRotationsKeepRadius();
	void testEarlyZonesIgnoreEop();
	void testUt1Rate();
	void testPolarMotion();
	void testSiderealTimes();
	void testVelocity();
	void testHourAngleAzEl();
	void testOutOfRange();
	void testGoldenStandalone();	// (SS) 2026-11-10 values printed by mercury_r6.cpp (TOPOCENTRIC true, Palomar), zones 3 and 4
};

#endif // TESTHORIZONSSITE_HPP

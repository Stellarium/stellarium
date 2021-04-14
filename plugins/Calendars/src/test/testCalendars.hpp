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


#ifndef TESTCALENDARS_HPP
#define TESTCALENDARS_HPP

#include <QObject>
#include <QtTest>

class TestCalendars : public QObject
{
Q_OBJECT
private slots:
	void testBasics();
	void testEuropean();
	void testNearEastern();
	void testRoman();
	void testFrenchRevolution();
	void testMesoamerican();
	void testIslamic();
	void testHebrew();
	void testPersian();
	void testBalinesePawukon();
	void testOldHindu();
};

#endif // _TESTCALENDARS_HPP


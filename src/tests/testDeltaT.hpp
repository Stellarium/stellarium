/*
 * Stellarium
 * Copyright (C) 2010 Matthew Gates
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

#ifndef TESTDELTAT_HPP
#define TESTDELTAT_HPP

#include <QObject>
#include <QtTest>
#include <QVector>
#include <QString>

class TestDeltaT : public QObject
{
	Q_OBJECT

private slots:
	void initTestCase();
	void testDeltaTByEspenakMeeus();
	void testDeltaTByChaprontMeeus();
	void testDeltaTByChaprontMeeusWideDates();
	void testDeltaTByMorrisonStephenson1982WideDates();	
	void testDeltaTByStephensonMorrison1984WideDates();	
	void testDeltaTByStephensonMorrison1995WideDates();	
	void testDeltaTByStephensonMorrison2004WideDates();	
	void testDeltaTByStephensonMorrisonHohenkerk2016GenericDates();
	void testDeltaTByStephenson1997WideDates();	
	void testDeltaTByMeeusSimons();	
	void testDeltaTByKhalidSultanaZaidiWideDates();
	void testDeltaTByMontenbruckPfleger();
	void testDeltaTByReingoldDershowitzWideDates();
	void testDeltaTByJPLHorizons();
	void testDeltaTByBorkowski();
	void testDeltaTByIAU();
	void testDeltaTByAstronomicalEphemeris();
	void testDeltaTByTuckermanGoldstine();		
	void testDeltaTStandardError();
private:
	QVariantList genericData;
};

#endif // _TESTDELTAT_HPP


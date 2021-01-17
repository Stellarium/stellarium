/*
 * Stellarium
 * Copyright (C) 2016 Alexander Wolf
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

#ifndef TESTEPHEMERIS_HPP
#define TESTEPHEMERIS_HPP

#include <QObject>
#include <QtTest>

class TestEphemeris : public QObject
{
	Q_OBJECT

private slots:
	void initTestCase();

	// VSOP87
	void testMercuryHeliocentricEphemerisVsop87();	
	void testVenusHeliocentricEphemerisVsop87();
	void testMarsHeliocentricEphemerisVsop87();
	void testJupiterHeliocentricEphemerisVsop87();
	void testSaturnHeliocentricEphemerisVsop87();
	void testUranusHeliocentricEphemerisVsop87();
	void testNeptuneHeliocentricEphemerisVsop87();

	// JPL DE430
	void testMercuryHeliocentricEphemerisDe430();
	void testVenusHeliocentricEphemerisDe430();
	void testMarsHeliocentricEphemerisDe430();
	void testJupiterHeliocentricEphemerisDe430();
	void testSaturnHeliocentricEphemerisDe430();
	void testUranusHeliocentricEphemerisDe430();
	void testNeptuneHeliocentricEphemerisDe430();

	// JPL DE431
	void testMercuryHeliocentricEphemerisDe431();	
	void testVenusHeliocentricEphemerisDe431();
	void testMarsHeliocentricEphemerisDe431();
	void testJupiterHeliocentricEphemerisDe431();
	void testSaturnHeliocentricEphemerisDe431();
	void testUranusHeliocentricEphemerisDe431();
	void testNeptuneHeliocentricEphemerisDe431();

	void testL12Theory();
	void testMarsSatTheory();

private:
	QString de430FilePath, de431FilePath;
	QVariantList mercury, venus, mars, jupiter, saturn, uranus, neptune;
};

#endif // _TESTEPHEMERIS_HPP


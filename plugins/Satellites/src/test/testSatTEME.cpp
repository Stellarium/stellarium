/*
 * Stellarium
 * Copyright (C) 2023 Andy Kirkham
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

#include "testSatTEME.hpp"

QTEST_GUILESS_MAIN(TestSatTEME)

void TestSatTEME::testISS()
{
	// Test the new SatTEME(OMM) constructor.
	// Begin by building an OMM from a known TLE set.
	QString q0("ISS (ZARYA)");
	QString q1("1 25544U 98067A   23196.73971065  .00010885  00000+0  20007-3 0  9992");
	QString q2("2 25544  51.6408 187.0394 0000384  67.0989 279.3150 15.49762966406217");
	OMM omm(q0, q1, q2);

	// Create a gSatTEME using an OMM model.
	gSatTEME dut(omm);

	// Set the current epoch to a known value (sets the state vecotrs)
	dut.setEpoch(2460141.3839699654); // 2023-07-15 22:12:55

	// Test the calculated state vectors against known examples.
	QCOMPARE(dut.getPos().toString(), "[-1670.64, -4212.8, 5054.45]");
	QCOMPARE(dut.getVel().toString(), "[7.39949, -0.67481, 1.87876]");

	// Test the calculated ground point against known examples.
	QCOMPARE(dut.getSubPoint().toString(), "[48.299, -3.32449, 422.422]");
}

void TestSatTEME::testHelios1B() 
{
	// This TLE presented "challenges" during testing.
	// Fixed the issue but made this Unit Test to ensure
	// it stays fixed.

	// Test the new SatTEME(OMM) constructor.
	// Begin by building an OMM from a known TLE set.
	QString q0("Helios 1B");
	QString q1("1 25977U 99064A   23196.67785369  .00000718  00000+0  94366-4 0  9997");
	QString q2("2 25977  98.2904  51.3270 0001183  32.0009 328.1276 14.83460740272860");
	OMM omm(q0, q1, q2);

	// Create a gSatTEME using an OMM model.
	gSatTEME dut(omm);

	// Set the current epoch to a known value (sets the state vecotrs)
	dut.setEpoch(2460143.1136806); // 2023-07-17 14:43:42

	// Test the calculated state vectors against known examples.
	QCOMPARE(dut.getPos().toString(), "[-2055.39, -1150.32, -6596.47]");
	QCOMPARE(dut.getVel().toString(), "[4.01421, 5.95049, -2.28831]");

	// Test the calculated ground point against known examples.
	QCOMPARE(dut.getSubPoint().toString(), "[-70.4603, 53.1402, 645.216]");
}



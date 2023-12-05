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

#include <ios>
#include <string.h>
#include <stdlib.h>

#include "VecMath.hpp"
#include "testSGP4.hpp"

QTEST_GUILESS_MAIN(TestSGP4)

void TestSGP4::testSGP4()
{
	const char *l0 = "ISS (ZARYA)";
	const char *l1 = "1 25544U 98067A   23191.40640406  .00007611  00000+0  14335-3 0  9995";
	const char *l2 = "2 25544  51.6398 233.5611 0000373  12.3897  91.4664 15.49560249404764";
	char *l1c, *l2c;
	l1c = strdup(l1);
	l2c = strdup(l2);
	gSatTEME sat(l0, l1c, l2c);
	Vec3d pos = sat.getPos();
	Vec3d vel = sat.getVel();

	QCOMPARE(pos.toString(), "[4259.68, -1120.18, 5166.77]");
	QCOMPARE(vel.toString(), "[3.50308, 6.662, -1.43989]");

	free(l1c);
	free(l2c);
}

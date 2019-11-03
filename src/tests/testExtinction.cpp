/*
 * Stellarium 
 * Copyright (C) 2015 Alexander Wolf
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

#include <QObject>
#include <QtDebug>

#include "tests/testExtinction.hpp"

QTEST_GUILESS_MAIN(TestExtinction)

void TestExtinction::initTestCase()
{
}

void TestExtinction::testBase()
{
	Extinction extCls;
	Vec3d v(1.,0.,0.);
	float mag=4.f;
	extCls.forward(v, &mag);
	QVERIFY(mag>=4.);

	Vec3d vert(0.,0.,1.);
	mag=2.0f;
	extCls.setExtinctionCoefficient(0.25);
	extCls.forward(vert, &mag);
	QVERIFY(fabs(mag-2.25)<0.0001);
}

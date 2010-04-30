/*
 * Stellarium
 * Copyright (C) 2010 Fabien Chereau
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include <QObject>
#include <QtDebug>
#include <QtTest>

#include "tests/testRefractionExtinction.hpp"

QTEST_MAIN(TestRefractionExtinction)

void TestRefractionExtinction::initTestCase()
{
}

void TestRefractionExtinction::testBase()
{
	RefractionExtinction refExt;
	Vec3d v(1.,0.,0.);
	float mag=4.f;
	refExt.forward(&v, &mag, 1);
	QVERIFY(mag>=4.);
	QVERIFY(v[2]>=0);


	Vec3d vert(0.,0.,1.);
	mag=2.0f;
	refExt.setExtinctionCoefficient(0.25);
	refExt.setTemperature(15.0);
	refExt.setPressure(1000.0);
	refExt.forward(&vert, &mag, 1);
	QVERIFY(mag==2.25);
	QVERIFY(vert[2]==1.0);

	Vec3d neg(std::sqrt(2.0),0.,std::sqrt(2.0));
	mag=2.0f;
	refExt.forward(&neg, &mag, 1);
	QVERIFY(mag==2.0);
	QVERIFY(neg[2]==std::sqrt(2.0));
}

void TestRefractionExtinction::benchmark()
{
}

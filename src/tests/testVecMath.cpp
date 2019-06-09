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

#include <QObject>
#include <QtDebug>
#include <QVariantList>
#include <QtTest>

#include "tests/testVecMath.hpp"

QTEST_GUILESS_MAIN(TestVecMath)

#define ERROR_LIMIT 1e-5

void TestVecMath::testVec2Math()
{
	Vec2i vi;
	Vec2f vf;
	Vec2d vd;

	vi.set(0,0);
	QVERIFY(vi==Vec2i(0,0));
	QVERIFY(vi!=Vec2i(0,1));
	vi+=Vec2i(5,5);
	QVERIFY(vi==Vec2i(5,5));
	vi-=Vec2i(2,2);
	QVERIFY(vi==Vec2i(3,3));
	vi*=Vec2i(2,2);
	QVERIFY(vi==Vec2i(6,6));
	vi/=Vec2i(3,3);
	QVERIFY(vi==Vec2i(2,2));
	vi = Vec2i(1,1) + Vec2i(9,9);
	QVERIFY(vi==Vec2i(10,10));
	vi = Vec2i(5,5) - Vec2i(6,6);
	QVERIFY(vi==Vec2i(-1,-1));
	vi = Vec2i(10,10) * Vec2i(2,1);
	QVERIFY(vi==Vec2i(20,10));
	vi = Vec2i(10,10) / Vec2i(2,5);
	QVERIFY(vi==Vec2i(5,2));
	vi.set(5,5);
	QVERIFY(vi.min(Vec2i(3,3))==Vec2i(3,3));
	QVERIFY(vi.max(Vec2i(3,3))==Vec2i(5,5));
	QVERIFY(vi.clamp(Vec2i(1,1),Vec2i(10,10))==Vec2i(5,5));
	QVERIFY(vi.dot(Vec2i(5,5))==50);
	vi.set(2,2);
	QVERIFY(vi.length()==2);
	QVERIFY(vi.lengthSquared()==8);
	QVERIFY(vi.toString()==QString("[2, 2]"));

	vf.set(0.f,0.f);
	QVERIFY(vf==Vec2f(0.f,0.f));
	QVERIFY(vf!=Vec2f(0.f,1.f));
	vf+=Vec2f(5.f,5.f);
	QVERIFY(vf==Vec2f(5.f,5.f));
	vf-=Vec2f(2.f,2.f);
	QVERIFY(vf==Vec2f(3.f,3.f));
	vf*=Vec2f(2.f,2.f);
	QVERIFY(vf==Vec2f(6.f,6.f));
	vf/=Vec2f(3.f,3.f);
	QVERIFY(vf==Vec2f(2.f,2.f));
	vf = Vec2f(1.f,1.f) + Vec2f(9.f,9.f);
	QVERIFY(vf==Vec2f(10.f,10.f));
	vf = Vec2f(5.f,5.f) - Vec2f(6.f,6.f);
	QVERIFY(vf==Vec2f(-1.f,-1.f));
	vf = Vec2f(10.f,10.f) * Vec2f(2.f,1.f);
	QVERIFY(vf==Vec2f(20.f,10.f));
	vf = Vec2f(10.f,10.f) / Vec2f(2.f,5.f);
	QVERIFY(vf==Vec2f(5.f,2.f));
	vf.set(5.f,5.f);
	QVERIFY(vf.min(Vec2f(3.f,3.f))==Vec2f(3.f,3.f));
	QVERIFY(vf.max(Vec2f(3.f,3.f))==Vec2f(5.f,5.f));
	QVERIFY(vf.clamp(Vec2f(1.f,1.f),Vec2f(10.f,10.f))==Vec2f(5.f,5.f));
	QVERIFY(qAbs(vf.dot(Vec2f(5.f,5.f)) - 50.f) <= ERROR_LIMIT);
	vf.set(2.f,2.f);
	QVERIFY(qAbs(vf.length() - 2.82843) <= ERROR_LIMIT);
	QVERIFY(qAbs(vf.lengthSquared() - 8.f) <= ERROR_LIMIT);

	vd.set(0.,0.);
	QVERIFY(vd==Vec2d(0.,0.));
	QVERIFY(vd!=Vec2d(0.,1.));
	vd+=Vec2d(5.,5.);
	QVERIFY(vd==Vec2d(5.,5.));
	vd-=Vec2d(2.,2.);
	QVERIFY(vd==Vec2d(3.,3.));
	vd*=Vec2d(2.,2.);
	QVERIFY(vd==Vec2d(6.,6.));
	vd/=Vec2d(3.,3.);
	QVERIFY(vd==Vec2d(2.,2.));
	vd = Vec2d(1.,1.) + Vec2d(9.,9.);
	QVERIFY(vd==Vec2d(10.,10.));
	vd = Vec2d(5.,5.) - Vec2d(6.,6.);
	QVERIFY(vd==Vec2d(-1.,-1.));
	vd = Vec2d(10.,10.) * Vec2d(2.,1.);
	QVERIFY(vd==Vec2d(20.,10.));
	vd = Vec2d(10.,10.) / Vec2d(2.,5.);
	QVERIFY(vd==Vec2d(5.,2.));
	vd.set(5.,5.);
	QVERIFY(vd.min(Vec2d(3.,3.))==Vec2d(3.,3.));
	QVERIFY(vd.max(Vec2d(3.,3.))==Vec2d(5.,5.));
	QVERIFY(vd.clamp(Vec2d(1.,1.),Vec2d(10.,10.))==Vec2d(5.,5.));
	QVERIFY(qAbs(vd.dot(Vec2d(5.,5.)) - 50.) <= ERROR_LIMIT);
	vd.set(2.,2.);
	QVERIFY(qAbs(vd.length() - 2.82843f) <= ERROR_LIMIT);
	QVERIFY(qAbs(vd.lengthSquared() - 8.) <= ERROR_LIMIT);
}

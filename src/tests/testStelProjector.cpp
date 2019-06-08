/*
 * Stellarium
 * Copyright (C) 2009 Matthew Gates
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

#include "tests/testStelProjector.hpp"

#include <QObject>
#include <QDebug>
#include <QTest>

#include "StelProjectorClasses.hpp"
#include "StelProjectorType.hpp"
#include "StelProjector.hpp"

QTEST_GUILESS_MAIN(TestStelProjector)

#define ERROR_LIMIT 1e-4

void TestStelProjector::testStelProjectorPerspective()
{
	Vec3f a;
	Vec3d b;
	StelProjector::ModelViewTranformP modelViewTransform;
	StelProjectorP projection = StelProjectorP(new StelProjectorPerspective(modelViewTransform));
	float maxFov = projection->getMaxFov();
	float expectedFov = 120.f;
	float actualError = qAbs(maxFov - expectedFov);
	QVERIFY2(actualError <= ERROR_LIMIT, QString("Max. FOV=%1deg expected FOV=%2deg error=%3 acceptable=%4")
						.arg(QString::number(maxFov, 'f', 3))
						.arg(QString::number(expectedFov, 'f', 3))
						.arg(QString::number(actualError, 'f', 5))
						.arg(QString::number(ERROR_LIMIT, 'f', 5))
						.toUtf8());
	a = Vec3f(0.f,0.f,0.f);
	QVERIFY(projection->forward(a));
	b = Vec3d(0.,0.,0.);
	QVERIFY(projection->backward(b));

	float sf = projection->fovToViewScalingFactor(0.f);
	float expectedSF = 0.f;
	actualError = qAbs(sf - expectedSF);
	QVERIFY2(actualError <= ERROR_LIMIT, QString("F2V scaling factor=%1 expected S.F.=%2 error=%3 acceptable=%4")
						.arg(QString::number(sf, 'f', 3))
						.arg(QString::number(expectedSF, 'f', 3))
						.arg(QString::number(actualError, 'f', 5))
						.arg(QString::number(ERROR_LIMIT, 'f', 5))
						.toUtf8());

	float vsf = projection->viewScalingFactorToFov(0.f);
	float expectedVSF = 0.f;
	actualError = qAbs(vsf - expectedVSF);
	QVERIFY2(actualError <= ERROR_LIMIT, QString("V2F scaling factor=%1 expected S.F.=%2 error=%3 acceptable=%4")
						.arg(QString::number(vsf, 'f', 3))
						.arg(QString::number(expectedVSF, 'f', 3))
						.arg(QString::number(actualError, 'f', 5))
						.arg(QString::number(ERROR_LIMIT, 'f', 5))
						.toUtf8());
}

void TestStelProjector::testStelProjectorEqualArea()
{
	Vec3f a;
	Vec3d b;
	StelProjector::ModelViewTranformP modelViewTransform;
	StelProjectorP projection = StelProjectorP(new StelProjectorEqualArea(modelViewTransform));
	float maxFov = projection->getMaxFov();
	float expectedFov = 360.f;
	float actualError = qAbs(maxFov - expectedFov);
	QVERIFY2(actualError <= ERROR_LIMIT, QString("Max. FOV=%1deg expected FOV=%2deg error=%3 acceptable=%4")
						.arg(QString::number(maxFov, 'f', 3))
						.arg(QString::number(expectedFov, 'f', 3))
						.arg(QString::number(actualError, 'f', 5))
						.arg(QString::number(ERROR_LIMIT, 'f', 5))
						.toUtf8());
	a = Vec3f(0.f,0.f,0.f);
	QVERIFY(projection->forward(a));
	b = Vec3d(0.,0.,0.);
	QVERIFY(projection->backward(b));

	float sf = projection->fovToViewScalingFactor(0.f);
	float expectedSF = 0.f;
	actualError = qAbs(sf - expectedSF);
	QVERIFY2(actualError <= ERROR_LIMIT, QString("Scaling factor=%1 expected S.F.=%2 error=%3 acceptable=%4")
						.arg(QString::number(sf, 'f', 3))
						.arg(QString::number(expectedSF, 'f', 3))
						.arg(QString::number(actualError, 'f', 5))
						.arg(QString::number(ERROR_LIMIT, 'f', 5))
						.toUtf8());

	float vsf = projection->viewScalingFactorToFov(0.f);
	float expectedVSF = 0.f;
	actualError = qAbs(vsf - expectedVSF);
	QVERIFY2(actualError <= ERROR_LIMIT, QString("V2F scaling factor=%1 expected S.F.=%2 error=%3 acceptable=%4")
						.arg(QString::number(vsf, 'f', 3))
						.arg(QString::number(expectedVSF, 'f', 3))
						.arg(QString::number(actualError, 'f', 5))
						.arg(QString::number(ERROR_LIMIT, 'f', 5))
						.toUtf8());
}

void TestStelProjector::testStelProjectorStereographic()
{
	Vec3f a;
	Vec3d b;
	StelProjector::ModelViewTranformP modelViewTransform;
	StelProjectorP projection = StelProjectorP(new StelProjectorStereographic(modelViewTransform));
	float maxFov = projection->getMaxFov();
	float expectedFov = 235.f;
	float actualError = qAbs(maxFov - expectedFov);
	QVERIFY2(actualError <= ERROR_LIMIT, QString("Max. FOV=%1deg expected FOV=%2deg error=%3 acceptable=%4")
						.arg(QString::number(maxFov, 'f', 3))
						.arg(QString::number(expectedFov, 'f', 3))
						.arg(QString::number(actualError, 'f', 5))
						.arg(QString::number(ERROR_LIMIT, 'f', 5))
						.toUtf8());
	a = Vec3f(0.f,0.f,0.f);
	QVERIFY(projection->forward(a));
	b = Vec3d(0.,0.,0.);
	QVERIFY(projection->backward(b));

	float sf = projection->fovToViewScalingFactor(0.f);
	float expectedSF = 0.f;
	actualError = qAbs(sf - expectedSF);
	QVERIFY2(actualError <= ERROR_LIMIT, QString("Scaling factor=%1 expected S.F.=%2 error=%3 acceptable=%4")
						.arg(QString::number(sf, 'f', 3))
						.arg(QString::number(expectedSF, 'f', 3))
						.arg(QString::number(actualError, 'f', 5))
						.arg(QString::number(ERROR_LIMIT, 'f', 5))
						.toUtf8());

	float vsf = projection->viewScalingFactorToFov(0.f);
	float expectedVSF = 0.f;
	actualError = qAbs(vsf - expectedVSF);
	QVERIFY2(actualError <= ERROR_LIMIT, QString("V2F scaling factor=%1 expected S.F.=%2 error=%3 acceptable=%4")
						.arg(QString::number(vsf, 'f', 3))
						.arg(QString::number(expectedVSF, 'f', 3))
						.arg(QString::number(actualError, 'f', 5))
						.arg(QString::number(ERROR_LIMIT, 'f', 5))
						.toUtf8());
}

void TestStelProjector::testStelProjectorFisheye()
{
	Vec3f a;
	Vec3d b;
	StelProjector::ModelViewTranformP modelViewTransform;
	StelProjectorP projection = StelProjectorP(new StelProjectorFisheye(modelViewTransform));
	float maxFov = projection->getMaxFov();
	float expectedFov = 180.f;
	float actualError = qAbs(maxFov - expectedFov);
	QVERIFY2(actualError <= ERROR_LIMIT, QString("Max. FOV=%1deg expected FOV=%2deg error=%3 acceptable=%4")
						.arg(QString::number(maxFov, 'f', 3))
						.arg(QString::number(expectedFov, 'f', 3))
						.arg(QString::number(actualError, 'f', 5))
						.arg(QString::number(ERROR_LIMIT, 'f', 5))
						.toUtf8());
	a = Vec3f(0.f,0.f,0.f);
	QVERIFY(projection->forward(a));
	b = Vec3d(0.,0.,0.);
	QVERIFY(projection->backward(b));

	float sf = projection->fovToViewScalingFactor(0.f);
	float expectedSF = 0.f;
	actualError = qAbs(sf - expectedSF);
	QVERIFY2(actualError <= ERROR_LIMIT, QString("Scaling factor=%1 expected S.F.=%2 error=%3 acceptable=%4")
						.arg(QString::number(sf, 'f', 3))
						.arg(QString::number(expectedSF, 'f', 3))
						.arg(QString::number(actualError, 'f', 5))
						.arg(QString::number(ERROR_LIMIT, 'f', 5))
						.toUtf8());

	float vsf = projection->viewScalingFactorToFov(0.f);
	float expectedVSF = 0.f;
	actualError = qAbs(vsf - expectedVSF);
	QVERIFY2(actualError <= ERROR_LIMIT, QString("V2F scaling factor=%1 expected S.F.=%2 error=%3 acceptable=%4")
						.arg(QString::number(vsf, 'f', 3))
						.arg(QString::number(expectedVSF, 'f', 3))
						.arg(QString::number(actualError, 'f', 5))
						.arg(QString::number(ERROR_LIMIT, 'f', 5))
						.toUtf8());
}

void TestStelProjector::testStelProjectorHammer()
{
	Vec3f a;
	Vec3d b;
	StelProjector::ModelViewTranformP modelViewTransform;
	StelProjectorP projection = StelProjectorP(new StelProjectorHammer(modelViewTransform));
	float maxFov = projection->getMaxFov();
	float expectedFov = 360.f;
	float actualError = qAbs(maxFov - expectedFov);
	QVERIFY2(actualError <= ERROR_LIMIT, QString("Max. FOV=%1deg expected FOV=%2deg error=%3 acceptable=%4")
						.arg(QString::number(maxFov, 'f', 3))
						.arg(QString::number(expectedFov, 'f', 3))
						.arg(QString::number(actualError, 'f', 5))
						.arg(QString::number(ERROR_LIMIT, 'f', 5))
						.toUtf8());
	a = Vec3f(0.f,0.f,0.f);
	QVERIFY(projection->forward(a));
	b = Vec3d(0.,0.,0.);
	QVERIFY(projection->backward(b));

	float sf = projection->fovToViewScalingFactor(0.f);
	float expectedSF = 0.f;
	actualError = qAbs(sf - expectedSF);
	QVERIFY2(actualError <= ERROR_LIMIT, QString("Scaling factor=%1 expected S.F.=%2 error=%3 acceptable=%4")
						.arg(QString::number(sf, 'f', 3))
						.arg(QString::number(expectedSF, 'f', 3))
						.arg(QString::number(actualError, 'f', 5))
						.arg(QString::number(ERROR_LIMIT, 'f', 5))
						.toUtf8());

	float vsf = projection->viewScalingFactorToFov(0.f);
	float expectedVSF = 0.f;
	actualError = qAbs(vsf - expectedVSF);
	QVERIFY2(actualError <= ERROR_LIMIT, QString("V2F scaling factor=%1 expected S.F.=%2 error=%3 acceptable=%4")
						.arg(QString::number(vsf, 'f', 3))
						.arg(QString::number(expectedVSF, 'f', 3))
						.arg(QString::number(actualError, 'f', 5))
						.arg(QString::number(ERROR_LIMIT, 'f', 5))
						.toUtf8());
}

void TestStelProjector::testStelProjectorCylinder()
{
	Vec3f a;
	Vec3d b;
	StelProjector::ModelViewTranformP modelViewTransform;
	StelProjectorP projection = StelProjectorP(new StelProjectorCylinder(modelViewTransform));
	float maxFov = projection->getMaxFov();
	float expectedFov = 175.f * 4.f/3.f;
	float actualError = qAbs(maxFov - expectedFov);
	QVERIFY2(actualError <= ERROR_LIMIT, QString("Max. FOV=%1deg expected FOV=%2deg error=%3 acceptable=%4")
						.arg(QString::number(maxFov, 'f', 3))
						.arg(QString::number(expectedFov, 'f', 3))
						.arg(QString::number(actualError, 'f', 5))
						.arg(QString::number(ERROR_LIMIT, 'f', 5))
						.toUtf8());
	a = Vec3f(0.f,0.f,0.f);
	QVERIFY(projection->forward(a));
	b = Vec3d(0.,0.,0.);
	QVERIFY(projection->backward(b));

	float sf = projection->fovToViewScalingFactor(0.f);
	float expectedSF = 0.f;
	actualError = qAbs(sf - expectedSF);
	QVERIFY2(actualError <= ERROR_LIMIT, QString("Scaling factor=%1 expected S.F.=%2 error=%3 acceptable=%4")
						.arg(QString::number(sf, 'f', 3))
						.arg(QString::number(expectedSF, 'f', 3))
						.arg(QString::number(actualError, 'f', 5))
						.arg(QString::number(ERROR_LIMIT, 'f', 5))
						.toUtf8());

	float vsf = projection->viewScalingFactorToFov(0.f);
	float expectedVSF = 0.f;
	actualError = qAbs(vsf - expectedVSF);
	QVERIFY2(actualError <= ERROR_LIMIT, QString("V2F scaling factor=%1 expected S.F.=%2 error=%3 acceptable=%4")
						.arg(QString::number(vsf, 'f', 3))
						.arg(QString::number(expectedVSF, 'f', 3))
						.arg(QString::number(actualError, 'f', 5))
						.arg(QString::number(ERROR_LIMIT, 'f', 5))
						.toUtf8());
}

void TestStelProjector::testStelProjectorMercator()
{
	Vec3f a;
	Vec3d b;
	StelProjector::ModelViewTranformP modelViewTransform;
	StelProjectorP projection = StelProjectorP(new StelProjectorMercator(modelViewTransform));
	float maxFov = projection->getMaxFov();
	float expectedFov = 175.f * 4.f/3.f;
	float actualError = qAbs(maxFov - expectedFov);
	QVERIFY2(actualError <= ERROR_LIMIT, QString("Max. FOV=%1deg expected FOV=%2deg error=%3 acceptable=%4")
						.arg(QString::number(maxFov, 'f', 3))
						.arg(QString::number(expectedFov, 'f', 3))
						.arg(QString::number(actualError, 'f', 5))
						.arg(QString::number(ERROR_LIMIT, 'f', 5))
						.toUtf8());
	a = Vec3f(0.f,0.f,0.f);
	QVERIFY(projection->forward(a));
	b = Vec3d(0.,0.,0.);
	QVERIFY(projection->backward(b));

	float sf = projection->fovToViewScalingFactor(0.f);
	float expectedSF = 0.f;
	actualError = qAbs(sf - expectedSF);
	QVERIFY2(actualError <= ERROR_LIMIT, QString("Scaling factor=%1 expected S.F.=%2 error=%3 acceptable=%4")
						.arg(QString::number(sf, 'f', 3))
						.arg(QString::number(expectedSF, 'f', 3))
						.arg(QString::number(actualError, 'f', 5))
						.arg(QString::number(ERROR_LIMIT, 'f', 5))
						.toUtf8());

	float vsf = projection->viewScalingFactorToFov(0.f);
	float expectedVSF = 0.f;
	actualError = qAbs(vsf - expectedVSF);
	QVERIFY2(actualError <= ERROR_LIMIT, QString("V2F scaling factor=%1 expected S.F.=%2 error=%3 acceptable=%4")
						.arg(QString::number(vsf, 'f', 3))
						.arg(QString::number(expectedVSF, 'f', 3))
						.arg(QString::number(actualError, 'f', 5))
						.arg(QString::number(ERROR_LIMIT, 'f', 5))
						.toUtf8());
}

void TestStelProjector::testStelProjectorOrthographic()
{
	Vec3f a;
	Vec3d b;
	StelProjector::ModelViewTranformP modelViewTransform;
	StelProjectorP projection = StelProjectorP(new StelProjectorOrthographic(modelViewTransform));
	float maxFov = projection->getMaxFov();
	float expectedFov = 179.9999f;
	float actualError = qAbs(maxFov - expectedFov);
	QVERIFY2(actualError <= ERROR_LIMIT, QString("Max. FOV=%1deg expected FOV=%2deg error=%3 acceptable=%4")
						.arg(QString::number(maxFov, 'f', 3))
						.arg(QString::number(expectedFov, 'f', 3))
						.arg(QString::number(actualError, 'f', 5))
						.arg(QString::number(ERROR_LIMIT, 'f', 5))
						.toUtf8());
	a = Vec3f(0.f,0.f,0.f);
	QVERIFY(projection->forward(a));
	b = Vec3d(0.,0.,0.);
	QVERIFY(projection->backward(b));

	float sf = projection->fovToViewScalingFactor(0.f);
	float expectedSF = 0.f;
	actualError = qAbs(sf - expectedSF);
	QVERIFY2(actualError <= ERROR_LIMIT, QString("Scaling factor=%1 expected S.F.=%2 error=%3 acceptable=%4")
						.arg(QString::number(sf, 'f', 3))
						.arg(QString::number(expectedSF, 'f', 3))
						.arg(QString::number(actualError, 'f', 5))
						.arg(QString::number(ERROR_LIMIT, 'f', 5))
						.toUtf8());

	float vsf = projection->viewScalingFactorToFov(0.f);
	float expectedVSF = 0.f;
	actualError = qAbs(vsf - expectedVSF);
	QVERIFY2(actualError <= ERROR_LIMIT, QString("V2F scaling factor=%1 expected S.F.=%2 error=%3 acceptable=%4")
						.arg(QString::number(vsf, 'f', 3))
						.arg(QString::number(expectedVSF, 'f', 3))
						.arg(QString::number(actualError, 'f', 5))
						.arg(QString::number(ERROR_LIMIT, 'f', 5))
						.toUtf8());
}

void TestStelProjector::testStelProjectorSinusoidal()
{
	Vec3f a;
	Vec3d b;
	StelProjector::ModelViewTranformP modelViewTransform;
	StelProjectorP projection = StelProjectorP(new StelProjectorSinusoidal(modelViewTransform));
	float maxFov = projection->getMaxFov();
	float expectedFov = 175.f * 4.f/3.f;
	float actualError = qAbs(maxFov - expectedFov);
	QVERIFY2(actualError <= ERROR_LIMIT, QString("Max. FOV=%1deg expected FOV=%2deg error=%3 acceptable=%4")
						.arg(QString::number(maxFov, 'f', 3))
						.arg(QString::number(expectedFov, 'f', 3))
						.arg(QString::number(actualError, 'f', 5))
						.arg(QString::number(ERROR_LIMIT, 'f', 5))
						.toUtf8());
	a = Vec3f(0.f,0.f,0.f);
	QVERIFY(projection->forward(a));
	b = Vec3d(0.,0.,0.);
	QVERIFY(projection->backward(b));

	float sf = projection->fovToViewScalingFactor(0.f);
	float expectedSF = 0.f;
	actualError = qAbs(sf - expectedSF);
	QVERIFY2(actualError <= ERROR_LIMIT, QString("Scaling factor=%1 expected S.F.=%2 error=%3 acceptable=%4")
						.arg(QString::number(sf, 'f', 3))
						.arg(QString::number(expectedSF, 'f', 3))
						.arg(QString::number(actualError, 'f', 5))
						.arg(QString::number(ERROR_LIMIT, 'f', 5))
						.toUtf8());

	float vsf = projection->viewScalingFactorToFov(0.f);
	float expectedVSF = 0.f;
	actualError = qAbs(vsf - expectedVSF);
	QVERIFY2(actualError <= ERROR_LIMIT, QString("V2F scaling factor=%1 expected S.F.=%2 error=%3 acceptable=%4")
						.arg(QString::number(vsf, 'f', 3))
						.arg(QString::number(expectedVSF, 'f', 3))
						.arg(QString::number(actualError, 'f', 5))
						.arg(QString::number(ERROR_LIMIT, 'f', 5))
						.toUtf8());
}

void TestStelProjector::testStelProjectorMiller()
{
	Vec3f a;
	Vec3d b;
	StelProjector::ModelViewTranformP modelViewTransform;
	StelProjectorP projection = StelProjectorP(new StelProjectorMiller(modelViewTransform));
	float maxFov = projection->getMaxFov();
	float expectedFov = 175.f * 4.f/3.f;
	float actualError = qAbs(maxFov - expectedFov);
	QVERIFY2(actualError <= ERROR_LIMIT, QString("Max. FOV=%1deg expected FOV=%2deg error=%3 acceptable=%4")
						.arg(QString::number(maxFov, 'f', 3))
						.arg(QString::number(expectedFov, 'f', 3))
						.arg(QString::number(actualError, 'f', 5))
						.arg(QString::number(ERROR_LIMIT, 'f', 5))
						.toUtf8());
	a = Vec3f(0.f,0.f,0.f);
	QVERIFY(projection->forward(a));
	b = Vec3d(0.,0.,0.);
	QVERIFY(projection->backward(b));

	float sf = projection->fovToViewScalingFactor(0.f);
	float expectedSF = 0.f;
	actualError = qAbs(sf - expectedSF);
	QVERIFY2(actualError <= ERROR_LIMIT, QString("Scaling factor=%1 expected S.F.=%2 error=%3 acceptable=%4")
						.arg(QString::number(sf, 'f', 3))
						.arg(QString::number(expectedSF, 'f', 3))
						.arg(QString::number(actualError, 'f', 5))
						.arg(QString::number(ERROR_LIMIT, 'f', 5))
						.toUtf8());

	float vsf = projection->viewScalingFactorToFov(0.f);
	float expectedVSF = 0.f;
	actualError = qAbs(vsf - expectedVSF);
	QVERIFY2(actualError <= ERROR_LIMIT, QString("V2F scaling factor=%1 expected S.F.=%2 error=%3 acceptable=%4")
						.arg(QString::number(vsf, 'f', 3))
						.arg(QString::number(expectedVSF, 'f', 3))
						.arg(QString::number(actualError, 'f', 5))
						.arg(QString::number(ERROR_LIMIT, 'f', 5))
						.toUtf8());
}

void TestStelProjector::testStelProjector2D()
{
	Vec3f a;
	Vec3d b;
	StelProjectorP projection = StelProjectorP(new StelProjector2d());
	float maxFov = projection->getMaxFov();
	float expectedFov = 360.f;
	float actualError = qAbs(maxFov - expectedFov);
	QVERIFY2(actualError <= ERROR_LIMIT, QString("Max. FOV=%1deg expected FOV=%2deg error=%3 acceptable=%4")
						.arg(QString::number(maxFov, 'f', 3))
						.arg(QString::number(expectedFov, 'f', 3))
						.arg(QString::number(actualError, 'f', 5))
						.arg(QString::number(ERROR_LIMIT, 'f', 5))
						.toUtf8());
	a = Vec3f(0.f,0.f,0.f);
	QVERIFY(projection->forward(a));
	b = Vec3d(0.,0.,0.);
	QVERIFY(projection->backward(b));

	float sf = projection->fovToViewScalingFactor(0.f);
	float expectedSF = 0.f;
	actualError = qAbs(sf - expectedSF);
	QVERIFY2(actualError <= ERROR_LIMIT, QString("Scaling factor=%1 expected S.F.=%2 error=%3 acceptable=%4")
						.arg(QString::number(sf, 'f', 3))
						.arg(QString::number(expectedSF, 'f', 3))
						.arg(QString::number(actualError, 'f', 5))
						.arg(QString::number(ERROR_LIMIT, 'f', 5))
						.toUtf8());

	float vsf = projection->viewScalingFactorToFov(0.f);
	float expectedVSF = 0.f;
	actualError = qAbs(vsf - expectedVSF);
	QVERIFY2(actualError <= ERROR_LIMIT, QString("V2F scaling factor=%1 expected S.F.=%2 error=%3 acceptable=%4")
						.arg(QString::number(vsf, 'f', 3))
						.arg(QString::number(expectedVSF, 'f', 3))
						.arg(QString::number(actualError, 'f', 5))
						.arg(QString::number(ERROR_LIMIT, 'f', 5))
						.toUtf8());
}

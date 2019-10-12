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
#include <QVariantList>

#include "tests/testRefraction.hpp"
#include "StelUtils.hpp"

QTEST_GUILESS_MAIN(TestRefraction)

void TestRefraction::initTestCase()
{
	Refraction refCls;
	Vec3d v(1.,0.,0.);
	refCls.forward(v);
	QVERIFY(v[2]>=0);
}

void TestRefraction::testSaemundssonEquation()
{
	Refraction refCls;
	float acceptableError = 0.07;

	// Theoretical values, what gives the Saemundsson equation for refraction.
	// Calculating apparent altitudes from the true altitudes.
	QVariantList data;
	data << 0 << 28.982;
	data << 5 << 9.674;
	data << 10 << 5.408;
	data << 15 << 3.675;
	data << 20 << 2.741;
	data << 25 << 2.154;
	data << 30 << 1.746;
	data << 35 << 1.443;
	data << 40 << 1.206;
	data << 45 << 1.013;
	data << 50 << 0.850;
	data << 55 << 0.710;
	data << 60 << 0.585;
	data << 65 << 0.472;
	data << 70 << 0.368;
	data << 75 << 0.271;
	data << 80 << 0.178;
	data << 85 << 0.087;
	data << 89 << 0.016;

	refCls.setPressure(1010);
	refCls.setTemperature(10);

	while(data.count() >= 2)
	{
		int height = data.takeFirst().toInt();
		double ref = data.takeFirst().toDouble();
		Vec3d v;
		double h = height * M_PI/180.;
		StelUtils::spheToRect(0.0, h, v);
		refCls.forward(v);
		double lng, lat;
		StelUtils::rectToSphe(&lng, &lat, v);
		double result = qAbs((h-lat)*M_180_PI)*60.;
		double actualError = qAbs(ref - result);
		QVERIFY2(actualError <= acceptableError, QString("height=%1deg result=%2\" expected=%3\" error=%4 acceptable=%5")
							.arg(height)
							.arg(result)
							.arg(ref)
							.arg(actualError)
							.arg(acceptableError)
							.toUtf8());

		float refF = (float)ref;
		Vec3f vF;
		float hF = height * M_PI/180.f;
		StelUtils::spheToRect(0.f, hF, vF);
		refCls.forward(vF);
		float lngF, latF;
		StelUtils::rectToSphe(&lngF, &latF, vF);
		float resultF = qAbs((hF-latF)*180.f/M_PI)*60.f;
		float actualErrorF = qAbs(refF - resultF);
		QVERIFY2(actualErrorF <= acceptableError, QString("height=%1deg result=%2\" expected=%3\" error=%4 acceptable=%5")
							.arg(height)
							.arg(resultF)
							.arg(refF)
							.arg(actualErrorF)
							.arg(acceptableError)
							.toUtf8());
	}
}

void TestRefraction::testBennettEquation()
{
	Refraction refCls;
	float acceptableError = 0.14;

	// Theoretical values, what gives the Bennett equation for refraction.
	// Calculating true altitudes from the apparent altitudes.
	QVariantList data;
	data << 0 << 34.478;
	data << 5 << 9.883;
	data << 10 << 5.392;
	data << 15 << 3.636;
	data << 20 << 2.703;
	data << 25 << 2.120;
	data << 30 << 1.717;
	data << 35 << 1.418;
	data << 40 << 1.185;
	data << 45 << 0.995;
	data << 50 << 0.835;
	data << 55 << 0.697;
	data << 60 << 0.575;
	data << 65 << 0.464;
	data << 70 << 0.362;
	data << 75 << 0.266;
	data << 80 << 0.175;
	data << 85 << 0.086;
	data << 89 << 0.016;

	refCls.setPressure(1010);
	refCls.setTemperature(10);

	while(data.count() >= 2)
	{
		int height = data.takeFirst().toInt();
		double ref = data.takeFirst().toDouble();
		Vec3d v;
		double h = height * M_PI/180.;
		StelUtils::spheToRect(0.0, h, v);
		refCls.backward(v);
		double lng, lat;
		StelUtils::rectToSphe(&lng, &lat, v);
		double result = qAbs((h-lat)*M_180_PI)*60.;
		double actualError = qAbs(ref - result);
		QVERIFY2(actualError <= acceptableError, QString("height=%1deg result=%2\" expected=%3\" error=%4 acceptable=%5")
							.arg(height)
							.arg(result)
							.arg(ref)
							.arg(actualError)
							.arg(acceptableError)
							.toUtf8());

		float refF = (float)ref;
		Vec3f vF;
		float hF = height * M_PI/180.f;
		StelUtils::spheToRect(0.f, hF, vF);
		refCls.backward(vF);
		float lngF, latF;
		StelUtils::rectToSphe(&lngF, &latF, vF);
		float resultF = qAbs((hF-latF)*180.f/M_PI)*60.f;
		double actualErrorF = qAbs(refF - resultF);
		QVERIFY2(actualErrorF <= acceptableError, QString("height=%1deg result=%2\" expected=%3\" error=%4 acceptable=%5")
							.arg(height)
							.arg(resultF)
							.arg(refF)
							.arg(actualErrorF)
							.arg(acceptableError)
							.toUtf8());
	}
}

void TestRefraction::testComplexRefraction()
{
	Refraction refCls;
	double acceptableError = 0.14;

	// The delta of the theoretical values, what gives Saemundsson and Bennett equations for refraction.
	// true alt. -> apparent alt. -> true alt.
	QVariantList data;
	data << 0 << 5.496;
	data << 5 << 0.209;
	data << 10 << 0.016;
	data << 15 << 0.039;
	data << 20 << 0.038;
	data << 25 << 0.033;
	data << 30 << 0.029;
	data << 35 << 0.025;
	data << 40 << 0.021;
	data << 45 << 0.018;
	data << 50 << 0.015;
	data << 55 << 0.013;
	data << 60 << 0.010;
	data << 65 << 0.008;
	data << 70 << 0.006;
	data << 75 << 0.005;
	data << 80 << 0.003;
	data << 85 << 0.001;
	data << 89 << 0.000;

	refCls.setPressure(1010);
	refCls.setTemperature(10);

	while(data.count() >= 2)
	{
		int height = data.takeFirst().toInt();
		double expD = data.takeFirst().toDouble();
		Vec3d v;
		double lng, latS, latB;
		double h = height * M_PI/180.;
		StelUtils::spheToRect(0.0, h, v);
		refCls.forward(v);
		StelUtils::rectToSphe(&lng, &latS, v);
		StelUtils::spheToRect(0.0, h, v);
		refCls.backward(v);
		StelUtils::rectToSphe(&lng, &latB, v);
		double rS = qAbs((h-latS)*M_180_PI)*60.;
		double rB = qAbs((h-latB)*M_180_PI)*60.;
		double result = qAbs(rS-rB);
		QVERIFY2(qAbs(result - expD) <= acceptableError, QString("height=%1deg result=%2\" expected=%3\" acceptable=%5")
							.arg(height)
							.arg(result)
							.arg(expD)
							.arg(acceptableError)
							.toUtf8());

		float expDF = (float)expD;
		Vec3f vF;
		float lngF, latSF, latBF;
		float hF = height * M_PI_180f;
		StelUtils::spheToRect(0.f, hF, vF);
		refCls.forward(vF);
		StelUtils::rectToSphe(&lngF, &latSF, vF);
		StelUtils::spheToRect(0.f, hF, vF);
		refCls.backward(vF);
		StelUtils::rectToSphe(&lngF, &latBF, vF);
		float rSF = qAbs((hF-latSF)*M_180_PIf)*60.f;
		float rBF = qAbs((hF-latBF)*M_180_PIf)*60.f;
		float resultF = qAbs(rSF-rBF);
		QVERIFY2(qAbs(resultF - expDF) <= acceptableError, QString("height=%1deg result=%2\" expected=%3\" acceptable=%5")
							.arg(height)
							.arg(resultF)
							.arg(expDF)
							.arg(acceptableError)
							.toUtf8());
	}
}

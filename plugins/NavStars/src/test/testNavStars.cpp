/*
 * Copyright (C) 2019 Alexander Wolf <alex.v.wolf@gmail.com>
 * Copyright (C) 2020 Andy Kirkham <kirkham.andy@gmail.com>
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

#include "test/testNavStars.hpp"
#include "NavStarsCalculator.hpp"

#include <QMap>
#include <QDebug>
#include <QString>

QTEST_GUILESS_MAIN(TestNavStars)

typedef QMap<QString, double> inputs_t;
typedef QMap<QString, QString> expects_t;

static void
examineCalc(NavStarsCalculator& calc)
{
	qWarning() << "getUTC: " << calc.getUTC();	
	qWarning() << "altAppPrintable" << calc.altAppPrintable();
	qWarning() << "gmstDegreesPrintable" << calc.gmstDegreesPrintable();
	qWarning() << "lmstDegreesPrintable" << calc.lmstDegreesPrintable();
	qWarning() << "shaPrintable" << calc.shaPrintable();
	qWarning() << "decPrintable" << calc.decPrintable();
	qWarning() << "ghaPrintable" << calc.ghaPrintable();
	qWarning() << "lhaPrintable" << calc.lhaPrintable();
	qWarning() << "latPrintable" << calc.latPrintable();
	qWarning() << "lonPrintable" << calc.lonPrintable();
	qWarning() << "hcPrintable" << calc.hcPrintable();
	qWarning() << "znPrintable" << calc.znPrintable();
}

static void performTest(QString utc, inputs_t& inputs, expects_t& expects, bool examine = false)
{
	NavStarsCalculator calc;
	calc.setUTC(utc)
		.setLatDeg(inputs["setLatDeg"])
		.setLonDeg(inputs["setLonDeg"])
		.setJd(inputs["setJd"])
		.setJde(inputs["setJde"])
		.setGmst(inputs["setGmst"])
		.setRaRad(inputs["setRaRad"])
		.setDecRad(inputs["setDecRad"])
		.setAzRad(inputs["setAzRad"])
		.setAltRad(inputs["setAltRad"])
		.setAzAppRad(inputs["setAzAppRad"])
		.setAltAppRad(inputs["setAltAppRad"])
		.execute();
	
	if(examine)
		examineCalc(calc);

	QVERIFY(calc.getUTC() == utc);	
	QVERIFY(calc.altAppPrintable() == expects["altAppPrintable"]);
	QVERIFY(calc.gmstDegreesPrintable() == expects["gmstDegreesPrintable"]);
	QVERIFY(calc.lmstDegreesPrintable() == expects["lmstDegreesPrintable"]);
	QVERIFY(calc.shaPrintable() == expects["shaPrintable"]);
	QVERIFY(calc.decPrintable() == expects["decPrintable"]);
	QVERIFY(calc.ghaPrintable() == expects["ghaPrintable"]);
	QVERIFY(calc.lhaPrintable() == expects["lhaPrintable"]);
	QVERIFY(calc.latPrintable() == expects["latPrintable"]);
	QVERIFY(calc.lonPrintable() == expects["lonPrintable"]);
	QVERIFY(calc.hcPrintable() == expects["hcPrintable"]);
	QVERIFY(calc.znPrintable() == expects["znPrintable"]);
}

void TestNavStars::TestAgainstAlmanacVega()
{
	inputs_t inputs;
	inputs["setLatDeg"] = 56.185647;
	inputs["setLonDeg"] = -2.557428;
	inputs["setJd"] = 2458861.8003982641;
	inputs["setJde"] = 2458861.8012185576;
	inputs["setGmst"] = 220.38903985816341;
	inputs["setRaRad"] = -1.4068775342410453;
	inputs["setDecRad"] = 0.67745840300450966;
	inputs["setAzRad"] = 1.6357089293679654;
	inputs["setAltRad"] = 0.80973585409014781;
	inputs["setAzAppRad"] = 1.6357089293679654;
	inputs["setAltAppRad"] = 0.81001296240472365;

	expects_t expects;	
	expects["altAppPrintable"] = "+46&deg;24.6'";
	expects["gmstDegreesPrintable"] = "+220.389&deg;";
	expects["lmstDegreesPrintable"] = "+217.832&deg;";
	expects["shaPrintable"] = "+80&deg;36.5'";
	expects["decPrintable"] = "+38&deg;48.9'";
	expects["ghaPrintable"] = "+300&deg;59.8'";
	expects["lhaPrintable"] = "+298&deg;26.4'";
	expects["latPrintable"] = "N56&deg;11.1'";
	expects["lonPrintable"] = "W2&deg;33.4'";
	expects["hcPrintable"] = "+46&deg;39.6'";
	expects["znPrintable"] = "+86&deg;36.9'";

	performTest(QString("2020-01-13T07:12:34"), inputs, expects, false);
}


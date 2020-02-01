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
#include "NavStars.hpp"
#include "NavStarsCalculator.hpp"

#ifdef _DEBUG
#include <string>
#include <QDebug>
#endif

QTEST_GUILESS_MAIN(TestNavStars)


void TestNavStars::TestAgainstAlmancVega()
{
#if 0
    QString extraText;
    QMap<QString, QString> strings;
    QMap<QString, double> inputs, expects, actuals;

	// Create a calculator and set date.
    NavStarsCalculator calc;
    calc.setUTC("2020-01-13T07:12:34");

	// Inputs into the calculator under test.
    calc.jd = 2458861.8003982641;
	calc.jde = 2458861.8012185576;
	inputs["getMeanSiderealTime"] = 220.38903985816341;
	inputs["getAltAzPosApparentALT"] = 0.81001296240472365;
	inputs["getAltAzPosApparentAZM"] = 1.6357089293679654;
	inputs["getAltAzPosGeometricALT"] = 0.80973585409014781;
	inputs["getAltAzPosGeometricAZM"] = 1.6357089293679654;
	inputs["getCurrentLocationLatitude"] = 56.208698272705078;
	inputs["getCurrentLocationLongitude"] = -3.0044000148773193;
	inputs["getEquinoxEquatorialPosApparentRA"] = -1.4068775342410453;
	inputs["getEquinoxEquatorialPosApparentDEC"] = 0.67745840300450966;

    NavStarsCalculatorDataProviderFake* pfake = new NavStarsCalculatorDataProviderFake();
    pfake->inputs = inputs;

	// Create the fake DI data provider for the calculator.
    NavStarsCalculatorDataProviderIf::ShPtr dataProvider = 
        NavStarsCalculatorDataProviderIf::ShPtr(pfake);

	// Set calculator to use a fake data provider.
    calc.setDataProvider(dataProvider);

	// List the calculator expectations.
	expects["Hc_rad"] = 0.81005249901058163;
	expects["Zn_rad"] = 1.5059402285716805;
	expects["alt_app_rad"] = 0.81001296240472365;
	expects["alt_rad"] = 0.80973585409014781;
	expects["az_app_rad"] = 1.6357089293679654;
	expects["az_rad"] = 1.6357089293679654;
	expects["gha_rad"] = 5.2533919150750137;
	expects["gmst_deg"] = 220.38903985816341;
	expects["gmst_rad"] = 3.8465143808339683;
	expects["jd"] = 2458861.8003982641;
	expects["jde"] = 2458861.8012185576;
	expects["lat_deg"] = 56.208698272705078;
	expects["lat_rad"] = 0.98102685311875315;
	expects["lha_rad"] = 5.2009552427684378;
	expects["lmst_deg"] = 217.38463984328610;
	expects["lmst_rad"] = 3.7940777085273925;
	expects["lon_deg"] = -3.0044000148773193;
	expects["lon_rad"] = -0.052436672306575845;
	expects["object_dec_rad"] = 0.67745840300450966;

	// Perform the calculations, this is the function being tested.
    calc.getCelestialNavData(actuals);

	// Test the actual vs the expected.
	for (QMap<QString, double>::iterator itor = expects.begin();
		itor != expects.end();
		itor++)
	{
		QVERIFY(actuals.contains(itor.key()));
		QVERIFY(QString::number(itor.value(), 'f', 6) 
			 == QString::number(actuals[itor.key()], 'f', 6));
	}
#endif

#if 0
	// Cannot run these tests as the extraInfoString() uses qc_()
	// which trys to do translations. It seems when called from a
	// Unit Test enviroment this xlation system simply crashes.
    calc.extraInfoStrings(outputs, strings, extraText);

    QVERIFY(strings.contains("alt_app_rad"));
	QVERIFY(strings.contains("gmst_deg"));
	QVERIFY(strings.contains("lmst_deg"));
	QVERIFY(strings.contains("gmst_rad"));
	QVERIFY(strings.contains("object_sha_rad"));
	QVERIFY(strings.contains("object_dec_rad"));
	QVERIFY(strings.contains("gha_rad"));
	QVERIFY(strings.contains("lha_rad"));
	QVERIFY(strings.contains("lat_rad"));
	QVERIFY(strings.contains("lon_rad"));
	QVERIFY(strings.contains("Hc_rad"));
	QVERIFY(strings.contains("Zn_rad"));

    QVERIFY("Ho: +46&deg;24.6'<br/>" == strings["alt_app_rad"]);
	QVERIFY("GMST: 220.388&deg;<br/>" == strings["gmst_deg"]);
	QVERIFY("LMST: 217.384&deg;<br/>" == strings["lmst_deg"]);
	QVERIFY("GHA&#9800;: +220&deg;23.3'<br/>" == strings["gmst_rad"]);
	QVERIFY("SHA: +80&deg;36.5'<br/>" == strings["object_sha_rad"]);
	QVERIFY("DEC: +38&deg;48.9'<br/>" == strings["object_dec_rad"]);
	QVERIFY("GHA: +300&deg;59.8'<br/>" == strings["gha_rad"]);
	QVERIFY("LHA: +297&deg;59.5'<br/>" == strings["lha_rad"]);
	QVERIFY("Lat: N56&deg;12.5'<br/>" == strings["lat_rad"]);
	QVERIFY("Lon: W3&deg;0.3'<br/>" == strings["lon_rad"]);
	QVERIFY("Hc: +46&deg;24.7'<br/>" == strings["Hc_rad"]);
	QVERIFY("Zn: +86&deg;17.0'<br/>" == strings["Zn_rad"]);
#endif
}


/*
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

#include "testOMM.hpp"

QTEST_GUILESS_MAIN(TestOMM)

void TestOMM::testLegacyTle()
{
	QString l0("ISS");
	QString l1("1 25544U 98067A   23190.18395514  .00010525  00000-0  19463-3 0  9990");
	QString l2("2 25544  51.6405 219.5048 0000226  85.5338  22.9089 15.49626907405204");
	PluginSatellites::omm dut(l0, l1, l2);
	QVERIFY(dut.getSourceType() == PluginSatellites::omm::SourceType::LegacyTle);
	QVERIFY(dut.hasValidLegacyTleData() == true);
	QVERIFY(dut.getLine0() == l0);
	QVERIFY(dut.getLine1() == l1);
	QVERIFY(dut.getLine2() == l2);
}

#if 0
void TestOMM::testSpaceTrackFormattedLine2()
{
    QString Line = "2  7530 101.7765 338.2965 0012116 317.3609 153.9519 12.53641545 65932";
    QString dut = Satellites::getSatIdFromLine2(Line);
    QVERIFY(!dut.isEmpty());
    QVERIFY("7530" == dut);
}

void TestOMM::testNoSatDuplication()
{
    QString LineA = "2 07530 101.7770 337.7317 0012122 318.4445 104.4962 12.53641440 65623";
    QString LineB = "2  7530 101.7765 338.2965 0012116 317.3609 153.9519 12.53641545 65932";
    QString dutA = Satellites::getSatIdFromLine2(LineA);
    QString dutB = Satellites::getSatIdFromLine2(LineB);
    QVERIFY(dutA == dutB);
}

void TestOMM::testSatZero()
{
    QString LineA = "2 00000 101.7770 337.7317 0012122 318.4445 104.4962 12.53641440 65623";
    QString LineB = "2     0 101.7765 338.2965 0012116 317.3609 153.9519 12.53641545 65932";
    QString dutA = Satellites::getSatIdFromLine2(LineA);
    QString dutB = Satellites::getSatIdFromLine2(LineB);
    QVERIFY(!dutA.isEmpty());
    QVERIFY(!dutB.isEmpty());
    QVERIFY("0" == dutA);
    QVERIFY("0" == dutB);
}

#endif


/*
 * Copyright (C) 2020 Andy Kirkham
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

#include <QString>
#include "testSatellites.hpp"

QTEST_GUILESS_MAIN(TestSatellites)

void TestSatellites::testCelestrackFormattedLine2()
{
    QString Line = "2 07530 101.7770 337.7317 0012122 318.4445 104.4962 12.53641440 65623";
    QString dut = Satellites::getSatIdFromLine2(Line);
    QVERIFY(!dut.isEmpty());
    QVERIFY("7530" == dut);
}

void TestSatellites::testSpaceTrackFormattedLine2()
{
    QString Line = "2  7530 101.7765 338.2965 0012116 317.3609 153.9519 12.53641545 65932";
    QString dut = Satellites::getSatIdFromLine2(Line);
    QVERIFY(!dut.isEmpty());
    QVERIFY("7530" == dut);
}

void TestSatellites::testNoSatDuplication()
{
    QString LineA = "2 07530 101.7770 337.7317 0012122 318.4445 104.4962 12.53641440 65623";
    QString LineB = "2  7530 101.7765 338.2965 0012116 317.3609 153.9519 12.53641545 65932";
    QString dutA = Satellites::getSatIdFromLine2(LineA);
    QString dutB = Satellites::getSatIdFromLine2(LineB);
    QVERIFY(dutA == dutB);
}



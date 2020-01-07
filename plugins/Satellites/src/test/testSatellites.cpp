/*
 * Copyright (C) 2019 Andy Kirkham <ToDo>
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

void TestSatellites::testIssue408_CelestrackFormattedLine2()
{
    QString Line  = "2 07530 101.7770 337.7317 0012122 318.4445 104.4962 12.53641440 65623";
    QString dut = Satellites::getSatIdFromLine2(Line);
    QVERIFY(!dut.isEmpty());
    QVERIFY("07530" == dut);
}

void TestSatellites::testIssue408_SpaceTrackFormattedLine2()
{
    QString Line = "2  7530 101.7765 338.2965 0012116 317.3609 153.9519 12.53641545 65932";
    QString dut = Satellites::getSatIdFromLine2(Line);
    QVERIFY(!dut.isEmpty());
    QVERIFY("7530" == dut);
}

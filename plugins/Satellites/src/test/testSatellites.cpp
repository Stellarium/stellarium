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
#include <QTemporaryFile>
#include "testSatellites.hpp"

QTEST_GUILESS_MAIN(TestSatellites)

void TestSatellites::testCsvEpochAcrossNewYear()
{
    const QByteArray csvHeader =
        "OBJECT_NAME,OBJECT_ID,EPOCH,MEAN_MOTION,ECCENTRICITY,INCLINATION,RA_OF_ASC_NODE,"
        "ARG_OF_PERICENTER,MEAN_ANOMALY,EPHEMERIS_TYPE,CLASSIFICATION_TYPE,NORAD_CAT_ID,"
        "ELEMENT_SET_NO,REV_AT_EPOCH,BSTAR,MEAN_MOTION_DOT,MEAN_MOTION_DDOT\n";

    const QByteArray contents = csvHeader +
        "SATELLITE A,1974-089A,2024-12-31T12:00:00.123456,12.53641440,0.0012122,"
        "101.7770,337.7317,318.4445,104.4962,0,U,7530,999,6562,0,0,0\n"
        "SATELLITE B,1974-089B,2025-01-01T00:00:00.000000,12.53641440,0.0012122,"
        "101.7770,337.7317,318.4445,104.4962,0,U,7531,999,6562,0,0,0\n";

    QTemporaryFile file;
    QVERIFY(file.open());
    QCOMPARE(file.write(contents), qint64(contents.size()));
    QVERIFY(file.seek(0));

    TleDataHash elements;
    Satellites::parseDataFile(file, elements);
    QCOMPARE(elements.size(), 2);
    // TLE line 1 stores the 14-character epoch at columns 19–32.
    QCOMPARE(elements.value("7530").first.mid(18, 14), QStringLiteral("24366.50000143"));
    QCOMPARE(elements.value("7531").first.mid(18, 14), QStringLiteral("25001.00000000"));
}

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

void TestSatellites::testSatZero()
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

void TestSatellites::testParseDataFileResult_data()
{
    // The updater uses this result to decide whether auto-removal is safe.
    // A source with no parseable elements fails, while a source with only
    // duplicate elements succeeds even though the combined hash stays the same.
    QTest::addColumn<QByteArray>("contents");
    QTest::addColumn<bool>("parsed");

    const QByteArray csvHeader =
        "OBJECT_NAME,OBJECT_ID,EPOCH,MEAN_MOTION,ECCENTRICITY,INCLINATION,RA_OF_ASC_NODE,"
        "ARG_OF_PERICENTER,MEAN_ANOMALY,EPHEMERIS_TYPE,CLASSIFICATION_TYPE,NORAD_CAT_ID,"
        "ELEMENT_SET_NO,REV_AT_EPOCH,BSTAR,MEAN_MOTION_DOT,MEAN_MOTION_DDOT\n";

    QTest::newRow("error response") << QByteArray("Invalid query\n") << false;
    QTest::newRow("html response") << QByteArray("<!doctype html>\n<p>Service unavailable</p>\n") << false;
    QTest::newRow("empty CSV") << csvHeader << false;
    QTest::newRow("duplicate TLE") << QByteArray(
        "SATELLITE\n"
        "1 07530U 74089A   26001.00000000  .00000000  00000-0  00000-0 0  9992\n"
        "2 07530 101.7770 337.7317 0012122 318.4445 104.4962 12.53641440 65623\n") << true;
    QTest::newRow("duplicate CSV") << (csvHeader +
        "SATELLITE,1974-089A,2026-01-01T00:00:00.000000,12.53641440,0.0012122,"
        "101.7770,337.7317,318.4445,104.4962,0,U,7530,999,6562,0,0,0\n") << true;
}

void TestSatellites::testParseDataFileResult()
{
    QFETCH(QByteArray, contents);
    QFETCH(bool, parsed);

    QTemporaryFile file;
    QVERIFY(file.open());
    QCOMPARE(file.write(contents), qint64(contents.size()));
    QVERIFY(file.seek(0));

    TleData existing{};
    existing.id = "7530";
    existing.name = "Existing entry";
    existing.addThis = true;
    TleDataHash elements{{existing.id, existing}};

    QCOMPARE(Satellites::parseDataFile(file, elements), parsed);
    QCOMPARE(elements.size(), 1);
    QCOMPARE(elements.value(existing.id).name, existing.name);
    QVERIFY(elements.value(existing.id).addThis);
}

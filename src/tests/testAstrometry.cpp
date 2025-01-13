/*
 * Stellarium
 * Copyright (C) 2024 Henry W. Leung
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
#include <QDir>

#include "tests/testAstrometry.hpp"
#include "modules/Star.hpp"
#include "modules/StarMgr.hpp"
#include "modules/ZoneArray.hpp"
#include "StelObjectType.hpp"

QTEST_GUILESS_MAIN(TestAstrometry)

void TestAstrometry::initTestCase()
{
    // Define the directory to search in
    QDir dir("../../stars/hip_gaia3/");
    
    // Make sure the directory exists
    if (!dir.exists()) {
        qDebug() << "Directory does not exist!";
        return;
    }

    // make a list of star catalog files stars_0_*.cat, stars_1_*.cat, stars_2_*.cat and stars_3_*.cat
    QStringList filters;
    filters << "stars_0_*.cat" << "stars_1_*.cat" << "stars_2_*.cat" << "stars_3_*.cat";

    // loop through the filters and set the ZoneArray pointers
    for (int i = 0; i < filters.size(); i++)
    {
        dir.setNameFilters(QStringList() << filters[i]);
        QStringList files = dir.entryList(QDir::Files);
        QString file;

        if (!files.isEmpty()) {
            qDebug() << "Found files:" << files[0];
            file = files[0];
        } else {
            qDebug() << "No matching files found.";
        }

        file = dir.path() + "/" + file;
        ZoneArray* z = ZoneArray::create(file, true);
        zoneArrays.append(z);
    }
}
void TestAstrometry::test6DAstrometryPropagation(StarId gaiaID, float depoch_from_catalogs, double expectedRA, double expectedDEC, double expectedPlx, double expectedPMRA, double expectedPMDEC, double expectedRV)
{
    double RA = 0, DE = 0, Plx = 0, pmra = 0, pmdec = 0, RV = 0;
    for (int i = 0; i < zoneArrays.size(); i++)
    {
        zoneArrays[i]->searchGaiaIDepochPos(gaiaID, depoch_from_catalogs, RA, DE, Plx, pmra, pmdec, RV);
        if (RA) {
            break;
        }
    }

    RA = RA / M_PI * 180.;
    DE = DE / M_PI * 180.;

    QVERIFY2(abs(RA - expectedRA) < 0.01, QString("RA: %1, expectedRA: %2 are not close enough").arg(RA).arg(expectedRA).toStdString().c_str());
    QVERIFY2(abs(DE - expectedDEC) < 0.01, QString("DE: %1, expectedDEC: %2 are not close enough").arg(DE).arg(expectedDEC).toStdString().c_str());
    QVERIFY2(abs(Plx - expectedPlx) < 1, QString("Plx: %1, expectedPlx: %2 are not close enough").arg(Plx).arg(expectedPlx).toStdString().c_str());
    QVERIFY2(abs(pmra - expectedPMRA) < 5, QString("pmra: %1, expectedPMRA: %2 are not close enough").arg(pmra).arg(expectedPMRA).toStdString().c_str());
    QVERIFY2(abs(pmdec - expectedPMDEC) < 5, QString("pmdec: %1, expectedPMDEC: %2 are not close enough").arg(pmdec).arg(expectedPMDEC).toStdString().c_str());
    QVERIFY2(abs(RV - expectedRV) < 2, QString("RV: %1, expectedRV: %2 are not close enough").arg(RV).arg(expectedRV).toStdString().c_str());
}

void TestAstrometry::testBarnardStar()  // test the Barnard's Star
{
    StarId gaiaID = 4472832130942575872;
    // star catalog epoch, to make sure the parameters used to calculate the expected values with astropy are consistent with the catalog
    test6DAstrometryPropagation(gaiaID, 0.f, 269.4485025254, 4.7394200511, 546.9759, -801.5510, 10362.3942, -110.1);
    // +50,000 years from star catalog epoch
    test6DAstrometryPropagation(gaiaID, 50000.0f, 94.31898499, 45.53314217, 1000. / 5.96896177, -106.8912309, -968.34471548, 139.49528674);
    // -50,000 years from star catalog epoch
    test6DAstrometryPropagation(gaiaID, -50000.0f, 272.05148359, -26.85371759, 1000. / 8.76802626, -38.93178342, 450.23851363, -141.01396308);
    // +5,000 years from star catalog epoch
    test6DAstrometryPropagation(gaiaID, 5000.0f, 267.78462028, 24.68056845, 1000. / 1.34632329, -1620.90567344, 19094.44118355, -72.63017957);
    // -5,000 years from star catalog epoch
    test6DAstrometryPropagation(gaiaID, -5000.0f, 270.28908849, -6.13067591, 1000. / 2.435383, -452.77813098, 5839.89405934, -125.15516343);
}

void TestAstrometry::testCrux()  // test a random Gaia star within the Crux that has no radial velocity
{
    StarId gaiaID = 6059204728680270080;
    // star catalog epoch, to make sure the parameters used to calculate the expected values with astropy are consistent with the catalog
    test6DAstrometryPropagation(gaiaID, 0.f, 187.43783675, -59.02388257, 1000. / 144.35847096, -2.733, -21.946, 0.);
    // +50000 years from star catalog epoch
    test6DAstrometryPropagation(gaiaID, 50000.0f, 187.36342614, -59.32866405, 1000. / 144.36052582, -2.75739473, -21.94230771, 0.08113282);
    // -50000 years from star catalog epoch
    test6DAstrometryPropagation(gaiaID, -50000.0f, 187.51094009, -58.7190592, 1000. / 144.36056489, -2.7089515, -21.94834105, -0.08113285);
    // +5,000 years from star catalog epoch
    test6DAstrometryPropagation(gaiaID, 5000.0f, 187.43045509, -59.05436291, 1000. / 144.35848975, -2.73542374, -21.94569162, 0.0081134);
    // -5,000 years from star catalog epoch
    test6DAstrometryPropagation(gaiaID, -5000.0f, 187.44520535, -58.99340181, 1000. / 144.35849365, -2.73057972, -21.94629486, -0.0081134);
}

void TestAstrometry::testBrightNoPlx()  // test a bright Gaia star without parallax but with radial velocity
{
    StarId gaiaID = 1998148532777850880;
    // star catalog epoch, to make sure the parameters used to calculate the expected values with astropy are consistent with the catalog
    test6DAstrometryPropagation(gaiaID, 0.f, 358.59593000622834, 57.49936804016163, 0., -4.6991464250248995, -3.1120532783839017, -55.2);
    // simply check the case where radial velocity can be inf/nan when there is no parallax for a star
    test6DAstrometryPropagation(gaiaID, 1.f, 358.59593000622834, 57.49936804016163, 0., -4.6991464250248995, -3.1120532783839017, -55.2);
}

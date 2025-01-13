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

#ifndef TESTASTROMETRY_HPP
#define TESTASTROMETRY_HPP

#include "modules/Star.hpp"
#include "modules/StarMgr.hpp"
#include "modules/ZoneArray.hpp"
#include <QObject>
#include <QtTest>

class TestAstrometry : public QObject
{
   Q_OBJECT
private slots:
   void initTestCase();
   //! Test the propagation of astrometry for a given star.
   //! @param gaiaID the Gaia ID of the star
   //! @param depoch_from_catalogs the epoch difference from the star catalog
   //! @param expectedRA the expected right ascension in degrees
   //! @param expectedDEC the expected declination in degrees
   //! @param expectedPlx the expected parallax in mas
   //! @param expectedPMRA the expected proper motion in right ascension in mas/yr
   //! @param expectedPMDEC the expected proper motion in declination in mas/yr
   //! @param expectedRV the expected radial velocity in km/s
   void test6DAstrometryPropagation(StarId gaiaID,
                                    float   depoch_from_catalogs,
                                    double  expectedRA,
                                    double  expectedDEC,
                                    double  expectedPlx,
                                    double  expectedPMRA,
                                    double  expectedPMDEC,
                                    double  expectedRV);
   void testBarnardStar();
   void testCrux();
   void testBrightNoPlx();
private:
   QList<ZoneArray *> zoneArrays;
};

#endif // _TESTASTROMETRY_HPP

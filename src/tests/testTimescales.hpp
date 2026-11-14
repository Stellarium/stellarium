/*
 * Stellarium
 * Copyright (C) 2026 Sylvain Simard
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

#ifndef TESTTIMESCALES_HPP
#define TESTTIMESCALES_HPP

#include <QObject>
#include <QtTest>

//! (SS) 2026-10-09 Tests of the time scale helpers of the JPL Horizons path (HorizonsZones). No StelCore, no StelApp.
class TestTimescales : public QObject
{
	Q_OBJECT

private slots:
	void testZoneBoundaries();
	void testZoneOutOfRange();
	void testZone4Zone5UseTT();
	void testZone4StartMoves();
	void testUtcDayStart();
};

#endif // TESTTIMESCALES_HPP

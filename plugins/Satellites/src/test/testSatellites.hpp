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

#ifndef TESTSATELLITES_HPP
#define TESTSATELLITES_HPP

#include <QtTest>

#include "Satellites.hpp"

class TestSatellites : public QObject
{
Q_OBJECT
private slots:
    void testCelestrackFormattedLine2();
    void testSpaceTrackFormattedLine2();
    void testNoSatDuplication();
};

#endif // TESTSATELLITES_HPP

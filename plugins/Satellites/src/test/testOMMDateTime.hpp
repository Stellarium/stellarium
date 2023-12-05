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

#ifndef TESTOMMDATETIME_HPP
#define TESTOMMDATETIME_HPP

#include <QtTest>

#include "OMMDateTime.hpp"

class TestOMMDateTime : public QObject
{
	Q_OBJECT
private slots:
	void testEpoch_DEFAULT();
	void testEpoch_TLE();
	void testEpoch_ISO();
	void testOperatorEquals();
	void testCopyCTOR();
	void testToISO8601();
};

#endif // TESTDATETIME_HPP

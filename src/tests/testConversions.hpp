/*
 * Stellarium
 * Copyright (C) 2013 Alexander Wolf
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

#ifndef _TESTCONVERSIONS_HPP_
#define _TESTCONVERSIONS_HPP_

#include <QObject>
#include <QtTest>

class TestConversions : public QObject
{
Q_OBJECT
private slots:
	void testHMSToRad();
	void testDMSToRad();
	void testRadToHMS();
	void testRadToDMS();
};

#endif // _TESTCONVERSIONS_HPP_


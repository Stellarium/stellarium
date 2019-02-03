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

#ifndef TESTCONVERSIONS_HPP
#define TESTCONVERSIONS_HPP

#include <QObject>
#include <QTest>

class TestConversions : public QObject
{
Q_OBJECT
private slots:
	void testHMSToRad();
	void testDMSToRad();
	void testRadToHMS();
	void testRadToDMS();	
	void testDDToDMS();
	void testRadToDD();
	void testStringCoordinateToRad();
	void testHMSToHours();
	void testHMSStringToHours();
	void testHoursToHMSStr();
	void testRadToHMSStr();
};

#endif // _TESTCONVERSIONS_HPP


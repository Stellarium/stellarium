/*
 * Stellarium
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

#ifndef TESTOMM_HPP
#define TESTOMM_HPP

#include <QtTest>

#include "OMM.hpp"

class TestOMM : public QObject
{
	Q_OBJECT
private slots:
	void testLegacyTle();
	void testXMLread();
	void testCtorMap();
	void testProcessTleLegacyLine0();
	void testProcessTleLegacyLine1();
	void testProcessTleLegacyLine2();
	void testLegacyTleVsXML();
	void testLegacyTleVsJSON();
	void testCopyCTOR();
	void testOperatorEquals();
	void testFetchJSONObj();
	void testToVariantMap();
};

#endif // TESTOMM_HPP

/*
 * Copyright (C) 2023 Alexander Wolf
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
#include "SolarSystemEditor.hpp"
#include "testSolarSystemEditor.hpp"
#include "MinorPlanet.hpp"
#include "Comet.hpp"

QTEST_GUILESS_MAIN(TestSolarSystemEditor)

void TestSolarSystemEditor::testUnpackingNumbers()
{
	QCOMPARE(SolarSystemEditor::unpackAlphanumericNumber('0', 1), 1);
	QCOMPARE(SolarSystemEditor::unpackAlphanumericNumber('9', 1), 91);
	QCOMPARE(SolarSystemEditor::unpackAlphanumericNumber('A', 1), 101);
	QCOMPARE(SolarSystemEditor::unpackAlphanumericNumber('Z', 1), 351);
}


void TestSolarSystemEditor::testUnpackingAsteroids()
{
	//Examples from https://www.minorplanetcenter.org/iau/info/PackedDes.html
	QCOMPARE(SolarSystemEditor::unpackMinorPlanetIAUDesignation("I95X00A"), QString("1895 XA"));
	QCOMPARE(SolarSystemEditor::unpackMinorPlanetIAUDesignation("J95X00A"), QString("1995 XA"));
	QCOMPARE(SolarSystemEditor::unpackMinorPlanetIAUDesignation("J95X01L"), QString("1995 XL1"));
	QCOMPARE(SolarSystemEditor::unpackMinorPlanetIAUDesignation("J95F13B"), QString("1995 FB13"));
	QCOMPARE(SolarSystemEditor::unpackMinorPlanetIAUDesignation("J98SA8Q"), QString("1998 SQ108"));
	QCOMPARE(SolarSystemEditor::unpackMinorPlanetIAUDesignation("J98SC7V"), QString("1998 SV127"));
	QCOMPARE(SolarSystemEditor::unpackMinorPlanetIAUDesignation("J98SG2S"), QString("1998 SS162"));
	QCOMPARE(SolarSystemEditor::unpackMinorPlanetIAUDesignation("K99AJ3Z"), QString("2099 AZ193"));
	QCOMPARE(SolarSystemEditor::unpackMinorPlanetIAUDesignation("K08Aa0A"), QString("2008 AA360"));
	QCOMPARE(SolarSystemEditor::unpackMinorPlanetIAUDesignation("K07Tf8A"), QString("2007 TA418"));
	QCOMPARE(SolarSystemEditor::unpackMinorPlanetIAUDesignation("PLS2040"), QString("2040 P-L"));
	QCOMPARE(SolarSystemEditor::unpackMinorPlanetIAUDesignation("T1S3138"), QString("3138 T-1"));
	QCOMPARE(SolarSystemEditor::unpackMinorPlanetIAUDesignation("T2S1010"), QString("1010 T-2"));
	QCOMPARE(SolarSystemEditor::unpackMinorPlanetIAUDesignation("T3S4101"), QString("4101 T-3"));
}


void TestSolarSystemEditor::testUnpackingAsteroidsHTML()
{
	QCOMPARE(MinorPlanet::renderIAUDesignationinHtml(SolarSystemEditor::unpackMinorPlanetIAUDesignation("J95X01L")), QString("1995 XL<sub>1</sub>"));
	QCOMPARE(MinorPlanet::renderIAUDesignationinHtml(SolarSystemEditor::unpackMinorPlanetIAUDesignation("J95F13B")), QString("1995 FB<sub>13</sub>"));
	QCOMPARE(MinorPlanet::renderIAUDesignationinHtml(SolarSystemEditor::unpackMinorPlanetIAUDesignation("J98SA8Q")), QString("1998 SQ<sub>108</sub>"));
}

void TestSolarSystemEditor::testRenderAsteroidsHTML()
{
	QCOMPARE(MinorPlanet::renderIAUDesignationinHtml("1999 RQ36"), QString("1999 RQ<sub>36</sub>"));
	QCOMPARE(MinorPlanet::renderIAUDesignationinHtml("1999 JU3"), QString("1999 JU<sub>3</sub>"));
	QCOMPARE(MinorPlanet::renderIAUDesignationinHtml("1971 FA"), QString("1971 FA"));
	QCOMPARE(MinorPlanet::renderIAUDesignationinHtml("2000 WR106"), QString("2000 WR<sub>106</sub>"));
}


void TestSolarSystemEditor::testUnpackingComets()
{
	//Examples from http://www.minorplanetcenter.org/iau/info/PackedDes.html and an actual data file.
	QCOMPARE(SolarSystemEditor::unpackCometIAUDesignation("J95A010"), QString("1995 A1"));
	QCOMPARE(SolarSystemEditor::unpackCometIAUDesignation("J94P01b"), QString("1994 P1-B"));
	QCOMPARE(SolarSystemEditor::unpackCometIAUDesignation("J94P010"), QString("1994 P1"));
	QCOMPARE(SolarSystemEditor::unpackCometIAUDesignation("K48X130"), QString("2048 X13"));
	QCOMPARE(SolarSystemEditor::unpackCometIAUDesignation("K33L89c"), QString("2033 L89-C"));
	QCOMPARE(SolarSystemEditor::unpackCometIAUDesignation("K88AA30"), QString("2088 A103"));
	QCOMPARE(SolarSystemEditor::unpackCometIAUDesignation("K10W00K"), QString("2010 WK"));
	QCOMPARE(SolarSystemEditor::unpackCometIAUDesignation("K12N00J"), QString("2012 NJ"));
	QCOMPARE(SolarSystemEditor::unpackCometIAUDesignation("K21H00S"), QString("2021 HS"));
}


void TestSolarSystemEditor::testRenderCometsHTML()
{
	QCOMPARE(Comet::renderDiscoveryDesignationHtml("1999a"), QString("1999a"));
	QCOMPARE(Comet::renderDiscoveryDesignationHtml("1999a1"), QString("1999a<sub>1</sub>"));
	QCOMPARE(Comet::renderDiscoveryDesignationHtml("2000f12"), QString("2000f<sub>12</sub>"));
}

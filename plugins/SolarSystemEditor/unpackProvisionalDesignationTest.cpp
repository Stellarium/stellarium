/*
Code for unpacking MPC packed provisional designations and unit tests for it
Copyright (C) 2010  Bogdan Marinov

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <QtTest/QtTest>

#include <QChar>
#include <QString>
#include <QRegExp>

class TestClass: public QObject
{
	Q_OBJECT
public:
	//Without any additional information, one can distinguish between
	//packed designations of comets and minor planets only by the last
	//character: 0 or lowecase letter for comets, uppercase letter for asteroids

	QString unpackMinorPlanetProvisionalDesignation (QString packedDesignation, bool returnHTML = false);
	//Comet designations don't require subscript, so no HTML option is needed.
	QString unpackCometProvisionalDesignation (QString packedDesignation);

private:
	int unpackYearNumber (QChar prefix, int lastTwoDigits);
	int unpackAlphanumericNumber (QChar prefix, int lastDigit);

private slots:
	void testUnpackingAsteroids();
	void testUnpackingAsteroidsHTML();
	void testUnpackingComets();
};

int TestClass::unpackYearNumber (QChar prefix, int lastTwoDigits)
{
	int year = lastTwoDigits;
	if (prefix == 'I')
		year += 1800;
	else if (prefix == 'J')
		year += 1900;
	else if (prefix == 'K')
		year += 2000;
	else
		year = 0; //Error

	return year;
}

//Can be used both for minor planets and comets with no additional modification,
//as the regular expression for comets will match only capital letters.
int TestClass::unpackAlphanumericNumber (QChar prefix, int lastDigit)
{
	int cycleCount = lastDigit;
	if (prefix.isDigit())
		cycleCount += prefix.digitValue() * 10;
	else if (prefix.isLetter() && prefix.isUpper())
		cycleCount += (10 + prefix.toAscii() - QChar('A').toAscii()) * 10;
	else if (prefix.isLetter() && prefix.isLower())
		cycleCount += (10 + prefix.toAscii() - QChar('a').toAscii()) * 10 + 26*10;
	else
		cycleCount = 0; //Error

	return cycleCount;
}

QString TestClass::unpackMinorPlanetProvisionalDesignation (QString packedDesignation, bool returnHTML)
{
	QRegExp packedFormat("^([IJK])(\\d\\d)([A-Z])([\\dA-Za-z])(\\d)([A-Z])$");
	if (packedFormat.indexIn(packedDesignation) != 0)
	{
		return QString();
	}

	//Year
	QChar yearPrefix = packedFormat.cap(1).at(0);
	int yearLastTwoDigits = packedFormat.cap(2).toInt();
	int year = unpackYearNumber(yearPrefix, yearLastTwoDigits);

	//Letters
	QString halfMonthLetter = packedFormat.cap(3);
	QString secondLetter = packedFormat.cap(6);

	//Second letter cycle count
	QChar cycleCountPrefix = packedFormat.cap(4).at(0);
	int cycleCountLastDigit = packedFormat.cap(5).toInt();
	int cycleCount = unpackAlphanumericNumber(cycleCountPrefix, cycleCountLastDigit);

	//Assemble the unpacked provisional designation
	QString result = QString("%1 %2%3").arg(year).arg(halfMonthLetter).arg(secondLetter);
	if (cycleCount != 0)
	{
		if(returnHTML)
			result.append(QString("<sub>%1</sub>").arg(cycleCount));
		else
			result.append(QString::number(cycleCount));
	}

	return result;
}

QString TestClass::unpackCometProvisionalDesignation(QString packedDesignation)
{
	QRegExp packedFormat("^([IJK])(\\d\\d)([A-Z])([\\dA-Z])(\\d)([0a-z])$");
	if (packedFormat.indexIn(packedDesignation) != 0)
	{
		return QString();
	}

	//Year
	QChar yearPrefix = packedFormat.cap(1).at(0);
	int yearLastTwoDigits = packedFormat.cap(2).toInt();
	int year = unpackYearNumber(yearPrefix, yearLastTwoDigits);

	//Half-month letter
	QString halfMonthLetter = packedFormat.cap(3);

	//Half-month number
	QChar numberPrefix = packedFormat.cap(4).at(0);
	int numberLastDigit = packedFormat.cap(5).toInt();
	int halfMonthNumber = unpackAlphanumericNumber(numberPrefix, numberLastDigit);

	//Fragment
	QChar fragmentDesignation = packedFormat.cap(6).at(0);

	QString result = QString("%1 %2%3").arg(year).arg(halfMonthLetter).arg(halfMonthNumber);
	if (fragmentDesignation != '0')
		result.append(QString("-%1").arg(fragmentDesignation.toUpper()));
		//Only uppercase fragment suffixes are alowed/used?
		//http://www.minorplanetcenter.org/iau/lists/CometResolution.html

	return result;
}

void TestClass::testUnpackingAsteroids()
{
	//Examples from http://www.minorplanetcenter.org/iau/info/PackedDes.html
	QCOMPARE(unpackMinorPlanetProvisionalDesignation("J95X00A"), QString("1995 XA"));
	QCOMPARE(unpackMinorPlanetProvisionalDesignation("J95X01L"), QString("1995 XL1"));
	QCOMPARE(unpackMinorPlanetProvisionalDesignation("J95F13B"), QString("1995 FB13"));
	QCOMPARE(unpackMinorPlanetProvisionalDesignation("J98SA8Q"), QString("1998 SQ108"));
	QCOMPARE(unpackMinorPlanetProvisionalDesignation("J98SC7V"), QString("1998 SV127"));
	QCOMPARE(unpackMinorPlanetProvisionalDesignation("J98SG2S"), QString("1998 SS162"));
	QCOMPARE(unpackMinorPlanetProvisionalDesignation("K99AJ3Z"), QString("2099 AZ193"));
	QCOMPARE(unpackMinorPlanetProvisionalDesignation("K08Aa0A"), QString("2008 AA360"));
	QCOMPARE(unpackMinorPlanetProvisionalDesignation("K07Tf8A"), QString("2007 TA418"));
}

void TestClass::testUnpackingAsteroidsHTML()
{
	QCOMPARE(unpackMinorPlanetProvisionalDesignation("J95X01L", true), QString("1995 XL<sub>1</sub>"));
	QCOMPARE(unpackMinorPlanetProvisionalDesignation("J95F13B", true), QString("1995 FB<sub>13</sub>"));
	QCOMPARE(unpackMinorPlanetProvisionalDesignation("J98SA8Q", true), QString("1998 SQ<sub>108</sub>"));
}

void TestClass::testUnpackingComets()
{
	//Examples from http://www.minorplanetcenter.org/iau/info/PackedDes.html
	QCOMPARE(unpackCometProvisionalDesignation("J95A010"), QString("1995 A1"));
	QCOMPARE(unpackCometProvisionalDesignation("J94P01b"), QString("1994 P1-B"));
	QCOMPARE(unpackCometProvisionalDesignation("J94P010"), QString("1994 P1"));
	QCOMPARE(unpackCometProvisionalDesignation("K48X130"), QString("2048 X13"));
	QCOMPARE(unpackCometProvisionalDesignation("K33L89c"), QString("2033 L89-C"));
	QCOMPARE(unpackCometProvisionalDesignation("K88AA30"), QString("2088 A103"));
}

QTEST_MAIN(TestClass)
#include "unpackProvisionalDesignationTest.moc"

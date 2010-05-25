
#include <QString>
#include <QDateTime>
#include <QDate>
#include <QMap>
#include <QList>
#include <QtGlobal>
#include <QLocale>
#include <QtDebug>
#include "StelUtils.hpp"
#include "tests/testDates.hpp"

QTEST_MAIN(TestDates);

void TestDates::dateRoundTrip()
{
	QMap<double, QString> map;
	map[0.0] = "-4712-01-01T12:00:00";
	map[-1.0] = "-4713-12-31T12:00:00";
	map[2454466.0] = "2007-12-31T12:00:00";
	map[1721058.0] = "0000-01-01T12:00:00";
	map[2500000.0] = "2132-08-31T12:00:00";
	map[366.0] = "-4711-01-01T12:00:00";
	map[2454534] = "2008-03-08T12:00:00";
	map[2299161.0] = "1582-10-15T12:00:00";
	map[2454466.5] = "2008-01-01T00:00:00";
	map[1720692.0] = "-0002-12-31T12:00:00";
	map[1720693.0] = "-0001-01-01T12:00:00";
	map[2400000.0] = "1858-11-16T12:00:00";
	map[2110516.00000] = "1066-04-12T12:00:00";
	map[1918395.00000] = "0540-04-12T12:00:00";
	map[1794575.00000] = "0201-04-12T12:00:00";
	map[1757319.00000] = "0099-04-12T12:00:00";
	map[1721424.0] = "0001-01-01T12:00:00";
	map[1721789.0] = "0002-01-01T12:00:00";
	map[-31.0] = "-4713-12-01T12:00:00";
	map[-61.0] = "-4713-11-01T12:00:00";
	map[-92.0] = "-4713-10-01T12:00:00";
	map[-122.0] = "-4713-09-01T12:00:00";
	map[-153.0] = "-4713-08-01T12:00:00";
	map[-184.0] = "-4713-07-01T12:00:00";
	map[-214.0] = "-4713-06-01T12:00:00";
	map[-245.0] = "-4713-05-01T12:00:00";
	map[-275.0] = "-4713-04-01T12:00:00";
	map[-306.0] = "-4713-03-01T12:00:00";
	map[-334.0] = "-4713-02-01T12:00:00"; // 28 days
	map[-365.0] = "-4713-01-01T12:00:00";
	map[-699.0] = "-4714-02-01T12:00:00"; // 28 days
	map[-1064.0] = "-4715-02-01T12:00:00"; // 28 days
	map[-1430.0] = "-4716-02-01T12:00:00"; // 29 days
	map[-1795.0] = "-4717-02-01T12:00:00"; // 28 days

	bool ok;
	for (QMap<double, QString>::ConstIterator i=map.constBegin();i!=map.constEnd();++i)
	{
		QCOMPARE(StelUtils::julianDayToISO8601String(i.key()), i.value());
		double tmp = StelUtils::getJulianDayFromISO8601String(i.value(), &ok);
		QVERIFY(ok);
		if (i.key()!=0.0)
			qFuzzyCompare(i.key(), tmp);
		else
			qFuzzyCompare(i.key()+1.0, tmp+1.0);
	}
}


void TestDates::formatting()
{
	QLocale usEN;

	// test formatting of StelUtils::localeDateString, the fall-back if QDateTime cannot do it.
	QLocale::setDefault(QLocale::German);
	QVERIFY2(QString::compare(StelUtils::localeDateString(2008, 03, 10, 0), QString("10.03.08")) == 0,
			 qPrintable("german for 2008-03-10 wrong: " + (StelUtils::localeDateString(2008, 03, 10, 0))));
	QVERIFY2(QString::compare(StelUtils::localeDateString(8, 03, 10, 0), QString("10.03.08")) == 0,
			 qPrintable("german for 8-03-10 wrong: " + (StelUtils::localeDateString(8, 03, 10, 0))));
	QVERIFY2(QString::compare(StelUtils::localeDateString(-8, 03, 10, 0), QString("10.03.-8")) == 0,
			 qPrintable("german for -8-03-10 wrong: " + (StelUtils::localeDateString(-8, 03, 10, 0))));
	QVERIFY2(QString::compare(StelUtils::localeDateString(-1118, 03, 10, 0), QString("10.03.-18")) == 0,
			 qPrintable("german for -1118-03-10 wrong: " + (StelUtils::localeDateString(-1118, 03, 10, 0))));
	QVERIFY2(QString::compare(StelUtils::localeDateString(-5118, 03, 10, 0), QString("10.03.-18")) == 0,
			 qPrintable("german for -5118-03-10 wrong: " + (StelUtils::localeDateString(-5118, 03, 10, 0))));

	QVERIFY2(-18 == (-5118 % 100), qPrintable("modulus arithmetic works diff: " + QString("%1").arg(-5118 % 100)));

	// test arbitrary fmt
	QLocale::setDefault(usEN);

	QString easyLong("d dd ddd dddd M MM MMM MMMM yy yyyy");
	QVERIFY2(QString::compare(QString("9 09 Sun Sunday 3 03 Mar March 08 2008"), StelUtils::localeDateString(2008, 3, 9, 6, easyLong)) == 0,
			 qPrintable("formatter1 not working: " + StelUtils::localeDateString(2008, 3, 9, 6, easyLong)));
	QString hardLong("dddddddd '''doh' ''yyyyyyy");
	QVERIFY2(QString::compare(QString("SundaySunday 'doh '200808y"), StelUtils::localeDateString(2008, 3, 9, 6, hardLong)) == 0,
			 qPrintable("formatter2 not working: " + StelUtils::localeDateString(2008, 3, 9, 6, hardLong)));

	// test detection of offset from UTC.
	double mar122008 = QDate(2008,3,12).toJulianDay();
	qFuzzyCompare(StelUtils::getGMTShiftFromQT(mar122008), -4.f);
	double mar012008 = QDate(2008,3,1).toJulianDay();
	qFuzzyCompare(StelUtils::getGMTShiftFromQT(mar012008), -5.f);

}

void TestDates::testRolloverAndValidity()
{
	QVERIFY2(31==StelUtils::numberOfDaysInMonthInYear(1, 2008), "1a");
	QVERIFY2(29==StelUtils::numberOfDaysInMonthInYear(2, 2008), "1b");
	QVERIFY2(28==StelUtils::numberOfDaysInMonthInYear(2, 2007), "1c");

	QVERIFY2(29==StelUtils::numberOfDaysInMonthInYear(2, 2000), "1d");
	QVERIFY2(28==StelUtils::numberOfDaysInMonthInYear(2, 1900), "1e");

	QVERIFY2(28==StelUtils::numberOfDaysInMonthInYear(2, 1001), "1f");
	QVERIFY2(29==StelUtils::numberOfDaysInMonthInYear(2, 1000), "1g");
	QVERIFY2(29==StelUtils::numberOfDaysInMonthInYear(2, 1200), "1h");

	QVERIFY2(29==StelUtils::numberOfDaysInMonthInYear(2, 0), "1i");
	QVERIFY2(28==StelUtils::numberOfDaysInMonthInYear(2, 1), "1j");
	QVERIFY2(29==StelUtils::numberOfDaysInMonthInYear(2, -4), "1k");

	int dy, dm, dd, dh, dmin,ds;

	QVERIFY2(StelUtils::changeDateTimeForRollover(2008, 1, 32, 12, 0, 0, &dy, &dm, &dd, &dh, &dmin, &ds), "1l");
	QVERIFY(2008==dy);
	QVERIFY(2==dm);
	QVERIFY(1==dd);
	QVERIFY(12==dh);
	QVERIFY(0==dmin);
	QVERIFY(0==ds);

	QVERIFY2(StelUtils::changeDateTimeForRollover(-1, 12, 32, 12, 0, 0, &dy, &dm, &dd, &dh, &dmin, &ds), "1m");
	QVERIFY(0==dy);
	QVERIFY(1==dm);
	QVERIFY(1==dd);
	QVERIFY(12==dh);
	QVERIFY(0==dmin);
	QVERIFY(0==ds);

	QVERIFY2(StelUtils::changeDateTimeForRollover(-1, 11, 45, 12, 0, 0, &dy, &dm, &dd, &dh, &dmin, &ds), "1n");
	QVERIFY(-1==dy);
	QVERIFY(12==dm);
	QVERIFY(15==dd);
	QVERIFY(12==dh);
	QVERIFY(0==dmin);
	QVERIFY(0==ds);
}

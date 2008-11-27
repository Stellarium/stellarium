#include <config.h>

#include <QString>
#include <QDateTime>
#include <QDate>
#include <QtGlobal>
#include <QLocale>
#include <QtDebug>
#include "StelUtils.hpp"

using namespace std;

void _assert(bool b, QString msg = "")
{
	if (!b)
	{
		qFatal("FAILED: %s",  qPrintable(msg));
	}
}

void _assertEquals(int a, int b, QString msg="")
{
	_assert(a == b, msg);
}

void dateRoundTrip()
{
	double t1d[] = { 
		0.0,
		-1.0,
		2454466.0,
		1721058.0,
		2500000.0,
		366.0,
		2454534,
		2299161.0,
		2454466.5,
		1720692.0,
		1720693.0,
		2400000.0,
		2110516.00000,
		1918395.00000,
		1794575.00000,
		1757319.00000,
		1721424.0,
		1721789.0,
		
		-31.0,
		-61.0,
		-92.0,
		
		-122.0,
		-153.0,
		-184.0,
		
		-214.0,
		-245.0,
		-275.0,
		
		-306.0,
		-334.0,
		-365.0,
		
		-699.0,
		-1064.0,
		-1420.0,
		
		-1785.0,
};

	QString t1s[] =
	{ 
		"-4712-01-01T12:00:00", 
		"-4713-12-31T12:00:00", 
		"2007-12-31T12:00:00",
		
		"0000-01-01T12:00:00", 
		"2132-08-31T12:00:00",
		"-4711-01-01T12:00:00", 
		
		"2008-03-08T12:00:00",
		"1582-10-15T12:00:00",
		"2008-01-01T00:00:00",
		
		"-0002-12-31T12:00:00",  
		"-0001-01-01T12:00:00",
		"1858-11-16T12:00:00",
	
		"1066-04-12T12:00:00",
		"0540-04-12T12:00:00",
		"0201-04-12T12:00:00",
	
		"0099-04-12T12:00:00",
		"0001-01-01T12:00:00", 
		"0002-01-01T12:00:00",
	
		"-4713-12-01T12:00:00", 
		"-4713-11-01T12:00:00", 
		"-4713-10-01T12:00:00", 
	
		"-4713-09-01T12:00:00", 
		"-4713-08-01T12:00:00", 
		"-4713-07-01T12:00:00", 
	
		"-4713-06-01T12:00:00", 
		"-4713-05-01T12:00:00", 
		"-4713-04-01T12:00:00", 
		
		"-4713-03-01T12:00:00", 
		"-4713-02-01T12:00:00", // 28 days
		"-4713-01-01T12:00:00", 
	
		"-4714-02-01T12:00:00", // 28 days
		"-4715-02-01T12:00:00", // 28 days
		"-4716-02-01T12:00:00", // 29 days
	
		"-4717-02-01T12:00:00", // 28 days
	};


	// does jdToIsoString() of item in t1d match string in t1s?
	QList<int> fromString;
	double jd;
	bool success;
	for (int i = 0; i < 31; i++)
	{
		_assert(QString::compare(StelUtils::jdToIsoString(t1d[i]),
		                         t1s[i]) == 0,
		        QString("%1").arg(i) + ": " +StelUtils::jdToIsoString(t1d[i]) + " ne " + t1s[i] + " (" + QDate::fromJulianDay((int)floor(t1d[i])).toString() + ")");

		fromString = StelUtils::getIntsFromISO8601String(t1s[i]);
		success = StelUtils::getJDFromDate( &jd, 
						    fromString[0],
						    fromString[1],
						    fromString[2],
						    fromString[3],
						    fromString[4],
						    fromString[5] );
		_assert(t1d[i] == jd && success,
			QString("%1").arg(i) + ": " + QString("failed with ") + t1s[i] + " " + 
			QString("%1").arg(jd, 10, 'f')  + " vs " + 
			QString("%1").arg(t1d[i], 10, 'f') );

	}
}


void formatting () 
{
	QLocale usEN;

	// test formatting of StelUtils::localeDateString, the fall-back if QDateTime cannot do it.

	QLocale::setDefault(QLocale::German);
	_assert(QString::compare(StelUtils::localeDateString(2008, 03, 10, 0),
	                         QString("10.03.08")) == 0,
	        "german for 2008-03-10 wrong: " + (StelUtils::localeDateString(2008, 03, 10, 0)));
	_assert(QString::compare(StelUtils::localeDateString(8, 03, 10, 0),
	                         QString("10.03.08")) == 0,
	        "german for 8-03-10 wrong: " + (StelUtils::localeDateString(8, 03, 10, 0)));
	_assert(QString::compare(StelUtils::localeDateString(-8, 03, 10, 0),
	                         QString("10.03.-8")) == 0,
	        "german for -8-03-10 wrong: " + (StelUtils::localeDateString(-8, 03, 10, 0)));
	_assert(QString::compare(StelUtils::localeDateString(-1118, 03, 10, 0),
	                         QString("10.03.-18")) == 0,
	        "german for -1118-03-10 wrong: " + (StelUtils::localeDateString(-1118, 03, 10, 0)));
	_assert(QString::compare(StelUtils::localeDateString(-5118, 03, 10, 0),
	                         QString("10.03.-18")) == 0,
	        "german for -5118-03-10 wrong: " + (StelUtils::localeDateString(-5118, 03, 10, 0)));

	_assert(-18 == (-5118 % 100), "modulus arithmetic works diff: " + QString("%1").arg(-5118 % 100));

	// test arbitrary fmt
	QLocale::setDefault(usEN);

	QString easyLong("d dd ddd dddd M MM MMM MMMM yy yyyy");
	_assert(QString::compare(QString("9 09 Sun Sunday 3 03 Mar March 08 2008"),
	                         StelUtils::localeDateString(2008, 3, 9, 6, easyLong)) == 0,
	        "formatter1 not working: " + StelUtils::localeDateString(2008, 3, 9, 6, easyLong));
	QString hardLong("dddddddd '''doh' ''yyyyyyy");
	_assert(QString::compare(QString("SundaySunday 'doh '200808y"),
	                         StelUtils::localeDateString(2008, 3, 9, 6, hardLong)) == 0,
	        "formatter2 not working: " + StelUtils::localeDateString(2008, 3, 9, 6, hardLong));

	// test detection of offset from UTC.

	double mar122008 = QDate(2008,3,12).toJulianDay();
	_assert(-4.0 == StelUtils::getGMTShiftFromQT(mar122008),
	        "gmt shift wrong: " + QString("%1").arg(StelUtils::getGMTShiftFromQT(mar122008)));
	double mar012008 = QDate(2008,3,1).toJulianDay();
	_assert(-5.0 == StelUtils::getGMTShiftFromQT(mar012008),
	        "gmt shift wrong: " + QString("%1").arg(StelUtils::getGMTShiftFromQT(mar012008)));


}

void qdateroundtrip() {

  QDate tst = QDate::currentDate();
  long jd = tst.toJulianDay();

  while ( tst.isValid() ) {
    jd --;
    tst = tst.addDays( -1 );

    QDate comp = QDate::fromJulianDay(jd);

    _assert( tst == comp, "failed at jd " + QString("%1").arg(jd) + " " + 
	     tst.toString() + " vs " + comp.toString() );
    if ( tst != comp ) {
      break;
    }
  }

}

void testrolloverandvalidity()
{

  _assertEquals(31, StelUtils::numberOfDaysInMonthInYear(1, 2008), "1a");
  _assertEquals(29, StelUtils::numberOfDaysInMonthInYear(2, 2008), "1b");
  _assertEquals(28, StelUtils::numberOfDaysInMonthInYear(2, 2007), "1c");

  _assertEquals(29, StelUtils::numberOfDaysInMonthInYear(2, 2000), "1d");
  _assertEquals(28, StelUtils::numberOfDaysInMonthInYear(2, 1900), "1e");

  _assertEquals(28, StelUtils::numberOfDaysInMonthInYear(2, 1001), "1f");
  _assertEquals(29, StelUtils::numberOfDaysInMonthInYear(2, 1000), "1g");
  _assertEquals(29, StelUtils::numberOfDaysInMonthInYear(2, 1200), "1h");

  _assertEquals(29, StelUtils::numberOfDaysInMonthInYear(2, 0), "1i");
  _assertEquals(28, StelUtils::numberOfDaysInMonthInYear(2, 1), "1j");
  _assertEquals(29, StelUtils::numberOfDaysInMonthInYear(2, -4), "1k");

  int dy, dm, dd, dh, dmin,ds;

  _assert( StelUtils::changeDateTimeForRollover(2008, 1, 32, 12, 0, 0,
				     &dy, &dm, &dd, &dh, &dmin, &ds), "1l");
  _assertEquals(2008, dy);
  _assertEquals(2, dm);
  _assertEquals(1, dd);
  _assertEquals(12, dh);
  _assertEquals(0, dmin);
  _assertEquals(0, ds);

  _assert( StelUtils::changeDateTimeForRollover(-1, 12, 32, 12, 0, 0,
				     &dy, &dm, &dd, &dh, &dmin, &ds), "1m");
  _assertEquals(0, dy);
  _assertEquals(1, dm);
  _assertEquals(1, dd);
  _assertEquals(12, dh);
  _assertEquals(0, dmin);
  _assertEquals(0, ds);

  _assert( StelUtils::changeDateTimeForRollover(-1, 11, 45, 12, 0, 0,
				     &dy, &dm, &dd, &dh, &dmin, &ds), "1n");
  _assertEquals(-1, dy);
  _assertEquals(12, dm);
  _assertEquals(15, dd);
  _assertEquals(12, dh);
  _assertEquals(0, dmin);
  _assertEquals(0, ds);

}

/************************************************************************
Run several of the time-related functions through paces.
 ************************************************************************/
int main(int argc, char* argv[])
{

  dateRoundTrip();
  formatting();
  testrolloverandvalidity();

}

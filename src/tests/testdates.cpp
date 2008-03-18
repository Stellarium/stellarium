#include <config.h>

#include <QString>
#include <QDateTime>
#include <QtGlobal>
#include <QLocale>
#include <QtDebug>
#include "StelUtils.hpp"

using namespace std;

/************************************************************************
expand this or use a real test framework someday.
 ************************************************************************/
void _assert(bool b, QString msg)
{
	if (! b)
	{
		qFatal("FAILED: %s",  msg.toUtf8().data());
	}
}

/************************************************************************
Run several of the time-related functions through paces.
 ************************************************************************/
int main(int argc, char* argv[])
{
	double t1d[] = { 1721424.00000,
	                 0.0,
	                 -1.0,

	                 2454466.0,
	                 1721424.0,
	                 1721058.0,

	                 2500000.0,
	                 366.0,
	                 2454534,

	                 2299161.0,
	                 2454466.5,
	                 1720692.0,
	                 1720693.0,

	               };
	QString t1s[] = { "0001-01-01T12:00:00",
	                  "-4712-01-01T12:00:00",
	                  "-4713-12-31T12:00:00",

	                  "2007-12-31T12:00:00",
	                  "0001-01-01T12:00:00",
	                  "0000-01-01T12:00:00",

	                  "2132-08-31T12:00:00",
	                  "-4711-01-01T12:00:00",
	                  "2008-03-08T12:00:00",

	                  "1582-10-15T12:00:00",
	                  "2008-01-01T00:00:00",
	                  "-0002-12-31T12:00:00",
	                  "-0001-01-01T12:00:00",
	                };

	// does jdToIsoString() of item in t1d match string in t1s?

	for (int i = 0; i < 12; i++)
	{
		_assert(QString::compare(StelUtils::jdToIsoString(t1d[i]),
		                         t1s[i]) == 0,
		        QString("%1").arg(i) + ": " +StelUtils::jdToIsoString(t1d[i]) + " ne " + t1s[i] + " (" + QDate::fromJulianDay((int)floor(t1d[i])).toString() + ")");
	}
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
	                         StelUtils::localeDateString(2008, 3, 9, 7, easyLong)),
	        "formatter not working: " + StelUtils::localeDateString(2008, 3, 9, 7, easyLong));
	QString hardLong("dddddddd '''doh' ''yyyyyyy");
	_assert(QString::compare(QString("SundaySunday 'doh '200808y"),
	                         StelUtils::localeDateString(2008, 3, 9, 7, hardLong)),
	        "formatter not working: " + StelUtils::localeDateString(2008, 3, 9, 7, hardLong));

	// test detection of offset from UTC.

	double mar122008 = QDate(2008,3,12).toJulianDay();
	_assert(-4.0 == StelUtils::get_GMT_shift_from_QT(mar122008),
	        "gmt shift wrong: " + QString("%1").arg(StelUtils::get_GMT_shift_from_QT(mar122008)));
	double mar012008 = QDate(2008,3,1).toJulianDay();
	_assert(-5.0 == StelUtils::get_GMT_shift_from_QT(mar012008),
	        "gmt shift wrong: " + QString("%1").arg(StelUtils::get_GMT_shift_from_QT(mar012008)));

}

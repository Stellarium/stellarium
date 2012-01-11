/*
 * Stellarium
 * Copyright (C) 2006 Fabien Chereau
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

#include "StelLocaleMgr.hpp"
#include "StelApp.hpp"
#include "StelUtils.hpp"
#include "StelFileMgr.hpp"

#include <QLocale>
#include <QDebug>
#include <QSettings>
#include <QString>
#include <QTextStream>

#include <QFile>

#include <ctime>

QMap<QString, QString> StelLocaleMgr::countryCodeToStringMap;

StelLocaleMgr::StelLocaleMgr() : skyTranslator("stellarium", StelFileMgr::getLocaleDir(), ""), GMTShift(0)
{
	//generateCountryList();

	// Load from file
	QString path;
	try
	{
		path = StelFileMgr::findFile("data/countryCodes.dat");
	}
	catch (std::runtime_error& e)
	{
		qWarning() << "ERROR - could not find country code data file." << e.what();
		return;
	}

	QFile file(path);
	file.open(QIODevice::ReadOnly);
	QDataStream in(&file);	// read the data serialized from the file
	in.setVersion(QDataStream::Qt_4_5);
	in >> countryCodeToStringMap;
	file.close();
}


StelLocaleMgr::~StelLocaleMgr()
{}

// Mehtod which generates and save the map between 2 letters country code and english country names
void StelLocaleMgr::generateCountryList()
{
	// Load ISO 3166-1 two-letter country codes from file (slow)
	// The format is "[code][tab][country name containing spaces][newline]"
	QFile textFile("data/iso3166-1-alpha-2.utf8");
	textFile.open(QFile::ReadOnly | QFile::Text);
	QTextStream list(&textFile);
	QString line;
	while(!(line = list.readLine()).isNull())
	{
		qDebug() << line.section(QChar('\t'), 0, 0) << ":" << line.section(QChar('\t'), 1, 1);
		countryCodeToStringMap.insert(line.section(QChar('\t'), 0, 0), line.section(QChar('\t'), 1, 1));
	}
	textFile.close();

	// Save to binary file
	QFile binaryFile("data/countryCodes.dat");
	binaryFile.open(QIODevice::WriteOnly);
	QDataStream out(&binaryFile);    // save the data serialized to the file
	out.setVersion(QDataStream::Qt_4_5);
	out << countryCodeToStringMap;
	binaryFile.close();
}

void StelLocaleMgr::init()
{
	QSettings* conf = StelApp::getInstance().getSettings();
	Q_ASSERT(conf);

	setSkyLanguage(conf->value("localization/sky_locale", "system").toString());
	setAppLanguage(conf->value("localization/app_locale", "system").toString());

	timeFormat = stringToSTimeFormat(conf->value("localization/time_display_format", "system_default").toString());
	dateFormat = stringToSDateFormat(conf->value("localization/date_display_format", "system_default").toString());
	// time_zone used to be in init_location section of config,
	// so use that as fallback when reading config - Rob
	QString tzstr = conf->value("localization/time_zone", conf->value("init_location/time_zone", "system_default").toString()).toString();
	if (tzstr == "system_default")
	{
		timeZoneMode = STzSystemDefault;
		// Set the program global intern timezones variables from the system locale
		tzset();
	}
	else
	{
		if (tzstr == "gmt+x") // TODO : handle GMT+X timezones form
		{
			timeZoneMode = STzGMTShift;
			// GMTShift = x;
		}
		else
		{
			// We have a custom time zone name
			timeZoneMode = STzCustom;
			setCustomTzName(tzstr);
		}
	}
}

/*************************************************************************
 Set the application locale. This apply to GUI, console messages etc..
*************************************************************************/
void StelLocaleMgr::setAppLanguage(const QString& newAppLanguageName)
{
	// Update the translator with new locale name
	StelTranslator::globalTranslator = StelTranslator("stellarium", StelFileMgr::getLocaleDir(), newAppLanguageName);
	qDebug() << "Application language is " << StelTranslator::globalTranslator.getTrueLocaleName();
	StelApp::getInstance().updateI18n();
}

/*************************************************************************
 Set the sky language.
*************************************************************************/
void StelLocaleMgr::setSkyLanguage(const QString& newSkyLanguageName)
{
	// Update the translator with new locale name
	skyTranslator = StelTranslator("stellarium-skycultures", StelFileMgr::getLocaleDir(), newSkyLanguageName);
	qDebug() << "Sky language is " << skyTranslator.getTrueLocaleName();
	StelApp::getInstance().updateI18n();
}

/*************************************************************************
 Get the language currently used for sky objects..
*************************************************************************/
QString StelLocaleMgr::getSkyLanguage() const
{
	return skyTranslator.getTrueLocaleName();
}

// Get the StelTranslator currently used for sky objects.
StelTranslator& StelLocaleMgr::getSkyTranslator()
{
	return skyTranslator;
}

StelTranslator& StelLocaleMgr::getAppStelTranslator() const
{
	return StelTranslator::globalTranslator;
}


// Return the time in ISO 8601 format that is : %Y-%m-%d %H:%M:%S
QString StelLocaleMgr::getISO8601TimeLocal(double JD) const
{
	double shift = 0.0;
	if (timeZoneMode == STzGMTShift)
	{
		shift = GMTShift;
	}
	else
	{
		shift = StelUtils::getGMTShiftFromQT(JD)*0.041666666666;
	}
	return StelUtils::julianDayToISO8601String(JD + shift);
}

//! get the six ints from an ISO8601 date time, understood to be local time, make a jdate out
//! of them.
double StelLocaleMgr::getJdFromISO8601TimeLocal(const QString& t, bool* ok) const
{
	double jd = StelUtils::getJulianDayFromISO8601String(t, ok);
	if (!*ok)
	{
		qWarning() << "StelLocaleMgr::getJdFromISO8601TimeLocal: invalid ISO8601 date. Returning JD=0";
		return 0.0;
	}
	
	// modified by shift
	if (timeZoneMode == STzGMTShift)
		jd -= GMTShift;
	else
		jd -= StelUtils::getGMTShiftFromQT(jd)*0.041666666666;
	return jd;
}


// Return a string with the local date formated according to the dateFormat variable
QString StelLocaleMgr::getPrintableDateLocal(double JD) const
{
	int year, month, day, dayOfWeek;
	double shift = 0.0;
	if (timeZoneMode == STzGMTShift)
	{
		shift = GMTShift;
	}
	else
	{
		shift = StelUtils::getGMTShiftFromQT(JD)*0.041666666666;
	}
	StelUtils::getDateFromJulianDay(JD+shift, &year, &month, &day);
	dayOfWeek = (int)floor(fmod(JD, 7));
	QString str;
	switch (dateFormat)
	{
		case SDateMMDDYYYY:
			str = QString("%1-%2-%3").arg(month,2,10,QLatin1Char('0')).arg(day,2,10,QLatin1Char('0')).arg(year,4,10);
			break;
		case SDateDDMMYYYY:
			str = QString("%1-%2-%3").arg(day,2,10,QLatin1Char('0')).arg(month,2,10,QLatin1Char('0')).arg(year,4,10);
			break;
		case SDateYYYYMMDD:
			str = QString("%1-%2-%3").arg(year,4,10).arg(month,2,10,QLatin1Char('0')).arg(day,2,10,QLatin1Char('0'));
			break;
		case SDateSystemDefault:
			str = StelUtils::localeDateString(year, month, day, dayOfWeek);
			break;
		default:
			qWarning() << "WARNING: unknown date format fallback to system default";
			str = StelUtils::localeDateString(year, month, day, dayOfWeek);
	}
	return str;
}

// Return a string with the local time (according to timeZoneMode variable) formated
// according to the timeFormat variable
QString StelLocaleMgr::getPrintableTimeLocal(double JD) const
{
	int hour, minute, second;
	double shift = 0.0;
	if (timeZoneMode == STzGMTShift)
	{
		shift = GMTShift;
	}
	else
	{
		shift = StelUtils::getGMTShiftFromQT(JD)*0.041666666666;
	}
	StelUtils::getTimeFromJulianDay(JD+shift, &hour, &minute, &second);

	QTime t(hour, minute, second);

	switch (timeFormat)
	{
		case STimeSystemDefault:
		return t.toString();
		case STime24h:
		return t.toString("hh:mm:ss");
		case STime12h:
		return t.toString("hh:mm:ss ap");
		default:
			qWarning() << "WARNING: unknown date format, fallback to system default";
		return t.toString(Qt::LocaleDate);
	}
}

// Convert the time format enum to its associated string and reverse
StelLocaleMgr::STimeFormat StelLocaleMgr::stringToSTimeFormat(const QString& tf) const
{
	if (tf == "system_default") return STimeSystemDefault;
	if (tf == "24h") return STime24h;
	if (tf == "12h") return STime12h;
	qWarning() << "WARNING: unrecognized time_display_format : " << tf << " system_default used.";
	return STimeSystemDefault;
}

QString StelLocaleMgr::sTimeFormatToString(STimeFormat tf) const
{
	if (tf == STimeSystemDefault) return "system_default";
	if (tf == STime24h) return "24h";
	if (tf == STime12h) return "12h";
	qWarning() << "WARNING: unrecognized time_display_format value : " << (int)tf << " system_default used.";
	return "system_default";
}

// Convert the date format enum to its associated string and reverse
StelLocaleMgr::SDateFormat StelLocaleMgr::stringToSDateFormat(const QString& df) const
{
	if (df == "system_default") return SDateSystemDefault;
	if (df == "mmddyyyy") return SDateMMDDYYYY;
	if (df == "ddmmyyyy") return SDateDDMMYYYY;
	if (df == "yyyymmdd") return SDateYYYYMMDD;  // iso8601
	qWarning() << "WARNING: unrecognized date_display_format : " << df << " system_default used.";
	return SDateSystemDefault;
}

QString StelLocaleMgr::sDateFormatToString(SDateFormat df) const
{
	if (df == SDateSystemDefault) return "system_default";
	if (df == SDateMMDDYYYY) return "mmddyyyy";
	if (df == SDateDDMMYYYY) return "ddmmyyyy";
	if (df == SDateYYYYMMDD) return "yyyymmdd";
	qWarning() << "WARNING: unrecognized date_display_format value : " << (int)df << " system_default used.";
	return "system_default";
}

void StelLocaleMgr::setCustomTzName(const QString& tzname)
{
	customTzName = tzname;
	timeZoneMode = STzCustom;

	if( customTzName != "")
	{
		// set the TZ environement variable and update c locale stuff
		putenv(strdup(qPrintable("TZ=" + customTzName)));
		tzset();
	}
}

float StelLocaleMgr::getGMTShift(double JD) const
{
	if (timeZoneMode == STzGMTShift) return GMTShift;
	else return StelUtils::getGMTShiftFromQT(JD);
}

// Convert a 2 letter country code to string
QString StelLocaleMgr::countryCodeToString(const QString& countryCode)
{
	QMap<QString, QString>::ConstIterator i = countryCodeToStringMap.find(countryCode);
	return (i!=countryCodeToStringMap.constEnd()) ? i.value() : QString();
}

// Return a list of all the known country names
QStringList StelLocaleMgr::getAllCountryNames()
{
	QStringList res;
	for (QMap<QString, QString>::ConstIterator i = countryCodeToStringMap.constBegin();i!=countryCodeToStringMap.constEnd();++i)
		res.append(i.value());
	res.sort();
	return res;
}

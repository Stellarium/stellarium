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

#include <QLocale>
#include <QDebug>
#include <QSettings>
#include <QString>
#include <QTextStream>
#include <QFile>

#include <ctime>

StelLocaleMgr::StelLocaleMgr()
	: skyTranslator(Q_NULLPTR)
	, planetaryFeaturesTranslator(Q_NULLPTR)
	, scriptsTranslator(Q_NULLPTR)
	, timeFormat()
	, dateFormat()
{
	core = StelApp::getInstance().getCore();
	createNameLists();
}


StelLocaleMgr::~StelLocaleMgr()
{
	delete skyTranslator;
	skyTranslator = Q_NULLPTR;
	delete planetaryFeaturesTranslator;
	planetaryFeaturesTranslator = Q_NULLPTR;
	delete scriptsTranslator;
	scriptsTranslator = Q_NULLPTR;
}

void StelLocaleMgr::init()
{
	QSettings* conf = StelApp::getInstance().getSettings();
	Q_ASSERT(conf);

	setSkyLanguage(conf->value("localization/sky_locale", "system").toString(), false);
	setAppLanguage(conf->value("localization/app_locale", "system").toString(), false);

	timeFormat = stringToSTimeFormat(conf->value("localization/time_display_format", "system_default").toString());
	dateFormat = stringToSDateFormat(conf->value("localization/date_display_format", "system_default").toString());
}

/*************************************************************************
 Set the application locale. This apply to GUI, console messages etc..
*************************************************************************/
void StelLocaleMgr::setAppLanguage(const QString& newAppLanguageName, bool refreshAll)
{
	// Update the translator with new locale name
	Q_ASSERT(StelTranslator::globalTranslator);
	delete StelTranslator::globalTranslator;
	StelTranslator::globalTranslator = new StelTranslator("stellarium", newAppLanguageName);
	qDebug().noquote() << "Application language:" << StelTranslator::globalTranslator->getTrueLocaleName();

	delete scriptsTranslator;
	// Update the translator with new locale name
	scriptsTranslator = new StelTranslator("stellarium-scripts", newAppLanguageName);
	qDebug().noquote() << "Scripts language:" << scriptsTranslator->getTrueLocaleName();

	createNameLists();
	if (refreshAll)
		StelApp::getInstance().updateI18n();
}

bool StelLocaleMgr::isAppRTL() const
{
	return QString("ar fa ckb ug ur he yi").contains(getAppLanguage());
}

/*************************************************************************
 Set the sky language.
*************************************************************************/
void StelLocaleMgr::setSkyLanguage(const QString& newSkyLanguageName, bool refreshAll)
{
	delete skyTranslator;
	// Update the translator with new locale name
	skyTranslator = new StelTranslator("stellarium-skycultures", newSkyLanguageName);
	qDebug().noquote() << "Sky language:" << skyTranslator->getTrueLocaleName();

	delete planetaryFeaturesTranslator;
	// Update the translator with new locale name
	planetaryFeaturesTranslator = new StelTranslator("stellarium-planetary-features", newSkyLanguageName);
	qDebug().noquote() << "Planetary features language:" << planetaryFeaturesTranslator->getTrueLocaleName();

	if (refreshAll)
		StelApp::getInstance().updateI18n();
}

/*************************************************************************
 Get the language currently used for sky objects..
*************************************************************************/
QString StelLocaleMgr::getSkyLanguage() const
{
	return skyTranslator->getTrueLocaleName();
}

bool StelLocaleMgr::isSkyRTL() const
{
	return QString("ar fa ckb ug ur he yi").contains(getSkyLanguage());
}

// Get the StelTranslator currently used for sky objects.
const StelTranslator& StelLocaleMgr::getSkyTranslator() const
{
	return *skyTranslator;
}

const StelTranslator& StelLocaleMgr::getPlanetaryFeaturesTranslator() const
{
	return *planetaryFeaturesTranslator;
}

const StelTranslator &StelLocaleMgr::getAppStelTranslator() const
{
	return *StelTranslator::globalTranslator;
}

const StelTranslator& StelLocaleMgr::getScriptsTranslator() const
{
	return *scriptsTranslator;
}

// Return the time in ISO 8601 format that is : %Y-%m-%d %H:%M:%S
QString StelLocaleMgr::getISO8601TimeLocal(double JD) const
{
	return StelUtils::julianDayToISO8601String(JD + core->getUTCOffset(JD)*StelCore::JD_HOUR);
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

	jd -= core->getUTCOffset(jd)*StelCore::JD_HOUR;
	return jd;
}


// Return a string with the local date formatted according to the dateFormat variable
QString StelLocaleMgr::getPrintableDateLocal(double JD) const
{
	int year, month, day, dayOfWeek;
	const double shift = core->getUTCOffset(JD)*StelCore::JD_HOUR;
	StelUtils::getDateFromJulianDay(JD+shift, &year, &month, &day);
	dayOfWeek = StelUtils::getDayOfWeek(year, month, day);
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
		case SDateWWMMDDYYYY:
			str = QString("%1, %2-%3-%4").arg(shortDayName(dayOfWeek)).arg(month,2,10,QLatin1Char('0')).arg(day,2,10,QLatin1Char('0')).arg(year,4,10);
			break;
		case SDateWWDDMMYYYY:
			str = QString("%1, %2-%3-%4").arg(shortDayName(dayOfWeek)).arg(day,2,10,QLatin1Char('0')).arg(month,2,10,QLatin1Char('0')).arg(year,4,10);
			break;
		case SDateWWYYYYMMDD:
			str = QString("%1, %2-%3-%4").arg(shortDayName(dayOfWeek)).arg(year,4,10).arg(month,2,10,QLatin1Char('0')).arg(day,2,10,QLatin1Char('0'));
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

// Return a string with the local time (according to timeZoneMode variable) formatted
// according to the timeFormat variable
QString StelLocaleMgr::getPrintableTimeLocal(double JD) const
{
	int hour, minute, second, millsec;
	const double shift = core->getUTCOffset(JD)*StelCore::JD_HOUR;
	StelUtils::getTimeFromJulianDay(JD+shift, &hour, &minute, &second, &millsec);
	QTime t(hour, minute, second, millsec);
	switch (timeFormat)
	{
		case STimeSystemDefault:
			return t.toString();
		case STime24h:
			return t.toString("hh:mm:ss");
		case STime12h:
			return t.toString("hh:mm:ss AP");
		default:
			qWarning() << "WARNING: unknown time format, fallback to system default";

#if (QT_VERSION>=QT_VERSION_CHECK(5,14,0))
			return t.toString(QLocale().dateFormat(QLocale::ShortFormat));
#else
			return t.toString(Qt::DefaultLocaleShortDate);
#endif
	}
}

QString StelLocaleMgr::getPrintableTimeZoneLocal(double JD) const
{
	QString timeZone = "";
	QString timeZoneST = "";
	if (core->getCurrentLocation().planetName=="Earth")
	{
		QString currTZ = core->getCurrentTimeZone();

		if (JD<=StelCore::TZ_ERA_BEGINNING || currTZ.contains("auto") || currTZ.contains("LMST"))
		{
			// TRANSLATORS: Local Mean Solar Time. Please use abbreviation.
			timeZoneST = qc_("LMST", "solar time");
		}

		if (currTZ.contains("LTST"))
		{
			// TRANSLATORS: Local True Solar Time. Please use abbreviation.
			timeZoneST = qc_("LTST", "solar time");
		}

		const double shift = core->getUTCOffset(JD);
		QTime tz = QTime(0, 0, 0).addSecs(static_cast<int>(3600*qAbs(shift)));
		if(shift<0.0)
			timeZone = QString("UTC-%1").arg(tz.toString("hh:mm"));
		else
			timeZone = QString("UTC+%1").arg(tz.toString("hh:mm"));

		if (!timeZoneST.isEmpty() && !core->getUseCustomTimeZone())
			timeZone = QString("%1 (%2)").arg(timeZone, timeZoneST);
	}
	else
	{
		// TODO: Make sure LMST/LTST would make sense on other planet, or inhibit it?
		const double shift = core->getUTCOffset(JD);
		QTime tz = QTime(0, 0, 0).addSecs(static_cast<int>(3600*qAbs(shift)));
		if(shift<0.0)
			timeZone = QString("UTC-%1").arg(tz.toString("hh:mm"));
		else
			timeZone = QString("UTC+%1").arg(tz.toString("hh:mm"));

		//if (!timeZoneST.isEmpty() && !core->getUseCustomTimeZone())
		//	timeZone = QString("%1 (%2)").arg(timeZone, timeZoneST);
	}
	return timeZone;
}

// Convert the time format enum to its associated string and reverse
StelLocaleMgr::STimeFormat StelLocaleMgr::stringToSTimeFormat(const QString& tf)
{
	static const QMap<QString, StelLocaleMgr::STimeFormat> map = {
		{"system_default", STimeSystemDefault},
		{"24h", STime24h},
		{"12h", STime12h}};
	if (!map.contains(tf))
		qWarning() << "WARNING: unrecognized time_display_format : " << tf << " system_default used.";
	return map.value(tf, STimeSystemDefault);
}

QString StelLocaleMgr::sTimeFormatToString(STimeFormat tf)
{
	static const QStringList tfmt={"system_default", "24h", "12h"};
	return tfmt[tf];
}

// Convert the date format enum to its associated string and reverse
StelLocaleMgr::SDateFormat StelLocaleMgr::stringToSDateFormat(const QString& df)
{
	static const QMap<QString, StelLocaleMgr::SDateFormat> map = {
		{"system_default", SDateSystemDefault},
		{"mmddyyyy",       SDateMMDDYYYY},
		{"ddmmyyyy",       SDateDDMMYYYY},
		{"yyyymmdd",       SDateYYYYMMDD}, // iso8601
		{"wwmmddyyyy",     SDateWWMMDDYYYY},
		{"wwddmmyyyy",     SDateWWDDMMYYYY},
		{"wwyyyymmdd",     SDateWWYYYYMMDD}};
	if (!map.contains(df))
		qWarning() << "WARNING: unrecognized date_display_format : " << df << " system_default used.";
	return map.value(df, SDateSystemDefault);
}

QString StelLocaleMgr::sDateFormatToString(SDateFormat df)
{
	QStringList dfmt = {
		"system_default",
		"mmddyyyy",
		"ddmmyyyy",
		"yyyymmdd",
		"wwddmmyyyy",
		"wwddmmyyyy",
		"wwyyyymmdd"};
	return dfmt[df];
}

QString StelLocaleMgr::getQtDateFormatStr() const
{
	QStringList dfmt = {
		"yyyy.MM.dd",
		"MM.dd.yyyy",
		"dd.MM.yyyy",
		"yyyy.MM.dd",
		"ddd, MM.dd.yyyy",
		"ddd, dd.MM.yyyy",
		"ddd, yyyy.MM.dd"};
	return dfmt[dateFormat];
}

QString StelLocaleMgr::shortDayName(int weekday)
{
	Q_ASSERT(weekday>=0);
	Q_ASSERT(shortWeekDays.length()==7);
	return shortWeekDays[weekday % 7];
}

QString StelLocaleMgr::longDayName(int weekday)
{
	Q_ASSERT(weekday>=0);
	Q_ASSERT(longWeekDays.length()==7);
	return longWeekDays[weekday % 7];
}

QString StelLocaleMgr::shortMonthName(int month)
{
	Q_ASSERT(month >= 0);
	Q_ASSERT(shortMonthNames.length()==12);
	return shortMonthNames[month % 12];
}

QString StelLocaleMgr::longMonthName(int month)
{
	Q_ASSERT(month >= 0);
	Q_ASSERT(longMonthNames.length()==12);
	return longMonthNames[month % 12];
}

QString StelLocaleMgr::longGenitiveMonthName(int month)
{
	Q_ASSERT(month >= 0);
	Q_ASSERT(longGenitiveMonthNames.length()==12);
	return longGenitiveMonthNames[month % 12];
}

QString StelLocaleMgr::romanMonthName(int month)
{
	Q_ASSERT(month >= 0);
	static const QStringList romanMonths = { "XII", "I", "II", "III", "IV", "V", "VI", "VII", "VIII", "IX", "X", "XI"};
	return romanMonths[month % 12];
}


void StelLocaleMgr::createNameLists()
{
	shortWeekDays.clear();
	shortWeekDays
		<< qc_("Sun", "short day name")
		<< qc_("Mon", "short day name")
		<< qc_("Tue", "short day name")
		<< qc_("Wed", "short day name")
		<< qc_("Thu", "short day name")
		<< qc_("Fri", "short day name")
		<< qc_("Sat", "short day name");
	longWeekDays.clear();
	longWeekDays
		<< qc_("Sunday",    "long day name")
		<< qc_("Monday",    "long day name")
		<< qc_("Tuesday",   "long day name")
		<< qc_("Wednesday", "long day name")
		<< qc_("Thursday",  "long day name")
		<< qc_("Friday",    "long day name")
		<< qc_("Saturday",  "long day name");

	shortMonthNames.clear();
	shortMonthNames
		<< qc_("Dec", "short month name")
		<< qc_("Jan", "short month name")
		<< qc_("Feb", "short month name")
		<< qc_("Mar", "short month name")
		<< qc_("Apr", "short month name")
		<< qc_("May", "short month name")
		<< qc_("Jun", "short month name")
		<< qc_("Jul", "short month name")
		<< qc_("Aug", "short month name")
		<< qc_("Sep", "short month name")
		<< qc_("Oct", "short month name")
		<< qc_("Nov", "short month name");

	longMonthNames.clear();
	longMonthNames
		<< qc_("December",  "long month name")
		<< qc_("January",   "long month name")
		<< qc_("February",  "long month name")
		<< qc_("March",     "long month name")
		<< qc_("April",     "long month name")
		<< qc_("May",       "long month name")
		<< qc_("June",      "long month name")
		<< qc_("July",      "long month name")
		<< qc_("August",    "long month name")
		<< qc_("September", "long month name")
		<< qc_("October",   "long month name")
		<< qc_("November",  "long month name");
	longGenitiveMonthNames.clear();
	longGenitiveMonthNames
		<< qc_("December",  "genitive")
		<< qc_("January",   "genitive")
		<< qc_("February",  "genitive")
		<< qc_("March",     "genitive")
		<< qc_("April",     "genitive")
		<< qc_("May",       "genitive")
		<< qc_("June",      "genitive")
		<< qc_("July",      "genitive")
		<< qc_("August",    "genitive")
		<< qc_("September", "genitive")
		<< qc_("October",   "genitive")
		<< qc_("November",  "genitive");
}

QStringList StelLocaleMgr::shortWeekDays;
QStringList StelLocaleMgr::longWeekDays;
QStringList StelLocaleMgr::shortMonthNames;
QStringList StelLocaleMgr::longMonthNames;
QStringList StelLocaleMgr::longGenitiveMonthNames;

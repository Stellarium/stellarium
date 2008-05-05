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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include <config.h>
#include "StelLocaleMgr.hpp"
#include "StelApp.hpp"
#include "StelUtils.hpp"
#include "StelFileMgr.hpp"
#include "TextEntryDateTimeValidator.hpp"

#include <QLocale>
#include <QDebug>
#include <QSettings>
#include <QString>

StelLocaleMgr::StelLocaleMgr() : skyTranslator(PACKAGE_NAME, INSTALL_LOCALEDIR, ""), GMT_shift(0)
{}


StelLocaleMgr::~StelLocaleMgr()
{}


void StelLocaleMgr::init()
{
	QSettings* conf = StelApp::getInstance().getSettings();
	assert(conf);

	setSkyLanguage(conf->value("localization/sky_locale", "system").toString());
	setAppLanguage(conf->value("localization/app_locale", "system").toString());
	
	time_format = string_to_s_time_format(conf->value("localization:time_display_format", "system_default").toString());
	date_format = string_to_s_date_format(conf->value("localization:date_display_format", "system_default").toString());
	// time_zone used to be in init_location section of config,
	// so use that as fallback when reading config - Rob
	QString tzstr = conf->value("localization/time_zone", conf->value("init_location/time_zone", "system_default").toString()).toString();
	if (tzstr == "system_default")
	{
		time_zone_mode = S_TZ_SYSTEM_DEFAULT;
		// Set the program global intern timezones variables from the system locale
		tzset();
	}
	else
	{
		if (tzstr == "gmt+x") // TODO : handle GMT+X timezones form
		{
			time_zone_mode = S_TZ_GMT_SHIFT;
			// GMT_shift = x;
		}
		else
		{
			// We have a custom time zone name
			time_zone_mode = S_TZ_CUSTOM;
			set_custom_tz_name(tzstr);
		}
	}
}

/*************************************************************************
 Set the application locale. This apply to GUI, console messages etc..
*************************************************************************/
void StelLocaleMgr::setAppLanguage(const QString& newAppLanguageName)
{
	// Update the translator with new locale name
	Translator::globalTranslator = Translator(PACKAGE_NAME, StelApp::getInstance().getFileMgr().getLocaleDir(), newAppLanguageName);
	qDebug() << "Application language is " << Translator::globalTranslator.getTrueLocaleName();
	StelApp::getInstance().updateI18n();
}

/*************************************************************************
 Set the sky language.
*************************************************************************/
void StelLocaleMgr::setSkyLanguage(const QString& newSkyLanguageName)
{
	// Update the translator with new locale name
	skyTranslator = Translator(PACKAGE_NAME, StelApp::getInstance().getFileMgr().getLocaleDir(), newSkyLanguageName);
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

// Get the Translator currently used for sky objects.
Translator& StelLocaleMgr::getSkyTranslator()
{
	return skyTranslator;
}


// Return the time in ISO 8601 format that is : %Y-%m-%d %H:%M:%S
QString StelLocaleMgr::get_ISO8601_time_local(double JD) const
{
	double shift = 0.0;
	if (time_zone_mode == S_TZ_GMT_SHIFT)
	{
		shift = GMT_shift;
	}
	else
	{
		shift = StelUtils::get_GMT_shift_from_QT(JD)*0.041666666666;
}
	return StelUtils::jdToIsoString(JD + shift);
}

//! get the six ints from an ISO8601 date time, understood to be local time, make a jdate out
//! of them.
double StelLocaleMgr::get_jd_from_ISO8601_time_local(const QString& t) const
{
	vector<int> numbers = TextEntryDateTimeValidator::get_ints_from_ISO8601_string(t);

	if (numbers.size() == 6)
{
		int y = numbers[0];
		int m = numbers[1];
		int d = numbers[2];
		int h = numbers[3];
		int mn = numbers[4];
		int s = numbers[5];

		double jd;

		if ( ! StelUtils::getJDFromDate(&jd, y, m, d, h, mn, s) )
		  {
		    qWarning() << "StelLocaleMgr::get_jd_from_ISO8601_time_local: StelUtils::getJDFromDate failed!  returning beginning of epoch!";
		    return 0.0;
		  }

		// modified by shift
	if (time_zone_mode == S_TZ_GMT_SHIFT)
			jd -= GMT_shift;
	else
			jd -= StelUtils::get_GMT_shift_from_QT(jd)*0.041666666666;
	
		return jd;
	
	}
	else
	{
		qWarning() << "StelLocaleMgr::get_jd_from_ISO8601_time_local: did not get 6 ints back from TextEntryDateTimeValidator for input " << t << ", returning beginning of epoch!";
		return 0.0;
	}
}
	

// Return a string with the local date formated according to the date_format variable
QString StelLocaleMgr::get_printable_date_local(double JD) const
	{
	int year, month, day, dayOfWeek;
	double shift = 0.0;
	if (time_zone_mode == S_TZ_GMT_SHIFT)
	{
		shift = GMT_shift;
	}
	else
	{
		shift = StelUtils::get_GMT_shift_from_QT(JD)*0.041666666666;
	}
	StelUtils::getDateFromJulianDay(JD+shift, &year, &month, &day);
	dayOfWeek = (int)floor(fmod(JD, 7));
	QString str;
	
	switch (date_format)
	{
		case S_DATE_MMDDYYYY:
		{
			str = QString("%1-%2-%3").arg(month,2,10,QLatin1Char('0')).arg(day,2,10,QLatin1Char('0')).arg(year,4,10);
		break;
		}
		case S_DATE_DDMMYYYY:
		{
			str = QString("%1-%2-%3").arg(day,2,10,QLatin1Char('0')).arg(month,2,10,QLatin1Char('0')).arg(year,4,10);
		break;
		}
		case S_DATE_YYYYMMDD:
		{
			str = QString("%1-%2-%3").arg(year,4,10).arg(month,2,10,QLatin1Char('0')).arg(day,2,10,QLatin1Char('0'));
		break;
		}
	case S_DATE_SYSTEM_DEFAULT:
		str = StelUtils::localeDateString(year, month, day, dayOfWeek);
		break;
		default:
			qWarning() << "WARNING: unknown date format fallback to system default";
		str = StelUtils::localeDateString(year, month, day, dayOfWeek);
	}
	return str;
}

// Return a string with the local time (according to time_zone_mode variable) formated
// according to the time_format variable
QString StelLocaleMgr::get_printable_time_local(double JD) const
{
	int hour, minute, second;
	double shift = 0.0;
	if (time_zone_mode == S_TZ_GMT_SHIFT)
	{
		shift = GMT_shift;
	}
	else
	{
		shift = StelUtils::get_GMT_shift_from_QT(JD)*0.041666666666;
	}
	StelUtils::getTimeFromJulianDay(JD+shift, &hour, &minute, &second);
	
	QTime t(hour, minute, second);

	switch (time_format)
	{
		case S_TIME_SYSTEM_DEFAULT:
		return t.toString();
		case S_TIME_24H:
		return t.toString("hh:mm:ss");
		case S_TIME_12H:
		return t.toString("hh:mm:ss ap");
		default:
			qWarning() << "WARNING: unknown date format, fallback to system default";
		return t.toString(Qt::LocaleDate);
	}
}

// Convert the time format enum to its associated string and reverse
StelLocaleMgr::S_TIME_FORMAT StelLocaleMgr::string_to_s_time_format(const QString& tf) const
{
	if (tf == "system_default") return S_TIME_SYSTEM_DEFAULT;
	if (tf == "24h") return S_TIME_24H;
	if (tf == "12h") return S_TIME_12H;
	qWarning() << "WARNING: unrecognized time_display_format : " << tf << " system_default used.";
	return S_TIME_SYSTEM_DEFAULT;
}

QString StelLocaleMgr::s_time_format_to_string(S_TIME_FORMAT tf) const
{
	if (tf == S_TIME_SYSTEM_DEFAULT) return "system_default";
	if (tf == S_TIME_24H) return "24h";
	if (tf == S_TIME_12H) return "12h";
	qWarning() << "WARNING: unrecognized time_display_format value : " << (int)tf << " system_default used.";
	return "system_default";
}

// Convert the date format enum to its associated string and reverse
StelLocaleMgr::S_DATE_FORMAT StelLocaleMgr::string_to_s_date_format(const QString& df) const
{
	if (df == "system_default") return S_DATE_SYSTEM_DEFAULT;
	if (df == "mmddyyyy") return S_DATE_MMDDYYYY;
	if (df == "ddmmyyyy") return S_DATE_DDMMYYYY;
	if (df == "yyyymmdd") return S_DATE_YYYYMMDD;  // iso8601
	qWarning() << "WARNING: unrecognized date_display_format : " << df << " system_default used.";
	return S_DATE_SYSTEM_DEFAULT;
}

QString StelLocaleMgr::s_date_format_to_string(S_DATE_FORMAT df) const
{
	if (df == S_DATE_SYSTEM_DEFAULT) return "system_default";
	if (df == S_DATE_MMDDYYYY) return "mmddyyyy";
	if (df == S_DATE_DDMMYYYY) return "ddmmyyyy";
	if (df == S_DATE_YYYYMMDD) return "yyyymmdd";
	qWarning() << "WARNING: unrecognized date_display_format value : " << (int)df << " system_default used.";
	return "system_default";
}

void StelLocaleMgr::set_custom_tz_name(const QString& tzname)
{
	custom_tz_name = tzname;
	time_zone_mode = S_TZ_CUSTOM;
	
	if( custom_tz_name != "")
	{
		// set the TZ environement variable and update c locale stuff
		putenv(strdup(qPrintable("TZ=" + custom_tz_name)));
		tzset();
	}
}

float StelLocaleMgr::get_GMT_shift(double JD) const
{
	if (time_zone_mode == S_TZ_GMT_SHIFT) return GMT_shift;
	else return StelUtils::get_GMT_shift_from_QT(JD);
}


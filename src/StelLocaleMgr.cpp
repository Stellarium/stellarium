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
#include "InitParser.hpp"
#include "StelFileMgr.hpp"

using namespace std;

StelLocaleMgr::StelLocaleMgr() : skyTranslator(PACKAGE_NAME, INSTALL_LOCALEDIR, ""), GMT_shift(0)
{}


StelLocaleMgr::~StelLocaleMgr()
{}


void StelLocaleMgr::init(const InitParser& conf)
{
	setSkyLanguage(conf.get_str("localization", "sky_locale", "system"));
	setAppLanguage(conf.get_str("localization", "app_locale", "system"));
	
	time_format = string_to_s_time_format(conf.get_str("localization:time_display_format"));
	date_format = string_to_s_date_format(conf.get_str("localization:date_display_format"));
	// time_zone used to be in init_location section of config,
	// so use that as fallback when reading config - Rob
	string tzstr = conf.get_str("localization", "time_zone", conf.get_str("init_location", "time_zone", "system_default"));
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
void StelLocaleMgr::setAppLanguage(const string& newAppLanguageName)
{
	// Update the translator with new locale name
	Translator::globalTranslator = Translator(PACKAGE_NAME, StelApp::getInstance().getFileMgr().getLocaleDir(), newAppLanguageName);
	cout << "Application language is " << Translator::globalTranslator.getTrueLocaleName() << endl;

	StelApp::getInstance().updateAppLanguage();
}

/*************************************************************************
 Set the sky language.
*************************************************************************/
void StelLocaleMgr::setSkyLanguage(const string& newSkyLanguageName)
{
	// Update the translator with new locale name
	skyTranslator = Translator(PACKAGE_NAME, StelApp::getInstance().getFileMgr().getLocaleDir(), newSkyLanguageName);
	cout << "Sky language is " << skyTranslator.getTrueLocaleName() << endl;
	
	StelApp::getInstance().updateSkyLanguage();
}

/*************************************************************************
 Get the language currently used for sky objects..
*************************************************************************/
string StelLocaleMgr::getSkyLanguage() const
{
	return skyTranslator.getTrueLocaleName();
}

// Get the Translator currently used for sky objects.
Translator& StelLocaleMgr::getSkyTranslator()
{
	return skyTranslator;
}

// Return a string with the UTC date formated according to the date_format variable
wstring StelLocaleMgr::get_printable_date_UTC(double JD) const
{
	QDateTime dateTime = StelUtils::jdToQDateTime(JD);
	switch(date_format)
	{
		case S_DATE_SYSTEM_DEFAULT:
			return dateTime.toString(Qt::LocaleDate).toStdWString();
		case S_DATE_MMDDYYYY:
			return dateTime.toString("MM.dd.yyyy").toStdWString();
		case S_DATE_DDMMYYYY:
			return dateTime.toString("dd.MM.yyyy").toStdWString();
		case S_DATE_YYYYMMDD:
			return dateTime.toString("yyyy.MM.dd").toStdWString();
		default:
			return dateTime.toString(Qt::LocaleDate).toStdWString();
	}
}

// Return a string with the UTC time formated according to the time_format variable
wstring StelLocaleMgr::get_printable_time_UTC(double JD) const
{
	QDateTime dateTime = StelUtils::jdToQDateTime(JD);
	switch(time_format)
	{
		case S_TIME_SYSTEM_DEFAULT:
			return dateTime.time().toString(Qt::LocaleDate).toStdWString();
		case S_TIME_24H:
			return dateTime.time().toString("HH:mm:ss").toStdWString();
		case S_TIME_12H:
			return dateTime.time().toString("HH:mm:ss ap").toStdWString();
		default:
			return dateTime.time().toString(Qt::LocaleDate).toStdWString();
	}
}

// Return the time in ISO 8601 format that is : %Y-%m-%d %H:%M:%S
string StelLocaleMgr::get_ISO8601_time_local(double JD) const
{
	QDateTime dateTime;
	if (time_zone_mode == S_TZ_GMT_SHIFT)
		dateTime = StelUtils::jdToQDateTime(JD + GMT_shift);
	else
		dateTime = StelUtils::jdToQDateTime(JD + get_GMT_shift_from_system(JD)*0.041666666666);
	return dateTime.toLocalTime().toString(Qt::ISODate).toStdString();
}


// Return a string with the local date formated according to the date_format variable
wstring StelLocaleMgr::get_printable_date_local(double JD) const
{
	QDateTime dateTime;
	if (time_zone_mode == S_TZ_GMT_SHIFT)
		dateTime = StelUtils::jdToQDateTime(JD + GMT_shift);
	else
		dateTime = StelUtils::jdToQDateTime(JD + get_GMT_shift_from_system(JD)*0.041666666666);
	dateTime = dateTime.toLocalTime();
	
	switch (date_format)
	{
		case S_DATE_SYSTEM_DEFAULT:
			return dateTime.date().toString(Qt::LocaleDate).toStdWString();
		case S_DATE_MMDDYYYY:
			return dateTime.date().toString("MM.dd.yyyy").toStdWString();
		case S_DATE_DDMMYYYY:
			return dateTime.date().toString("dd.MM.yyyy").toStdWString();
		case S_DATE_YYYYMMDD:
			return dateTime.date().toString("yyyy.MM.dd").toStdWString();
		default:
			return dateTime.date().toString(Qt::LocaleDate).toStdWString();
	}
}

// Return a string with the local time (according to time_zone_mode variable) formated
// according to the time_format variable
wstring StelLocaleMgr::get_printable_time_local(double JD) const
{
	QDateTime dateTime;
	if (time_zone_mode == S_TZ_GMT_SHIFT)
		dateTime = StelUtils::jdToQDateTime(JD + GMT_shift);
	else
		dateTime = StelUtils::jdToQDateTime(JD + get_GMT_shift_from_system(JD)*0.041666666666);
	dateTime = dateTime.toLocalTime();
	
	switch (time_format)
	{
		case S_TIME_SYSTEM_DEFAULT:
			return dateTime.time().toString(Qt::LocaleDate).toStdWString();
		case S_TIME_24H:
			return dateTime.time().toString("HH:mm:ss").toStdWString();
		case S_TIME_12H:
			return dateTime.time().toString("HH:mm:ss ap").toStdWString();
		default:
			return dateTime.time().toString(Qt::LocaleDate).toStdWString();
	}
}

// Convert the time format enum to its associated string and reverse
StelLocaleMgr::S_TIME_FORMAT StelLocaleMgr::string_to_s_time_format(const string& tf) const
{
	if (tf == "system_default") return S_TIME_SYSTEM_DEFAULT;
	if (tf == "24h") return S_TIME_24H;
	if (tf == "12h") return S_TIME_12H;
	cerr << "WARNING: unrecognized time_display_format : " << tf << " system_default used." << endl;
	return S_TIME_SYSTEM_DEFAULT;
}

string StelLocaleMgr::s_time_format_to_string(S_TIME_FORMAT tf) const
{
	if (tf == S_TIME_SYSTEM_DEFAULT) return "system_default";
	if (tf == S_TIME_24H) return "24h";
	if (tf == S_TIME_12H) return "12h";
	cerr << "WARNING: unrecognized time_display_format value : " << tf << " system_default used." << endl;
	return "system_default";
}

// Convert the date format enum to its associated string and reverse
StelLocaleMgr::S_DATE_FORMAT StelLocaleMgr::string_to_s_date_format(const string& df) const
{
	if (df == "system_default") return S_DATE_SYSTEM_DEFAULT;
	if (df == "mmddyyyy") return S_DATE_MMDDYYYY;
	if (df == "ddmmyyyy") return S_DATE_DDMMYYYY;
	if (df == "yyyymmdd") return S_DATE_YYYYMMDD;  // iso8601
	cerr << "WARNING: unrecognized date_display_format : " << df << " system_default used." << endl;
	return S_DATE_SYSTEM_DEFAULT;
}

string StelLocaleMgr::s_date_format_to_string(S_DATE_FORMAT df) const
{
	if (df == S_DATE_SYSTEM_DEFAULT) return "system_default";
	if (df == S_DATE_MMDDYYYY) return "mmddyyyy";
	if (df == S_DATE_DDMMYYYY) return "ddmmyyyy";
	if (df == S_DATE_YYYYMMDD) return "yyyymmdd";
	cerr << "WARNING: unrecognized date_display_format value : " << df << " system_default used." << endl;
	return "system_default";
}

void StelLocaleMgr::set_custom_tz_name(const string& tzname)
{
	custom_tz_name = tzname;
	time_zone_mode = S_TZ_CUSTOM;
	
	if( custom_tz_name != "")
	{
		// set the TZ environement variable and update c locale stuff
		putenv(strdup((string("TZ=") + custom_tz_name).c_str()));
		tzset();
	}
}

float StelLocaleMgr::get_GMT_shift(double JD) const
{
	if (time_zone_mode == S_TZ_GMT_SHIFT) return GMT_shift;
	else return get_GMT_shift_from_system(JD);
}

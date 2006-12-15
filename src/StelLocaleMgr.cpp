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

using namespace std;

#include "StelLocaleMgr.h"
#include "stellarium.h"
#include "stelapp.h"
#include "stel_utility.h"
#include "init_parser.h"

StelLocaleMgr::StelLocaleMgr() : skyTranslator(APP_NAME, LOCALEDIR, ""), GMT_shift(0)
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
	Translator::globalTranslator = Translator(PACKAGE, StelApp::getInstance().getLocaleDir(), newAppLanguageName);
	cout << "Application language is " << Translator::globalTranslator.getTrueLocaleName() << endl;

	StelApp::getInstance().updateAppLanguage();
}

/*************************************************************************
 Set the sky language.
*************************************************************************/
void StelLocaleMgr::setSkyLanguage(const string& newSkyLanguageName)
{
	// Update the translator with new locale name
	skyTranslator = Translator(PACKAGE, StelApp::getInstance().getLocaleDir(), newSkyLanguageName);
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
	struct tm time_utc;
	get_tm_from_julian(JD, &time_utc);
	
	static char date[255];
	switch(date_format)
	{
	case S_DATE_SYSTEM_DEFAULT : StelUtils::my_strftime(date, 254, "%x", &time_utc); break;
	case S_DATE_MMDDYYYY : StelUtils::my_strftime(date, 254, "%m/%d/%Y", &time_utc); break;
	case S_DATE_DDMMYYYY : StelUtils::my_strftime(date, 254, "%d/%m/%Y", &time_utc); break;
	case S_DATE_YYYYMMDD : StelUtils::my_strftime(date, 254, "%Y-%m-%d", &time_utc); break;
	}
	return StelUtils::stringToWstring(date);
}

// Return a string with the UTC time formated according to the time_format variable
// TODO : for some locales (french) the %p returns nothing
wstring StelLocaleMgr::get_printable_time_UTC(double JD) const
{
	struct tm time_utc;
	get_tm_from_julian(JD, &time_utc);
	
	static char heure[255];
	switch(time_format)
	{
	case S_TIME_SYSTEM_DEFAULT : StelUtils::my_strftime(heure, 254, "%X", &time_utc); break;
	case S_TIME_24H : StelUtils::my_strftime(heure, 254, "%H:%M:%S", &time_utc); break;
	case S_TIME_12H : StelUtils::my_strftime(heure, 254, "%I:%M:%S %p", &time_utc); break;
	}
	return StelUtils::stringToWstring(heure);
}

// Return the time in ISO 8601 format that is : %Y-%m-%d %H:%M:%S
string StelLocaleMgr::get_ISO8601_time_local(double JD) const
{
	struct tm time_local;
	if (time_zone_mode == S_TZ_GMT_SHIFT)
		get_tm_from_julian(JD + GMT_shift, &time_local);
	else
		get_tm_from_julian(JD + get_GMT_shift_from_system(JD)*0.041666666666, &time_local);
	
	static char isotime[255];
StelUtils::my_strftime(isotime, 254, "%Y-%m-%d %H:%M:%S", &time_local);
	return isotime;
}


// Return a string with the local date formated according to the date_format variable
wstring StelLocaleMgr::get_printable_date_local(double JD) const
{
	struct tm time_local;
	
	if (time_zone_mode == S_TZ_GMT_SHIFT)
		get_tm_from_julian(JD + GMT_shift, &time_local);
	else
		get_tm_from_julian(JD + get_GMT_shift_from_system(JD)*0.041666666666, &time_local);
	
	static char date[255];
	switch(date_format)
	{
	case S_DATE_SYSTEM_DEFAULT : StelUtils::my_strftime(date, 254, "%x", &time_local); break;
	case S_DATE_MMDDYYYY : StelUtils::my_strftime(date, 254, "%m/%d/%Y", &time_local); break;
	case S_DATE_DDMMYYYY : StelUtils::my_strftime(date, 254, "%d/%m/%Y", &time_local); break;
	case S_DATE_YYYYMMDD : StelUtils::my_strftime(date, 254, "%Y-%m-%d", &time_local); break;
	}
	
	return StelUtils::stringToWstring(date);
}

// Return a string with the local time (according to time_zone_mode variable) formated
// according to the time_format variable
wstring StelLocaleMgr::get_printable_time_local(double JD) const
{
	struct tm time_local;
	
	if (time_zone_mode == S_TZ_GMT_SHIFT)
		get_tm_from_julian(JD + GMT_shift, &time_local);
	else
		get_tm_from_julian(JD + get_GMT_shift_from_system(JD)*0.041666666666, &time_local);
	
	static char heure[255];
	switch(time_format)
	{
	case S_TIME_SYSTEM_DEFAULT : StelUtils::my_strftime(heure, 254, "%X", &time_local); break;
	case S_TIME_24H : StelUtils::my_strftime(heure, 254, "%H:%M:%S", &time_local); break;
	case S_TIME_12H : StelUtils::my_strftime(heure, 254, "%I:%M:%S %p", &time_local); break;
	}
	return StelUtils::stringToWstring(heure);
}

// Convert the time format enum to its associated string and reverse
StelLocaleMgr::S_TIME_FORMAT StelLocaleMgr::string_to_s_time_format(const string& tf) const
{
	if (tf == "system_default") return S_TIME_SYSTEM_DEFAULT;
	if (tf == "24h") return S_TIME_24H;
	if (tf == "12h") return S_TIME_12H;
cout << "ERROR : unrecognized time_display_format : " << tf << " system_default used." << endl;
	return S_TIME_SYSTEM_DEFAULT;
}

string StelLocaleMgr::s_time_format_to_string(S_TIME_FORMAT tf) const
{
	if (tf == S_TIME_SYSTEM_DEFAULT) return "system_default";
	if (tf == S_TIME_24H) return "24h";
	if (tf == S_TIME_12H) return "12h";
cout << "ERROR : unrecognized time_display_format value : " << tf << " system_default used." << endl;
	return "system_default";
}

// Convert the date format enum to its associated string and reverse
StelLocaleMgr::S_DATE_FORMAT StelLocaleMgr::string_to_s_date_format(const string& df) const
{
	if (df == "system_default") return S_DATE_SYSTEM_DEFAULT;
	if (df == "mmddyyyy") return S_DATE_MMDDYYYY;
	if (df == "ddmmyyyy") return S_DATE_DDMMYYYY;
	if (df == "yyyymmdd") return S_DATE_YYYYMMDD;  // iso8601
cout << "ERROR : unrecognized date_display_format : " << df << " system_default used." << endl;
	return S_DATE_SYSTEM_DEFAULT;
}

string StelLocaleMgr::s_date_format_to_string(S_DATE_FORMAT df) const
{
	if (df == S_DATE_SYSTEM_DEFAULT) return "system_default";
	if (df == S_DATE_MMDDYYYY) return "mmddyyyy";
	if (df == S_DATE_DDMMYYYY) return "ddmmyyyy";
	if (df == S_DATE_YYYYMMDD) return "yyyymmdd";
	cout << "ERROR : unrecognized date_display_format value : " << df << " system_default used." << endl;
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

float StelLocaleMgr::get_GMT_shift(double JD, bool _local) const
{
	if (time_zone_mode == S_TZ_GMT_SHIFT) return GMT_shift;
	else return get_GMT_shift_from_system(JD,_local);
}

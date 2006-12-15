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

#ifndef STELLOCALEMGR_H
#define STELLOCALEMGR_H

#include <string>
#include "translator.h"

using namespace std;

// Predeclaration of some classes
class InitParser;

//!	Manage i18n operation such as messages translations and date/time localization.
//! @author Fabien Chereau
class StelLocaleMgr{
public:
    StelLocaleMgr();

    ~StelLocaleMgr();

	//! @brief Init itself from a config file.
	void init(const InitParser& conf);
	
	///////////////////////////////////////////////////////////////////////////
	// MESSAGES TRANSLATIONS
	///////////////////////////////////////////////////////////////////////////
	//! @brief Get the application language currently used for GUI etc..
	//! This function has no permanent effect on the global locale.
	//! @return the name of the language (e.g fr).
	string getAppLanguage() const { return Translator::globalTranslator.getTrueLocaleName(); }
	
	//! @brief Set the application language. This applies to GUI etc..
	//! This function has no permanent effect on the global locale.
	//! @param newAppLocaleName the name of the language (e.g fr).
	void setAppLanguage(const string& newAppLangName);			   
	
	//! @brief Get the Translator currently used for global application.
	Translator& getAppTranslator() const;
	
	//! @brief Get the language currently used for sky objects.
	//! This function has no permanent effect on the global locale.
	//! @return the name of the language (e.g fr).
	string getSkyLanguage() const;		   
	
	//! @brief Set the sky language and reload the sky objects names with the new translation.
	//! This function has no permanent effect on the global locale
	//!@param newSkyLangName The name of the locale (e.g fr) to use for sky object labels
	void setSkyLanguage(const string& newSkyLangName);
	
	//! @brief Get the Translator currently used for sky objects.
	Translator& getSkyTranslator();
	
	///////////////////////////////////////////////////////////////////////////
	// DATE & TIME LOCALIZATION
	///////////////////////////////////////////////////////////////////////////
	string get_time_format_str(void) const {return s_time_format_to_string(time_format);}
	void set_time_format_str(const string& tf) {time_format=string_to_s_time_format(tf);}
	string get_date_format_str(void) const {return s_date_format_to_string(date_format);}
	void set_date_format_str(const string& df) {date_format=string_to_s_date_format(df);}
	
	void setCustomTimezone(string _time_zone) { set_custom_tz_name(_time_zone); }

	
	enum S_TIME_FORMAT {S_TIME_24H,	S_TIME_12H,	S_TIME_SYSTEM_DEFAULT};
	enum S_DATE_FORMAT {S_DATE_MMDDYYYY, S_DATE_DDMMYYYY, S_DATE_SYSTEM_DEFAULT, S_DATE_YYYYMMDD};
	
	wstring get_printable_date_UTC(double JD) const;
	wstring get_printable_date_local(double JD) const;
	wstring get_printable_time_UTC(double JD) const;
	wstring get_printable_time_local(double JD) const;
	
	enum S_TZ_FORMAT
	{
		S_TZ_CUSTOM,
		S_TZ_GMT_SHIFT,
		S_TZ_SYSTEM_DEFAULT
	};
	
	//! Return the current time shift at observator time zone with respect to GMT time
	void set_GMT_shift(int t) {GMT_shift=t;}
	float get_GMT_shift(double JD = 0, bool _local=0) const;
	void set_custom_tz_name(const string& tzname);
	string get_custom_tz_name(void) const {return custom_tz_name;}
	S_TZ_FORMAT get_tz_format(void) const {return time_zone_mode;}
	
	// Return the time in ISO 8601 format that is : %Y-%m-%d %H:%M:%S
	string get_ISO8601_time_local(double JD) const;
private:
	// The translator used for astronomical object naming
	Translator skyTranslator;
	
	// Date and time variables
	S_TIME_FORMAT time_format;
	S_DATE_FORMAT date_format;
	S_TZ_FORMAT time_zone_mode;		// Can be the system default or a user defined value
	
	string custom_tz_name;			// Something like "Europe/Paris"
	float GMT_shift;				// Time shift between GMT time and local time in hour. (positive for Est of GMT)
	
	// Convert the time format enum to its associated string and reverse
	S_TIME_FORMAT string_to_s_time_format(const string&) const;
	string s_time_format_to_string(S_TIME_FORMAT) const;
	
	// Convert the date format enum to its associated string and reverse
	S_DATE_FORMAT string_to_s_date_format(const string& df) const;
	string s_date_format_to_string(S_DATE_FORMAT df) const;
};

#endif

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
#include "Translator.hpp"

using namespace std;

// Predeclaration of some classes
class InitParser;

//! @class StelLocaleMgr
//! Manage i18n operations such as message translation and date/time localization.
//! @author Fabien Chereau
class StelLocaleMgr{
public:
	StelLocaleMgr();
	~StelLocaleMgr();

	//! Initialize object. This process includes:
	//! - Setting the sky and application languages
	//! - Setting the time and date formats
	//! - Setting up the time zone
	void init();
	
	///////////////////////////////////////////////////////////////////////////
	// MESSAGES TRANSLATIONS
	///////////////////////////////////////////////////////////////////////////
	//! Get the application language currently used for GUI etc.
	//! This function has no permanent effect on the global locale.
	//! @return the abbreviated name of the language (e.g "fr").
	QString getAppLanguage() const { return Translator::globalTranslator.getTrueLocaleName(); }
	
	//! Set the application language. 
	//! This applies to GUI etc. This function has no permanent effect on the 
	//! global locale.
	//! @param newAppLocaleName the abbreviated name of the language (e.g fr).
	void setAppLanguage(const QString& newAppLangName);
	
	//! Get the Translator object currently used for global application.
	Translator& getAppTranslator() const;
	
	//! Get the language currently used for sky objects.
	//! This function has no permanent effect on the global locale.
	//! @return the name of the language (e.g fr).
	QString getSkyLanguage() const;
	
	//! Set the sky language and reload the sky object names with the new 
	//! translation.  This function has no permanent effect on the global locale.
	//! @param newSkyLangName The abbreviated name of the locale (e.g fr) to use 
	//! for sky object labels.
	void setSkyLanguage(const QString& newSkyLangName);
	
	//! Get a reference to the Translator object currently used for sky objects.
	Translator& getSkyTranslator();
	
	///////////////////////////////////////////////////////////////////////////
	// DATE & TIME LOCALIZATION
	///////////////////////////////////////////////////////////////////////////
	//! Get the format string which describes the current time format.
	//! Valid values are:
	//! - "system_default"
	//! - "24h"
	//! - "12h"
	//!
	//! These values correspond to the similarly named values in the S_TIME_FORMAT enum.
	string get_time_format_str(void) const {return s_time_format_to_string(time_format);}
	//! Set the time format from a format string.
	//! @tf value values are the same as the return values for get_time_format_str().
	void set_time_format_str(const QString& tf) {time_format=string_to_s_time_format(tf);}
	//! Get the format string which describes the current date format.
	//! Valid values:
	//! - "mmddyyyy"
	//! - "ddmmyyyy"
	//! - "system_default"
	//! - "yyyymmdd"
	//!
	//! These values correspond to the similarly named values in the S_DATE_FORMAT enum.
	string get_date_format_str(void) const {return s_date_format_to_string(date_format);}
	void set_date_format_str(const QString& df) {date_format=string_to_s_date_format(df);}
	
	//! Set the time zone.
	//! @param _time_zone the time zone string as parsed from the TZ environment 
	//! varibale by the tzset function from libc.
	void setCustomTimezone(QString _time_zone) { set_custom_tz_name(_time_zone); }

	//! @enum S_TIME_FORMAT
	//! The time display format.
	enum S_TIME_FORMAT {
		S_TIME_24H,		//!< 24 hour clock, e.g. "18:22:00"
		S_TIME_12H,		//!< 12 hour clock, e.g. "06:22:00 pm"
  		S_TIME_SYSTEM_DEFAULT	//!< use the system default format.
	};
	
	//! @enum S_DATE_FORMAT
	//! The date display format.
	enum S_DATE_FORMAT {
		S_DATE_MMDDYYYY,	//!< e.g. "07-05-1998" for July 5th 1998
		S_DATE_DDMMYYYY,	//!< e.g. "05-07-1998" for July 5th 1998
		S_DATE_SYSTEM_DEFAULT,	//!< Use the system default date format
		S_DATE_YYYYMMDD		//!< e.g. "1998-07-05" for July 5th 1998
	};
	
	//! Get a localized, formatted string representation of the date component of a Julian date.
	wstring get_printable_date_local(double JD) const;
	
	//! Get a localized, formatted string representation of the time component of a Julian date.
	wstring get_printable_time_local(double JD) const;
	
	//! @enum S_TZ_FORMAT
	enum S_TZ_FORMAT
	{
		S_TZ_CUSTOM,		//!< User-specified timezone.
		S_TZ_GMT_SHIFT,		//!< GMT + offset.
		S_TZ_SYSTEM_DEFAULT	//!< System default.
	};
	
	//! Get the current time shift at observator time zone with respect to GMT time.
	void set_GMT_shift(int t) {GMT_shift=t;}
	//! Get the current time shift at observator time zone with respect to GMT time.
	float get_GMT_shift(double JD = 0) const;
	//! Set the timezone by a TZ-style string (see tzset in the libc manual).
	void set_custom_tz_name(const QString& tzname);
	//! Get the timezone name (a TZ-style string - see tzset in the libc manual).
	QString get_custom_tz_name(void) const {return custom_tz_name;}
	//! Get the current timezone format mode.
	S_TZ_FORMAT get_tz_format(void) const {return time_zone_mode;}
	
	//! Return the time in ISO 8601 format that is : %Y-%m-%d %H:%M:%S
	//! @param JD the time and date expressed as a Julian date value.
	QString get_ISO8601_time_local(double JD) const;
private:
	// The translator used for astronomical object naming
	Translator skyTranslator;
	
	// Date and time variables
	S_TIME_FORMAT time_format;
	S_DATE_FORMAT date_format;
	S_TZ_FORMAT time_zone_mode;		// Can be the system default or a user defined value
	
	QString custom_tz_name;			// Something like "Europe/Paris"
	float GMT_shift;				// Time shift between GMT time and local time in hour. (positive for Est of GMT)
	
	// Convert the time format enum to its associated string and reverse
	S_TIME_FORMAT string_to_s_time_format(const QString&) const;
	string s_time_format_to_string(S_TIME_FORMAT) const;
	
	// Convert the date format enum to its associated string and reverse
	S_DATE_FORMAT string_to_s_date_format(const QString& df) const;
	string s_date_format_to_string(S_DATE_FORMAT df) const;
};

#endif

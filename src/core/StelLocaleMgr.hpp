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

#ifndef _STELLOCALEMGR_HPP_
#define _STELLOCALEMGR_HPP_

#include "StelTranslator.hpp"

//! @class StelLocaleMgr
//! Manage i18n operations such as message translation and date/time localization.
//! @author Fabien Chereau
class StelLocaleMgr
{
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
	QString getAppLanguage() const { return StelTranslator::globalTranslator.getTrueLocaleName(); }
	
	//! Set the application language. 
	//! This applies to GUI etc. This function has no permanent effect on the global locale.
	//! @param newAppLangName the abbreviated name of the language (e.g fr).
	void setAppLanguage(const QString& newAppLangName);
	
	//! Get the StelTranslator object currently used for global application.
	StelTranslator& getAppStelTranslator() const;
	
	//! Get the language currently used for sky objects.
	//! This function has no permanent effect on the global locale.
	//! @return the name of the language (e.g fr).
	QString getSkyLanguage() const;
	
	//! Set the sky language and reload the sky object names with the new 
	//! translation.  This function has no permanent effect on the global locale.
	//! @param newSkyLangName The abbreviated name of the locale (e.g fr) to use 
	//! for sky object labels.
	void setSkyLanguage(const QString& newSkyLangName);
	
	//! Get a reference to the StelTranslator object currently used for sky objects.
	StelTranslator& getSkyTranslator();
	
	///////////////////////////////////////////////////////////////////////////
	// DATE & TIME LOCALIZATION
	///////////////////////////////////////////////////////////////////////////
	//! Get the format string which describes the current time format.
	//! Valid values are:
	//! - "system_default"
	//! - "24h"
	//! - "12h"
	//!
	//! These values correspond to the similarly named values in the STimeFormat enum.
	QString getTimeFormatStr(void) const {return sTimeFormatToString(timeFormat);}
	//! Set the time format from a format string.
	//! @param tf values are the same as the return values for getTimeFormatStr().
	void setTimeFormatStr(const QString& tf) {timeFormat=stringToSTimeFormat(tf);}
	//! Get the format string which describes the current date format.
	//! Valid values:
	//! - "mmddyyyy"
	//! - "ddmmyyyy"
	//! - "system_default"
	//! - "yyyymmdd"
	//!
	//! These values correspond to the similarly named values in the SDateFormat enum.
	QString getDateFormatStr(void) const {return sDateFormatToString(dateFormat);}
	void setDateFormatStr(const QString& df) {dateFormat=stringToSDateFormat(df);}
	
	//! Set the time zone.
	//! @param tZ the time zone string as parsed from the TZ environment 
	//! varibale by the tzset function from libc.
	void setCustomTimezone(QString tZ) { setCustomTzName(tZ); }

	//! @enum STimeFormat
	//! The time display format.
	enum STimeFormat {
		STime24h,		//!< 24 hour clock, e.g. "18:22:00"
		STime12h,		//!< 12 hour clock, e.g. "06:22:00 pm"
  		STimeSystemDefault	//!< use the system default format.
	};
	
	//! @enum SDateFormat
	//! The date display format.
	enum SDateFormat {
		SDateMMDDYYYY,	//!< e.g. "07-05-1998" for July 5th 1998
		SDateDDMMYYYY,	//!< e.g. "05-07-1998" for July 5th 1998
		SDateSystemDefault,	//!< Use the system default date format
		SDateYYYYMMDD		//!< e.g. "1998-07-05" for July 5th 1998
	};
	
	//! Get a localized, formatted string representation of the date component of a Julian date.
	QString getPrintableDateLocal(double JD) const;
	
	//! Get a localized, formatted string representation of the time component of a Julian date.
	QString getPrintableTimeLocal(double JD) const;
	
	//! @enum STzFormat
	enum STzFormat
	{
		STzCustom,		//!< User-specified timezone.
		STzGMTShift,		//!< GMT + offset.
		STzSystemDefault	//!< System default.
	};
	
	//! Get the current time shift at observator time zone with respect to GMT time.
	void setGMTShift(int t)
	{
		GMTShift=t;
	}
	//! Get the current time shift in hours at observator time zone with respect to GMT time.
	float getGMTShift(double JD = 0) const;
	//! Set the timezone by a TZ-style string (see tzset in the libc manual).
	void setCustomTzName(const QString& tzname);
	//! Get the timezone name (a TZ-style string - see tzset in the libc manual).
	QString getCustomTzName(void) const
	{
		return customTzName;
	}
	//! Get the current timezone format mode.
	STzFormat getTzFormat(void) const
	{
		return timeZoneMode;
	}
	
	//! Return the time in ISO 8601 format that is : %Y-%m-%dT%H:%M:%S
	//! @param JD the time and date expressed as a Julian date value.
	QString getISO8601TimeLocal(double JD) const;

	//! Return the JD time for a local time ISO 8601 format that is:
	//! %Y-%m-%dT%H:%M:%S, but %Y can be a large number with sign, and
	//! %Y can be zero.
	//! @param str the local time in ISO 8601 format.
	//! @param ok set to false if the string was an invalid date.
	double getJdFromISO8601TimeLocal(const QString& str, bool* ok) const;

	//! Convert a 2 letter country code to string
	static QString countryCodeToString(const QString& countryCode);
	
	//! Return an alphabetically ordered list of all the known country names
	static QStringList getAllCountryNames();
	
private:
	// The translator used for astronomical object naming
	StelTranslator skyTranslator;
	
	// Date and time variables
	STimeFormat timeFormat;
	SDateFormat dateFormat;
	STzFormat timeZoneMode;		// Can be the system default or a user defined value
	
	QString customTzName;			// Something like "Europe/Paris"
	float GMTShift;				// Time shift between GMT time and local time in hour. (positive for Est of GMT)
	
	// Convert the time format enum to its associated string and reverse
	STimeFormat stringToSTimeFormat(const QString&) const;
	QString sTimeFormatToString(STimeFormat) const;
	
	// Convert the date format enum to its associated string and reverse
	SDateFormat stringToSDateFormat(const QString& df) const;
	QString sDateFormatToString(SDateFormat df) const;
	
	static QMap<QString, QString> countryCodeToStringMap;
	
	static void generateCountryList();
};

#endif // _STELLOCALEMGR_HPP_

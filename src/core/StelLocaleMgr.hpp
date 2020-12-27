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

#ifndef STELLOCALEMGR_HPP
#define STELLOCALEMGR_HPP

#include "StelTranslator.hpp"
#include "StelCore.hpp"

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
	QString getAppLanguage() const { return StelTranslator::globalTranslator->getTrueLocaleName(); }
	
	//! Set the application language. 
	//! This applies to GUI etc. This function has no permanent effect on the global locale.
	//! @param newAppLangName the abbreviated name of the language (e.g fr).
	void setAppLanguage(const QString& newAppLangName, bool refreshAll=true);
	
	//! Get the StelTranslator object currently used for global application.
	const StelTranslator& getAppStelTranslator() const;

	//! Get the type (RTL or LTR) of application language currently used for GUI etc.
	bool isAppRTL() const;
	
	//! Get the language currently used for sky objects.
	//! This function has no permanent effect on the global locale.
	//! @return the name of the language (e.g fr).
	QString getSkyLanguage() const;
	
	//! Set the sky language and reload the sky object names with the new 
	//! translation.  This function has no permanent effect on the global locale.
	//! @param newSkyLangName The abbreviated name of the locale (e.g fr) to use 
	//! for sky object labels.
	void setSkyLanguage(const QString& newSkyLangName, bool refreshAll=true);
	
	//! Get a reference to the StelTranslator object currently used for sky objects.
	const StelTranslator &getSkyTranslator() const;

	//! Get a reference to the StelTranslator object currently used for planetary features.
	const StelTranslator &getPlanetaryFeaturesTranslator() const;

	//! Get a reference to the StelTranslator object currently used for scripts.
	const StelTranslator &getScriptsTranslator() const;

	//! Get the type (RTL or LTR) of language currently used for sky objects
	bool isSkyRTL() const;
	
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

	//! Get the format string which describes the current date format (Qt style).
	QString getQtDateFormatStr(void) const;
	
	//! @enum STimeFormat
	//! The time display format.
	enum STimeFormat {
		STimeSystemDefault,	//!< use the system default format.
		STime24h,		//!< 24 hour clock, e.g. "18:22:00"
		STime12h		//!< 12 hour clock, e.g. "06:22:00 pm"
	};
	
	//! @enum SDateFormat
	//! The date display format.
	enum SDateFormat {
		SDateSystemDefault,	//!< Use the system default date format
		SDateMMDDYYYY,		//!< e.g. "07-05-1998" for July 5th 1998
		SDateDDMMYYYY,		//!< e.g. "05-07-1998" for July 5th 1998
		SDateYYYYMMDD,		//!< e.g. "1998-07-05" for July 5th 1998
		SDateWWMMDDYYYY,	//!< e.g. "Sun, 07-05-1998" for Sunday, July 5th 1998
		SDateWWDDMMYYYY,	//!< e.g. "Sun, 05-07-1998" for Sunday, July 5th 1998
		SDateWWYYYYMMDD		//!< e.g. "Sun, 1998-07-05" for Sunday, July 5th 1998
	};
	
	//! Get a localized, formatted string representation of the date component of a Julian date.
	QString getPrintableDateLocal(double JD) const;
	
	//! Get a localized, formatted string representation of the time component of a Julian date.
	QString getPrintableTimeLocal(double JD) const;

	//! Get a localized, formatted string representation of the time zone of a Julian date.
	QString getPrintableTimeZoneLocal(double JD) const;

	//! Return the time in ISO 8601 format that is : %Y-%m-%dT%H:%M:%S
	//! @param JD the time and date expressed as a Julian date value.
	QString getISO8601TimeLocal(double JD) const;

	//! Return the JD time for a local time ISO 8601 format that is:
	//! %Y-%m-%dT%H:%M:%S, but %Y can be a large number with sign, and
	//! %Y can be zero.
	//! @param str the local time in ISO 8601 format.
	//! @param ok set to false if the string was an invalid date.
	double getJdFromISO8601TimeLocal(const QString& str, bool* ok) const;

	//! Convert a 2 letter country code to string. Returns empty string if countryCode unknown.
	static QString countryCodeToString(const QString& countryCode);
	//! Convert a countryName to 2 letter country code. Returns "??" if not found.
	static QString countryNameToCode(const QString& countryName);

	//! Return an alphabetically ordered list of all the known country names
	static QStringList getAllCountryNames();

	//! Returns the short name of the \a weekday [0=Sunday, 1=Monday..6]; weekday mod 7)
	static QString shortDayName(int weekday);

	//! Returns the long name of the \a weekday ([0..6]; weekday mod 7)
	static QString longDayName(int weekday);

	//! Returns the short name of the \a month [1..12]
	static QString shortMonthName(int month);

	//! Returns the long name of the \a month [1..12]
	static QString longMonthName(int month);

	//! Returns the genitive long name of the \a month [1..12]
	static QString longGenitiveMonthName(int month);

	//! Returns the Roman name (a number) of the \a month [1..12]
	static QString romanMonthName(int month);
	
private:
	//! fill the class-inherent lists with translated names for weekdays, month names etc. in the current language.
	//! Call this at program start and then after each language change.
	static void createNameLists();
	// The translator used for astronomical object naming
	StelTranslator* skyTranslator;
	StelTranslator* planetaryFeaturesTranslator;
	StelTranslator* scriptsTranslator;
	StelCore* core;
	
	// Date and time variables
	STimeFormat timeFormat;
	SDateFormat dateFormat;

	// Convert the time format enum to its associated string and reverse
	static STimeFormat stringToSTimeFormat(const QString&);
	static QString sTimeFormatToString(STimeFormat);
	
	// Convert the date format enum to its associated string and reverse
	static SDateFormat stringToSDateFormat(const QString& df);
	static QString sDateFormatToString(SDateFormat df);
	
	static QMap<QString, QString> countryCodeToStringMap;
	
	// Lists for rapid access. These must be recreated on language change by createNameLists().
	static QStringList shortWeekDays;
	static QStringList longWeekDays;
	static QStringList shortMonthNames;
	static QStringList longMonthNames;
	static QStringList longGenitiveMonthNames;

	static void generateCountryList();
};

#endif // STELLOCALEMGR_HPP

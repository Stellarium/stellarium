/*
* Stellarium
* Copyright (C) 2005 Fabien Chereau
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

#ifndef STELTRANSLATOR_HPP
#define STELTRANSLATOR_HPP

//! @file StelTranslator.hpp
//! Define some translation macros.

#include <QMap>
#include <QString>

//! @def q_(str)
//! Return the gettext translated english text @a str using the current global translator.
//! The returned value is a localized QString.
#define q_(str) StelTranslator::globalTranslator->qtranslate(str)

//! @def qc_(str, ctxt)
//! Return the gettext translated english text @a str in context @b ctxt using the current global translator.
//! The returned value is a localized QString.
#define qc_(str, ctxt) StelTranslator::globalTranslator->qtranslate(str, ctxt)

//! @def N_(str)
//! A pseudo function call that serves as a marker for the automated extraction of messages.
//! A call to N_() doesn't translate.
#define N_(str) str

//! @def NC_(str, ctxt)
//! A pseudo function call that serves as a marker for the automated extraction of messages in some context.
//! A call to NC_() doesn't translate.
#define NC_(str, ctxt) str

//! Class used to translate strings to any language.
//! Implements a nice interface to gettext which is UTF-8 compliant and is somewhat multiplateform
//! All its operations do not modify the global locale.
//! The purpose of this class is to remove all non-OO C locale functions from stellarium.
//! @author Fabien Chereau
class StelTranslator
{
public:
	//! Create a translator from a language name.
	//! If the passed locale name cannot be handled by the system, default value will be used.
	//! The passed language name is a language code string like "fr" or "fr_FR".
	//! This class wrap gettext to simulate an object oriented multiplateform gettext UTF8 translator
	//! @param adomain The name of the domain to use for translation
	//! @param alangName The C locale name or language name like "fr" or "fr_FR". If string is "" or "system" it will use the system locale.
	StelTranslator(const QString& adomain, const QString& alangName);
	
	virtual ~StelTranslator();

	//! Checks whether any translations have been loaded
	virtual bool isEmpty() const;
	
	//! Translate input message and return it as a QString.
	//! If the string is not translated in the current locale, the input string is returned unchanged.
	//! @param string input string in english.
	//! @param context disambiguation string (gettext "context" string).
	//! @return The translated QString
	QString qtranslate(const QString& string, const QString& context = QString()) const;

	//! Same as #qtranslate, but with additional code to handle Chinese names of stars
	QString qTranslateStar(const QString& string, const QString& context = QString()) const;

	//! Same as #qTranslateStar, but specifically for the \p pronounce entries
	QString qTranslateStarPronounce(const QString& string, const QString& context = QString()) const;

	//! Try to translate input message and return it as a QString. If no translation
	//! exist for the current StelTranslator language, a null string is returned.
	//! @param string input string in english.
	//! @param context disambiguation string (gettext "context" string).
	//! @return The translated QString
	virtual QString tryQtranslate(const QString& string, const QString& context = QString()) const;

	//! Same as #tryQtranslate, but with additional code to handle Chinese names of stars
	QString tryQtranslateStar(const QString& string, const QString& context = QString()) const;

	//! Same as #tryQtranslateStar, but specifically for the \p pronounce entries
	QString tryQtranslateStarPronounce(const QString& string, const QString& context = QString()) const;

	//! Get true translator locale name. Actual locale, never "system".
	//! @return Locale name e.g "fr_FR"
	const QString& getTrueLocaleName() const
	{
		if (langName=="system" || langName=="system_default")
			return StelTranslator::systemLangName;
		else
			return langName;
	}

	//! Used as a global translator by the whole app
	static StelTranslator* globalTranslator;

	//! Get available language name in native language from passed locales directory
	QStringList getAvailableLanguagesNamesNative(const QString& localeDir="", const QString &section="") const;

	//! Convert from ISO639-1 language code to native language name
	//! @param languageCode the code to look up
	static QString iso639_1CodeToNativeName(const QString& languageCode);
	
	//! Convert from native language name to ISO639-1 2 letters language code 
	static QString nativeNameToIso639_1Code(const QString& languageName);
	
	//! Initialize Translation
	//! @param fileName file containing the list of language codes
	static void init(const QString& fileName);
	
protected:
	StelTranslator() = default;

	//! The domain name
	QString domain;

	//! The two letter string defining the current language name (ISO 639)
	QString langName;

	//! QTranslator instance
	class QTranslator* translator = nullptr;

private:
	StelTranslator(const StelTranslator& );
	const StelTranslator& operator=(const StelTranslator&);

	QString tryTranslateChineseStar(const QString& string, const QString& context) const;
	QString tryTranslateChineseStarPronounce(const QString& string, const QString& context) const;
	
	//! Initialize the languages code list from the passed file
	//! @param fileName file containing the list of language codes
	static void initIso639_1LanguageCodes(const QString& fileName);
	
	//! Get available language codes from passed locales directory
	static QStringList getAvailableIso639_1Codes(const QString& localeDir="");

	//! Try to determine system language from system configuration
	static void initSystemLanguage(void);
	
	//! Store the system default language name as taken from LANGUAGE environment variable
	static QString systemLangName;
	
	//! Contains the list of all iso639 languages codes
	static QMap<QString, QString> iso639codes;
};

class StelSkyTranslator : public StelTranslator
{
public:
	StelSkyTranslator(const QString& langName);
	QString tryQtranslate(const QString& string, const QString& context = QString()) const override;
	bool isEmpty() const override;
private:
	//! Used as a skyculture-independent fallback when current sky culture doesn't have a translation
	StelTranslator commonSkyTranslator;
};

#endif // STELTRANSLATOR_HPP


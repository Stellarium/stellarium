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
* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#ifndef TRANSLATOR_H
#define TRANSLATOR_H

#include <config.h>

#include <string>
#include <locale>
#include <iostream>
#include <cerrno>

/**
 * Class used to translate strings to any language.
 * Implements a nice interface to gettext thru use of C++ std::messages class.
 * All its operations do not modify the global locale. 
 * The purpose of this class is to remove all non-OO C locale functions from stellarium.
 * It could be extended for all locale management using e.g the portable IBM ICU library.
 * @author Fabien Chereau
 */

// These macro are used as global function replacing standard gettext operation
#ifndef MACOSX
#include "gettext.h"
#define _(String) Translator::globalTranslator.translate( gettext_noop(String) ).c_str()
#define N_(String) gettext_noop(String)
#else
# include "POSupport.h"
# define _(String) localizedUTF8String(String)
# define N_(String) (String)
#endif

using namespace std;

class Translator{
public:

	/**
	 * @brief Create a translator from a locale or language name.
     * If the passed locale name cannot be handled by the system, close values will be tried
     * until one works. E.g if the passed string is "fr", the method will try "fr", then "fr_FR", then "fr_FR.utf8" etc..
     * If nothing works the system default locale will be used. The locale supported by the system can usually be listed 
     * with the command "locale -a" on unix.
     * @param domain The name of the domain to use for translation
	 * @param moDirectory The directory where to look for the domain.mo translation files.
     * @param localeName The C locale name or language name. If string is "" or "system" it will use the system locale.
     */
     Translator(const std::string& _domain, const std::string& _moDirectory, const std::string& _langName) :
     	domain(_domain), moDirectory(_moDirectory), langName(_langName)
     {
	 }
	 
	/**
	 * @brief Translate input message.
	 * @param s input string in english.
	 * @return The translated string in UTF-8 characters.
	 */
	 std::string translateUTF8(const std::string& s);
	 
	/**
	 * @brief Translate input message.
	 * @param s input string in english.
	 * @return The translated string in wide characters.
	 */
	 std::wstring translate(const std::string& s)
	 {
	 	return UTF8stringToWstring(translateUTF8(s));
	 }	 
	 
	 /**
	  * @brief Get translator locale name. This name can be used to create a translator.
	  * @return Locale name e.g "fr_FR"
	  */
	 std::string getLocaleName(void)
	 {
	 	return langName;
	 }
	 
	 /** Used as a global translator by the whole app */
	 static Translator globalTranslator;
	 
private:
	//! @brief Create a locale from locale name.
	//! If the locale cannot be created, try different names until one work. If none work fall back to "C" locale.
	//! @param localeName Locale name e.g fr_FR, fr_FR.utf8, french etc..
	//! @return std::locale matching with the locale name
	//static std::locale tryLocale(const string& localeName);	
	
	/** Reload the current locale info so that gettext use them */
	void reload();
	
	/** The domain name */
	string domain;
	
	/** The directory where the locale file tree stands */
	string moDirectory;
	
	/** The two letter string defining the current language name */
	string langName;
	
	/** Keep in memory which one was the last used transator to prevent reloading it at each tranlate() call */
	static Translator* lastUsed;
	
	/** Convert from UTF-8 to wchar_t */
	std::wstring UTF8stringToWstring(const string& s);
};

#endif

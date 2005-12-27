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

#include <string>
#include <locale>
#include <iostream>
#include "iconv.h"

/**
 * Class used to translate strings to any language.
 * Implements a nice interface to gettext thru use of C++ std::messages class.
 * All its operations do not modify the global locale. 
 * The purpose of this class is to remove all non-OO C locale functions from stellarium.
 * @author Fabien Chereau
 */

using namespace std;

class Translator{
public:
	/**
	 * @brief Create a Translator with passed std::locale
	 * @param domain The name of the domain to use for translation
	 * @param moDirectory The directory where to look for the domain.mo translation files.
	 * @param loc The locale to use.
	 */
    Translator(const std::string& domain, const std::string& moDirectory, const std::locale& _loc) : loc(_loc)
    {
		init(domain, moDirectory);
	}

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
     Translator(const std::string& domain, const std::string& moDirectory, const std::string& localeName)
     {
     	loc = std::locale(localeName.c_str());
     	init(domain, moDirectory);
	 }
	
    /** Destructor */
    ~Translator()
    {
    	iconv_close(iconvDesc);
    }
	 
	/**
	 * @brief Translate input message.
	 * @param s input string in english.
	 * @return The translated string in UTF-8 characters.
	 */
	 std::string translateUTF8(const std::string& s)
	 {
	 	return msg->get(cat, 0, 0, s).c_str();
	 }
	 
	/**
	 * @brief Translate input message.
	 * @param s input string in english.
	 * @return The translated string in wide characters.
	 */
	 std::wstring translate(const std::string& s)
	 {
	 	return UTF8stringToWstring(translateUTF8(s));
	 }	 
	 
private:
	
	/** The locale internally used */
	std::locale loc;
	
	/** The messages facet, it is pointing on an object internal to loc and therefore desn't need to be de-allocated */
	const std::messages<char>* msg;
	
	/** The iconv conversion descriptor from UTF-8 to wchar_t */
	iconv_t iconvDesc;
	
	/** The catalog to use */
	std::messages_base::catalog cat;
	
	/** init everything  : call global bind_textdomain_codeset function.. */
	void init(const std::string& domain, const std::string& moDirectory);
	
	/** Convert from UTF-8 to wchar_t */
	std::wstring UTF8stringToWstring(const string& s)
	{
		size_t inbytesleft = s.length();
		char* inbuf = new char[inbytesleft];
		char* inbufsav = inbuf;
		strcpy(inbuf, s.c_str());
		// In the maximum case there is one wchar_t per char (e.g for an ASCII string)
		size_t outbytesleft = (inbytesleft)*sizeof(wchar_t);
		wchar_t* outbuf = new wchar_t[outbytesleft];
		wchar_t* outbufsav = outbuf;
		size_t n = iconv(iconvDesc, &inbuf, &inbytesleft, (char**)&outbuf, &outbytesleft);
		assert(n != (size_t) (-1));
		std::wstring ws(outbufsav);
		delete[]inbufsav;
		delete[]outbufsav;
		return ws;
	}
};

#endif

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

#include <config.h>
#include "translator.h"

// Use system locale language by default
Translator Translator::globalTranslator = Translator(PACKAGE, LOCALEDIR, "");

void Translator::init(const std::string& domain, const std::string& moDirectory)
{
		// The stellarium domain uses UTF-8 codeset to allow for locale independant coding.
		// There is the possibility to use WCHAR_T type to directly obtain wchar_t type strings
		// But it causes troubles when the string isn't defined : in this case gettext returns char* instead of wchar_t!!
		// Fortunately UTF-8 is ASCII compatible therefore no translation is also valid UTF-8.
		std::string result = bind_textdomain_codeset(domain.c_str(), "UTF-8");
		assert(result=="UTF-8");
		
		// Create the message catalog and open it
		msg = &(std::use_facet<std::messages<char> >(loc));
		cat = msg->open(domain.c_str(), loc, moDirectory.c_str());
}

/** Convert from ASCII to wchar_t */
// static wchar_t *ASCII_to_UNICODE(wchar_t *unicode, const char *text, int len)
// {
// 	int i;
// 
// 	for ( i=0; i < len; ++i ) {
// 		unicode[i] = ((const unsigned char *)text)[i];
// 	}
// 	unicode[i] = 0;
// 
// 	return unicode;
// }

static wchar_t *UTF8_to_UNICODE(wchar_t *unicode, const char *utf8, int len)
{
	int i, j;
	wchar_t ch;

	for ( i=0, j=0; i < len; ++i, ++j ) {
		ch = ((const unsigned char *)utf8)[i];
		if ( ch >= 0xF0 ) {
			ch  =  (wchar_t)(utf8[i]&0x07) << 18;
			ch |=  (wchar_t)(utf8[++i]&0x3F) << 12;
			ch |=  (wchar_t)(utf8[++i]&0x3F) << 6;
			ch |=  (wchar_t)(utf8[++i]&0x3F);
		} else
		if ( ch >= 0xE0 ) {
			ch  =  (wchar_t)(utf8[i]&0x3F) << 12;
			ch |=  (wchar_t)(utf8[++i]&0x3F) << 6;
			ch |=  (wchar_t)(utf8[++i]&0x3F);
		} else
		if ( ch >= 0xC0 ) {
			ch  =  (wchar_t)(utf8[i]&0x3F) << 6;
			ch |=  (wchar_t)(utf8[++i]&0x3F);
		}
		unicode[j] = ch;
	}
	unicode[j] = 0;

	return unicode;
}

/** Convert from UTF-8 to wchar_t */
std::wstring Translator::UTF8stringToWstring(const string& s)
{
	wchar_t* outbuf = new wchar_t[s.length()+1];
	UTF8_to_UNICODE(outbuf, s.c_str(), s.length());
	wstring ws(outbuf);
	delete[] outbuf;
	return ws;
}


//! @brief Create a locale matching with a locale name
std::locale Translator::tryLocale(const string& localeName)
{
	std::locale loc;
	if (localeName=="system_default" || localeName=="system")
	{
		return std::locale("");
	}
	else
	{
		try
		{
			loc = std::locale(localeName.c_str());
		}
		catch (const std::exception& e)
		{
			cout << e.what() << "\"" << localeName << "\" : revert to default locale \"" << std::locale("").name() << "\"" << endl;
			// Fallback with current locale
			loc = std::locale();
		}
	}
	return loc;
}

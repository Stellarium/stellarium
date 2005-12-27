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

#include "translator.h"

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
		
		// Create the UTF-8 -> WCHAR_T convertor with iconv
		iconvDesc = iconv_open("WCHAR_T", "UTF-8");
		assert(iconvDesc!=(iconv_t)(-1));
}

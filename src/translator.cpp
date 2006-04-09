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
#include <cassert>
#include <dirent.h>
#include <cstdio>

#include "bytes.h"
#include "translator.h"

Translator* Translator::lastUsed = NULL;

#ifndef MINGW32
// This crashes the application on windows..
const string Translator::systemLangName = (getenv("LANGUAGE")!=NULL) ? getenv("LANGUAGE") : getenv("LANG");
#else
const string Translator::systemLangName = "en";
#endif

// Use system locale language by default
Translator Translator::globalTranslator = Translator(PACKAGE, LOCALEDIR, "system");

void Translator::reload()
{
	if (Translator::lastUsed == this) return;
	// This needs to be static as it is used a each gettext call... It tooks me quite a while before I got that :(
	static char envstr[25];
	if (langName=="system" || langName=="system_default")
	{
		snprintf(envstr, 25, "LANGUAGE=%s", Translator::systemLangName.c_str());
	}
	else
	{
		snprintf(envstr, 25, "LANGUAGE=%s", langName.c_str());
	}
	//printf("Setting locale: %s\n", envstr);
	putenv(envstr);
	setlocale(LC_MESSAGES, "");

	std::string result = bind_textdomain_codeset(domain.c_str(), "UTF-8");
	assert(result=="UTF-8");
	bindtextdomain (domain.c_str(), moDirectory.c_str());
	textdomain (domain.c_str());
	Translator::lastUsed = this;
}

//! Convert from ASCII to wchar_t
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

//! Convert from char* UTF-8 to wchar_t UCS4 - stolen from SDL_ttf library
static wchar_t *UTF8_to_UNICODE(wchar_t *unicode, const char *utf8, int len)
{
	int i, j;
	unsigned short ch;  // 16 bits

	for ( i=0, j=0; i < len; ++i, ++j )
	{
		ch = ((const unsigned char *)utf8)[i];
		if ( ch >= 0xF0 )
		{
			ch  =  (unsigned short)(utf8[i]&0x07) << 18;
			ch |=  (unsigned short)(utf8[++i]&0x3F) << 12;
			ch |=  (unsigned short)(utf8[++i]&0x3F) << 6;
			ch |=  (unsigned short)(utf8[++i]&0x3F);
		}
		else
			if ( ch >= 0xE0 )
			{
				ch  =  (unsigned short)(utf8[i]&0x3F) << 12;
				ch |=  (unsigned short)(utf8[++i]&0x3F) << 6;
				ch |=  (unsigned short)(utf8[++i]&0x3F);
			}
			else
				if ( ch >= 0xC0 )
				{
					ch  =  (unsigned short)(utf8[i]&0x3F) << 6;
					ch |=  (unsigned short)(utf8[++i]&0x3F);
				}

#ifdef WORDS_BIGENDIAN
		unicode[j] = bswap_16(ch);
#else
		unicode[j] = ch;
#endif

	}
	unicode[j] = 0;

	return unicode;
}

//! Convert from UTF-8 to wchar_t
//! Warning this is likely to be not very portable
std::wstring Translator::UTF8stringToWstring(const string& s)
{
	wchar_t* outbuf = new wchar_t[s.length()+1];
	UTF8_to_UNICODE(outbuf, s.c_str(), s.length());
	wstring ws(outbuf);
	delete[] outbuf;
	return ws;
}

//! Get available language codes from directory tree
std::string Translator::getAvailableLanguagesCodes(const string& localeDir)
{
	struct dirent *entryp;
	DIR *dp;
	string result="";
	
	cout << "Reading stellarium translations in directory: " << localeDir << endl;

	if ((dp = opendir(localeDir.c_str())) == NULL)
	{
		cerr << "Unable to find locale directory containing translations:" << localeDir << endl;
		return "";
	}

	while ((entryp = readdir(dp)) != NULL)
	{
		string tmp = entryp->d_name;
		string tmpdir = localeDir+"/"+tmp+"/LC_MESSAGES/stellarium.mo";
		FILE* fic = fopen(tmpdir.c_str(), "r");
		if (fic)
		{
			if (result!="") result+="\n"; 
			result += tmp;
			fclose(fic);
		}
	}
	closedir(dp);
	
	return result;
}


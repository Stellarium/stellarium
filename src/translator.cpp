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
#include <vector>
#include <algorithm>

#include "bytes.h"
#include "translator.h"

Translator* Translator::lastUsed = NULL;

string Translator::systemLangName = "C";

// Use system locale language by default
Translator Translator::globalTranslator = Translator(PACKAGE, LOCALEDIR, "system");

#ifdef WIN32
# include <Windows.h>
# include <Winnls.h>
#endif

//! Try to determine system language from system configuration
void Translator::initSystemLanguage(void)
{
	char* lang = getenv("LANGUAGE");
	if (lang) Translator::systemLangName = lang;
	else
	{
		lang = getenv("LANG");
		if (lang) Translator::systemLangName = lang;
		else
		{
#ifdef WIN32            
            char cc[3];
            if(GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SISO639LANGNAME, cc, 3))
            {
                cc[2] = '\0';
                Translator::systemLangName = cc;
            }
            else
            {
                Translator::systemLangName = "C";
            }
#else
     Translator::systemLangName = "C";
#endif
        }
	}

	//change systemLangName to ISO 639 / ISO 3166.
	unsigned int pos = Translator::systemLangName.find(':', 0);
	if(pos != Translator::systemLangName.npos)
	       Translator::systemLangName.resize(pos);
	pos = Translator::systemLangName.find('.', 0);
	if(pos == 5) Translator::systemLangName.resize(pos);
}

void Translator::reload()
{
	if (Translator::lastUsed == this) return;
	// This needs to be static as it is used a each gettext call... It tooks me quite a while before I got that :(
	static char envstr[25];
#ifndef MACOSX	
	if (langName=="system" || langName=="system_default")
	{
		snprintf(envstr, 25, "LANGUAGE=%s", Translator::systemLangName.c_str());
//		cout << "TEST=" << envstr << " " << Translator::systemLangName << endl;
	}
	else
	{
		snprintf(envstr, 25, "LANGUAGE=%s", langName.c_str());
	}
#else
	if (langName=="system" || langName=="system_default")
	{
		snprintf(envstr, 25, "LANG=%s", Translator::systemLangName.c_str());
	}
	else
	{
		snprintf(envstr, 25, "LANG=%s", langName.c_str());	
#endif
	//printf("Setting locale: %s\n", envstr);
	putenv(envstr);
	setlocale(LC_MESSAGES, "");

	std::string result = bind_textdomain_codeset(domain.c_str(), "UTF-8");
	assert(result=="UTF-8");
	bindtextdomain (domain.c_str(), moDirectory.c_str());
	textdomain (domain.c_str());
	Translator::lastUsed = this;
}


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

		unicode[j] = ch;
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
	std::vector<string> result;
	
	//cout << "Reading stellarium translations in directory: " << localeDir << endl;

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
			result.push_back(tmp);
			fclose(fic);
		}
	}
	closedir(dp);
	
	// Sort the language names by alphabetic order
	std::sort(result.begin(), result.end());

	string output;
	std::vector<string>::iterator iter;
	for (iter=result.begin();iter!=result.end();++iter)
	{
		if (iter!=result.begin()) output+="\n";
		output+=*iter;
	}

	return output;
}


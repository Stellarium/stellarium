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
#include <algorithm>
#include <fstream>

#include "bytes.h"
#include "StelUtils.hpp"
#include "Translator.hpp"

// Init static members
Translator* Translator::lastUsed = NULL;
map<string, wstring> Translator::iso639codes;

string Translator::systemLangName = "C";

// Use system locale language by default
Translator Translator::globalTranslator = Translator(PACKAGE, LOCALEDIR, "system");

#ifdef WIN32
# include <windows.h>
# include <winnls.h>
#endif

//! Try to determine system language from system configuration
void Translator::initSystemLanguage(void)
{
	char* lang = getenv("LANGUAGE");
	if (lang) systemLangName = lang;
	else
	{
		lang = getenv("LANG");
		if (lang) systemLangName = lang;
		else
		{
#ifdef WIN32
			char cc[3];
			if (GetLocaleInfo(LOCALE_USER_DEFAULT,
			                  LOCALE_SISO639LANGNAME, cc, 3))
			{
				cc[2] = '\0';
				systemLangName = cc;
			}
			else
			{
				systemLangName = "C";
			}
#else
			systemLangName = "C";
#endif
		}
	}

	//change systemLangName to ISO 639 / ISO 3166.
	string::size_type pos = systemLangName.find(':', 0);
	if (pos != string::npos) systemLangName.resize(pos+1);
	pos = systemLangName.find('.', 0);
	if (pos == 5) systemLangName.resize(pos+1);
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
	}
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

//! Convert from ISO639-1 2 letters langage code to native language name
std::wstring Translator::iso639_1LanguageCodeToNativeName(const string& languageCode)
{
	if (iso639codes.find(languageCode)!=iso639codes.end())
		return iso639codes[languageCode];
	else
		return StelUtils::stringToWstring(languageCode);
}
	
//! Convert from native language name to ISO639-1 2 letters langage code 
std::string Translator::nativeLanguageNameCodeToIso639_1(const wstring& languageName)
{
	std::map<string, wstring>::iterator iter;
	for (iter=iso639codes.begin();iter!=iso639codes.end();++iter)
	{
		if ((*iter).second == languageName)
			return (*iter).first;
	}
	return StelUtils::wstringToString(languageName);
}

//! Get available language codes from directory tree
std::wstring Translator::getAvailableLanguagesNamesNative(const string& localeDir)
{
	std::vector<string> codeList = getAvailableLanguagesIso639_1Codes(localeDir);
	
	wstring output;
	std::vector<string>::iterator iter;
	for (iter=codeList.begin();iter!=codeList.end();++iter)
	{
		if (iter!=codeList.begin()) output+=L"\n";
		output+=iso639_1LanguageCodeToNativeName(*iter);
	}
	return output;
}

//! Get available language codes from directory tree
std::vector<string> Translator::getAvailableLanguagesIso639_1Codes(const string& localeDir)
{
	struct dirent *entryp;
	DIR *dp;
	std::vector<string> result;
	
	//cout << "Reading stellarium translations in directory: " << localeDir << endl;

	if ((dp = opendir(localeDir.c_str())) == NULL)
	{
		cerr << "Unable to find locale directory containing translations:" << localeDir << endl;
		return result;
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
	
	return result;
}

//! Initialize the languages code list from the passed file
//! @param fileName file containing the list of language codes
void Translator::initIso639_1LanguageCodes(const string& fileName)
{
	std::ifstream inf(fileName.c_str());
	if (!inf.is_open())
	{
		cerr << "Can't open ISO639 codes file " << fileName << endl;
		assert(0);
	}
	
	if (!iso639codes.empty())
		iso639codes.clear();
		
	string record;
	
	while(!getline(inf, record).eof())
	{
		string::size_type pos;
		if ((pos = record.find('\t', 4))==string::npos)
		{
			cerr << "Error: invalid entry in ISO639 codes: " << fileName << endl;
		}
		else
		{
			iso639codes.insert(pair<string, wstring>(record.substr(0, 2), UTF8stringToWstring(record.substr(pos+1))));
		}
	}
}

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
#include <clocale>
#include <cstdlib>
#include <QFile>
#include <QDebug>
#include <QStringList>

#include "StelUtils.hpp"
#include "Translator.hpp"


// Init static members
Translator* Translator::lastUsed = NULL;
QMap<QString, QString> Translator::iso639codes;
QString Translator::systemLangName;

// Use system locale language by default
#if defined(MACOSX)
#include "MacosxDirs.hpp"
Translator Translator::globalTranslator = Translator(PACKAGE_NAME, MacosxDirs::getApplicationResourcesDirectory().append( "/locale" ), "system");
#else
Translator Translator::globalTranslator = Translator(PACKAGE_NAME, INSTALL_LOCALEDIR, "system");
#endif

#ifdef WIN32
# include <windows.h>
# include <winnls.h>
#define putenv(x) _putenv((x))
#endif

//! Initialize Translation
//! @param fileName file containing the list of language codes
void Translator::init(const QString& fileName)
{
	Translator::initSystemLanguage();
	Translator::initIso639_1LanguageCodes(fileName);
}
		  
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
			if (GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SISO639LANGNAME, cc, 3))
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
	int pos = systemLangName.indexOf(':', 0);
	if (pos != -1) systemLangName.resize(pos+1);
	pos = systemLangName.indexOf('.', 0);
	if (pos == 5) systemLangName.resize(pos+1);
}

void Translator::reload()
{
	if (Translator::lastUsed == this)
		return;
	
	// Find out what the system language is if not defined yet
	if (systemLangName.isEmpty())
		initSystemLanguage();
	
	// Apply that
	// This needs to be static as it is used a each gettext call... It tooks me quite a while before I got that :(
	static char envstr[25];
	if (langName=="system" || langName=="system_default")
#if defined (MACOSX)	// MACOSX
	{
		snprintf(envstr, 25, "LANG=%s", Translator::systemLangName.toUtf8().constData());
	}
	else
	{
		snprintf(envstr, 25, "LANG=%s", langName.toUtf8().constData());
	}
#elif defined (_MSC_VER) //MSCVER
	{
		_snprintf(envstr, 25, "LANGUAGE=%s", Translator::systemLangName.toUtf8().constData());
	}
	else
	{
		_snprintf(envstr, 25, "LANGUAGE=%s", langName.toUtf8().constData());
	}
#else // UNIX
	{
		snprintf(envstr, 25, "LANGUAGE=%s", Translator::systemLangName.toUtf8().constData());
	}
	else
	{
		snprintf(envstr, 25, "LANGUAGE=%s", langName.toUtf8().constData());
	}
#endif

	putenv(envstr);
#if !defined (_MSC_VER)
	setlocale(LC_MESSAGES, "");
#else
	setlocale(LC_CTYPE,"");
#endif
	QString result = bind_textdomain_codeset(domain.toUtf8().constData(), "UTF-8");
	assert(result=="UTF-8");
	bindtextdomain (domain.toUtf8().constData(), QFile::encodeName(moDirectory).constData());
	textdomain (domain.toUtf8().constData());
	Translator::lastUsed = this;
}


//! Convert from ISO639-1 2 letters langage code to native language name
QString Translator::iso639_1CodeToNativeName(const QString& languageCode)
{
	if (iso639codes.find(languageCode)!=iso639codes.end())
		return iso639codes[languageCode];
	else
		return languageCode;
}
	
//! Convert from native language name to ISO639-1 2 letters langage code 
QString Translator::nativeNameToIso639_1Code(const QString& languageName)
{
	QMap<QString, QString>::iterator iter;
	for (iter=iso639codes.begin();iter!=iso639codes.end();++iter)
	{
		if (iter.value() == languageName)
			return iter.key();
	}
	return languageName;
}

//! Get available native language names from directory tree
QStringList Translator::getAvailableLanguagesNamesNative(const QString& localeDir) const
{
	QString tmpDir = localeDir;
	if (tmpDir.isEmpty())
		tmpDir = moDirectory;
	QStringList codeList = getAvailableIso639_1Codes(tmpDir);
	QStringList output;
	foreach (QString lang, codeList)
	{
		output+=iso639_1CodeToNativeName(lang);
	}
	return output;
}

//! Get available language codes from directory tree
QStringList Translator::getAvailableIso639_1Codes(const QString& localeDir) const
{
	QString locDir = localeDir;
	if (locDir.isEmpty())
		locDir = moDirectory;
	struct dirent *entryp;
	DIR *dp;
	QStringList result;

	if ((dp = opendir(QFile::encodeName(locDir).constData())) == NULL)
	{
		qWarning() << "Unable to find locale directory containing translations:" << localeDir;
		return result;
	}

	while ((entryp = readdir(dp)) != NULL)
	{
		QString tmp = entryp->d_name;
		QString tmpdir = locDir+"/"+tmp+"/LC_MESSAGES/stellarium.mo";
		FILE* fic = fopen(QFile::encodeName(tmpdir).constData(), "r");
		if (fic)
		{
			result.push_back(tmp);
			fclose(fic);
		}
	}
	closedir(dp);
	
	// Sort the language names by alphabetic order
	result.sort();
	
	return result;
}

//! Initialize the languages code list from the passed file
//! @param fileName file containing the list of language codes
void Translator::initIso639_1LanguageCodes(const QString& fileName)
{
	QFile inf(fileName);
	if (!inf.open(QIODevice::ReadOnly))
	{
		qWarning() << "Can't open ISO639 codes file " << fileName;
		assert(0);
	}
	
	if (!iso639codes.empty())
		iso639codes.clear();
	
	while (!inf.atEnd())
	{
		QByteArray record = inf.readLine();
		int pos = record.indexOf('\t', 4);
		if (pos==-1)
		{
			qWarning() << "Error: invalid entry in ISO639 codes: " << fileName;
		}
		else
		{
			// remove trailing newline, see QFile.readLine()
			record.chop(1);
			iso639codes.insert(record.left(2), QString::fromUtf8(record.mid(pos+1)));
		}
	}
}

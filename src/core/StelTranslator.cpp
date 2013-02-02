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

#include <dirent.h>
#include <cstdio>
#include <algorithm>
#include <fstream>
#include <clocale>
#include <cstdlib>
#include <QFile>
#include <QDebug>
#include <QStringList>
#include <QRegExp>
#include <QLocale>

#include "StelUtils.hpp"
#include "StelTranslator.hpp"
#include "StelFileMgr.hpp"

#ifdef ANDROID
#include "android/JavaWrapper.hpp"
#endif

// Init static members
StelTranslator* StelTranslator::lastUsed = NULL;
QMap<QString, QString> StelTranslator::iso639codes;
QString StelTranslator::systemLangName;

// Use system locale language by default
StelTranslator StelTranslator::globalTranslator = StelTranslator("stellarium", "./locale/", "system");

#ifdef Q_OS_WIN
# include <windows.h>
# include <winnls.h>
#define putenv(x) _putenv((x))
#endif

//! Initialize Translation
//! @param fileName file containing the list of language codes
void StelTranslator::init(const QString& fileName)
{
	StelTranslator::initSystemLanguage();
	StelTranslator::initIso639_1LanguageCodes(fileName);
}

//! Try to determine system language from system configuration
void StelTranslator::initSystemLanguage(void)
{
#ifdef ANDROID
	systemLangName = JavaWrapper::getLocaleString();
#else
	char* lang = getenv("LANGUAGE");
	if (lang) systemLangName = lang;
	else
	{
		lang = getenv("LANG");
		if (lang) systemLangName = lang;
		else
		{
#ifdef Q_OS_WIN
			char ulng[3], ctry[3];
			if (GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SISO639LANGNAME, ulng, 3))
			{
				ulng[2] = '\0';
				if (GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SISO3166CTRYNAME, ctry, 3))
				{
					ctry[2] = '\0';
					systemLangName = QString("%1_%2").arg(ulng).arg(ctry);
				}
				else
				{
					systemLangName = ulng;
				}
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
#endif //ANDROID-else

	if (systemLangName.isEmpty())
		systemLangName = "en";

	//change systemLangName to ISO 639 / ISO 3166.
	int pos = systemLangName.indexOf(':', 0);
	if (pos != -1) systemLangName.resize(pos);
	pos = systemLangName.indexOf('.', 0);
	if (pos != -1) systemLangName.resize(pos);
}

void StelTranslator::reload()
{
	if (StelTranslator::lastUsed == this)
		return;

	// Find out what the system language is if not defined yet
	if (systemLangName.isEmpty())
		initSystemLanguage();

	// Apply that
	// This needs to be static as it is used a each gettext call... It tooks me quite a while before I got that :(
	static char envstr[128];
	if (langName=="system" || langName=="system_default")
#ifdef Q_OS_MAC
	{
		qsnprintf(envstr, 128, "LANG=%s", StelTranslator::systemLangName.toUtf8().constData());
	}
	else
	{
		qsnprintf(envstr, 128, "LANG=%s", langName.toUtf8().constData());
	}
#elif defined (Q_OS_WIN) //MSCVER
	{
		qsnprintf(envstr, 128, "LANGUAGE=%s", StelTranslator::systemLangName.toUtf8().constData());
	}
	else
	{
		qsnprintf(envstr, 128, "LANGUAGE=%s", langName.toUtf8().constData());
	}
#elif ANDROID
	{
		langName = StelTranslator::systemLangName;
	}
#else // UNIX
	{
		qsnprintf(envstr, 128, "LANGUAGE=%s", StelTranslator::systemLangName.toUtf8().constData());
	}
	else
	{
		qsnprintf(envstr, 128, "LANGUAGE=%s", langName.toUtf8().constData());
	}
#endif

	putenv(envstr);
#ifndef Q_OS_WIN
	setlocale(LC_MESSAGES, "");
#else
	setlocale(LC_CTYPE,"");
#endif

#ifdef ANDROID
	loadMessageCatalog(domain.toUtf8().constData(), QFile::encodeName(moDirectory + "/" + langName + "/LC_MESSAGES/" + domain + ".mo").constData());
#else
	QString result = bind_textdomain_codeset(domain.toUtf8().constData(), "UTF-8");
	Q_ASSERT(result=="UTF-8");
	bindtextdomain (domain.toUtf8().constData(), QFile::encodeName(moDirectory).constData());
#endif
	textdomain (domain.toUtf8().constData());
	StelTranslator::lastUsed = this;
}


//! Convert from ISO639-1 2(+3) letters langage code to native language name
QString StelTranslator::iso639_1CodeToNativeName(const QString& languageCode)
{
	if (languageCode=="C")
		return "English";

	//QLocale loc(languageCode);
	//QString l = loc.name();
	// There is a QLocale for this code.  This should be the case for most
	// language codes, but there are a few without QLocales, e.g. Interlingua
	//if (l.contains('_'))
	//	l.truncate(l.indexOf('_'));
	//if (iso639codes.find(l)!=iso639codes.end())
	//	return iso639codes[l]+ (languageCode.size()==2 ? "" : QString(" (")+QLocale::countryToString(loc.country())+")");

	// For codes which return the locale C, use the language code to do the lookup
	if (iso639codes.contains(languageCode))
		return iso639codes[languageCode];

	// qWarning() << "WARNING: Cannot determine name of language for code" << languageCode;
	return languageCode;
}

//! Convert from native language name to ISO639-1 2(+3) letters langage code
QString StelTranslator::nativeNameToIso639_1Code(const QString& languageName)
{
	QMap<QString, QString>::iterator iter;
	for (iter=iso639codes.begin();iter!=iso639codes.end();++iter)
		if (iter.value() == languageName)
			return iter.key();

	return languageName;
}

//! Get available native language names from directory tree
QStringList StelTranslator::getAvailableLanguagesNamesNative(const QString& localeDir) const
{
	QString tmpDir = localeDir;
	if (tmpDir.isEmpty())
		tmpDir = moDirectory;
	QStringList codeList = getAvailableIso639_1Codes(tmpDir);
	QStringList output;
	foreach (QString lang, codeList)
	{
		output += iso639_1CodeToNativeName(lang);
	}
	return output;
}

//! Get available language codes from directory tree
QStringList StelTranslator::getAvailableIso639_1Codes(const QString& localeDir) const
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
void StelTranslator::initIso639_1LanguageCodes(const QString& fileName)
{
	QFile inf(fileName);
	if (!inf.open(QIODevice::ReadOnly))
	{
		qWarning() << "Can't open ISO639 codes file " << fileName;
		Q_ASSERT(0);
	}

	if (!iso639codes.empty())
		iso639codes.clear();

	while (!inf.atEnd())
	{
		QString record = QString::fromUtf8(inf.readLine());
		record.remove(QRegExp("[\\n\\r]*$")); // chomp new lines
		const QStringList& fields = record.split("\t", QString::SkipEmptyParts);
		iso639codes.insert(fields.at(0), fields.at(2));
	}
}

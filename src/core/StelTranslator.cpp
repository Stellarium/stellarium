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

#include "StelTranslator.hpp"
#include "StelFileMgr.hpp"
#include "StelUtils.hpp"

#include <QFile>
#include <QDebug>
#include <QStringList>
#include <QRegExp>
#include <QLocale>
#include <QDir>
#include <QTranslator>


// Init static members
QMap<QString, QString> StelTranslator::iso639codes;
QString StelTranslator::systemLangName;

// Use system locale language by default
StelTranslator* StelTranslator::globalTranslator = Q_NULLPTR;

StelTranslator::StelTranslator(const QString& adomain, const QString& alangName)
	: domain(adomain),
	  langName(alangName)
{
	translator = new QTranslator();
	bool res = translator->load(StelFileMgr::getLocaleDir()+"/"+adomain+"/"+getTrueLocaleName()+".qm");
	if (!res)
		qWarning() << "Couldn't load translations for language " << getTrueLocaleName() << "in section" << adomain;
	if (translator->isEmpty())
		qWarning() << "Empty translation file for language " << getTrueLocaleName() << "in section" << adomain;
}

StelTranslator::~StelTranslator()
{
	delete translator;
	translator = Q_NULLPTR;
}

QString StelTranslator::qtranslate(const QString& s, const QString& c) const
{
	if (s.isEmpty())
		return "";
	QString res = translator->translate("", s.toUtf8().constData(), c.toUtf8().constData());
	if (res.isEmpty())
		return s;
	return res;
}

QString StelTranslator::tryQtranslate(const QString &s, const QString &c) const
{
	return translator->translate("", s.toUtf8().constData(),c.toUtf8().constData());
}
	
//! Initialize Translation
//! @param fileName file containing the list of language codes
void StelTranslator::init(const QString& fileName)
{
	StelTranslator::initSystemLanguage();
	StelTranslator::initIso639_1LanguageCodes(fileName);
	
	Q_ASSERT(StelTranslator::globalTranslator==Q_NULLPTR);
	StelTranslator::globalTranslator = new StelTranslator("stellarium", "system");
}

//! Try to determine system language from system configuration
void StelTranslator::initSystemLanguage()
{
	systemLangName = QLocale::system().name();
	if (systemLangName.isEmpty())
		systemLangName = "en";

	//change systemLangName to ISO 639 / ISO 3166.
	int pos = systemLangName.indexOf(':', 0);
	if (pos != -1) systemLangName.resize(pos);
	pos = systemLangName.indexOf('.', 0);
	if (pos != -1) systemLangName.resize(pos);
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
	QMap<QString, QString>::ConstIterator iter;
	for (iter=iso639codes.begin();iter!=iso639codes.end();++iter)
		if (iter.value() == languageName)
			return iter.key();

	return languageName;
}

//! Get available native language names from directory tree
QStringList StelTranslator::getAvailableLanguagesNamesNative(const QString& localeDir, const QString& section) const
{
	QString tmpDir = localeDir;
	if (section.isEmpty() || section=="stellarium")
		tmpDir.append("/stellarium/");
	else
		tmpDir.append("/stellarium-" + section + "/");
	QStringList codeList = getAvailableIso639_1Codes(tmpDir);
	QStringList output;
	for (const auto& lang : codeList)
	{
		output += iso639_1CodeToNativeName(lang);
	}
	return output;
}

//! Get available language codes from directory tree
QStringList StelTranslator::getAvailableIso639_1Codes(const QString& localeDir)
{
	QDir dir(localeDir);

	if (!dir.exists())
	{
		qWarning() << "Unable to find locale directory containing translations:" << QDir::toNativeSeparators(localeDir);
		return QStringList();
	}

	QStringList result;
	for (auto path : dir.entryList(QDir::Files, QDir::Name))
	{
		if (!path.endsWith(".qm"))
			continue;
		path.chop(3);
		result.append(path);
	}
	return result;
}

//! Initialize the languages code list from the passed file
//! @param fileName file containing the list of language codes
void StelTranslator::initIso639_1LanguageCodes(const QString& fileName)
{
	QFile inf(fileName);
	if (!inf.open(QIODevice::ReadOnly))
	{
		qWarning() << "Can't open ISO639 codes file " << QDir::toNativeSeparators(fileName);
		Q_ASSERT(0);
	}

	if (!iso639codes.empty())
		iso639codes.clear();

	while (!inf.atEnd())
	{
		QString record = QString::fromUtf8(inf.readLine());
		record.remove(QRegExp("[\\n\\r]*$")); // chomp new lines
		#if (QT_VERSION>=QT_VERSION_CHECK(5, 14, 0))
		const QStringList& fields = record.split("\t", Qt::SkipEmptyParts);
		#else
		const QStringList& fields = record.split("\t", QString::SkipEmptyParts);
		#endif
		iso639codes.insert(fields.at(0), fields.at(2));
	}
}

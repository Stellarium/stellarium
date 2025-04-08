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

#include <QFile>
#include <QDebug>
#include <QStringList>
#include <QRegularExpression>
#include <QLocale>
#include <QDir>
#include <QTranslator>

namespace
{
int parseRomanNumeral(const QStringView& roman)
{
	const auto romanLat = roman.toLatin1();
	int v = 0;
	const char* p = romanLat.data();

	/**/ if (strncmp(p, "XC",   2) == 0) { v += 90; p += 2; }
	else if (strncmp(p, "LXXX", 4) == 0) { v += 80; p += 4; }
	else if (strncmp(p, "LXX",  3) == 0) { v += 70; p += 3; }
	else if (strncmp(p, "LX",   2) == 0) { v += 60; p += 2; }
	else if (strncmp(p, "L",    1) == 0) { v += 50; p += 1; }
	else if (strncmp(p, "XL",   2) == 0) { v += 40; p += 2; }
	else if (strncmp(p, "XXX",  3) == 0) { v += 30; p += 3; }
	else if (strncmp(p, "XX",   2) == 0) { v += 20; p += 2; }
	else if (strncmp(p, "X",    1) == 0) { v += 10; p += 1; }

	/**/ if (strncmp(p, "IX",   2) == 0) { v += 9;  p += 2; }
	else if (strncmp(p, "VIII", 4) == 0) { v += 8;  p += 4; }
	else if (strncmp(p, "VII",  3) == 0) { v += 7;  p += 3; }
	else if (strncmp(p, "VI",   2) == 0) { v += 6;  p += 2; }
	else if (strncmp(p, "V",    1) == 0) { v += 5;  p += 1; }
	else if (strncmp(p, "IV",   2) == 0) { v += 4;  p += 2; }
	else if (strncmp(p, "III",  3) == 0) { v += 3;  p += 3; }
	else if (strncmp(p, "II",   2) == 0) { v += 2;  p += 2; }
	else if (strncmp(p, "I",    1) == 0) { v += 1;  p += 1; }

	return v;
}
}

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

bool StelTranslator::isEmpty() const
{
	return translator->isEmpty();
}

QString StelTranslator::qtranslate(const QString& s, const QString& c) const
{
	if (s.isEmpty())
		return "";
	const auto res = tryQtranslate(s, c);
	if (res.isEmpty())
		return s;
	return res;
}

QString StelTranslator::qTranslateStar(const QString& s, const QString& c) const
{
	if (s.isEmpty())
		return "";
	const auto res = tryQtranslateStar(s, c);
	if (res.isEmpty())
		return s;
	return res;
}

QString StelTranslator::tryTranslateChineseStar(const QString& s, const QString& c) const
{
	static const auto re = []{ QRegularExpression re("(.+)( [IXVLCDM]+)([*?]*)$"); re.optimize(); return re; }();
	const auto match = re.match(s);
	if (!match.hasMatch()) return {};

	auto constellation = match.captured(1);
	bool addedPresent = false;
	static const QString addedPattern(" Added");
	if (constellation.endsWith(addedPattern))
	{
		constellation.chop(addedPattern.length());
		addedPresent = true;
	}

	const auto translatedConstellation = tryQtranslate(constellation, c);
	if (translatedConstellation.isEmpty()) return {};

	QString number = match.captured(2);
	if (getTrueLocaleName().startsWith("zh"))
	{
		const auto num = parseRomanNumeral(QStringView(number).mid(1));
		Q_ASSERT(num < 100);

		static const char16_t chars[10+1] = u"十一二三四五六七八九";

		number = "";
		int tens = num / 10;
		const int units = num % 10;
		if (tens >= 2)
		{
			number += chars[tens];
			tens = 1;
		}
		if (tens == 1)
		{
			number += QChar(chars[0]);
		}
		if (units)
		{
			number += chars[units];
		}
	}
	const QString extra = match.captured(3);
	if (!addedPresent) return translatedConstellation + number + extra;

	// TRANSLATORS: The string " Added" stars with a space. Carefully keep the space if needed with your language.
	const QString& translatedAdded = qtranslate(" Added", "chinese skycultures");
	return translatedConstellation + translatedAdded + number + extra;
}

QString StelTranslator::tryQtranslate(const QString &s, const QString &c) const
{
	return translator->translate("", s.toUtf8().constData(),c.toUtf8().constData());
}

QString StelTranslator::tryQtranslateStar(const QString &s, const QString &c) const
{
	const auto translated = tryTranslateChineseStar(s, c);
	if (!translated.isEmpty()) return translated;
	return tryQtranslate(s, c);
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

	qInfo().noquote() << "System language (ISO 639 / ISO 3166):" << systemLangName;
}


//! Convert from ISO639-1 2(+3) letters language code to native language name
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

	// qWarning() << "Cannot determine name of language for code" << languageCode;
	return languageCode;
}

//! Convert from native language name to ISO639-1 2(+3) letters language code
QString StelTranslator::nativeNameToIso639_1Code(const QString& languageName)
{
	QMap<QString, QString>::ConstIterator iter;
	for (iter=iso639codes.constBegin();iter!=iso639codes.constEnd();++iter)
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
	const QStringList codeList = getAvailableIso639_1Codes(tmpDir);
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

	const QStringList entries=dir.entryList(QDir::Files, QDir::Name);
	QStringList result;
	for (auto path : entries)
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

		if (record.startsWith("//") || record.startsWith("#") || record.isEmpty()) // skip comments and empty lines
			continue;

		static const QRegularExpression nlExp("[\\n\\r]*$");
		record.remove(nlExp); // chomp new lines
		#if (QT_VERSION>=QT_VERSION_CHECK(5, 14, 0))
		const QStringList& fields = record.split("\t", Qt::SkipEmptyParts);
		#else
		const QStringList& fields = record.split("\t", QString::SkipEmptyParts);
		#endif
		iso639codes.insert(fields.at(0), fields.at(2));
	}
}

StelSkyTranslator::StelSkyTranslator(const QString& langName)
	: StelTranslator("stellarium-skycultures", langName)
	, commonSkyTranslator("stellarium-sky", langName)
{
	if (commonSkyTranslator.isEmpty())
		qWarning() << "Empty skyculture-independent translation file for language " << getTrueLocaleName();
}

QString StelSkyTranslator::tryQtranslate(const QString& s, const QString& c) const
{
	const auto res = StelTranslator::tryQtranslate(s, c);
	if (!res.isEmpty()) return res;
	return commonSkyTranslator.tryQtranslate(s, c);
}

bool StelSkyTranslator::isEmpty() const
{
	return StelTranslator::isEmpty() && commonSkyTranslator.isEmpty();
}

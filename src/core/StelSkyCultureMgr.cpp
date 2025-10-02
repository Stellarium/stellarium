/*
 * Stellarium
 * Copyright (C) 2006 Fabien Chereau
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

#include "StelSkyCultureMgr.hpp"
#include "Star.hpp"
#include "StarMgr.hpp"
#include "StelFileMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StelObjectMgr.hpp"
#include "StelTranslator.hpp"
#include "StelLocaleMgr.hpp"
#include "ConstellationMgr.hpp"
#include "StelApp.hpp"

#include <md4c-html.h>

#include <QSettings>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QDebug>
#include <QMap>
#include <QMapIterator>
#include <QDir>
#include <QJsonObject>
#include <QJsonDocument>
#include <QRegularExpression>
#include <QMetaEnum>

namespace
{

#if (QT_VERSION>=QT_VERSION_CHECK(5, 14, 0))
constexpr auto SkipEmptyParts = Qt::SkipEmptyParts;
#else
constexpr auto SkipEmptyParts = QString::SkipEmptyParts;
#endif

void applyStyleToMarkdown(QString& string)
{
	static const auto pPattern = []{
		// Locates any paragraph. This will make sure we get the smallest paragraphs. If
		// we look for the styled ones from the beginning, we'll get too greedy matches.
		QRegularExpression p("<p>.+?</p>", QRegularExpression::DotMatchesEverythingOption);
		p.optimize();
		return p;
	}();
	static const auto pStyledPattern = []{
		// This is for the actual paragraphs we are interested in.
		QRegularExpression sp(R"regex(<p>(.+?){:\s*\.(\S+?)\s*}\s*</p>)regex",
		                      QRegularExpression::DotMatchesEverythingOption);
		sp.optimize();
		return sp;
	}();
	bool replaced;
	do
	{
		replaced = false;
		for(auto matches = pPattern.globalMatch(string); matches.hasNext(); )
		{
			const auto& match = matches.next();
			const auto substr = match.captured(0);
			if(substr.contains(pStyledPattern))
			{
				// Don't replace the original string directly: otherwise
				// we'll again have greedy matches.
				auto newSubstr = substr;
				newSubstr.replace(pStyledPattern, R"(<p class="\2">\1</p>)");

				// Also force the caption to be below the image,
				// rather than to the right of it.
				static const QRegularExpression re("(<img [^>]+>)");
				newSubstr.replace(re, "\\1<br>");

				string.replace(substr, newSubstr);

				replaced = true;
				break;
			}
		}
	} while(replaced);
}

QString markdownToHTML(QString input)
{
	const auto inputUTF8 = input.toStdString();

	std::string outputUTF8;
	::md_html(inputUTF8.data(), inputUTF8.size(),
	          [](const char* html, const MD_SIZE size, void* output)
	          { static_cast<std::string*>(output)->append(html, size); },
	          &outputUTF8, MD_DIALECT_GITHUB, 0);

	auto result = QString::fromStdString(outputUTF8);
	applyStyleToMarkdown(result);
	return result;
}

QString convertReferenceLinks(QString text)
{
	static const QRegularExpression re(" ?\\[#([0-9]+)\\]", QRegularExpression::MultilineOption);
	text.replace(re,
	             "<sup><a href=\"#cite_\\1\">[\\1]</a></sup>");
	return text;
}

}

QString StelSkyCultureMgr::getSkyCultureEnglishName(const QString& idFromJSON) const
{
	const auto skyCultureId = idFromJSON;
	const QString descPath = StelFileMgr::findFile("skycultures/" + skyCultureId + "/description.md");
	if (descPath.isEmpty())
	{
		qWarning() << "Can't find description for skyculture" << skyCultureId;
		return idFromJSON;
	}

	QFile f(descPath);
	if (!f.open(QIODevice::ReadOnly))
	{
		qWarning().nospace() << "Failed to open sky culture description file " << descPath << ": " << f.errorString();
		return idFromJSON;
	}

	for (int lineNum = 1;; ++lineNum)
	{
		const auto line = QString::fromUtf8(f.readLine()).trimmed();
		if (line.isEmpty()) continue;
		if (!line.startsWith("#"))
		{
			qWarning().nospace() << "Sky culture description file " << descPath << " at line "
			                     << lineNum << " has wrong format (expected a top-level header, got " << line;
			return idFromJSON;
		}
		return line.mid(1).trimmed();
	}

	qWarning() << "Failed to find sky culture name in" << descPath;
	return idFromJSON;
}

StelSkyCultureMgr::StelSkyCultureMgr(): flagOverrideUseCommonNames(false), flagUseAbbreviatedNames(false)
{
	setObjectName("StelSkyCultureMgr");
	if (StelApp::isInitialized()) // allow unit test...
	{
		QSettings *conf=StelApp::getInstance().getSettings();
		setFlagUseAbbreviatedNames(conf->value("viewing/flag_constellation_abbreviations", false).toBool());
	}
	makeCulturesList(); // First load needed for testing only.
}

StelSkyCultureMgr::~StelSkyCultureMgr()
{
}

void StelSkyCultureMgr::makeCulturesList()
{
	QSet<QString> cultureDirNames = StelFileMgr::listContents("skycultures",StelFileMgr::Directory);
	for (const auto& dir : std::as_const(cultureDirNames))
	{
		constexpr char indexFileName[] = "/index.json";
		const QString filePath = StelFileMgr::findFile("skycultures/" + dir + indexFileName);
		if (filePath.isEmpty())
		{
			qCritical() << "Failed to find" << indexFileName << "file in sky culture directory" << QDir::toNativeSeparators(dir);
			continue;
		}
		QFile file(filePath);
		if (!file.open(QFile::ReadOnly))
		{
			qCritical() << "Failed to open" << indexFileName << "file in sky culture directory" << QDir::toNativeSeparators(dir);
			continue;
		}
		const auto jsonText = file.readAll();
		if (jsonText.isEmpty())
		{
			qCritical() << "Failed to read data from" << indexFileName << "file in sky culture directory"
			            << QDir::toNativeSeparators(dir);
			continue;
		}
		QJsonParseError error;
		const auto jsonDoc = QJsonDocument::fromJson(jsonText, &error);
		if (error.error != QJsonParseError::NoError)
		{
			qCritical().nospace() << "Failed to parse " << indexFileName << " from sky culture directory "
			                      << QDir::toNativeSeparators(dir) << ": " << error.errorString();
			continue;
		}
		if (!jsonDoc.isObject())
		{
			qCritical() << "Failed to find the expected JSON structure in" << indexFileName << " from sky culture directory"
			            << QDir::toNativeSeparators(dir);
			continue;
		}
		const auto data = jsonDoc.object();

		auto& culture = dirToNameEnglish[dir];
		culture.path = StelFileMgr::dirName(filePath);
		const auto id = data["id"].toString();
		if(id != dir)
			qWarning() << "Sky culture id" << id << "doesn't match directory name" << dir;
		culture.id = id;
		culture.englishName = getSkyCultureEnglishName(dir);
		culture.region = data["region"].toString();
		if (culture.region.length()==0)
		{
			qWarning() << "No geographic region declared in skyculture" << id << ". setting \"World\"";
			culture.region = "World";
		}
		if (data["constellations"].isArray())
		{
			culture.constellations = data["constellations"].toArray();
		}
		else
		{
			qWarning() << "No \"constellations\" array found in JSON data in sky culture directory"
			           << QDir::toNativeSeparators(dir);
		}

		culture.asterisms = data["asterisms"].toArray();
		culture.langsUseNativeNames = data["langs_use_native_names"].toArray();

		culture.boundariesType = StelSkyCulture::BoundariesType::None; // default value if not specified in the JSON file
		if (data.contains("edges") && data.contains("edges_type"))
		{
			const QString type = data["edges_type"].toString();
			const QString typeSimp = type.simplified().toUpper();
			static const QMap<QString, StelSkyCulture::BoundariesType> map={
			        {"IAU", StelSkyCulture::BoundariesType::IAU},
			        {"OWN", StelSkyCulture::BoundariesType::Own},
			        {"NONE", StelSkyCulture::BoundariesType::None}
			};
			if (!map.contains(typeSimp))
				qWarning().nospace() << "Unexpected edges_type value in sky culture " << dir
				                     << ": " << type << ". Will resort to Own.";
			culture.boundariesType = map.value(typeSimp, StelSkyCulture::BoundariesType::Own);
		}
		culture.boundaries = data["edges"].toArray();
		culture.boundariesEpoch = data["edges_epoch"].toString("J2000");
		culture.fallbackToInternationalNames = (flagOverrideUseCommonNames || data["fallback_to_international_names"].toBool());
		culture.names = data["common_names"].toObject();

		if (data.contains("zodiac"))
			culture.zodiac = data["zodiac"].toObject();
		if (data.contains("lunar_system"))
			culture.lunarSystem = data["lunar_system"].toObject();

		const auto classifications = data["classification"].toArray();
		if (classifications.isEmpty())
		{
			culture.classification = StelSkyCulture::INCOMPLETE;
		}
		else
		{
			static const QMap <QString, StelSkyCulture::CLASSIFICATION>classificationMap={
				{ "traditional",  StelSkyCulture::TRADITIONAL},
				{ "historical",   StelSkyCulture::HISTORICAL},
				{ "ethnographic", StelSkyCulture::ETHNOGRAPHIC},
				{ "single",       StelSkyCulture::SINGLE},
				{ "comparative",  StelSkyCulture::COMPARATIVE},
				{ "personal",     StelSkyCulture::PERSONAL},
				{ "incomplete",   StelSkyCulture::INCOMPLETE},
			};
			const auto classificationStr = classifications[0].toString(); // We'll take only the first item for now.
			const auto classification=classificationMap.value(classificationStr.toLower(), StelSkyCulture::INCOMPLETE);
			if (!classificationMap.contains(classificationStr.toLower()))
			{
				qInfo() << "Skyculture " << dir << "has UNKNOWN classification: " << classificationStr;
				qInfo() << "Please edit index.json and change to a supported value. For now, this equals 'incomplete'";
			}
			culture.classification = classification;
		}
	}
}

//! Init itself from a config file.
void StelSkyCultureMgr::init()
{
	QSettings* settings = StelApp::getInstance().getSettings();
	Q_ASSERT(settings);
	setFlagOverrideUseCommonNames(settings->value("viewing/flag_skyculture_always_fallback_to_international_names", false).toBool());

	makeCulturesList(); // Reload after setting this flag!
	defaultSkyCultureID = StelApp::getInstance().getSettings()->value("localization/sky_culture", "modern").toString();
	if (defaultSkyCultureID=="western") // switch to new Sky Culture ID
		defaultSkyCultureID = "modern";
	setCurrentSkyCultureID(defaultSkyCultureID);
}

void StelSkyCultureMgr::reloadSkyCulture()
{
	const QString currentID=currentSkyCulture.id;
	makeCulturesList();
	setCurrentSkyCultureID(currentID);
}

//! Set the current sky culture from the passed directory
bool StelSkyCultureMgr::setCurrentSkyCultureID(const QString& cultureDir)
{
	QString scID = cultureDir;
	bool result = true;
	// make sure culture definition exists before attempting or will die
	if (directoryToSkyCultureEnglish(cultureDir) == "")
	{
		qWarning() << "Invalid sky culture directory: " << QDir::toNativeSeparators(cultureDir);
		scID = "modern";
		result = false;
	}

	currentSkyCulture = dirToNameEnglish[scID];

	// Lookup culture Style!
	setScreenLabelStyle(getScreenLabelStyle());
	setInfoLabelStyle(getInfoLabelStyle());

	emit currentSkyCultureChanged(currentSkyCulture);
	emit currentSkyCultureIDChanged(currentSkyCulture.id);
	return result;
}

// Set the default sky culture from the ID.
bool StelSkyCultureMgr::setDefaultSkyCultureID(const QString& id)
{
	// make sure culture definition exists before attempting or will die
	if (directoryToSkyCultureEnglish(id) == "")
	{
		qWarning() << "Invalid sky culture ID: " << id;
		return false;
	}
	defaultSkyCultureID = id;
	QSettings* conf = StelApp::getInstance().getSettings();
	Q_ASSERT(conf);
	conf->setValue("localization/sky_culture", id);

	emit defaultSkyCultureIDChanged(id);
	return true;
}

QString StelSkyCultureMgr::getCurrentSkyCultureNameI18() const
{
	const StelTranslator& trans = StelApp::getInstance().getLocaleMgr().getSkyCultureDescriptionsTranslator();
	return trans.qtranslate(currentSkyCulture.englishName, "sky culture");
}

QString StelSkyCultureMgr::getCurrentSkyCultureEnglishName() const
{
	return currentSkyCulture.englishName;
}

StelSkyCulture::BoundariesType StelSkyCultureMgr::getCurrentSkyCultureBoundariesType() const
{
	return currentSkyCulture.boundariesType;
}

StelSkyCulture::CLASSIFICATION StelSkyCultureMgr::getCurrentSkyCultureClassificationIdx() const
{
	return currentSkyCulture.classification;
}

QString StelSkyCultureMgr::getCurrentSkyCultureHtmlClassification() const
{
	QString classification, description, color;
	switch (currentSkyCulture.classification)
	{
		case StelSkyCulture::ETHNOGRAPHIC:
			color = "#33ff33"; // "green" area
			classification = qc_("ethnographic", "sky culture classification");
			description = q_("Provided by ethnographic researchers based on interviews of indigenous people.");
			break;
		case StelSkyCulture::HISTORICAL:
			color = "#33ff33"; // "green" area
			classification = qc_("historical", "sky culture classification");
			description = q_("Provided by historians based on historical written sources from a (usually short) period of the past.");
			break;
		case StelSkyCulture::SINGLE:
			color = "#33ff33"; // "green" area
			classification = qc_("single", "sky culture classification");
			description = q_("Represents a single source like a historical atlas, or publications of a single author.");
			break;
		case StelSkyCulture::COMPARATIVE:
			color = "#2090ff"; // "blue" area
			classification = qc_("comparative", "sky culture classification");
			description = q_("Compares and confronts elements from at least two sky cultures with each other.");
			break;
		case StelSkyCulture::TRADITIONAL:
			color = "#33ff33"; // "green" area
			classification = qc_("traditional", "sky culture classification");
			description = q_("Content represents 'common' knowledge by several members of an ethnic community, and the sky culture has been developed by members of such community.");
			break;
		case StelSkyCulture::PERSONAL:
			color = "#ffff00"; // "yellow" area
			classification = qc_("personal", "sky culture classification");
			description = q_("This is a personally developed sky culture which is not founded in published historical or ethnological research. Stellarium may include it when it is 'pretty enough' without really approving its contents.");
			break;
		case StelSkyCulture::INCOMPLETE:
			color = "#ff6633"; // "red" area
			classification = qc_("incomplete", "sky culture classification");
			description = q_("The accuracy of the sky culture description cannot be given, although it looks like it is built on a solid background. More work would be needed.");
			break;
		default: // undefined
			color = "#ff00cc";
			classification = qc_("undefined", "sky culture classification");
			description = QString();
			break;
	}

	QString html = QString();
	if (!description.isEmpty()) // additional info for sky culture (metainfo): let's use italic
		html = QString("<dl><dt><span style='color:%4;'>%5</span> <strong>%1: %2</strong></dt><dd><em>%3</em></dd></dl>").arg(q_("Classification"), classification, description, color, QChar(0x25CF));

	return html;
}


std::pair<QString/*color*/,QString/*info*/> StelSkyCultureMgr::getLicenseDescription(const QString& license, const bool singleLicenseForAll) const
{
	QString color, description;

	if (license.isEmpty())
	{
		color = "#2090ff"; // "blue" area
		description = q_("This sky culture is provided under unknown license. Please ask authors for details about license for this sky culture.");
	}
	else if (license.contains("GPL", Qt::CaseSensitive))
	{
		color = "#33ff33"; // "green" area; free license
		if (singleLicenseForAll)
			description = q_("This sky culture is provided under GNU General Public License. You can use it for commercial "
			                 "and non-commercial purposes, freely adapt it and share adapted work.");
		else
			description = q_("You can use it for commercial and non-commercial purposes, freely adapt it and share adapted work.");
	}
	else if (license.contains("MIT", Qt::CaseSensitive))
	{
		color = "#33ff33"; // "green" area; free license
		if (singleLicenseForAll)
			description = q_("This sky culture is provided under MIT License. You can use it for commercial and non-commercial "
			                 "purposes, freely adapt it and share adapted work.");
		else
			description = q_("You can use it for commercial and non-commercial purposes, freely adapt it and share adapted work.");
	}
	else if (license.contains("Public Domain"))
	{
		color = "#33ff33"; // "green" area; free license
		if (singleLicenseForAll)
			description = q_("This sky culture is distributed as public domain.");
		else
			description = q_("This is distributed as public domain.");
	}
	else if (license.startsWith("CC", Qt::CaseSensitive) || license.contains("Creative Commons", Qt::CaseInsensitive))
	{
		if (singleLicenseForAll)
			description = q_("This sky culture is provided under Creative Commons License.");

		QStringList details = license.split(" ", SkipEmptyParts);

		const QMap<QString, QString>options = {
			{ "BY",       q_("You may distribute, remix, adapt, and build upon this sky culture, even commercially, as long "
			                 "as you credit authors for the original creation.") },
			{ "BY-SA",    q_("You may remix, adapt, and build upon this sky culture even for commercial purposes, as long as "
			                 "you credit authors and license the new creations under the identical terms. This license is often "
			                 "compared to “copyleft” free and open source software licenses.") },
			{ "BY-ND",    q_("You may reuse this sky culture for any purpose, including commercially; however, adapted work "
			                 "cannot be shared with others, and credit must be provided by you.") },
			{ "BY-NC",    q_("You may remix, adapt, and build upon this sky culture non-commercially, and although your new works "
			                 "must also acknowledge authors and be non-commercial, you don’t have to license your derivative works "
			                 "on the same terms.") },
			{ "BY-NC-SA", q_("You may remix, adapt, and build upon this sky culture non-commercially, as long as you credit "
			                 "authors and license your new creations under the identical terms.") },
			{ "BY-NC-ND", q_("You may use this sky culture and share them with others as long as you credit authors, but you can’t "
			                 "change it in any way or use it commercially.") },
		};

		color = "#33ff33"; // "green" area; free license
		if (license.contains("ND", Qt::CaseSensitive))
			color = "#ffff00"; // "yellow" area; nonfree license - weak restrictions
		if (license.contains("NC", Qt::CaseSensitive))
			color = "#ff6633"; // "red" area; nonfree license - strong restrictions

		if (!details.at(0).startsWith("CC0", Qt::CaseInsensitive)) // Not public domain!
			description.append(QString(" %1").arg(options.value(details.at(1), "")));
		else
			description = q_("This sky culture is distributed as public domain.");
	}
	else if (license.contains("FAL", Qt::CaseSensitive) || license.contains("Free Art License", Qt::CaseSensitive))
	{
		color = "#33ff33"; // "green" area; free license
		description.append(QString(" %1").arg(q_("Free Art License grants the right to freely copy, distribute, and transform.")));
	}

	return std::make_pair(color, description);
}

QString StelSkyCultureMgr::getCurrentSkyCultureHtmlLicense() const
{
	static const QRegularExpression licRe("\\s*\n+\\s*");
	const auto lines = currentSkyCulture.license.split(licRe, SkipEmptyParts);
	if (lines.isEmpty()) return "";

	if (lines.size() == 1)
	{
		const auto parts = lines[0].split(":", SkipEmptyParts);
		const auto licenseName = convertReferenceLinks(parts.size() == 1 ? parts[0] : parts[1]);
		const auto [color, description] = getLicenseDescription(licenseName, true);
		if (!description.isEmpty())
		{
			return QString("<dl><dt><span style='color:%4;'>%5</span> <strong>%1: %2</strong></dt><dd><em>%3</em></dd></dl>")
				.arg(q_("License"),
				     currentSkyCulture.license.isEmpty() ? q_("unknown") : licenseName,
				     description, color, QChar(0x25CF));
		}
		else
		{
			return QString("<dl><dt><span style='color:%3;'>%4</span> <strong>%1: %2</strong></dt></dl>")
				.arg(q_("License"),
				     currentSkyCulture.license.isEmpty() ? q_("unknown") : licenseName, color, QChar(0x25CF));
		}
		return QString{};
	}
	else
	{
		QString html = "<h1>" + q_("License") + "</h1>\n";
		QString addendum;
		for (const auto& line : lines)
		{
			static const QRegularExpression re("\\s*:\\s*");
			const auto parts = line.split(re, SkipEmptyParts);
			if (parts.size() == 1)
			{
				addendum += line + "<br>\n";
				continue;
			}
			const auto [color, description] = getLicenseDescription(parts[1], false);
			if (description.isEmpty())
			{
				html += QString("<dl><dt><span style='color:%2;'>%3</span> <strong>%1</strong></dt></dl>")
				               .arg(convertReferenceLinks(line), color, QChar(0x25CF));
			}
			else
			{
				html += QString("<dl><dt><span style='color:%3;'>%4</span> <strong>%1</strong></dt><dd><em>%2</em></dd></dl>")
				               .arg(convertReferenceLinks(line), description, color, QChar(0x25CF));
			}
		}
		return html + addendum;
	}
}

QString StelSkyCultureMgr::getCurrentSkyCultureHtmlRegion() const
{
	QString html = "", region = currentSkyCulture.region.trimmed();
	QString description = q_("The region indicates the geographical area of origin of a given sky culture.");

	// special case: modern sky culture
	if (getCurrentSkyCultureID().contains("modern", Qt::CaseInsensitive))
	{
		// TRANSLATIONS: This is the name of a geographical "pseudo-region" on Earth
		region = N_("World");
		description = q_("All 'modern' sky cultures are based on the IAU-approved 88 constellations with standardized boundaries and are used worldwide. The origins of these constellations are pan-European.");
	}

	if (!region.isEmpty()) // Region marker is always 'green'
		html = QString("<dl><dt><span style='color:#33ff33;'>%4</span> <strong>%1 %2</strong></dt><dd><em>%3</em></dd></dl>").arg(q_("Region:"), q_(region), description, QChar(0x25CF));

	return html;
}

bool StelSkyCultureMgr::setCurrentSkyCultureNameI18(const QString& cultureName)
{
	return setCurrentSkyCultureID(skyCultureI18ToDirectory(cultureName));
}

// returns list of human readable culture names in english
QStringList StelSkyCultureMgr::getSkyCultureListEnglish(void) const
{
	QStringList cultures;
	QMapIterator<QString, StelSkyCulture> i(dirToNameEnglish);
	while(i.hasNext())
	{
		i.next();
		cultures << i.value().englishName;
	}
	return cultures;
}

//! returns newline delimited list of human readable culture names translated to current locale
QStringList StelSkyCultureMgr::getSkyCultureListI18(void) const
{
	const StelTranslator& trans = StelApp::getInstance().getLocaleMgr().getSkyCultureDescriptionsTranslator();
	QStringList cultures;
	QMapIterator<QString, StelSkyCulture> i(dirToNameEnglish);
	while (i.hasNext())
	{
		i.next();
		cultures += trans.qtranslate(i.value().englishName, "sky culture");
	}
	// Sort for GUI use.
	std::sort(cultures.begin(), cultures.end(), [](const QString& a, const QString& b)
	          { return a.localeAwareCompare(b) < 0; });
	return cultures;
}

QStringList StelSkyCultureMgr::getSkyCultureListIDs(void) const
{
	return dirToNameEnglish.keys();
}

std::vector<std::pair<QString, QString>> StelSkyCultureMgr::getConstellationsDescriptions(QString consSection) const
{
	static const QRegularExpression startWithL5Section("^#####[^#]");
	consSection = consSection.trimmed();
	if (!consSection.contains(startWithL5Section))
	{
		// Invalid formatting of the Constellations section
		return {};
	}

	const QRegularExpression l5SectionNamePat("^[ \t]*#####[ \t]*([^#\n][^\n]*)$",
	                                          QRegularExpression::MultilineOption);
	std::vector<std::pair<QString/*constellation*/, QString/*description*/>> descrMap;
	QString prevSectionName;
	qsizetype prevBodyStartPos = -1;
	for (auto it = l5SectionNamePat.globalMatch(consSection); it.hasNext(); )
	{
		const auto match = it.next();
		const auto sectionName = match.captured(1);
		const auto nameStartPos = match.capturedStart(0);
		const auto bodyStartPos = match.capturedEnd(0);
		if (!prevSectionName.isEmpty())
		{
			const auto sectionText = consSection.mid(prevBodyStartPos, nameStartPos - prevBodyStartPos);
			if (!sectionText.isEmpty())
				descrMap.push_back({prevSectionName, sectionText});
		}
		prevBodyStartPos = bodyStartPos;
		prevSectionName = sectionName;
	}
	if (prevBodyStartPos >= 0)
	{
		const auto sectionText = consSection.mid(prevBodyStartPos);
		if (!sectionText.isEmpty())
			descrMap.push_back({prevSectionName, sectionText});
	}
	return descrMap;
}

QString StelSkyCultureMgr::convertMarkdownLevel2Section(const QString& markdown, const QString& sectionName,
                                                        const qsizetype bodyStartPos, const qsizetype bodyEndPos,
                                                        const StelTranslator& trans)
{
	auto textEng = markdown.mid(bodyStartPos, bodyEndPos - bodyStartPos);
	static const QRegularExpression re("^\n*|\n*$");
	textEng.replace(re, "");
	auto textTr = trans.tryQtranslate(textEng);
	if (textTr.isEmpty() && sectionName == "Constellations")
	{
		// Legacy way of translating the whole Constellations section didn't work.
		// Now try the correct way: split the section into descriptions of individual
		// constellations and translate each of them.
		const auto map = getConstellationsDescriptions(textEng);
		if (map.empty()) return textEng;
		const auto cMgr = GETSTELMODULE(ConstellationMgr);
		for (const auto& entry : map)
		{
			const auto consEngName = entry.first;
			const auto cons = cMgr->searchByName(consEngName);
			const auto consName = cons ? cons->getNameI18n() : consEngName;
			textTr += "<h5>" + consName + "</h5>\n";
			textTr += markdownToHTML(trans.qtranslate(entry.second.trimmed()));
		}
		return textTr;
	}

	if (textTr.isEmpty()) textTr = textEng;

	if (sectionName.trimmed() == "References")
	{
		static const QRegularExpression refRe("^ *- \\[#([0-9]+)\\]: (.*)$", QRegularExpression::MultilineOption);
		textTr.replace(refRe, "\\1. <span id=\"cite_\\1\">\\2</span>");
	}
	else
	{
		textTr = convertReferenceLinks(textTr);
	}

	if (sectionName.trimmed() == "License")
	{
		currentSkyCulture.license = textTr;
		return "";
	}

	return markdownToHTML(textTr);
}

QString StelSkyCultureMgr::descriptionMarkdownToHTML(const QString& markdownInput, const QString& descrPath)
{
	// Section names should be available for translation
	(void)NC_("Extras"      , "Name of a section in sky culture description");
	(void)NC_("References"  , "Name of a section in sky culture description");
	(void)NC_("Authors"     , "Name of a section in sky culture description");
	const StelTranslator& trans = StelApp::getInstance().getLocaleMgr().getSkyCultureDescriptionsTranslator();

	// Strip comments before translating
	static const QRegularExpression commentPat("<!--.*?-->");
	const auto markdown = QString(markdownInput).replace(commentPat, "");

	static const QRegularExpression headerPat("^# +(.+)$", QRegularExpression::MultilineOption);
	const auto match = headerPat.match(markdown);
	QString name;
	if (match.isValid())
	{
		name = match.captured(1);
	}
	else
	{
		qCritical().nospace() << "Failed to get sky culture name in file " << descrPath
		                      << ": got " << match.lastCapturedIndex() << " matches instead of 1";
		name = "Unknown";
	}

	QString text = "<h1>" + trans.qtranslate(name, "sky culture") + "</h1>";
	static const QRegularExpression sectionNamePat("^## +(.+)$", QRegularExpression::MultilineOption);
	QString prevSectionName;
	qsizetype prevBodyStartPos = -1;
	for (auto it = sectionNamePat.globalMatch(markdown); it.hasNext(); )
	{
		const auto match = it.next();
		const auto sectionName = match.captured(1);
		const auto nameStartPos = match.capturedStart(0);
		const auto bodyStartPos = match.capturedEnd(0);
		if (!prevSectionName.isEmpty())
		{
			const auto sectionText = convertMarkdownLevel2Section(markdown, prevSectionName, prevBodyStartPos, nameStartPos, trans);
			if(prevSectionName != "Introduction" && prevSectionName != "Description")
				text += "<h2>" + qc_(prevSectionName, "Name of a section in sky culture description") + "</h2>\n";
			if (!sectionText.isEmpty())
			{
				text += sectionText + "\n";
			}
		}
		prevBodyStartPos = bodyStartPos;
		prevSectionName = sectionName;
	}
	if (prevBodyStartPos >= 0)
	{
		const auto sectionText = convertMarkdownLevel2Section(markdown, prevSectionName, prevBodyStartPos, markdown.size(), trans);
		if (!sectionText.isEmpty())
		{
			text += "<h2>" + qc_(prevSectionName, "Name of a section in sky culture description") + "</h2>\n";
			text += sectionText;
		}
	}

	return text;
}

QString StelSkyCultureMgr::getCurrentSkyCultureHtmlDescription()
{
	QString lang = StelApp::getInstance().getLocaleMgr().getAppLanguage();
	if (!QString("pt_BR zh_CN zh_HK zh_TW").contains(lang))
	{
		lang = lang.split("_").at(0);
	}
	const QString descPath = currentSkyCulture.path + "/description.md";
	const bool pathExists = QFileInfo::exists(descPath);
	if (!pathExists)
		qWarning() << "Can't find description for skyculture" << currentSkyCulture.id;

	QString description;
	if (!pathExists)
	{
		description = QString("<h2>%1</2><p>%2</p>").arg(getCurrentSkyCultureNameI18(), q_("No description"));
	}
	else
	{
		QFile f(descPath);
		if(f.open(QIODevice::ReadOnly))
		{
			const auto markdown = QString::fromUtf8(f.readAll());
			description = descriptionMarkdownToHTML(markdown, descPath);
		}
		else
		{
			qWarning().nospace() << "Failed to open sky culture description file " << descPath << ": " << f.errorString();
		}
	}

	description.append(getCurrentSkyCultureHtmlLicense());
	description.append(getCurrentSkyCultureHtmlClassification());
	description.append(getCurrentSkyCultureHtmlRegion());


	return description;
}

QString StelSkyCultureMgr::directoryToSkyCultureEnglish(const QString& directory) const
{
	return dirToNameEnglish[directory].englishName;
}

QString StelSkyCultureMgr::directoryToSkyCultureI18(const QString& directory) const
{
	QString culture = dirToNameEnglish[directory].englishName;
	if (culture=="")
	{
		qWarning().nospace() << "StelSkyCultureMgr::directoryToSkyCultureI18("
		                     << QDir::toNativeSeparators(directory) << "): could not find directory";
		return "";
	}
	return q_(culture);
}

QString StelSkyCultureMgr::skyCultureI18ToDirectory(const QString& cultureName) const
{
	const StelTranslator& trans = StelApp::getInstance().getLocaleMgr().getSkyCultureDescriptionsTranslator();
	QMapIterator<QString, StelSkyCulture> i(dirToNameEnglish);
	while (i.hasNext())
	{
		i.next();
		if (trans.qtranslate(i.value().englishName, "sky culture") == cultureName)
			return i.key();
	}
	return "";
}

void StelSkyCultureMgr::setFlagOverrideUseCommonNames(bool override)
{
	flagOverrideUseCommonNames=override;
	emit flagOverrideUseCommonNamesChanged(override);
}

void StelSkyCultureMgr::setFlagUseAbbreviatedNames(bool b)
{
	flagUseAbbreviatedNames=b;
	StelApp::immediateSave("viewing/flag_constellation_abbreviations", b);
	emit flagUseAbbreviatedNamesChanged(b);
}

StelObject::CulturalDisplayStyle StelSkyCultureMgr::convertCulturalDisplayStyleFromCSVstring(const QString &csv)
{
	static const QMap<QString, StelObject::CulturalDisplayStyle> cdsEnumParts=
	{ {"none", StelObject::CulturalDisplayStyle::NONE},
	  {"modern", StelObject::CulturalDisplayStyle::Modern},
	  {"byname", StelObject::CulturalDisplayStyle::Byname},
	  {"ipa", StelObject::CulturalDisplayStyle::IPA},
	  {"translated", StelObject::CulturalDisplayStyle::Translated},
	  {"translit", StelObject::CulturalDisplayStyle::Translit},
	  {"pronounce", StelObject::CulturalDisplayStyle::Pronounce},
	  {"native", StelObject::CulturalDisplayStyle::Native}};

	StelObject::CulturalDisplayStyle styleEnum = StelObject::CulturalDisplayStyle::NONE;
	const QStringList styleParts=csv.split(",", SkipEmptyParts);

	for (const QString &part: styleParts)
	{
		styleEnum = static_cast<StelObject::CulturalDisplayStyle>( int(styleEnum) |  int(cdsEnumParts.value(part.trimmed().toLower(), StelObject::CulturalDisplayStyle::NONE)));
	}
	return styleEnum;
}

QString StelSkyCultureMgr::convertCulturalDisplayStyleToCSVstring(const StelObject::CulturalDisplayStyle style)
{
	return QVariant::fromValue(style).toString().replace('_', ',');
}


// Returns the screen labeling setting for the currently active skyculture
StelObject::CulturalDisplayStyle StelSkyCultureMgr::getScreenLabelStyle() const
{
	// This is needed for testing mode
	if (defaultSkyCultureID.isEmpty())
		return StelObject::CulturalDisplayStyle::Translated;

	static QSettings *conf=StelApp::getInstance().getSettings();
	QVariant val= conf->value(QString("SCScreenLabelStyle/%1").arg(getCurrentSkyCultureID()), "Translated");
	//qDebug() << "StelSkyCultureMgr::getScreenLabelStyle(): found " << val << "(" << val.toString() << ")";
	return convertCulturalDisplayStyleFromCSVstring(val.toString());
}
// Scripting version
QString StelSkyCultureMgr::getScreenLabelStyleString() const
{
	return convertCulturalDisplayStyleToCSVstring(getScreenLabelStyle());
}


// Sets the screen labeling setting for the currently active skyculture
void StelSkyCultureMgr::setScreenLabelStyle(const StelObject::CulturalDisplayStyle style)
{
	// This is needed for testing mode
	if (defaultSkyCultureID.isEmpty())
		return;

	static QSettings *conf=StelApp::getInstance().getSettings();
	conf->setValue(QString("SCScreenLabelStyle/%1").arg(getCurrentSkyCultureID()), convertCulturalDisplayStyleToCSVstring(style));
	//qInfo() << QString("SCScreenLabelStyle/%1=%2").arg(getCurrentSkyCultureID(), convertCulturalDisplayStyleToCSVstring(style));
	emit screenLabelStyleChanged(style);
}

// style can be the enum string like Native_IPA_Translated, or a comma-separated string like "Translated, native, IPA"
void StelSkyCultureMgr::setScreenLabelStyle(const QString &style)
{
	setScreenLabelStyle(convertCulturalDisplayStyleFromCSVstring(style));
}

// Returns the InfoString Labeling setting for the currently active skyculture
StelObject::CulturalDisplayStyle StelSkyCultureMgr::getInfoLabelStyle() const
{
	// This is needed for testing mode
	if (defaultSkyCultureID.isEmpty())
		return StelObject::CulturalDisplayStyle::Translated;

	static QSettings *conf=StelApp::getInstance().getSettings();
	QVariant val= conf->value(QString("SCInfoLabelStyle/%1").arg(getCurrentSkyCultureID()), "Translated");
	//qDebug() << "StelSkyCultureMgr::getInfoLabelStyle(): found " << val << "(" << val.toString() << ")";
	return convertCulturalDisplayStyleFromCSVstring(val.toString());
}
// Scripting version
QString StelSkyCultureMgr::getInfoLabelStyleString() const
{
	return convertCulturalDisplayStyleToCSVstring(getInfoLabelStyle());
}

// Sets the InfoString Labeling setting for the currently active skyculture
void StelSkyCultureMgr::setInfoLabelStyle(const StelObject::CulturalDisplayStyle style)
{
	// This is needed for testing mode
	if (defaultSkyCultureID.isEmpty())
		return;

	static QSettings *conf=StelApp::getInstance().getSettings();
	conf->setValue(QString("SCInfoLabelStyle/%1").arg(getCurrentSkyCultureID()), convertCulturalDisplayStyleToCSVstring(style));
	//qInfo() << QString("SCInfoLabelStyle/%1=%2").arg(getCurrentSkyCultureID(), convertCulturalDisplayStyleToCSVstring(style));
	emit infoLabelStyleChanged(style);
}

void StelSkyCultureMgr::setInfoLabelStyle(const QString &style)
{
	setInfoLabelStyle(convertCulturalDisplayStyleFromCSVstring(style));
}

// Returns the screen labeling setting for the currently active skyculture
StelObject::CulturalDisplayStyle StelSkyCultureMgr::getZodiacLabelStyle() const
{
	// This is needed for testing mode
	if (defaultSkyCultureID.isEmpty())
		return StelObject::CulturalDisplayStyle::Translated;

	static QSettings *conf=StelApp::getInstance().getSettings();
	QVariant val= conf->value(QString("SCZodiacLabelStyle/%1").arg(getCurrentSkyCultureID()), "Translated");
	//qDebug() << "StelSkyCultureMgr::getZodiacLabelStyle(): found " << val << "(" << val.toString() << ")";
	return convertCulturalDisplayStyleFromCSVstring(val.toString());
}
// Scripting version
QString StelSkyCultureMgr::getZodiacLabelStyleString() const
{
	return convertCulturalDisplayStyleToCSVstring(getZodiacLabelStyle());
}


// Sets the screen labeling setting for the currently active skyculture
void StelSkyCultureMgr::setZodiacLabelStyle(const StelObject::CulturalDisplayStyle style)
{
	// This is needed for testing mode
	if (defaultSkyCultureID.isEmpty())
		return;

	static QSettings *conf=StelApp::getInstance().getSettings();
	conf->setValue(QString("SCZodiacLabelStyle/%1").arg(getCurrentSkyCultureID()), convertCulturalDisplayStyleToCSVstring(style));
	//qInfo() << QString("SCZodiacLabelStyle/%1=%2").arg(getCurrentSkyCultureID(), convertCulturalDisplayStyleToCSVstring(style));
	emit zodiacLabelStyleChanged(style);
}

// style can be the enum string like Native_IPA_Translated, or a comma-separated string like "Translated, native, IPA"
void StelSkyCultureMgr::setZodiacLabelStyle(const QString &style)
{
	setZodiacLabelStyle(convertCulturalDisplayStyleFromCSVstring(style));
}

// Returns the lunar_system labeling setting for the currently active skyculture
StelObject::CulturalDisplayStyle StelSkyCultureMgr::getLunarSystemLabelStyle() const
{
	// This is needed for testing mode
	if (defaultSkyCultureID.isEmpty())
		return StelObject::CulturalDisplayStyle::Translated;

	static QSettings *conf=StelApp::getInstance().getSettings();
	QVariant val= conf->value(QString("SCLunarSystemLabelStyle/%1").arg(getCurrentSkyCultureID()), "Translated");
	//qInfo() << "StelSkyCultureMgr::getLunarSystemLabelStyle(): found " << val << "(" << val.toString() << ")";
	return convertCulturalDisplayStyleFromCSVstring(val.toString());
}
// Scripting version
QString StelSkyCultureMgr::getLunarSystemLabelStyleString() const
{
	return convertCulturalDisplayStyleToCSVstring(getLunarSystemLabelStyle());
}


// Sets the lunar_system labeling setting for the currently active skyculture
void StelSkyCultureMgr::setLunarSystemLabelStyle(const StelObject::CulturalDisplayStyle style)
{
	// This is needed for testing mode
	if (defaultSkyCultureID.isEmpty())
		return;

	static QSettings *conf=StelApp::getInstance().getSettings();
	conf->setValue(QString("SCLunarSystemLabelStyle/%1").arg(getCurrentSkyCultureID()), convertCulturalDisplayStyleToCSVstring(style));
	//qInfo() << QString("SCLunarSystemLabelStyle/%1=%2").arg(getCurrentSkyCultureID(), convertCulturalDisplayStyleToCSVstring(style));
	emit lunarSystemLabelStyleChanged(style);
}

// style can be the enum string like Native or Translated
void StelSkyCultureMgr::setLunarSystemLabelStyle(const QString &style)
{
	setLunarSystemLabelStyle(convertCulturalDisplayStyleFromCSVstring(style));
}



QString StelSkyCultureMgr::createCulturalLabel(const StelObject::CulturalName &cName,
					       const StelObject::CulturalDisplayStyle style,
					       const QString &commonNameI18n,
					       const QString &abbrevI18n) const
{
	// Each element may be in an RTL language (e.g. Arab). However,
	// - for most (left-to-right) languages we want a canonical order of left to right elements.
	// - for Arab and other right-to-left user languages, we set a canonical order of right-to-left elements.
	// This requires building Unicode isolation cells.
	// Unicode constants from Unicode Standard Annex #9, section 2.
	//static const QString LRE{"\u202a"}; // Left-to-right embedding: Treat following text as embedded left-to-right
	//static const QString RLE{"\u202b"}; // Right-to-left embedding: Treat following text as embedded right-to-left
	//static const QString LRO{"\u202d"}; // Left-to-right override: Force following characters to be treated as strong left-to-right chars.
	//static const QString RLO{"\u202e"}; // Right-to-left override: Force following characters to be treated as strong right-to-left chars.
	//static const QString PDF{"\u202c"}; // Pop directional formatting: terminate scope of last LRE/RLE/LRO/RLO
	//static const QString LRI{"\u2066"}; // left-to-right isolate: Treat following text as isolated and left-to-right
	//static const QString RLI{"\u2067"}; // right-to-left isolate: Treat following text as isolated and right-to-left
	static const QString FSI{"\u2068"}; // First strong isolate: Treat following text as isolated and in the direction of its first strong directional character.
					    // ASSUMPTION: Can be used as autodetect feature? Mark all parts inside embeddings?
	static const QString PDI{"\u2069"}; // Pop directional isolate: terminate scope of last LRI/RLI/FSI
	//static const QString LRM{"\u200e"}; // left-to-right mark: zero-width char
	//static const QString RLM{"\u200f"}; // right-to-left mark: right to left zero-width non-Arabic char
	//static const QString ALM{"\u061c"}; // right-to-left mark: right to left zero-width Arabic char
	static const QString ZWS{"\u200b"}; // zero-width space (we use them to combine cultural label groups)

	StelObject::CulturalName lName;
	// copy over filled elements from cName, but enclose each with Unicode isolation markers.
	if (!cName.native         .isEmpty()) lName.native          = FSI+cName.native+PDI;
	if (!cName.pronounce      .isEmpty()) lName.pronounce       = FSI+cName.pronounce+PDI;
	if (!cName.pronounceI18n  .isEmpty()) lName.pronounceI18n   = FSI+cName.pronounceI18n+PDI;
	if (!cName.transliteration.isEmpty()) lName.transliteration = FSI+cName.transliteration+PDI;
	if (!cName.translated     .isEmpty()) lName.translated      = FSI+cName.translated+PDI;
	if (!cName.translatedI18n .isEmpty()) lName.translatedI18n  = FSI+cName.translatedI18n+PDI;
	if (!cName.IPA            .isEmpty()) lName.IPA             = FSI+cName.IPA+PDI;
	if (!cName.byname         .isEmpty()) lName.byname          = FSI+cName.byname+PDI;
	if (!cName.bynameI18n     .isEmpty()) lName.bynameI18n      = FSI+cName.bynameI18n+PDI;
	const QString lCommonNameI18n=FSI+commonNameI18n+PDI;

	// At least while many fields have not been filled, we should create a few fallbacks
	// If native contains non-Latin glyphs, pronounce or transliteration is mandatory.
	QString pronounceStr=(lName.pronounceI18n.isEmpty() ? lName.pronounce : lName.pronounceI18n);
	QString nativeOrPronounce = (lName.native.isEmpty() ? lName.pronounceI18n : lName.native);
	QString pronounceOrNative = (pronounceStr.isEmpty() ? lName.native : pronounceStr);
	QString translitOrPronounce = (lName.transliteration.isEmpty() ? pronounceStr : lName.transliteration);

	// If you call this with an actual argument abbrevI18n, you really only want a short label.
	if (flagUseAbbreviatedNames && !abbrevI18n.isNull())
		return (abbrevI18n.startsWith('.') ? QString() : abbrevI18n);

	const int styleInt=int(style);
	QString label;
	switch (style)
	{
		case StelObject::CulturalDisplayStyle::Native: // native if available. fallback to pronounce and english entries
			return lName.native.isEmpty() ? (lName.pronounceI18n.isEmpty() ? lName.translatedI18n : lName.pronounceI18n) : lName.native;
		case StelObject::CulturalDisplayStyle::Pronounce: // pronounce if available. fallback to native
			return pronounceOrNative;
		case StelObject::CulturalDisplayStyle::Translit:
			return translitOrPronounce;
		case StelObject::CulturalDisplayStyle::Translated:
			return (lName.translatedI18n.isEmpty() ? (pronounceStr.isEmpty() ? lName.native : pronounceStr) : lName.translatedI18n);
		case StelObject::CulturalDisplayStyle::IPA: // really only IPA?
			return lName.IPA;
		case StelObject::CulturalDisplayStyle::NONE: // fully non-cultural!
		case StelObject::CulturalDisplayStyle::Modern:
			return lCommonNameI18n;
		case StelObject::CulturalDisplayStyle::Byname:
			return (lName.bynameI18n.isEmpty() ? (pronounceStr.isEmpty() ? lName.native : pronounceStr) : lName.bynameI18n);
		default:
			break;
	}
	// simple cases done. Now build-up label of form "primary [common transliteration aka pronounce, scientific transliteration] [IPA] (translation) <modern>"
	// (The first of the square brackets is formatted with "turtle brackets" so that only IPA is in regular square brackets.)
	// "primary" is usually either native or one of the reading aids, or translation or even modern when other options are switched off.
	// Rules:
	// Styles with Native_* start with just native, but we must fallback to other strings if native is empty.
	// Styles with Pronounce_* start with Pronounce or transliteration, but Pronounce_Translit_... must show Translit in turtle brackets when both exist.
	// Styles with Translit_* start with Transliteration or fallback to Pronounce
	// Styles with ...IPA... must add IPA (when exists) in square brackets, conditionally after the comma-separated turtle brackets with Pronounce and Transliteration
	// Styles with ...Translated have translation in brackets appended
	// Styles with ...Modern have the modern name (lCommonNameI18n) in slightly decorative curved angle brackets appended

	QStringList braced; // the contents of the secondary term, i.e. pronunciation and transliteration
	if (styleInt & int(StelObject::CulturalDisplayStyle::Native))
	{
		label=nativeOrPronounce+ZWS;
		// Add pronounciation and Translit in braces
		if (styleInt & int(StelObject::CulturalDisplayStyle::Pronounce))
			braced.append(pronounceStr);
		if (styleInt & int(StelObject::CulturalDisplayStyle::Translit))
			braced.append(lName.transliteration);
	}
	else // not including native
	{
		// Use the first valid of pronunciation or transliteration as main name (fallback to native), add the others in braces if applicable
		if (styleInt & int(StelObject::CulturalDisplayStyle::Pronounce))
		{
			label=pronounceOrNative+ZWS;
			if (styleInt & int(StelObject::CulturalDisplayStyle::Translit))
				braced.append(lName.transliteration);
		}

		else if (styleInt & int(StelObject::CulturalDisplayStyle::Translit))
		{
			label=translitOrPronounce+ZWS;
		}
	}

	braced.removeDuplicates();
	braced.removeOne(QString(""));
	braced.removeOne(QString());
	braced.removeOne(label); // avoid repeating the main thing if it was used as fallback!

	if (!braced.isEmpty())
	{
		QString pronTrans=QString(" %1%3%2").arg(QChar(0x2997), QChar(0x2998), braced.join(", "+ZWS));
			label.append(pronTrans+ZWS);
	}

	// Add IPA (where possible)
	if ((styleInt & int(StelObject::CulturalDisplayStyle::IPA)) && (!lName.IPA.isEmpty()) && (label != lName.IPA))
	{
		QString ipa=QString(" [%1]").arg(lName.IPA);
			label.append(ipa+ZWS);
	}

	// Add translation and optional byname in brackets

	QStringList bracketed;
	if ((styleInt & int(StelObject::CulturalDisplayStyle::Translated)) && (!lName.translatedI18n.isEmpty()))
	{
		if (label.isEmpty())
			label=lName.translatedI18n;
		else if (!label.startsWith(lName.translatedI18n, Qt::CaseInsensitive)) // seems useless to add translation into same string

			//label.append(QString(" (%1)").arg(lName.translatedI18n));
			bracketed.append(lName.translatedI18n+ZWS);
	}

	if ( (styleInt & int(StelObject::CulturalDisplayStyle::Byname)) && (!lName.bynameI18n.isEmpty()))
		bracketed.append(lName.bynameI18n+ZWS);
	if (!bracketed.isEmpty())
	{
		QString transBy=QString(" (%1)").arg(bracketed.join(", "+ZWS));
			label.append(transBy+ZWS);
	}


	// Add an explanatory modern name in decorative angle brackets
	if ((styleInt & int(StelObject::CulturalDisplayStyle::Modern)) && (!commonNameI18n.isEmpty()) && (!label.startsWith(lCommonNameI18n)) && (lCommonNameI18n!=lName.translatedI18n))
	{
		QString modern=QString(" %1%3%2").arg(QChar(0x29FC), QChar(0x29FD), lCommonNameI18n);
			label.append(modern+ZWS);
	}
	if ((styleInt & int(StelObject::CulturalDisplayStyle::Modern)) && label.isEmpty()) // if something went wrong?
		label=lCommonNameI18n;

	return label;
}

//! Returns whether current skyculture uses (incorporates) common names.
bool StelSkyCultureMgr::currentSkycultureUsesCommonNames() const
{
	return currentSkyCulture.fallbackToInternationalNames;
}

// Call this as scripting function. This shall provide information about unicode and QString properties
void StelSkyCultureMgr::analyzeScreenLabel() const
{
	static StelObjectMgr* omgr = GETSTELMODULE(StelObjectMgr);
	if (omgr->getSelectedObject().isEmpty())
		return;

	//StelObjectP obj = omgr->getSelectedObject()[0];
	StelObjectP obj = omgr->getLastSelectedObject();
	qDebug() << "Obj:" << obj->getObjectType() << obj->getEnglishName();
	QString label;
	// obj may be a star, then we need a StarWrapper...
	if (obj->getObjectType().contains("star"))
	{
		qDebug() << "star. searching deeper...";
		obj=GETSTELMODULE(StarMgr)->searchByName(obj->getEnglishName());
		static const QRegularExpression hpRx("^(HIP|HP)\\s*(\\d+)\\s*.*$", QRegularExpression::CaseInsensitiveOption);
		QRegularExpressionMatch match=hpRx.match(obj->getEnglishName());
		if (match.hasMatch())
		{
			bool ok;
			int hpNum = match.captured(2).toInt(&ok);
			if (ok)
			{
				label=GETSTELMODULE(StarMgr)->getCulturalScreenLabel(hpNum);
			}
		}
	}
	else
		label=obj->getScreenLabel();

	qDebug() << "Analyze label: " << label;
	std::u32string label32=label.toStdU32String();

#if (QT_VERSION < QT_VERSION_CHECK(6,0,0))
	QList<uint> label32l=label.toUcs4().toList();
#else
	QList<uint> label32l=label.toUcs4();
#endif
	// Unfortunately, QChar enums are not Q_ENUMs. The names are not available from the MetaObject.
	static const QMap <QChar::Category, QString> charCatMap = {
		{ QChar::Mark_NonSpacing         , " 0 Mn Mark_NonSpacing         " },
		{ QChar::Mark_SpacingCombining   , " 1 Mc Mark_SpacingCombining   " },
		{ QChar::Mark_Enclosing          , " 2 Me Mark_Enclosing          " },
		{ QChar::Number_DecimalDigit     , " 3 Nd Number_DecimalDigit     " },
		{ QChar::Number_Letter           , " 4 Nl Number_Letter           " },
		{ QChar::Number_Other            , " 5 No Number_Other            " },
		{ QChar::Separator_Space         , " 6 Zs Separator_Space         " },
		{ QChar::Separator_Line          , " 7 Zl Separator_Line          " },
		{ QChar::Separator_Paragraph     , " 8 Zp Separator_Paragraph     " },
		{ QChar::Other_Control           , " 9 Cc Other_Control           " },
		{ QChar::Other_Format            , "10 Cf Other_Format            " },
		{ QChar::Other_Surrogate         , "11 Cs Other_Surrogate         " },
		{ QChar::Other_PrivateUse        , "12 Co Other_PrivateUse        " },
		{ QChar::Other_NotAssigned       , "13 Cn Other_NotAssigned       " },
		{ QChar::Letter_Uppercase        , "14 Lu Letter_Uppercase        " },
		{ QChar::Letter_Lowercase        , "15 Ll Letter_Lowercase        " },
		{ QChar::Letter_Titlecase        , "16 Lt Letter_Titlecase        " },
		{ QChar::Letter_Modifier         , "17 Lm Letter_Modifier         " },
		{ QChar::Letter_Other            , "18 Lo Letter_Other            " },
		{ QChar::Punctuation_Connector   , "19 Pc Punctuation_Connector   " },
		{ QChar::Punctuation_Dash        , "20 Pd Punctuation_Dash        " },
		{ QChar::Punctuation_Open        , "21 Ps Punctuation_Open        " },
		{ QChar::Punctuation_Close       , "22 Pe Punctuation_Close       " },
		{ QChar::Punctuation_InitialQuote, "23 Pi Punctuation_InitialQuote" },
		{ QChar::Punctuation_FinalQuote  , "24 Pf Punctuation_FinalQuote  " },
		{ QChar::Punctuation_Other       , "25 Po Punctuation_Other       " },
		{ QChar::Symbol_Math             , "26 Sm Symbol_Math             " },
		{ QChar::Symbol_Currency         , "27 Sc Symbol_Currency         " },
		{ QChar::Symbol_Modifier         , "28 Sk Symbol_Modifier         " },
		{ QChar::Symbol_Other            , "29 So Symbol_Other            " }
	};
	static const QMap <QChar::Script, QString> charScriptMap = {
		{ QChar::Script_Unknown                 , "  0 Unknown" },   // For unassigned, private-use, noncharacter, and surrogate code points.
		{ QChar::Script_Inherited               , "  1 Inherited" }, // For characters that may be used with multiple scripts and that inherit their script from the preceding characters. These include nonspacing marks, enclosing marks, and zero width joiner/non-joiner characters.
		{ QChar::Script_Common                  , "  2 Common" },    // For characters that may be used with multiple scripts and that do not inherit their script from the preceding characters.
		{ QChar::Script_Adlam                   , "132 Adlam" },
		{ QChar::Script_Ahom                    , "126 Ahom" },
		{ QChar::Script_AnatolianHieroglyphs    , "127 AnatolianHieroglyphs" },
		{ QChar::Script_Arabic                  , "  8 Arabic" },
		{ QChar::Script_Armenian                , "  6 Armenian" },
		{ QChar::Script_Avestan                 , " 80 Avestan" },
		{ QChar::Script_Balinese                , " 62 Balinese" },
		{ QChar::Script_Bamum                   , " 84 Bamum" },
		{ QChar::Script_BassaVah                , "104 BassaVah" },
		{ QChar::Script_Batak                   , " 93 Batak" },
		{ QChar::Script_Bengali                 , " 12 Bengali" },
		{ QChar::Script_Bhaiksuki               , "133 Bhaiksuki" },
		{ QChar::Script_Bopomofo                , " 36 Bopomofo" },
		{ QChar::Script_Brahmi                  , " 94 Brahmi" },
		{ QChar::Script_Braille                 , " 54 Braille" },
		{ QChar::Script_Buginese                , " 55 Buginese" },
		{ QChar::Script_Buhid                   , " 44 Buhid" },
		{ QChar::Script_CanadianAboriginal      , " 29 CanadianAboriginal" },
		{ QChar::Script_Carian                  , " 75 Carian" },
		{ QChar::Script_CaucasianAlbanian       , "103 CaucasianAlbanian" },
		{ QChar::Script_Chakma                  , " 96 Chakma" },
		{ QChar::Script_Cham                    , " 77 Cham" },
		{ QChar::Script_Cherokee                , " 28 Cherokee" },
		{ QChar::Script_Coptic                  , " 46 Coptic" },
		{ QChar::Script_Cuneiform               , " 63 Cuneiform" },
		{ QChar::Script_Cypriot                 , " 53 Cypriot" },
		{ QChar::Script_Cyrillic                , "  5 Cyrillic" },
		{ QChar::Script_Deseret                 , " 41 Deseret" },
		{ QChar::Script_Devanagari              , " 11 Devanagari" },
		{ QChar::Script_Duployan                , "105 Duployan" },
		{ QChar::Script_EgyptianHieroglyphs     , " 81 EgyptianHieroglyphs" },
		{ QChar::Script_Elbasan                 , "106 Elbasan" },
		{ QChar::Script_Ethiopic                , " 27 Ethiopic" },
		{ QChar::Script_Georgian                , " 25 Georgian" },
		{ QChar::Script_Glagolitic              , " 57 Glagolitic" },
		{ QChar::Script_Gothic                  , " 40 Gothic" },
		{ QChar::Script_Grantha                 , "107 Grantha" },
		{ QChar::Script_Greek                   , "  4 Greek" },
		{ QChar::Script_Gujarati                , " 14 Gujarati" },
		{ QChar::Script_Gurmukhi                , " 13 Gurmukhi" },
		{ QChar::Script_Han                     , " 37 Han" },
		{ QChar::Script_Hangul                  , " 26 Hangul" },
		{ QChar::Script_Hanunoo                 , " 43 Hanunoo" },
		{ QChar::Script_Hatran                  , "128 Hatran" },
		{ QChar::Script_Hebrew                  , "  7 Hebrew" },
		{ QChar::Script_Hiragana                , " 34 Hiragana" },
		{ QChar::Script_ImperialAramaic         , " 87 ImperialAramaic" },
		{ QChar::Script_InscriptionalPahlavi    , " 90 InscriptionalPahlavi" },
		{ QChar::Script_InscriptionalParthian   , " 89 InscriptionalParthian" },
		{ QChar::Script_Javanese                , " 85 Javanese" },
		{ QChar::Script_Kaithi                  , " 92 Kaithi" },
		{ QChar::Script_Kannada                 , " 18 Kannada" },
		{ QChar::Script_Katakana                , " 35 Katakana" },
		{ QChar::Script_KayahLi                 , " 72 KayahLi" },
		{ QChar::Script_Kharoshthi              , " 61 Kharoshthi" },
		{ QChar::Script_Khmer                   , " 32 Khmer" },
		{ QChar::Script_Khojki                  , "109 Khojki" },
		{ QChar::Script_Khudawadi               , "123 Khudawadi" },
		{ QChar::Script_Lao                     , " 22 Lao" },
		{ QChar::Script_Latin                   , "  3 Latin" },
		{ QChar::Script_Lepcha                  , " 68 Lepcha" },
		{ QChar::Script_Limbu                   , " 47 Limbu" },
		{ QChar::Script_LinearA                 , "110 LinearA" },
		{ QChar::Script_LinearB                 , " 49 LinearB" },
		{ QChar::Script_Lisu                    , " 83 Lisu" },
		{ QChar::Script_Lycian                  , " 74 Lycian" },
		{ QChar::Script_Lydian                  , " 76 Lydian" },
		{ QChar::Script_Mahajani                , "111 Mahajani" },
		{ QChar::Script_Malayalam               , " 19 Malayalam" },
		{ QChar::Script_Mandaic                 , " 95 Mandaic" },
		{ QChar::Script_Manichaean              , "112 Manichaean" },
		{ QChar::Script_Marchen                 , "134 Marchen" },
		{ QChar::Script_MasaramGondi            , "138 MasaramGondi" },
		{ QChar::Script_MeeteiMayek             , " 86 MeeteiMayek" },
		{ QChar::Script_MendeKikakui            , "113 MendeKikakui" },
		{ QChar::Script_MeroiticCursive         , " 97 MeroiticCursive" },
		{ QChar::Script_MeroiticHieroglyphs     , " 98 MeroiticHieroglyphs" },
		{ QChar::Script_Miao                    , " 99 Miao" },
		{ QChar::Script_Modi                    , "114 Modi" },
		{ QChar::Script_Mongolian               , " 33 Mongolian" },
		{ QChar::Script_Mro                     , "115 Mro" },
		{ QChar::Script_Multani                 , "129 Multani" },
		{ QChar::Script_Myanmar                 , " 24 Myanmar" },
		{ QChar::Script_Nabataean               , "117 Nabataean" },
		{ QChar::Script_Newa                    , "135 Newa" },
		{ QChar::Script_NewTaiLue               , " 56 NewTaiLue" },
		{ QChar::Script_Nko                     , " 66 Nko" },
		{ QChar::Script_Nushu                   , "139 Nushu" },
		{ QChar::Script_Ogham                   , " 30 Ogham" },
		{ QChar::Script_OlChiki                 , " 69 OlChiki" },
		{ QChar::Script_OldHungarian            , "130 OldHungarian" },
		{ QChar::Script_OldItalic               , " 39 OldItalic" },
		{ QChar::Script_OldNorthArabian         , "116 OldNorthArabian" },
		{ QChar::Script_OldPermic               , "120 OldPermic" },
		{ QChar::Script_OldPersian              , " 60 OldPersian" },
		{ QChar::Script_OldSouthArabian         , " 88 OldSouthArabian" },
		{ QChar::Script_OldTurkic               , " 91 OldTurkic" },
		{ QChar::Script_Oriya                   , " 15 Oriya" },
		{ QChar::Script_Osage                   , "136 Osage" },
		{ QChar::Script_Osmanya                 , " 52 Osmanya" },
		{ QChar::Script_PahawhHmong             , "108 PahawhHmong" },
		{ QChar::Script_Palmyrene               , "118 Palmyrene" },
		{ QChar::Script_PauCinHau               , "119 PauCinHau" },
		{ QChar::Script_PhagsPa                 , " 65 PhagsPa" },
		{ QChar::Script_Phoenician              , " 64 Phoenician" },
		{ QChar::Script_PsalterPahlavi          , "121 PsalterPahlavi" },
		{ QChar::Script_Rejang                  , " 73 Rejang" },
		{ QChar::Script_Runic                   , " 31 Runic" },
		{ QChar::Script_Samaritan               , " 82 Samaritan" },
		{ QChar::Script_Saurashtra              , " 71 Saurashtra" },
		{ QChar::Script_Sharada                 , "100 Sharada" },
		{ QChar::Script_Shavian                 , " 51 Shavian" },
		{ QChar::Script_Siddham                 , "122 Siddham" },
		{ QChar::Script_SignWriting             , "131 SignWriting" },
		{ QChar::Script_Sinhala                 , " 20 Sinhala" },
		{ QChar::Script_SoraSompeng             , "101 SoraSompeng" },
		{ QChar::Script_Soyombo                 , "140 Soyombo" },
		{ QChar::Script_Sundanese               , " 67 Sundanese" },
		{ QChar::Script_SylotiNagri             , " 59 SylotiNagri" },
		{ QChar::Script_Syriac                  , "  9 Syriac" },
		{ QChar::Script_Tagalog                 , " 42 Tagalog" },
		{ QChar::Script_Tagbanwa                , " 45 Tagbanwa" },
		{ QChar::Script_TaiLe                   , " 48 TaiLe" },
		{ QChar::Script_TaiTham                 , " 78 TaiTham" },
		{ QChar::Script_TaiViet                 , " 79 TaiViet" },
		{ QChar::Script_Takri                   , "102 Takri" },
		{ QChar::Script_Tamil                   , " 16 Tamil" },
		{ QChar::Script_Tangut                  , "137 Tangut" },
		{ QChar::Script_Telugu                  , " 17 Telugu" },
		{ QChar::Script_Thaana                  , " 10 Thaana" },
		{ QChar::Script_Thai                    , " 21 Thai" },
		{ QChar::Script_Tibetan                 , " 23 Tibetan" },
		{ QChar::Script_Tifinagh                , " 58 Tifinagh" },
		{ QChar::Script_Tirhuta                 , "124 Tirhuta " },
		{ QChar::Script_Ugaritic                , " 50 Ugaritic" },
		{ QChar::Script_Vai                     , " 70 Vai" },
		{ QChar::Script_WarangCiti              , "125 WarangCiti" },
		{ QChar::Script_Yi                      , " 38 Yi" },
		{ QChar::Script_ZanabazarSquare         , "141 ZanabazarSquare" },

	#if (QT_VERSION>=QT_VERSION_CHECK(5,15,0))
		{ QChar::Script_KhitanSmallScript       , "155 KhitanSmallScript" },
		{ QChar::Script_Makasar                 , "145 Makasar" },
		{ QChar::Script_Medefaidrin             , "146 Medefaidrin" },
		{ QChar::Script_Nandinagari             , "150 Nandinagari" },
		{ QChar::Script_NyiakengPuachueHmong    , "151 NyiakengPuachueHmong" },
		{ QChar::Script_OldSogdian              , "147 OldSogdian" },
		{ QChar::Script_Sogdian                 , "148 Sogdian" },
		{ QChar::Script_Wancho                  , "152 Wancho" },
		{ QChar::Script_Yezidi                  , "156 Yezidi" },
	#endif
	#if (QT_VERSION>=QT_VERSION_CHECK(6,3,0))
		{ QChar::Script_CyproMinoan             , "157 CyproMinoan" },
		{ QChar::Script_OldUyghur               , "158 OldUyghur" },
		{ QChar::Script_Tangsa                  , "159 Tangsa" },
		{ QChar::Script_Toto                    , "160 Toto" },
		{ QChar::Script_Vithkuqi                , "161 Vithkuqi" },
	#endif
	#if (QT_VERSION>=QT_VERSION_CHECK(6,5,0))
		{ QChar::Script_NagMundari              , "163 NagMundari" }, // Qt>=6.3, but CI fails.
		{ QChar::Script_Kawi                    , "162 Kawi" }
	#endif
	};
	static const QMap <QChar::Direction, QString> charDirMap = {
		{ QChar::DirL    , " 0 DirL  " },
		{ QChar::DirR    , " 1 DirR  " },
		{ QChar::DirEN   , " 2 DirEN " },
		{ QChar::DirES   , " 3 DirES " },
		{ QChar::DirET   , " 4 DirET " },
		{ QChar::DirAN   , " 5 DirAN " },
		{ QChar::DirCS   , " 6 DirCS " },
		{ QChar::DirB    , " 7 DirB  " },
		{ QChar::DirS    , " 8 DirS  " },
		{ QChar::DirWS   , " 9 DirWS " },
		{ QChar::DirON   , "10 DirON " },
		{ QChar::DirLRE  , "11 DirLRE" },
		{ QChar::DirLRO  , "12 DirLRO" },
		{ QChar::DirAL   , "13 DirAL " },
		{ QChar::DirRLE  , "14 DirRLE" },
		{ QChar::DirRLO  , "15 DirRLO" },
		{ QChar::DirPDF  , "16 DirPDF" },
		{ QChar::DirNSM  , "17 DirNSM" },
		{ QChar::DirBN   , "18 DirBN " },
		{ QChar::DirLRI  , "19 DirLRI" },
		{ QChar::DirRLI  , "20 DirRLI" },
		{ QChar::DirFSI  , "21 DirFSI" },
		{ QChar::DirPDI  , "22 DirPDI" }
	};
	static const QMap <QChar::Decomposition, QString> charDecompositionMap = {

		{ QChar::NoDecomposition, " 0 NoDecomposition" },
		{ QChar::Canonical      , " 1 Canonical      " },
		{ QChar::Circle         , " 8 Circle         " },
		{ QChar::Compat         , "16 Compat         " },
		{ QChar::Final          , " 6 Final          " },
		{ QChar::Font           , " 2 Font           " },
		{ QChar::Fraction       , "17 Fraction       " },
		{ QChar::Initial        , " 4 Initial        " },
		{ QChar::Isolated       , " 7 Isolated       " },
		{ QChar::Medial         , " 5 Medial         " },
		{ QChar::Narrow         , "13 Narrow         " },
		{ QChar::NoBreak        , " 3 NoBreak        " },
		{ QChar::Small          , "14 Small          " },
		{ QChar::Square         , "15 Square         " },
		{ QChar::Sub            , "10 Sub            " },
		{ QChar::Super          , " 9 Super          " },
		{ QChar::Vertical       , "11 Vertical       " },
		{ QChar::Wide           , "12 QChar::Wide    " }
	};

	foreach(const uint letter, label32l )
	{
		qDebug().noquote() << QChar::digitValue(letter) << "(u" << QString::number(letter, 16)
		      #if (QT_VERSION >= QT_VERSION_CHECK(6,0,0))
				   << "/" << QChar::fromUcs4(letter)
		      #endif
				   << ")"
			    "\tcat." << charCatMap.value(QChar::category(letter), "UNK") <<
			    "scr." << charScriptMap.value(QChar::script(letter), "UNK") <<
			    "\tdir." << charDirMap.value(QChar::direction(letter), "UNK") <<
			    "decomp." <<  charDecompositionMap.value(QChar::decompositionTag(letter), "UNK");
	}
}

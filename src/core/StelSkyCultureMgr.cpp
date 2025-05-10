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
#include "StarMgr.hpp"
#include "StelFileMgr.hpp"
#include "StelTranslator.hpp"
#include "StelLocaleMgr.hpp"
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

StelSkyCultureMgr::StelSkyCultureMgr()
{
	setObjectName("StelSkyCultureMgr");
	makeCulturesList();
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
		culture.fallbackToInternationalNames = data["fallback_to_international_names"].toBool();
		culture.names = data["common_names"].toObject();

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

int StelSkyCultureMgr::getCurrentSkyCultureClassificationIdx() const
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

//! returns newline delimited list of human readable culture names in english
QString StelSkyCultureMgr::getSkyCultureListEnglish(void) const
{
	QString cultures;
	QMapIterator<QString, StelSkyCulture> i(dirToNameEnglish);
	while(i.hasNext())
	{
		i.next();
		cultures += QString("%1\n").arg(i.value().englishName);
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

QString StelSkyCultureMgr::convertMarkdownLevel2Section(const QString& markdown, const QString& sectionName,
                                                        const qsizetype bodyStartPos, const qsizetype bodyEndPos,
                                                        const StelTranslator& trans)
{
	auto text = markdown.mid(bodyStartPos, bodyEndPos - bodyStartPos);
	static const QRegularExpression re("^\n*|\n*$");
	text.replace(re, "");
	text = trans.qtranslate(text);

	if (sectionName.trimmed() == "References")
	{
		static const QRegularExpression refRe("^ *- \\[#([0-9]+)\\]: (.*)$", QRegularExpression::MultilineOption);
		text.replace(refRe, "\\1. <span id=\"cite_\\1\">\\2</span>");
	}
	else
	{
		text = convertReferenceLinks(text);
	}

	if (sectionName.trimmed() == "License")
	{
		currentSkyCulture.license = text;
		return "";
	}

	return markdownToHTML(text);
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


struct hullEntry
{
	StelObjectP obj;
	double x;
	double y;
};
// simple function only for ordering from Sedgewick 1990, Algorithms in C (p.353) used for sorting.
// The result fulfills the same purpose as some atan2, but with simpler operations.
static double theta(const hullEntry &p1, const hullEntry &p2)
{
	const double dx = p2.x-p1.x;
	const double ax = abs(dx);
	const double dy = p2.y-p1.y;
	const double ay = abs(dy);
	const double asum = ax+ay;
	double t = (asum==0) ? 0.0 : dy/asum;
	if (dx<0.)
		t=2-t;
	else if (dy<0.)
		t=4+t;
	return t*M_PI_2;
}

SphericalRegionP StelSkyCultureMgr::makeConvexHull(const std::vector<StelObjectP> &starLines, const std::vector<StelObjectP> &hullExtension, const std::vector<Vec3d> &darkLines, const Vec3d projectionCenter, const double hullRadius)
{
	static StelCore *core=StelApp::getInstance().getCore();
	// 1. Project every first star of a line pair (or just coordinates from a dark constellation) into a 2D tangential plane around projectionCenter.
	// Stars more than 90° from center cannot be projected of course and are skipped.
	double raC, deC;
	StelUtils::rectToSphe(&raC, &deC, projectionCenter);

	// starLines contains pairs of vertices, and some stars (or DSO) occur more than twice.
	QStringList idList;
	QList<StelObjectP>uniqueObjectList;
	foreach(auto &obj, starLines)
	{
		if (!idList.contains(obj->getID()))
		{
			// Take this star into consideration. However, the "star"s may be pointers to the same star, we must compare IDs.
			idList.append(obj->getID());
			uniqueObjectList.append(obj);
		}
	}
	foreach(auto &obj, hullExtension)
	{
		if (!idList.contains(obj->getID()))
		{
			// Take this star into consideration. However, the "star"s may be pointers to the same star, we must compare IDs.
			idList.append(obj->getID());
			uniqueObjectList.append(obj);
		}
	}

	QList<hullEntry> hullList;
	// Perspective (gnomonic) projection from Snyder 1987, Map Projections: A Working Manual (USGS).
	foreach(auto &obj, uniqueObjectList)
	{
		hullEntry entry;
		entry.obj=obj;
		double ra, de;
		StelUtils::rectToSphe(&ra, &de, entry.obj->getJ2000EquatorialPos(core));
		const double cosC=sin(deC)*sin(de) + cos(deC)*cos(de)*cos(ra-raC);
		if (cosC<=0.) // distance 90° or more from projection center? Discard!
		{
			qWarning() << "Cannot include object" << entry.obj->getID() <<  "in convex hull: too far from projection center.";
			continue;
		}
		const double kP=1./cosC;
		entry.x = -kP*cos(de)*sin(ra-raC); // x must be negative here.
		entry.y =  kP*(cos(deC)*sin(de)-sin(deC)*cos(de)*cos(ra-raC));
		hullList.append(entry);
		//qDebug().noquote().nospace() << "[ " << entry.x << " " << entry.y << " " << ra*M_180_PI/15 << " " << de*M_180_PI << " (" << entry.obj->getID() << ") ]"; // allows Postscript graphics, looks OK.
	}
	//qDebug() << "Hull candidates: " << hullList.length();
	if (hullList.count() < 3)
	{
		//qDebug() << "List length" << hullList.count() << " not enough for a convex hull... create circular area";
		SphericalRegionP res;
		switch (hullList.count())
		{
			case 0:
				res = new EmptySphericalRegion;
				break;
			case 1:
				res = new SphericalCap(projectionCenter, cos(hullRadius*M_PI_180));
				break;
			case 2:
				double halfDist=0.5*acos(hullList.at(0).obj->getJ2000EquatorialPos(core).dot(hullList.at(1).obj->getJ2000EquatorialPos(core)));
				res = new SphericalCap(projectionCenter, cos(halfDist+hullRadius*M_PI_180));
				break;
		}
		return res;
	}

	// 2. Apply Package Wrapping from Sedgewick 1990, Algorithms in C to find the outer points wrapping all points.
	// find minY
	int min=0;
	for(int i=1; i<hullList.count(); ++i)
	{
		const hullEntry &entry=hullList.at(i);
		if (entry.y<hullList.at(min).y)
			min=i;
	}
	//qDebug() << "min entry is " << hullList.at(min).obj->getID();

	const int N=hullList.count(); // N...number of unique stars in constellation.
	//qDebug() << "unique stars N=" << N;
	hullList.append(hullList.at(min));

	//QStringList debugList;
	//// DUMP HULL LINE
	//for(int i=0; i<hullList.count(); ++i)
	//{
	//	const hullEntry &entry=hullList.at(i);
	//	debugList << entry.obj->getID();
	//}
	//qDebug() << "Hull candidate: " << debugList.join(" - ");
	//debugList.clear();

	int M=0;
	double th=0.0;
	for (M=0; M<N; ++M)
	{
		hullList.swapItemsAt(M, min);

		//// DUMP HULL LINE
		//for(int i=0; i<hullList.count(); ++i)
		//{
		//	const hullEntry &entry=hullList.at(i);
		//	debugList << entry.obj->getID();
		//	if (i==M)
		//		debugList << "|";
		//}
		//qDebug() << "Hull candidate after swap at M=" << M << debugList.join(" - ");
		//debugList.clear();

		min=N; double v=th; th=M_PI*2.0;
		for (int i=M+1; i<=N; ++i)
		{
			//qDebug() << "From M:" << M << "(" << hullList.at(M).obj->getID() << ") to i: " << i << "=" << hullList.at(i).obj->getID() << ": th=" << theta(hullList.at(M), hullList.at(i)) * M_180_PI ;

			if (theta(hullList.at(M), hullList.at(i)) > v)
				if(theta(hullList.at(M), hullList.at(i))< th)
				{
					min=i;
					th=theta(hullList.at(M), hullList.at(min));
					//qDebug() << "min:" << min << "th:" << th * M_180_PI;
				}
		}

		if (min==N)
		{
			//qDebug().nospace() << "min==N=" << N << ", we're done sorting. Hull should be of length M=" << M;
			break; // now M+1 is holds number of "valid" hull points.
		}
	}
	++M;
	//qDebug() << "Hull length" << M << "of" << hullList.count();
#if (QT_VERSION<QT_VERSION_CHECK(6,0,0))
	for (int e=0; e<=N-M; ++e) hullList.pop_back();
#else
	hullList.remove(M, N-M);
#endif
	//hullList.remove(M-1, N-M+1);

	//// DUMP HULL LINE
	//for(int i=0; i<hullList.count(); ++i)
	//{
	//	const hullEntry &entry=hullList.at(i);
	//	debugList << entry.obj->getID();
	//}
	//qDebug() << "Final Hull, M=" << M << debugList.join(" - ");
	//debugList.clear();

	//Now create a SphericalRegion
	QList<Vec3d> hullPoints;
	foreach(const hullEntry &entry, hullList)
	{
		Vec3d pos=entry.obj->getJ2000EquatorialPos(core);
		hullPoints.append(pos);
	}
#if (QT_VERSION<QT_VERSION_CHECK(6,0,0))
	SphericalPolygon *hull=new SphericalPolygon(hullPoints.toVector());
#else
	SphericalPolygon *hull=new SphericalPolygon(hullPoints);
#endif
	//qDebug() << "Successful hull:" << hull->toJSON();
	return hull;
}

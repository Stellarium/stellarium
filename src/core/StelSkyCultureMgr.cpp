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
#include "StelFileMgr.hpp"
#include "StelTranslator.hpp"
#include "StelLocaleMgr.hpp"
#include "StelApp.hpp"
#include "StelIniParser.hpp"

#include <QSettings>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QDebug>
#include <QMap>
#include <QMapIterator>
#include <QDir>
#include <QRegularExpression>

StelSkyCultureMgr::StelSkyCultureMgr()
{
	setObjectName("StelSkyCultureMgr");

	QSet<QString> cultureDirNames = StelFileMgr::listContents("skycultures",StelFileMgr::Directory);
	
	for (const auto& dir : std::as_const(cultureDirNames))
	{
		QString pdFile = StelFileMgr::findFile("skycultures/" + dir + "/info.ini");
		if (pdFile.isEmpty())
		{
			qWarning() << "WARNING: unable to successfully read info.ini file from skyculture dir" << QDir::toNativeSeparators(dir);
			continue;
		}
		QSettings pd(pdFile, StelIniFormat);
		dirToNameEnglish[dir].englishName = pd.value("info/name").toString();
		dirToNameEnglish[dir].author = pd.value("info/author").toString();
		dirToNameEnglish[dir].credit = pd.value("info/credit").toString();		
		dirToNameEnglish[dir].license = pd.value("info/license", "").toString();
		dirToNameEnglish[dir].region = pd.value("info/region", "").toString();
		QString boundariesStr = pd.value("info/boundaries", "none").toString();
		static const QMap<QString, StelSkyCulture::BoundariesType> boundariesMap = {
			{ "none",    StelSkyCulture::BoundariesType::None},
			{ "iau",     StelSkyCulture::BoundariesType::IAU},
			{ "generic", StelSkyCulture::BoundariesType::IAU}, // deprecated, add warning below
			{ "own",     StelSkyCulture::BoundariesType::Own},
		};
		const auto boundariesType = boundariesMap.value(boundariesStr.toLower(), StelSkyCulture::BoundariesType::None);
		if (boundariesStr.contains("generic", Qt::CaseInsensitive))
		{
			qDebug() << "Skyculture " << dir << "'s boundaries is given with deprecated 'generic'. Please edit info.ini and change to 'iau'";
		}
		else if (!boundariesMap.contains(boundariesStr.toLower()))
		{
			qDebug() << "Skyculture " << dir << "'s boundaries value unknown:" << boundariesStr;
			qDebug() << "Please edit info.ini and change to a supported value. For now, this equals 'none'";
		}
		dirToNameEnglish[dir].boundariesType = boundariesType;
		// Use 'traditional' as default
		QString classificationStr = pd.value("info/classification", "traditional").toString();
		static const QMap <QString, StelSkyCulture::CLASSIFICATION>classificationMap={
			{ "traditional",  StelSkyCulture::TRADITIONAL},
			{ "historical",   StelSkyCulture::HISTORICAL},
			{ "ethnographic", StelSkyCulture::ETHNOGRAPHIC},
			{ "single",       StelSkyCulture::SINGLE},
			{ "comparative",  StelSkyCulture::COMPARATIVE},
			{ "personal",     StelSkyCulture::PERSONAL},			
			{ "incomplete",   StelSkyCulture::INCOMPLETE},
		};
		StelSkyCulture::CLASSIFICATION classification=classificationMap.value(classificationStr.toLower(), StelSkyCulture::INCOMPLETE);
		if (classificationMap.constFind(classificationStr.toLower()) == classificationMap.constEnd()) // not included
		{
			qDebug() << "Skyculture " << dir << "has UNKNOWN classification: " << classificationStr;
			qDebug() << "Please edit info.ini and change to a supported value. For now, this equals 'incomplete'";
		}
		dirToNameEnglish[dir].classification = classification;
	}	
}

StelSkyCultureMgr::~StelSkyCultureMgr()
{
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
	emit currentSkyCultureIDChanged(currentSkyCultureDir);
}

//! Set the current sky culture from the passed directory
bool StelSkyCultureMgr::setCurrentSkyCultureID(const QString& cultureDir)
{
	//prevent unnecessary changes
	if(cultureDir==currentSkyCultureDir)
		return false;

	QString scID = cultureDir;
	bool result = true;
	// make sure culture definition exists before attempting or will die
	if (directoryToSkyCultureEnglish(cultureDir) == "")
	{
		qWarning() << "Invalid sky culture directory: " << QDir::toNativeSeparators(cultureDir);
		scID = "modern";
		result = false;
	}

	currentSkyCultureDir = scID;
	currentSkyCulture = dirToNameEnglish[scID];

	emit currentSkyCultureIDChanged(currentSkyCultureDir);
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
	return qc_(currentSkyCulture.englishName, "sky culture");
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

QString StelSkyCultureMgr::getCurrentSkyCultureHtmlLicense() const
{
	QString description, color, license = currentSkyCulture.license.trimmed();

	if (license.isEmpty()) // License is not defined
	{
		color = "#2090ff"; // "blue" area
		license = q_("unknown");
		description = q_("This sky culture is provided under unknown license. Please ask authors for details about license for this sky culture.");
	}
	else
	{
		if (license.contains("GPL", Qt::CaseSensitive))
		{
			color = "#33ff33"; // "green" area; free license
			description = q_("This sky culture is provided under GNU General Public License. You can use it for commercial and non-commercial purposes, freely adapt it and share adapted work.");
		}
		else if (license.contains("MIT", Qt::CaseSensitive))
		{
			color = "#33ff33"; // "green" area; free license
			description = q_("This sky culture is provided under MIT License. You can use it for commercial and non-commercial purposes, freely adapt it and share adapted work.");
		}
		else if (license.startsWith("CC", Qt::CaseSensitive) || license.contains("Creative Commons", Qt::CaseInsensitive))
		{
			description = q_("This sky culture is provided under Creative Commons License.");
			#if (QT_VERSION>=QT_VERSION_CHECK(5, 14, 0))
			QStringList details = license.split(" ", Qt::SkipEmptyParts);
			#else
			QStringList details = license.split(" ", QString::SkipEmptyParts);
			#endif

			const QMap<QString, QString>options = {
				{ "BY",       q_("You may distribute, remix, adapt, and build upon this sky culture, even commercially, as long as you credit authors for the original creation.") },
				{ "BY-SA",    q_("You may remix, adapt, and build upon this sky culture even for commercial purposes, as long as you credit authors and license the new creations under the identical terms. This license is often compared to “copyleft” free and open source software licenses.") },
				{ "BY-ND",    q_("You may reuse this sky culture for any purpose, including commercially; however, adapted work cannot be shared with others, and credit must be provided by you.") },
				{ "BY-NC",    q_("You may remix, adapt, and build upon this sky culture non-commercially, and although your new works must also acknowledge authors and be non-commercial, you don’t have to license your derivative works on the same terms.") },
				{ "BY-NC-SA", q_("You may remix, adapt, and build upon this sky culture non-commercially, as long as you credit authors and license your new creations under the identical terms.") },
				{ "BY-NC-ND", q_("You may use this sky culture and share them with others as long as you credit authors, but you can’t change it in any way or use it commercially.") },
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

		if (!currentSkyCulture.credit.isEmpty())
		{
			// TRANSLATORS: A phrase like "Please credit: XY Museum". The creditee is in a skyculture's info.ini.
			description.append(QString(" %1 %2.").arg(q_("Please credit:"), currentSkyCulture.credit));
		}

		if (license.contains("FAL", Qt::CaseSensitive) || license.contains("Free Art License", Qt::CaseSensitive))
			description.append(QString(" %1").arg(q_("Illustrations are provided under Free Art License that grants the right to freely copy, distribute, and transform.")));
	}

	QString html = QString();
	if (!description.isEmpty()) // additional info for sky culture (metainfo): let's use italic
		html = QString("<dl><dt><span style='color:%4;'>%5</span> <strong>%1: %2</strong></dt><dd><em>%3</em></dd></dl>").arg(q_("License"), license, description, color, QChar(0x25CF));

	return html;
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
	QStringList cultures;
	QMapIterator<QString, StelSkyCulture> i(dirToNameEnglish);
	while (i.hasNext())
	{
		i.next();
		cultures += qc_(i.value().englishName, "sky culture");
	}
	// Sort for GUI use. Note that e.g. German Umlauts are sorted after Z. TODO: Fix this!
	cultures.sort(Qt::CaseInsensitive);
	return cultures;
}

QStringList StelSkyCultureMgr::getSkyCultureListIDs(void) const
{
	return dirToNameEnglish.keys();
}

QString StelSkyCultureMgr::getCurrentSkyCultureHtmlDescription() const
{
	QString skyCultureId = getCurrentSkyCultureID();
	QString lang = StelApp::getInstance().getLocaleMgr().getAppLanguage();
	if (!QString("pt_BR zh_CN zh_HK zh_TW").contains(lang))
	{
		lang = lang.split("_").at(0);
	}
	QString descPath = StelFileMgr::findFile("skycultures/" + skyCultureId + "/description."+lang+".utf8");
	if (descPath.isEmpty())
	{
		descPath = StelFileMgr::findFile("skycultures/" + skyCultureId + "/description.en.utf8");
		if (descPath.isEmpty())
			qWarning() << "WARNING: can't find description for skyculture" << skyCultureId;
	}

	QString description;
	if (descPath.isEmpty())
	{
		description = QString("<h2>%1</2><p>%2</p>").arg(getCurrentSkyCultureNameI18(), q_("No description"));
	}
	else
	{
		QFile f(descPath);
		if(f.open(QIODevice::ReadOnly))
		{
			description = QString::fromUtf8(f.readAll());
			f.close();
		}
	}

	description.append(getCurrentSkyCultureHtmlReferences());
	description.append(getCurrentSkyCultureHtmlLicense());
	description.append(getCurrentSkyCultureHtmlClassification());
	description.append(getCurrentSkyCultureHtmlRegion());

	return description;
}

QString StelSkyCultureMgr::getCurrentSkyCultureHtmlReferences() const
{
	QString reference = "";
	QString referencePath = StelFileMgr::findFile("skycultures/" + getCurrentSkyCultureID() + "/reference.fab");
	if (!referencePath.isEmpty())
	{
		QFile refFile(referencePath);
		if (!refFile.open(QIODevice::ReadOnly | QIODevice::Text))
		{
			qWarning() << "WARNING - could not open" << QDir::toNativeSeparators(referencePath);
			return reference;
		}
		QString record;
		// Allow empty and comment lines where first char (after optional blanks) is #
		static const QRegularExpression commentRx("^(\\s*#.*|\\s*)$");
		reference = QString("<h2>%1</h2><ol>").arg(q_("References / Further Reading"));
		int totalRecords=0;
		int readOk=0;
		int lineNumber=0;
		while(!refFile.atEnd())
		{
			record = QString::fromUtf8(refFile.readLine()).trimmed();
			lineNumber++;
			if (commentRx.match(record).hasMatch())
				continue;

			totalRecords++;
			static const QRegularExpression refRx("\\|");
			#if (QT_VERSION>=QT_VERSION_CHECK(5, 14, 0))
			QStringList ref = record.split(refRx, Qt::KeepEmptyParts);
			#else
			QStringList ref = record.split(refRx, QString::KeepEmptyParts);
			#endif
			// 1 - URID; 2 - Reference; 3 - URL (optional)
			if (ref.count()<2)
				qWarning() << "ERROR - cannot parse record at line" << lineNumber << "in references file" << QDir::toNativeSeparators(referencePath);
			else if (ref.count()<3)
			{
				qWarning() << "WARNING - record at line" << lineNumber << "in references file" << QDir::toNativeSeparators(referencePath) << " has wrong format (RefID: " << ref.at(0) << ")! Let's use fallback mode...";
				reference.append(QString("<li>%1</li>").arg(ref.at(1)));
				readOk++;
			}
			else
			{
				if (ref.at(2).isEmpty())
					reference.append(QString("<li>%1</li>").arg(ref.at(1)));
				else
					reference.append(QString("<li><a href='%2' class='external text' rel='nofollow'>%1</a></li>").arg(ref.at(1), ref.at(2)));
				readOk++;
			}
		}
		refFile.close();
		reference.append("</ol>");
		qDebug() << "Loaded" << readOk << "/" << totalRecords << "references";
	}
	return reference;
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
		qWarning() << "WARNING: StelSkyCultureMgr::directoryToSkyCultureI18(\""
			   << QDir::toNativeSeparators(directory) << "\"): could not find directory";
		return "";
	}
	return q_(culture);
}

QString StelSkyCultureMgr::skyCultureI18ToDirectory(const QString& cultureName) const
{
	QMapIterator<QString, StelSkyCulture> i(dirToNameEnglish);
	while (i.hasNext())
	{
		i.next();
		if (qc_(i.value().englishName, "sky culture") == cultureName)
			return i.key();
	}
	return "";
}

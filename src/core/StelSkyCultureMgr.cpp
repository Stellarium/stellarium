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

StelSkyCultureMgr::StelSkyCultureMgr()
{
	setObjectName("StelSkyCultureMgr");

	QSet<QString> cultureDirNames = StelFileMgr::listContents("skycultures",StelFileMgr::Directory);
	
	for (const auto& dir : cultureDirNames)
	{
		QString pdFile = StelFileMgr::findFile("skycultures/" + dir + "/info.ini");
		if (pdFile.isEmpty())
		{
			qWarning() << "WARNING: unable to successfully read info.ini file from skyculture dir" << QDir::toNativeSeparators(dir);
			return;
		}
		QSettings pd(pdFile, StelIniFormat);
		dirToNameEnglish[dir].englishName = pd.value("info/name").toString();
		dirToNameEnglish[dir].author = pd.value("info/author").toString();
		// TODO: Define license info (+separate license info for artwork?) and use it in description of skyculture like for plugins and scripts
		dirToNameEnglish[dir].license = pd.value("info/license", "").toString();
		QString boundariesStr = pd.value("info/boundaries", "none").toString();
		static const QMap<QString, StelSkyCulture::BOUNDARIES>boundariesMap={
			{ "none",    StelSkyCulture::NONE},
			{ "iau",     StelSkyCulture::IAU},
			{ "generic", StelSkyCulture::IAU}, // deprecated, add warning below
			{ "own",     StelSkyCulture::OWN},
		};
		StelSkyCulture::BOUNDARIES boundaries = boundariesMap.value(boundariesStr.toLower(), StelSkyCulture::NONE);
		if (boundariesStr.contains("generic", Qt::CaseInsensitive))
		{
			qDebug() << "Skyculture " << dir << "'s boundaries is given with deprecated 'generic'. Please edit info.ini and change to 'iau'";
		}
		else if (!boundariesMap.contains(boundariesStr.toLower()))
			{
				qDebug() << "Skyculture " << dir << "'s boundaries value unknown:" << boundariesStr;
				qDebug() << "Please edit info.ini and change to a supported value. For now, this equals 'none'";
			}
		dirToNameEnglish[dir].boundaries = boundaries;
		// Use 'traditional' as default
		QString classificationStr = pd.value("info/classification", "traditional").toString();
		static const QMap <QString, StelSkyCulture::CLASSIFICATION>classificationMap={
			{ "traditional",  StelSkyCulture::TRADITIONAL},
			{ "historical",   StelSkyCulture::HISTORICAL},
			{ "ethnographic", StelSkyCulture::ETHNOGRAPHIC},
			{ "single",       StelSkyCulture::SINGLE},
			{ "personal",     StelSkyCulture::PERSONAL},			
			{ "incomplete",   StelSkyCulture::INCOMPLETE},
		};
		StelSkyCulture::CLASSIFICATION classification=classificationMap.value(classificationStr.toLower(), StelSkyCulture::INCOMPLETE);
		if (!classificationMap.keys().contains(classificationStr.toLower()))
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
	defaultSkyCultureID = StelApp::getInstance().getSettings()->value("localization/sky_culture", "western").toString();
	setCurrentSkyCultureID(defaultSkyCultureID);
}

//! Set the current sky culture from the passed directory
bool StelSkyCultureMgr::setCurrentSkyCultureID(const QString& cultureDir)
{
	//prevent unnecessary changes
	if(cultureDir==currentSkyCultureDir)
		return false;

	// make sure culture definition exists before attempting or will die
	if (directoryToSkyCultureEnglish(cultureDir) == "")
	{
		qWarning() << "Invalid sky culture directory: " << QDir::toNativeSeparators(cultureDir);
		return false;
	}
	currentSkyCultureDir = cultureDir;
	currentSkyCulture = dirToNameEnglish[cultureDir];

	emit currentSkyCultureChanged(currentSkyCultureDir);
	return true;
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

	emit defaultSkyCultureChanged(id);
	return true;
}
	
QString StelSkyCultureMgr::getCurrentSkyCultureNameI18() const
{
	return q_(currentSkyCulture.englishName);
}

QString StelSkyCultureMgr::getCurrentSkyCultureEnglishName() const
{
	return currentSkyCulture.englishName;
}

int StelSkyCultureMgr::getCurrentSkyCultureBoundariesIdx() const
{
	return currentSkyCulture.boundaries;
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
		cultures += q_(i.value().englishName);
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
	description.append(getCurrentSkyCultureHtmlClassification());

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
		QRegExp commentRx("^(\\s*#.*|\\s*)$");
		reference = QString("<h3>%1</h3><ul>").arg(q_("References"));
		int totalRecords=0;
		int readOk=0;
		int lineNumber=0;
		while(!refFile.atEnd())
		{
			record = QString::fromUtf8(refFile.readLine()).trimmed();
			lineNumber++;
			if (commentRx.exactMatch(record))
				continue;

			totalRecords++;
			#if (QT_VERSION>=QT_VERSION_CHECK(5, 14, 0))
			QStringList ref = record.split(QRegExp("\\|"), Qt::KeepEmptyParts);
			#else
			QStringList ref = record.split(QRegExp("\\|"), QString::KeepEmptyParts);
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
		reference.append("</ul>");
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
		if (q_(i.value().englishName) == cultureName)
			return i.key();
	}
	return "";
}

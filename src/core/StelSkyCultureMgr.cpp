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
	updateListOfAvailableSkyCultures();
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

void StelSkyCultureMgr::updateListOfAvailableSkyCultures()
{
	QSet<QString> cultureDirNames = StelFileMgr::listContents("skycultures",StelFileMgr::Directory);
	dirToNameEnglish.clear();
	foreach (const QString& dir, cultureDirNames)
	{
		QString pdFile = StelFileMgr::findFile("skycultures/" + dir + "/info.ini");
		if (pdFile.isEmpty())
		{
			qWarning() << "WARNING: unable to successfully read info.ini file from skyculture dir"
				   << QDir::toNativeSeparators(dir);
			return;
		}
		QSettings pd(pdFile, StelIniFormat);
		dirToNameEnglish[dir].englishName = pd.value("info/name").toString();
		dirToNameEnglish[dir].author = pd.value("info/author").toString();
	}

	if (!currentSkyCultureDir.isEmpty() && !dirToNameEnglish.contains(currentSkyCultureDir))
	{
		if (getCurrentSkyCultureID() == defaultSkyCultureID)
		{
			setDefaultSkyCultureID(dirToNameEnglish.firstKey());
		}
		setCurrentSkyCultureID(defaultSkyCultureID);
	}
}

//! Set the current sky culture from the passed directory
bool StelSkyCultureMgr::setCurrentSkyCultureID(const QString& cultureDir)
{
	// make sure culture definition exists before attempting or will die
	if (directoryToSkyCultureEnglish(cultureDir) == "")
	{
		qWarning() << "Invalid sky culture directory: " << QDir::toNativeSeparators(cultureDir);
		return false;
	}
	currentSkyCultureDir = cultureDir;
	currentSkyCulture = dirToNameEnglish[cultureDir];
	StelApp::getInstance().updateSkyCulture();
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
	return true;
}

QString StelSkyCultureMgr::getCurrentSkyCultureNameI18() const {return q_(currentSkyCulture.englishName);}

QString StelSkyCultureMgr::getCurrentSkyCultureEnglishName() const {return currentSkyCulture.englishName;}

//! returns newline delimited list of human readable culture names in english
QString StelSkyCultureMgr::getSkyCultureListEnglish(void)
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
QStringList StelSkyCultureMgr::getSkyCultureListI18(void)
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

QStringList StelSkyCultureMgr::getSkyCultureListIDs(void)
{
	return dirToNameEnglish.keys();
}

QString StelSkyCultureMgr::directoryToSkyCultureEnglish(const QString& directory)
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

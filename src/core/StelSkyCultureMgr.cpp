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
#include "StelLocaleMgr.hpp"

#include <QSettings>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QDebug>
#include <QMap>
#include <QMapIterator>
#include <QDir>
#include <QUrl>

StelSkyCultureMgr::StelSkyCultureMgr()
{
	QSet<QString> cultureDirNames = StelFileMgr::listContents("skycultures",StelFileMgr::Directory);
	
	foreach (const QString& dir, cultureDirNames)
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
	// make sure culture definition exists before attempting or will die
	if (!dirToNameEnglish.contains(cultureDir))
	{
		qWarning() << "Invalid sky culture directory: " << QDir::toNativeSeparators(cultureDir);
		return false;
	}
	currentSkyCultureDir = cultureDir;
	currentSkyCulture = dirToNameEnglish[cultureDir];
	StelApp::getInstance().updateSkyCulture();
	emit changed();
	return true;
}

// Set the default sky culture from the ID.
bool StelSkyCultureMgr::setDefaultSkyCultureID(const QString& id)
{
	// make sure culture definition exists before attempting or will die
	if (!dirToNameEnglish.contains(id))
	{
		qWarning() << "Invalid sky culture ID: " << id;
		return false;
	}
	defaultSkyCultureID = id;
	QSettings* conf = StelApp::getInstance().getSettings();
	Q_ASSERT(conf);
	conf->setValue("localization/sky_culture", id);
	emit changed();
	return true;
}

QString StelSkyCultureMgr::getSkyCultureI18(const QString& id) const
{
	return q_(dirToNameEnglish[id].englishName);
}

QStringList StelSkyCultureMgr::getSkyCultureListIDs(void)
{
	return dirToNameEnglish.keys();
}

QString StelSkyCultureMgr::getSkyCultureDesc(const QString& id) const
{
	StelApp& app = StelApp::getInstance();
	QString lang = app.getLocaleMgr().getAppLanguage();
	if (!QString("pt_BR zh_CN zh_HK zh_TW").contains(lang))
	{
		lang = lang.split("_").at(0);
	}
	QString descPath = StelFileMgr::findFile("skycultures/" + id + "/description."+lang+".utf8");
	if (descPath.isEmpty())
	{
		descPath = StelFileMgr::findFile("skycultures/" + id + "/description.en.utf8");
		if (descPath.isEmpty())
			qWarning() << "WARNING: can't find description for skyculture" << id;
	}
	QStringList searchPaths;
	searchPaths << StelFileMgr::findFile("skycultures/" + id);
	QFile f(descPath);
	QString htmlFile;
	if (f.open(QIODevice::ReadOnly))
	{
		htmlFile = QString::fromUtf8(f.readAll());
		f.close();
	}
	return htmlFile;
}

QUrl StelSkyCultureMgr::getSkyCultureBaseUrl(const QString& id) const
{
	QString path = StelFileMgr::findFile("skycultures/" + id);
	return QUrl("file://" + QDir(path).absolutePath() + "/");
}

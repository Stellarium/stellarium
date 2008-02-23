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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "StelSkyCultureMgr.hpp"
#include "StelFileMgr.hpp"
#include "Translator.hpp"
#include "StelApp.hpp"
#include "StelIniParser.hpp"

#include <QSettings>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QDebug>
#include <QMap>
#include <QMapIterator>

StelSkyCultureMgr::StelSkyCultureMgr()
{
	QSet<QString> cultureDirNames;
	StelFileMgr& fileMan(StelApp::getInstance().getFileMgr());
	
	try
	{
		cultureDirNames = fileMan.listContents("skycultures",StelFileMgr::DIRECTORY);
	}
	catch(exception& e)
	{
		cerr << "ERROR while trying list sky cultures:" << e.what() << endl;	
	}
	
	for(QSet<QString>::iterator dir=cultureDirNames.begin(); dir!=cultureDirNames.end(); dir++)
	{
		try
		{
			QSettings pd(fileMan.findFile("skycultures/" + *dir + "/info.ini"), StelIniFormat);
			dirToNameEnglish[*dir] = pd.value("info/name").toString();
		}
		catch (exception& e)
		{
			qWarning() << "WARNING: unable to successfully read info.ini file from skyculture dir" << *dir;
		}
	}	
}


StelSkyCultureMgr::~StelSkyCultureMgr()
{
}


//! Init itself from a config file.
void StelSkyCultureMgr::init()
{
	setSkyCultureDir(StelApp::getInstance().getSettings()->value("localization/sky_culture", "western").toString());
}

//! Set the current sky culture from the passed directory
bool StelSkyCultureMgr::setSkyCultureDir(const QString& cultureDir)
{
	// make sure culture definition exists before attempting or will die
	if(directoryToSkyCultureEnglish(cultureDir) == "")
	{
		qWarning() << "Invalid sky culture directory: " << cultureDir;
		return false;
	}
	skyCultureDir = cultureDir;
	StelApp::getInstance().updateSkyCulture();
	return true;
}

//! returns newline delimited list of human readable culture names in english
QString StelSkyCultureMgr::getSkyCultureListEnglish(void)
{
	QString cultures;
	QMapIterator<QString, QString> i(dirToNameEnglish);
	while(i.hasNext())
	{
		i.next();
		cultures += QString("%1\n").arg(i.value());
	}
	return cultures;
}

//! returns newline delimited list of human readable culture names translated to current locale
QString StelSkyCultureMgr::getSkyCultureListI18(void)
{
	QString cultures;
	QMapIterator<QString, QString> i(dirToNameEnglish);
	while(i.hasNext())
	{
		i.next();
		cultures += QString("%1\n").arg(q_(i.value()));
	}
	// remove last newline - strangly inconsistent with getSkyCultureListEnglish...
	cultures.chop(1);
	return cultures;
}

//! returns newline delimited hash of human readable culture names translated to current locale
//! and the directory names
QString StelSkyCultureMgr::getSkyCultureHash(void) const
{
	QString cultures;
	QMapIterator<QString, QString> i(dirToNameEnglish);
	while(i.hasNext())
	{
		i.next();
		if(i.value() == "") continue;
		cultures += QString("%1\n%2\n").arg(q_(i.value())).arg(i.key());
	}

	return cultures;
}

QString StelSkyCultureMgr::getSkyCulture() const
{
	return directoryToSkyCultureI18(skyCultureDir);
}


QString StelSkyCultureMgr::directoryToSkyCultureEnglish(const QString& directory)
{
	return dirToNameEnglish[directory];
}

QString StelSkyCultureMgr::directoryToSkyCultureI18(const QString& directory) const
{
	QString culture = dirToNameEnglish[directory];
	if (culture=="")
	{
		qWarning() << "WARNING: StelSkyCultureMgr::directoryToSkyCultureI18(\""
		           << directory << "\"): could not find directory";
		return "";
	}
	return q_(culture);
}

QString StelSkyCultureMgr::skyCultureToDirectory(const QString& cultureName)
{
	QMapIterator<QString, QString> i(dirToNameEnglish);
	while(i.hasNext())
	{
		i.next();
		if (q_(i.value()) == cultureName)
			return i.key();
	}
	return "";
}

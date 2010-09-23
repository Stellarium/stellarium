/*
 * Comet and asteroids importer plug-in for Stellarium
 * 
 * Copyright (C) 2010 Bogdan Marinov
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

#include "CAImporter.hpp"

#include "StelApp.hpp"
#include "StelGui.hpp"
#include "StelGuiItems.hpp"
#include "StelFileMgr.hpp"
#include "StelIniParser.hpp"
#include "StelLocaleMgr.hpp"
#include "StelModuleMgr.hpp"
#include "SolarSystem.hpp"

#include <QDebug>
#include <QFile>
#include <QSettings>
#include <QString>

#include <stdexcept>


StelModule* CAImporterStelPluginInterface::getStelModule() const
{
	return new CAImporter();
}

StelPluginInfo CAImporterStelPluginInterface::getPluginInfo() const
{
	//Q_INIT_RESOURCE(caImporter);
	
	StelPluginInfo info;
	info.id = "CAImporter";
	info.displayedName = "Comets and Asteroids Importer";
	info.authors = "Bogdan Marinov";
	info.contact = "http://stellarium.org";
	info.description = "A plug-in that allows importing asteroid and comet data in different formats to Stellarium's ssystem.ini file. It's still a work in progress far from maturity.";
	return info;
}

Q_EXPORT_PLUGIN2(CAImporter, CAImporterStelPluginInterface)

CAImporter::CAImporter()
{
	setObjectName("CAImporter");
}

CAImporter::~CAImporter()
{

}

void CAImporter::init()
{
	try
	{
		qDebug() << "Initialization.";//Do something
	}
	catch (std::runtime_error &e)
	{
		qWarning() << "init() error: " << e.what();
		return;
	}
}

void CAImporter::deinit()
{
	//
}

void CAImporter::update(double deltaTime)
{
	//
}

void CAImporter::draw(StelCore* core)
{
	//
}

double CAImporter::getCallOrder(StelModuleActionName actionName) const
{
	return 0.;
}

bool CAImporter::configureGui(bool show)
{
	if(show)
	{
		//mainWindow->setVisible(true);

		if (cloneSolarSystemConfigurationFile())
		{
			//Import Encke for a start
			importMpcOneLineCometElements("0002P         2010 08  6.5102  0.336152  0.848265  186.5242  334.5718   11.7843  20100104  11.5  6.0  2P/Encke                                                 MPC 59600");

			/*//This doesn't cause a crash, but it causes two entries to appear in
			//the search dialog for 2P/Encke and FOUR - for the other planets.
			//It's possible that the planets are duplicated, too, but it's hard
			//to see.
			GETSTELMODULE(SolarSystem)->init();*/

			//This code results in one entry for 2P/Encke and two - for
			//everything else. Progress!
			//Memory leak?
			StelApp::getInstance().getModuleMgr().unloadModule("SolarSystem", false);
			SolarSystem* ssystem = new SolarSystem();
			ssystem->init();
			StelApp::getInstance().getModuleMgr().registerModule(ssystem);
		}
	}
	return true;
}

bool CAImporter::cloneSolarSystemConfigurationFile()
{
	QString defaultFilePath	= StelFileMgr::getInstallationDir() + "/data/ssystem.ini";
	QString userFilePath	= StelFileMgr::getUserDir() + "/data/ssystem.ini";

	if (QFile::exists(userFilePath))
	{
		qDebug() << "CAI: A ssystem.ini file already exists in the user directory";
		return true;
	}

	if (QFile::exists(defaultFilePath))
	{
		return QFile::copy(defaultFilePath, userFilePath);
	}
	else
	{
		qDebug() << "CAI: This should be impossible.";
		return false;
	}
}

bool CAImporter::importMpcOneLineCometElements(QString oneLineElements)
{
	QSettings solarSystemFile(StelFileMgr::getUserDir() + "/data/ssystem.ini", StelIniFormat, this);
	QRegExp mpcParser("^\\s*(\\d{4})?([A-Z])(\\w{7})?\\s+(\\d{4})\\s+(\\d{2})\\s+(\\d{1,2}\\.\\d{4})\\s+(\\d{1,2}\\.\\d{6})\\s+(\\d\\.\\d{6})\\s+(\\d{1,3}\\.\\d{4})\\s+(\\d{1,3}\\.\\d{4})\\s+(\\d{1,3}\\.\\d{4})\\s+((\\d{4})(?:\\d\\d)(\\d\\d))?\\s+(\\d{1,2}\\.\\d)\\s+(\\d{1,2}\\.\\d)\\s+(\\S.{55})\\s+(\\S.*)$");//

	int match = mpcParser.indexIn(oneLineElements);
	//qDebug() << "RegExp captured:" << match << mpcParser.capturedTexts();

	if (match < 0)
	{
		qWarning() << "No match";
		return false;
	}

	QString name = mpcParser.cap(17).trimmed();
	QString sectionName(name);
	sectionName.remove('\\');
	sectionName.remove('/');
	sectionName.remove('#');
	solarSystemFile.beginGroup(sectionName);

	if (mpcParser.cap(1).isEmpty() && mpcParser.cap(3).isEmpty())
	{
		qWarning() << "Comet is missing both comet number AND provisional designation.";
		return false;
	}
	solarSystemFile.setValue("name", name);
	solarSystemFile.setValue("parent", "Sun");
	solarSystemFile.setValue("coord_func","comet_orbit");

	solarSystemFile.setValue("lighting", false);
	solarSystemFile.setValue("color", "1.0, 1.0, 1.0");//TODO
	solarSystemFile.setValue("tex_map", "nomap.png");

	bool ok = false;

	int year	= mpcParser.cap(4).toInt();
	int month	= mpcParser.cap(5).toInt();
	double dayFraction	= mpcParser.cap(6).toDouble(&ok);
	int day = (int) dayFraction;
	QDate datePerihelionPassage(year, month, day);
	int fraction = (int) ((dayFraction - day) * 24 * 60 * 60);
	int seconds = fraction % 60; fraction /= 60;
	int minutes = fraction % 60; fraction /= 60;
	int hours = fraction % 24;
	//qDebug() << hours << minutes << seconds << fraction;
	QTime timePerihelionPassage(hours, minutes, seconds, 0);
	QDateTime dtPerihelionPassage(datePerihelionPassage, timePerihelionPassage, Qt::UTC);
	double jdPerihelionPassage = StelUtils::qDateTimeToJd(dtPerihelionPassage);
	solarSystemFile.setValue("orbit_TimeAtPericenter", jdPerihelionPassage);

	double perihelionDistance = mpcParser.cap(7).toDouble(&ok);//AU
	solarSystemFile.setValue("orbit_PericenterDistance", perihelionDistance);

	double eccentricity = mpcParser.cap(8).toDouble(&ok);//degrees
	solarSystemFile.setValue("orbit_Eccentricity", eccentricity);

	double argumentOfPerihelion = mpcParser.cap(9).toDouble(&ok);//J2000.0, degrees
	solarSystemFile.setValue("orbit_ArgOfPericenter", argumentOfPerihelion);

	double longitudeOfTheAscendingNode = mpcParser.cap(10).toDouble(&ok);//J2000.0, degrees
	solarSystemFile.setValue("orbit_AscendingNode", longitudeOfTheAscendingNode);

	double inclination = mpcParser.cap(11).toDouble(&ok);
	solarSystemFile.setValue("orbit_Inclination", inclination);

	//Albedo doesn't work at all
	//TODO: Make sure comets don't display magnitude
	double absoluteMagnitude = mpcParser.cap(15).toDouble(&ok);
	double radius = 5; //Fictitious
	solarSystemFile.setValue("radius", radius);
	qDebug() << 1329 * pow(10, (absoluteMagnitude/-5));
	double albedo = pow(( (1329 * pow(10, (absoluteMagnitude/-5))) / (2 * radius)), 2);//from http://www.physics.sfasu.edu/astro/asteroids/sizemagnitude.html
	solarSystemFile.setValue("albedo", albedo);

	solarSystemFile.endGroup();

	return true;
}

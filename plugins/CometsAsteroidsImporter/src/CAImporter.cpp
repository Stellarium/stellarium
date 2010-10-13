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
#include "SolarSystemManagerWindow.hpp"

#include "StelApp.hpp"
#include "StelGui.hpp"
#include "StelGuiItems.hpp"
#include "StelFileMgr.hpp"
#include "StelIniParser.hpp"
#include "StelLocaleMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StelObjectMgr.hpp"
#include "SolarSystem.hpp"

#include <QDate>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QSettings>
#include <QString>

#include <cmath>
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
	info.displayedName = "Comets and Asteroids";
	info.authors = "Bogdan Marinov";
	info.contact = "http://stellarium.org";
	info.description = "A plug-in that allows importing asteroid and comet data in different formats to Stellarium's ssystem.ini file. It's still a work in progress far from maturity.";
	return info;
}

Q_EXPORT_PLUGIN2(CAImporter, CAImporterStelPluginInterface)

CAImporter::CAImporter()
{
	setObjectName("CAImporter");

	isInitialized = false;
	mainWindow = NULL;
	solarSystemConfigurationFile = NULL;

	//I really hope that the file manager is instantiated before this
	defaultSolarSystemFilePath	= QFileInfo(StelFileMgr::getInstallationDir() + "/data/ssystem.ini").absoluteFilePath();
	customSolarSystemFilePath	= QFileInfo(StelFileMgr::getUserDir() + "/data/ssystem.ini").absoluteFilePath();
}

CAImporter::~CAImporter()
{
	if (solarSystemConfigurationFile != NULL)
	{
		delete solarSystemConfigurationFile;
	}
}

void CAImporter::init()
{
	//Get a list of the "default" Solar System objects' IDs:
	//TODO: Use it as validation for the loading of the plug-in
	if (QFile::exists(defaultSolarSystemFilePath))
	{
		defaultSsoIds = readAllActiveSsoIdsInFile(defaultSolarSystemFilePath);
	}
	else
	{
		//TODO: Better error message
		qDebug() << "Something is horribly wrong:" << StelFileMgr::getInstallationDir();
		return;
	}

	try
	{
		//Make sure that a user ssystem.ini actually exists
		if (!cloneSolarSystemConfigurationFile())
			return;

		mainWindow = new SolarSystemManagerWindow();
	}
	catch (std::runtime_error &e)
	{
		qWarning() << "init() error: " << e.what();
		return;
	}

	isInitialized = true;
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
	//If the plug-in has failed to initialize, disable the button
	//TODO: Display a message in the window instead.
	if (!isInitialized)
		return false;

	if(show)
	{
		mainWindow->setVisible(true);

		//Debugging block
		//if (cloneSolarSystemConfigurationFile())
		{
			//Import Encke for a start
			/*SsoElements SSO = readMpcOneLineCometElements("0002P         2010 08  6.5102  0.336152  0.848265  186.5242  334.5718   11.7843  20100104  11.5  6.0  2P/Encke                                                 MPC 59600");
			if (!appendToSolarSystemConfigurationFile(SSO))
				return true;
			*/

			//Import a list of comets on the desktop. The file is from
			//http://www.minorplanetcenter.org/iau/Ephemerides/Comets/Soft00Cmt.txt
			//It seems that some entries in the list don't match the described format
			/*QList<SsoElements> objectList = readMpcOneLineCometElementsFromFile(StelFileMgr::getDesktopDir() + "/Soft00Cmt.txt");
			if (!appendToSolarSystemConfigurationFile(objectList))
				return true;
			*/

			//Import Cruithne
			/*SsoElements asteroid = readMpcOneLineMinorPlanetElements("03753   15.6   0.15 K107N 205.95453   43.77037  126.27658   19.80793  0.5149179  0.98898552   0.9977217  3 MPO183459   488  28 1973-2010 0.58 M-h 3Eh MPC        0000           (3753) Cruithne   20100822");
			if (!appendToSolarSystemConfigurationFile(asteroid))
				return true;
			*/

			//Import a list of asteroids. The file is from
			//http://www.minorplanetcenter.org/iau/Ephemerides/Bright/2010/Soft00Bright.txt
			/*QList<SsoElements> objectList = readMpcOneLineMinorPlanetElementsFromFile(StelFileMgr::getDesktopDir() + "/Soft00Bright.txt");
			if (!appendToSolarSystemConfigurationFile(objectList))
				return true;*/

			//Destroy and re-create the Solal System
			//GETSTELMODULE(SolarSystem)->reloadPlanets();
		}
	}
	return true;
}

bool CAImporter::cloneSolarSystemConfigurationFile()
{
	QDir userDataDirectory(StelFileMgr::getUserDir());
	if (!userDataDirectory.exists())
	{
		qDebug() << "Unable to find user data directory:" << userDataDirectory.absolutePath();
		return false;
	}
	if (!userDataDirectory.exists("data") && !userDataDirectory.mkdir("data"))
	{
		qDebug() << "Unable to create a \"data\" subdirectory in" << userDataDirectory.absolutePath();
		return false;
	}

	if (QFile::exists(customSolarSystemFilePath))
	{
		qDebug() << "Using the ssystem.ini file that already exists in the user directory...";
		return true;
	}

	if (QFile::exists(defaultSolarSystemFilePath))
	{
		qDebug() << "Trying to copy ssystem.ini to" << customSolarSystemFilePath;
		return QFile::copy(defaultSolarSystemFilePath, customSolarSystemFilePath);
	}
	else
	{
		qDebug() << "This should be impossible.";
		return false;
	}
}

bool CAImporter::resetSolarSystemConfigurationFile()
{
	if (QFile::exists(customSolarSystemFilePath))
	{
		if (!QFile::remove((customSolarSystemFilePath)))
		{
			qWarning() << "Unable to delete" << customSolarSystemFilePath
			         << endl << "Please remove the file manually.";
			return false;
		}
	}

	return cloneSolarSystemConfigurationFile();
}

void CAImporter::resetSolarSystemToDefault()
{
	if (isInitialized)
	{
		if (resetSolarSystemConfigurationFile())
		{
			GETSTELMODULE(SolarSystem)->reloadPlanets();
			emit solarSystemChanged();
		}
	}
}

bool CAImporter::copySolarSystemConfigurationFileTo(QString filePath)
{
	if (QFile::exists(customSolarSystemFilePath))
	{
		QFileInfo targetFile(filePath);
		return QFile::copy(customSolarSystemFilePath, targetFile.absoluteFilePath());
	}
	else
	{
		return false;
	}
}

bool CAImporter::replaceSolarSystemConfigurationFileWith(QString filePath)
{
	if (!QFile::exists(filePath))
	{
		//TODO: Message
		return false;
	}

	//Is it a valid configuration file?
	QSettings settings(filePath, QSettings::IniFormat);
	if (settings.status() != QSettings::NoError)
	{
		qWarning() << filePath << "is not a valid configuration file.";
		return false;
	}

	//Remove the existingfile
	if (QFile::exists(customSolarSystemFilePath))
	{
		if(!QFile::remove(customSolarSystemFilePath))
		{
			//TODO: Message
			return false;
		}
	}

	//Copy the new file
	//If the copy fails, reset to the default configuration
	if (QFile::copy(filePath, customSolarSystemFilePath))
	{
		GETSTELMODULE(SolarSystem)->reloadPlanets();
		emit solarSystemChanged();
		return true;
	}
	else
	{
		//TODO: Message
		if (cloneSolarSystemConfigurationFile())
		{
			GETSTELMODULE(SolarSystem)->reloadPlanets();
			emit solarSystemChanged();
			return true;
		}
		else
		{
			//TODO: Message
			return false;
		}
	}
}

QStringList CAImporter::readAllActiveSsoIdsInFile(QString filePath)
{
	if (!QFile::exists(filePath))
		return QStringList();

	QSettings solarSystem(filePath, QSettings::IniFormat);
	if (solarSystem.status() != QSettings::NoError)
		return QStringList();

	QStringList groups = solarSystem.childGroups();
	QStringList planetNames = GETSTELMODULE(SolarSystem)->getAllPlanetEnglishNames();
	QStringList ssoIds;
	foreach (QString group, groups)
	{
		QString name = solarSystem.value(group + "/name").toString();
		if (planetNames.contains(name))
		{
			ssoIds.append(group);
		}
	}
	return ssoIds;
}

QStringList CAImporter::readAllCurrentSsoIds()
{
	if (QFile::exists(customSolarSystemFilePath))
	{
		return readAllActiveSsoIdsInFile(customSolarSystemFilePath);
	}
	else
	{
		//TODO: Error message
		return QStringList();
	}
}

bool CAImporter::removeSsoWithId(QString id)
{
	if (id.isEmpty())
		return false;

	//qDebug() << id;
	if (getAllDefaultSsoIds().contains(id))
	{
		qWarning() << "You can't delete the default Solar System objects for the moment.";
		return false;
	}

	//Make sure that the file exists
	if (!QFile::exists(customSolarSystemFilePath))
	{
		qDebug() << "Can't remove" << id << "to ssystem.ini: Unable to find" << customSolarSystemFilePath;
		return false;
	}

	//Open the file
	QSettings settings(customSolarSystemFilePath, QSettings::IniFormat);
	if (settings.status() != QSettings::NoError)
	{
		qDebug() << "Error opening ssystem.ini:" << customSolarSystemFilePath;
		return false;
	}

	//Remove the section
	settings.remove(id);
	settings.sync();

	//Deselect all currently selected objects
	//TODO: I bet that someone will complains, so: unselect only the removed one
	GETSTELMODULE(StelObjectMgr)->unSelect();

	//Reload the Solar System
	GETSTELMODULE(SolarSystem)->reloadPlanets();
	emit solarSystemChanged();

	return true;
}

//TODO: Strings that have failed to be parsed. The usual source of discrepancies is
//http://www.minorplanetcenter.org/iau/Ephemerides/Comets/Soft00Cmt.txt
//It seems that some entries in the list don't match the described format.
/*
  "    CJ95O010  1997 03 31.4141  0.906507  0.994945  130.5321  282.6820   89.3193  20100723  -2.0  4.0  C/1995 O1 (Hale-Bopp)                                    MPC 61436" -> minus sign, fixed
  "    CK09K030  2011 01  9.266   3.90156   1.00000   251.413     0.032   146.680              8.5  4.0  C/2009 K3 (Beshore)                                      MPC 66205" -> lower precision than the spec, fixed
  "    CK10F040  2010 04  6.109   0.61383   1.00000   120.718   237.294    89.143             13.5  4.0  C/2010 F4 (Machholz)                                     MPC 69906" -> lower precision than the spec, fixed
  "    CK10M010  2012 02  7.840   2.29869   1.00000   265.318    82.150    78.373              9.0  4.0  C/2010 M1 (Gibbs)                                        MPC 70817" -> lower precision than the spec, fixed
  "    CK10R010  2011 11 28.457   6.66247   1.00000    96.009   345.949   157.437              6.0  4.0  C/2010 R1 (LINEAR)                                       MPEC 2010-R99" -> lower precision than the spec, fixed
  "0128P      b  2007 06 13.8064  3.062504  0.320891  210.3319  214.3583    4.3606  20100723   8.5  4.0  128P/Shoemaker-Holt                                      MPC 51822" -> fragment?
  "0141P      d  2010 05 29.7106  0.757809  0.749215  149.3298  246.0849   12.8032  20100723  12.0 12.0  141P/Machholz                                            MPC 59599" -> fragment?
*/
CAImporter::SsoElements CAImporter::readMpcOneLineCometElements(QString oneLineElements)
{
	SsoElements result;

	QRegExp mpcParser("^\\s*(\\d{4})?([A-Z])(\\w{7})?\\s+(\\d{4})\\s+(\\d{2})\\s+(\\d{1,2}\\.\\d{3,4})\\s+(\\d{1,2}\\.\\d{5,6})\\s+(\\d\\.\\d{5,6})\\s+(\\d{1,3}\\.\\d{3,4})\\s+(\\d{1,3}\\.\\d{3,4})\\s+(\\d{1,3}\\.\\d{3,4})\\s+(?:(\\d{4})(\\d\\d)(\\d\\d))?\\s+(\\-?\\d{1,2}\\.\\d)\\s+(\\d{1,2}\\.\\d)\\s+(\\S.{55})\\s+(\\S.*)$");//

	int match = mpcParser.indexIn(oneLineElements);
	//qDebug() << "RegExp captured:" << match << mpcParser.capturedTexts();

	if (match < 0)
	{
		qWarning() << "No match for" << oneLineElements;
		return result;
	}

	QString name = mpcParser.cap(17).trimmed();
	QString sectionName(name);
	//TODO: Should I remove all non-alphanumeric, or only the obviously problematic?
	sectionName.remove('\\');
	sectionName.remove('/');
	sectionName.remove('#');
	sectionName.remove(' ');
	sectionName.remove('-');
	sectionName = sectionName.toLower();

	if (mpcParser.cap(1).isEmpty() && mpcParser.cap(3).isEmpty())
	{
		qWarning() << "Comet is missing both comet number AND provisional designation.";
		return result;
	}

	result.insert("section_name", sectionName);
	result.insert("name", name);
	result.insert("parent", "Sun");
	result.insert("coord_func","comet_orbit");
	result.insert("type", "comet");

	result.insert("lighting", false);
	result.insert("color", "1.0, 1.0, 1.0");
	result.insert("tex_map", "nomap.png");

	bool ok = false;
	//TODO: Use this for VALIDATION!

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
	result.insert("orbit_TimeAtPericenter", jdPerihelionPassage);

	double perihelionDistance = mpcParser.cap(7).toDouble(&ok);//AU
	result.insert("orbit_PericenterDistance", perihelionDistance);

	double eccentricity = mpcParser.cap(8).toDouble(&ok);//degrees
	result.insert("orbit_Eccentricity", eccentricity);

	double argumentOfPerihelion = mpcParser.cap(9).toDouble(&ok);//J2000.0, degrees
	result.insert("orbit_ArgOfPericenter", argumentOfPerihelion);

	double longitudeOfTheAscendingNode = mpcParser.cap(10).toDouble(&ok);//J2000.0, degrees
	result.insert("orbit_AscendingNode", longitudeOfTheAscendingNode);

	double inclination = mpcParser.cap(11).toDouble(&ok);
	result.insert("orbit_Inclination", inclination);

	double absoluteMagnitude = mpcParser.cap(15).toDouble(&ok);
	result.insert("absolute_magnitude", absoluteMagnitude);

	//This is not the same "slope parameter" as used in asteroids. Better name?
	double slopeParameter = mpcParser.cap(16).toDouble(&ok);
	result.insert("slope_parameter", slopeParameter);

	double radius = 5; //Fictitious
	result.insert("radius", radius);
	result.insert("albedo", 1);

	return result;
}

CAImporter::SsoElements CAImporter::readMpcOneLineMinorPlanetElements(QString oneLineElements)
{
	SsoElements result;

	//This time I'll try splitting the line to columns, instead of
	//using a regular expression.
	//Using QString::mid() allows parsing it in a random sequence.

	//Length validation
	if (oneLineElements.isEmpty() ||
	    oneLineElements.length() > 202 ||
	    oneLineElements.length() < 152) //The column ends at 160, but is left-aligned
	{
		return result;
	}

	QString column;
	bool ok = false;
	//bool isLongForm = (oneLineElements.length() > 160) ? true : false;

	//Minor planet number or provisional designation
	column = oneLineElements.mid(0, 7).trimmed();
	if (column.isEmpty())
	{
		return result;
	}
	int minorPlanetNumber = 0;
	QString provisionalDesignation;
	QString name;
	if (column.toInt(&ok) || ok)
	{
		minorPlanetNumber = column.toInt();
	}
	else
	{
		//See if it is a number, but packed
		//I hope the format is right (I've seen prefixes only between A and P)
		QRegExp packedMinorPlanetNumber("^([A-Za-z])(\\d+)$");
		if (packedMinorPlanetNumber.indexIn(column) == 0)
		{
			minorPlanetNumber = packedMinorPlanetNumber.cap(2).toInt(&ok);
			//TODO: Validation
			QChar prefix = packedMinorPlanetNumber.cap(1).at(0);
			if (prefix.isUpper())
			{
				minorPlanetNumber += ((10 + prefix.toAscii() - 'A') * 10000);
			}
			else
			{
				minorPlanetNumber += ((10 + prefix.toAscii() - 'a' + 26) * 10000);
			}
		}
		else
		{
			provisionalDesignation = unpackMinorPlanetProvisionalDesignation(column);
		}
	}

	if (minorPlanetNumber)
	{
		name = QString::number(minorPlanetNumber);
	}
	else if(provisionalDesignation.isEmpty())
	{
		qDebug() << "readMpcOneLineMinorPlanetElements():"
		         << column
		         << "is not a valid number or packed provisional designation";
		return SsoElements();
	}
	else
	{
		name = provisionalDesignation;
	}

	//In case the longer format is used, extract the human-readable name
	column = oneLineElements.mid(166, 28).trimmed();
	if (!column.isEmpty())
	{
		if (minorPlanetNumber)
		{
			QRegExp asteroidName("^\\((\\d+)\\)\\s+(\\S.+)$");
			if (asteroidName.indexIn(column) == 0)
			{
				name = asteroidName.cap(2);
				result.insert("minor_planet_number", minorPlanetNumber);
			}
			else
			{
				//Use the whole string, just in case
				name = column;
			}
		}
		//In the other case, the name is already the provisional designation
	}

	result.insert("name", name);

	//Section name
	QString sectionName(name);
	//TODO: Should I remove all non-alphanumeric, or only the obviously problematic?
	sectionName.remove('\\');
	sectionName.remove('/');
	sectionName.remove('#');
	sectionName.remove(' ');
	sectionName.remove('-');
	sectionName = sectionName.toLower();
	//TODO: Check if something remained in the name
	result.insert("section_name", sectionName);

	//After a name has been determined, insert the essential keys
	result.insert("parent", "Sun");
	result.insert("coord_func","comet_orbit");
	result.insert("type", "asteroid");

	result.insert("lighting", false);
	result.insert("color", "1.0, 1.0, 1.0");
	result.insert("tex_map", "nomap.png");

	//Magnitude and slope parameter
	column = oneLineElements.mid(8,5).trimmed();
	double absoluteMagnitude = column.toDouble(&ok);
	//TODO: Validation
	column = oneLineElements.mid(14,5).trimmed();
	double slopeParameter = column.toDouble(&ok);
	//TODO: Validation
	result.insert("absolute_magnitude", absoluteMagnitude);
	result.insert("slope_parameter", slopeParameter);

	//Orbital parameters
	column = oneLineElements.mid(37, 9).trimmed();
	double argumentOfPerihelion = column.toDouble(&ok);//J2000.0, degrees
	//TODO: Validation
	result.insert("orbit_ArgOfPericenter", argumentOfPerihelion);

	column = oneLineElements.mid(48, 9).trimmed();
	double longitudeOfTheAscendingNode = column.toDouble(&ok);//J2000.0, degrees
	//TODO: Validation
	result.insert("orbit_AscendingNode", longitudeOfTheAscendingNode);

	column = oneLineElements.mid(59, 9).trimmed();
	double inclination = column.toDouble(&ok);//J2000.0, degrees
	//TODO: Validation
	result.insert("orbit_Inclination", inclination);

	column = oneLineElements.mid(70, 9).trimmed();
	double eccentricity = column.toDouble(&ok);//degrees
	//TODO: Validation
	result.insert("orbit_Eccentricity", eccentricity);

	column = oneLineElements.mid(80, 11).trimmed();
	double meanDailyMotion = column.toDouble(&ok);//degrees per day
	//TODO: Validation
	result.insert("orbit_MeanMotion", meanDailyMotion);

	column = oneLineElements.mid(92, 11).trimmed();
	double semiMajorAxis = column.toDouble(&ok);
	//TODO: Validation
	result.insert("orbit_SemiMajorAxis", semiMajorAxis);

	column = oneLineElements.mid(20, 5).trimmed();//Epoch, in packed form
	QRegExp packedDateFormat("^([IJK])(\\d\\d)([1-9A-C])([1-9A-V])$");
	if (packedDateFormat.indexIn(column) != 0)
	{
		qDebug() << "readMpcOneLineMinorPlanetElements():"
		         << column << "is not a date in packed format";
		return SsoElements();
	}
	int year = packedDateFormat.cap(2).toInt();
	switch (packedDateFormat.cap(1).at(0).toAscii())
	{
		case 'I':
			year += 1800;
			break;
		case 'J':
			year += 1900;
			break;
		case 'K':
		default:
			year += 2000;
	}
	int month = unpackDayOrMonthNumber(packedDateFormat.cap(3).at(0));
	int day   = unpackDayOrMonthNumber(packedDateFormat.cap(4).at(0));
	//qDebug() << column << year << month << day;
	QDate epochDate(year, month, day);
	if (!epochDate.isValid())
	{
		qDebug() << "readMpcOneLineMinorPlanetElements():"
		         << column << "unpacks to"
		         << QString("%1-%2-%3").arg(year).arg(month).arg(day)
				 << "This is not a valid date for an Epoch.";
		return SsoElements();
	}
	int epochJD = epochDate.toJulianDay();
	result.insert("orbit_Epoch", epochJD);

	column = oneLineElements.mid(26, 9).trimmed();
	double meanAnomalyAtEpoch = column.toDouble(&ok);//degrees
	//TODO: Validation
	result.insert("orbit_MeanAnomaly", meanAnomalyAtEpoch);

	//Radius and albedo
	//Assume albedo of 0.15 and calculate a radius based on the absolute magnitude
	//as described here: http://www.physics.sfasu.edu/astro/asteroids/sizemagnitude.html
	double albedo = 0.15; //Assumed
	double radius = std::ceil((1329 / std::sqrt(albedo)) * std::pow(10, -0.2 * absoluteMagnitude));
	result.insert("albedo", albedo);
	result.insert("radius", radius);

	return result;
}

QList<CAImporter::SsoElements> CAImporter::readMpcOneLineCometElementsFromFile(QString filePath)
{
	QList<CAImporter::SsoElements> objectList;

	if (!QFile::exists(filePath))
	{
		qDebug() << "Can't find" << filePath;
		return objectList;
	}

	QFile mpcElementsFile(filePath);
	if (mpcElementsFile.open(QFile::ReadOnly | QFile::Text ))//| QFile::Unbuffered
	{
		int candidatesCount = 0;
		int lineCount = 0;

		while(!mpcElementsFile.atEnd())
		{
			QString oneLineElements = QString(mpcElementsFile.readLine(200));
			if(oneLineElements.endsWith('\n'))
			{
				oneLineElements.chop(1);
			}
			if (oneLineElements.isEmpty())
			{
				qDebug() << "Empty line skipped.";
				continue;
			}
			lineCount++;

			SsoElements ssObject = readMpcOneLineCometElements(oneLineElements);
			if(!ssObject.isEmpty() && !ssObject.value("section_name").toString().isEmpty())
			{
				objectList << ssObject;
				candidatesCount++;
			}
		}
		mpcElementsFile.close();
		qDebug() << "Done reading comet orbital elements."
		         << "Recognized" << candidatesCount << "candidate objects"
		         << "out of" << lineCount << "lines.";

		return objectList;
	}
	else
	{
		qDebug() << "Unable to open for reading" << filePath;
		qDebug() << "File error:" << mpcElementsFile.errorString();
		return objectList;
	}

	return objectList;
}

QList<CAImporter::SsoElements> CAImporter::readMpcOneLineMinorPlanetElementsFromFile(QString filePath)
{
	QList<CAImporter::SsoElements> objectList;

	if (!QFile::exists(filePath))
	{
		qDebug() << "Can't find" << filePath;
		return objectList;
	}

	QFile mpcElementsFile(filePath);
	if (mpcElementsFile.open(QFile::ReadOnly | QFile::Text ))//| QFile::Unbuffered
	{
		int candidatesCount = 0;
		int lineCount = 0;

		while(!mpcElementsFile.atEnd())
		{
			QString oneLineElements = QString(mpcElementsFile.readLine(202 + 2));//Allow for end-of-line characters
			if(oneLineElements.endsWith('\n'))
			{
				oneLineElements.chop(1);
			}
			if (oneLineElements.isEmpty())
			{
				qDebug() << "Empty line skipped.";
				continue;
			}
			lineCount++;

			SsoElements ssObject = readMpcOneLineMinorPlanetElements(oneLineElements);
			if(!ssObject.isEmpty() && !ssObject.value("section_name").toString().isEmpty())
			{
				objectList << ssObject;
				candidatesCount++;
			}
		}
		mpcElementsFile.close();
		qDebug() << "Done reading minor planet orbital elements."
		         << "Recognized" << candidatesCount << "candidate objects"
		         << "out of" << lineCount << "lines.";

		return objectList;
	}
	else
	{
		qDebug() << "Unable to open for reading" << filePath;
		qDebug() << "File error:" << mpcElementsFile.errorString();
		return objectList;
	}

	return objectList;
}

bool CAImporter::appendToSolarSystemConfigurationFile(QList<SsoElements> objectList)
{
	if (objectList.isEmpty())
	{
		return false;
	}

	//Check if the configuration file exists
	if (!QFile::exists(customSolarSystemFilePath))
	{
		qDebug() << "Can't append object data to ssystem.ini: Unable to find" << customSolarSystemFilePath;
		return false;
	}

	//Remove duplicates
	QSettings * solarSystemSettings = new QSettings(customSolarSystemFilePath, QSettings::IniFormat);
	if (solarSystemSettings->status() != QSettings::NoError)
	{
		qDebug() << "Error opening ssystem.ini:" << customSolarSystemFilePath;
		return false;
	}
	foreach (SsoElements object, objectList)
	{
		if (!object.contains("section_name"))
			continue;

		QString sectionName = object.value("section_name").toString();
		if (sectionName.isEmpty())
			continue;

		solarSystemSettings->remove(sectionName);
	}
	delete solarSystemSettings;//This should call QSettings::sync()
	solarSystemSettings = NULL;

	//Write to file
	//TODO: The usual validation
	qDebug() << "Appending to file...";
	QFile solarSystemConfigurationFile(customSolarSystemFilePath);
	if(solarSystemConfigurationFile.open(QFile::WriteOnly | QFile::Append | QFile::Text))
	{
		QTextStream output (&solarSystemConfigurationFile);
		bool appendedAtLeastOne = false;

		foreach (SsoElements object, objectList)
		{
			if (!object.contains("section_name"))
				continue;

			QString sectionName = object.value("section_name").toString();
			if (sectionName.isEmpty())
				continue;
			object.remove("section_name");

			output << endl << QString("[%1]").arg(sectionName) << endl;
			foreach(QString key, object.keys())
			{
				output << QString("%1 = %2").arg(key).arg(object.value(key).toString()) << endl;
			}
			output.flush();
			qDebug() << "Appended successfully" << sectionName;
			appendedAtLeastOne = true;
		}

		solarSystemConfigurationFile.close();
		return appendedAtLeastOne;
	}
	else
	{
		qDebug() << "Unable to open for writing" << customSolarSystemFilePath;
		return false;
	}
}

bool CAImporter::appendToSolarSystemConfigurationFile(SsoElements object)
{
	if (!object.contains("section_name") || object.value("section_name").toString().isEmpty())
	{
		qDebug() << "appendToSolarSystemConfigurationFile(): Invalid object:" << object;
		return false;
	}

	QList<SsoElements> list;
	list << object;
	return appendToSolarSystemConfigurationFile(list);
}

int CAImporter::unpackDayOrMonthNumber(QChar digit)
{
	//0-9, 0 is an invalid value, but the function is supposed to return 0 on failure.
	if (digit.isDigit())
	{
		return digit.digitValue();
	}

	if (digit.isUpper())
	{
		char letter = digit.toAscii();
		if (letter < 'A' || letter > 'V')
			return 0;
		return (10 + (letter - 'A'));
	}
	else
	{
		return 0;
	}
}

int CAImporter::unpackYearNumber (QChar prefix, int lastTwoDigits)
{
	int year = lastTwoDigits;
	if (prefix == 'I')
		year += 1800;
	else if (prefix == 'J')
		year += 1900;
	else if (prefix == 'K')
		year += 2000;
	else
		year = 0; //Error

	return year;
}

//Can be used both for minor planets and comets with no additional modification,
//as the regular expression for comets will match only capital letters.
int CAImporter::unpackAlphanumericNumber (QChar prefix, int lastDigit)
{
	int cycleCount = lastDigit;
	if (prefix.isDigit())
		cycleCount += prefix.digitValue() * 10;
	else if (prefix.isLetter() && prefix.isUpper())
		cycleCount += (10 + prefix.toAscii() - QChar('A').toAscii()) * 10;
	else if (prefix.isLetter() && prefix.isLower())
		cycleCount += (10 + prefix.toAscii() - QChar('a').toAscii()) * 10 + 26*10;
	else
		cycleCount = 0; //Error

	return cycleCount;
}

QString CAImporter::unpackMinorPlanetProvisionalDesignation (QString packedDesignation)
{
	QRegExp packedFormat("^([IJK])(\\d\\d)([A-Z])([\\dA-Za-z])(\\d)([A-Z])$");
	if (packedFormat.indexIn(packedDesignation) != 0)
	{
		QRegExp packedSurveyDesignation("^(PL|T1|T2|T3)S(\\d+)$");
		if (packedSurveyDesignation.indexIn(packedDesignation) == 0)
		{
			int number = packedSurveyDesignation.cap(2).toInt();
			if (packedSurveyDesignation.cap(1) == "PL")
			{
				return QString("%1 P-L").arg(number);
			}
			else if (packedSurveyDesignation.cap(1) == "T1")
			{
				return QString("%1 T-1").arg(number);
			}
			else if (packedSurveyDesignation.cap(1) == "T2")
			{
				return QString("%1 T-2").arg(number);
			}
			else
			{
				return QString("%1 T-3").arg(number);
			}
			//TODO: Are there any other surveys?
		}
		else
		{
			return QString();
		}
	}

	//Year
	QChar yearPrefix = packedFormat.cap(1).at(0);
	int yearLastTwoDigits = packedFormat.cap(2).toInt();
	int year = unpackYearNumber(yearPrefix, yearLastTwoDigits);

	//Letters
	QString halfMonthLetter = packedFormat.cap(3);
	QString secondLetter = packedFormat.cap(6);

	//Second letter cycle count
	QChar cycleCountPrefix = packedFormat.cap(4).at(0);
	int cycleCountLastDigit = packedFormat.cap(5).toInt();
	int cycleCount = unpackAlphanumericNumber(cycleCountPrefix, cycleCountLastDigit);

	//Assemble the unpacked provisional designation
	QString result = QString("%1 %2%3").arg(year).arg(halfMonthLetter).arg(secondLetter);
	if (cycleCount != 0)
	{
		result.append(QString::number(cycleCount));
	}

	return result;
}

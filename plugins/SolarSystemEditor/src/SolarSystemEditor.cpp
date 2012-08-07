/*
 * Solar System editor plug-in for Stellarium
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
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */

#include "SolarSystemEditor.hpp"
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


StelModule* SolarSystemEditorStelPluginInterface::getStelModule() const
{
	return new SolarSystemEditor();
}

StelPluginInfo SolarSystemEditorStelPluginInterface::getPluginInfo() const
{
	//Q_INIT_RESOURCE(solarSystemEditor);
	
	StelPluginInfo info;
	info.id = "SolarSystemEditor";
	info.displayedName = N_("Solar System Editor");
	info.authors = "Bogdan Marinov";
	info.contact = "http://stellarium.org";
	info.description = N_("An interface for adding asteroids and comets to Stellarium. It can download object lists from the Minor Planet Center's website and perform searches in its online database. Still a work in progress.");
	return info;
}

Q_EXPORT_PLUGIN2(SolarSystemEditor, SolarSystemEditorStelPluginInterface)

SolarSystemEditor::SolarSystemEditor()
{
	setObjectName("SolarSystemEditor");

	isInitialized = false;
	mainWindow = NULL;
	solarSystemConfigurationFile = NULL;
	solarSystemManager = GETSTELMODULE(SolarSystem);

	//I really hope that the file manager is instantiated before this
	defaultSolarSystemFilePath	= QFileInfo(StelFileMgr::getInstallationDir() + "/data/ssystem.ini").absoluteFilePath();
	customSolarSystemFilePath	= QFileInfo(StelFileMgr::getUserDir() + "/data/ssystem.ini").absoluteFilePath();
}

SolarSystemEditor::~SolarSystemEditor()
{
	if (solarSystemConfigurationFile != NULL)
	{
		delete solarSystemConfigurationFile;
	}
}

void SolarSystemEditor::init()
{
	//Get a list of the "default" Solar System objects' names:
	//TODO: Use it as validation for the loading of the plug-in
	if (QFile::exists(defaultSolarSystemFilePath))
	{
		defaultSsoIdentifiers = listAllLoadedObjectsInFile(defaultSolarSystemFilePath);
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

	connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(updateI18n()));
	isInitialized = true;
}

void SolarSystemEditor::deinit()
{
	//
}

void SolarSystemEditor::update(double) //deltaTime
{
	//
}

void SolarSystemEditor::draw(StelCore*, class StelRenderer*) //core
{
	//
}

double SolarSystemEditor::getCallOrder(StelModuleActionName) const// actionName
{
	return 0.;
}

bool SolarSystemEditor::configureGui(bool show)
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
			//solarSystemManager->reloadPlanets();
		}
	}
	return true;
}

void SolarSystemEditor::updateI18n()
{
	//The Solar System MUST be translated before updating the window
	//TODO: Remove this if/when you merge this module in the Solar System module
	solarSystemManager->updateI18n();
}

bool SolarSystemEditor::cloneSolarSystemConfigurationFile()
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

bool SolarSystemEditor::resetSolarSystemConfigurationFile()
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

void SolarSystemEditor::resetSolarSystemToDefault()
{
	if (isInitialized)
	{
		if (resetSolarSystemConfigurationFile())
		{
			//Deselect all currently selected objects
			StelObjectMgr * objectManager = GETSTELMODULE(StelObjectMgr);
			//TODO
			objectManager->unSelect();

			solarSystemManager->reloadPlanets();
			emit solarSystemChanged();
		}
	}
}

bool SolarSystemEditor::copySolarSystemConfigurationFileTo(QString filePath)
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

bool SolarSystemEditor::replaceSolarSystemConfigurationFileWith(QString filePath)
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
		solarSystemManager->reloadPlanets();
		emit solarSystemChanged();
		return true;
	}
	else
	{
		//TODO: Message
		if (cloneSolarSystemConfigurationFile())
		{
			solarSystemManager->reloadPlanets();
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

QHash<QString,QString> SolarSystemEditor::listAllLoadedObjectsInFile(QString filePath)
{
	if (!QFile::exists(filePath))
		return QHash<QString,QString>();

	QSettings solarSystem(filePath, QSettings::IniFormat);
	if (solarSystem.status() != QSettings::NoError)
		return QHash<QString,QString>();

	QStringList groups = solarSystem.childGroups();
	QStringList planetNames = solarSystemManager->getAllPlanetEnglishNames();
	QHash<QString,QString> loadedObjects;
	foreach (QString group, groups)
	{
		QString name = solarSystem.value(group + "/name").toString();
		if (planetNames.contains(name))
		{
			loadedObjects.insert(name, group);
		}
	}
	return loadedObjects;
}

QHash<QString,QString> SolarSystemEditor::listAllLoadedSsoIdentifiers()
{
	if (QFile::exists(customSolarSystemFilePath))
	{
		return listAllLoadedObjectsInFile(customSolarSystemFilePath);
	}
	else
	{
		//TODO: Error message
		return QHash<QString,QString>();
	}
}

bool SolarSystemEditor::removeSsoWithName(QString name)
{
	if (name.isEmpty())
		return false;

	//qDebug() << name;
	if (defaultSsoIdentifiers.keys().contains(name))
	{
		qWarning() << "You can't delete the default Solar System objects for the moment.";
		return false;
	}

	//Make sure that the file exists
	if (!QFile::exists(customSolarSystemFilePath))
	{
		qDebug() << "Can't remove" << name << "to ssystem.ini: Unable to find" << customSolarSystemFilePath;
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
	foreach (QString group, settings.childGroups())
	{
		if (settings.value(group + "/name").toString() == name)
		{
			settings.remove(group);
			settings.sync();
			break;
		}
	}

	//Deselect all currently selected objects
	//TODO: I bet that someone will complains, so: unselect only the removed one
	GETSTELMODULE(StelObjectMgr)->unSelect();

	//Reload the Solar System
	solarSystemManager->reloadPlanets();
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
  "0128P      b  2007 06 13.8064  3.062504  0.320891  210.3319  214.3583    4.3606  20100723   8.5  4.0  128P/Shoemaker-Holt                                      MPC 51822" -> fragment, fixed
  "0141P      d  2010 05 29.7106  0.757809  0.749215  149.3298  246.0849   12.8032  20100723  12.0 12.0  141P/Machholz                                            MPC 59599" -> fragment, fixed
*/
SsoElements SolarSystemEditor::readMpcOneLineCometElements(QString oneLineElements)
{
	SsoElements result;

	QRegExp mpcParser("^\\s*(\\d{4})?([A-Z])((?:\\w{6}|\\s{6})?[0a-zA-Z])?\\s+(\\d{4})\\s+(\\d{2})\\s+(\\d{1,2}\\.\\d{3,4})\\s+(\\d{1,2}\\.\\d{5,6})\\s+(\\d\\.\\d{5,6})\\s+(\\d{1,3}\\.\\d{3,4})\\s+(\\d{1,3}\\.\\d{3,4})\\s+(\\d{1,3}\\.\\d{3,4})\\s+(?:(\\d{4})(\\d\\d)(\\d\\d))?\\s+(\\-?\\d{1,2}\\.\\d)\\s+(\\d{1,2}\\.\\d)\\s+(\\S.{1,54}\\S)(?:\\s+(\\S.*))?$");//

	int match = mpcParser.indexIn(oneLineElements);
	//qDebug() << "RegExp captured:" << match << mpcParser.capturedTexts();

	if (match < 0)
	{
		qWarning() << "No match for" << oneLineElements;
		return result;
	}

	QString numberString = mpcParser.cap(1).trimmed();
	//QChar cometType = mpcParser.cap(2).at(0);
	QString provisionalDesignation = mpcParser.cap(3).trimmed();

	if (numberString.isEmpty() && provisionalDesignation.isEmpty())
	{
		qWarning() << "Comet is missing both comet number AND provisional designation.";
		return result;
	}

	QString name = mpcParser.cap(17).trimmed();

	//Fragment suffix
	if (provisionalDesignation.length() == 1)
	{
		QChar fragmentIndex = provisionalDesignation.at(0);
		name.append(' ');
		name.append(fragmentIndex.toUpper());
	}

	if (name.isEmpty())
	{
		return SsoElements();
	}
	result.insert("name", name);

	QString sectionName = convertToGroupName(name);
	if (sectionName.isEmpty())
	{
		return SsoElements();
	}
	result.insert("section_name", sectionName);

	//After a name has been determined, insert the essential keys
	result.insert("parent", "Sun");
	result.insert("type", "comet");
	//"comet_orbit" is used for all cases:
	//"ell_orbit" interprets distances as kilometers, not AUs
	result.insert("coord_func","comet_orbit");

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

SsoElements SolarSystemEditor::readMpcOneLineMinorPlanetElements(QString oneLineElements)
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
	if (name.isEmpty())
	{
		return SsoElements();
	}
	result.insert("name", name);

	//Section name
	QString sectionName = convertToGroupName(name, minorPlanetNumber);
	if (sectionName.isEmpty())
	{
		return SsoElements();
	}
	result.insert("section_name", sectionName);

	//After a name has been determined, insert the essential keys
	result.insert("parent", "Sun");
	result.insert("type", "asteroid");
	//"comet_orbit" is used for all cases:
	//"ell_orbit" interprets distances as kilometers, not AUs
	result.insert("coord_func","comet_orbit");

	result.insert("lighting", false);
	result.insert("color", "1.0, 1.0, 1.0");
	result.insert("tex_map", "nomap.png");

	//Magnitude and slope parameter
	column = oneLineElements.mid(8,5).trimmed();
	double absoluteMagnitude = column.toDouble(&ok);
	if (!ok)
		return SsoElements();
	column = oneLineElements.mid(14,5).trimmed();
	double slopeParameter = column.toDouble(&ok);
	if (!ok)
		return SsoElements();
	result.insert("absolute_magnitude", absoluteMagnitude);
	result.insert("slope_parameter", slopeParameter);

	//Orbital parameters
	column = oneLineElements.mid(37, 9).trimmed();
	double argumentOfPerihelion = column.toDouble(&ok);//J2000.0, degrees
	if (!ok)
		return SsoElements();
	result.insert("orbit_ArgOfPericenter", argumentOfPerihelion);

	column = oneLineElements.mid(48, 9).trimmed();
	double longitudeOfTheAscendingNode = column.toDouble(&ok);//J2000.0, degrees
	if (!ok)
		return SsoElements();
	result.insert("orbit_AscendingNode", longitudeOfTheAscendingNode);

	column = oneLineElements.mid(59, 9).trimmed();
	double inclination = column.toDouble(&ok);//J2000.0, degrees
	if (!ok)
		return SsoElements();
	result.insert("orbit_Inclination", inclination);

	column = oneLineElements.mid(70, 9).trimmed();
	double eccentricity = column.toDouble(&ok);//degrees
	if (!ok)
		return SsoElements();
	result.insert("orbit_Eccentricity", eccentricity);

	column = oneLineElements.mid(80, 11).trimmed();
	double meanDailyMotion = column.toDouble(&ok);//degrees per day
	if (!ok)
		return SsoElements();
	result.insert("orbit_MeanMotion", meanDailyMotion);

	column = oneLineElements.mid(92, 11).trimmed();
	double semiMajorAxis = column.toDouble(&ok);
	if (!ok)
		return SsoElements();
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
	//Epoch is at .0 TT, i.e. midnight
	double epochJD;
	StelUtils::getJDFromDate(&epochJD, year, month, day, 0, 0, 0);
	result.insert("orbit_Epoch", epochJD);

	column = oneLineElements.mid(26, 9).trimmed();
	double meanAnomalyAtEpoch = column.toDouble(&ok);//degrees
	if (!ok)
		return SsoElements();
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

SsoElements SolarSystemEditor::readXEphemOneLineElements(QString oneLineElements)
{
	SsoElements result;

	enum OrbitType {Elliptic, Hyperbolic, Parabolic} orbitType;

	QStringList fields = oneLineElements.split(',');
	if (fields.isEmpty() || fields.count() < 10 || fields.count() > 14)
		return result;
	//qDebug() << fields;

	QString name = fields.at(0).trimmed();
	if (name.isEmpty() || fields.at(1).isEmpty())
		return result;

	QChar orbitTypeFlag = fields.at(1).trimmed().at(0);
	if (orbitTypeFlag == 'e')
		orbitType = Elliptic;
	else if(orbitTypeFlag == 'h')
		orbitType = Hyperbolic;
	else if (orbitTypeFlag == 'p')
		orbitType = Parabolic;
	else
	{
		qDebug() << "Unrecognised orbit type:" << orbitTypeFlag;
		return result;
	}

	//"comet_orbit" is used for all cases:
	//"ell_orbit" interprets distances as kilometers, not AUs
	result.insert("coord_func", "comet_orbit");

	//Type detection and name parsing
	QString objectType;
	int minorPlanetNumber = 0;
	QRegExp cometProvisionalDesignationStart("^[PCDX]/");
	QRegExp cometDesignationStart("^(\\d)+[PCDX]/");
	if (cometDesignationStart.indexIn(name) == 0 ||
		cometProvisionalDesignationStart.indexIn(name) == 0)
	{
		objectType = "comet";
	}
	else
	{
		objectType = "asteroid";
		QRegExp asteroidProvisionalDesignation("(\\d{4}\\s[A-Z]{2})(\\d*)$");
		int pdIndex = asteroidProvisionalDesignation.indexIn(name);
		if (pdIndex != 0)
		{
			int spaceIndex = name.indexOf(' ');
			if (spaceIndex > 0)
			{
				QString numberString = name.left(spaceIndex);
				//qDebug() << numberString;
				minorPlanetNumber = numberString.toInt();
				if (minorPlanetNumber)
					name = name.right(name.length() - spaceIndex - 1);
				//qDebug() << name;
			}
		}
	}
	if (name.isEmpty())
	{
		return SsoElements();
	}
	result.insert("name", name);
	result.insert("type", objectType);
	if (minorPlanetNumber)
		result.insert("minor_planet_number", minorPlanetNumber);

	//Section name
	QString sectionName = convertToGroupName(name, minorPlanetNumber);
	if (sectionName.isEmpty())
	{
		return SsoElements();
	}
	result.insert("section_name", sectionName);

	//After a name has been determined, insert the essential keys
	result.insert("parent", "Sun");

	result.insert("lighting", false);
	result.insert("color", "1.0, 1.0, 1.0");
	result.insert("tex_map", "nomap.png");

	//Orbital elements
	bool ok;
	QString field;

	if (orbitType == Elliptic)
		field = fields.at(2);//Field 3
	else
		field = fields.at(3);//Field 4
	double inclination = field.trimmed().toDouble(&ok);
	if (!ok)
		return SsoElements();
	result.insert("orbit_Inclination", inclination);

	if (orbitType == Elliptic)
		field = fields.at(3);//Field 4
	else if (orbitType == Hyperbolic)
		field = fields.at(4);//Field 5
	else
		field = fields.at(6);//Field 7
	double longitudeOfTheAscendingNode = field.toDouble(&ok);//J2000.0, degrees
	if (!ok)
		return SsoElements();
	result.insert("orbit_AscendingNode", longitudeOfTheAscendingNode);

	if (orbitType == Hyperbolic)
		field = fields.at(5);//Field 6
	else
		field = fields.at(4);//Field 5
	double argumentOfPerihelion = field.toDouble(&ok);//J2000.0, degrees
	if (!ok)
		return SsoElements();
	result.insert("orbit_ArgOfPericenter", argumentOfPerihelion);

	if (orbitType == Elliptic)
	{
		field = fields.at(5);//Field 6
		double semiMajorAxis = field.toDouble(&ok);
		if (!ok)
			return SsoElements();
		result.insert("orbit_SemiMajorAxis", semiMajorAxis);

		field = fields.at(6);//Field 7
		double meanDailyMotion = field.toDouble(&ok);//degrees per day
		if (!ok)
			return SsoElements();
		result.insert("orbit_MeanMotion", meanDailyMotion);
	}

	double eccentricity;
	if (orbitType == Elliptic)
		eccentricity = fields.at(7).toDouble(&ok);//Field 8
	else if (orbitType == Hyperbolic)
		eccentricity = fields.at(6).toDouble(&ok);//Field 7
	else
	{
		//Parabolic orbit
		eccentricity = 1.0;
		ok = true;
	}
	if (!ok)
		return SsoElements();
	result.insert("orbit_Eccentricity", eccentricity);

	if (orbitType == Elliptic)
	{
		double meanAnomalyAtEpoch = fields.at(8).toDouble(&ok);//degrees
		if (!ok)
			return SsoElements();
		result.insert("orbit_MeanAnomaly", meanAnomalyAtEpoch);
	}

	if (orbitType == Elliptic)
		field = fields.at(9);//Field 10
	else
		field = fields.at(2);//Field 3
	QStringList dateStrings = field.trimmed().split('/');
	//TODO: Validation
	int year	= dateStrings.at(2).toInt();
	int month	= dateStrings.at(0).toInt();
	double dayFraction	= dateStrings.at(1).toDouble(&ok);
	int day = (int) dayFraction;
	QDate date(year, month, day);
	int fraction = (int) ((dayFraction - day) * 24 * 60 * 60);
	int seconds = fraction % 60; fraction /= 60;
	int minutes = fraction % 60; fraction /= 60;
	int hours = fraction % 24;
	//qDebug() << hours << minutes << seconds << fraction;
	QTime time(hours, minutes, seconds, 0);
	QDateTime dt(date, time, Qt::UTC);
	double jd = StelUtils::qDateTimeToJd(dt);
	if (orbitType == Elliptic)
		result.insert("orbit_Epoch", jd);
	else
		result.insert("orbit_TimeAtPericenter", jd);

	if (orbitType != Elliptic)
	{
		if (orbitType == Hyperbolic)
			field = fields.at(7);//Field 8
		else
			field = fields.at(5);//Field 6
		double perihelionDistance = field.toDouble(&ok);//AU
		if (!ok)
			return SsoElements();
		result.insert("orbit_PericenterDistance", perihelionDistance);
	}

	//Magnitude
	if (orbitType == Elliptic)
		field = fields.at(11);//Field 12
	else if (orbitType == Hyperbolic)
		field = fields.at(9);//Field 10
	else
		field = fields.at(8);//Field 9
	QRegExp magnitudePrefix("^([Hg]\\s*)?(\\d.+)");
	if (magnitudePrefix.indexIn(field) != 0)
		return SsoElements();
	field = magnitudePrefix.cap(2);
	double absoluteMagnitude = field.toDouble(&ok);
	if (!ok)
		return SsoElements();
	result.insert("absolute_magnitude", absoluteMagnitude);

	if (orbitType == Elliptic)
		field = fields.at(12);//Field 13
	else if (orbitType == Hyperbolic)
		field = fields.at(10);//Field 11
	else
		field = fields.at(9);//Field 10
	double slopeParameter = field.toDouble(&ok);
	if (!ok)
		return SsoElements();
	result.insert("slope_parameter", slopeParameter);

	//Radius and albedo
	double albedo = 0.04;
	double radius = 5.0;
	if (objectType == "asteroid")
	{
		//Assume albedo of 0.15 and calculate a radius based on the absolute magnitude
		//http://www.physics.sfasu.edu/astro/asteroids/sizemagnitude.html
		albedo = 0.15;
		radius = std::ceil((1329 / std::sqrt(albedo)) * std::pow(10, -0.2 * absoluteMagnitude));
	}
	result.insert("albedo", albedo);
	result.insert("radius", radius);

	return result;
}

QList<SsoElements> SolarSystemEditor::readMpcOneLineCometElementsFromFile(QString filePath)
{
	QList<SsoElements> objectList;

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

QList<SsoElements> SolarSystemEditor::readMpcOneLineMinorPlanetElementsFromFile(QString filePath)
{
	QList<SsoElements> objectList;

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

QList<SsoElements> SolarSystemEditor::readXEphemOneLineElementsFromFile(QString filePath)
{
	QList<SsoElements> objectList;

	if (!QFile::exists(filePath))
	{
		qDebug() << "Can't find" << filePath;
		return objectList;
	}

	QFile xEphemElementsFile(filePath);
	if (xEphemElementsFile.open(QFile::ReadOnly | QFile::Text ))
	{
		int candidatesCount = 0;
		int lineCount = 0;

		while(!xEphemElementsFile.atEnd())
		{
			QString oneLineElements = QString(xEphemElementsFile.readLine());
			if(oneLineElements.endsWith('\n'))
			{
				oneLineElements.chop(1);
			}
			if (oneLineElements.isEmpty())
			{
				qDebug() << "Empty line skipped.";
				continue;
			}
			if (oneLineElements.startsWith('#'))
			{
				qDebug() << "Comment skipped.";
				continue;
			}
			lineCount++;

			SsoElements ssObject = readXEphemOneLineElements(oneLineElements);
			if(!ssObject.isEmpty() && !ssObject.value("section_name").toString().isEmpty())
			{
				objectList << ssObject;
				candidatesCount++;
			}
		}
		xEphemElementsFile.close();
		qDebug() << "Done reading minor planet orbital elements."
				 << "Recognized" << candidatesCount << "candidate objects"
				 << "out of" << lineCount << "lines.";

		return objectList;
	}
	else
	{
		qDebug() << "Unable to open for reading" << filePath;
		qDebug() << "File error:" << xEphemElementsFile.errorString();
		return objectList;
	}

	return objectList;
}

bool SolarSystemEditor::appendToSolarSystemConfigurationFile(QList<SsoElements> objectList)
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


	QHash<QString,QString> loadedObjects = listAllLoadedSsoIdentifiers();

	//Remove duplicates (identified by name, not by section name)
	QSettings * solarSystemSettings = new QSettings(customSolarSystemFilePath, QSettings::IniFormat);
	if (solarSystemSettings->status() != QSettings::NoError)
	{
		qDebug() << "Error opening ssystem.ini:" << customSolarSystemFilePath;
		return false;
	}
	foreach (SsoElements object, objectList)
	{
		QString name = object.value("name").toString();
		if (name.isEmpty())
			continue;

		QString group = object.value("section_name").toString();
		if (group.isEmpty())
			continue;

		if (loadedObjects.contains(name))
		{
			solarSystemSettings->remove(loadedObjects.value(name));
			loadedObjects.remove(name);
		}
		else if (solarSystemSettings->childGroups().contains(group))
		{
			loadedObjects.remove(solarSystemSettings->value(group + "/name").toString());
			solarSystemSettings->remove(group);
		}
	}
	solarSystemSettings->sync();
	delete solarSystemSettings;
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

			QString name = object.value("name").toString();
			if (name.isEmpty())
				continue;

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

bool SolarSystemEditor::appendToSolarSystemConfigurationFile(SsoElements object)
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

bool SolarSystemEditor::updateSolarSystemConfigurationFile(QList<SsoElements> objectList, UpdateFlags flags)
{
	if (objectList.isEmpty())
	{
		//Empty lists can be added without any problem. :)
		qWarning() << "updateSolarSystemConfigurationFile(): The source list is empty.";
		return true;
	}

	//Check if the configuration file exists
	if (!QFile::exists(customSolarSystemFilePath))
	{
		qDebug() << "Can't update ssystem.ini: Unable to find" << customSolarSystemFilePath;
		return false;
	}

	QSettings solarSystem(customSolarSystemFilePath, QSettings::IniFormat);
	if (solarSystem.status() != QSettings::NoError)
	{
		qDebug() << "Error opening ssystem.ini:" << customSolarSystemFilePath;
		return false;
	}
	QStringList existingSections = solarSystem.childGroups();
	QHash<QString,QString> loadedObjects = listAllLoadedSsoIdentifiers();
	//TODO: Move to contstructor?
	QStringList orbitalElementsKeys;
	orbitalElementsKeys << "coord_func"
			<< "orbit_ArgOfPericenter"
			<< "orbit_AscendingNode"
			<< "orbit_Eccentricity"
			<< "orbit_Epoch"
			<< "orbit_Inclination"
			<< "orbit_LongOfPericenter"
			<< "orbit_MeanAnomaly"
			<< "orbit_MeanLongitude"
			<< "orbit_MeanMotion"
			<< "orbit_PericenterDistance"
			<< "orbit_Period"
			<< "orbit_SemiMajorAxis"
			<< "orbit_TimeAtPericenter";

	qDebug() << "Updating objects...";
	foreach (SsoElements object, objectList)
	{
		QString name = object.value("name").toString();
		if (name.isEmpty())
			continue;

		QString sectionName = object.value("section_name").toString();
		if (sectionName.isEmpty())
			continue;
		object.remove("section_name");

		if (loadedObjects.contains(name))
		{
			if (sectionName != loadedObjects.value(name))
			{
				//Is this a name conflict between an asteroid and a moon?
				QString currentParent = solarSystem.value(loadedObjects.value(name) + "/parent").toString();
				QString newParent = object.value("parent").toString();
				if (newParent != currentParent)
				{
					name.append('*');
					object.insert("name", name);

					if (!existingSections.contains(sectionName))
					{
						solarSystem.beginGroup(sectionName);
						foreach (QString property, object.keys())
						{
							solarSystem.setValue(property, object.value(property));
						}
						solarSystem.endGroup();
					}
				}
				else
				{
					//If the parent is the same, update that object
					sectionName = loadedObjects.value(name);
				}
			}
		}
		else
		{
			qDebug() << "Skipping update of" << sectionName << ", as no object with this name exists.";
			continue;
		}

		solarSystem.beginGroup(sectionName);

		if (flags.testFlag(UpdateNameAndNumber))
		{
			updateSsoProperty(solarSystem, object, "name");
			updateSsoProperty(solarSystem, object, "minor_planet_number");
		}

		if (flags.testFlag(UpdateType))
		{
			updateSsoProperty(solarSystem, object, "type");
		}

		if (flags.testFlag(UpdateOrbitalElements))
		{
			//Remove all orbital elements first, in case
			//the new ones use another coordinate function
			foreach (QString key, orbitalElementsKeys)
			{
				solarSystem.remove(key);
			}

			foreach (QString key, orbitalElementsKeys)
			{
				updateSsoProperty(solarSystem, object, key);
			}
		}

		if (flags.testFlag(UpdateMagnitudeParameters))
		{
			if (object.contains("absolute_magnitude") && object.contains("slope_parameter"))
			{
				QString type = solarSystem.value("type").toString();
				if (type == "asteroid" || type == "comet" )
				{
					updateSsoProperty(solarSystem, object, "absolute_magnitude");
					updateSsoProperty(solarSystem, object, "slope_parameter");
				}
				else
				{
					//TODO: Do what, log a message?
				}
			}
		}

		solarSystem.endGroup();
		qDebug() << "Updated successfully" << sectionName;
	}

	return true;
}

void SolarSystemEditor::updateSsoProperty(QSettings & settings, SsoElements & properties, QString key)
{
	if (properties.contains(key))
	{
		settings.setValue(key, properties.value(key));
	}
}

QString SolarSystemEditor::convertToGroupName(QString &name, int minorPlanetNumber)
{
	//TODO: Should I remove all non-alphanumeric, or only the obviously problematic?
	QString groupName(name);
	groupName.remove('\\');
	groupName.remove('/');
	groupName.remove('#');
	groupName.remove(' ');
	groupName.remove('-');
	groupName = groupName.toLower();

	//To prevent mix-up between asteroids and satellites:
	//insert the minor planet number in the section name
	//(if an asteroid is named, it must be numbered)
	if (minorPlanetNumber)
	{
		groupName.prepend(QString::number(minorPlanetNumber));
	}

	return groupName;
}

int SolarSystemEditor::unpackDayOrMonthNumber(QChar digit)
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

int SolarSystemEditor::unpackYearNumber (QChar prefix, int lastTwoDigits)
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
int SolarSystemEditor::unpackAlphanumericNumber (QChar prefix, int lastDigit)
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

QString SolarSystemEditor::unpackMinorPlanetProvisionalDesignation (QString packedDesignation)
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

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

#include "StelUtils.hpp"
#include "StelApp.hpp"
#include "StelGui.hpp"
#include "StelGuiItems.hpp"
#include "StelFileMgr.hpp"
#include "StelIniParser.hpp"
#include "StelLocaleMgr.hpp"
#include "StelModuleMgr.hpp"
#include "StelObjectMgr.hpp"
#include "SolarSystem.hpp"
#include "Orbit.hpp"

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
	info.contact = STELLARIUM_URL;
	info.description = N_("An interface for adding asteroids and comets to Stellarium. It can download object lists from the Minor Planet Center's website and perform searches in its online database.");
	info.version = SOLARSYSTEMEDITOR_PLUGIN_VERSION;
	info.license = SOLARSYSTEMEDITOR_PLUGIN_LICENSE;
	return info;
}

SolarSystemEditor::SolarSystemEditor():
	isInitialized(false),
	mainWindow(Q_NULLPTR),
	solarSystemConfigurationFile(Q_NULLPTR)
{
	setObjectName("SolarSystemEditor");
	solarSystem = GETSTELMODULE(SolarSystem);

	//I really hope that the file manager is instantiated before this
	// GZ new: Not sure whether this should be major or minor here.
	defaultSolarSystemFilePath	= QFileInfo(StelFileMgr::getInstallationDir() + "/data/ssystem_minor.ini").absoluteFilePath();
	majorSolarSystemFilePath	= QFileInfo(StelFileMgr::getInstallationDir() + "/data/ssystem_major.ini").absoluteFilePath();
	customSolarSystemFilePath	= QFileInfo(StelFileMgr::getUserDir() + "/data/ssystem_minor.ini").absoluteFilePath();
}

SolarSystemEditor::~SolarSystemEditor()
{
	if (solarSystemConfigurationFile != Q_NULLPTR)
	{
		delete solarSystemConfigurationFile;
	}
}

void SolarSystemEditor::init()
{
	//Get a list of the "default" minor Solar System objects' names:
	//TODO: Use it as validation for the loading of the plug-in
	// GZ TBD: Maybe only load the names from ssystem_major.ini, so that even default minor bodies can be reduced by the user.
	// The current solution loads the default major objects and thus prevents Pluto from deletion.
//	if (QFile::exists(defaultSolarSystemFilePath))
//	{
//		// GZ We no longer prevent users from deleting minor bodies even when they came in the default file!
//		//defaultSsoIdentifiers = listAllLoadedObjectsInFile(defaultSolarSystemFilePath);
//	}
//	else
//	{
//		//TODO: Better error message
//		qDebug() << "SolarSystemEditor: Something is horribly wrong with installdir (minor)" << QDir::toNativeSeparators(StelFileMgr::getInstallationDir());
//		return;
//	}
	if (QFile::exists(majorSolarSystemFilePath))
	{
		//defaultSsoIdentifiers.unite(listAllLoadedObjectsInFile(majorSolarSystemFilePath));
		defaultSsoIdentifiers = listAllLoadedObjectsInFile(majorSolarSystemFilePath);
	}
	else
	{
		qDebug() << "SolarSystemEditor: Something is horribly wrong with installdir (major)" << QDir::toNativeSeparators(StelFileMgr::getInstallationDir());
		return;
	}

	try
	{
		//Make sure that a user ssystem_minor.ini actually exists
		if (!cloneSolarSystemConfigurationFile())
		{
			qWarning() << "SolarSystemEditor: Cannot copy ssystem_minor.ini to user data directory. Plugin will not work.";
			return;
		}

		mainWindow = new SolarSystemManagerWindow();
	}
	catch (std::runtime_error &e)
	{
		qWarning() << "init() error: " << e.what();
		return;
	}

	connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(updateI18n()));
	isInitialized = true;

	// key bindings and other actions
	addAction("actionShow_MPC_Import", N_("Solar System Editor"), N_("Import orbital elements in MPC format..."), mainWindow, "newImportMPC()", "Ctrl+Alt+S");
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
	//NOTE: Remove this if/when you merge this module in the Solar System module
	solarSystem->updateI18n();
}

bool SolarSystemEditor::cloneSolarSystemConfigurationFile() const
{
	QDir userDataDirectory(StelFileMgr::getUserDir());
	if (!userDataDirectory.exists())
	{
		qCritical() << "Unable to find user data directory:" << QDir::toNativeSeparators(userDataDirectory.absolutePath());
		return false;
	}
	if (!userDataDirectory.exists("data") && !userDataDirectory.mkdir("data"))
	{
		qDebug() << "Unable to create a \"data\" subdirectory in" << QDir::toNativeSeparators(userDataDirectory.absolutePath());
		return false;
	}

	if (QFile::exists(customSolarSystemFilePath))
	{
		qDebug() << "Using the ssystem_minor.ini file that already exists in the user directory...";
		return true;
	}

	if (QFile::exists(defaultSolarSystemFilePath))
	{
		qDebug() << "Trying to copy ssystem_minor.ini to" << QDir::toNativeSeparators(customSolarSystemFilePath);
		return QFile::copy(defaultSolarSystemFilePath, customSolarSystemFilePath);
	}
	else
	{
		qCritical() << "SolarSystemEditor: Cannot find default ssystem_minor.ini. This branch should be unreachable.";
		Q_ASSERT(0);
		return false;
	}
}

bool SolarSystemEditor::resetSolarSystemConfigurationFile() const
{
	if (QFile::exists(customSolarSystemFilePath))
	{
		if (!QFile::remove((customSolarSystemFilePath)))
		{
			qWarning() << "Unable to delete" << QDir::toNativeSeparators(customSolarSystemFilePath)
				 << StelUtils::getEndLineChar() << "Please remove the file manually.";
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

			solarSystem->reloadPlanets();
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
		qCritical() << "SolarSystemEditor: Cannot replace, file not found: " << filePath;
		return false;
	}

	//Is it a valid configuration file?
	QSettings settings(filePath, StelIniFormat);
	if (settings.status() != QSettings::NoError)
	{
		qCritical() << "SolarSystemEditor: " << QDir::toNativeSeparators(filePath) << "is not a valid configuration file.";
		return false;
	}

	//Remove the existingfile
	if (QFile::exists(customSolarSystemFilePath))
	{
		if(!QFile::remove(customSolarSystemFilePath))
		{
			qCritical() << "SolarSystemEditor: Cannot remove old file.";
			return false;
		}
	}

	//Copy the new file
	//If the copy fails, reset to the default configuration
	if (QFile::copy(filePath, customSolarSystemFilePath))
	{
		solarSystem->reloadPlanets();
		emit solarSystemChanged();
		return true;
	}
	else
	{
		qWarning() << "SolarSystemEditor: Could not replace file. Restoring default.";
		if (cloneSolarSystemConfigurationFile())
		{
			solarSystem->reloadPlanets();
			emit solarSystemChanged();
			return true;
		}
		else
		{
			qCritical() << "SolarSystemEditor: Could neither replace nor then restore default file. This comes unexpected.";
			return false;
		}
	}
}

bool SolarSystemEditor::addFromSolarSystemConfigurationFile(QString filePath)
{
	if (!QFile::exists(filePath))
	{
		qCritical() << "SolarSystemEditor: Cannot add data from file, file" << filePath << "not found.";
		return false;
	}

	//Is it a valid configuration file?
	QSettings newData(filePath, StelIniFormat);
	if (newData.status() != QSettings::NoError)
	{
		qCritical() << "SolarSystemEditor: Cannot add data from file," << QDir::toNativeSeparators(filePath) << "is not a valid configuration file.";
		return false;
	}

	//Process the existing and new files:
	if (QFile::exists(customSolarSystemFilePath))
	{
		QSettings minorBodies(customSolarSystemFilePath, StelIniFormat);

		// add and overwrite existing data in the user's ssystem_minor.ini by the data in the new file.
		qDebug() << "ADD OBJECTS: Data for " << newData.childGroups().count() << "objects to minor file with " << minorBodies.childGroups().count() << "entries";
		for (auto group : newData.childGroups())
		{
			QString fixedGroupName = fixGroupName(group);
			newData.beginGroup(group);
			qDebug() << "  Group: " << group << "for object " << newData.value("name");
			qDebug() << "   ";
			QStringList minorChildGroups = minorBodies.childGroups();
			if (minorChildGroups.contains(fixedGroupName))
			{
				qDebug() << "This group " << fixedGroupName << "already exists. Updating values";
			}
			else
				qDebug() << "This group " << fixedGroupName << "does not yet exist. Adding values";

			minorBodies.beginGroup(fixedGroupName);
			QStringList newKeys = newData.allKeys(); // limited to the group!
			for (auto key : newKeys)
			{
				minorBodies.setValue(key, newData.value(key));
			}
			minorBodies.endGroup();
			newData.endGroup();
		}
		minorBodies.sync();
		qDebug() << "Minor groups now: " << minorBodies.childGroups();
		qDebug() << "Checking for stupid General group.";
		// There may be a generic group "General" in the updated file, created from comments. We must remove it.
		if (minorBodies.childGroups().contains("General"))
		{
			minorBodies.remove("General");
		}
		qDebug() << "Minor groups after fix now: " << minorBodies.childGroups();
		minorBodies.sync();

		solarSystem->reloadPlanets();
		emit solarSystemChanged();
		return true;
	}
	else
	{
		qCritical() << "SolarSystemEditor: Cannot add data to use file," << QDir::toNativeSeparators(customSolarSystemFilePath) << "not found.";
		return false;
	}
}

QHash<QString,QString> SolarSystemEditor::listAllLoadedObjectsInFile(QString filePath) const
{
	if (!QFile::exists(filePath))
		return QHash<QString,QString>();

	QSettings solarSystemIni(filePath, StelIniFormat);
	if (solarSystemIni.status() != QSettings::NoError)
		return QHash<QString,QString>();

	QStringList groups = solarSystemIni.childGroups();
	QStringList minorBodies = solarSystem->getAllMinorPlanetCommonEnglishNames();
	QHash<QString,QString> loadedObjects;
	for (auto group : groups)
	{
		QString name = solarSystemIni.value(group + "/name").toString();
		if (minorBodies.contains(name))
		{
			loadedObjects.insert(name, group);
		}
	}
	return loadedObjects;
}

QHash<QString,QString> SolarSystemEditor::listAllLoadedSsoIdentifiers() const
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
//	if (defaultSsoIdentifiers.keys().contains(name))
//	{
//		qWarning() << "You can't delete the default Solar System objects like" << name << "for the moment.";
//		qCritical() << "As of 0.16, this line should be impossible to reach!";
//		return false;
//	}

	//Make sure that the file exists
	if (!QFile::exists(customSolarSystemFilePath))
	{
		qDebug() << "Can't remove" << name << "from ssystem_minor.ini: Unable to find" << QDir::toNativeSeparators(customSolarSystemFilePath);
		return false;
	}

	//Open the file
	QSettings settings(customSolarSystemFilePath, StelIniFormat);
	if (settings.status() != QSettings::NoError)
	{
		qDebug() << "Error opening ssystem_minor.ini:" << QDir::toNativeSeparators(customSolarSystemFilePath);
		return false;
	}

	//Remove the section
	for (auto group : settings.childGroups())
	{
		if (settings.value(group + "/name").toString() == name)
		{
			settings.remove(group);
			settings.sync();
			break;
		}
	}

	//Deselect all currently selected objects
	//TODO: I bet that someone will complain, so: unselect only the removed one
	GETSTELMODULE(StelObjectMgr)->unSelect();

	//Reload the Solar System
	//solarSystem->reloadPlanets();
	// Better: just remove this one object!
	solarSystem->removeMinorPlanet(name);
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
SsoElements SolarSystemEditor::readMpcOneLineCometElements(QString oneLineElements) const
{
	SsoElements result;
	//qDebug() << "readMpcOneLineCometElements started...";

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
	//result.insert("parent", "Sun"); // 0.16: omit obvious default.
	result.insert("type", "comet");
	//"kepler_orbit" is used for all cases:
	//"ell_orbit" interprets distances as kilometers, not AUs
	// result.insert("coord_func", "kepler_orbit"); // 0.20: omit default
	result.insert("coord_func", "comet_orbit"); // 0.20: add this default for compatibility with earlier versions!
	// GZ: moved next line below!
	//result.insert("orbit_good", 1000); // default validity for osculating elements, days

	//result.insert("color", "1.0, 1.0, 1.0");  // 0.16: omit obvious default.
	//result.insert("tex_map", "nomap.png");    // 0.16: omit obvious default.

	bool ok = false;
	//TODO: Use this for VALIDATION!

	int year	= mpcParser.cap(4).toInt();
	int month	= mpcParser.cap(5).toInt();
	double dayFraction	= mpcParser.cap(6).toDouble(&ok);
	int day = static_cast<int>(dayFraction);
	QDate datePerihelionPassage(year, month, day);
	int fraction = static_cast<int>((dayFraction - day) * 24 * 60 * 60);
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

	double eccentricity = mpcParser.cap(8).toDouble(&ok);//NOT degrees, but without dimension.
	result.insert("orbit_Eccentricity", eccentricity);

	double argumentOfPerihelion = mpcParser.cap(9).toDouble(&ok);//J2000.0, degrees
	result.insert("orbit_ArgOfPericenter", argumentOfPerihelion);

	double longitudeOfTheAscendingNode = mpcParser.cap(10).toDouble(&ok);//J2000.0, degrees
	result.insert("orbit_AscendingNode", longitudeOfTheAscendingNode);

	double inclination = mpcParser.cap(11).toDouble(&ok);
	result.insert("orbit_Inclination", inclination);

	// GZ: We should reduce orbit_good for elliptical orbits to one half period before/after perihel!
	if (eccentricity < 1.0)
	{
		// Heafner, Fundamental Ephemeris Computations, p.71
		const double mu=(0.01720209895*0.01720209895); // GAUSS_GRAV_CONST^2
		const double a=perihelionDistance/(1.-eccentricity); // semimajor axis.
		const double meanMotion=std::sqrt(mu/(a*a*a)); // radians/day
		double period=M_PI*2.0 / meanMotion; // period, days
		result.insert("orbit_good", qMin(1000, static_cast<int>(floor(0.5*period)))); // validity for elliptical osculating elements, days. Goes from aphel to next aphel or max 1000 days.		
	}
	else
		result.insert("orbit_good", 1000); // default validity for osculating elements, days

	double absoluteMagnitude = mpcParser.cap(15).toDouble(&ok);
	result.insert("absolute_magnitude", absoluteMagnitude);

	//This is not the same "slope parameter" as used in asteroids. Better name?
	double slopeParameter = mpcParser.cap(16).toDouble(&ok);
	result.insert("slope_parameter", slopeParameter);

	result.insert("radius", 5); //Fictitious default assumption
	result.insert("albedo", 0.1); // GZ 2014-01-10: Comets are very dark, should even be 0.03!
	result.insert("dust_lengthfactor", 0.4); // dust tail length w.r.t. gas tail length
	result.insert("dust_brightnessfactor", 1.5); // dust tail brightness w.r.t. gas tail.
	result.insert("dust_widthfactor", 1.5); // opening w.r.t. gas tail opening width.
	//qDebug() << "readMpcOneLineCometElements done\n";
	return result;
}

SsoElements SolarSystemEditor::readMpcOneLineMinorPlanetElements(QString oneLineElements) const
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
	QString objectType = "asteroid";
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
				minorPlanetNumber += ((10 + prefix.toLatin1() - 'A') * 10000);
			}
			else
			{
				minorPlanetNumber += ((10 + prefix.toLatin1() - 'a' + 26) * 10000);
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
	//result.insert("parent", "Sun");	 // 0.16: omit obvious default.
	//"kepler_orbit" is used for all cases:
	//"ell_orbit" interprets distances as kilometers, not AUs
	//result.insert("coord_func","kepler_orbit"); // 0.20: omit default
	result.insert("coord_func", "comet_orbit"); // 0.20: add this default for compatibility with earlier versions!

	//result.insert("color", "1.0, 1.0, 1.0"); // 0.16: omit obvious default.
	//result.insert("tex_map", "nomap.png");   // 0.16: omit obvious default.

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
		qWarning() << "readMpcOneLineMinorPlanetElements():"
		         << column << "is not a date in packed format";
		return SsoElements();
	}
	int year = packedDateFormat.cap(2).toInt();
	switch (packedDateFormat.cap(1).at(0).toLatin1())
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
		qWarning() << "readMpcOneLineMinorPlanetElements():"
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

	// add period for visualization of orbit
	if (semiMajorAxis>0)
		result.insert("orbit_visualization_period", KeplerOrbit::calculateSiderealPeriod(semiMajorAxis, 1.));

	// 2:3 resonance to Neptune [https://en.wikipedia.org/wiki/Plutino]
	if (static_cast<int>(semiMajorAxis) == 39)
		objectType = "plutino";

	// Classical Kuiper belt objects [https://en.wikipedia.org/wiki/Classical_Kuiper_belt_object]
	if (semiMajorAxis>=40 && semiMajorAxis<=50)
		objectType = "cubewano";

	// Calculate perihelion
	const double q = (1 - eccentricity)*semiMajorAxis;

	// Scattered disc objects
	if (q > 35)
		objectType = "scattered disc object";

	// Sednoids [https://en.wikipedia.org/wiki/Planet_Nine]
	if (q > 30 && semiMajorAxis > 250)
		objectType = "sednoid";

	//Radius and albedo
	//Assume albedo of 0.15 and calculate a radius based on the absolute magnitude
	//as described here: http://www.physics.sfasu.edu/astro/asteroids/sizemagnitude.html
	double albedo = 0.15; //Assumed
	double radius = std::ceil(0.5*(1329 / std::sqrt(albedo)) * std::pow(10, -0.2 * absoluteMagnitude)); // Original formula is for diameter!
	result.insert("albedo", albedo);
	result.insert("radius", radius);
	result.insert("type", objectType);

	return result;
}

QList<SsoElements> SolarSystemEditor::readMpcOneLineCometElementsFromFile(QString filePath) const
{
	QList<SsoElements> objectList;

	if (!QFile::exists(filePath))
	{
		qDebug() << "Can't find" << QDir::toNativeSeparators(filePath);
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
		qDebug() << "Unable to open for reading" << QDir::toNativeSeparators(filePath);
		qDebug() << "File error:" << mpcElementsFile.errorString();
		return objectList;
	}
}

QList<SsoElements> SolarSystemEditor::readMpcOneLineMinorPlanetElementsFromFile(QString filePath) const
{
	QList<SsoElements> objectList;

	if (!QFile::exists(filePath))
	{
		qDebug() << "Can't find" << QDir::toNativeSeparators(filePath);
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
		qDebug() << "Unable to open for reading" << QDir::toNativeSeparators(filePath);
		qDebug() << "File error:" << mpcElementsFile.errorString();
		return objectList;
	}
}

bool SolarSystemEditor::appendToSolarSystemConfigurationFile(QList<SsoElements> objectList)
{
	qDebug() << "appendToSolarSystemConfigurationFile begin ... ";
	if (objectList.isEmpty())
	{
		return false;
	}

	//Check if the configuration file exists
	if (!QFile::exists(customSolarSystemFilePath))
	{
		qDebug() << "Can't append object data to ssystem_minor.ini: Unable to find" << QDir::toNativeSeparators(customSolarSystemFilePath);
		return false;
	}

	// load all existing minor bodies
	QHash<QString,QString> loadedObjects = listAllLoadedSsoIdentifiers();

	QSettings * solarSystemSettings = new QSettings(customSolarSystemFilePath, StelIniFormat);
	if (solarSystemSettings->status() != QSettings::NoError)
	{
		qDebug() << "Error opening ssystem_minor.ini:" << QDir::toNativeSeparators(customSolarSystemFilePath);
		return false;
	}

	//Remove loaded objects (identified by name, not by section name) that are also in the proposed objectList
	for (auto object : objectList)
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
	solarSystemSettings = Q_NULLPTR;

	//Write to file. (Handle as regular text file, not QSettings.)
	//TODO: The usual validation
	qDebug() << "Appending to file...";
	QFile solarSystemConfigurationFile(customSolarSystemFilePath);
	if(solarSystemConfigurationFile.open(QFile::WriteOnly | QFile::Append | QFile::Text))
	{
		QTextStream output (&solarSystemConfigurationFile);
		bool appendedAtLeastOne = false;

		for (auto object : objectList)
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

			output << StelUtils::getEndLineChar() << QString("[%1]").arg(sectionName) << StelUtils::getEndLineChar();
			for (auto key : object.keys())
			{
				output << QString("%1 = %2").arg(key).arg(object.value(key).toString()) << StelUtils::getEndLineChar();
			}
			output.flush();
			qDebug() << "Appended successfully" << sectionName;
			appendedAtLeastOne = true;
		}

		solarSystemConfigurationFile.close();
		qDebug() << "appendToSolarSystemConfigurationFile appended: " << appendedAtLeastOne;

		return appendedAtLeastOne;
	}
	else
	{
		qDebug() << "Unable to open for writing" << QDir::toNativeSeparators(customSolarSystemFilePath);
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
		qDebug() << "Can't update ssystem.ini: Unable to find" << QDir::toNativeSeparators(customSolarSystemFilePath);
		return false;
	}

	QSettings solarSystem(customSolarSystemFilePath, StelIniFormat);
	if (solarSystem.status() != QSettings::NoError)
	{
		qDebug() << "Error opening ssystem.ini:" << QDir::toNativeSeparators(customSolarSystemFilePath);
		return false;
	}

	QStringList existingSections = solarSystem.childGroups();
	QHash<QString,QString> loadedObjects = listAllLoadedSsoIdentifiers();
	//TODO: Move to constructor?
	// This list of elements gets temporarily deleted.
	// GZ: Note that the original implementation assumed that the coord_func could ever change. This is not possible at least in 0.13 and later:
	// ell_orbit is used for moons (distances in km) while kepler_orbit (comet_orbit) is used for minor bodies around the sun.
	static const QStringList orbitalElementsKeys = {
		"coord_func",
		"orbit_ArgOfPericenter",
		"orbit_AscendingNode",
		"orbit_Eccentricity",
		"orbit_Epoch",
// GZ It seems to have been an error to include these.  They might simply be updated without prior deletion.
//		"orbit_good",
//		"dust_lengthfactor",
//		"dust_brightnessfactor",
//		"dust_widthfactor",
		"orbit_Inclination",
		"orbit_LongOfPericenter",
		"orbit_MeanAnomaly",
		"orbit_MeanLongitude",
		"orbit_MeanMotion",
		"orbit_PericenterDistance",
		"orbit_Period",
		"orbit_SemiMajorAxis",
		"orbit_TimeAtPericenter"};

	qDebug() << "Updating objects...";
	for (auto object : objectList)
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
						for (auto property : object.keys())
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
			// GZ This seems completely useless now. Type of orbit will not change as it is always kepler_orbit.
			for (auto key : orbitalElementsKeys)
			{
				solarSystem.remove(key);
			}

			for (auto key : orbitalElementsKeys)
			{
				updateSsoProperty(solarSystem, object, key);
			}
		}

		if (flags.testFlag(UpdateMagnitudeParameters))
		{
			if (object.contains("absolute_magnitude") && object.contains("slope_parameter"))
			{
				QString type = solarSystem.value("type").toString();
				if (type == "asteroid" || type == "plutino" || type == "cubewano" || type == "scattered disc object" || type == "sednoid" || type == "comet" )
				{
					updateSsoProperty(solarSystem, object, "absolute_magnitude");
					updateSsoProperty(solarSystem, object, "slope_parameter");
				}
				else
				{
					qWarning() << "cannot update magnitude for type " << type;
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

QString SolarSystemEditor::fixGroupName(QString &name)
{
	QString groupName(name);
	groupName.replace("%25", "%");
	groupName.replace("%28", "(");
	groupName.replace("%29", ")");
	return groupName;
}

int SolarSystemEditor::unpackDayOrMonthNumber(QChar digit)
{
	//0-9, 0 is an invalid value in the designed use of this function.
	if (digit.isDigit())
	{
		return digit.digitValue();
	}

	if (digit.isUpper())
	{
		char letter = digit.toLatin1();
		if (letter < 'A' || letter > 'V')
			return 0;
		return (10 + (letter - 'A'));
	}
	else
	{
		return -1;
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
		cycleCount += (10 + prefix.toLatin1() - QChar('A').toLatin1()) * 10;
	else if (prefix.isLetter() && prefix.isLower())
		cycleCount += (10 + prefix.toLatin1() - QChar('a').toLatin1()) * 10 + 26*10;
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

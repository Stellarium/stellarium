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
#include "StelFileMgr.hpp"
#include "StelIniParser.hpp"
#include "StelModuleMgr.hpp"
#include "StelObjectMgr.hpp"
#include "SolarSystem.hpp"

#include <QDate>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QSettings>
#include <QString>
#include <QBuffer>
#include <QRegularExpression>
#if (QT_VERSION < QT_VERSION_CHECK(6,0,0))
#include <QTextCodec>
#endif

#include <cmath>
#include <stdexcept>


StelModule* SolarSystemEditorStelPluginInterface::getStelModule() const
{
	return new SolarSystemEditor();
}

StelPluginInfo SolarSystemEditorStelPluginInterface::getPluginInfo() const
{
	// Allow to load the resources when used as a static plugin
	Q_INIT_RESOURCE(SolarSystemEditor);
	
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
	mainWindow(nullptr),
	solarSystemConfigurationFile(nullptr)
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
	if (solarSystemConfigurationFile != nullptr)
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

	// { Asteroid Number, "Periodic Comet Number"}
	cometLikeAsteroids = {
		{   2060,  "95P" }, {   4015, "107P" }, {   7968, "133P" }, {  60558, "174P" },
		{ 118401, "176P" }, { 248370, "433P" }, { 300163, "288P" }, { 323137, "282P" },
		{ 457175, "362P" }
	};

	initMinorPlanetData();
	initCometCrossref();

	connect(&StelApp::getInstance(), SIGNAL(languageChanged()), this, SLOT(updateI18n()));
	isInitialized = true;

	// key bindings and other actions
	addAction("actionShow_MPC_Import", N_("Solar System Editor"), N_("Import orbital elements in MPC format..."), mainWindow, "newImportMPC()", "Ctrl+Alt+S");
}

//double SolarSystemEditor::getCallOrder(StelModuleActionName) const// actionName
//{
//	return 0.;
//}

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

bool SolarSystemEditor::isFileEncodingValid(QString filePath) const
{
	QFile checkFile(filePath);
	if (!checkFile.open(QIODevice::ReadOnly))
	{
		qWarning() << "[Solar System Editor] Cannot open " << QDir::toNativeSeparators(filePath);
		return false;
	}
	else
	{
#if (QT_VERSION>=QT_VERSION_CHECK(6,0,0))
		QTextStream in(&checkFile);
		in.setEncoding(QStringConverter::Utf8);
		in.readAll();
		checkFile.close();

		if (in.status()!=QTextStream::Ok)
#else
		QByteArray byteArray = checkFile.readAll();
		checkFile.close();

		QTextCodec::ConverterState state;
		QTextCodec *codec = QTextCodec::codecForName("UTF-8");
		const QString text = codec->toUnicode(byteArray.constData(), byteArray.size(), &state);
		Q_UNUSED(text)
		if (state.invalidChars > 0)
#endif
		{
			qDebug() << "[Solar System Editor] Not a valid UTF-8 sequence in file " << filePath;
			return false;
		}
		else
			return true;
	}
}

bool SolarSystemEditor::replaceSolarSystemConfigurationFileWith(QString filePath)
{
	if (!QFile::exists(filePath))
	{
		qCritical() << "SolarSystemEditor: Cannot replace, file not found: " << filePath;
		return false;
	}

	if (!isFileEncodingValid(filePath))
		return false;

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

	if (!isFileEncodingValid(filePath))
		return false;

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
			QStringList minorChildGroups = minorBodies.childGroups();
			if (minorChildGroups.contains(fixedGroupName))
			{
				qDebug() << "   This group " << fixedGroupName << "already exists. Updating values";
			}
			else
				qDebug() << "   This group " << fixedGroupName << "does not yet exist. Adding values";

			minorBodies.beginGroup(fixedGroupName);
			QStringList newKeys = newData.allKeys(); // limited to the group!
			for (const auto &key : qAsConst(newKeys))
			{
				minorBodies.setValue(key, newData.value(key));
			}
			minorBodies.endGroup();
			newData.endGroup();
		}
		minorBodies.sync();
		qDebug() << "Minor groups now: " << minorBodies.childGroups();
		// There may be a generic group "General" in the updated file, created from comments. We must remove it.
		if (minorBodies.childGroups().contains("General"))
		{
			qDebug() << "Removing unwanted General group.";
			minorBodies.remove("General");
			qDebug() << "Minor groups after fix now: " << minorBodies.childGroups();
		}
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
	for (const auto &group : qAsConst(groups))
	{
		QString name = solarSystemIni.value(group + "/name").toString();
		if (minorBodies.contains(name))
		{
			loadedObjects.insert(name, group);
		}
	}
	return loadedObjects;
}

void SolarSystemEditor::initCometCrossref()
{
	Q_ASSERT(cometCrossref.isEmpty());

	QFile cdata(":/SolarSystemEditor/comet_discovery.fab");
	if(cdata.open(QFile::ReadOnly | QFile::Text))
	{
		// regular expression to find the comments and empty lines
		static const QRegularExpression commentRx("^(\\s*#.*|\\s*)$");

		while(!cdata.atEnd())
		{
			QString line = QString::fromUtf8(cdata.readLine());

			// Skip comments
			if (commentRx.match(line).hasMatch() || line.startsWith("//"))
				continue;

			// Find data
			if (!line.isEmpty())
			{
				CometDiscoveryData comet;
				QStringList columns = line.split("|");
				// New IAU Designation ("1994 V1", "1995 Q3", etc.)
				//    A year followed by an upper-case letter indicating the half-month of discovery,
				//    followed by a number indicating the order of discovery. This may, in turn, be
				//    followed by a dash and another capital letter indicating one part of a fragmented comet.
				// Old IAU Permanent Designation ("1378","1759 I", "1993 XIII", ...)
				//    A year usually followed by a Roman numeral (which must be in upper-case) indicating
				//    the order of perihelion passage of the comet in that year. When there was only one
				//    comet passing perihelion in a year, then just the year number is used for this designation.
				// Old IAU Provisional Designation ("1982i", "1887a", "1991a1" ...)
				//    A year followed by a single lower-case letter, optionally followed by a single digit.
				// Periodic Comet Number ("1P", "2P", ... )
				//    1-3 digits followed by an upper-case "P", indicating one of the multiple-apparition
				//    objects in the file. This is part of the new designation system.
				//
				// Source: https://pds-smallbodies.astro.umd.edu/data_sb/resources/designation_formats.shtml
				//
				// Equivalences in ssystem_minor.ini:
				// New (1994) IAU Designation      is equal to date_code, or a number like 33P or 3D for periodic or defunct comets, resp.
				// Old IAU Permanent Designation   is equal to perihelion_code
				// Old IAU Provisional Designation is equal to discovery_code
				// number                          is equal to comet_number

				// [1] IAU designation or standard designation (Periodic Comet Number) for periodic comets
				QString designation = columns.at(0).trimmed();
				// [2] IAU designation or date code for comets
				comet.date_code       = columns.at(1).simplified(); // remove extra internal spaces
				// [3] perihelion code for comets
				comet.perihelion_code = columns.at(2).simplified(); // remove extra internal spaces
				// [4] discovery code for comets
				comet.discovery_code  = columns.at(3).trimmed();
				// [5] discovery date for comets
				comet.discovery_date  = columns.at(4).trimmed();
				// [6] discoverer of comets
				comet.discoverer      = columns.at(5).trimmed();

				if (cometCrossref.contains(designation))
					qWarning() << "SSE: Comet cross-index data: Overwriting entry for" << designation;
				cometCrossref.insert(designation, comet);
			}
		}
		cdata.close();
	}
}

void SolarSystemEditor::initMinorPlanetData()
{
	Q_ASSERT(numberedMinorPlanets.isEmpty());

	QFile dcdata(":/SolarSystemEditor/discovery_circumstances.dat");
	// Open file
	if (dcdata.open(QIODevice::ReadOnly))
	{
		QByteArray data = StelUtils::uncompress(dcdata);
		dcdata.close();

		//check if decompressing was successful
		if(data.isEmpty())
			return;

		//create and open a QBuffer for reading
		QBuffer buf(&data);
		buf.open(QIODevice::ReadOnly);

		// regular expression to find the comments and empty lines
		static const QRegularExpression commentRx("^(\\s*#.*|\\s*)$");

		while (!buf.atEnd())
		{
			QString line = QString::fromUtf8(buf.readLine());

			// Skip comments
			if (commentRx.match(line).hasMatch() || line.startsWith("//"))
				continue;

			QStringList list=line.split("\t");
			int number = list.at(0).trimmed().toInt();
			// The first entry is the date of discovery, the second is the name of discoverer
			DiscoveryCircumstances discovery(list.at(1).trimmed(), list.at(2).trimmed());

			numberedMinorPlanets.insert(number, discovery);
		}
		buf.close();
	}
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
	for (const auto &group : settings.childGroups())
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
	static const QRegularExpression leadingZerosExpr("^0*");
	SsoElements result;
	// New and exact definition, directly from https://minorplanetcenter.net/iau/info/CometOrbitFormat.html
	// We parse by documented columns and process extracted strings later.
	/*
	 * Wrong regexp; GH: #2281
	QRegularExpression mpcParser("^([[:print:]]{4})"      //    1 -   4  i4     1. Periodic comet number
				    "([CPDIA])"               //    5        a1     2. Orbit type (generally `C', `P' or `D') -- A=reclassified as Asteroid? I=Interstellar.
				    "([[:print:]]{7}).{2}"    //    6 -  12  a7     3. IAU designation (in packed form)
				    "([[:print:]]{4})."       //   15 -  18  i4     4. Year of perihelion passage
				    "([[:print:]]{2})."       //   20 -  21  i2     5. Month of perihelion passage
				    "([[:print:]]{7})."       //   23 -  29  f7.4   6. Day of perihelion passage (TT)
				    "([[:print:]]{9}).{2}"    //   31 -  39  f9.6   7. Perihelion distance (AU)
				    "([[:print:]]{8}).{2}"    //   42 -  49  f8.6   8. Orbital eccentricity
				    "([[:print:]]{8}).{2}"    //   52 -  59  f8.4   9. Argument of perihelion, J2000.0 (degrees)
				    "([[:print:]]{8}).{2}"    //   62 -  69  f8.4  10. Longitude of the ascending node, J2000.0 (degrees)
				    "([[:print:]]{8}).{2}"    //   72 -  79  f8.4  11. Inclination in degrees, J2000.0 (degrees)
				    "([[:print:]]{4})"        //   82 -  85  i4    12. Year of epoch for perturbed solutions
				    "([[:print:]]{2})"        //   86 -  87  i2    13. Month of epoch for perturbed solutions
				    "([[:print:]]{2})\\s{2}"  //   88 -  89  i2    14. Day of epoch for perturbed solutions
				    "([[:print:]]{4})\\s"     //   92 -  95  f4.1  15. Absolute magnitude
				    "([[:print:]]{4})\\s{2}"  //   97 - 100  f4.0  16. Slope parameter
				    "([[:print:]]{56})\\s"    //  103 - 158  a56   17. Designation and Name
				    "([[:print:]]{1,9}).*$"   //  160 - 168  a9    18. Reference
	    );
	*/

	static const QRegularExpression mpcParser("^\\s*(\\d{4})?"                                 //    1 -   4  i4     1. Periodic comet number
						  "([CPDIA])"                                      //    5        a1     2. Orbit type (generally `C', `P' or `D') -- A=reclassified as Asteroid? I=Interstellar.
						  "((?:\\w|\\s|\\.|/)(?:\\w{4,5}|\\s{4,5})?[0a-zA-Z]{1,2})?\\s+"    //    6 -  12  a7     3. IAU designation (in packed form) [last group is 2 long if first is 4 long.]
						  "(\\d{4}|\\*{4})\\s+"                            //   15 -  18  i4     4. Year of perihelion passage (or **** for negative years!)
						  "(\\d{2})\\s+"                                   //   20 -  21  i2     5. Month of perihelion passage
						  "(\\d{1,2}(?:\\.\\d{1,4})?)\\s+"                 //   23 -  29  f7.4   6. Day of perihelion passage (TT)
						  "(\\d{1,2}\\.\\d{2,6})\\s+"                      //   31 -  39  f9.6   7. Perihelion distance (AU)
						  "(\\d\\.\\d{2,6})\\s+"                           //   42 -  49  f8.6   8. Orbital eccentricity
						  "(\\d{1,3}(?:\\.\\d{1,4})?)\\s+"                 //   52 -  59  f8.4   9. Argument of perihelion, J2000.0 (degrees)
						  "(\\d{1,3}(?:\\.\\d{1,4})?)\\s+"                 //   62 -  69  f8.4  10. Longitude of the ascending node, J2000.0 (degrees)
						  "(\\d{1,3}(?:\\.\\d{1,4})?)\\s+"                 //   72 -  79  f8.4  11. Inclination in degrees, J2000.0 (degrees)
						  "(?:(\\d{4})"                                    //   82 -  85  i4    12. Year of epoch for perturbed solutions
						  "(\\d\\d)"                                       //   86 -  87  i2    13. Month of epoch for perturbed solutions
						  "(\\d\\d))?\\s+"                                 //   88 -  89  i2    14. Day of epoch for perturbed solutions
						  "(\\-?\\d{1,2}\\.\\d)\\s+"                       //   92 -  95  f4.1  15. Absolute magnitude
						  "(\\d{1,2}\\.\\d)\\s+"                           //   97 - 100  f4.0  16. Slope parameter
						  "(\\S.{1,54}\\S)"                                //  103 - 158  a56   17. Designation and Name
						  "(?:\\s+(\\S.*))?$");                            //  160 - 168  a9    18. Reference
	if (!mpcParser.isValid())
	{
		qCritical() << "Bad Regular Expression:" << mpcParser.errorString();
		qCritical() << "Error offset:" << mpcParser.patternErrorOffset();
	}
	QRegularExpressionMatch mpcMatch=mpcParser.match(oneLineElements);
	//qDebug() << "RegExp captured:" << mpcMatch.capturedTexts();
	if (!mpcMatch.hasMatch())
	{
		qWarning() << "No match for" << oneLineElements;
		return result;
	}

	QString periodicNumberString = mpcMatch.captured(1).trimmed();
	periodicNumberString.remove(leadingZerosExpr);
	QChar orbitType = mpcMatch.captured(2).at(0);
	QString packedIauDesignation = mpcMatch.captured(3).trimmed(); // packed IAU designation, or a fragment code
	QString iauDesignation; // This either remains empty (for periodical comets) or receives something like P/2023 T2-B
	QString fragment; // periodic comets may have fragments with several letters!
	if (packedIauDesignation.length()==7)
	{
		iauDesignation=QString("%1/%2").arg(orbitType, unpackCometIAUDesignation(packedIauDesignation)); // like "P/2023 T2-B"
	}
	else
	{
		Q_ASSERT(packedIauDesignation.isLower());
		fragment=packedIauDesignation.toUpper();
	}

	const QString mpcName = mpcMatch.captured(17).trimmed(); // From MPC file, name format is either "332P-C/Ikeya-Murakami" or "C/2023 P1 (Nishimura)".
	// Now, split this and separate out the parts!

	if (periodicNumberString.isEmpty() && packedIauDesignation.isEmpty())
	{
		qWarning() << "Comet" << mpcName << "is missing both comet number AND IAU designation. Skipping.";
		return result;
	}
	if (mpcName.isEmpty())
	{
		qWarning() << "Comet element line cannot be decoded. Skipping " << oneLineElements;
		return result;
	}

	// Sanity check: Make sure a numbered periodic name has the same number.
	if ((periodicNumberString.length()) && (!mpcName.startsWith(periodicNumberString)))
			qCritical() << "Parsed number '" << periodicNumberString << "' does not fit number in name! Line " << oneLineElements;


	// Derive name: Just use MPC name as-is for unnumbered comets, or add perihel year in brackets to numbered ones.
	QString name;
	if (mpcName.contains("(")) // "C/2023 P1 (Nishimura)" style
	{
		// Sanity check: Just make sure it matches!
		if ((!iauDesignation.isEmpty()) && (!mpcName.startsWith(iauDesignation)))
			qCritical() << "Parsed designation '" << iauDesignation <<  "' does not fit number in name! Line " << oneLineElements;
		name=mpcName;
	}
	else if (mpcName.length()==9 && !mpcName.startsWith("A/") && periodicNumberString.isEmpty()) // anonymous?
	{
		QStringList nameList=mpcName.split('/');
		name=QString("%1/%2 (%3)").arg(orbitType, nameList.at(1).trimmed(), q_("Anonymous")); // "C/1500 H1 (Anonymous)"
	}
	else // "332P-C/Ikeya-Murakami" style
	{
		QStringList nameList=mpcName.split('/');
		QString fragmentPostfix;
		if (!fragment.isEmpty())
			fragmentPostfix=QString("-%1").arg(fragment);
		if (nameList.count()!=2)
		{
			qInfo() << "Odd name (no error, but no proper name for periodic comet) given in line " << oneLineElements;
			name=nameList.at(0);
		}
		else
			name=QString("%1%2%3/%4").arg(periodicNumberString, orbitType, fragmentPostfix, nameList.at(1).trimmed()); // "332P-C/Ikeya-Murakami"

		// Sanity check: Make sure this is identical to the mpcName...
		if (name!=mpcName)
			qWarning() << "Some mismatch in names: constructed " << name << "vs. directly read" << mpcName;
		name.append(QString(" (%1)").arg(mpcMatch.captured(4).toInt())); // "332P-C/Ikeya-Murakami (2021)"
	}

	result.insert("name", name);
	if (!iauDesignation.isEmpty())
		result.insert("iau_designation", iauDesignation);

	// Now fill in missing data which we have in our file database (cometsData)

	QString key;
	static const QRegularExpression periodicRe("^([1-9][0-9]*[PD](-\\w+)?)"); // No "/" at end, there are nameless numbered comets! (e.g. 362, 396)
	QRegularExpressionMatch periodMatch=periodicRe.match(name);
	if (periodMatch.hasMatch())
		key=periodMatch.captured(1);
	else
		key = name.split("(").at(0).trimmed(); // IAU 1994 date style

	if (cometCrossref.contains(key))
	{
		qDebug() << "Filling extra data for " << name << "with key" << key;
		const CometDiscoveryData comet = cometCrossref.value(key);
		// It is still possible that we match one nnP entry with data from a different apparition. The additional IDs should be limited to the right perihelion date.
		// add IAU designation in addition to the old-style designation
		if (!comet.date_code.isEmpty() && !result.contains("iau_designation"))
			result.insert("iau_designation", comet.date_code);

		// The data parsed from MPC may contain a makeshift perihelion code that only has a year. Only overwrite if we are in the same year!
		if (!comet.perihelion_code.isEmpty() &&
				(!result.contains("perihelion_code") ||
				 (result.value("perihelion_code").toString().length()==4) && comet.perihelion_code.contains(result.value("perihelion_code").toString())))
		{
			result.insert("perihelion_code", comet.perihelion_code);
			if (!comet.discovery_code.isEmpty() && result.contains("discovery_code") && (comet.discovery_code!=result.value("discovery_code")))
				qWarning() << "DIFFERENT COMET. Won't update " << comet.discovery_code << "for " << name;
			else if (!comet.discovery_code.isEmpty() && !result.contains("discovery_code"))
				result.insert("discovery_code", comet.discovery_code);
		}
		if (!comet.discovery_date.isEmpty())
			result.insert("discovery", comet.discovery_date);
		if (!comet.discoverer.isEmpty())
			result.insert("discoverer", comet.discoverer);
	}

	QString sectionName = convertToGroupName(name);
	if (sectionName.isEmpty())
	{
		qWarning() << "Cannot derive a comet section name from" << name << ". Skipping.";
		return SsoElements();
	}
	result.insert("section_name", sectionName);

	//After a name has been determined, insert the essential keys
	result.insert("type", orbitType == 'I' ? "interstellar object" : "comet"); // GZ: One could expect A=reclassified as asteroid, but eccentricity can be >1, so we leave A as comets.
	//result.insert("parent", "Sun"); // 0.16: omit obvious default.
	//"kepler_orbit" could be used for all cases:
	//"ell_orbit" interprets distances as kilometers, not AUs
	// result.insert("coord_func", "kepler_orbit"); // 0.20: omit default
	//result.insert("coord_func", "comet_orbit"); // 0.20: add this default for compatibility with earlier versions! TBD: remove for 1.0
	//result.insert("color", "1.0, 1.0, 1.0");  // 0.16: omit obvious default.
	//result.insert("tex_map", "nomap.png");    // 0.16: omit obvious default.

	bool ok1=false, ok2=false, ok3=false, ok4=false, ok5=false;

	int year	= mpcMatch.captured(4).toInt(&ok1);
	if (!ok1) // seen "****"?
	{
		QString iauStr=unpackCometIAUDesignation(packedIauDesignation);
		year=iauStr.split(' ').at(0).toInt(&ok1);
	}
	int month	= mpcMatch.captured(5).toInt(&ok2);
	double dayFraction	= mpcMatch.captured(6).toDouble(&ok3);

	if (ok1 && ok2 && ok3)
	{
		double jde;
		StelUtils::getJDFromDate(&jde, year, month, 0, 0, 0, 0.f);
		jde+=dayFraction;
		result.insert("orbit_TimeAtPericenter", jde);
	}
	else
	{
		qWarning() << "Cannot read perihelion date for comet " << name;
		return SsoElements();
	}

	double perihelionDistance = mpcMatch.captured(7).toDouble(&ok1); //AU
	double eccentricity = mpcMatch.captured(8).toDouble(&ok2); //NOT degrees, but without dimension.
	double argumentOfPerihelion = mpcMatch.captured(9).toDouble(&ok3);//J2000.0, degrees
	double longitudeOfAscendingNode = mpcMatch.captured(10).toDouble(&ok4);//J2000.0, degrees
	double inclination = mpcMatch.captured(11).toDouble(&ok5);
	if (ok1 && ok2 && ok3 && ok4 && ok5)
	{
		result.insert("orbit_PericenterDistance", perihelionDistance);
		result.insert("orbit_Eccentricity", eccentricity);
		result.insert("orbit_ArgOfPericenter", argumentOfPerihelion);
		result.insert("orbit_AscendingNode", longitudeOfAscendingNode);
		result.insert("orbit_Inclination", inclination);
	}
	else
	{
		qWarning() << "Cannot read main elements for comet " << name;
		return SsoElements();
	}

	// We should reduce orbit_good for elliptical orbits to one half period before/after epoch!
	// TBD: Can be removed in 1.0, relegating to handling by the loader.
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

	// Epoch
	year    = mpcMatch.captured(12).toInt(&ok1);
	month   = mpcMatch.captured(13).toInt(&ok2);
	int day = mpcMatch.captured(14).toInt(&ok3);
	if (ok1 && ok2 && ok3)
	{
		double jde;
		StelUtils::getJDFromDate(&jde, year, month, day, 0, 0, 0.f);
		result.insert("orbit_Epoch", jde);
	}
	else
	{
		qWarning() << "Warning: Cannot read element epoch date for comet " << name << ". Will use T_perihel at load time.";
	}

	double absoluteMagnitude = mpcMatch.captured(15).toDouble(&ok1);
	if (ok1)
		result.insert("absolute_magnitude", absoluteMagnitude);
	double slopeParameter = mpcMatch.captured(16).toDouble(&ok2);
	if (ok2)
		result.insert("slope_parameter", slopeParameter);
	if (!(ok1 && ok2))
	{
		qWarning() << "Warning: Problem reading magnitude parameters for comet " << name;
	}

	result.insert("ref", mpcMatch.captured(18).simplified());
	if (orbitType=='A')
	{
		//result.insert("radius", 5); //Fictitious default assumption. Use default of 1 instead.
		//result.insert("albedo", 0.1); // asteroid default
		result.insert("dust_lengthfactor", 0.0); // dust tail length w.r.t. gas tail length
		result.insert("dust_brightnessfactor", 0.0); // dust tail brightness w.r.t. gas tail.
		result.insert("dust_widthfactor", 0.0); // opening w.r.t. gas tail opening width.
		result.insert("outgas_intensity",0.0); // These are inactive objects!
		result.insert("outgas_falloff", 0.0);
	}
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

	//Minor planet number or IAU designation
	column = oneLineElements.mid(0, 7).trimmed();
	if (column.isEmpty())
	{
		return result;
	}
	int minorPlanetNumber = 0;
	QString iauDesignation;
	QString name;
	if (column.toInt(&ok) || ok)
	{
		minorPlanetNumber = column.toInt();
	}
	else
	{
		//See if it is a number, but packed
		//I hope the format is right (I've seen prefixes only between A and P)
		static const QRegularExpression packedMinorPlanetNumber("^([A-Za-z])(\\d+)$");
		QRegularExpressionMatch mpMatch;
		if (column.indexOf(packedMinorPlanetNumber, 0, &mpMatch) == 0)
		{
			minorPlanetNumber = mpMatch.captured(2).toInt(&ok);
			//TODO: Validation
			QChar prefix = mpMatch.captured(1).at(0);
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
			iauDesignation = unpackMinorPlanetIAUDesignation(column);
		}
	}

	if (minorPlanetNumber)
	{
		name = QString::number(minorPlanetNumber);
	}
	else if(iauDesignation.isEmpty())
	{
		qDebug() << "readMpcOneLineMinorPlanetElements():"
		         << column
			 << "is not a valid number or packed IAU designation";
		return SsoElements();
	}
	else
	{
		name = iauDesignation;
	}

	//In case the longer format is used, extract the human-readable name
	column = oneLineElements.mid(166, 28).trimmed();
	if (!column.isEmpty())
	{
		if (minorPlanetNumber)
		{
			static const QRegularExpression asteroidName("^\\((\\d+)\\)\\s+(\\S.+)$");
			QRegularExpressionMatch astMatch;
			if (column.indexOf(asteroidName, 0, &astMatch) == 0)
			{
				name = astMatch.captured(2);
				result.insert("minor_planet_number", minorPlanetNumber);
			}
			else
			{
				//Use the whole string, just in case
				name = column;
			}
			// Add Periodic Comet Number for comet-like asteroids
			QString dateCode = cometLikeAsteroids.value(minorPlanetNumber, "");
			if (!dateCode.isEmpty())
			{
				if (name==QString("(%1)").arg(QString::number(minorPlanetNumber)))
					result.insert("date_code", QString("%1/%2").arg(dateCode, name));
				else
					result.insert("date_code", QString("%1/(%2) %3").arg(dateCode, QString::number(minorPlanetNumber), name));
			}
		}
		//In the other case, the name is already the IAU designation
	}
	if (name.isEmpty())
		return SsoElements();

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
	//result.insert("coord_func", "comet_orbit"); // 0.20: add this default for compatibility with earlier versions!

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
	static const QRegularExpression packedDateFormat("^([IJK])(\\d\\d)([1-9A-C])([1-9A-V])$");
	QRegularExpressionMatch dateMatch;
	if (column.indexOf(packedDateFormat, 0, &dateMatch) != 0)
	{
		qWarning() << "readMpcOneLineMinorPlanetElements():"
		         << column << "is not a date in packed format";
		return SsoElements();
	}
	int year  = unpackYearNumber(dateMatch.captured(1).at(0).toLatin1(), dateMatch.captured(2).toInt());
	int month = unpackDayOrMonthNumber(dateMatch.captured(3).at(0));
	int day   = unpackDayOrMonthNumber(dateMatch.captured(4).at(0));
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
	double epochJDE;
	StelUtils::getJDFromDate(&epochJDE, year, month, day, 0, 0, 0);
	result.insert("orbit_Epoch", epochJDE);

	column = oneLineElements.mid(26, 9).trimmed();
	double meanAnomalyAtEpoch = column.toDouble(&ok);//degrees
	if (!ok)
		return SsoElements();
	result.insert("orbit_MeanAnomaly", meanAnomalyAtEpoch);
	// We assume from now on that orbit_good is 1/2 orbital duration for elliptical orbits if not given explicitly.
	// This allows following full ellipses. However, elsewhere we should signal to users when
	// it should be assumed that elements are outdated and thus positions wrong. (Let's take "1000 earth days" for that.)
	if (eccentricity >=1.)
	{
	    // This should actually never happen for minor planets!
	    qWarning() << "Strange eccentricity for " << name << ":" << eccentricity;
	    result.insert("orbit_good", 1000); // default validity for osculating elements for parabolic/hyperbolic comets, days
	}

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

	DiscoveryCircumstances dc = numberedMinorPlanets.value(minorPlanetNumber, DiscoveryCircumstances("",""));
	if (!dc.first.isEmpty())
	{
		result.insert("discovery", dc.first);
		result.insert("discoverer", dc.second);
	}

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
	for (const auto &object : objectList)
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
	solarSystemSettings = nullptr;

	const int width = -30;
	QList<DiscoveryCircumstances> extraData;

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

			output << StelUtils::getEndLineChar() << QString("[%1]").arg(sectionName) << "\n"; // << StelUtils::getEndLineChar();
			QHash<QString, QVariant>::const_iterator i=object.cbegin();
			while (i != object.cend())
			{
				// formatting strings
				output << QString("%1 = %2").arg(i.key(), width).arg(i.value().toString()) << "\n"; // << StelUtils::getEndLineChar();
				++i;
			}

			extraData.clear();
			const int mpn = object.value("minor_planet_number").toInt();
			if (mpn==0)
			{
				// this is a minor planet
				DiscoveryCircumstances dc = numberedMinorPlanets.value(mpn, DiscoveryCircumstances("",""));
				if (!dc.first.isEmpty())
				{
					extraData.append(DiscoveryCircumstances("discovery", dc.first));
					extraData.append(DiscoveryCircumstances("discoverer", dc.second));
				}
			}
			else
			{
				// this is a comet
				QString ckey = name.split("/").at(0).trimmed();
				CometDiscoveryData comet;
				if (cometCrossref.contains(ckey))
				{
					// standard designation [1P]
					comet = cometCrossref.value(ckey);
					// add IAU designation in addition to the old-style designation
					if (!comet.date_code.isEmpty())
						extraData.append(DiscoveryCircumstances("iau_designation", comet.date_code));
				}
				else
					comet = cometCrossref.value(name.split("(").at(0).trimmed()); // IAU designation [P/1682 Q1]

				if (!comet.perihelion_code.isEmpty())
					extraData.append(DiscoveryCircumstances("perihelion_code", comet.perihelion_code));
				if (!comet.discovery_code.isEmpty())
					extraData.append(DiscoveryCircumstances("discovery_code", comet.discovery_code));
				if (!comet.discovery_date.isEmpty())
					extraData.append(DiscoveryCircumstances("discovery", comet.discovery_date));
				if (!comet.discoverer.isEmpty())
					extraData.append(DiscoveryCircumstances("discoverer", comet.discoverer));
			}
			for (const auto &ed : extraData)
			{
				// formatting strings
				output << QString("%1 = %2").arg(ed.first, width).arg(ed.second); // << StelUtils::getEndLineChar();
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
						QHash<QString, QVariant>::const_iterator i=object.cbegin();
						while (i != object.cend())
						{
							solarSystem.setValue(i.key(), i.value());
							++i;
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
			updateSsoProperty(solarSystem, object, "iau_designation");
			// Comet codes
			updateSsoProperty(solarSystem, object, "date_code");
			updateSsoProperty(solarSystem, object, "perihelion_code");
			updateSsoProperty(solarSystem, object, "discovery_code");
			// Discovery Circumstances
			updateSsoProperty(solarSystem, object, "discovery");
			updateSsoProperty(solarSystem, object, "discoverer");
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
			//for (const auto &key : orbitalElementsKeys)
			//{
			//	solarSystem.remove(key);
			//}

			for (const auto &key : orbitalElementsKeys)
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
		if ((letter < 'A') || (letter > 'V'))
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
	// Prefix codes as used in https://www.minorplanetcenter.net/iau/MPCORB/AllCometEls.txt
	static const QMap<QChar, int>centuryMap={
		{'.', -2},
		{'/', -1},
		{'A', 10},
		{'B', 11},
		{'C', 12},
		{'D', 13},
		{'E', 14},
		{'F', 15},
		{'G', 16},
		{'H', 17},
		{'I', 18},
		{'J', 19},
		{'K', 20},
		{'L', 21},
		{'M', 22}
	};
	int century=centuryMap.value(prefix, prefix.digitValue()); // CAVEAT: May return -1 for unknown prefix code!
	return 100*century+lastTwoDigits;
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

QString SolarSystemEditor::unpackMinorPlanetIAUDesignation (const QString &packedDesignation)
{
	static const QRegularExpression packedFormat("^([IJK])(\\d\\d)([A-Z])([\\dA-Za-z])(\\d)([A-Z])$");
	QRegularExpressionMatch pfMatch;
	if (packedDesignation.indexOf(packedFormat, 0, &pfMatch) != 0)
	{
		static const QRegularExpression packedSurveyDesignation("^(PL|T1|T2|T3)S(\\d+)$");
		QRegularExpressionMatch psMatch = packedSurveyDesignation.match(packedDesignation);
		if (psMatch.hasMatch())
		{
			int number = psMatch.captured(2).toInt();
			if (psMatch.captured(1) == "PL")
				return QString("%1 P-L").arg(number);
			else if (psMatch.captured(1) == "T1")
				return QString("%1 T-1").arg(number);
			else if (psMatch.captured(1) == "T2")
				return QString("%1 T-2").arg(number);
			else
				return QString("%1 T-3").arg(number);
			//TODO: Are there any other surveys?
		}
		else
			return QString();
	}

	//Year
	QChar yearPrefix = pfMatch.captured(1).at(0);
	int yearLastTwoDigits = pfMatch.captured(2).toInt();
	int year = unpackYearNumber(yearPrefix, yearLastTwoDigits);

	//Letters
	QString halfMonthLetter = pfMatch.captured(3);
	QString secondLetter = pfMatch.captured(6);

	//Second letter cycle count
	QChar cycleCountPrefix = pfMatch.captured(4).at(0);
	int cycleCountLastDigit = pfMatch.captured(5).toInt();
	int cycleCount = unpackAlphanumericNumber(cycleCountPrefix, cycleCountLastDigit);

	//Assemble the unpacked IAU designation
	QString result = QString("%1 %2%3").arg(year).arg(halfMonthLetter, secondLetter);
	if (cycleCount != 0)
	{
		result.append(QString::number(cycleCount));
	}

	return result;
}

// Should be complete as of 08/2023.
QString SolarSystemEditor::unpackCometIAUDesignation (const QString &packedDesignation)
{
	Q_ASSERT(packedDesignation.length()==7);
	static const QRegularExpression packedFormat("^([0-9A-M/.])(\\d\\d)([A-Z])([\\dA-Za-z])(\\d)([A-Za-z0])$");
	QRegularExpressionMatch pfMatch = packedFormat.match(packedDesignation);

	//Year
	const QChar yearPrefix = pfMatch.captured(1).at(0);
	const int yearLastTwoDigits = pfMatch.captured(2).toInt();
	const int year = unpackYearNumber(yearPrefix, yearLastTwoDigits);

	//Letters
	QString halfMonthLetter = pfMatch.captured(3);
	QString fragmentLetter = pfMatch.captured(6); // this is either lowercase for a fragment, or a second character for the week code.
	if (fragmentLetter != "0" && fragmentLetter.isUpper())
		halfMonthLetter.append(fragmentLetter); // in this case there are two letters. Presumably a renamed preliminary asteroid code?

	int cycleCount = unpackAlphanumericNumber(pfMatch.captured(4).at(0), pfMatch.captured(5).toInt());

	//Assemble the unpacked IAU designation
	QString result = QString("%1 %2%3").arg(QString::number(year), halfMonthLetter, cycleCount>0 ? QString::number(cycleCount) : "");
	if (fragmentLetter != "0" && fragmentLetter.isLower())
	{
		result.append(QString("-%1").arg(fragmentLetter.toUpper()));
	}

	//qDebug() << "Decoded" << packedDesignation << "as" << result;
	return result;
}

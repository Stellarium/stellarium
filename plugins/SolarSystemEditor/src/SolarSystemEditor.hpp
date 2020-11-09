/*
 * Solar System editor plug-in for Stellarium
 * 
 * Copyright (C) 2010 Bogdan Marinov
 * Copyright (C) 2017 Georg Zotti (changes for 0.16)
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

#ifndef SOLARSYSTEMEDITOR_HPP
#define SOLARSYSTEMEDITOR_HPP

#include "StelGui.hpp"
#include "StelModule.hpp"
//#include "CAIMainWindow.hpp"

#include <QHash>
#include <QList>
#include <QString>
#include <QVariant>

class SolarSystemManagerWindow;
class SolarSystem;
class QSettings;

//! Convenience type for storage of SSO properties in ssystem_minor.ini format.
//! This is an easy way of storing data in the format used in Stellarium's
//! solar system configuration file.
//! What would be key/value pairs in a section in the ssystem_minor.ini file
//! are key/value pairs in the hash. The section name is stored with key
//! "section_name".
//! As it is a hash, key names are not stored alphabetically. This allows
//! for rapid addition and look-up of values, unlike a real QSettings
//! object in StelIniFormat.
//! Also, using this way may allow scripts to define SSOs.
//! \todo Better name.
typedef QHash<QString, QVariant> SsoElements;

/*!
 \class SolarSystemEditor
 \brief Main class of the Solar System Editor plug-in which allows editing (add, delete, update) of the minor bodies.
 \author Bogdan Marinov, Georg Zotti (0.16 changes)

 Solar System bodies are identified by their names in Stellarium, but entries
 in the configuration file are identified by their group (section) names.
 This makes the detection of duplicates more difficult.
*/
class SolarSystemEditor : public StelModule
{
	Q_OBJECT

public:
	SolarSystemEditor();
	virtual ~SolarSystemEditor() Q_DECL_OVERRIDE;
	
	///////////////////////////////////////////////////////////////////////////
	// Methods inherited from the StelModule class
	//! called when the plug-in is loaded.
	//! All initializations should be done here.
	virtual void init() Q_DECL_OVERRIDE;

	//! called before the plug-in is un-loaded.
	//! Useful for stopping processes, unloading textures, etc.
	virtual void deinit() Q_DECL_OVERRIDE {}

	//! Does nothing.
	virtual void update(double deltaTime) Q_DECL_OVERRIDE {Q_UNUSED(deltaTime)}

	//! draws on the view port.
	//! Dialog windows don't need explicit drawing, it's done automatically.
	virtual void draw(StelCore * core) Q_DECL_OVERRIDE {Q_UNUSED(core)}

	virtual double getCallOrder(StelModuleActionName actionName) const Q_DECL_OVERRIDE;

	//! called when the "configure" button in the "Plugins" tab is pressed
	virtual bool configureGui(bool show) Q_DECL_OVERRIDE;
	
	//! Reads a single comet's orbital elements from a string.
	//! This function converts a line of comet orbital elements in MPC format
	//! to a hash in Stellarium's ssystem.ini format.
	//! The MPC's one-line orbital elements format for comets
	//! is described on their website:
	//! http://www.minorplanetcenter.org/iau/info/CometOrbitFormat.html
	//! \returns an empty hash if there is an error or the source string is not
	//! a valid line in MPC format.
	//! \todo Recognise the long form packed designations (to handle fragments)
	//! \todo Handle better any unusual symbols in section names (URL encoding?)
	//! \todo Use column cuts intead of a regular expression?
	SsoElements readMpcOneLineCometElements(QString oneLineElements) const;

	//! Reads a single minor planet's orbital elements from a string.
	//! This function converts a line of minor planet orbital elements in
	//! MPC format to a hash in Stellarium's ssystem.ini format.
	//! The MPC's one-line orbital elements format for minor planets
	//! is described on their website:
	//! http://www.minorplanetcenter.org/iau/info/MPOrbitFormat.html
	//! \returns an empty hash if there is an error or the source string is not
	//! a valid line in MPC format.
	//! \todo Handle better any unusual symbols in section names (URL encoding?)
	SsoElements readMpcOneLineMinorPlanetElements(QString oneLineElements) const;

	//! Reads a list of comet orbital elements from a file.
	//! This function reads a list of comet orbital elements in MPC's one-line
	//! format from a file (one comet per line) and converts it to a list of
	//! hashes in Stellarium's ssystem.ini format.
	//! Example source file is the list of observable comets on the MPC's site:
	//! http://www.minorplanetcenter.org/iau/Ephemerides/Comets/Soft00Cmt.txt
	//! readMpcOneLineCometElements() is used internally to parse each line.
	QList<SsoElements> readMpcOneLineCometElementsFromFile(QString filePath) const;

	//! Reads a list of minor planet orbital elements from a file.
	//! This function reads a list of minor planets orbital elements in MPC's
	//! one-line format from a file (one comet per line) and converts it to
	//! a list of hashes in Stellarium's ssystem.ini format.
	//! Example source file is the list of bright asteroids on the MPC's site:
	//! http://www.minorplanetcenter.org/iau/Ephemerides/Bright/2010/Soft00Bright.txt
	//! readMpcOneLineMinorPlanetElements() is used internally to parse each line.
	QList<SsoElements> readMpcOneLineMinorPlanetElementsFromFile(QString filePath) const;

	//! Adds a new entry at the end of the user solar system configuration file.
	//! This function writes directly to the file. See the note on why QSettings
	//! was not used in the description of
	//! appendToSolarSystemConfigurationFile(QList<SsoElements>)
	//! Duplicates are removed: If any section in the file matches the
	//! "section_name" value of the inserted entry, it is removed.
	bool appendToSolarSystemConfigurationFile(SsoElements object);

	//! Adds new entries at the end of the user solar system configuration file.
	//! XXX Also updates existing objects (by removing and then appending)
	//! This function writes directly to the file. QSettings was not used, as:
	//!  - Using QSettings with QSettings::IniFormat causes the list in the
	//!    "color" field (e.g. "1.0, 1.0, 1.0") to be wrapped in double quotation
	//!    marks (Stellarium requires no quotation marks).
	//!    (Has been fixed by using StelIniFormat)
	//!  - Using QSettings with StelIniFormat causes unacceptable append times
	//!    when the file grows (>~40 entries). This most probably happens because
	//!    StelIniParser uses QMap internally for the entry list. QMap orders its
	//!    keys (in the case of strings - alphabetically) and it has to find
	//!    the appropriate place in the ordering for every new key, which takes
	//!    more and more time as the list grows.
	//!    (0.16pre: This problem seems to have been solved: I (GZ) have changed
	//!     this plugin to use StelIniParser, and loading 1000comets takes about 4 seconds.
	//!     This is very acceptable, and avoids tons of problems with the default QSettings::IniFormat)
	//!
	//! Duplicates are removed: If any section in the file matches the
	//! "section_name" value of a new entry, it is removed.
	//! Invalid entries in the list (that don't contain a value for
	//! "section_name" or it is an empty string) are skipped and the processing
	//! continues from the next entry.
	//! \todo Protect the default Solar System configuration?
	//! \todo At least warn when overwriting old entries?
	bool appendToSolarSystemConfigurationFile(QList<SsoElements>);

	//! Flags to control the updateSolarSystemConfigurationFile() function.
	enum UpdateFlag {
		UpdateNameAndNumber 		= 0x01,	//!< Update the name and minor planet number, if any.
		UpdateType 			= 0x02,	//!< Update objects that lack the "type" parameter.
		UpdateOrbitalElements 		= 0x04,	//!< Update the orbital elements, including the orbit function.
		UpdateMagnitudeParameters 	= 0x08 	//!< Update the values in the two parameter system, or add them if they are missing and the type allows.
	};
	Q_DECLARE_FLAGS(UpdateFlags, UpdateFlag)

	//! Updates entries in the user solar system configuration file.
	//! \param objects a list of data for already existing objects (non-existing ones are skipped);
	//! \param flags flags controlling what is being updated. See UpdateFlag.
	//! \returns false if the operation has failed for some reason.
	bool updateSolarSystemConfigurationFile(QList<SsoElements> objects, UpdateFlags flags);

	//! Returns the names of the objects listed in the default ssystem_major.ini.
	//! The default solar system configuration file is assumed to be the one
	//! in the installation directory.
	QHash<QString,QString> getDefaultSsoIdentifiers() const {return defaultSsoIdentifiers;}

	//! Lists the objects listed in the current user ssystem.ini.
	//! As the name suggests, the list is compiled when the function is run.
	QHash<QString,QString> listAllLoadedSsoIdentifiers() const;

	//! Removes an object from the user Solar System configuration file.
	//! Reloads the Solar System on successful removal.
	//! \arg name true name of the object ("name" parameter in the configuration file)
	//! \returns true if the entry has been removed successfully or there is
	//! no such entry
	//! \returns false if there was an error
	bool removeSsoWithName(QString name);

	//! Export current minor bodies file from user data directory (if it exists) to filePath. Return true on success.
	bool copySolarSystemConfigurationFileTo(QString filePath);

	//! Replace current minor bodies file in the user data directory.
	//! Writes warning to logfile and returns false in case of problems.
	bool replaceSolarSystemConfigurationFileWith(QString filePath);

	//! (new 0.16)
	//! Loads all new objects from filePath (an .ini file), and updates existing objects
	//! Default proposal is ssystem_1000comets.ini in the installation dir.
	bool addFromSolarSystemConfigurationFile(QString filePath);

	//! returns the path
	QString getCustomSolarSystemFilePath() const {return customSolarSystemFilePath;}
	
public slots:
	//! Resets the Solar System configuration file and reloads the Solar System.
	//! \todo Return a bool and make the GUI display a message if it was not successful.
	void resetSolarSystemToDefault();

signals:
	//TODO: This should be part of SolarSystem::reloadPlanets()
	void solarSystemChanged();

private slots:
	void updateI18n();

private:
	bool isInitialized;

	//! Main window of the module's GUI
	SolarSystemManagerWindow * mainWindow;

	QSettings * solarSystemConfigurationFile;
	SolarSystem * solarSystem;

	QString customSolarSystemFilePath;
	QString defaultSolarSystemFilePath;
	QString majorSolarSystemFilePath; // install dir's ssystem_major.ini

	//! A hash matching SSO names with the group names used to identify them
	//! in the configuration file.

	//! The names and group names of all objects in the default ssystem_major.ini.
	//! The keys are the names, the values are the group names.
	//! Initialized in init().
	QHash<QString,QString> defaultSsoIdentifiers;

	//! Gets the names of the minor planet objects listed in a ssystem.ini-formatted file.
	//! Used internally in readAllCurrentSsoNames() and in init() to initialize
	//! defaultSsoNames.
	QHash<QString,QString> listAllLoadedObjectsInFile(QString filePath) const;

	//! Creates a copy of the default ssystem.ini file in the user data directory.
	//! \returns true if a file already exists or the copying has been successful
	bool cloneSolarSystemConfigurationFile() const;

	//! Replaces the user copy of ssystem.ini with the default one.
	//! This function simply deletes the file, if it exists, and calls
	//! cloneSolarSystemConfigurationFile().
	//! \returns true if the replacement has been successfull.
	bool resetSolarSystemConfigurationFile() const;

	//! Converts an alphanumeric digit as used in MPC packed dates to an integer.
	//! See http://www.minorplanetcenter.org/iau/info/PackedDates.html
	//! Interprets the digits from 0 to 9 normally, and the capital letters
	//! from A to V as numbers between 10 and 31.
	//! \returns -1 if the digit is invalid (0 is also an invalid ordinal number
	//! for a day or month, so this is not a problem)
	static int unpackDayOrMonthNumber (QChar digit);

	//! Converts an alphanumeric year number as used in MPC packed dates to an integer.
	//! See http://www.minorplanetcenter.org/iau/info/PackedDates.html
	//! Also used in packed provisional designations, see
	//! http://www.minorplanetcenter.org/iau/info/PackedDes.html
	static int unpackYearNumber (QChar prefix, int lastTwoDigits);

	//! Converts a two-character number used in MPC packed provisional designations.
	//! See http://www.minorplanetcenter.org/iau/info/PackedDes.html
	//! This function is used for both asteroid and comet designations.
	static int unpackAlphanumericNumber (QChar prefix, int lastDigit);

	//TODO: This should be public, perhaps?
	//! Unpacks an MPC packed minor planet provisional designation.
	//! See http://www.minorplanetcenter.org/iau/info/PackedDes.html
	//! \returns an empty string if the argument is not a valid packed
	//! provisional designation.
	static QString unpackMinorPlanetProvisionalDesignation(QString packedDesignation);

	//! Updates a value in a configuration file with a value with the same key in a SsoElements hash.
	static void updateSsoProperty(QSettings& configuration, SsoElements& properties, QString key);

	//! Converts an object name to a key (group) name in a configuration file.
	static QString convertToGroupName(QString& name, int minorPlanetNumber = 0);
	
	//! replaces "%25" by "%", then replaces "%28" by "(" and "%29" by ")".
	static QString fixGroupName(QString &name);
};



#include <QObject>
#include "StelPluginInterface.hpp"

//! This class is used by Qt to manage a plug-in interface
class SolarSystemEditorStelPluginInterface : public QObject, public StelPluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID StelPluginInterface_iid)
	Q_INTERFACES(StelPluginInterface)
public:
	virtual StelModule* getStelModule() const;
	virtual StelPluginInfo getPluginInfo() const;
	virtual QObjectList getExtensionList() const { return QObjectList(); }
};

#endif // SOLARSYSTEMEDITOR_HPP

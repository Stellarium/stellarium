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

#ifndef _C_A_IMPORTER_HPP_
#define _C_A_IMPORTER_HPP_

#include "StelGui.hpp"
#include "StelModule.hpp"
//#include "CAIMainWindow.hpp"

#include <QHash>
#include <QList>
#include <QString>
#include <QVariant>

class SolarSystemManagerWindow;
class QSettings;
/*!
 \class CAImporter
 \brief Main class of the Comets and Asteroids Importer plug-in.
 \author Bogdan Marinov
*/
class CAImporter : public StelModule
{
	Q_OBJECT

public:
	CAImporter();
	virtual ~CAImporter();
	
	///////////////////////////////////////////////////////////////////////////
	// Methods inherited from the StelModule class
	//! called when the plug-in is loaded.
	//! All initializations should be done here.
	virtual void init();
	//! called before the plug-in is un-loaded.
	//! Useful for stopping processes, unloading textures, etc.
	virtual void deinit();
	virtual void update(double deltaTime);
	//! draws on the view port.
	//! Dialog windows don't need explicit drawing, it's done automatically.
	//! If a plug-in draws on the screen, it should be able to respect
	//! the night vision mode.
	virtual void draw(StelCore * core);
	virtual double getCallOrder(StelModuleActionName actionName) const;
	//! called when the "configure" button in the "Plugins" tab is pressed
	virtual bool configureGui(bool show);

	//! Convenience type for storage of SSO properties in ssystem.ini format.
	//! This is an easy way of storing data in the format used in Stellarium's
	//! solar system configuration file.
	//! What would be key/value pairs in a section in the ssystem.ini file
	//! are key/value pairs in the hash. The section name is stored with key
	//! "section_name".
	//! As it is a hash, key names are not stored alphabetically. This allows
	//! for rapid addition and look-up of values, unlike a real QSettings
	//! object in StelIniFormat.
	//! Also, using this way may allow scripts to define SSOs.
	//! \todo Better name.
	typedef QHash<QString, QVariant> SsoElements;
	
	//! Reads a single comet's orbital elements from a string.
	//! This function converts a line of comet orbital elements in MPC format
	//! to a hash in Stellarium's ssystem.ini format.
	//! MPC's one-line orbital elements format for comets is described on their
	//! site: http://www.minorplanetcenter.org/iau/info/CometOrbitFormat.html
	//! \returns an empty hash if there is an error or the source string is not
	//! a valid line in MPC format.
	SsoElements readMpcOneLineCometElements (QString oneLineElements);

	//! Reads a list of comet orbital elements from a file.
	//! This function reads a list of comet orbital elements in MPC's one-line
	//! format from a file (one comet per line) and converts it to a list of
	//! hashes in Stellarium's ssystem.ini format.
	//! Example source file is the list of observable comets on the MPC's site:
	//! http://www.minorplanetcenter.org/iau/Ephemerides/Comets/Soft00Cmt.txt
	//! readMpcOneLineCometElements() is used internally to parse each line.
	QList<SsoElements> readMpcOneLineCometElementsFromFile (QString filePath);

	//! Adds a new entry at the end of the user solar system configuration file.
	//! This function writes directly to the file. See the note on why QSettings
	//! was not used in the description of
	//! appendToSolarSystemConfigurationFile(QList<SsoElements>)
	//! Duplicates are removed: If any section in the file matches the
	//! "section_name" value of the inserted entry, it is removed.
	bool appendToSolarSystemConfigurationFile(SsoElements object);

	//! Adds new entries at the end of the user solar system configuration file.
	//! This function writes directly to the file. QSettings was not used, as:
	//!  - Using QSettings with QSettings::IniFormat causes the list in the
	//! "color" field (e.g. "1.0, 1.0, 1.0") to be wrapped in double quotation
	//! marks (Stellarium requires no quotation marks).
	//!  - Using QSettings with StelIniFormat causes unaccepptable append times
	//! when the file grows (>~40 entries). This most probably happens because
	//! StelIniParser uses QMap internally for the entry list. QMap orders its
	//! keys (in the case of strings - alphabetically) and it has to find
	//! the appropriate place in the ordering for every new key, which takes
	//! more and more time as the list grows.
	//!
	//! Duplicates are removed: If any section in the file matches the
	//! "section_name" value of a new entry, it is removed.
	//! Invalid entries in the list (that don't contain a value for
	//! "section_name" or it is an empty string) are skipped and the processing
	//! continues from the next entry.
	bool appendToSolarSystemConfigurationFile(QList<SsoElements>);
	
public slots:
	
private:
	SolarSystemManagerWindow * mainWindow;

	//! Creates a copy of the default ssystem.ini file in the user data directory.
	//! @returns true if a file already exists or the copying has been successful
	bool cloneSolarSystemConfigurationFile();
	//! Removes the user copy of ssystem.ini, if any exists.
	bool resetSolarSystemConfigurationFile();

	QSettings * solarSystemConfigurationFile;
};


#include "fixx11h.h"
#include <QObject>
#include "StelPluginInterface.hpp"

//! This class is used by Qt to manage a plug-in interface
class CAImporterStelPluginInterface : public QObject, public StelPluginInterface
{
	Q_OBJECT
	Q_INTERFACES(StelPluginInterface)
public:
	virtual StelModule* getStelModule() const;
	virtual StelPluginInfo getPluginInfo() const;
};

#endif //_C_A_IMPORTER_HPP_

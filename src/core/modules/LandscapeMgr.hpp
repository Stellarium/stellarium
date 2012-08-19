/*
 * Stellarium
 * Copyright (C) 2006 Fabien Chereau
 * Copyright (C) 2010 Bogdan Marinov (add/remove landscapes feature)
 * Copyright (C) 2012 Timothy Reaves
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

#ifndef _LANDSCAPEMGR_HPP_
#define _LANDSCAPEMGR_HPP_

#include "StelModule.hpp"
#include "StelUtils.hpp"

#include <QMap>
#include <QStringList>

class Landscape;
class Atmosphere;
class Cardinals;
class QSettings;

//! @class LandscapeMgr
//! Manages all the rendering at the level of the observer's surroundings.
//! This includes landscape textures, fog, atmosphere and cardinal points.
//! I decided to put all these elements together in a single class because they are
//! inherently linked, especially when we start moving the observer in altitude.
class LandscapeMgr : public StelModule
{
	Q_OBJECT
	Q_PROPERTY(bool atmosphereDisplayed
			   READ getFlagAtmosphere
			   WRITE setFlagAtmosphere
			   NOTIFY atmosphereDisplayedChanged)
	Q_PROPERTY(bool cardinalsPointsDisplayed
			   READ getFlagCardinalsPoints
			   WRITE setFlagCardinalsPoints
			   NOTIFY cardinalsPointsDisplayedChanged)
	Q_PROPERTY(bool fogDisplayed
			   READ getFlagFog
			   WRITE setFlagFog
			   NOTIFY fogDisplayedChanged)
	Q_PROPERTY(bool landscapeDisplayed
			   READ getFlagLandscape
			   WRITE setFlagLandscape
			   NOTIFY landscapeDisplayedChanged)

public:
	LandscapeMgr();
	virtual ~LandscapeMgr();

	///////////////////////////////////////////////////////////////////////////
	// Methods defined in the StelModule class
	//! Initialize the LandscapeManager class.
	//! Operations performed:
	//! - Load the default landscape as specified in the application configuration
	//! - Set up landscape-releated display flags from ini parser object
	virtual void init();

	//! Draw the landscape graphics, cardinal points and atmosphere.
	virtual void draw(StelCore* core, class StelRenderer* renderer);

	//! Update time-dependent state.
	//! Includes:
	//! - Landscape, atmosphere and cardinal point on/off fading.
	//! - Atmophere colour calulation based on location, position of sun
	//!   and moon.
	//! - updates adaptation lumenescence lased on visible bright objects.
	virtual void update(double deltaTime);

	//! Get the order in which this module will draw it's objects relative to other modules.
	virtual double getCallOrder(StelModuleActionName actionName) const;


	///////////////////////////////////////////////////////////////////////////
	// Method specific to the landscape manager
	//! Return the global landscape luminance, for being used e.g for setting eye adaptation.
	float getLuminance();

	//! Load a landscape based on a hash of parameters mirroring the landscape.ini
	//! file and make it the current landscape.
	bool loadLandscape(QMap<QString, QString>& param);

	//! Create a new landscape from the files which describe it.
	//! Reads a landscape.ini file which is passed as the first parameter, determines
	//! the landscape type, and creates a new object for the landscape of the proper
	//! type.  The load member is then called, passing both parameters.
	//! @param landscapeFile This is the path to a landscape.ini file.
	//! @param landscapeId This is the landscape ID, which is also the name of the
	//! directory in which the files (textures and so on) for the landscape reside.
	//! @return A pointer to the newly created landscape object.
	Landscape* createFromFile(const QString& landscapeFile, const QString& landscapeId);

public slots:
	///////////////////////////////////////////////////////////////////////////
	// Methods callable from script and GUI
	//! Retrieve a list of the names of all the available landscapes in
	//! the file search path sub-directories of the landscape area
	//! @return the names of the landscapes, which are the values of the name parameter in the landscape.ini files
	QStringList getAllLandscapeNames() const;

	//! Retrieve a list of the identifiers of all the available landscapes in
	//! the file search path sub-directories of the landscape area
	//! @return the identifiers of the landscapes, which are the names of the directories containing the landscapes' files
	QStringList getAllLandscapeIDs() const;

	//! Retrieve a list of the identifiers of all user-installed landscapes.
	//! Effectively, this returns the results of getAllLandscapeIDs() without
	//! the landscapes specified in the #packagedLandscapeIDs list.
	QStringList getUserLandscapeIDs() const;

	//! Get the current landscape ID.
	const QString& getCurrentLandscapeID() const {return currentLandscapeID;}
	//! Change the current landscape to the landscape with the ID specified.
	//! @param id the ID of the new landscape
	//! @return false if the new landscape could not be set (e.g. no landscape of that ID was found).
	bool setCurrentLandscapeID(const QString& id);
	
	//! Get the current landscape name.
	QString getCurrentLandscapeName() const;
	//! Change the current landscape to the landscape with the name specified.
	//! @param name the name of the new landscape, as found in the landscape:name key of the landscape.ini file.
	bool setCurrentLandscapeName(const QString& name);

	//! Get the default landscape ID.
	const QString& getDefaultLandscapeID() const {return defaultLandscapeID;}
	//! Change the default landscape to the landscape with the ID specified.
	//! @param id the ID of the landscape to use by default
	//! @return false if the new landscape could not be set (e.g. no landscape of that ID was found). True on success.
	bool setDefaultLandscapeID(const QString& id);

	//! Return a pseudo HTML formated string with all informations on the current landscape
	QString getCurrentLandscapeHtmlDescription() const;

	//! Return a pseudo HTML formated string with information from description or ini file
	QString getDescription() const;

	//! Get flag for displaying Landscape.
	bool getFlagLandscape() const;
	//! Set flag for displaying Landscape.
	void setFlagLandscape(const bool displayed);

	//! Get flag for displaying Fog.
	bool getFlagFog() const;
	//! Set flag for displaying Fog.
	void setFlagFog(const bool displayed);

	//! Return the value of the flag determining if a change of landscape will update the observer location.
	bool getFlagLandscapeSetsLocation() const {return flagLandscapeSetsLocation;}
	//! Set the value of the flag determining if a change of landscape will update the observer location.
	void setFlagLandscapeSetsLocation(bool b) {flagLandscapeSetsLocation=b;}

	//! Get flag for displaying Cardinals Points.
	bool getFlagCardinalsPoints() const;
	//! Set flag for displaying Cardinals Points.
	void setFlagCardinalsPoints(const bool displayed);

	//! Get Cardinals Points color.
	Vec3f getColorCardinalPoints() const;
	//! Set Cardinals Points color.
	void setColorCardinalPoints(const Vec3f& v);

	//! Get flag for displaying Atmosphere.
	bool getFlagAtmosphere() const;
	//! Set flag for displaying Atmosphere.
	void setFlagAtmosphere(const bool displayed);

	//! Get atmosphere fade duration in s.
	float getAtmosphereFadeDuration() const;
	//! Set atmosphere fade duration in s.
	void setAtmosphereFadeDuration(float f);

	//! Set the light pollution following the Bortle Scale
	void setAtmosphereBortleLightPollution(int bIndex);
	//! Get the light pollution following the Bortle Scale
	int getAtmosphereBortleLightPollution();

	//! Set the rotation of the landscape about the z-axis.
	//! This is intended for special uses such as when the landscape consists of
	//! a vehicle which might change orientation over time (e.g. a ship).
	//! @param d the rotation angle in degrees as an offset from the originally loaded value.
	void setZRotation(float d);

	//! Install a landscape from a ZIP archive.
	//! This function searches for a file named "landscape.ini" in the root
	//! directory of the archive. If it is not found there, the function
	//! searches inside the topmost sub-directories (if any), but no deeper.
	//! If a landscape configuration file is found:
	//!  - if a "landscapes" directory does not exist in the user data
	//! directory, it is created;
	//!  - inside it, a sub-directory is created with the landscape identifier
	//! for a name;
	//!  - all files in the archive directory that contains the "landscape.ini"
	//! file are extracted to the new sub-directory of "landscapes";
	//!  - all sub-directories of that archive directory will be skipped along
	//! with any other files or directories in the archive.
	//!
	//! The landscape identifier is either:
	//!  - the name of the folder in the archive that contains "landscape.ini",
	//!  - or the first 65 (or less) characters of the archive name, if the
	//! "landscape.ini" file is in the nameless root directory of the archive.
	//!
	//! The landscape identifier must be unique.
	//! @param pathToSourceArchive path to the source archive file.
	//! @param display If true, the landscape will be set to be the current
	//! landscape after installation.
	//! @param forAllUsers If true, this function will try to install the
	//! landscape in a way that meakes it is available to all users of this
	//! computer. May require running Stellarium as an administrator (root)
	//! on some Windows or *nix systems. (NOT IMPLEMENTED!)
	//! @returns the installed landscape's identifier, or
	//! an empty string on failure.
	//! @todo Find a better way to pass error messages.
	QString installLandscapeFromArchive(QString pathToSourceArchive, bool display = false, bool forAllUsers = false);

	// //! Install a landscape from a directory.
	// //! Expected directory structure: the name of the directory that contains
	// //! a landscape.ini file is assumed to be the landscape ID and should be
	// //! unique.
	// //! This directory and all files in it will be installed, but its
	// //! subdirectories will be skipped along with any other files or
	// //! directories in the archive.
	// //! @param pathToSourceLandscapeIni path to a landscape.ini file. Its parent
	// //! directory is assumed to be the landscape source directory.
	// //! @param display If true, the landscape will be set to be the current
	// //! landscape after installation.
	// //! @param forAllUsers If true, this function will try to install the
	// //! landscape in a way that meakes it is available to all users of this
	// //! computer. May require running Stellarium as an administrator (root)
	// //! on some Windows or *nix systems. (NOT IMPLEMENTED!)
	// //! @returns the installed landscape's identifier (the folder name), or
	// //! an empty string on failure.
	// QString installLandscapeFromDirectory(QString pathToSourceLandscapeIni, bool display = false, bool forAllUsers = false);

	//! This function removes a landscape from the user data directory.
	//! It tries to recursively delete all files in the landscape directory
	//! and then remove it from the list of available landscapes.
	//! If the function encounters any file that can't be deleted
	//! it aborts the operation (previously deleted files are not restored).
	//! Landscapes that were packaged with Stellarium can't be removed,
	//! thanks to the #packagedLandscapeIDs list.
	//! @param landscapeID an installed landscape's identifier (the folder name)
	//! @todo Find a better way to pass error messages.
	bool removeLandscape(QString landscapeID);

	//! This function reads a landscape's name from its configuration file.
	//! @param landscapeID an installed landscape's identifier (the folder name)
	//! @returns an empty string if there is no such landscape or some other
	//! error occurs
	QString loadLandscapeName(QString landscapeID);

	//! This function calculates and returns a landscape's disc size in bytes.
	//! It adds up the sizes of all files in the landscape's folder. It assumes
	//! that there are no sub-directories. (There shouldn't be any anyway.)
	//! @param landscapeID an installed landscape's identifier (the folder name)
	quint64 loadLandscapeSize(QString landscapeID);

signals:
	void atmosphereDisplayedChanged(const bool displayed);
	void cardinalsPointsDisplayedChanged(const bool displayed);
	void fogDisplayedChanged(const bool displayed);
	void landscapeDisplayedChanged(const bool displayed);

	//! Emitted when a landscape has been installed or un-installed.
	//! For example, it is used to update the list of landscapes in
	//! the Sky and viewing options window (the ViewDialog class)
	void landscapesChanged();

	//! Emitted when installLandscapeFromArchive() can't read from, write to or
	//! create a file or a directory.
	//! (A way of moving the need for translatable error messages to the GUI.)
	//! \param path path to the file or directory
	void errorUnableToOpen(QString path);
	//! Emitted when the file passed to installLandscapeFromArchive() is not a
	//! ZIP archive or does not contain a valid landscape.
	//! (A way of moving the need for translatable error messages to the GUI.)
	void errorNotArchive();
	//! Emitted when installLandscapeFromArchive() tries to install a landscape
	//! with the same name or identifier as an already installed one.
	//! (A way of moving the need for translatable error messages to the GUI.)
	//! \param nameOrID the name or the identifier of the landscape
	void errorNotUnique(QString nameOrID);
	//! Emitted when removeLandscape() is unable to remove all the files of
	//! a landscape.
	//! (A way of moving the need for translatable error messages to the GUI.)
	//! \param path the path to the landscape's directory
	void errorRemoveManually(QString path);

private slots:
	//! Load a color scheme from a configuration object
	void setStelStyle(const QString& section);

	//! Translate labels to new language settings.
	void updateI18n();	

private:
	//! Get light pollution luminance level.
	float getAtmosphereLightPollutionLuminance() const;
	//! Set light pollution luminance level.
	void setAtmosphereLightPollutionLuminance(float f);

	//! For a given landscape name, return the landscape ID.
	//! This takes a name of the landscape, as described in the landscape:name item in the
	//! landscape.ini, and returns the landscape ID which corresponds to that name.
	QString nameToID(const QString& name);

	//! Return a map of landscape name to landscape ID (directory name).
	QMap<QString,QString> getNameToDirMap() const;

	//! Returns the path to an installed landscape's directory.
	//! It uses StelFileMgr to look for it in the possible directories.
	//! @param landscapeID an installed landscape's identifier (the folder name)
	//! @returns an empty string, if no such landscape was found.
	QString getLandscapePath(QString landscapeID);

	Atmosphere* atmosphere;			// Atmosphere
	Cardinals* cardinalsPoints;		// Cardinals points
	Landscape* landscape;			// The landscape i.e. the fog, the ground and "decor"

	// Define whether the observer location is to be updated when the landscape is updated.
	bool flagLandscapeSetsLocation;

	// The ID of the currently loaded landscape
	QString currentLandscapeID;

	// The ID of the default landscape
	QString defaultLandscapeID;

	//! List of the IDs of the landscapes packaged by default with Stellarium.
	//! (So that they can't be removed.)
	//! It is populated in LandscapeMgr() and has to be updated
	//! manually on changes.
	//! @todo Find a way to update it automatically.
	QStringList packagedLandscapeIDs;
};

#endif // _LANDSCAPEMGR_HPP_

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
#include "Landscape.hpp"

#include <QMap>
#include <QStringList>

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
	Q_PROPERTY(bool illuminationDisplayed
			   READ getFlagIllumination
			   WRITE setFlagIllumination
			   NOTIFY illuminationDisplayedChanged)
	Q_PROPERTY(bool labelsDisplayed
			   READ getFlagLabels
			   WRITE setFlagLabels
			   NOTIFY labelsDisplayedChanged)
	Q_PROPERTY(bool databaseUsage
			READ getFlagUseLightPollutionFromDatabase
			WRITE setFlagUseLightPollutionFromDatabase
			NOTIFY lightPollutionUsageChanged)
	Q_PROPERTY(bool autoSelect MEMBER flagLandscapeAutoSelection NOTIFY changed)
	Q_PROPERTY(int atmosphereBortleLightPollution READ getAtmosphereBortleLightPollution WRITE setAtmosphereBortleLightPollution NOTIFY lightPollutionChanged)
	Q_PROPERTY(bool landscapeSetsLocation MEMBER flagLandscapeSetsLocation NOTIFY changed)
	Q_PROPERTY(QString currentLandscapeID READ getCurrentLandscapeID WRITE setCurrentLandscapeID NOTIFY changed)
	Q_PROPERTY(QString defaultLandscapeID READ getDefaultLandscapeID WRITE setDefaultLandscapeID NOTIFY changed)
	Q_PROPERTY(bool useMinimalBrightness MEMBER flagLandscapeUseMinimalBrightness NOTIFY changed)
	Q_PROPERTY(double defaultMinimalBrightness MEMBER defaultMinimalBrightness NOTIFY changed)
	Q_PROPERTY(bool setMinimalBrighntess MEMBER flagLandscapeSetsMinimalBrightness NOTIFY changed)

public:
	LandscapeMgr();
	virtual ~LandscapeMgr();

	///////////////////////////////////////////////////////////////////////////
	// Methods defined in the StelModule class
	//! Initialize the LandscapeManager class.
	//! Operations performed:
	//! - Load the default landscape as specified in the application configuration
	//! - Set up landscape-related display flags from ini parser object
	virtual void init();

	//! Draw the landscape graphics, cardinal points and atmosphere.
	virtual void draw(StelCore* core);

	//! Update time-dependent state.
	//! Includes:
	//! - Landscape, atmosphere and cardinal point on/off fading.
	//! - Atmophere colour calculation based on location, position of sun
	//!   and moon.
	//! - updates adaptation luminescence based on visible bright objects.
	//! - Landscape and lightscape brightness computations based on sun position and whether atmosphere is on or off.
	virtual void update(double deltaTime);

	//! Get the order in which this module will draw its objects relative to other modules.
	virtual double getCallOrder(StelModuleActionName actionName) const;


	///////////////////////////////////////////////////////////////////////////
	// Method specific to the landscape manager
	//! Return the global landscape luminance [0..1], for being used e.g for setting eye adaptation.
	//! It returns 1 if atmosphere drawing is on and no eclipse underway, 0 if atmosphere is switched off.
	//! The actual brightness is of no concern here. You may use getAtmosphereAverageLuminance() for this.
	float getLuminance() const;
	//! return average luminance [cd/m^2] of atmosphere. Around 10 at sunset, 6400 in daylight, >0 in dark night.
	float getAtmosphereAverageLuminance() const;

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

	// GZ: implement StelModule's method. For test purposes only, we implement a manual transparency sampler.
	// TODO: comment this away for final builds. Please leave it in until this feature is finished.
	// virtual void handleMouseClicks(class QMouseEvent*);

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
	//! @param changeLocationDuration the duration of the transition animation
	//! @return false if the new landscape could not be set (e.g. no landscape of that ID was found).
	bool setCurrentLandscapeID(const QString& id, const double changeLocationDuration = 1.0);
	
	//! Get the current landscape name.
	QString getCurrentLandscapeName() const;
	//! Change the current landscape to the landscape with the name specified.
	//! @param name the name of the new landscape, as found in the landscape:name key of the landscape.ini file.
	//! @param changeLocationDuration the duration of the transition animation
	bool setCurrentLandscapeName(const QString& name, const double changeLocationDuration = 1.0);

	//! Get the current landscape object.
	Landscape* getCurrentLandscape() const { return landscape; }

	//! Get the default landscape ID.
	const QString& getDefaultLandscapeID() const {return defaultLandscapeID;}
	//! Change the default landscape to the landscape with the ID specified.
	//! @param id the ID of the landscape to use by default
	//! @return false if the new landscape could not be set (e.g. no landscape of that ID was found). True on success.
	bool setDefaultLandscapeID(const QString& id);

	//! Return a pseudo HTML formatted string with all informations on the current landscape
	QString getCurrentLandscapeHtmlDescription() const;

	//! Return a pseudo HTML formatted string with information from description or ini file
	QString getDescription() const;

	//! Get flag for displaying Landscape.
	bool getFlagLandscape() const;
	//! Set flag for displaying Landscape.
	void setFlagLandscape(const bool displayed);

	//! Get whether the landscape is currently visible. If true, objects below landscape's limiting altitude limit can be omitted.
	bool getIsLandscapeFullyVisible() const;
	//! Get the sine of current landscape's minimal altitude. Useful to construct bounding caps.
	float getLandscapeSinMinAltitudeLimit() const;
	
	//! Get flag for displaying Fog.
	bool getFlagFog() const;
	//! Set flag for displaying Fog.
	void setFlagFog(const bool displayed);
	//! Get flag for displaying illumination layer
	bool getFlagIllumination() const;
	//! Set flag for displaying illumination layer
	void setFlagIllumination(const bool on);
	//! Get flag for displaying landscape labels
	bool getFlagLabels() const;
	//! Set flag for displaying landscape labels
	void setFlagLabels(const bool on);

	//! Return the value of the flag determining if a change of landscape will update the observer location.
	bool getFlagLandscapeSetsLocation() const {return flagLandscapeSetsLocation;}
	//! Set the value of the flag determining if a change of landscape will update the observer location.
	void setFlagLandscapeSetsLocation(bool b) {flagLandscapeSetsLocation=b;}

	//! Return the value of the flag determining if a minimal brightness should be used to keep landscape visible.
	bool getFlagLandscapeUseMinimalBrightness() const {return flagLandscapeUseMinimalBrightness; }
	//! Set the value of the flag determining if a minimal brightness should be used to keep landscape visible.
	void setFlagLandscapeUseMinimalBrightness(bool b) {flagLandscapeUseMinimalBrightness=b; }
	//! Return the value of the flag determining if the minimal brightness should be taken from landscape.ini
	bool getFlagLandscapeSetsMinimalBrightness() const {return flagLandscapeSetsMinimalBrightness;}
	//! Sets the value of the flag determining if the minimal brightness should be taken from landscape.ini
	void setFlagLandscapeSetsMinimalBrightness(bool b) {flagLandscapeSetsMinimalBrightness=b;}
	//! Return the minimal brightness value of the landscape
	double getDefaultMinimalBrightness() const {return defaultMinimalBrightness;}
	//! Set the minimal brightness value of the landscape.
	void setDefaultMinimalBrightness(const double b) {defaultMinimalBrightness=b;}
	//! Sets the value of the flag usage light pollution (and bortle index) from locations database.
	void setFlagUseLightPollutionFromDatabase(const bool usage);
	//! Return the value of flag usage light pollution (and bortle index) from locations database.
	bool getFlagUseLightPollutionFromDatabase() const;

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
	void setAtmosphereFadeDuration(const float f);

	//! Set the light pollution following the Bortle Scale. Emits lightPollutionChanged().
	void setAtmosphereBortleLightPollution(const int bIndex);
	//! Get the light pollution following the Bortle Scale
	int getAtmosphereBortleLightPollution() const;

	//! Set the rotation of the landscape about the z-axis.
	//! This is intended for special uses such as when the landscape consists of
	//! a vehicle which might change orientation over time (e.g. a ship).
	//! @param d the rotation angle in degrees as an offset from the originally loaded value.
	void setZRotation(const float d);

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
	QString installLandscapeFromArchive(QString pathToSourceArchive, const bool display = false, const bool forAllUsers = false);

	/* GZ: leaving doc without the method confuses Doxygen. Commenting out completely.
	//! Install a landscape from a directory.
	//! Expected directory structure: the name of the directory that contains
	//! a landscape.ini file is assumed to be the landscape ID and should be
	//! unique.
	//! This directory and all files in it will be installed, but its
	//! subdirectories will be skipped along with any other files or
	//! directories in the archive.
	//! @param pathToSourceLandscapeIni path to a landscape.ini file. Its parent
	//! directory is assumed to be the landscape source directory.
	//! @param display If true, the landscape will be set to be the current
	//! landscape after installation.
	//! @param forAllUsers If true, this function will try to install the
	//! landscape in a way that meakes it is available to all users of this
	//! computer. May require running Stellarium as an administrator (root)
	//! on some Windows or *nix systems. (NOT IMPLEMENTED!)
	//! @returns the installed landscape's identifier (the folder name), or
	//! an empty string on failure.
	//QString installLandscapeFromDirectory(QString pathToSourceLandscapeIni, bool display = false, bool forAllUsers = false);
	*/

	//! This function removes a landscape from the user data directory.
	//! It tries to recursively delete all files in the landscape directory
	//! and then remove it from the list of available landscapes.
	//! If the function encounters any file that can't be deleted
	//! it aborts the operation (previously deleted files are not restored).
	//! Landscapes that were packaged with Stellarium can't be removed,
	//! thanks to the #packagedLandscapeIDs list.
	//! @param landscapeID an installed landscape's identifier (the folder name)
	//! @todo Find a better way to pass error messages.
	bool removeLandscape(const QString landscapeID);

	//! This function reads a landscape's name from its configuration file.
	//! @param landscapeID an installed landscape's identifier (the folder name)
	//! @returns an empty string if there is no such landscape or some other
	//! error occurs
	QString loadLandscapeName(const QString landscapeID);

	//! This function calculates and returns a landscape's disc size in bytes.
	//! It adds up the sizes of all files in the landscape's folder. It assumes
	//! that there are no sub-directories. (There shouldn't be any anyway.)
	//! @param landscapeID an installed landscape's identifier (the folder name)
	quint64 loadLandscapeSize(const QString landscapeID) const;

	//! Get flag for autoselect of landscapes for planets.
	bool getFlagLandscapeAutoSelection() const;
	//! Set flag for autoselect of landscapes for planets.
	void setFlagLandscapeAutoSelection(bool enableAutoSelect);

	//! Get flag for auto-enable of atmospheres for planets.
	bool getFlagAtmosphereAutoEnable() const;
	//! Set flag for auto-enable atmosphere for planets with atmospheres in location window
	void setFlagAtmosphereAutoEnable(bool b);

	//! Forward opacity query to current landscape.
	//! @param azalt direction of view line to sample in azaltimuth coordinates.
	float getLandscapeOpacity(Vec3d azalt) const {return landscape->getOpacity(azalt);}

signals:
	void atmosphereDisplayedChanged(const bool displayed);
	void cardinalsPointsDisplayedChanged(const bool displayed);
	void fogDisplayedChanged(const bool displayed);
	void landscapeDisplayedChanged(const bool displayed);
	void illuminationDisplayedChanged(const bool displayed);
	void labelsDisplayedChanged(const bool displayed);
	void lightPollutionUsageChanged(const bool usage);
	void changed();

	//! Emitted when a landscape has been installed or un-installed.
	//! For example, it is used to update the list of landscapes in
	//! the Sky and viewing options window (the ViewDialog class)
	void landscapesChanged();

	//! emitted by setAtmosphereBortleLightPollution().
	void lightPollutionChanged();

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
	void setAtmosphereLightPollutionLuminance(const float f);


	//! For a given landscape name, return the landscape ID.
	//! This takes a name of the landscape, as described in the landscape:name item in the
	//! landscape.ini, and returns the landscape ID which corresponds to that name.
	QString nameToID(const QString& name) const;

	//! Return a map of landscape name to landscape ID (directory name).
	QMap<QString,QString> getNameToDirMap() const;

	//! Returns the path to an installed landscape's directory.
	//! It uses StelFileMgr to look for it in the possible directories.
	//! @param landscapeID an installed landscape's identifier (the folder name)
	//! @returns an empty string, if no such landscape was found.
	QString getLandscapePath(const QString landscapeID) const;

	Atmosphere* atmosphere;			// Atmosphere
	Cardinals* cardinalsPoints;		// Cardinals points
	Landscape* landscape;			// The landscape i.e. the fog, the ground and "decor"
	Landscape* oldLandscape;		// Used only during transitions to newly loaded landscape.

	// Define whether the observer location is to be updated when the landscape is updated.
	bool flagLandscapeSetsLocation;

	bool flagLandscapeAutoSelection;

	bool flagLightPollutionFromDatabase;

	//! Indicate use of the default minimal brightness value specified in config.ini.
	bool flagLandscapeUseMinimalBrightness;
	//! A minimal brightness value to keep landscape visible.
	float defaultMinimalBrightness;
	//! Indicate use of the minimal brightness value specified in the current landscape.ini, if present.
	bool flagLandscapeSetsMinimalBrightness;
	//! Indicate auto-enable atmosphere for planets with atmospheres in location window
	bool flagAtmosphereAutoEnabling;

	// The ID of the currently loaded landscape
	QString currentLandscapeID;

	// The ID of the default landscape
	QString defaultLandscapeID;

	//! List of the IDs of the landscapes packaged by default with Stellarium.
	//! (So that they can't be removed.)
	QStringList packagedLandscapeIDs;

};

#endif // _LANDSCAPEMGR_HPP_

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

#ifndef LANDSCAPEMGR_HPP
#define LANDSCAPEMGR_HPP

#include "StelModule.hpp"
#include "StelUtils.hpp"
#include "Landscape.hpp"

#include <QMap>
#include <QStringList>
#include <QCache>

class Atmosphere;
class Cardinals;
class QSettings;

//! @class LandscapeMgr
//! Manages all the rendering at the level of the observer's surroundings.
//! This includes landscape textures, fog, atmosphere and cardinal points.
//! I decided to put all these elements together in a single class because they are
//! inherently linked, especially when we start moving the observer in altitude.
//! \note
//! The Bortle scale index setting was removed from this class, because it was duplicated
//! from StelSkyDrawer, complicating code that changes it.
//! It is now only in StelSkyDrawer and can be accessed
//! with \link StelSkyDrawer::getBortleScaleIndex getBortleScaleIndex \endlink
//! and \link StelSkyDrawer::setBortleScaleIndex setBortleScaleIndex \endlink.
//! Slots setAtmosphereBortleLightPollution and getAtmosphereBortleLightPollution
//! in this class have been removed/made private.
//! If script access is desired, use
//! \link StelMainScriptAPI::getBortleScaleIndex StelMainScriptAPI::get \endlink/\link StelMainScriptAPI::setBortleScaleIndex setBortleScaleIndex \endlink
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
	Q_PROPERTY(Vec3f cardinalsPointsColor
		   READ getColorCardinalPoints
		   WRITE setColorCardinalPoints
		   NOTIFY cardinalsPointsColorChanged)
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
	Q_PROPERTY(bool flagPolyLineDisplayedOnly
		   READ getFlagPolyLineDisplayed
		   WRITE setFlagPolyLineDisplayed
		   NOTIFY flagPolyLineDisplayedChanged)
	Q_PROPERTY(int polyLineThickness
		   READ getPolyLineThickness
		   WRITE setPolyLineThickness
		   NOTIFY polyLineThicknessChanged)
	Q_PROPERTY(bool flagUseLightPollutionFromDatabase
		   READ getFlagUseLightPollutionFromDatabase
		   WRITE setFlagUseLightPollutionFromDatabase
		   NOTIFY flagUseLightPollutionFromDatabaseChanged)
	Q_PROPERTY(bool flagLandscapeAutoSelection
		   READ getFlagLandscapeAutoSelection
		   WRITE setFlagLandscapeAutoSelection
		   NOTIFY flagLandscapeAutoSelectionChanged)
	Q_PROPERTY(bool flagLandscapeSetsLocation
		   READ getFlagLandscapeSetsLocation
		   WRITE setFlagLandscapeSetsLocation
		   NOTIFY flagLandscapeSetsLocationChanged)
	Q_PROPERTY(bool flagLandscapeUseMinimalBrightness
		   READ getFlagLandscapeUseMinimalBrightness
		   WRITE setFlagLandscapeUseMinimalBrightness
		   NOTIFY flagLandscapeUseMinimalBrightnessChanged)
	Q_PROPERTY(bool flagLandscapeSetsMinimalBrightness
		   READ getFlagLandscapeSetsMinimalBrightness
		   WRITE setFlagLandscapeSetsMinimalBrightness
		   NOTIFY flagLandscapeSetsMinimalBrightnessChanged)
	Q_PROPERTY(double defaultMinimalBrightness
		   READ getDefaultMinimalBrightness
		   WRITE setDefaultMinimalBrightness
		   NOTIFY defaultMinimalBrightnessChanged)
	Q_PROPERTY(bool flagEnvironmentAutoEnabling
		   READ getFlagEnvironmentAutoEnable
		   WRITE setFlagEnvironmentAutoEnable
		   NOTIFY setFlagEnvironmentAutoEnableChanged)
	Q_PROPERTY(QString currentLandscapeID
		   READ getCurrentLandscapeID
		   WRITE setCurrentLandscapeID
		   NOTIFY currentLandscapeChanged)
	Q_PROPERTY(QStringList allLandscapeNames
		   READ getAllLandscapeNames)
	Q_PROPERTY(QString currentLandscapeName
		   READ getCurrentLandscapeName
		   WRITE setCurrentLandscapeName
		   NOTIFY currentLandscapeChanged)
	Q_PROPERTY(QString currentLandscapeHtmlDescription
		   READ getCurrentLandscapeHtmlDescription
		   NOTIFY currentLandscapeChanged)
	Q_PROPERTY(QString defaultLandscapeID
		   READ getDefaultLandscapeID
		   WRITE setDefaultLandscapeID
		   NOTIFY defaultLandscapeChanged)

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

	//! Draw the atmosphere, landscape graphics, and cardinal points.
	virtual void draw(StelCore* core);
	//! Draw landscape graphics and cardinal points. This only will redraw a polygonal line (if defined), the gazetteer and the Cardinal points.
	//! This can be called outside the usual call order, if any foreground has to be overdrawn, e.g. 3D sceneries.
	void drawPolylineOnly(StelCore* core);

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
	// Methods specific to the landscape manager

	// Load a landscape based on a hash of parameters mirroring the landscape.ini
	// file and make it the current landscape.
	// GZ: This was declared, but not implemented(?)
	//bool loadLandscape(QMap<QString, QString>& param);

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
	// Methods callable from scripts and GUI
	//! Return the global landscape luminance [0..1], for being used e.g for setting eye adaptation.
	//! It returns 1 if atmosphere drawing is on and no eclipse underway, 0 if atmosphere is switched off.
	//! The actual brightness is of no concern here. You may use getAtmosphereAverageLuminance() for this.
	float getLuminance() const;
	//! return average luminance [cd/m^2] of atmosphere. Expect 10 at sunset, 6400 in daylight, >0 in dark night.
	float getAtmosphereAverageLuminance() const;

	//! Override autocomputed value and set average luminance [cd/m^2] of atmosphere.  This is around 10 at sunset, 6400 in daylight, >0 in dark night.
	//! Usually there is no need to call this, the luminance is properly computed. This is a function which can be
	//! useful in rare cases, e.g. in scripts when you want to create images of adjacent sky regions with the same brightness setting,
	//! or for creation of a virtual camera which can deliberately show over- or underexposure.
	//! For these cases, it is advisable to first center the brightest luminary (sun or moon), call getAtmosphereAverageLuminance() and then set
	//! this value explicitly to freeze it during image export. To unfreeze, call this again with any negative value.
	void setAtmosphereAverageLuminance(const float overrideLuminance);

	//! Return a map of landscape names to landscape IDs (directory names).
	static QMap<QString,QString> getNameToDirMap();

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
	const QString getCurrentLandscapeID() const {return currentLandscapeID;}
	//! Change the current landscape to the landscape with the ID specified.
	//! Emits currentLandscapeChanged() if the landscape changed (true returned)
	//! @param id the ID of the new landscape
	//! @param changeLocationDuration the duration of the transition animation
	//! @return false if the new landscape could not be set (e.g. no landscape of that ID was found).
	bool setCurrentLandscapeID(const QString& id, const double changeLocationDuration = 1.0);
	
	//! Get the current landscape name.
	QString getCurrentLandscapeName() const;
	//! Change the current landscape to the landscape with the name specified.
	//! Emits currentLandscapeChanged() if the landscape changed (true returned)
	//! @param name the name of the new landscape, as found in the landscape:name key of the landscape.ini file.
	//! @param changeLocationDuration the duration of the transition animation
	bool setCurrentLandscapeName(const QString& name, const double changeLocationDuration = 1.0);

	//! Get the current landscape or lightscape brightness (0..1)
	//! @param light true to retrieve the light layer brightness value.
	float getCurrentLandscapeBrightness(const bool light=false) const {return static_cast<float>(light? landscape->getLightscapeBrightness() : landscape->getBrightness());}

	//! Preload a landscape into cache.
	//! @param id the ID of a landscape
	//! @param replace true if existing landscape entry should be replaced (useful during development to reload after edit)
	//! @return false if landscape could not be found, or if it already existed in cache and replace was false.
	bool precacheLandscape(const QString& id, const bool replace=true);
	//! Remove a landscape from the cache of landscapes.
	//! @param id the ID of a landscape
	//! @return false if landscape could not be found
	bool removeCachedLandscape(const QString& id);
	//! Set size of the landscape cache, in MB.
	//! Default size is 100MB, or configured as [landscape/cache_size_mb] from config.ini.
	//! The landscape sizes returned in Landscape::getMemorySize() are only approximate, but include image and texture sizes.
	//! A big landscape may well take 150MB or more.
	//! On a 32bit system, keep this rather small. On 64bit with 16GB RAM and no other tasks, 4GB is no problem.
	//! Modern GPUs may have 4 or even 8GB of dedicated texture memory. Most of this may be filled with landscape textures.
	//! Example: a museum installation with 20 large (16384x2048) old_style landscapes can require up to 3.5GB. Allow 4GB cache,
	//! and the system will never have to load a landscape during the show when all have been preloaded.
	void setCacheSize(int mb) { landscapeCache.setMaxCost(mb);}
	//! Retrieve total size of cache (MB).
	int getCacheSize() const {return landscapeCache.maxCost();}
	//! Retrieve sum of currently used memory in cache (MB, approximate)
	int getCacheFilledSize() const {return landscapeCache.totalCost();}
	//! Return number of landscapes already in the cache.
	int getCacheCount() const {return landscapeCache.count();}

	//! Get the current landscape object.
	Landscape* getCurrentLandscape() const { return landscape; }

	//! Get the default landscape ID.
	const QString getDefaultLandscapeID() const {return defaultLandscapeID;}
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
	double getLandscapeSinMinAltitudeLimit() const;
	
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

	//! Retrieve flag for rendering polygonal line (if one is defined)
	bool getFlagPolyLineDisplayed() const {return flagPolyLineDisplayedOnly;}
	//! Set flag for rendering polygonal line (if one is defined)
	void setFlagPolyLineDisplayed(bool b) {if(b!=flagPolyLineDisplayedOnly){ flagPolyLineDisplayedOnly=b; emit flagPolyLineDisplayedChanged(b);}}
	//! Retrieve thickness for rendering polygonal line (if one is defined)
	int getPolyLineThickness() const {return polyLineThickness;}
	//! Set thickness for rendering polygonal line (if one is defined)
	void setPolyLineThickness(int thickness) {polyLineThickness=thickness; emit polyLineThicknessChanged(thickness);}

	//! Return the value of the flag determining if a change of landscape will update the observer location.
	bool getFlagLandscapeSetsLocation() const {return flagLandscapeSetsLocation;}
	//! Set the value of the flag determining if a change of landscape will update the observer location.
	void setFlagLandscapeSetsLocation(bool b) {if(b!=flagLandscapeSetsLocation){ flagLandscapeSetsLocation=b; emit flagLandscapeSetsLocationChanged(b);}}

	//! Return the value of the flag determining if a minimal brightness should be used to keep landscape visible.
	bool getFlagLandscapeUseMinimalBrightness() const {return flagLandscapeUseMinimalBrightness; }
	//! Set the value of the flag determining if a minimal brightness should be used to keep landscape visible.
	void setFlagLandscapeUseMinimalBrightness(bool b) {if(b!=flagLandscapeUseMinimalBrightness){ flagLandscapeUseMinimalBrightness=b; emit flagLandscapeUseMinimalBrightnessChanged(b);}}
	//! Return the value of the flag determining if the minimal brightness should be taken from landscape.ini
	bool getFlagLandscapeSetsMinimalBrightness() const {return flagLandscapeSetsMinimalBrightness;}
	//! Sets the value of the flag determining if the minimal brightness should be taken from landscape.ini
	void setFlagLandscapeSetsMinimalBrightness(bool b) {if(b!=flagLandscapeSetsMinimalBrightness){ flagLandscapeSetsMinimalBrightness=b; emit flagLandscapeSetsMinimalBrightnessChanged(b);}}
	//! Return the minimal brightness value of the landscape
	double getDefaultMinimalBrightness() const {return defaultMinimalBrightness;}
	//! Set the minimal brightness value of the landscape.
	void setDefaultMinimalBrightness(const double b) {if(fabs(b-defaultMinimalBrightness)>0.0){ defaultMinimalBrightness=b; emit defaultMinimalBrightnessChanged(b);}}
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

	//! Get current display intensity of atmosphere ([0..1], for smoother transitions)
	float getAtmosphereFadeIntensity() const;

	//! Get atmosphere fade duration in s.
	float getAtmosphereFadeDuration() const;
	//! Set atmosphere fade duration in s.
	void setAtmosphereFadeDuration(const float f);

	/*
	//This method has been removed, use StelSkyDrawer::getBortleScaleIndex instead, or StelMainScriptAPI::getBortleScaleIndex in scripts
	//Also, if required, please use StelSkyDrawer::setBortleScaleIndex or StelMainScriptAPI::setBortleScaleIndex instead of LandscapeMgr::setAtmosphereBortleLightPollution
	int getAtmosphereBortleLightPollution() const;
	*/

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

	//! Get flag for auto-enable of atmospheres and landscapes for planets.
	bool getFlagEnvironmentAutoEnable() const;
	//! Set flag for auto-enable atmosphere and landscape for planets with atmospheres in location window
	void setFlagEnvironmentAutoEnable(bool b);

	//! Forward opacity query to current landscape.
	//! @param azalt direction of view line to sample in azaltimuth coordinates.
	float getLandscapeOpacity(Vec3d azalt) const {return landscape->getOpacity(azalt);}
	// This variant is required for scripting!
	float getLandscapeOpacity(Vec3f azalt) const {return landscape->getOpacity(azalt.toVec3d());}
	//! Forward opacity query to current landscape.
	//! @param azimuth in degrees
	//! @param altitude in degrees
	float getLandscapeOpacity(float azimuth, float altitude) const {
		Vec3d azalt;
		StelUtils::spheToRect((180.0f-azimuth)*M_PI_180f, altitude*M_PI_180f, azalt);
		return landscape->getOpacity(azalt);
	}

signals:
	void atmosphereDisplayedChanged(const bool displayed);
	void cardinalsPointsDisplayedChanged(const bool displayed);
	void cardinalsPointsColorChanged(const Vec3f & newColor) const;
	void fogDisplayedChanged(const bool displayed);
	void landscapeDisplayedChanged(const bool displayed);
	void illuminationDisplayedChanged(const bool displayed);
	void labelsDisplayedChanged(const bool displayed);
	void flagPolyLineDisplayedChanged(const bool enabled);
	void polyLineThicknessChanged(const int thickness);
	void flagUseLightPollutionFromDatabaseChanged(const bool usage);
	void flagLandscapeAutoSelectionChanged(const bool value);
	void flagLandscapeSetsLocationChanged(const bool value);
	void flagLandscapeUseMinimalBrightnessChanged(const bool value);
	void flagLandscapeSetsMinimalBrightnessChanged(const bool value);
	void defaultMinimalBrightnessChanged(const double value);
	void setFlagEnvironmentAutoEnableChanged(const bool enabled);

	//! Emitted whenever the default landscape is changed
	//! @param id the landscape id of the new default landscape
	void defaultLandscapeChanged(const QString& id);

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

	//! Emitted when the current landscape was changed
	//! \param currentLandscapeID the ID of the new landscape
	//! \param currentLandscapeName the name of the new landscape
	void currentLandscapeChanged(QString currentLandscapeID,QString currentLandscapeName);

private slots:
	//! Set the light pollution following the Bortle Scale.
	//! This should not be called from script code, use StelMainScriptAPI::setBortleScaleIndex if you want to change the light pollution.
	void setAtmosphereBortleLightPollution(const int bIndex);

	//! Reacts to StelCore::locationChanged.
	void onLocationChanged(const StelLocation &loc);
	void onTargetLocationChanged(const StelLocation &loc);

	//! Translate labels to new language settings.
	void updateI18n();

	void increaseLightPollution();
	void reduceLightPollution();
	void cyclicChangeLightPollution();

private:
	//! Get light pollution luminance level.
	float getAtmosphereLightPollutionLuminance() const;
	//! Set light pollution luminance level.
	void setAtmosphereLightPollutionLuminance(const float f);


	//! For a given landscape name, return the landscape ID.
	//! This takes a name of the landscape, as described in the landscape:name item in the
	//! landscape.ini, and returns the landscape ID which corresponds to that name.
	static QString nameToID(const QString& name);

	//! Returns the path to an installed landscape's directory.
	//! It uses StelFileMgr to look for it in the possible directories.
	//! @param landscapeID an installed landscape's identifier (the folder name)
	//! @returns an empty string, if no such landscape was found.
	static QString getLandscapePath(const QString landscapeID);

	Atmosphere* atmosphere;			// Atmosphere
	Cardinals* cardinalsPoints;		// Cardinals points
	Landscape* landscape;			// The landscape i.e. the fog, the ground and "decor"
	Landscape* oldLandscape;		// Used only during transitions to newly loaded landscape.

	// Define whether the observer location is to be updated when the landscape is updated.
	bool flagLandscapeSetsLocation;

	bool flagLandscapeAutoSelection;

	bool flagLightPollutionFromDatabase;

	//! control drawing of a Polygonal line, if one is defined.
	bool flagPolyLineDisplayedOnly;
	//! thickness of polygonal horizon line
	int polyLineThickness;

	//! Indicate use of the default minimal brightness value specified in config.ini.
	bool flagLandscapeUseMinimalBrightness;
	//! A minimal brightness value to keep landscape visible.
	double defaultMinimalBrightness;
	//! Indicate use of the minimal brightness value specified in the current landscape.ini, if present.
	bool flagLandscapeSetsMinimalBrightness;
	//! Indicate auto-enable atmosphere and landscape for planets with atmospheres in location window
	bool flagEnvironmentAutoEnabling;

	//! The ID of the currently loaded landscape
	QString currentLandscapeID;

	//! The ID of the default landscape
	QString defaultLandscapeID;

	//! List of the IDs of the landscapes packaged by default with Stellarium.
	//! (So that they can't be removed.)
	QStringList packagedLandscapeIDs;

	//! QCache of landscapes kept in memory for faster access, esp. when frequently switching between several big landscapes.
	//! Example: a 16384-size old_style landscape takes about 10 seconds to load. Kept in cache, it is back instantly.
	//! Of course, this requires lots of RAM and GPU texture memory, but in the age of 64bit CPUs and 4GB and more GPU
	//! texture memory, it is no problem to keep even 20 or more landscapes.
	//! This is esp. useful in a context of automated setup (museum show or such) where a list of landscapes is preloaded
	//! at system start (e.g. in the startup.ssc script) and then retrieved while script is running.
	//! The key is just the LandscapeID.
	QCache<QString,Landscape> landscapeCache;

	//! Core current planet name, used to react to planet change.
	QString currentPlanetName;
};

#endif // LANDSCAPEMGR_HPP

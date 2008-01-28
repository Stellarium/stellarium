/*
 * Stellarium
 * Copyright (C) 2006 Fabien Chereau
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

#ifndef LANDSCAPEMGR_H_
#define LANDSCAPEMGR_H_

#include <QMap>
#include "StelModule.hpp"
#include "StelUtils.hpp"

class Landscape;
class Atmosphere;
class Cardinals;
class QSettings;

//! @class LandscapeMgr 
//! Manages all the rendering a the level of the observer's surrounding.
//! This includes landscape textures, fog, atmosphere and cardinal points
//! I decided to put all these elements together in a single class because they are 
//! inherently linked, especially when we start moving the observer in altitude.
class LandscapeMgr : public StelModule
{
	Q_OBJECT
			
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
	virtual double draw(StelCore* core);
	
	//! Update time-dependent state.
	//! Includes:
	//! - Landscape, atmosphere and cardinal point on/off fading.
	//! - Atmophere colour calulation based on location, position of sun 
	//!   and moon.
	//! - updates adaptation lumenescence lased on visible bright objects.
	virtual void update(double deltaTime);
	
	//! Translate labels to new language settings.
	virtual void updateI18n();
	
	//! Called for all registered modules if the sky culture changes.
	virtual void updateSkyCulture() {;}
	
	//! Load a color scheme from a configration object
	//! @param conf the configuration object containing the color scheme
	//! @param section of conf containing the color scheme
	virtual void setColorScheme(const QSettings* conf, const QString& section);
	
	//! Get the order in which this module will draw it's objects relative to other modules.
	virtual double getCallOrder(StelModuleActionName actionName) const;
	
	
	///////////////////////////////////////////////////////////////////////////
	// Method specific to the landscape manager
	//! Return the global landscape luminance, for being used e.g for setting eye adaptation.
	float getLuminance(void);

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
	//! Retrieve a \n separated list of the names of all the available landscape in the
	//! file search path sub-directories of the landscape area
	//! @return the names of the landscapes, which are the values of the name parameter in the landscape.ini files
	QString getAllLandscapeNames();
		
	//! Get the current landscape ID.
	const QString& getLandscapeId() {return currentLandscapeID;}
	//! Return the real name of the current landscape.
	QString getLandscapeName();
	//! Return the author name of the current landscape.
	QString getLandscapeAuthorName();
	//! Return the description of the current landscape.
	QString getLandscapeDescription();
	//! Return the translated name of the planet where the current landscape is located.
	QString getLandscapePlanetName();
	//! Return a wstring with the longitude, latitude and altitude of the current landscape.
	QString getLandscapeLocationDescription();
	
	//! Change the current landscape to the landscape with the name specified.
	//! @param newLandscapeName the name of the new landscape, as found in the 
	//! landscape:name key of the landscape.ini file.
	//! @return false if the new landscape could not be set (e.g. no landscape 
	//! of that name was found). True on success.
	bool setLandscapeByName(const QString& newLandscapeName);

	//! Change the current landscape to the landscape with the ID specified.
	//! @param newLandscapeID the ID of the new landscape
	//! @return false if the new landscape could not be set (e.g. no landscape 
	//! of that ID was found). True on success.
	bool setLandscapeByID(const QString& newLandscapeID);
	
	//! Get flag for displaying Landscape.
	bool getFlagLandscape() const;
	//! Set flag for displaying Landscape.
	void setFlagLandscape(bool b);

	//! Get flag for displaying Fog.
	bool getFlagFog() const;
	//! Set flag for displaying Fog.
	void setFlagFog(bool b);

	//! Return the value of the flag determining if a change of landscape will update the observer location.
	bool getFlagLandscapeSetsLocation() const {return flagLandscapeSetsLocation;}
	//! Set the value of the flag determining if a change of landscape will update the observer location.
	void setFlagLandscapeSetsLocation(bool b) {flagLandscapeSetsLocation=b;}

	//! Get flag for displaying Cardinals Points.
	bool getFlagCardinalsPoints() const;
	//! Set flag for displaying Cardinals Points.
	void setFlagCardinalsPoints(bool b);

	//! Get Cardinals Points color.
	Vec3f getColorCardinalPoints() const;
	//! Set Cardinals Points color.
	void setColorCardinalPoints(const Vec3f& v);
	
	//! Get flag for displaying Atmosphere.
	bool getFlagAtmosphere() const;
	//! Set flag for displaying Atmosphere.
	void setFlagAtmosphere(bool b);

	//! Get atmosphere fade duration in s.
	float getAtmosphereFadeDuration() const;
	//! Set atmosphere fade duration in s.
	void setAtmosphereFadeDuration(float f);

	//! Get light pollution luminance level.
	float getAtmosphereLightPollutionLuminance() const;
	//! Set light pollution luminance level.
	void setAtmosphereLightPollutionLuminance(float f);
	
private:
	//! For a given landscape name, return the landscape ID.
	//! This takes a name of the landscape, as described in the landscape:name item in the 
	//! landscape.ini, and returns the landscape ID which corresponds to that name.
	QString nameToID(const QString& name);
	
	//! Create landscape from parameters passed in a hash.
	//! This is similar in function to createFromFile, except the landscape details
	//! are passed in a hash rather than a ini parser object.
	//! NOTE: maptex must be full path and filename
	//! @param param an STL map of the keys and values which describe the landscape.
	//! @return a pointer to the newly create landscape object.
	Landscape* createFromHash(QMap<QString, QString>& param);
	
	//! Return a map of landscape name to landscape ID (directory name).
	QMap<QString,QString> getNameToDirMap(void);
		
	Atmosphere* atmosphere;			// Atmosphere
	Cardinals* cardinals_points;		// Cardinals points
	Landscape* landscape;			// The landscape i.e. the fog, the ground and "decor"
	
	// Define whether the observer location is to be updated when the landscape is updated.
	bool flagLandscapeSetsLocation;
	
	// The ID of the currently loaded landscape
	QString currentLandscapeID;
};

#endif /*LANDSCAPEMGR_H_*/

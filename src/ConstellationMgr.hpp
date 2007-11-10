/*
 * Stellarium
 * Copyright (C) 2002 Fabien Chereau
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

#ifndef _CONSTELLATION_MGR_H_
#define _CONSTELLATION_MGR_H_

#include <vector>

#include "StelObjectType.hpp"
#include "StelObjectModule.hpp"

class LoadingBar;
class ToneReproducer;
class StarMgr;
class Constellation;
class Projector;
class Navigator;
class SFont;

//! @class ConstellationMgr
//! Display and manage the constellations.
//! It can display constellations lines, names, art textures and boundaries.
//! It also supports several different sky cultures.
class ConstellationMgr : public StelObjectModule
{
	Q_OBJECT;
	Q_PROPERTY(bool flagArt READ getFlagArt WRITE setFlagArt);
	Q_PROPERTY(double artFadeDuration READ getArtFadeDuration WRITE setArtFadeDuration);
	Q_PROPERTY(double artIntensity READ getArtIntensity WRITE setArtIntensity);
	Q_PROPERTY(bool flagLines READ getFlagLines WRITE setFlagLines);
	Q_PROPERTY(bool flagBoundaries READ getFlagBoundaries WRITE setFlagBoundaries);
	Q_PROPERTY(bool flagNames READ getFlagNames WRITE setFlagNames);
	Q_PROPERTY(bool flagIsolateSelected READ getFlagIsolateSelected WRITE setFlagIsolateSelected);
	Q_PROPERTY(Vec3f linesColor READ getLinesColor WRITE setLinesColor);		
	Q_PROPERTY(Vec3f boundariesColor READ getBoundariesColor WRITE setBoundariesColor);
	Q_PROPERTY(Vec3f namesColor READ getNamesColor WRITE setNamesColor);
	Q_PROPERTY(double fontSize READ getFontSize WRITE setFontSize);
	
public:
	ConstellationMgr(StarMgr *stars);
	virtual ~ConstellationMgr();

	///////////////////////////////////////////////////////////////////////////
	// Methods defined in the StelModule class
	//! Initialize the ConstellationMgr.
	//! Reads from the configuration parser object and updates the loading bar
	//! as constellation objects are loaded for the required sky culture.
	//! @param conf the ini file parser object
	//! @param lb a reference to the LoadingBar object
	virtual void init(const InitParser& conf, LoadingBar& lb);
	
	//! Draw constellation lines, art, names and boundaries.
	virtual double draw(Projector *prj, const Navigator *nav, ToneReproducer *eye); 
	
	//! Updates time-varying state for each Constellation.
	virtual void update(double deltaTime);
	
	//! Update i18n names from English names according to current 
	//! locale, and update font for locale.
	//! The translation is done using gettext with translated strings defined 
	//! in translations.h
	virtual void updateI18n();
	
	//! Loads new constellation data and art if the SkyCulture has changed.
	virtual void updateSkyCulture(LoadingBar& lb);
	
	//! Limit the number of constellations to draw based on selected stars.
	//! The selected objects changed, check if some stars are selected and display the 
	//! matching constellations if isolate_selected mode is activated.
	//! @param added not used at present (?)
	virtual void selectedObjectChangeCallBack(bool added = false);
	
	//! 
	virtual void setColorScheme(const InitParser& conf, const QString& section);
	virtual double getCallOrder(StelModuleActionName actionName) const;
	
	///////////////////////////////////////////////////////////////////////////
	// Methods defined in StelObjectManager class
	virtual vector<StelObjectP> searchAround(const Vec3d& v, double limitFov, const Navigator * nav, const Projector * prj) const;
	
	//! Return the matching constellation object's pointer if exists or NULL
	//! @param nameI18n The case sensistive constellation name
	virtual StelObjectP searchByNameI18n(const wstring& nameI18n) const;
	
	//! Return the matching constellation if exists or NULL
	//! @param name The case sensistive standard program name (three letter abbreviation)
	virtual StelObjectP searchByName(const string& name) const;

	//! Find and return the list of at most maxNbItem objects auto-completing the passed object I18n name.
	//! @param objPrefix the case insensitive first letters of the searched object
	//! @param maxNbItem the maximum number of returned object names
	//! @return a vector of matching object name by order of relevance, or an empty vector if nothing match
	virtual vector<wstring> listMatchingObjectsI18n(const wstring& objPrefix, unsigned int maxNbItem=5) const;
	
	///////////////////////////////////////////////////////////////////////////
	// Properties setters and getters
public slots:
	//! Set constellation art fade duration
	void setArtFadeDuration(float duration);
	//! Get constellation art fade duration
	float getArtFadeDuration() const {return artFadeDuration;}
		
	//! Set constellation maximum art intensity
	void setArtIntensity(float f);
	//! Set constellation maximum art intensity
	float getArtIntensity() const {return artMaxIntensity;}
	
	//! Set whether constellation art will be displayed
	void setFlagArt(bool b);
	//! Get whether constellation art is displayed
	bool getFlagArt(void) const {return flagArt;}
	
	//! Set whether constellation path lines will be displayed
	void setFlagLines(bool b);
	//! Get whether constellation path lines are displayed
	bool getFlagLines(void) const {return flagLines;}
	
	//! Set whether constellation boundaries lines will be displayed
	void setFlagBoundaries(bool b);
	//! Get whether constellation boundaries lines are displayed
	bool getFlagBoundaries(void) const {return flagBoundaries;}
	
	//! Set whether constellation names will be displayed
	void setFlagNames(bool b);
	//! Set whether constellation names are displayed
	bool getFlagNames(void) const {return flagNames;}
	
	//! Set whether selected constellation must be displayed alone
	void setFlagIsolateSelected(bool s) { isolateSelected = s; }
	//! Get whether selected constellation is displayed alone
	bool getFlagIsolateSelected(void) const { return isolateSelected;}
	
	//! Define line color
	void setLinesColor(const Vec3f& c);
	//! Get line color
	Vec3f getLinesColor() const;
	
	//! Define boundary color
	void setBoundariesColor(const Vec3f& c);
	//! Get current boundary color
	Vec3f getBoundariesColor() const;
		
	//! Set label color for names
	void setNamesColor(const Vec3f& c);
	//! Get label color for names
	Vec3f getNamesColor() const;
	
	//! Set the font size used for constellation names display
	void setFontSize(double newFontSize);
	//! Get the font size used for constellation names display
	double getFontSize() const;
	
private:
	//! Read constellation names from the given file.
	//! @param namesFile Name of the file containing the constellation names in english
	void loadNames(const string& names_file);
	
	//! Load constellation line shapes, art textures and boundaries shapes from data files.
	//! @param fileName The name of the constellation data file
	//! @param artFileName The name of the constellation art data file
	//! @param cultureName A string ID of the current skyculture
	//! @param lb The loading progress bar object
	void loadLinesAndArt(const string &fileName, const string &artfileName, const string& cultureName, LoadingBar& lb);
	
	//! Load the constellation boundary file.
	//! This function deletes any currently loaded constellation boundaries
	//! and loads a new set from the file passed as the parameter.  The boundary 
	//! data file consists of whitespace separated values (space, tab or newline).  
	//! Each boundary may span multiple lines, and consists of the following ordered
	//! data items:
	//!  - The number of vertexes which make up in the boundary (integer).
	//!  - For each vertex, two floating point numbers describing the ra and dec 
	//!    of the vertex.
	//!  - The number of constellations which this boundary separates (always 2).
	//!  - Two constellation abbreviations representing the constellations which 
	//!    the boundary separates.
	//! @param conCatFile the path to the file which contains 
	//! the constellation boundary data.
	bool loadBoundaries(const string& conCatFile);
	//! Draw the constellation lines.
	void drawLines(Projector * prj) const;
	//! Draw the constellation art.
	void drawArt(Projector * prj, const Navigator * nav) const;
	//! Draw the constellation name labels.
	void drawNames(Projector * prj) const;
	//! Draw the constellation boundaries.
	void drawBoundaries(Projector* prj) const;	
	//! Handle single and multi-constellation selections.
	void setSelectedConst(Constellation* c);
	//! Define which constellation is selected from its abbreviation.
	void setSelected(const string& abbreviation);
	//! Define which constellation is selected and return brightest star.
	StelObjectP setSelectedStar(const string& abbreviation);
	//! Define which constellation is selected from a star number.
	void setSelected(const StelObject* s) {if (!s) setSelectedConst(NULL); else setSelectedConst(isStarIn(s));}
	//! Remove all selected constellations.
	void deselect() {setSelected(NULL);}
	//! Get the first selected constellation.
	//! NOTE: this function should return a list of all, or may be deleted. Please 
	//! do not use until it exhibits the proper behaviour.
	StelObject* getSelected(void) const;
	
	vector<Constellation*> selected; // More than one can be selected at a time
	
	Constellation* isStarIn(const StelObject *s) const;
	Constellation* findFromAbbreviation(const string& abbreviation) const;		
	vector<Constellation*> asterisms;
	double fontSize;
	SFont* asterFont;
	StarMgr* hipStarMgr;

	bool isolateSelected;
	vector<vector<Vec3f> *> allBoundarySegments;

	string lastLoadedSkyCulture;	// Store the last loaded sky culture directory name
	
	// These are THE master settings - individual constellation settings can vary based on selection status
	bool flagNames;
	bool flagLines;
	bool flagArt;
	bool flagBoundaries;
	float artFadeDuration;
	float artMaxIntensity;
};

#endif // _CONSTELLATION_MGR_H_

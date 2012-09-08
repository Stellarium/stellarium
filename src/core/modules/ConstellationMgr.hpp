/*
 * Stellarium
 * Copyright (C) 2002 Fabien Chereau
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

#ifndef _CONSTELLATIONMGR_HPP_
#define _CONSTELLATIONMGR_HPP_

#include <vector>
#include <QString>
#include <QStringList>
#include <QFont>

#include "StelObjectType.hpp"
#include "StelObjectModule.hpp"
#include "StelProjectorType.hpp"

class StelToneReproducer;
class StarMgr;
class Constellation;
class StelProjector;

//! @class ConstellationMgr
//! Display and manage the constellations.
//! It can display constellations lines, names, art textures and boundaries.
//! It also supports several different sky cultures.
class ConstellationMgr : public StelObjectModule
{
	Q_OBJECT
	Q_PROPERTY(bool artDisplayed
			   READ getFlagArt
			   WRITE setFlagArt
			   NOTIFY artDisplayedChanged)
	Q_PROPERTY(bool artFadeDuration
			   READ getArtFadeDuration
			   WRITE setArtFadeDuration
			   NOTIFY artFadeDurationChanged)
	Q_PROPERTY(bool artIntensity
			   READ getArtIntensity
			   WRITE setArtIntensity
			   NOTIFY artIntensityChanged)
	Q_PROPERTY(bool boundariesColor
			   READ getBoundariesColor
			   WRITE setBoundariesColor
			   NOTIFY boundariesColorChanged)
	Q_PROPERTY(bool boundariesDisplayed
			   READ getFlagBoundaries
			   WRITE setFlagBoundaries
			   NOTIFY boundariesDisplayedChanged)
	Q_PROPERTY(bool fontSize
			   READ getFontSize
			   WRITE setFontSize
			   NOTIFY fontSizeChanged)
	Q_PROPERTY(bool isolateSelected
			   READ getFlagIsolateSelected
			   WRITE setFlagIsolateSelected
			   NOTIFY isolateSelectedChanged)
	Q_PROPERTY(bool linesColor
			   READ getLinesColor
			   WRITE setLinesColor
			   NOTIFY linesColorChanged)
	Q_PROPERTY(bool linesDisplayed
			   READ getFlagLines
			   WRITE setFlagLines
			   NOTIFY linesDisplayedChanged)
	Q_PROPERTY(bool namesColor
			   READ getLabelsColor
			   WRITE setLabelsColor
			   NOTIFY namesColorChanged)
	Q_PROPERTY(bool namesDisplayed
			   READ getFlagLabels
			   WRITE setFlagLabels
			   NOTIFY namesDisplayedChanged)

public:
	//! Constructor
	ConstellationMgr(StarMgr *stars);
	//! Destructor
	virtual ~ConstellationMgr();

	///////////////////////////////////////////////////////////////////////////
	// Methods defined in the StelModule class
	//! Initialize the ConstellationMgr.
	//! Reads from the configuration parser object and updates the loading bar
	//! as constellation objects are loaded for the required sky culture.
	virtual void init();

	//! Draw constellation lines, art, names and boundaries.
	virtual void draw(StelCore* core, class StelRenderer* renderer);

	//! Updates time-varying state for each Constellation.
	virtual void update(double deltaTime);

	//! Return the value defining the order of call for the given action
	//! @param actionName the name of the action for which we want the call order
	//! @return the value defining the order. The closer to 0 the earlier the module's action will be called
	virtual double getCallOrder(StelModuleActionName actionName) const;

	///////////////////////////////////////////////////////////////////////////
	// Methods defined in StelObjectManager class
	virtual QList<StelObjectP> searchAround(const Vec3d& v, double limitFov, const StelCore* core) const;

	//! Return the matching constellation object's pointer if exists or NULL
	//! @param nameI18n The case in-sensistive constellation name
	virtual StelObjectP searchByNameI18n(const QString& nameI18n) const;

	//! Return the matching constellation if exists or NULL
	//! @param name The case in-sensistive standard program name (three letter abbreviation)
	virtual StelObjectP searchByName(const QString& name) const;

	//! Find and return the list of at most maxNbItem objects auto-completing the passed object I18n name.
	//! @param objPrefix the case insensitive first letters of the searched object
	//! @param maxNbItem the maximum number of returned object names
	//! @return a vector of matching object name by order of relevance, or an empty vector if nothing match
	virtual QStringList listMatchingObjectsI18n(const QString& objPrefix, int maxNbItem=5) const;
	virtual QStringList listAllObjects(bool inEnglish) const;
	virtual QString getName() const { return "Constellations"; }

	///////////////////////////////////////////////////////////////////////////
	// Properties setters and getters
public slots:	
	//! Set whether constellation art will be displayed
	void setFlagArt(const bool displayed);
	//! Get whether constellation art is displayed
	bool getFlagArt(void) const;

	//! Set constellation art fade duration in second
	void setArtFadeDuration(const float duration);
	//! Get constellation art fade duration in second
	float getArtFadeDuration() const;

	//! Set constellation maximum art intensity (between 0 and 1)
	void setArtIntensity(const double intensity);
	//! Set constellation maximum art intensity (between 0 and 1)
	double getArtIntensity() const;

	//! Define boundary color
	void setBoundariesColor(const Vec3f& color);
	//! Get current boundary color
	Vec3f getBoundariesColor() const;

	//! Set whether constellation boundaries lines will be displayed
	void setFlagBoundaries(const bool displayed);
	//! Get whether constellation boundaries lines are displayed
	bool getFlagBoundaries(void) const;

	//! Set whether selected constellation must be displayed alone
	void setFlagIsolateSelected(const bool isolate);
	//! Get whether selected constellation is displayed alone
	bool getFlagIsolateSelected(void) const;

	//! Define line color
	void setLinesColor(const Vec3f& color);
	//! Get line color
	Vec3f getLinesColor() const;

	//! Set whether constellation path lines will be displayed
	void setFlagLines(const bool displayed);
	//! Get whether constellation path lines are displayed
	bool getFlagLines(void) const;

	//! Set label color for names
	void setLabelsColor(const Vec3f& color);
	//! Get label color for names
	Vec3f getLabelsColor() const;

	//! Set whether constellation names will be displayed
	void setFlagLabels(bool displayed);
	//! Set whether constellation names are displayed
	bool getFlagLabels(void) const;

	//! Set the font size used for constellation names display
	void setFontSize(const float newFontSize);
	//! Get the font size used for constellation names display
	float getFontSize() const;

signals:
	void artDisplayedChanged(const bool displayed) const;
	void artFadeDurationChanged(const float duration) const;
	void artIntensityChanged(const double intensity) const;
	void boundariesColorChanged(const Vec3f & color) const;
	void boundariesDisplayedChanged(const bool displayed) const;
	void fontSizeChanged(const float newSize) const;
	void isolateSelectedChanged(const bool isolate) const;
	void linesColorChanged(const Vec3f & color) const;
	void linesDisplayedChanged(const bool displayed) const;
	void namesColorChanged(const Vec3f & color) const;
	void namesDisplayedChanged(const bool displayed) const;

private slots:
	//! Limit the number of constellations to draw based on selected stars.
	//! The selected objects changed, check if some stars are selected and display the
	//! matching constellations if isolateSelected mode is activated.
	//! @param action define whether to add to, replace, or remove from the existing selection
	void selectedObjectChange(StelModule::StelModuleSelectAction action);

	//! Load a color scheme
	void setStelStyle(const QString& section);

	//! Loads new constellation data and art if the SkyCulture has changed.
	//! @param skyCultureDir the name of the directory containing the sky culture to use.
	void updateSkyCulture(const QString& skyCultureDir);

	//! Update i18n names from English names according to current
	//! locale, and update font for locale.
	//! The translation is done using gettext with translated strings defined
	//! in translations.h
	void updateI18n();

private:
	//! Read constellation names from the given file.
	//! @param namesFile Name of the file containing the constellation names in english
	void loadNames(const QString& namesFile);

	//! Load constellation line shapes, art textures and boundaries shapes from data files.
	//! @param fileName The name of the constellation data file
	//! @param artFileName The name of the constellation art data file
	//! @param cultureName A string ID of the current skyculture
	void loadLinesAndArt(const QString& fileName, const QString& artfileName, const QString& cultureName);

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
	//! @param conCatFile the path to the file which contains the constellation boundary data.
	bool loadBoundaries(const QString& conCatFile);
        //! Draw the constellation lines at the epoch given by the StelCore.
	void drawLines(class StelRenderer* renderer, StelProjectorP projector, const StelCore* core) const;
	//! Draw the constellation art.
	//!
	//! @param renderer  Renderer to draw with.
	//! @param projector Projector to project vertices to viewport.
	void drawArt(class StelRenderer* renderer, StelProjectorP projector) const;
	//! Draw the constellation name labels.
	//! 
	//! @param renderer  Renderer to draw with.
	//! @param projector Projector to project vertices to viewport.
	//! @param font      Font to draw the names with.
	void drawNames(class StelRenderer* renderer, StelProjectorP projector, QFont& font) const;
	//! Draw the constellation boundaries.
	//! 
	//! @param renderer  Renderer to draw with.
	//! @param projector Projector to project vertices to viewport.
	void drawBoundaries(StelRenderer* renderer, StelProjectorP projector) const;
	//! Handle single and multi-constellation selections.
	void setSelectedConst(Constellation* c);
	//! Handle unselecting a single constellation.
	void unsetSelectedConst(Constellation* c);
	//! Define which constellation is selected from its abbreviation.
	void setSelected(const QString& abbreviation);
	//! Define which constellation is selected and return brightest star.
	StelObjectP setSelectedStar(const QString& abbreviation);
	//! Define which constellation is selected from a star number.
	void setSelected(const StelObject* s) {if (!s) setSelectedConst(NULL); else setSelectedConst(isStarIn(s));}
	//! Remove all selected constellations.
	void deselect() {setSelected(NULL);}
	//! Get the first selected constellation.
	//! NOTE: this function should return a list of all, or may be deleted. Please
	//! do not use until it exhibits the proper behaviour.
	StelObject* getSelected(void) const;
	//! Remove constellations from selected objects
	void deselectConstellations(void);

	std::vector<Constellation*> selected; // More than one can be selected at a time

	Constellation* isStarIn(const StelObject *s) const;
	Constellation* findFromAbbreviation(const QString& abbreviation) const;
	std::vector<Constellation*> asterisms;
	QFont asterFont;
	StarMgr* hipStarMgr;

	bool isolateSelected;
	std::vector<std::vector<Vec3f> *> allBoundarySegments;

	QString lastLoadedSkyCulture;	// Store the last loaded sky culture directory name

	// These are THE master settings - individual constellation settings can vary based on selection status
	float artFadeDuration;
	float artIntensity;
	bool artDisplayed;
	bool boundariesDisplayed;
	bool linesDisplayed;
	bool namesDisplayed;
};

#endif // _CONSTELLATIONMGR_HPP_

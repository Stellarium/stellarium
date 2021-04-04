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

#ifndef CONSTELLATIONMGR_HPP
#define CONSTELLATIONMGR_HPP

#include "StelObjectType.hpp"
#include "StelObjectModule.hpp"
#include "StelProjectorType.hpp"

#include <vector>
#include <QString>
#include <QStringList>
#include <QFont>

class StelToneReproducer;
class StarMgr;
class Constellation;
class StelProjector;
class StelPainter;

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
	Q_PROPERTY(float artFadeDuration
		   READ getArtFadeDuration
		   WRITE setArtFadeDuration
		   NOTIFY artFadeDurationChanged)
	Q_PROPERTY(float artIntensity
		   READ getArtIntensity
		   WRITE setArtIntensity
		   NOTIFY artIntensityChanged)
	Q_PROPERTY(Vec3f boundariesColor
		   READ getBoundariesColor
		   WRITE setBoundariesColor
		   NOTIFY boundariesColorChanged)
	Q_PROPERTY(bool boundariesDisplayed
		   READ getFlagBoundaries
		   WRITE setFlagBoundaries
		   NOTIFY boundariesDisplayedChanged)
	Q_PROPERTY(float fontSize
		   READ getFontSize
		   WRITE setFontSize
		   NOTIFY fontSizeChanged)
	Q_PROPERTY(bool isolateSelected
		   READ getFlagIsolateSelected
		   WRITE setFlagIsolateSelected
		   NOTIFY isolateSelectedChanged)
	Q_PROPERTY(Vec3f linesColor
		   READ getLinesColor
		   WRITE setLinesColor
		   NOTIFY linesColorChanged)
	Q_PROPERTY(bool linesDisplayed
		   READ getFlagLines
		   WRITE setFlagLines
		   NOTIFY linesDisplayedChanged)
	Q_PROPERTY(Vec3f namesColor
		   READ getLabelsColor
		   WRITE setLabelsColor
		   NOTIFY namesColorChanged)
	Q_PROPERTY(bool namesDisplayed
		   READ getFlagLabels
		   WRITE setFlagLabels
		   NOTIFY namesDisplayedChanged)
	Q_PROPERTY(ConstellationDisplayStyle constellationDisplayStyle
		   READ getConstellationDisplayStyle
		   WRITE setConstellationDisplayStyle
		   NOTIFY constellationsDisplayStyleChanged)
	Q_PROPERTY(int constellationLineThickness
		   READ getConstellationLineThickness
		   WRITE setConstellationLineThickness
		   NOTIFY constellationLineThicknessChanged)
	Q_PROPERTY(int constellationBoundariesThickness
		   READ getConstellationBoundariesThickness
		   WRITE setConstellationBoundariesThickness
		   NOTIFY constellationBoundariesThicknessChanged)
	Q_ENUMS(ConstellationDisplayStyle)

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
	virtual void draw(StelCore* core);

	//! Updates time-varying state for each Constellation.
	virtual void update(double deltaTime);

	//! Return the value defining the order of call for the given action
	//! @param actionName the name of the action for which we want the call order
	//! @return the value defining the order. The closer to 0 the earlier the module's action will be called
	virtual double getCallOrder(StelModuleActionName actionName) const;

	///////////////////////////////////////////////////////////////////////////
	// Methods defined in StelObjectModule class
	virtual QList<StelObjectP> searchAround(const Vec3d& v, double limitFov, const StelCore* core) const;

	//! @return the matching constellation object's pointer if exists or Q_NULLPTR
	//! @param nameI18n The case in-sensitive constellation name
	virtual StelObjectP searchByNameI18n(const QString& nameI18n) const;

	//! @return the matching constellation if exists or Q_NULLPTR
	//! @param name The case in-sensitive standard program name (three letter abbreviation)
	virtual StelObjectP searchByName(const QString& name) const;

	virtual StelObjectP searchByID(const QString &id) const;

	virtual QStringList listAllObjects(bool inEnglish) const;
	virtual QString getName() const { return "Constellations"; }
	virtual QString getStelObjectType() const;
	//! Describes how to display constellation labels. The viewDialog GUI has a combobox which corresponds to these values.
	enum ConstellationDisplayStyle
	{
		constellationsAbbreviated	= 0,
		constellationsNative		= 1,
		constellationsTranslated	= 2,
		constellationsEnglish		= 3 // Maybe this is not useful?
	};	

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
	//! Note that the art intensity is linearly faded out if the FOV is in a specific interval,
	//! which can be adjusted using setArtIntensityMinimumFov() and setArtIntensityMaximumFov()
	void setArtIntensity(const float intensity);
	//! Return constellation maximum art intensity (between 0 and 1)
	//! Note that the art intensity is linearly faded out if the FOV is in a specific interval,
	//! which can be adjusted using setArtIntensityMinimumFov() and setArtIntensityMaximumFov()
	float getArtIntensity() const;

	//! Sets the lower bound on the FOV at which the art intensity fades to zero.
	//!  See LP:#1294483. The default is 1.0.
	void setArtIntensityMinimumFov(const double fov);
	//! Returns the lower bound on the FOV at which the art intensity fades to zero.
	//! See LP:#1294483. The default is 1.0.
	double getArtIntensityMinimumFov() const;

	//! Sets the upper bound on the FOV at which the art intensity becomes the maximum
	//! set by setArtIntensity()
	//!  See LP:#1294483. The default is 2.0.
	void setArtIntensityMaximumFov(const double fov);
	//! Returns the upper bound on the FOV at which the art intensity becomes the maximum
	//! set by setArtIntensity()
	//!  See LP:#1294483. The default is 2.0.
	double getArtIntensityMaximumFov() const;

	//! Define boundary color
	//! @param color The color of boundaries
	//! @code
	//! // example of usage in scripts
	//! ConstellationMgr.setBoundariesColor(Vec3f(1.0,0.0,0.0));
	//! @endcode
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

	//! Set whether only one selected constellation must be displayed
	void setFlagConstellationPick(const bool mode);
	//! Get whether only one selected constellation is displayed
	bool getFlagConstellationPick(void) const;

	//! Define line color
	//! @param color The color of lines
	//! @code
	//! // example of usage in scripts
	//! ConstellationMgr.setLinesColor(Vec3f(1.0,0.0,0.0));
	//! @endcode
	void setLinesColor(const Vec3f& color);
	//! Get line color
	Vec3f getLinesColor() const;

	//! Set whether constellation lines will be displayed
	void setFlagLines(const bool displayed);
	//! Get whether constellation lines are displayed
	bool getFlagLines(void) const;

	//! Set label color for names
	//! @param color The color of labels
	//! @code
	//! // example of usage in scripts
	//! ConstellationMgr.setLabelsColor(Vec3f(1.0,0.0,0.0));
	//! @endcode
	void setLabelsColor(const Vec3f& color);
	//! Get label color for names
	Vec3f getLabelsColor() const;

	//! Set whether constellation names will be displayed
	void setFlagLabels(const bool displayed);
	//! Set whether constellation names are displayed
	bool getFlagLabels(void) const;

	//! Set the font size used for constellation names display
	void setFontSize(const float newFontSize);
	//! Get the font size used for constellation names display
	float getFontSize() const;

	//! Set the way how constellation names are displayed: abbbreviated/as-given/translated
	//! @param style the new display style
	void setConstellationDisplayStyle(ConstellationMgr::ConstellationDisplayStyle style);
	//! get the way how constellation names are displayed: abbbreviated/as-given/translated
	ConstellationMgr::ConstellationDisplayStyle getConstellationDisplayStyle();
	//! Returns the currently set constellation display style as string, instead of enum
	//! @see getConstellationDisplayStyle()
	static QString getConstellationDisplayStyleString(ConstellationMgr::ConstellationDisplayStyle style);

	//! Set the thickness of lines of the constellations
	//! @param thickness of line in pixels
	void setConstellationLineThickness(const int thickness);
	//! Get the thickness of lines of the constellations
	int getConstellationLineThickness() const { return constellationLineThickness; }

	//! Set the thickness of boundaries of the constellations
	//! @param thickness of line in pixels
	void setConstellationBoundariesThickness(const int thickness);
	//! Get the thickness of boundaries of the constellations
	int getConstellationBoundariesThickness() const { return constellationBoundariesThickness; }

	//! Remove constellations from selected objects
	void deselectConstellations(void);

	//! Select all constellations
	void selectAllConstellations(void);

	//! Get the list of English names of all constellations for loaded sky culture
	QStringList getConstellationsEnglishNames();

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
	void constellationsDisplayStyleChanged(const ConstellationMgr::ConstellationDisplayStyle style) const;
	void constellationLineThicknessChanged(int thickness) const;
	void constellationBoundariesThicknessChanged(int thickness) const;

private slots:
	//! Limit the number of constellations to draw based on selected stars.
	//! The selected objects changed, check if some stars are selected and display the
	//! matching constellations if isolateSelected mode is activated.
	//! @param action define whether to add to, replace, or remove from the existing selection
	void selectedObjectChange(StelModule::StelModuleSelectAction action);

	//! Loads new constellation data and art if the SkyCulture has changed.
	//! @param skyCultureDir the name of the directory containing the sky culture to use.
	void updateSkyCulture(const QString& skyCultureDir);

	//! Update i18n names from English names according to current
	//! locale, and update font for locale.
	//! The translation is done using gettext with translated strings defined
	//! in translations.h
	void updateI18n();

	void reloadSkyCulture(void);

	void setFlagCheckLoadingData(const bool flag) { checkLoadingData = flag; }
	bool getFlagCheckLoadingData(void) const { return checkLoadingData; }

private:
	//! Read constellation names from the given file.
	//! @param namesFile Name of the file containing the constellation names
	//!        in a format consisting of abbreviation, native name and translatable english name.
	//! @note The abbreviation must occur in the lines file loaded first in @name loadLinesAndArt()!
	void loadNames(const QString& namesFile);

	//! Load constellation line shapes, art textures and boundaries shapes from data files.
	//! @param fileName The name of the constellation data file
	//! @param artFileName The name of the constellation art data file
	//! @param cultureName A string ID of the current skyculture
	//! @note The abbreviation used in @param filename is required for cross-identifying translatable names in @name loadNames():
	void loadLinesAndArt(const QString& fileName, const QString& artfileName, const QString& cultureName);

	//! Load the constellation boundary file.
	//! This function deletes any currently loaded constellation boundaries
	//! and loads a new set from the file passed as the parameter.  The boundary
	//! data file consists of whitespace separated values (space, tab or newline).
	//! Each boundary may span multiple lines, and consists of the following ordered
	//! data items:
	//!  - The number of vertices which make up in the boundary (integer).
	//!  - For each vertex, two floating point numbers describing the ra and dec
	//!    of the vertex.
	//!  - The number of constellations which this boundary separates (always 2).
	//!  - Two constellation abbreviations representing the constellations which
	//!    the boundary separates.
	//! @param conCatFile the path to the file which contains the constellation boundary data.
	bool loadBoundaries(const QString& conCatFile);

	//! Read seasonal rules for displaying constellations from the given file.
	//! @param rulesFile Name of the file containing the seasonal rules
	void loadSeasonalRules(const QString& rulesFile);

	//! Draw the constellation lines at the epoch given by the StelCore.
	void drawLines(StelPainter& sPainter, const StelCore* core) const;
	//! Draw the constellation art.
	void drawArt(StelPainter& sPainter) const;
	//! Draw the constellation name labels.
	void drawNames(StelPainter& sPainter) const;
	//! Draw the constellation boundaries.
	void drawBoundaries(StelPainter& sPainter) const;
	//! Handle single and multi-constellation selections.
	void setSelectedConst(Constellation* c);
	//! Handle unselecting a single constellation.
	void unsetSelectedConst(Constellation* c);
	//! Define which constellation is selected from its abbreviation.
	void setSelected(const QString& abbreviation);
	//! Define which constellation is selected and return brightest star.
	StelObjectP setSelectedStar(const QString& abbreviation);
	//! Define which constellation is selected from a star number.
	void setSelected(const StelObject* s);
	//! Remove all selected constellations.
	void deselect() { setSelected(Q_NULLPTR); }
	//! Get the first selected constellation.
	//! NOTE: this function should return a list of all, or may be deleted. Please
	//! do not use until it exhibits the proper behaviour.
	StelObject* getSelected(void) const;

	std::vector<Constellation*> selected; // More than one can be selected at a time

	Constellation* isStarIn(const StelObject *s) const;
	Constellation* isObjectIn(const StelObject *s) const;
	Constellation* findFromAbbreviation(const QString& abbreviation) const;
	std::vector<Constellation*> constellations;
	QFont asterFont;
	StarMgr* hipStarMgr;

	bool isolateSelected; // true to pick individual constellations.
	bool constellationPickEnabled;
	std::vector<std::vector<Vec3d> *> allBoundarySegments;

	QString lastLoadedSkyCulture;	// Store the last loaded sky culture directory name

	QStringList constellationsEnglishNames;

	//! this controls how constellations (and also star names) are printed: Abbreviated/as-given/translated
	ConstellationDisplayStyle constellationDisplayStyle;

	// These are THE master settings - individual constellation settings can vary based on selection status
	float artFadeDuration;
	float artIntensity;
	double artIntensityMinimumFov;
	double artIntensityMaximumFov;
	bool artDisplayed;
	bool boundariesDisplayed;
	bool linesDisplayed;
	bool namesDisplayed;
	bool checkLoadingData;

	// Store the thickness of lines of the constellations
	int constellationLineThickness;

	// Store the thickness of boundaries of the constellations
	int constellationBoundariesThickness;
};

#endif // CONSTELLATIONMGR_HPP

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

#include <vector>
#include <QString>
#include <QStringList>
#include <QFont>

class StelToneReproducer;
class StarMgr;
class Constellation;
class StelProjector;
class StelPainter;
class StelSkyCulture;

//! @class ConstellationMgr
//! Display and manage the constellations.
//! It can display constellations lines, names, art textures and boundaries.
//! It also supports several different sky cultures.
class ConstellationMgr : public StelObjectModule
{
	Q_OBJECT
	Q_PROPERTY(bool artDisplayed					READ getFlagArt							WRITE setFlagArt							NOTIFY artDisplayedChanged)
	Q_PROPERTY(float artFadeDuration				READ getArtFadeDuration					WRITE setArtFadeDuration					NOTIFY artFadeDurationChanged)
	Q_PROPERTY(float artIntensity					READ getArtIntensity						WRITE setArtIntensity						NOTIFY artIntensityChanged)
	Q_PROPERTY(Vec3f boundariesColor				READ getBoundariesColor					WRITE setBoundariesColor					NOTIFY boundariesColorChanged)
	Q_PROPERTY(bool boundariesDisplayed			READ getFlagBoundaries					WRITE setFlagBoundaries					NOTIFY boundariesDisplayedChanged)
	Q_PROPERTY(float boundariesFadeDuration		READ getBoundariesFadeDuration			WRITE setBoundariesFadeDuration			NOTIFY boundariesFadeDurationChanged)
	Q_PROPERTY(int fontSize						READ getFontSize							WRITE setFontSize							NOTIFY fontSizeChanged)
	Q_PROPERTY(bool isolateSelected				READ getFlagIsolateSelected				WRITE setFlagIsolateSelected				NOTIFY isolateSelectedChanged)
	Q_PROPERTY(bool flagConstellationPick			READ getFlagConstellationPick				WRITE setFlagConstellationPick				NOTIFY flagConstellationPickChanged)
	Q_PROPERTY(Vec3f linesColor					READ getLinesColor						WRITE setLinesColor						NOTIFY linesColorChanged)
	Q_PROPERTY(bool linesDisplayed				READ getFlagLines						WRITE setFlagLines						NOTIFY linesDisplayedChanged)
	Q_PROPERTY(float linesFadeDuration				READ getLinesFadeDuration					WRITE setLinesFadeDuration				NOTIFY linesFadeDurationChanged)
	Q_PROPERTY(Vec3f namesColor					READ getLabelsColor						WRITE setLabelsColor						NOTIFY namesColorChanged)
	Q_PROPERTY(bool namesDisplayed				READ getFlagLabels						WRITE setFlagLabels						NOTIFY namesDisplayedChanged)
	Q_PROPERTY(float namesFadeDuration			READ getLabelsFadeDuration				WRITE setLabelsFadeDuration				NOTIFY namesFadeDurationChanged)
	Q_PROPERTY(ConstellationDisplayStyle constellationDisplayStyle	READ getConstellationDisplayStyle	WRITE setConstellationDisplayStyle		NOTIFY constellationsDisplayStyleChanged)
	Q_PROPERTY(int constellationLineThickness		READ getConstellationLineThickness			WRITE setConstellationLineThickness			NOTIFY constellationLineThicknessChanged)
	Q_PROPERTY(int constellationBoundariesThickness	READ getConstellationBoundariesThickness	WRITE setConstellationBoundariesThickness	NOTIFY constellationBoundariesThicknessChanged)

public:
	//! Constructor
	ConstellationMgr(StarMgr *stars);
	//! Destructor
	~ConstellationMgr() override;

	///////////////////////////////////////////////////////////////////////////
	// Methods defined in the StelModule class
	//! Initialize the ConstellationMgr.
	//! Reads from the configuration parser object and updates the loading bar
	//! as constellation objects are loaded for the required sky culture.
	void init() override;

	//! Draw constellation lines, art, names and boundaries.
	void draw(StelCore* core) override;

	//! Updates time-varying state for each Constellation.
	void update(double deltaTime) override;

	//! Return the value defining the order of call for the given action
	//! @param actionName the name of the action for which we want the call order
	//! @return the value defining the order. The closer to 0 the earlier the module's action will be called
	double getCallOrder(StelModuleActionName actionName) const override;

	///////////////////////////////////////////////////////////////////////////
	// Methods defined in StelObjectModule class
	QList<StelObjectP> searchAround(const Vec3d& v, double limitFov, const StelCore* core) const override;

	//! @return the matching constellation object's pointer if exists or Q_NULLPTR
	//! @param nameI18n The case in-sensitive constellation name
	StelObjectP searchByNameI18n(const QString& nameI18n) const override;

	//! @return the matching constellation if exists or Q_NULLPTR
	//! @param name The case in-sensitive standard program name (three letter abbreviation)
	StelObjectP searchByName(const QString& name) const override;

	StelObjectP searchByID(const QString &id) const override;

	QStringList listAllObjects(bool inEnglish) const override;
	QString getName() const override { return "Constellations"; }
	QString getStelObjectType() const override;
	//! Describes how to display constellation labels. The viewDialog GUI has a combobox which corresponds to these values.
	//! TODO: Move to become SkycultureMgr::DisplayStyle? Then apply separately to Constellations and Planets, and whether applied to screen labels or infoString.
	enum ConstellationDisplayStyle
	{
		Abbreviated	= 0, // short label
		Native		= 1, // may show non-Latin glyphs
		Translated	= 2, // user language
		English		= 3, // Useful in case of adding names in modern English terminology (planets etc.). Maybe "Modern" would be better, and should show object scientific name in modern terminology, translated.
		Translit	= 4, // user-language transliteration/pronunciation aid
		Native_Translit,             // combinations: just help reading foreign glyphs. MORE OPTIONS POSSIBLE!
		Native_Translit_Translated,  // help reading foreign glyphs, show translations
		Native_Translit_IPA_Translated, // help reading foreign glyphs, phonetics, show translations
		Native_Translated,           // glyphs + user language
		Translit_Translated,         // user language letters + translation
		Translit_IPA_Translated,     // user language letters, phonetic + translation
		Translit_Translated_English, // user language letters + translation + English Name
		Translit_IPA_Translated_English, // user language letters + translation + English Name

	};	
	Q_ENUM(ConstellationDisplayStyle)

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
	//! // example of usage in scripts (Qt6-based Stellarium)
	//! var c = new Color(1.0, 0.0, 0.0);
	//! ConstellationMgr.setBoundariesColor(c.toVec3f());
	//! @endcode
	void setBoundariesColor(const Vec3f& color);
	//! Get current boundary color
	Vec3f getBoundariesColor() const;

	//! Set whether constellation boundaries lines will be displayed
	void setFlagBoundaries(const bool displayed);
	//! Get whether constellation boundaries lines are displayed
	bool getFlagBoundaries(void) const;

	//! Set constellation boundaries fade duration in second
	void setBoundariesFadeDuration(const float duration);
	//! Get constellation boundaries fade duration in second
	float getBoundariesFadeDuration() const;

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
	//! // example of usage in scripts (Qt6-based Stellarium)
	//! var c = new Color(1.0, 0.0, 0.0);
	//! ConstellationMgr.setLinesColor(c.toVec3f());
	//! @endcode
	void setLinesColor(const Vec3f& color);
	//! Get line color
	Vec3f getLinesColor() const;

	//! Set whether constellation lines will be displayed
	void setFlagLines(const bool displayed);
	//! Get whether constellation lines are displayed
	bool getFlagLines(void) const;

	//! Set constellation lines fade duration in second
	void setLinesFadeDuration(const float duration);
	//! Get constellation lines fade duration in second
	float getLinesFadeDuration() const;

	//! Set label color for names
	//! @param color The color of labels
	//! @code
	//! // example of usage in scripts (Qt6-based Stellarium)
	//! var c = new Color(1.0, 0.0, 0.0);
	//! ConstellationMgr.setLabelsColor(c.toVec3f());
	//! @endcode
	void setLabelsColor(const Vec3f& color);
	//! Get label color for names
	Vec3f getLabelsColor() const;

	//! Set whether constellation names will be displayed
	void setFlagLabels(const bool displayed);
	//! Set whether constellation names are displayed
	bool getFlagLabels(void) const;

	//! Set constellation labels fade duration in second
	void setLabelsFadeDuration(const float duration);
	//! Get constellation labels fade duration in second
	float getLabelsFadeDuration() const;

	//! Set the font size used for constellation names display
	void setFontSize(const int newFontSize);
	//! Get the font size used for constellation names display
	int getFontSize() const;

	//! Set the way how constellation names are displayed: abbreviated/as-given/translated
	//! @param style the new display style
	void setConstellationDisplayStyle(ConstellationMgr::ConstellationDisplayStyle style);
	//! get the way how constellation names are displayed: abbreviated/as-given/translated
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

	//! Select the constellation by his English name. Calling this method will enable
	//! isolated selection for the constellations if it is not enabled yet.
	//! @param englishName the English name of the constellation
	//! @code
	//! // example of usage in scripts: select the Orion constellation
	//! ConstellationMgr.selectConstellation("Orion");
	//! @endcode
	void selectConstellation(const QString& englishName);
	//! Select the constellation where celestial body with English name is located.
	//! Calling this method will enable isolated selection for the constellations if it is
	//! not enabled yet.
	//! @param englishName the English name of the celestial body
	//! @code
	//! // example of usage in scripts: select constellation where Venus is located
	//! ConstellationMgr.selectConstellationByObjectName("Venus");
	//! @endcode
	//! @note the method will be correct work for sky cultures with boundaries
	//! otherwise you may use star names from constellation lines as celestial body
	void selectConstellationByObjectName(const QString& englishName);
	//! Remove the constellation from list of selected constellations by his English
	//! name. Calling this method will enable isolated selection for the constellations
	//! if it is not enabled yet.
	//! @param englishName the English name of the constellation
	//! @code
	//! // example of usage in scripts: remove selection from the Orion constellation
	//! ConstellationMgr.deselectConstellation("Orion");
	//! @endcode
	//! @note all constellations will be hidden when list of selected constellations will be empty
	void deselectConstellation(const QString& englishName);

	//! Get the list of English names of all constellations for loaded sky culture
	QStringList getConstellationsEnglishNames();

signals:
	void artDisplayedChanged(const bool displayed);
	void artFadeDurationChanged(const float duration);
	void artIntensityChanged(const double intensity);
	void boundariesColorChanged(const Vec3f & color);
	void boundariesDisplayedChanged(const bool displayed);
	void boundariesFadeDurationChanged(const float duration);
	void fontSizeChanged(const int newSize);
	void isolateSelectedChanged(const bool isolate);
	void flagConstellationPickChanged(const bool mode);
	void linesColorChanged(const Vec3f & color);
	void linesDisplayedChanged(const bool displayed);
	void linesFadeDurationChanged(const float duration);
	void namesColorChanged(const Vec3f & color);
	void namesDisplayedChanged(const bool displayed);
	void namesFadeDurationChanged(const float duration);
	void constellationsDisplayStyleChanged(const ConstellationMgr::ConstellationDisplayStyle style);
	void constellationLineThicknessChanged(int thickness);
	void constellationBoundariesThicknessChanged(int thickness);

private slots:
	//! Limit the number of constellations to draw based on selected stars.
	//! The selected objects changed, check if some stars are selected and display the
	//! matching constellations if isolateSelected mode is activated.
	//! @param action define whether to add to, replace, or remove from the existing selection
	void selectedObjectChange(StelModule::StelModuleSelectAction action);

	//! Loads new constellation data and art if the SkyCulture has changed.
	//! @param skyCultureDir the name of the directory containing the sky culture to use.
	void updateSkyCulture(const StelSkyCulture& skyCulture);

	//! Update i18n names from English names according to current
	//! locale, and update font for locale.
	//! The translation is done using gettext with translated strings defined
	//! in translations.h
	void updateI18n();

	void reloadSkyCulture(void);

private:
	void setFlagCheckLoadingData(const bool flag) { checkLoadingData = flag; }
	bool getFlagCheckLoadingData(void) const { return checkLoadingData; }

	//! Load constellation line shapes, art textures and boundaries shapes from data files.
	//! @param constellationsData The structure describing all the constellations
	void loadLinesNamesAndArt(const QJsonArray& constellationsData, const StelSkyCulture& culture, bool preferNativeNames);

	//! Load the constellation boundary file.
	//! This function deletes any currently loaded constellation boundaries
	//! and loads a new set from the data passed as the parameter.
	//! @param epoch: Specified as JSON key "edges_epoch".
	//! Can be "Bxxxx.x" (Besselian year), "Jxxxx.x" (Julian year), "JDjjjjjjjj.jjj" (Julian day number) and pure doubles as JD.
	//! The most common cases, "B1875" (for IAU boundaries) and "J2000" are handled most efficiently.
	bool loadBoundaries(const QJsonArray& boundaryData, const QString& epoch);

	//! Draw the constellation lines at the epoch given by the StelCore.
	void drawLines(StelPainter& sPainter, const StelCore* core) const;
	//! Draw the constellation art. obsVelocity required for aberration
	void drawArt(StelPainter& sPainter, const Vec3d &obsVelocity) const;
	//! Draw the constellation name labels.
	void drawNames(StelPainter& sPainter, const Vec3d &obsVelocity) const;
	//! Draw the constellation boundaries.
	//! @param obsVelocity is the speed vector of the observer planet to distort boundaries by aberration.
	void drawBoundaries(StelPainter& sPainter, const Vec3d &obsVelocity) const;
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
	//! do not use until it exhibits the proper behavior.
	StelObject* getSelected(void) const;

	std::vector<Constellation*> selected; // More than one can be selected at a time

	Constellation* isStarIn(const StelObject *s) const;
	Constellation* isObjectIn(const StelObject *s) const;
	Constellation* findFromAbbreviation(const QString& abbreviation) const;
	std::vector<Constellation*> constellations;
	QFont asterFont;
	StarMgr* hipStarMgr;

	bool isolateSelected; // true to pick individual constellations.
	bool flagConstellationPick; // TODO: CLEAR DESCRIPTION
	std::vector<std::vector<Vec3d> *> allBoundarySegments;

	QStringList constellationsEnglishNames;

	//! this controls how constellations (and also star names) are printed: Abbreviated/as-given/translated
	ConstellationDisplayStyle constellationDisplayStyle;
	static const QMap<QString, ConstellationDisplayStyle>ConstellationDisplayStyleMap;

	// These are THE master settings - individual constellation settings can vary based on selection status
	float artFadeDuration;
	float artIntensity;
	double artIntensityMinimumFov;
	double artIntensityMaximumFov;
	bool artDisplayed;
	bool boundariesDisplayed;
	float boundariesFadeDuration;
	bool linesDisplayed;
	float linesFadeDuration;
	bool namesDisplayed;
	float namesFadeDuration;
	bool checkLoadingData;

	// Store the thickness of lines of the constellations
	int constellationLineThickness;

	// Store the thickness of boundaries of the constellations
	int constellationBoundariesThickness;
};

#endif // CONSTELLATIONMGR_HPP

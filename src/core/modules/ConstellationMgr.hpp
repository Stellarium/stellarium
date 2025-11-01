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
#include "StelObject.hpp"
#include "StelSkyCultureSkyPartition.hpp"

#include <vector>
#include <QString>
#include <QStringList>

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
//! Some sky cultures have up to 2 optional @ref StelSkycultureSkyPartition elements related to concepts of a Zodiac (the known cases are all similar, 12x30 degrees along the ecliptic),
//! or of sections along ecliptic or equator related to the Moon, called LunarSystems. These are more diverse, but still only at most one per skyculture.
//! Both of these systems bear concepts of coordinates and are important for timekeeping, but esp. the Chinese systems also for actual use as celestial coordinates.
class ConstellationMgr : public StelObjectModule
{
	Q_OBJECT
	Q_PROPERTY(bool artDisplayed              READ getFlagArt                        WRITE setFlagArt                       NOTIFY artDisplayedChanged)
	Q_PROPERTY(float artFadeDuration          READ getArtFadeDuration                WRITE setArtFadeDuration               NOTIFY artFadeDurationChanged)
	Q_PROPERTY(float artIntensity             READ getArtIntensity                   WRITE setArtIntensity                  NOTIFY artIntensityChanged)
	Q_PROPERTY(Vec3f boundariesColor          READ getBoundariesColor                WRITE setBoundariesColor               NOTIFY boundariesColorChanged)
	Q_PROPERTY(bool boundariesDisplayed       READ getFlagBoundaries                 WRITE setFlagBoundaries                NOTIFY boundariesDisplayedChanged)
	Q_PROPERTY(float boundariesFadeDuration	  READ getBoundariesFadeDuration         WRITE setBoundariesFadeDuration        NOTIFY boundariesFadeDurationChanged)
	Q_PROPERTY(int fontSize	                  READ getFontSize                       WRITE setFontSize                      NOTIFY fontSizeChanged)
	Q_PROPERTY(bool isolateSelected	          READ getFlagIsolateSelected            WRITE setFlagIsolateSelected           NOTIFY isolateSelectedChanged)
	Q_PROPERTY(bool flagConstellationPick     READ getFlagConstellationPick          WRITE setFlagConstellationPick	        NOTIFY flagConstellationPickChanged)
	Q_PROPERTY(Vec3f linesColor               READ getLinesColor                     WRITE setLinesColor                    NOTIFY linesColorChanged)
	Q_PROPERTY(bool linesDisplayed            READ getFlagLines                      WRITE setFlagLines                     NOTIFY linesDisplayedChanged)
	Q_PROPERTY(float linesFadeDuration        READ getLinesFadeDuration              WRITE setLinesFadeDuration             NOTIFY linesFadeDurationChanged)
	Q_PROPERTY(Vec3f namesColor               READ getLabelsColor                    WRITE setLabelsColor                   NOTIFY namesColorChanged)
	Q_PROPERTY(bool namesDisplayed            READ getFlagLabels                     WRITE setFlagLabels                    NOTIFY namesDisplayedChanged)
	Q_PROPERTY(float namesFadeDuration        READ getLabelsFadeDuration             WRITE setLabelsFadeDuration            NOTIFY namesFadeDurationChanged)
	Q_PROPERTY(int constellationLineThickness READ getConstellationLineThickness     WRITE setConstellationLineThickness    NOTIFY constellationLineThicknessChanged)
	Q_PROPERTY(int boundariesThickness	  READ getBoundariesThickness            WRITE setBoundariesThickness           NOTIFY boundariesThicknessChanged)
	Q_PROPERTY(bool  hullsDisplayed           READ getFlagHulls                      WRITE setFlagHulls                     NOTIFY hullsDisplayedChanged)
	Q_PROPERTY(Vec3f hullsColor               READ getHullsColor                     WRITE setHullsColor                    NOTIFY hullsColorChanged)
	Q_PROPERTY(int   hullsThickness	          READ getHullsThickness                 WRITE setHullsThickness                NOTIFY hullsThicknessChanged)
	Q_PROPERTY(float hullsFadeDuration	  READ getHullsFadeDuration              WRITE setHullsFadeDuration             NOTIFY hullsFadeDurationChanged)

	Q_PROPERTY(bool  zodiacDisplayed          READ getFlagZodiac                     WRITE setFlagZodiac                    NOTIFY zodiacDisplayedChanged)
	Q_PROPERTY(Vec3f zodiacColor              READ getZodiacColor                    WRITE setZodiacColor                   NOTIFY zodiacColorChanged)
	Q_PROPERTY(int   zodiacThickness	  READ getZodiacThickness                WRITE setZodiacThickness               NOTIFY zodiacThicknessChanged)
	Q_PROPERTY(float zodiacFadeDuration	  READ getZodiacFadeDuration             WRITE setZodiacFadeDuration            NOTIFY zodiacFadeDurationChanged)
	Q_PROPERTY(bool  lunarSystemDisplayed     READ getFlagLunarSystem                WRITE setFlagLunarSystem               NOTIFY lunarSystemDisplayedChanged)
	Q_PROPERTY(Vec3f lunarSystemColor         READ getLunarSystemColor               WRITE setLunarSystemColor              NOTIFY lunarSystemColorChanged)
	Q_PROPERTY(int   lunarSystemThickness     READ getLunarSystemThickness           WRITE setLunarSystemThickness          NOTIFY lunarSystemThicknessChanged)
	Q_PROPERTY(float lunarSystemFadeDuration  READ getLunarSystemFadeDuration        WRITE setLunarSystemFadeDuration       NOTIFY lunarSystemFadeDurationChanged)


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

	//! Search for StelObject in an area around a specified point.
	//! The function searches in a disk of diameter limitFov centered on v.
	//! Only visible objects (i.e. currently displayed on screen) should be returned.
	//! @param v equatorial position at epoch J2000 (without aberration).
	//! @param limitFov angular diameter of the searching zone in degree. (ignored here. Only v is queried.)
	//! @param core the StelCore instance to use.
	//! @return a list of constellations identified from their hulls when clicked inside.
	//! This can probably be used for selection when IAU borders don't exist and click does not identify a star in a constellation line.
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

	///////////////////////////////////////////////////////////////////////////
	//! Returns whether the current skyculture defines a zodiac-type cultural coordinate system
	bool hasZodiac() const {return !zodiac.isNull();}
	//! Returns whether the current skyculture defines a lunar-related cultural coordinate system (lunar stations or mansions)
	bool hasLunarSystem() const {return !lunarSystem.isNull();}
	//! @return the translated name of the Zodiac system
	QString getZodiacSystemName() const;
	//! @return the translated name of the Lunar system
	QString getLunarSystemName() const;
	//!@return longitude in the culture's zodiacal longitudes (usually sign, degrees, minutes)
	QString getZodiacCoordinate(Vec3d eqNow) const;
	//! @return lunar station in the culture's Lunar system
	QString getLunarSystemCoordinate(Vec3d eqNow) const;

	//! @return the translated name of IAU constellation cst
	static QString getIAUconstellationName(const QString &cst);

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

	//! Set constellation labels fade duration in seconds
	void setLabelsFadeDuration(const float duration);
	//! Get constellation labels fade duration in seconds
	float getLabelsFadeDuration() const;

	//! Define hull line color
	//! @param color The color of hull lines
	//! @code
	//! // example of usage in scripts (Qt6-based Stellarium)
	//! var c = new Color(1.0, 0.0, 0.0);
	//! ConstellationMgr.setHullsColor(c.toVec3f());
	//! @endcode
	void setHullsColor(const Vec3f& color);
	//! Get current hulls color
	Vec3f getHullsColor() const;

	//! Set whether constellation hull lines will be displayed
	void setFlagHulls(const bool displayed);
	//! Get whether constellation boundaries lines are displayed
	bool getFlagHulls(void) const;

	//! Set constellation hulls fade duration in second
	void setHullsFadeDuration(const float duration);
	//! Get constellation hulls fade duration in second
	float getHullsFadeDuration() const;

	//! Set the font size used for constellation names display
	void setFontSize(const int newFontSize);
	//! Get the font size used for constellation names display
	int getFontSize() const;

	//! Set the thickness of lines of the constellations
	//! @param thickness of line in pixels
	void setConstellationLineThickness(const int thickness);
	//! Get the thickness of lines of the constellations
	int getConstellationLineThickness() const { return constellationLineThickness; }

	//! Set the thickness of boundaries of the constellations
	//! @param thickness of line in pixels
	void setBoundariesThickness(const int thickness);
	//! Get the thickness of boundaries of the constellations
	int getBoundariesThickness() const { return boundariesThickness; }

	//! Set the thickness of constellation hulls
	//! @param thickness of line in pixels
	void setHullsThickness(const int thickness);
	//! Get the thickness of constellations hulls
	int getHullsThickness() const { return hullsThickness; }


	//! Define zodiac line color
	//! @param color The color of zodiac related lines
	//! @code
	//! // example of usage in scripts (Qt6-based Stellarium)
	//! var c = new Color(1.0, 0.0, 0.0);
	//! ConstellationMgr.setZodiacColor(c.toVec3f());
	//! @endcode
	void setZodiacColor(const Vec3f& color);
	//! Get current zodiac color
	Vec3f getZodiacColor() const;

	//! Set whether zodiac will be displayed, if defined
	void setFlagZodiac(const bool displayed);
	//! Get whether zodiac-related lines are displayed (if defined in the skyculture)
	bool getFlagZodiac(void) const;

	//! Set zodiac fade duration in second
	void setZodiacFadeDuration(const float duration);
	//! Get zodiac fade duration in second
	float getZodiacFadeDuration() const;

	//! Define lunarSystem line color
	//! @param color The color of lunarSystem lines
	//! @code
	//! // example of usage in scripts (Qt6-based Stellarium)
	//! var c = new Color(1.0, 0.0, 0.0);
	//! ConstellationMgr.setLunarSystemColor(c.toVec3f());
	//! @endcode
	void setLunarSystemColor(const Vec3f& color);
	//! Get current lunarSystem color
	Vec3f getLunarSystemColor() const;

	//! Set whether lunarSystem lines will be displayed, if defined
	void setFlagLunarSystem(const bool displayed);
	//! Get whether lunarSystem lines are displayed
	bool getFlagLunarSystem(void) const;

	//! Set lunarSystem fade duration in second
	void setLunarSystemFadeDuration(const float duration);
	//! Get lunarSystem fade duration in second
	float getLunarSystemFadeDuration() const;

	//! Set the thickness of zodiac-related lines
	//! @param thickness of line in pixels
	void setZodiacThickness(const int thickness);
	//! Get the thickness of zodiac-related lines
	int getZodiacThickness() const { return zodiacThickness; }
	//! Set the thickness of lunarSystem-related lines
	//! @param thickness of line in pixels
	void setLunarSystemThickness(const int thickness);
	//! Get the thickness of lunarSystem-related lines
	int getLunarSystemThickness() const { return lunarSystemThickness; }


	//! Remove constellations from selected objects
	void deselectConstellations(void);

	//! Select all constellations
	void selectAllConstellations(void);

	//! Select the constellation by its English name. Calling this method will enable
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
	//! @note the method will correctly work for sky cultures with boundaries
	//! otherwise you may use star names from constellation lines as celestial body
	void selectConstellationByObjectName(const QString& englishName);
	//! Remove the constellation from list of selected constellations by its English
	//! name. Calling this method will enable isolated selection for the constellations
	//! if it is not enabled yet.
	//! @param englishName the English name of the constellation
	//! @code
	//! // example of usage in scripts: remove Orion from the selection of constellations
	//! ConstellationMgr.deselectConstellation("Orion");
	//! @endcode
	//! @note all constellations will be hidden when list of selected constellations is empty
	void deselectConstellation(const QString& englishName);

	//! Get the list of English names of all constellations for loaded sky culture
	QStringList getConstellationsEnglishNames();

	//! Create a list of entries: Constellation: Hull_area to logfile
	//! @todo: Extend with GUI etc?
	void outputHullAreas(const QString &fileNamePrefix="hullAreas") const;

	//! Create a list of stars (CSV file) within the convex hull of constellation.
	//! This command is perfect to be used as scripting command to extract a data list.
	//! @param englishName name of the constellation. Either englishName or abbreviation.
	//! @param hipOnly (default: true) list only Hipparcos stars
	//! @param maxMag (default: 25) list stars down to this magnitude.
	//! @param fileNamePrefix prefix (name start) name of CSV file to be written.
	//! The file will be written into the Stellarium User Data directory,
	//! the full filename will be fileNamePrefix_englishName_maxMag.csv.
	void starsInHullOf(const QString &englishName, const bool hipOnly=true, const float maxMag=25.0f, const QString &fileNamePrefix="hullStars") const;


signals:
	void artDisplayedChanged(const bool displayed);
	void artFadeDurationChanged(const float duration);
	void artIntensityChanged(const double intensity);
	void boundariesColorChanged(const Vec3f & color);
	void boundariesDisplayedChanged(const bool displayed);
	void boundariesFadeDurationChanged(const float duration);
	void boundariesThicknessChanged(int thickness);
	void hullsColorChanged(const Vec3f & color);
	void hullsDisplayedChanged(const bool displayed);
	void hullsFadeDurationChanged(const float duration);
	void hullsThicknessChanged(int thickness);
	void zodiacColorChanged(const Vec3f & color);
	void zodiacDisplayedChanged(const bool displayed);
	void zodiacFadeDurationChanged(const float duration);
	void zodiacThicknessChanged(int thickness);
	void lunarSystemColorChanged(const Vec3f & color);
	void lunarSystemDisplayedChanged(const bool displayed);
	void lunarSystemFadeDurationChanged(const float duration);
	void lunarSystemThicknessChanged(int thickness);
	void fontSizeChanged(const int newSize);
	void isolateSelectedChanged(const bool isolate);
	void flagConstellationPickChanged(const bool mode);
	void linesColorChanged(const Vec3f & color);
	void linesDisplayedChanged(const bool displayed);
	void linesFadeDurationChanged(const float duration);
	void namesColorChanged(const Vec3f & color);
	void namesDisplayedChanged(const bool displayed);
	void namesFadeDurationChanged(const float duration);
	void constellationsDisplayStyleChanged(const StelObject::CulturalDisplayStyle style);
	void constellationLineThicknessChanged(int thickness);

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
	void loadLinesNamesAndArt(const StelSkyCulture& culture);

	//! Recreate convex hulls. This needs to be done when stars have shifted due to proper motion.
	//! Should be ocasionally triggered in update().
	void recreateHulls();

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
	//! Draw the constellation hulls.
	//! @param obsVelocity is the speed vector of the observer planet to distort hulls by aberration.
	void drawHulls(StelPainter& sPainter, const Vec3d &obsVelocity) const;
	//! Draw the zodiac, if any is defined in the current skyculture.
	//! @param obsVelocity is the speed vector of the observer planet to distort zodiac lines by aberration.
	void drawZodiac(StelPainter& sPainter, const Vec3d &obsVelocity) const;
	//! Draw the lunar system lines, if any is defined in the current skyculture.
	//! @param obsVelocity is the speed vector of the observer planet to distort lunarSystem lines by aberration.
	void drawLunarSystem(StelPainter& sPainter, const Vec3d &obsVelocity) const;

	//! Handle single and multi-constellation selections.
	void setSelectedConst(QList <Constellation*> cList);
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
	QList<Constellation*> getSelected(void) const;

	QList<Constellation*> selected; // More than one can be selected at a time

public:
	//! Return list of constellations the object is member of.
	//! In case of IAU constellations, the list is guaranteed to be of length 1.
	//! @param useHull Prefer to use constellation hull, not IAU borders
	QList<Constellation*> isObjectIn(const StelObject *s, bool useHull) const;
private:
	Constellation* findFromAbbreviation(const QString& abbreviation) const;
	QList<Constellation*> constellations; //!< Constellations in the current SkyCulture
	StarMgr* hipStarMgr;

	bool isolateSelected; //!< true to pick individual constellations.
	bool flagConstellationPick; // TODO: CLEAR DESCRIPTION
	std::vector<std::vector<Vec3d> *> allBoundarySegments;
	static QMap<QString, QString>iauConstellationNames; //!< maps abbreviation to full (translated) name

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
	bool hullsDisplayed;
	float hullsFadeDuration;
	float zodiacFadeDuration;
	float lunarSystemFadeDuration;

	bool checkLoadingData;

	int fontSize;
	int constellationLineThickness;   //!< line width of the constellation lines
	int boundariesThickness;          //!< line width of the constellation boundaries
	int hullsThickness;               //!< line width of the constellation boundaries
	int zodiacThickness;              //!< line width of the zodiac lines, if any
	int lunarSystemThickness;         //!< line width of the lunarSystem lines, if any
	Vec3f zodiacColor;
	Vec3f lunarSystemColor;
	LinearFader zodiacFader;
	LinearFader lunarSystemFader;

	//! optional zodiac description from index.json
	StelSkyCultureSkyPartitionP zodiac;
	//! optional lunarSystem description from index.json
	StelSkyCultureSkyPartitionP lunarSystem;
};

#endif // CONSTELLATIONMGR_HPP

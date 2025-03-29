/*
 * Stellarium
 * Copyright (C) 2002 Fabien Chereau
 * Copyright (c) 2010 Bogdan Marinov
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

#ifndef SOLARSYSTEM_HPP
#define SOLARSYSTEM_HPP
//sun is already defined in Sun C/Solaris
#if defined(sun)
#undef sun
#endif

#include "StelObjectModule.hpp"
#include "StelTextureTypes.hpp"
#include "Planet.hpp"
#include "StelHips.hpp"

#include <QFont>

class Orbit;
class StelSkyCulture;
class StelTranslator;
class StelObject;
class StelCore;
class StelProjector;
class QSettings;

typedef QSharedPointer<Planet> PlanetP;

//! @class SolarSystem
//! This StelObjectModule derivative is used to model SolarSystem bodies.
//! This includes the Major Planets (class Planet), Minor Planets (class MinorPlanet) and Comets (class Comet).
// GZ's documentation attempt, early 2017.
//! This class and the handling of solar system data has seen many changes, and unfortunately, not much has been consistently documented.
//! The following is a reverse-engineered analysis.
//!
class SolarSystem : public StelObjectModule, protected QOpenGLFunctions
{
	Q_OBJECT
	// This is a "forwarding property" which sets labeling into all planets.
	Q_PROPERTY(bool labelsDisplayed			READ getFlagLabels			WRITE setFlagLabels			NOTIFY labelsDisplayedChanged)
	Q_PROPERTY(bool trailsDisplayed			READ getFlagTrails			WRITE setFlagTrails			NOTIFY trailsDisplayedChanged)
	Q_PROPERTY(int maxTrailPoints			READ getMaxTrailPoints			WRITE setMaxTrailPoints			NOTIFY maxTrailPointsChanged)
	Q_PROPERTY(int maxTrailTimeExtent		READ getMaxTrailTimeExtent		WRITE setMaxTrailTimeExtent		NOTIFY maxTrailTimeExtentChanged)
	Q_PROPERTY(int trailsThickness			READ getTrailsThickness			WRITE setTrailsThickness		NOTIFY trailsThicknessChanged)
	// This is a "forwarding property" only, without own variable.
	Q_PROPERTY(bool flagHints			READ getFlagHints			WRITE setFlagHints			NOTIFY flagHintsChanged)
	Q_PROPERTY(bool flagMarkers			READ getFlagMarkers			WRITE setFlagMarkers			NOTIFY markersDisplayedChanged)
	Q_PROPERTY(bool flagPointer			READ getFlagPointer			WRITE setFlagPointer			NOTIFY flagPointerChanged)
	//Q_PROPERTY(bool //		READ getFlagNativePlanetNames		WRITE setFlagNativePlanetNames		NOTIFY flagNativePlanetNamesChanged)
	Q_PROPERTY(bool planetsDisplayed		READ getFlagPlanets			WRITE setFlagPlanets			NOTIFY flagPlanetsDisplayedChanged)
	Q_PROPERTY(bool flagOrbits			READ getFlagOrbits			WRITE setFlagOrbits			NOTIFY flagOrbitsChanged)
	Q_PROPERTY(bool flagPlanetsOrbits		READ getFlagPlanetsOrbits		WRITE setFlagPlanetsOrbits		NOTIFY flagPlanetsOrbitsChanged)
	Q_PROPERTY(bool flagPlanetsOrbitsOnly		READ getFlagPlanetsOrbitsOnly		WRITE setFlagPlanetsOrbitsOnly		NOTIFY flagPlanetsOrbitsOnlyChanged)
	Q_PROPERTY(bool flagPermanentOrbits		READ getFlagPermanentOrbits		WRITE setFlagPermanentOrbits		NOTIFY flagPermanentOrbitsChanged)
	Q_PROPERTY(bool flagIsolatedOrbits		READ getFlagIsolatedOrbits		WRITE setFlagIsolatedOrbits		NOTIFY flagIsolatedOrbitsChanged)
	Q_PROPERTY(bool flagOrbitsWithMoons		READ getFlagOrbitsWithMoons		WRITE setFlagOrbitsWithMoons		NOTIFY flagOrbitsWithMoonsChanged)
	Q_PROPERTY(bool flagIsolatedTrails		READ getFlagIsolatedTrails		WRITE setFlagIsolatedTrails		NOTIFY flagIsolatedTrailsChanged)
	Q_PROPERTY(int numberIsolatedTrails		READ getNumberIsolatedTrails		WRITE setNumberIsolatedTrails		NOTIFY numberIsolatedTrailsChanged)
	Q_PROPERTY(bool flagLightTravelTime		READ getFlagLightTravelTime		WRITE setFlagLightTravelTime		NOTIFY flagLightTravelTimeChanged)
	Q_PROPERTY(bool flagUseObjModels		READ getFlagUseObjModels		WRITE setFlagUseObjModels		NOTIFY flagUseObjModelsChanged)
	Q_PROPERTY(bool flagShowObjSelfShadows		READ getFlagShowObjSelfShadows		WRITE setFlagShowObjSelfShadows		NOTIFY flagShowObjSelfShadowsChanged)
	Q_PROPERTY(bool flagMoonScale			READ getFlagMoonScale			WRITE setFlagMoonScale			NOTIFY flagMoonScaleChanged)
	Q_PROPERTY(double moonScale			READ getMoonScale			WRITE setMoonScale			NOTIFY moonScaleChanged)
	Q_PROPERTY(bool flagMinorBodyScale		READ getFlagMinorBodyScale		WRITE setFlagMinorBodyScale		NOTIFY flagMinorBodyScaleChanged)
	Q_PROPERTY(double minorBodyScale		READ getMinorBodyScale			WRITE setMinorBodyScale			NOTIFY minorBodyScaleChanged)
	Q_PROPERTY(bool flagPlanetScale			READ getFlagPlanetScale			WRITE setFlagPlanetScale		NOTIFY flagPlanetScaleChanged)
	Q_PROPERTY(double planetScale			READ getPlanetScale			WRITE setPlanetScale			NOTIFY planetScaleChanged)
	Q_PROPERTY(bool flagSunScale			READ getFlagSunScale			WRITE setFlagSunScale			NOTIFY flagSunScaleChanged)
	Q_PROPERTY(double sunScale			READ getSunScale			WRITE setSunScale			NOTIFY sunScaleChanged)
	Q_PROPERTY(double labelsAmount			READ getLabelsAmount			WRITE setLabelsAmount			NOTIFY labelsAmountChanged)
	Q_PROPERTY(bool flagPermanentSolarCorona	READ getFlagPermanentSolarCorona	WRITE setFlagPermanentSolarCorona	NOTIFY flagPermanentSolarCoronaChanged)
	// Ephemeris-related properties
	Q_PROPERTY(bool ephemerisMarkersDisplayed	READ getFlagEphemerisMarkers		WRITE setFlagEphemerisMarkers		NOTIFY ephemerisMarkersChanged)
	Q_PROPERTY(bool ephemerisHorizontalCoordinates	READ getFlagEphemerisHorizontalCoordinates	WRITE setFlagEphemerisHorizontalCoordinates	NOTIFY ephemerisHorizontalCoordinatesChanged)
	Q_PROPERTY(bool ephemerisDatesDisplayed		READ getFlagEphemerisDates		WRITE setFlagEphemerisDates		NOTIFY ephemerisDatesChanged)
	Q_PROPERTY(bool ephemerisMagnitudesDisplayed	READ getFlagEphemerisMagnitudes		WRITE setFlagEphemerisMagnitudes	NOTIFY ephemerisMagnitudesChanged)
	Q_PROPERTY(bool ephemerisLineDisplayed		READ getFlagEphemerisLine		WRITE setFlagEphemerisLine		NOTIFY ephemerisLineChanged)
	Q_PROPERTY(int ephemerisLineThickness		READ getEphemerisLineThickness		WRITE setEphemerisLineThickness		NOTIFY ephemerisLineThicknessChanged)
	Q_PROPERTY(bool ephemerisSkippedData		READ getFlagEphemerisSkipData		WRITE setFlagEphemerisSkipData		NOTIFY ephemerisSkipDataChanged)
	Q_PROPERTY(bool ephemerisSkippedMarkers		READ getFlagEphemerisSkipMarkers	WRITE setFlagEphemerisSkipMarkers	NOTIFY ephemerisSkipMarkersChanged)
	Q_PROPERTY(int ephemerisDataStep		READ getEphemerisDataStep		WRITE setEphemerisDataStep		NOTIFY ephemerisDataStepChanged)
	Q_PROPERTY(int ephemerisDataLimit		READ getEphemerisDataLimit		WRITE setEphemerisDataLimit		NOTIFY ephemerisDataLimitChanged)
	Q_PROPERTY(bool ephemerisSmartDates		READ getFlagEphemerisSmartDates		WRITE setFlagEphemerisSmartDates	NOTIFY ephemerisSmartDatesChanged)
	Q_PROPERTY(bool ephemerisScaleMarkersDisplayed	READ getFlagEphemerisScaleMarkers	WRITE setFlagEphemerisScaleMarkers	NOTIFY ephemerisScaleMarkersChanged)
	Q_PROPERTY(bool ephemerisAlwaysOn		READ getFlagEphemerisAlwaysOn		WRITE setFlagEphemerisAlwaysOn		NOTIFY ephemerisAlwaysOnChanged)
	Q_PROPERTY(bool ephemerisNow			READ getFlagEphemerisNow		WRITE setFlagEphemerisNow		NOTIFY ephemerisNowChanged)
	// Great Red Spot (GRS) properties
	Q_PROPERTY(int grsLongitude			READ getGrsLongitude			WRITE setGrsLongitude			NOTIFY grsLongitudeChanged)
	Q_PROPERTY(double grsDrift			READ getGrsDrift			WRITE setGrsDrift			NOTIFY grsDriftChanged)
	Q_PROPERTY(double grsJD				READ getGrsJD				WRITE setGrsJD				NOTIFY grsJDChanged)
	// Eclipse algorithm properties
	Q_PROPERTY(bool earthShadowEnlargementDanjon    READ getFlagEarthShadowEnlargementDanjon    WRITE setFlagEarthShadowEnlargementDanjon   NOTIFY earthShadowEnlargementDanjonChanged)
	// Colors
	Q_PROPERTY(Vec3f labelsColor			READ getLabelsColor			WRITE setLabelsColor			NOTIFY labelsColorChanged)
	Q_PROPERTY(Vec3f pointerColor			READ getPointerColor			WRITE setPointerColor			NOTIFY pointerColorChanged)
	Q_PROPERTY(Vec3f trailsColor			READ getTrailsColor			WRITE setTrailsColor			NOTIFY trailsColorChanged)
	Q_PROPERTY(Vec3f orbitsColor			READ getOrbitsColor			WRITE setOrbitsColor			NOTIFY orbitsColorChanged)
	Q_PROPERTY(Vec3f majorPlanetsOrbitsColor	READ getMajorPlanetsOrbitsColor		WRITE setMajorPlanetsOrbitsColor	NOTIFY majorPlanetsOrbitsColorChanged)
	Q_PROPERTY(Vec3f minorPlanetsOrbitsColor	READ getMinorPlanetsOrbitsColor		WRITE setMinorPlanetsOrbitsColor	NOTIFY minorPlanetsOrbitsColorChanged)
	Q_PROPERTY(Vec3f dwarfPlanetsOrbitsColor	READ getDwarfPlanetsOrbitsColor		WRITE setDwarfPlanetsOrbitsColor	NOTIFY dwarfPlanetsOrbitsColorChanged)
	Q_PROPERTY(Vec3f moonsOrbitsColor		READ getMoonsOrbitsColor		WRITE setMoonsOrbitsColor		NOTIFY moonsOrbitsColorChanged)
	Q_PROPERTY(Vec3f cubewanosOrbitsColor		READ getCubewanosOrbitsColor		WRITE setCubewanosOrbitsColor		NOTIFY cubewanosOrbitsColorChanged)
	Q_PROPERTY(Vec3f plutinosOrbitsColor		READ getPlutinosOrbitsColor		WRITE setPlutinosOrbitsColor		NOTIFY plutinosOrbitsColorChanged)
	Q_PROPERTY(Vec3f scatteredDiskObjectsOrbitsColor	READ getScatteredDiskObjectsOrbitsColor		WRITE setScatteredDiskObjectsOrbitsColor	NOTIFY scatteredDiskObjectsOrbitsColorChanged)
	Q_PROPERTY(Vec3f oortCloudObjectsOrbitsColor	READ getOortCloudObjectsOrbitsColor	WRITE setOortCloudObjectsOrbitsColor		NOTIFY oortCloudObjectsOrbitsColorChanged)
	Q_PROPERTY(Vec3f cometsOrbitsColor		READ getCometsOrbitsColor		WRITE setCometsOrbitsColor		NOTIFY cometsOrbitsColorChanged)
	Q_PROPERTY(Vec3f sednoidsOrbitsColor		READ getSednoidsOrbitsColor		WRITE setSednoidsOrbitsColor		NOTIFY sednoidsOrbitsColorChanged)
	Q_PROPERTY(Vec3f interstellarOrbitsColor	READ getInterstellarOrbitsColor		WRITE setInterstellarOrbitsColor	NOTIFY interstellarOrbitsColorChanged)
	Q_PROPERTY(Vec3f mercuryOrbitColor		READ getMercuryOrbitColor		WRITE setMercuryOrbitColor		NOTIFY mercuryOrbitColorChanged)
	Q_PROPERTY(Vec3f venusOrbitColor		READ getVenusOrbitColor			WRITE setVenusOrbitColor		NOTIFY venusOrbitColorChanged)
	Q_PROPERTY(Vec3f earthOrbitColor		READ getEarthOrbitColor			WRITE setEarthOrbitColor		NOTIFY earthOrbitColorChanged)
	Q_PROPERTY(Vec3f marsOrbitColor			READ getMarsOrbitColor			WRITE setMarsOrbitColor			NOTIFY marsOrbitColorChanged)
	Q_PROPERTY(Vec3f jupiterOrbitColor		READ getJupiterOrbitColor		WRITE setJupiterOrbitColor		NOTIFY jupiterOrbitColorChanged)
	Q_PROPERTY(Vec3f saturnOrbitColor		READ getSaturnOrbitColor		WRITE setSaturnOrbitColor		NOTIFY saturnOrbitColorChanged)
	Q_PROPERTY(Vec3f uranusOrbitColor		READ getUranusOrbitColor		WRITE setUranusOrbitColor		NOTIFY uranusOrbitColorChanged)
	Q_PROPERTY(Vec3f neptuneOrbitColor		READ getNeptuneOrbitColor		WRITE setNeptuneOrbitColor		NOTIFY neptuneOrbitColorChanged)
	// Ephemeris-related properties
	Q_PROPERTY(Vec3f ephemerisGenericMarkerColor	READ getEphemerisGenericMarkerColor	WRITE setEphemerisGenericMarkerColor	NOTIFY ephemerisGenericMarkerColorChanged)
	Q_PROPERTY(Vec3f ephemerisSecondaryMarkerColor	READ getEphemerisSecondaryMarkerColor	WRITE setEphemerisSecondaryMarkerColor	NOTIFY ephemerisSecondaryMarkerColorChanged)
	Q_PROPERTY(Vec3f ephemerisSelectedMarkerColor	READ getEphemerisSelectedMarkerColor	WRITE setEphemerisSelectedMarkerColor	NOTIFY ephemerisSelectedMarkerColorChanged)
	Q_PROPERTY(Vec3f ephemerisMercuryMarkerColor	READ getEphemerisMercuryMarkerColor	WRITE setEphemerisMercuryMarkerColor	NOTIFY ephemerisMercuryMarkerColorChanged)
	Q_PROPERTY(Vec3f ephemerisVenusMarkerColor	READ getEphemerisVenusMarkerColor	WRITE setEphemerisVenusMarkerColor	NOTIFY ephemerisVenusMarkerColorChanged)
	Q_PROPERTY(Vec3f ephemerisMarsMarkerColor	READ getEphemerisMarsMarkerColor	WRITE setEphemerisMarsMarkerColor	NOTIFY ephemerisMarsMarkerColorChanged)
	Q_PROPERTY(Vec3f ephemerisJupiterMarkerColor	READ getEphemerisJupiterMarkerColor	WRITE setEphemerisJupiterMarkerColor	NOTIFY ephemerisJupiterMarkerColorChanged)
	Q_PROPERTY(Vec3f ephemerisSaturnMarkerColor	READ getEphemerisSaturnMarkerColor	WRITE setEphemerisSaturnMarkerColor	NOTIFY ephemerisSaturnMarkerColorChanged)
	// Color style
	Q_PROPERTY(QString orbitColorStyle		READ getOrbitColorStyle			WRITE setOrbitColorStyle		NOTIFY orbitColorStyleChanged)
	Q_PROPERTY(QString apparentMagnitudeAlgorithmOnEarth	READ getApparentMagnitudeAlgorithmOnEarth	WRITE setApparentMagnitudeAlgorithmOnEarth	NOTIFY apparentMagnitudeAlgorithmOnEarthChanged)
	Q_PROPERTY(int orbitsThickness			READ getOrbitsThickness			WRITE setOrbitsThickness		NOTIFY orbitsThicknessChanged)
	Q_PROPERTY(bool flagDrawMoonHalo		READ getFlagDrawMoonHalo		WRITE setFlagDrawMoonHalo		NOTIFY flagDrawMoonHaloChanged)
	Q_PROPERTY(bool flagDrawSunHalo			READ getFlagDrawSunHalo			WRITE setFlagDrawSunHalo		NOTIFY flagDrawSunHaloChanged)
	Q_PROPERTY(int extraThreads                     READ getExtraThreads                    WRITE setExtraThreads                   NOTIFY extraThreadsChanged)
	Q_PROPERTY(double markerMagThreshold            READ getMarkerMagThreshold              WRITE setMarkerMagThreshold             NOTIFY markerMagThresholdChanged)

public:
	SolarSystem();
	~SolarSystem() override;

	///////////////////////////////////////////////////////////////////////////
	// Methods defined in the StelModule class
	//! Initialize the SolarSystem.
	//! Includes:
	//! - loading planetary body orbital and model data from data/ssystem.ini
	//! - perform initial planet position calculation
	//! - set display options from application settings
	void init() override;

	void deinit() override;
	
	//! Draw SolarSystem objects (planets).
	//! @param core The StelCore object.
	//! @return The maximum squared distance in pixels that any SolarSystem object
	//! has travelled since the last update.
	void draw(StelCore *core) override;

	//! Update time-varying components.
	//! This includes planet motion trails.
	void update(double deltaTime) override;

	//! Used to determine what order to draw the various StelModules.
	double getCallOrder(StelModuleActionName actionName) const override;

	///////////////////////////////////////////////////////////////////////////
	// Methods defined in StelObjectModule class
	//! Search for SolarSystem objects in some area around a point.
	//! @param v A vector representing a point in the sky in equatorial J2000 coordinates (without aberration).
	//! @param limitFov The radius of the circle around the point v which
	//! defines the size of the area to search.
	//! @param core the core object
	//! @return QList of StelObjectP (pointers) containing all SolarSystem objects
	//! found in the specified area. This vector is not sorted by distance from v.
	QList<StelObjectP> searchAround(const Vec3d& v, double limitFov, const StelCore* core) const override;

	//! Search for a SolarSystem object based on the localised name.
	//! @param nameI18n the case in-sensitive translated planet name.
	//! @return a StelObjectP for the object if found, else Q_NULLPTR.
	StelObjectP searchByNameI18n(const QString& nameI18n) const override;

	//! Search for a SolarSystem object based on the English name.
	//! @param name the case in-sensitive English planet name.
	//! @return a StelObjectP for the object if found, else Q_NULLPTR.
	StelObjectP searchByName(const QString& name) const override;

	StelObjectP searchByID(const QString &id) const override
	{
		return searchByName(id);
	}

	//! Find and return the list of at most maxNbItem objects auto-completing the passed object name.
	//! @param objPrefix the case insensitive first letters of the searched object
	//! @param maxNbItem the maximum number of returned object names
	//! @param useStartOfWords the autofill mode for returned objects names
	//! @return a list of matching object name by order of relevance, or an empty list if nothing match
	QStringList listMatchingObjects(const QString& objPrefix, int maxNbItem=5, bool useStartOfWords=false) const override;
	QStringList listAllObjects(bool inEnglish) const override;
	QStringList listAllObjectsByType(const QString& objType, bool inEnglish) const override;
	QString getName() const override { return "Solar System"; }
	QString getStelObjectType() const override { return Planet::PLANET_TYPE; }

public slots:
	///////////////////////////////////////////////////////////////////////////
	// Method callable from script and GUI
	// Properties setters and getters
	//! Set flag which determines if planets are drawn or hidden.
	void setFlagPlanets(bool b);
	//! Get the current value of the flag which determines if planet are drawn or hidden.
	bool getFlagPlanets() const;

	//! Set flag which determines if planet trails are drawn or hidden.
	void setFlagTrails(bool b);
	//! Get the current value of the flag which determines if planet trails are drawn or hidden.
	bool getFlagTrails() const;

	//! Set thickness of trails.
	void setTrailsThickness(int v);
	//! Get thickness of trail.
	int getTrailsThickness() const {return trailsThickness;}

	//! Set maximum number of trail points. Too many points may slow down the application. 5000 seems to be a good balance.
	//! The trails are drawn for a maximum of 365 days and then fade out.
	//! If drawing many trails slows down the application, you can set a new maximum trail step length.
	//! Note that the fadeout may require more points or a decent simulation speed.
	void setMaxTrailPoints(int max);
	//! Get maximum number of trail points. Too many points may slow down the application. 5000 seems to be a good balance.
	int getMaxTrailPoints() const {return maxTrailPoints;}

	//! Set maximum number of trail time extent in years.
	//! Too many points may slow down the application. One year (365 days) seems to be a good balance.
	//! If drawing many trails slows down the application, you can set a new maximum trail time extent or step length.
	//! Note that the fadeout may require more points or a decent simulation speed.
	void setMaxTrailTimeExtent(int max);
	//! Get maximum number of trail time extent in years. Too many points may slow down the application. One year (365 days) seems to be a good balance.
	int getMaxTrailTimeExtent() const {return maxTrailTimeExtent;}

	//! Set flag which determines if planet hints are drawn or hidden along labels
	void setFlagHints(bool b);
	//! Get the current value of the flag which determines if planet hints are drawn or hidden along labels
	bool getFlagHints() const;

	//! Set flag which determines if planet markers are drawn for minor bodies
	void setFlagMarkers(bool b);
	//! Get the current value of the flag which determines if planet markers are drawn for minor bodies
	bool getFlagMarkers() const;

	//! Set flag which determines if planet labels are drawn or hidden.
	void setFlagLabels(bool b);
	//! Get the current value of the flag which determines if planet labels are drawn or hidden.
	bool getFlagLabels() const;

	//! Set the amount of planet labels. The real amount is also proportional with FOV.
	//! The limit is set in function of the planets magnitude
	//! @param a the amount between 0 and 10. 0 is no labels, 10 is maximum of labels
	void setLabelsAmount(double a) {if(!fuzzyEquals(a, labelsAmount)) {labelsAmount=a; StelApp::immediateSave("astro/labels_amount", a); emit labelsAmountChanged(a);}}
	//! Get the amount of planet labels. The real amount is also proportional with FOV.
	//! @return the amount between 0 and 10. 0 is no labels, 10 is maximum of labels
	double getLabelsAmount(void) const {return labelsAmount;}

	//! Set flag which determines if planet orbits are drawn or hidden.
	void setFlagOrbits(bool b);
	//! Get the current value of the flag which determines if planet orbits are drawn or hidden.
	bool getFlagOrbits() const {return flagOrbits;}

	//! Set flag which determines if the planet pointer (red cross) is drawn or hidden on a selected planet.
	void setFlagPointer(bool b) { if (b!=flagPointer) { flagPointer=b; StelApp::immediateSave("astro/flag_planets_pointers", b); emit flagPointerChanged(b); }}
	//! Get the current value of the flag which determines if planet pointers are drawn or hidden.
	bool getFlagPointer() const { return flagPointer;}

	//! Set flag which determines if the light travel time calculation is used or not.
	void setFlagLightTravelTime(bool b);
	//! Get the current value of the flag which determines if light travel time
	//! calculation is used or not.
	bool getFlagLightTravelTime(void) const {return flagLightTravelTime;}

	//! Set flag whether to use OBJ models for rendering, where available
	void setFlagUseObjModels(bool b) { if(b!=flagUseObjModels) { flagUseObjModels = b; StelApp::immediateSave("astro/flag_use_obj_models", b); emit flagUseObjModelsChanged(b); } }
	//! Get the current value of the flag which determines whether to use OBJ models for rendering, where available
	bool getFlagUseObjModels(void) const { return flagUseObjModels; }

	//! Set flag whether OBJ models should render self-shadowing (using a shadow map)
	void setFlagShowObjSelfShadows(bool b);
	//! Get the current value of the flag which determines whether OBJ models should render self-shadowing (using a shadow map)
	bool getFlagShowObjSelfShadows(void) const { return flagShowObjSelfShadows; }

	//! Set planet names font size.
	//! @return font size
	void setFontSize(int newFontSize);

	//! Set the color used to draw planet labels.
	//! @param c The color of the planet labels (R,G,B)
	//! @code
	//! // example of usage in scripts (Qt6-based Stellarium)
	//! var c = new Color(1.0, 0.0, 0.0);
	//! SolarSystem.setLabelsColor(c.toVec3f());
	//! @endcode
	void setLabelsColor(const Vec3f& c);
	//! Get the current color used to draw planet labels.
	//! @return current color
	Vec3f getLabelsColor(void) const;

	//! Set the color used to draw solar system object orbit lines.
	//! @param c The color of the solar system object orbit lines (R,G,B)
	//! @code
	//! // example of usage in scripts (Qt6-based Stellarium)
	//! var c = new Color(1.0, 0.0, 0.0);
	//! SolarSystem.setOrbitsColor(c.toVec3f());
	//! @endcode
	void setOrbitsColor(const Vec3f& c);
	//! Get the current color used to draw solar system object orbit lines.
	//! @return current color
	Vec3f getOrbitsColor(void) const;

	//! Set the color used to draw orbits lines of the major planets.
	//! @param c The color of orbits lines of the major planets (R,G,B)
	//! @code
	//! // example of usage in scripts (Qt6-based Stellarium)
	//! var c = new Color(1.0, 0.0, 0.0);
	//! SolarSystem.setMajorPlanetsOrbitsColor(c.toVec3f());
	//! @endcode
	void setMajorPlanetsOrbitsColor(const Vec3f& c);
	//! Get the current color used to draw orbits lines of the major planets.
	//! @return current color
	Vec3f getMajorPlanetsOrbitsColor(void) const;

	//! Set the color used to draw orbits lines of moons of planets.
	//! @param c The color of orbits lines of moons of planets lines (R,G,B)
	//! @code
	//! // example of usage in scripts (Qt6-based Stellarium)
	//! var c = new Color(1.0, 0.0, 0.0);
	//! SolarSystem.setMoonsOrbitsColor(c.toVec3f());
	//! @endcode
	void setMoonsOrbitsColor(const Vec3f& c);
	//! Get the current color used to draw orbits lines of moons of planets.
	//! @return current color
	Vec3f getMoonsOrbitsColor(void) const;

	//! Set the color used to draw orbits lines of the minor planets.
	//! @param c The color of orbits lines of the minor planets (R,G,B)
	//! @code
	//! // example of usage in scripts (Qt6-based Stellarium)
	//! var c = new Color(1.0, 0.0, 0.0);
	//! SolarSystem.setMinorPlanetsOrbitsColor(c.toVec3f());
	//! @endcode
	void setMinorPlanetsOrbitsColor(const Vec3f& c);
	//! Get the current color used to draw orbits lines of the minor planets.
	//! @return current color
	Vec3f getMinorPlanetsOrbitsColor(void) const;

	//! Set the color used to draw orbits lines of the dwarf planets.
	//! @param c The color of orbits lines of the dwarf planets (R,G,B)
	//! @code
	//! // example of usage in scripts (Qt6-based Stellarium)
	//! var c = new Color(1.0, 0.0, 0.0);
	//! SolarSystem.setDwarfPlanetsOrbitsColor(c.toVec3f());
	//! @endcode
	void setDwarfPlanetsOrbitsColor(const Vec3f& c);
	//! Get the current color used to draw orbits lines of the dwarf planets.
	//! @return current color
	Vec3f getDwarfPlanetsOrbitsColor(void) const;

	//! Set the color used to draw orbits lines of cubewanos.
	//! @param c The color of orbits lines of cubewanos (R,G,B)
	//! @code
	//! // example of usage in scripts (Qt6-based Stellarium)
	//! var c = new Color(1.0, 0.0, 0.0);
	//! SolarSystem.setCubewanosOrbitsColor(c.toVec3f());
	//! @endcode
	void setCubewanosOrbitsColor(const Vec3f& c);
	//! Get the current color used to draw orbits lines of cubewanos.
	//! @return current color
	Vec3f getCubewanosOrbitsColor(void) const;

	//! Set the color used to draw orbits lines of plutinos.
	//! @param c The color of orbits lines of plutinos (R,G,B)
	//! @code
	//! // example of usage in scripts (Qt6-based Stellarium)
	//! var c = new Color(1.0, 0.0, 0.0);
	//! SolarSystem.setPlutinosOrbitsColor(c.toVec3f());
	//! @endcode
	void setPlutinosOrbitsColor(const Vec3f& c);
	//! Get the current color used to draw orbits lines of plutinos.
	//! @return current color
	Vec3f getPlutinosOrbitsColor(void) const;

	//! Set the color used to draw orbits lines of scattered disk objects.
	//! @param c The color of orbits lines of scattered disk objects (R,G,B)
	//! @code
	//! // example of usage in scripts (Qt6-based Stellarium)
	//! var c = new Color(1.0, 0.0, 0.0);
	//! SolarSystem.setScatteredDiskObjectsOrbitsColor(c.toVec3f());
	//! @endcode
	void setScatteredDiskObjectsOrbitsColor(const Vec3f& c);
	//! Get the current color used to draw orbits lines of scattered disk objects.
	//! @return current color
	Vec3f getScatteredDiskObjectsOrbitsColor(void) const;

	//! Set the color used to draw orbits lines of Oort cloud objects.
	//! @param c The color of orbits lines of Oort cloud objects (R,G,B)
	//! @code
	//! // example of usage in scripts (Qt6-based Stellarium)
	//! var c = new Color(1.0, 0.0, 0.0);
	//! SolarSystem.setOortCloudObjectsOrbitsColor(c.toVec3f());
	//! @endcode
	void setOortCloudObjectsOrbitsColor(const Vec3f& c);
	//! Get the current color used to draw orbits lines of Oort cloud objects.
	//! @return current color
	Vec3f getOortCloudObjectsOrbitsColor(void) const;

	//! Set the color used to draw comet orbit lines.
	//! @param c The color of the comet orbit lines (R,G,B)
	//! @code
	//! // example of usage in scripts (Qt6-based Stellarium)
	//! var c = new Color(1.0, 0.0, 0.0);
	//! SolarSystem.setCometsOrbitsColor(c.toVec3f());
	//! @endcode
	void setCometsOrbitsColor(const Vec3f& c);
	//! Get the current color used to draw comet orbit lines.
	//! @return current color
	Vec3f getCometsOrbitsColor(void) const;

	//! Set the color used to draw sednoid orbit lines.
	//! @param c The color of the sednoid orbit lines (R,G,B)
	//! @code
	//! // example of usage in scripts (Qt6-based Stellarium)
	//! var c = new Color(1.0, 0.0, 0.0);
	//! SolarSystem.setSednoidsOrbitsColor(c.toVec3f());
	//! @endcode
	void setSednoidsOrbitsColor(const Vec3f& c);
	//! Get the current color used to draw sednoid orbit lines.
	//! @return current color
	Vec3f getSednoidsOrbitsColor(void) const;

	//! Set the color used to draw interstellar orbit (hyperbolic trajectory) lines.
	//! @param c The color of the interstellar orbit lines (R,G,B)
	//! @code
	//! // example of usage in scripts (Qt6-based Stellarium)
	//! var c = new Color(1.0, 0.0, 0.0);
	//! SolarSystem.setInterstellarOrbitsColor(c.toVec3f());
	//! @endcode
	void setInterstellarOrbitsColor(const Vec3f& c);
	//! Get the current color used to draw interstellar orbit lines.
	//! @return current color
	Vec3f getInterstellarOrbitsColor(void) const;

	//! Set the color used to draw Mercury orbit line.
	//! @param c The color of Mercury orbit line (R,G,B)
	//! @code
	//! // example of usage in scripts (Qt6-based Stellarium)
	//! var c = new Color(1.0, 0.0, 0.0);
	//! SolarSystem.setMercuryOrbitColor(c.toVec3f());
	//! @endcode
	void setMercuryOrbitColor(const Vec3f& c);
	//! Get the current color used to draw Mercury orbit line.
	//! @return current color
	Vec3f getMercuryOrbitColor(void) const;

	//! Set the color used to draw Venus orbit line.
	//! @param c The color of Venus orbit line (R,G,B)
	//! @code
	//! // example of usage in scripts (Qt6-based Stellarium)
	//! var c = new Color(1.0, 0.0, 0.0);
	//! SolarSystem.setVenusOrbitColor(c.toVec3f());
	//! @endcode
	void setVenusOrbitColor(const Vec3f& c);
	//! Get the current color used to draw Venus orbit line.
	//! @return current color
	Vec3f getVenusOrbitColor(void) const;

	//! Set the color used to draw Earth orbit line.
	//! @param c The color of Earth orbit line (R,G,B)
	//! @code
	//! // example of usage in scripts (Qt6-based Stellarium)
	//! var c = new Color(1.0, 0.0, 0.0);
	//! SolarSystem.setEarthOrbitColor(c.toVec3f());
	//! @endcode
	void setEarthOrbitColor(const Vec3f& c);
	//! Get the current color used to draw Earth orbit line.
	//! @return current color
	Vec3f getEarthOrbitColor(void) const;

	//! Set the color used to draw Mars orbit line.
	//! @param c The color of Mars orbit line (R,G,B)
	//! @code
	//! // example of usage in scripts (Qt6-based Stellarium)
	//! var c = new Color(1.0, 0.0, 0.0);
	//! SolarSystem.setMarsOrbitColor(c.toVec3f());
	//! @endcode
	void setMarsOrbitColor(const Vec3f& c);
	//! Get the current color used to draw Mars orbit line.
	//! @return current color
	Vec3f getMarsOrbitColor(void) const;

	//! Set the color used to draw Jupiter orbit line.
	//! @param c The color of Jupiter orbit line (R,G,B)
	//! @code
	//! // example of usage in scripts (Qt6-based Stellarium)
	//! var c = new Color(1.0, 0.0, 0.0);
	//! SolarSystem.setJupiterOrbitColor(c.toVec3f());
	//! @endcode
	void setJupiterOrbitColor(const Vec3f& c);
	//! Get the current color used to draw Jupiter orbit line.
	//! @return current color
	Vec3f getJupiterOrbitColor(void) const;

	//! Set the color used to draw Saturn orbit line.
	//! @param c The color of Saturn orbit line (R,G,B)
	//! @code
	//! // example of usage in scripts (Qt6-based Stellarium)
	//! var c = new Color(1.0, 0.0, 0.0);
	//! SolarSystem.setSaturnOrbitColor(c.toVec3f());
	//! @endcode
	void setSaturnOrbitColor(const Vec3f& c);
	//! Get the current color used to draw Saturn orbit line.
	//! @return current color
	Vec3f getSaturnOrbitColor(void) const;

	//! Set the color used to draw Uranus orbit line.
	//! @param c The color of Uranus orbit line (R,G,B)
	//! @code
	//! // example of usage in scripts (Qt6-based Stellarium)
	//! var c = new Color(1.0, 0.0, 0.0);
	//! SolarSystem.setUranusOrbitColor(c.toVec3f());
	//! @endcode
	void setUranusOrbitColor(const Vec3f& c);
	//! Get the current color used to draw Uranus orbit line.
	//! @return current color
	Vec3f getUranusOrbitColor(void) const;

	//! Set the color used to draw Neptune orbit line.
	//! @param c The color of Neptune orbit line (R,G,B)
	//! @code
	//! // example of usage in scripts (Qt6-based Stellarium)
	//! var c = new Color(1.0, 0.0, 0.0);
	//! SolarSystem.setNeptuneOrbitColor(c.toVec3f());
	//! @endcode
	void setNeptuneOrbitColor(const Vec3f& c);
	//! Get the current color used to draw Neptune orbit line.
	//! @return current color
	Vec3f getNeptuneOrbitColor(void) const;

	//! Set the color used to draw planet trails lines.
	//! @param c The color of the planet trails lines (R,G,B)
	//! @code
	//! // example of usage in scripts (Qt6-based Stellarium)
	//! var c = new Color(1.0, 0.0, 0.0);
	//! SolarSystem.setTrailsColor(c.toVec3f());
	//! @endcode
	void setTrailsColor(const Vec3f& c) {if (c!=trailsColor) { trailsColor=c; emit trailsColorChanged(c);}}
	//! Get the current color used to draw planet trails lines.
	//! @return current color
	Vec3f getTrailsColor() const {return trailsColor;}

	//! Set the color used to draw planet pointers.
	//! @param c The color of the planet pointers
	//! @code
	//! // example of usage in scripts (Qt6-based Stellarium)
	//! var c = new Color(1.0, 0.0, 0.0);
	//! SolarSystem.setPointerColor(c.toVec3f());
	//! @endcode
	void setPointerColor(const Vec3f& c) {if (c!=pointerColor) {pointerColor=c; emit pointerColorChanged(c);}}
	//! Get the current color used to draw planet pointers.
	//! @return current color
	Vec3f getPointerColor() const {return pointerColor;}

	//! Set flag which determines if Earth's moon is scaled or not.
	void setFlagMoonScale(bool b);
	//! Get the current value of the flag which determines if Earth's moon is scaled or not.
	bool getFlagMoonScale(void) const {return flagMoonScale;}

	//! Set the display scaling factor for Earth's moon.
	void setMoonScale(double f);
	//! Get the display scaling factor for Earth's moon.
	double getMoonScale(void) const {return moonScale;}

	//! Set flag which determines if minor bodies (everything except the 8 planets) are drawn scaled or not.
	void setFlagMinorBodyScale(bool b);
	//! Get the current value of the flag which determines if minor bodies (everything except the 8 planets) are drawn scaled or not.
	bool getFlagMinorBodyScale(void) const {return flagMinorBodyScale;}

	//! Set the display scaling factor for minor bodies.
	void setMinorBodyScale(double f);
	//! Get the display scaling factor for minor bodies.
	double getMinorBodyScale(void) const {return minorBodyScale;}

	//! Set flag which determines if planets are displayed scaled or not.
	void setFlagPlanetScale(bool b);
	//! Get the current value of the flag which determines if planets are displayed scaled or not.
	bool getFlagPlanetScale(void) const {return flagPlanetScale;}

	//! Set the display scaling factor for planets.
	void setPlanetScale(double f);
	//! Get the display scaling factor for planets.
	double getPlanetScale(void) const {return planetScale;}

	//! Set flag which determines if Sun is scaled or not.
	void setFlagSunScale(bool b);
	//! Get the current value of the flag which determines if Sun is scaled or not.
	bool getFlagSunScale(void) const {return flagSunScale;}

	//! Set the display scaling factor for Sun.
	void setSunScale(double f);
	//! Get the display scaling factor for Sun.
	double getSunScale(void) const {return sunScale;}

	//! Translate names. (public so that SolarSystemEditor can call it).
	void updateI18n();

	//! Get the V magnitude for Solar system bodies for scripts
	//! @param planetName the case in-sensitive English planet name.
	//! @param withExtinction the flag for use extinction effect for magnitudes (default not use)
	//! @return a magnitude
	float getPlanetVMagnitude(const QString &planetName, bool withExtinction=false) const;

	//! Get type for Solar system bodies for scripts
	//! @param planetName the case in-sensitive English planet name.
	//! @return a type of planet (star, planet, moon, observer, artificial, asteroid, plutino, comet, dwarf planet, cubewano, scattered disc object, Oort cloud object, sednoid, interstellar object)
	QString getPlanetType(const QString &planetName) const;

	//! Get distance to Solar system bodies for scripts
	//! @param planetName the case in-sensitive English planet name.
	//! @return a distance (in AU)
	double getDistanceToPlanet(const QString &planetName) const;

	//! Get elongation for Solar system bodies for scripts
	//! @param planetName the case in-sensitive English planet name.
	//! @return a elongation (in radians)
	double getElongationForPlanet(const QString &planetName) const;

	//! Get phase angle for Solar system bodies for scripts
	//! @param planetName the case in-sensitive English planet name.
	//! @return a phase angle (in radians)
	double getPhaseAngleForPlanet(const QString &planetName) const;

	//! Get phase for Solar system bodies for scripts
	//! @param planetName the case in-sensitive English planet name.
	//! @return phase, i.e. illuminated fraction [0..1]
	float getPhaseForPlanet(const QString &planetName) const;

	//! Set the algorithm for computation of apparent magnitudes for planets in case observer on the Earth.
	//! Possible values:
	//! @li @c Mueller1893 [Explanatory Supplement to the Astronomical Ephemeris, 1961] (visual magnitudes, based on visual observations by G. Mueller, 1877-91)
	//! @li @c AstrAlm1984 [Astronomical Almanac 1984] and later. These give V (instrumental) magnitudes.
	//! @li @c ExpSup1992 [Explanatory Supplement to the Astronomical Almanac, 1992] (algorithm contributed by Pere Planesas, Observatorio Astronomico Nacional)
	//! @li @c ExpSup2013 [Explanatory Supplement to the Astronomical Almanac, 3rd edition, 2013]
	//! @li @c Generic Visual magnitude based on phase angle and albedo.
	//! Details:
	//! @li J. Meeus "Astronomical Algorithms" (2nd ed. 1998, with corrections as of August 10, 2009) p.283-286.
	//! @li O. Montenbruck, T. Pfleger "Astronomy on the Personal Computer" (4th ed.) p.143-145.
	//! @li Daniel L. Harris "Photometry and Colorimetry of Planets and Satellites" http://adsabs.harvard.edu/abs/1961plsa.book..272H
	//! @li Sean E. Urban and P. Kenneth Seidelmann "Explanatory Supplement to the Astronomical Almanac" (3rd edition, 2013)
	//! It is interesting to note that Meeus in his discussion of "Harris" states that Harris did not give new values.
	//! The book indeed mentions a few values for the inner planets citing Danjon, but different from those then listed by Meeus.
	//! Therefore it must be assumed that the "Harris" values are misnomed, and are the least certain set.
	//! Hint: Default option in config.ini: astro/apparent_magnitude_algorithm = ExpSup2013
	//! @param algorithm the case in-sensitive algorithm name
	//! @note: The structure of algorithms is almost identical, just the numbers are different!
	//!        You should activate Mueller's algorithm to simulate the eye's impression. (Esp. Venus!)
	void setApparentMagnitudeAlgorithmOnEarth(const QString &algorithm);
	//! overload with numeric ID
	void setApparentMagnitudeAlgorithmOnEarth(const Planet::ApparentMagnitudeAlgorithm id);
	//! Get the algorithm used for computation of apparent magnitudes for planets in case observer on the Earth
	//! @see setApparentMagnitudeAlgorithmOnEarth()
	QString getApparentMagnitudeAlgorithmOnEarth() const;

	////! Set flag which enable use native names for planets or not.
	//void setFlagNativePlanetNames(bool b);
	////! Get the current value of the flag which enables showing native names for planets or not.
	//bool getFlagNativePlanetNames(void) const;

	//! Set flag which enabled the showing of isolated trails for selected objects only or not
	void setFlagIsolatedTrails(bool b);
	//! Get the current value of the flag which enables showing of isolated trails for selected objects only or not.
	bool getFlagIsolatedTrails(void) const;

	//! Set number of displayed of isolated trails for latest selected objects
	void setNumberIsolatedTrails(int n);
	//! Get the number of displayed of isolated trails for latest selected objects
	int getNumberIsolatedTrails(void) const;

	//! Set flag which enabled the showing of isolated orbits for selected objects only or not
	void setFlagIsolatedOrbits(bool b);
	//! Get the current value of the flag which enables showing of isolated orbits for selected objects only or not.
	bool getFlagIsolatedOrbits(void) const;

	//! Set flag which enabled the showing of planets orbits, regardless of the other orbit settings
	void setFlagPlanetsOrbits(bool b);
	//! Get the current value of the flag which enables showing of planets orbits, regardless of the other orbit settings.
	bool getFlagPlanetsOrbits(void) const;

	//! Set flag which enabled the showing of planets orbits only or not
	void setFlagPlanetsOrbitsOnly(bool b);
	//! Get the current value of the flag which enables showing of planets orbits only or not.
	bool getFlagPlanetsOrbitsOnly(void) const;

	//! Set flag which enables showing of planets orbits together mith orbits of their moons.
	void setFlagOrbitsWithMoons(bool b);
	//! Get the current value of the flag for showing of planets orbits together mith orbits of their moons.
	bool getFlagOrbitsWithMoons(void) const;

	//! Set flag which enabled the showing of solar corona when atmosphere is disabled (true) of draw the corona when total solar eclipses is happened only (false)
	void setFlagPermanentSolarCorona(bool b) {if (flagPermanentSolarCorona!=b){ flagPermanentSolarCorona = b; StelApp::immediateSave("viewing/flag_draw_sun_corona", b); emit flagPermanentSolarCoronaChanged(b); } }
	//! Get the current value of the flag which enables showing of solar corona when atmosphere is disabled or when total solar eclipses is happened only.
	bool getFlagPermanentSolarCorona(void) const { return flagPermanentSolarCorona; }

	//! Set longitude of Great Red Spot (System II is used)
	//! @param longitude (degrees)
	void setGrsLongitude(int longitude);
	//! Get longitude of Great Red Spot (System II is used)
	//! @return a longitude (degrees)
	int getGrsLongitude() const;

	//! Set speed of annual drift for Great Red Spot (System II is used)
	//! @param annual drift (degrees)
	void setGrsDrift(double drift);
	//! Get speed of annual drift for Great Red Spot (System II is used)
	double getGrsDrift() const;

	//! Set initial JD for calculation of position of Great Red Spot
	//! @param JD
	// TODO (GZ): Clarify whether this is JD or rather JDE?
	void setGrsJD(double JD);
	//! Get initial JD for calculation of position of Great Red Spot
	double getGrsJD();

	//! Set whether earth shadow should be enlarged following Danjon's method
	void setFlagEarthShadowEnlargementDanjon(bool b);
	//! Get whether earth shadow should be enlarged following Danjon's method
	bool getFlagEarthShadowEnlargementDanjon() const;

	//! Set style of colors of orbits for Solar system bodies
	//! @param style One of one_color | groups | major_planets | major_planets_minor_types
	void setOrbitColorStyle(const QString &style);
	//! Get style of colors of orbits for Solar system bodies
	//! @return One of one_color | groups | major_planets | major_planets_minor_types
	QString getOrbitColorStyle() const;

	//! Get list of objects by type
	//! @param objType object type
	QStringList getObjectsList(QString objType="all") const;

	//! Set flag which enables display of orbits for planets even if they are off screen
	void setFlagPermanentOrbits(bool b);
	bool getFlagPermanentOrbits() const;

	void setOrbitsThickness(int v);
	int getOrbitsThickness() const;

	void setFlagDrawMoonHalo(bool b);
	bool getFlagDrawMoonHalo() const;

	void setFlagDrawSunHalo(bool b);
	bool getFlagDrawSunHalo() const;

	//! Reload the planets. This can be helpful as scripting command when editing planet details, e.g. comet tails or magnitude parameters.
	void reloadPlanets();

	//! Reset and recreate trails
	void recreateTrails();

	//! Reset textures for planet @param planetName
	//! @note if @param planetName is empty then reset will happen for all solar system objects
	void resetTextures(const QString& planetName);

	//! Replace the texture for the planet @param planetName
	//! @param planetName - English name of the planet
	//! @param texName - file path for texture
	//! The texture path starts in the scripts directory.
	void setTextureForPlanet(const QString &planetName, const QString &texName);

	//! Return the number of additional threads (in addition to the main thread) configured to compute planet positions.
	int getExtraThreads() const {return extraThreads;}
	//! Configure the number of additional threads (in addition to the main thread) to compute planet positions.
	//! The argument will be bounded by 0 and QThreadPool::globalInstance()->maxThreadCount()-1
	void setExtraThreads(int n);

	//! Return the limiting absolute magnitude configured for plotting minor bodies. (min. mag. 37 includes de facto "all".)
	double getMarkerMagThreshold() const {return markerMagThreshold;}
	//! Configure the limiting absolute magnitude for plotting minor bodies. Configured value is clamped to -2..37 (practical limit)
	void setMarkerMagThreshold(double m);

signals:
	void labelsDisplayedChanged(bool b);
	void flagOrbitsChanged(bool b);
	void flagHintsChanged(bool b);
	void markersDisplayedChanged(bool b);
	void flagDrawMoonHaloChanged(bool b);
	void flagDrawSunHaloChanged(bool b);
	void trailsDisplayedChanged(bool b);
	void trailsThicknessChanged(int v);
	void orbitsThicknessChanged(int v);
	void maxTrailPointsChanged(int max);
	void maxTrailTimeExtentChanged(int max);
	void flagPointerChanged(bool b);
	//void flagNativePlanetNamesChanged(bool b);
	void flagPlanetsDisplayedChanged(bool b);
	void flagPlanetsOrbitsChanged(bool b);
	void flagPlanetsOrbitsOnlyChanged(bool b);
	void flagPermanentOrbitsChanged(bool b);
	void flagIsolatedOrbitsChanged(bool b);
	void flagOrbitsWithMoonsChanged(bool b);
	void flagIsolatedTrailsChanged(bool b);
	void numberIsolatedTrailsChanged(int n);
	void flagLightTravelTimeChanged(bool b);
	void flagUseObjModelsChanged(bool b);
	void flagShowObjSelfShadowsChanged(bool b);
	void flagMoonScaleChanged(bool b);
	void moonScaleChanged(double f);
	void flagMinorBodyScaleChanged(bool b);
	void minorBodyScaleChanged(double f);
	void flagPlanetScaleChanged(bool b);
	void planetScaleChanged(double f);
	void flagSunScaleChanged(bool b);
	void sunScaleChanged(double f);
	void labelsAmountChanged(double f);
	void ephemerisMarkersChanged(bool b);
	void ephemerisHorizontalCoordinatesChanged(bool b);
	void ephemerisDatesChanged(bool b);
	void ephemerisMagnitudesChanged(bool b);
	void ephemerisLineChanged(bool b);
	void ephemerisAlwaysOnChanged(bool b);
	void ephemerisNowChanged(bool b);
	void ephemerisLineThicknessChanged(int v);
	void ephemerisSkipDataChanged(bool b);
	void ephemerisSkipMarkersChanged(bool b);
	void ephemerisDataStepChanged(int s);
	void ephemerisDataLimitChanged(int s);
	void ephemerisSmartDatesChanged(bool b);
	void ephemerisScaleMarkersChanged(bool b);
	void grsLongitudeChanged(int l);
	void grsDriftChanged(double drift);
	void grsJDChanged(double JD);
	void earthShadowEnlargementDanjonChanged(bool b);
	void flagPermanentSolarCoronaChanged(bool b);

	void labelsColorChanged(const Vec3f & color);
	void pointerColorChanged(const Vec3f & color);
	void trailsColorChanged(const Vec3f & color);
	void orbitsColorChanged(const Vec3f & color);
	void nomenclatureColorChanged(const Vec3f & color);
	void majorPlanetsOrbitsColorChanged(const Vec3f & color);
	void minorPlanetsOrbitsColorChanged(const Vec3f & color);
	void dwarfPlanetsOrbitsColorChanged(const Vec3f & color);
	void moonsOrbitsColorChanged(const Vec3f & color);
	void cubewanosOrbitsColorChanged(const Vec3f & color);
	void plutinosOrbitsColorChanged(const Vec3f & color);
	void scatteredDiskObjectsOrbitsColorChanged(const Vec3f & color);
	void oortCloudObjectsOrbitsColorChanged(const Vec3f & color);
	void cometsOrbitsColorChanged(const Vec3f & color);
	void sednoidsOrbitsColorChanged(const Vec3f & color);
	void interstellarOrbitsColorChanged(const Vec3f & color);
	void mercuryOrbitColorChanged(const Vec3f & color);
	void venusOrbitColorChanged(const Vec3f & color);
	void earthOrbitColorChanged(const Vec3f & color);
	void marsOrbitColorChanged(const Vec3f & color);
	void jupiterOrbitColorChanged(const Vec3f & color);
	void saturnOrbitColorChanged(const Vec3f & color);
	void uranusOrbitColorChanged(const Vec3f & color);
	void neptuneOrbitColorChanged(const Vec3f & color);
	void ephemerisGenericMarkerColorChanged(const Vec3f & color);
	void ephemerisSecondaryMarkerColorChanged(const Vec3f & color);
	void ephemerisSelectedMarkerColorChanged(const Vec3f & color);
	void ephemerisMercuryMarkerColorChanged(const Vec3f & color);
	void ephemerisVenusMarkerColorChanged(const Vec3f & color);
	void ephemerisMarsMarkerColorChanged(const Vec3f & color);
	void ephemerisJupiterMarkerColorChanged(const Vec3f & color);
	void ephemerisSaturnMarkerColorChanged(const Vec3f & color);

	void orbitColorStyleChanged(const QString &style);
	void apparentMagnitudeAlgorithmOnEarthChanged(const QString &algorithm);

	void solarSystemDataReloaded();
	void requestEphemerisVisualization();

	void extraThreadsChanged(const int);

	void markerMagThresholdChanged(double m);

public:
	///////////////////////////////////////////////////////////////////////////
	// Other public methods
	//! Get a pointer to a Planet object.
	//! @param planetEnglishName the English name of the desired planet.
	//! @return The matching planet pointer if exists or Q_NULLPTR.
	PlanetP searchByEnglishName(const QString &planetEnglishName) const;

	PlanetP searchMinorPlanetByEnglishName(const QString &planetEnglishName) const;

	//! Get the Planet object pointer for the Sun.
	PlanetP getSun() const {return sun;}

	//! Get the Planet object pointer for the Earth.
	PlanetP getEarth() const {return earth;}

	//! Get the Planet object pointer for Earth's moon.
	PlanetP getMoon() const {return moon;}

	//! Determine if a lunar eclipse is close at hand?
	bool nearLunarEclipse() const;

	//! Get the list of all the planet english names
	QStringList getAllPlanetEnglishNames() const;

	//! Get the list of all the planet localized names
	QStringList getAllPlanetLocalizedNames() const;

	//! Get the list of all the minor planet common english names
	QStringList getAllMinorPlanetCommonEnglishNames() const;


	//! New 0.16: delete a planet from the solar system. Writes a warning to log if this is not a minor object.
	bool removeMinorPlanet(const QString &name);

	//! Determines relative amount of sun visible from the observer's position (first element) and the Planet object pointer for eclipsing celestial body (second element).
	//! Full sun is 1.0, fully covered sun is 0.0.
	//! In the unlikely event of multiple objects in front of the sun, only the largest will be reported.
	QPair<double, PlanetP> getSolarEclipseFactor(const StelCore *core) const;

	//! Opening angle of the bright Solar crescent, radians
	//! From: J. Meeus, Morsels IV, ch.15
	//! @param lunarSize: apparent Lunar angular size (radius or diameter), angular units of your preference
	//! @param solarSize: apparent Solar angular size (radius or diameter, resp.), same angular units
	//! @param eclipseMagnitude: covered fraction of the Solar diameter.
	static double getEclipseCrescentAngle(const double lunarSize, const double solarSize, const double eclipseMagnitude);

	//! Retrieve Radius of Umbra and Penumbra at the distance of the Moon.
	//! Returns a pair (umbra, penumbra) in (geocentric_arcseconds, AU, geometric_AU).
	//! * sizes in arcseconds are the usual result found as Bessel element in eclipse literature.
	//!   It includes scaling for effects of atmosphere either after Chauvenet (2%) or after Danjon. (see Espenak: 5000 Years Canon of Lunar Eclipses.)
	//! * sizes in AU are the same, converted back to AU in Lunar distance.
	//! * sizes in geometric_AU derived from pure geometrical evaluations without scalings applied.
	QPair<Vec3d,Vec3d> getEarthShadowRadiiAtLunarDistance() const;

	//! Compute the position and transform matrix for every element of the solar system.
	//! @param core the central StelCore instance
	//! @param dateJDE the Julian Day in JDE (Ephemeris Time or equivalent)	
	//! @param observerPlanet planet of the observer (Required for light travel time or aberration computation).
	void computePositions(StelCore *core, double dateJDE, PlanetP observerPlanet);

	//! Get the list of all the bodies of the solar system.	
	const QList<PlanetP>& getAllPlanets() const {return systemPlanets;}
	//! Get the list of all the bodies of the solar system.
	const QList<PlanetP>& getAllMinorBodies() const {return systemMinorBodies;}
	//! Get the list of all minor bodies names.
	const QStringList getMinorBodiesList() const { return minorBodies; }

private slots:
	//! Called when a new object is selected.
	void selectedObjectChange(StelModule::StelModuleSelectAction action);

	//! Called when the sky culture is updated.
	//! Loads native names of planets for a given sky culture.
	void updateSkyCulture(const StelSkyCulture& skyCulture);

	void loadCultureSpecificNames(const QJsonObject& data);

	//! Called following StelMainView::reloadShadersRequested
	void reloadShaders();

	//! Set flag which enabled the showing of ephemeris markers or not
	void setFlagEphemerisMarkers(bool b);
	//! Get the current value of the flag which enabled the showing of ephemeris markers or not
	bool getFlagEphemerisMarkers() const;

	//! Set flag which enabled the showing of ephemeris line between markers or not
	void setFlagEphemerisLine(bool b);
	//! Get the current value of the flag which enabled the showing of ephemeris line between markers or not
	bool getFlagEphemerisLine() const;

	//! Set flag which enables ephemeris lines and marks always on
	void setFlagEphemerisAlwaysOn(bool b);
	//! Get the current value of the flag which makes ephemeris lines and marks always on
	bool getFlagEphemerisAlwaysOn() const;

	//! Set flag, which enables ephemeris marks on position "now"
	void setFlagEphemerisNow(bool b);
	//! Get the current value of the flag which makes ephemeris marks on position "now"
	bool getFlagEphemerisNow() const;

	//! Set the thickness of ephemeris line
	void setEphemerisLineThickness(int v);
	//! Get the thickness of ephemeris line
	int getEphemerisLineThickness() const;

	//! Set flag which enabled the showing of ephemeris markers in horizontal coordinates or not
	void setFlagEphemerisHorizontalCoordinates(bool b);
	//! Get the current value of the flag which enabled the showing of ephemeris markers in horizontal coordinates or not
	bool getFlagEphemerisHorizontalCoordinates() const;

	//! Set flag which enable the showing the date near ephemeris markers or not
	void setFlagEphemerisDates(bool b);
	//! Get the current value of the flag which enable the showing the date near ephemeris markers or not
	bool getFlagEphemerisDates() const;

	//! Set flag which enable the showing the magnitude near ephemeris markers or not
	void setFlagEphemerisMagnitudes(bool b);
	//! Get the current value of the flag which enable the showing the magnitude near ephemeris markers or not
	bool getFlagEphemerisMagnitudes() const;

	//! Set flag which allow skipping dates near ephemeris markers
	void setFlagEphemerisSkipData(bool b);
	//! Get the current value of the flag which allow skipping dates near ephemeris markers
	bool getFlagEphemerisSkipData() const;

	//! Set flag which allow skipping the ephemeris markers without dates
	void setFlagEphemerisSkipMarkers(bool b);
	//! Get the current value of the flag which allow skipping the ephemeris markers without dates
	bool getFlagEphemerisSkipMarkers() const;

	//! Set flag which allow using smart format for dates near ephemeris markers
	void setFlagEphemerisSmartDates(bool b);
	//! Get the current value of the flag which allow using smart format for dates near ephemeris markers
	bool getFlagEphemerisSmartDates() const;

	//! Set flag which allow scaling the ephemeris markers
	void setFlagEphemerisScaleMarkers(bool b);
	//! Get the current value of the flag which allow scaling the ephemeris markers
	bool getFlagEphemerisScaleMarkers() const;

	//! Set the step of skip for date of ephemeris markers (and markers if it enabled)
	void setEphemerisDataStep(int step);
	//! Get the step of skip for date of ephemeris markers
	int getEphemerisDataStep() const;

	//! Set the limit for data: we computed ephemeris for 1, 2 or 5 celestial bodies
	void setEphemerisDataLimit(int limit);
	//! Get the limit of the data (how many celestial bodies was in computing of ephemeris)
	int getEphemerisDataLimit() const;

	void setEphemerisGenericMarkerColor(const Vec3f& c);
	Vec3f getEphemerisGenericMarkerColor(void) const;

	void setEphemerisSecondaryMarkerColor(const Vec3f& c);
	Vec3f getEphemerisSecondaryMarkerColor(void) const;

	void setEphemerisSelectedMarkerColor(const Vec3f& c);
	Vec3f getEphemerisSelectedMarkerColor(void) const;

	void setEphemerisMercuryMarkerColor(const Vec3f& c);
	Vec3f getEphemerisMercuryMarkerColor(void) const;

	void setEphemerisVenusMarkerColor(const Vec3f& c);
	Vec3f getEphemerisVenusMarkerColor(void) const;

	void setEphemerisMarsMarkerColor(const Vec3f& c);
	Vec3f getEphemerisMarsMarkerColor(void) const;

	void setEphemerisJupiterMarkerColor(const Vec3f& c);
	Vec3f getEphemerisJupiterMarkerColor(void) const;

	void setEphemerisSaturnMarkerColor(const Vec3f& c);
	Vec3f getEphemerisSaturnMarkerColor(void) const;

	//! Called when a new Hips survey has been loaded by the hips mgr.
	void onNewSurvey(HipsSurveyP survey);

	//! Taking the JD dates for each ephemeride and preparation the human readable dates according to the settings for dates
	void fillEphemerisDates();

	//! When some aspect of orbit drawing changes, update their configuration
	void reconfigureOrbits();
private:
	//! Search for SolarSystem objects which are close to the position given
	//! in earth equatorial position.
	//! @param v A position in earth equatorial position.
	//! @param core the StelCore object.
	//! @return a pointer to a StelObject if found, else Q_NULLPTR
	StelObjectP search(Vec3d v, const StelCore* core) const;

	//! Compute the transformation matrix for every elements of the solar system.
	//! observerPos is needed for light travel time computation.
	void computeTransMatrices(double dateJDE, const Vec3d& observerPos = Vec3d(0.));

	//! Draw a nice animated pointer around the object.
	void drawPointer(const StelCore* core);

	//! Draw ephemeris lines and markers
	void drawEphemerisItems(const StelCore* core);

	//! Draw a nice markers for ephemeris of objects.
	void drawEphemerisMarkers(const StelCore* core);

	//! Draw a line, who connected markers for ephemeris of objects.
	void drawEphemerisLine(const StelCore* core);

	//! Load planet data from the Solar System configuration files.
	//! This function attempts to load every possible instance of the
	//! Solar System configuration files in the file paths, falling back if a
	//! given path can't be loaded.
	void loadPlanets();

	//! Load planet data from the given file
	bool loadPlanets(const QString& filePath);

	Vec3f getEphemerisMarkerColor(int index) const;

	//! Calculate a color of Solar system bodies
	//! @param bV value of B-V color index
	static unsigned char BvToColorIndex(double bV);

	//! Used to count how many planets actually need shadow information
	int shadowPlanetCount;
	//! Used to track whether earth shadow enlargement shall be computed after Danjon (1951)
	bool earthShadowEnlargementDanjon;
	PlanetP sun;
	PlanetP moon;
	PlanetP earth;

	//! Set selected planets by englishName.
	//! @param englishName The planet name or "" to select no planet
	void setSelected(const QString& englishName);
	//! Set selected object from its pointer.
	void setSelected(PlanetP obj);
	//! Get selected object's pointer.
	PlanetP getSelected(void) const {return selected;}
	//! The currently selected planet.
	PlanetP selected;
	std::vector<PlanetP> selectedSSO; // More than one can be selected at a time

	// Allow enlargements of the planets. May be useful to highlight the planets in in overview plots
	// Separate Moon and minor body scale values. The latter make sense to zoom up and observe irregularly formed 3D objects like minor moons of the outer planets.
	bool flagMoonScale;
	double moonScale;
	bool flagMinorBodyScale;
	double minorBodyScale;
	bool flagPlanetScale;
	double planetScale;
	bool flagSunScale;
	double sunScale;

	QFont planetNameFont;

	//! The amount of planet labels (between 0 and 10).
	double labelsAmount;

	// Flag to follow the state of drawing of solar corona
	bool flagPermanentSolarCorona;

	//! List of all the bodies of the solar system.
	QList<PlanetP> systemPlanets;
	//! List of all the minor bodies of the solar system.
	QList<PlanetP> systemMinorBodies;

	// Master settings
	bool flagOrbits;
	bool flagLightTravelTime;
	bool flagUseObjModels;
	bool flagShowObjSelfShadows;

	//! The selection pointer texture.
	StelTextureSP texPointer;
	StelTextureSP texEphemerisMarker;
	StelTextureSP texEphemerisCometMarker;
	StelTextureSP texEphemerisNowMarker;

	bool flagShow;
	bool flagPointer;                           // show red cross selection pointer?
	//bool flagNativePlanetNames;                 // show native names for planets?
	bool flagIsolatedTrails;
	int numberIsolatedTrails;
	int maxTrailPoints;                         // limit trails to a manageable size.
	int maxTrailTimeExtent;
	int trailsThickness;
	bool flagIsolatedOrbits;
	bool flagPlanetsOrbits;				// Show orbits of the major planets, regardless of other orbit settings
	bool flagPlanetsOrbitsOnly;			// show orbits of the major planets only (no minor bodies in any case)
	bool flagOrbitsWithMoons;			// Show moon systems if planet orbits are displayed
	bool ephemerisMarkersDisplayed;
	bool ephemerisDatesDisplayed;
	bool ephemerisMagnitudesDisplayed;
	bool ephemerisHorizontalCoordinates;
	bool ephemerisLineDisplayed;
	bool ephemerisAlwaysOn;
	bool ephemerisNow;
	int ephemerisLineThickness;
	bool ephemerisSkipDataDisplayed;
	bool ephemerisSkipMarkersDisplayed;
	int ephemerisDataStep;				// How many days skip for dates near ephemeris markers (and the markers if it enabled)
	int ephemerisDataLimit;				// Number of celestial bodies in ephemeris data (how many celestial bodies was in computing of ephemeris)
	bool ephemerisSmartDatesDisplayed;
	bool ephemerisScaleMarkersDisplayed;
	Vec3f ephemerisGenericMarkerColor;
	Vec3f ephemerisSecondaryMarkerColor;
	Vec3f ephemerisSelectedMarkerColor;
	Vec3f ephemerisMercuryMarkerColor;
	Vec3f ephemerisVenusMarkerColor;
	Vec3f ephemerisMarsMarkerColor;
	Vec3f ephemerisJupiterMarkerColor;
	Vec3f ephemerisSaturnMarkerColor;

	class TrailGroup* allTrails;
	QSettings* conf;
	LinearFader trailFader;
	Vec3f trailsColor;
	Vec3f pointerColor;

	static const QMap<Planet::ApparentMagnitudeAlgorithm, QString> vMagAlgorithmMap;

	//QHash<QString, QString> planetNativeNamesMap, planetNativeNamesMeaningMap;
	QStringList minorBodies;

	// 0.16pre observation GZ: this list contains pointers to all orbit objects,
	// while the planets don't own their orbit objects.
	// Would it not be better to hand over the orbit object ownership to the Planet object?
	// This list could then be removed.
	// In case this was originally intended to provide some fast access for time-dependent computation with the same JD,
	// note that we must also always compensate to light time travel, so likely each computation has to be done twice,
	// with current JDE and JDE-lightTime(distance).
	QList<Orbit*> orbits;           // Pointers on created elliptical orbits. 0.16pre: WHY DO WE NEED THIS???

	//! Number of additional threads. This could be automatically derived, but for now we can experiment.
	int extraThreads;

	// BEGIN OF BLOCK RELATED TO MASS MARKER DISPLAY
	// Variables used for GL optimization when displaying little markers for the minor bodies.
	// These data structures were borrowed from StelSkyDrawer. However, we need only one color.
	// Maybe, to extend the idea, have several such Arrays for category-colored main belt, Jupiter Trojans, NEA, KBO etc.
	//! Vertex format for a minor body marker.
	//! Texture pos is stored in another separately.
	struct MarkerVertex {
		Vec2f pos;
		unsigned char color[4]; // can we remove that?
	};
	static_assert(sizeof(MarkerVertex) == 2*4+4, "Size of MarkerVertex must be 12 bytes");

	//! Buffer for storing the marker positions
	MarkerVertex* markerArray;
	//! Buffer for storing the texture coordinate array data.
	unsigned char* textureCoordArray;

	class QOpenGLShaderProgram* markerShaderProgram;
	struct MarkerShaderVars {
		int projectionMatrix;
		int texCoord;
		int pos;
		int color; // Can we remove that?
		int texture;
	};
	MarkerShaderVars markerShaderVars;

	//! Current number of sources stored in the buffer (still to display)
	unsigned int nbMarkers;
	std::unique_ptr<QOpenGLVertexArrayObject> vao;
	std::unique_ptr<QOpenGLBuffer> vbo;
	//! Binds actual VAO if it's supported, sets up the relevant state manually otherwise.
	void bindVAO();
	//! Sets the vertex attribute states for the currently bound VAO so that glDraw* commands can work.
	void setupCurrentVAO();
	//! Binds zero VAO if VAO is supported, manually disables the relevant vertex attributes otherwise.
	void releaseVAO();

	//! Maximum number of markers which can be stored in the buffers
	constexpr static unsigned int maxMarkers=2048;
	void postDrawAsteroidMarkers(StelPainter *sPainter);
	StelTextureSP markerCircleTex; // An optional marker to have "something" in the sky even if object not visible.
	LinearFader markerFader;         // Useful for markers displayed for minor bodies regardless of magnitude

public:
	bool drawAsteroidMarker(StelCore* core, StelPainter* sPainter, const float x, const float y, Vec3f &color);
	float getMarkerValue() const {return markerFader.getInterstate();}

private:
	//! absolute value of the dimmest SSO drawn by drawAsteroidMarker()
	double markerMagThreshold;
	//! Preliminary experimental select-per-config.ini.
	//! 0: original single-threaded 3 loops
	//! 1: blockingMap
	//! 2: strided pool threads plus main thread
	//! 3: Ruslan's 1-pass loop (suspected to have a logical error.)
	//! To configure the experimental new solution, add [devel]/compute_positions_algorithm=0|1|2|3. Default=2
	int computePositionsAlgorithm;
	// END OF BLOCK RELATED TO MASS MARKER DISPLAY
};


#endif // SOLARSYSTEM_HPP

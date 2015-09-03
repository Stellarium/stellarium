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

#ifndef _SOLARSYSTEM_HPP_
#define _SOLARSYSTEM_HPP_
//sun is already defined in Sun C/Solaris
#if defined(sun)
#undef sun
#endif

#include "StelObjectModule.hpp"
#include "StelTextureTypes.hpp"
#include "Planet.hpp"

#include <QFont>

class Orbit;
class StelTranslator;
class StelObject;
class StelCore;
class StelProjector;
class QSettings;

typedef QSharedPointer<Planet> PlanetP;

//! @class SolarSystem
//! This StelObjectModule derivative is used to model SolarSystem bodies.
//! This includes the Major Planets, Minor Planets and Comets.
class SolarSystem : public StelObjectModule
{
	Q_OBJECT
	Q_PROPERTY(bool labelsDisplayed
		   READ getFlagLabels
		   WRITE setFlagLabels)
	Q_PROPERTY(bool orbitsDisplayed
		   READ getFlagOrbits
		   WRITE setFlagOrbits)
	Q_PROPERTY(bool trailsDisplayed
		   READ getFlagTrails
		   WRITE setFlagTrails)
	Q_PROPERTY(bool planetsDisplayed
		   READ getFlagPlanets
		   WRITE setFlagPlanets)
	Q_PROPERTY(bool hintsDisplayed
		   READ getFlagHints
		   WRITE setFlagHints)
	Q_PROPERTY(bool markersDisplayed
		   READ getFlagMarkers
		   WRITE setFlagMarkers)
	Q_PROPERTY(bool nativeNamesDisplayed
		   READ getFlagNativeNames
		   WRITE setFlagNativeNames)

public:
	SolarSystem();
	virtual ~SolarSystem();

	///////////////////////////////////////////////////////////////////////////
	// Methods defined in the StelModule class
	//! Initialize the SolarSystem.
	//! Includes:
	//! - loading planetary body orbital and model data from data/ssystem.ini
	//! - perform initial planet position calculation
	//! - set display options from application settings
	virtual void init();

	virtual void deinit();
	
	//! Draw SolarSystem objects (planets).
	//! @param core The StelCore object.
	//! @return The maximum squared distance in pixels that any SolarSystem object
	//! has travelled since the last update.
	virtual void draw(StelCore *core);

	//! Update time-varying components.
	//! This includes planet motion trails.
	virtual void update(double deltaTime);

	//! Used to determine what order to draw the various StelModules.
	virtual double getCallOrder(StelModuleActionName actionName) const;

	///////////////////////////////////////////////////////////////////////////
	// Methods defined in StelObjectManager class
	//! Search for SolarSystem objects in some area around a point.
	//! @param v A vector representing a point in the sky.
	//! @param limitFov The radius of the circle around the point v which
	//! defines the size of the area to search.
	//! @param core the core object
	//! @return A STL vector of StelObjectP (pointers) containing all SolarSystem
	//! objects found in the specified area. This vector is not sorted by distance
	//! from v.
	virtual QList<StelObjectP> searchAround(const Vec3d& v, double limitFov, const StelCore* core) const;

	//! Search for a SolarSystem object based on the localised name.
	//! @param nameI18n the case in-sensistive translated planet name.
	//! @return a StelObjectP for the object if found, else NULL.
	virtual StelObjectP searchByNameI18n(const QString& nameI18n) const;

	//! Search for a SolarSystem object based on the English name.
	//! @param name the case in-sensistive English planet name.
	//! @return a StelObjectP for the object if found, else NULL.
	virtual StelObjectP searchByName(const QString& name) const;

	//! Find objects by translated name prefix.
	//! Find and return the list of at most maxNbItem objects auto-completing
	//! the passed object I18n name.
	//! @param objPrefix the case insensitive first letters of the searched object.
	//! @param maxNbItem the maximum number of returned object names.
	//! @param useStartOfWords the autofill mode for returned objects names.
	//! @return a list of matching object name by order of relevance, or an empty list if nothing matches.
	virtual QStringList listMatchingObjectsI18n(const QString& objPrefix, int maxNbItem=5, bool useStartOfWords=false) const;
	//! Find objects by translated name prefix.
	//! Find and return the list of at most maxNbItem objects auto-completing
	//! the passed object English name.
	//! @param objPrefix the case insensitive first letters of the searched object.
	//! @param maxNbItem the maximum number of returned object names.
	//! @param useStartOfWords the autofill mode for returned objects names.
	//! @return a list of matching object name by order of relevance, or an empty list if nothing matches.
	virtual QStringList listMatchingObjects(const QString& objPrefix, int maxNbItem=5, bool useStartOfWords=false) const;
	virtual QStringList listAllObjects(bool inEnglish) const { Q_UNUSED(inEnglish) return QStringList(); }
	virtual QStringList listAllObjectsByType(const QString& objType, bool inEnglish) const;
	virtual QString getName() const { return "Solar System"; }

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

	//! Set flag which determines if planet hints are drawn or hidden along labels
	void setFlagHints(bool b);
	//! Get the current value of the flag which determines if planet hints are drawn or hidden along labels
	bool getFlagHints() const;

	//! Set flag which determines if planet labels are drawn or hidden.
	void setFlagLabels(bool b);
	//! Get the current value of the flag which determines if planet labels are drawn or hidden.
	bool getFlagLabels() const;

	//! Set the amount of planet labels. The real amount is also proportional with FOV.
	//! The limit is set in function of the planets magnitude
	//! @param a the amount between 0 and 10. 0 is no labels, 10 is maximum of labels
	void setLabelsAmount(float a) {labelsAmount=a;}
	//! Get the amount of planet labels. The real amount is also proportional with FOV.
	//! @return the amount between 0 and 10. 0 is no labels, 10 is maximum of labels
	float getLabelsAmount(void) const {return labelsAmount;}

	//! Set flag which determines if planet orbits are drawn or hidden.
	void setFlagOrbits(bool b);
	//! Get the current value of the flag which determines if planet orbits are drawn or hidden.
	bool getFlagOrbits() const {return flagOrbits;}

	//! Set flag which determines if planet markers are drawn or hidden.
	void setFlagMarkers(bool b) { flagMarker=b; }
	//! Get the current value of the flag which determines if planet markers are drawn or hidden.
	bool getFlagMarkers() const {return flagMarker;}

	//! Set flag which determines if the light travel time calculation is used or not.
	void setFlagLightTravelTime(bool b);
	//! Get the current value of the flag which determines if light travel time
	//! calculation is used or not.
	bool getFlagLightTravelTime(void) const {return flagLightTravelTime;}

	//! Set planet names font size.
	void setFontSize(float newFontSize);

	//! Set the color used to draw planet labels.
	//! @param c The color of the planet labels
	//! @code
	//! // example of usage in scripts
	//! SolarSystem.setLabelsColor(Vec3f(1.0,0.0,0.0));
	//! @endcode
	void setLabelsColor(const Vec3f& c);
	//! Get the current color used to draw planet labels.
	const Vec3f& getLabelsColor(void) const;

	//! Set the color used to draw planet orbit lines.
	//! @param c The color of the planet orbit lines
	//! @code
	//! // example of usage in scripts
	//! SolarSystem.setOrbitsColor(Vec3f(1.0,0.0,0.0));
	//! @endcode
	void setOrbitsColor(const Vec3f& c);
	//! Get the current color used to draw planet orbit lines.
	Vec3f getOrbitsColor(void) const;

	//! Set the color used to draw planet trails lines.
	//! @param c The color of the planet trails lines
	//! @code
	//! // example of usage in scripts
	//! SolarSystem.setTrailsColor(Vec3f(1.0,0.0,0.0));
	//! @endcode
	void setTrailsColor(const Vec3f& c) {trailColor=c;}
	//! Get the current color used to draw planet trails lines.
	Vec3f getTrailsColor() const {return trailColor;}

	//! Set the color used to draw planet pointers.
	//! @param c The color of the planet pointers
	//! @code
	//! // example of usage in scripts
	//! SolarSystem.setPointersColor(Vec3f(1.0,0.0,0.0));
	//! @endcode
	void setPointersColor(const Vec3f& c) {pointerColor=c;}
	//! Get the current color used to draw planet pointers.
	Vec3f getPointersColor() const {return pointerColor;}

	//! Set flag which determines if Earth's moon is scaled or not.
	void setFlagMoonScale(bool b);
	//! Get the current value of the flag which determines if Earth's moon is scaled or not.
	bool getFlagMoonScale(void) const {return flagMoonScale;}

	//! Set the display scaling factor for Earth's moon.
	void setMoonScale(double f);
	//! Get the display scaling factor for Earth's oon.
	float getMoonScale(void) const {return moonScale;}

	//! Translate names. (public so that SolarSystemEditor can call it).
	void updateI18n();

	//! Get the V magnitude for Solar system bodies from scripts
	//! @param planetName the case in-sensistive English planet name.
	//! @param withExtinction the flag for use extinction effect for magnitudes (default not use)
	//! @return a magnitude
	float getPlanetVMagnitude(QString planetName, bool withExtinction=false) const;

	//! Get type for Solar system bodies from scripts
	//! @param planetName the case in-sensistive English planet name.
	//! @return a type of planet (planet, moon, asteroid, comet, plutoid)
	QString getPlanetType(QString planetName) const;

	//! Get distance to Solar system bodies from scripts
	//! @param planetName the case in-sensistive English planet name.
	//! @return a distance (in AU)
	double getDistanceToPlanet(QString planetName) const;

	//! Get elongation for Solar system bodies from scripts
	//! @param planetName the case in-sensistive English planet name.
	//! @return a elongation (in radians)
	double getElongationForPlanet(QString planetName) const;

	//! Get phase angle for Solar system bodies from scripts
	//! @param planetName the case in-sensistive English planet name.
	//! @return a phase angle (in radians)
	double getPhaseAngleForPlanet(QString planetName) const;

	//! Get phase for Solar system bodies from scripts
	//! @param planetName the case in-sensistive English planet name.
	//! @return a phase
	float getPhaseForPlanet(QString planetName) const;

	//! Set the algorithm for computation of apparent magnitudes for planets in case observer on the Earth.
	//! Possible values:
	//! * Planesas (algorithm provided by Pere Planesas (Observatorio Astronomico Nacional))
	//! * Mueller (G. Mueller, based on visual observations 1877-91. [Expl.Suppl.1961])
	//! * Harris (Astronomical Almanac 1984 and later. These give V (instrumental) magnitudes)
	//! Details:
	//! J. Meeus "Astronomical Algorithms" (2nd ed., with corrections as of August 10, 2009) p.283-286.
	//! O. Montenbruck, T. Pfleger "Astronomy on the Personal Computer" (4th ed.) p.143-145.
	//! Daniel L. Harris "Photometry and Colorimetry of Planets and Satellites" http://adsabs.harvard.edu/abs/1961plsa.book..272H
	//! Hint: Default option in config.ini: astro/apparent_magnitude_algorithm = Harris
	//! @param algorithm the case in-sensitive algorithm name
	//! @note: The structure of algorithms is almost identical, just the numbers are different! You should activate
	//! Mueller's algorithm for simulate the eye's impression. (Esp. Venus!)
	void setApparentMagnitudeAlgorithmOnEarth(QString algorithm);

	//! Get the algorithm used for computation of apparent magnitudes for planets in case  observer on the Earth
	QString getApparentMagnitudeAlgorithmOnEarth() const;

	//! Set flag which enable use native names for planets or not.
	void setFlagNativeNames(bool b);
	//! Get the current value of the flag which enables showing native names for planets or not.
	bool getFlagNativeNames(void) const;

	//! Set flag which enable use translated names for planets or not.
	void setFlagTranslatedNames(bool b);
	//! Get the current value of the flag which enables showing translated names for planets or not.
	bool getFlagTranslatedNames(void) const;

	//! Set flag which enabled the showing of isolated trails for selected objects only or not
	void setFlagIsolatedTrails(bool b);
	//! Get the current value of the flag which enables showing of isolated trails for selected objects only or not.
	bool getFlagIsolatedTrails(void) const;

	//! Set flag which enabled the showing of isolated orbits for selected objects only or not
	void setFlagIsolatedOrbits(bool b);
	//! Get the current value of the flag which enables showing of isolated orbits for selected objects only or not.
	bool getFlagIsolatedOrbits(void) const;

public:
	///////////////////////////////////////////////////////////////////////////
	// Other public methods
	//! Get a pointer to a Planet object.
	//! @param planetEnglishName the English name of the desired planet.
	//! @return The matching planet pointer if exists or NULL.
	PlanetP searchByEnglishName(QString planetEnglishName) const;

	//! Get the Planet object pointer for the Sun.
	PlanetP getSun() const {return sun;}

	//! Get the Planet object pointer for the Earth.
	PlanetP getEarth() const {return earth;}

	//! Get the Planet object pointer for Earth's moon.
	PlanetP getMoon() const {return moon;}

	//! Determine if a lunar eclipse is close at hand?
	bool nearLunarEclipse();

	//! Get the list of all the planet english names
	QStringList getAllPlanetEnglishNames() const;

	//! Get the list of all the planet localized names
	QStringList getAllPlanetLocalizedNames() const;

	//! Reload the planets
	void reloadPlanets();

	///////////////////////////////////////////////////////////////////////////////////////
	// DEPRECATED
	///////////////////////////////////////////////////////////////////////////////////////
	//! Get a hash of locale and ssystem.ini names for use with the TUI.
	//! @return A newline delimited hash of localized:standard planet names.
	//! Planet translated name is PARENT : NAME
	//! \deprecated ???
	QString getPlanetHashString();

	//! Compute the position and transform matrix for every element of the solar system.
	//! @param observerPos Position of the observer in heliocentric ecliptic frame (Required for light travel time computation).
	//! @param dateJDE the Julian Day in JDE (Ephemeris Time or equivalent)
	//! \deprecated ??? In the "deprecated" section, but used in SolarSystem::init()
	void computePositions(double dateJDE, const Vec3d& observerPos = Vec3d(0.));

	//! Get the list of all the bodies of the solar system.
	//! \deprecated Used in LandscapeMgr::update(), but commented out.
	const QList<PlanetP>& getAllPlanets() const {return systemPlanets;}	

private slots:
	//! Called when a new object is selected.
	void selectedObjectChange(StelModule::StelModuleSelectAction action);

	//! Load a color scheme
	void setStelStyle(const QString& section);

	//! Called when the sky culture is updated.
	//! Loads native names of planets for a given sky culture.
	//! @param skyCultureDir the name of the directory containing the sky culture to use.
	void updateSkyCulture(const QString& skyCultureDir);

private:
	//! Search for SolarSystem objects which are close to the position given
	//! in earth equatorial position.
	//! @param v A position in earth equatorial position.
	//! @param core the StelCore object.
	//! @return a pointer to a StelObject if found, else NULL
	StelObjectP search(Vec3d v, const StelCore* core) const;

	//! Compute the transformation matrix for every elements of the solar system.
	//! observerPos is needed for light travel time computation.
	void computeTransMatrices(double dateJDE, const Vec3d& observerPos = Vec3d(0.));

	//! Draw a nice animated pointer around the object.
	void drawPointer(const StelCore* core);

	//! Load planet data from the Solar System configuration file.
	//! This function attempts to load every possible instance of the
	//! Solar System configuration file in the file paths, falling back if a
	//! given path can't be loaded.
	void loadPlanets();

	//! Load planet data from the given file
	bool loadPlanets(const QString& filePath);

	void recreateTrails();

	//! Used to count how many planets actually need shadow information
	int shadowPlanetCount;
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

	// Moon scale value
	bool flagMoonScale;
	float moonScale;

	QFont planetNameFont;

	//! The amount of planets labels (between 0 and 10).
	float labelsAmount;

	//! List of all the bodies of the solar system.
	QList<PlanetP> systemPlanets;

	// Master settings
	bool flagOrbits;
	bool flagLightTravelTime;

	//! The selection pointer texture.
	StelTextureSP texPointer;

	bool flagShow;
	bool flagMarker;
	bool flagNativeNames;
	bool flagTranslatedNames;
	bool flagIsolatedTrails;
	bool flagIsolatedOrbits;

	class TrailGroup* allTrails;
	LinearFader trailFader;
	Vec3f trailColor;
	Vec3f pointerColor;

	QHash<QString, QString> planetNativeNamesMap;

	//////////////////////////////////////////////////////////////////////////////////
	// DEPRECATED
	//////////////////////////////////////////////////////////////////////////////////
	QList<Orbit*> orbits;           // Pointers on created elliptical orbits
};


#endif // _SOLARSYSTEM_HPP_

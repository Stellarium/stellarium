/*
 * Stellarium
 * Copyright (C) 2002 Fabien Chereau
 * Copyright (c) 2010 Bogdan Marinov
 * Copyright (c) 2012 Timothy Reaves
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

#include <QFont>
#include "StelObjectModule.hpp"
#include "StelTextureTypes.hpp"
#include "Planet.hpp"

class Orbit;
class StelTranslator;
class StelObject;
class StelCore;
class StelProjector;
class StelNavigator;
class QSettings;

typedef QSharedPointer<Planet> PlanetP;

//! @class SolarSystem
//! This StelObjectModule derivative is used to model SolarSystem bodies.
//! This includes the Major Planets, Minor Planets and Comets.
class SolarSystem : public StelObjectModule
{
	Q_OBJECT
	Q_PROPERTY(float fontSize
			   READ getFontSize
			   WRITE setFontSize
			   NOTIFY fontSizeChanged)
	Q_PROPERTY(bool hintsDisplayed
			   READ isHintsDisplayed
			   WRITE setHintsDisplayed
			   NOTIFY hintsDisplayedChanged)
	Q_PROPERTY(Vec3f labelsColor
			   READ getLabelsColor
			   WRITE setLabelsColor
			   NOTIFY labelsColorChanged)
	Q_PROPERTY(bool labelsDisplayed
			   READ isLabelsDisplayed
			   WRITE setLabelsDisplayed
			   NOTIFY labelsDisplayedChanged)
	Q_PROPERTY(float moonScale
			   READ getMoonScale
			   WRITE setMoonScale
			   NOTIFY moonScaleChanged)
	Q_PROPERTY(bool moonScaled
			   READ isMoonScaled
			   WRITE setMoonScaled
			   NOTIFY moonScaledChanged)
	Q_PROPERTY(Vec3f orbitsColor
			   READ getOrbitsColor
			   WRITE setOrbitsColor
			   NOTIFY orbitsColorChanged)
	Q_PROPERTY(bool orbitsDisplayed
			   READ isOrbitsDisplayed
			   WRITE setOrbitsDisplayed
			   NOTIFY orbitsDisplayedChanged)
	Q_PROPERTY(bool planetsDisplayed
			   READ isPlanetsDisplayed
			   WRITE setPlanetsDisplayed
			   NOTIFY planetsDisplayedChanged)
	Q_PROPERTY(Vec3f trailsColor
			   READ getTrailsColor
			   WRITE setTrailsColor
			   NOTIFY trailsColorChanged)
	Q_PROPERTY(bool trailsDisplayed
			   READ isTrailsDisplayed
			   WRITE setTrailsDisplayed
			   NOTIFY trailsDisplayedChanged)

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
	//! @return a list of matching object name by order of relevance, or an empty list if nothing matches.
	virtual QStringList listMatchingObjectsI18n(const QString& objPrefix, int maxNbItem=5) const;

public slots:
	///////////////////////////////////////////////////////////////////////////
	// Method callable from script and GUI
	// Properties setters and getters
	//! Set flag which determines if planets are drawn or hidden.
	void setPlanetsDisplayed(const bool displayed);
	//! Get the current value of the flag which determines if planet are drawn or hidden.
	bool isPlanetsDisplayed() const;

	//! Set flag which determines if planet trails are drawn or hidden.
	void setTrailsDisplayed(const bool displayed);
	//! Get the current value of the flag which determines if planet trails are drawn or hidden.
	bool isTrailsDisplayed() const;

	//! Set flag which determines if planet hints are drawn or hidden along labels
	void setHintsDisplayed(const bool displayed);
	//! Get the current value of the flag which determines if planet hints are drawn or hidden along labels
	bool isHintsDisplayed() const;

	//! Set flag which determines if planet labels are drawn or hidden.
	void setLabelsDisplayed(const bool displayed);
	//! Get the current value of the flag which determines if planet labels are drawn or hidden.
	bool isLabelsDisplayed() const;

	//! Set the amount of planet labels. The real amount is also proportional with FOV.
	//! The limit is set in function of the planets magnitude
	//! @param a the amount between 0 and 10. 0 is no labels, 10 is maximum of labels
	void setLabelsAmount(const float a);
	//! Get the amount of planet labels. The real amount is also proportional with FOV.
	//! @return the amount between 0 and 10. 0 is no labels, 10 is maximum of labels
	float getLabelsAmount(void) const;

	//! Set flag which determines if planet orbits are drawn or hidden.
	void setOrbitsDisplayed(const bool displayed);
	//! Get the current value of the flag which determines if planet orbits are drawn or hidden.
	bool isOrbitsDisplayed() const;

	//! Set flag which determines if the light travel time calculation is used or not.
	void setFlagLightTravelTime(const bool displayed);
	//! Get the current value of the flag which determines if light travel time
	//! calculation is used or not.
	bool getFlagLightTravelTime(void) const {return flagLightTravelTime;}

	//! Set planet names font size.
	void setFontSize(const float newFontSize);
	float getFontSize() const;

	//! Set the color used to draw planet labels.
	void setLabelsColor(const Vec3f& color);
	//! Get the current color used to draw planet labels.
	const Vec3f& getLabelsColor(void) const;

	//! Set the color used to draw planet orbit lines.
	void setOrbitsColor(const Vec3f& color);
	//! Get the current color used to draw planet orbit lines.
	Vec3f getOrbitsColor(void) const;

	//! Set the color used to draw planet trails lines.
	void setTrailsColor(const Vec3f& color);
	//! Get the current color used to draw planet trails lines.
	Vec3f getTrailsColor() const;

	//! Set flag which determines if Earth's moon is scaled or not.
	void setMoonScaled(const bool scaled);
	//! Get the current value of the flag which determines if Earth's moon is scaled or not.
	bool isMoonScaled(void) const;

	//! Set the display scaling factor for Earth's moon.
	void setMoonScale(const float factor);
	//! Get the display scaling factor for Earth's oon.
	float getMoonScale(void) const;

	//! Translate names. (public so that SolarSystemEditor can call it).
	void updateI18n();

signals:
	void fontSizeChanged(const float newSize);
	void hintsDisplayedChanged(const bool displayed);
	void labelsColorChanged(const Vec3f & newColor);
	void labelsDisplayedChanged(const bool displayed);
	void moonScaleChanged(const float newScale);
	void moonScaledChanged(const bool scaled);
	void orbitsColorChanged(const Vec3f & newColor);
	void orbitsDisplayedChanged(const bool displayed);
	void planetsDisplayedChanged(const bool displayed);
	void trailsColorChanged(const Vec3f & newColor);
	void trailsDisplayedChanged(const bool displayed);


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
	//! @param date the date in JDay
	//! \deprecated ??? In the "deprecated" section, but used in SolarSystem::init()
	//! and StelNavigator::updateTime()
	void computePositions(double date, const Vec3d& observerPos = Vec3d(0.));

	//! Get the list of all the bodies of the solar system.
	//! \deprecated Used in LandscapeMgr::update(), but commented out.
	const QList<PlanetP>& getAllPlanets() const {return systemPlanets;}

private slots:
	//! Called when a new object is selected.
	void selectedObjectChange(StelModule::StelModuleSelectAction action);

	//! Load a color scheme
	void setStelStyle(const QString& section);


private:
	//! Search for SolarSystem objects which are close to the position given
	//! in earth equatorial position.
	//! @param v A position in earth equatorial position.
	//! @param core the StelCore object.
	//! @return a pointer to a StelObject if found, else NULL
	StelObjectP search(Vec3d v, const StelCore* core) const;

	//! Compute the transformation matrix for every elements of the solar system.
	//! observerPos is needed for light travel time computation.
	void computeTransMatrices(double date, const Vec3d& observerPos = Vec3d(0.));

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

	bool hintsDisplayed;
	bool labelsDisplayed;

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

	class TrailGroup* allTrails;
	LinearFader trailFader;
	Vec3f trailColor;

	//////////////////////////////////////////////////////////////////////////////////
	// DEPRECATED
	//////////////////////////////////////////////////////////////////////////////////
	QList<Orbit*> orbits;           // Pointers on created elliptical orbits
};


#endif // _SOLARSYSTEM_HPP_

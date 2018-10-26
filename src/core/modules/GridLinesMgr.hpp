/*
 * Stellarium
 * Copyright (C) 2007 Fabien Chereau
 * Copyright (C) 2015 Georg Zotti (more grids to illustrate precession issues)
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

#ifndef GRIDLINESMGR_HPP
#define GRIDLINESMGR_HPP

#include "VecMath.hpp"
#include "StelModule.hpp"
#include "Planet.hpp"

class SkyGrid;
class SkyLine;
class SkyPoint;

//! @class GridLinesMgr
//! The GridLinesMgr controls the drawing of the Azimuthal, Equatorial, Ecliptical and Galactic Grids,
//! as well as the great circles: Meridian Line, Ecliptic Lines of J2000.0 and date, Equator Line (of J2000.0 and date),
//! Precession Circles, and a special line marking conjunction or opposition in ecliptical longitude (of date).
class GridLinesMgr : public StelModule
{
	Q_OBJECT
	Q_PROPERTY(bool gridlinesDisplayed
		   READ getFlagGridlines
		   WRITE setFlagGridlines
		   NOTIFY gridlinesDisplayedChanged)

	Q_PROPERTY(bool azimuthalGridDisplayed
		   READ getFlagAzimuthalGrid
		   WRITE setFlagAzimuthalGrid
		   NOTIFY azimuthalGridDisplayedChanged)
	Q_PROPERTY(Vec3f azimuthalGridColor
		   READ getColorAzimuthalGrid
		   WRITE setColorAzimuthalGrid
		   NOTIFY azimuthalGridColorChanged)

	Q_PROPERTY(bool equatorGridDisplayed
		   READ getFlagEquatorGrid
		   WRITE setFlagEquatorGrid
		   NOTIFY equatorGridDisplayedChanged)
	Q_PROPERTY(Vec3f equatorGridColor
		   READ getColorEquatorGrid
		   WRITE setColorEquatorGrid
		   NOTIFY equatorGridColorChanged)

	Q_PROPERTY(bool equatorJ2000GridDisplayed
		   READ getFlagEquatorJ2000Grid
		   WRITE setFlagEquatorJ2000Grid
		   NOTIFY equatorJ2000GridDisplayedChanged)
	Q_PROPERTY(Vec3f equatorJ2000GridColor
		   READ getColorEquatorJ2000Grid
		   WRITE setColorEquatorJ2000Grid
		   NOTIFY equatorJ2000GridColorChanged)

	Q_PROPERTY(bool eclipticJ2000GridDisplayed
		   READ getFlagEclipticJ2000Grid
		   WRITE setFlagEclipticJ2000Grid
		   NOTIFY eclipticJ2000GridDisplayedChanged)
	Q_PROPERTY(Vec3f eclipticJ2000GridColor
		   READ getColorEclipticJ2000Grid
		   WRITE setColorEclipticJ2000Grid
		   NOTIFY eclipticJ2000GridColorChanged)

	Q_PROPERTY(bool eclipticGridDisplayed
		   READ getFlagEclipticGrid
		   WRITE setFlagEclipticGrid
		   NOTIFY eclipticGridDisplayedChanged)
	Q_PROPERTY(Vec3f eclipticGridColor
		   READ getColorEclipticGrid
		   WRITE setColorEclipticGrid
		   NOTIFY eclipticGridColorChanged)

	Q_PROPERTY(bool galacticGridDisplayed
		   READ getFlagGalacticGrid
		   WRITE setFlagGalacticGrid
		   NOTIFY galacticGridDisplayedChanged)
	Q_PROPERTY(Vec3f galacticGridColor
		   READ getColorGalacticGrid
		   WRITE setColorGalacticGrid
		   NOTIFY galacticGridColorChanged)

	Q_PROPERTY(bool supergalacticGridDisplayed
		   READ getFlagSupergalacticGrid
		   WRITE setFlagSupergalacticGrid
		   NOTIFY supergalacticGridDisplayedChanged)
	Q_PROPERTY(Vec3f supergalacticGridColor
		   READ getColorSupergalacticGrid
		   WRITE setColorSupergalacticGrid
		   NOTIFY supergalacticGridColorChanged)

	Q_PROPERTY(bool equatorLineDisplayed
		   READ getFlagEquatorLine
		   WRITE setFlagEquatorLine
		   NOTIFY equatorLineDisplayedChanged)
	Q_PROPERTY(Vec3f equatorLineColor
		   READ getColorEquatorLine
		   WRITE setColorEquatorLine
		   NOTIFY equatorLineColorChanged)

	Q_PROPERTY(bool equatorJ2000LineDisplayed
		   READ getFlagEquatorJ2000Line
		   WRITE setFlagEquatorJ2000Line
		   NOTIFY equatorJ2000LineDisplayedChanged)
	Q_PROPERTY(Vec3f equatorJ2000LineColor
		   READ getColorEquatorJ2000Line
		   WRITE setColorEquatorJ2000Line
		   NOTIFY equatorJ2000LineColorChanged)

	// This is now ecl. of date.
	Q_PROPERTY(bool eclipticLineDisplayed
		   READ getFlagEclipticLine
		   WRITE setFlagEclipticLine
		   NOTIFY eclipticLineDisplayedChanged)
	Q_PROPERTY(Vec3f eclipticLineColor
		   READ getColorEclipticLine
		   WRITE setColorEclipticLine
		   NOTIFY eclipticLineColorChanged)

	// new name, but replaces old ecliptic.
	Q_PROPERTY(bool eclipticJ2000LineDisplayed
		   READ getFlagEclipticJ2000Line
		   WRITE setFlagEclipticJ2000Line
		   NOTIFY eclipticJ2000LineDisplayedChanged)
	Q_PROPERTY(Vec3f eclipticJ2000LineColor
		   READ getColorEclipticJ2000Line
		   WRITE setColorEclipticJ2000Line
		   NOTIFY eclipticJ2000LineColorChanged)

	Q_PROPERTY(bool precessionCirclesDisplayed
		   READ getFlagPrecessionCircles
		   WRITE setFlagPrecessionCircles
		   NOTIFY precessionCirclesDisplayedChanged)
	Q_PROPERTY(Vec3f precessionCirclesColor
		   READ getColorPrecessionCircles
		   WRITE setColorPrecessionCircles
		   NOTIFY precessionCirclesColorChanged)

	Q_PROPERTY(bool meridianLineDisplayed
		   READ getFlagMeridianLine
		   WRITE setFlagMeridianLine
		   NOTIFY meridianLineDisplayedChanged)
	Q_PROPERTY(Vec3f meridianLineColor
		   READ getColorMeridianLine
		   WRITE setColorMeridianLine
		   NOTIFY meridianLineColorChanged)

	Q_PROPERTY(bool longitudeLineDisplayed
		   READ getFlagLongitudeLine
		   WRITE setFlagLongitudeLine
		   NOTIFY longitudeLineDisplayedChanged)
	Q_PROPERTY(Vec3f longitudeLineColor
		   READ getColorLongitudeLine
		   WRITE setColorLongitudeLine
		   NOTIFY longitudeLineColorChanged)

	Q_PROPERTY(bool horizonLineDisplayed
		   READ getFlagHorizonLine
		   WRITE setFlagHorizonLine
		   NOTIFY horizonLineDisplayedChanged)
	Q_PROPERTY(Vec3f horizonLineColor
		   READ getColorHorizonLine
		   WRITE setColorHorizonLine
		   NOTIFY horizonLineColorChanged)

	Q_PROPERTY(bool galacticEquatorLineDisplayed
		   READ getFlagGalacticEquatorLine
		   WRITE setFlagGalacticEquatorLine
		   NOTIFY galacticEquatorLineDisplayedChanged)
	Q_PROPERTY(Vec3f galacticEquatorLineColor
		   READ getColorGalacticEquatorLine
		   WRITE setColorGalacticEquatorLine
		   NOTIFY galacticEquatorLineColorChanged)

	Q_PROPERTY(bool supergalacticEquatorLineDisplayed
		   READ getFlagSupergalacticEquatorLine
		   WRITE setFlagSupergalacticEquatorLine
		   NOTIFY supergalacticEquatorLineDisplayedChanged)
	Q_PROPERTY(Vec3f supergalacticEquatorLineColor
		   READ getColorSupergalacticEquatorLine
		   WRITE setColorSupergalacticEquatorLine
		   NOTIFY supergalacticEquatorLineColorChanged)

	Q_PROPERTY(bool primeVerticalLineDisplayed
		   READ getFlagPrimeVerticalLine
		   WRITE setFlagPrimeVerticalLine
		   NOTIFY primeVerticalLineDisplayedChanged)
	Q_PROPERTY(Vec3f primeVerticalLineColor
		   READ getColorPrimeVerticalLine
		   WRITE setColorPrimeVerticalLine
		   NOTIFY primeVerticalLineColorChanged)

	Q_PROPERTY(bool colureLinesDisplayed
		   READ getFlagColureLines
		   WRITE setFlagColureLines
		   NOTIFY colureLinesDisplayedChanged)
	Q_PROPERTY(Vec3f colureLinesColor
		   READ getColorColureLines
		   WRITE setColorColureLines
		   NOTIFY colureLinesColorChanged)

	Q_PROPERTY(bool circumpolarCirclesDisplayed
		   READ getFlagCircumpolarCircles
		   WRITE setFlagCircumpolarCircles
		   NOTIFY circumpolarCirclesDisplayedChanged)
	Q_PROPERTY(Vec3f circumpolarCirclesColor
		   READ getColorCircumpolarCircles
		   WRITE setColorCircumpolarCircles
		   NOTIFY circumpolarCirclesColorChanged)

	Q_PROPERTY(bool celestialJ2000PolesDisplayed
		   READ getFlagCelestialJ2000Poles
		   WRITE setFlagCelestialJ2000Poles
		   NOTIFY celestialJ2000PolesDisplayedChanged)
	Q_PROPERTY(Vec3f celestialJ2000PolesColor
		   READ getColorCelestialJ2000Poles
		   WRITE setColorCelestialJ2000Poles
		   NOTIFY celestialJ2000PolesColorChanged)

	Q_PROPERTY(bool celestialPolesDisplayed
		   READ getFlagCelestialPoles
		   WRITE setFlagCelestialPoles
		   NOTIFY celestialPolesDisplayedChanged)
	Q_PROPERTY(Vec3f celestialPolesColor
		   READ getColorCelestialPoles
		   WRITE setColorCelestialPoles
		   NOTIFY celestialPolesColorChanged)

	Q_PROPERTY(bool zenithNadirDisplayed
		   READ getFlagZenithNadir
		   WRITE setFlagZenithNadir
		   NOTIFY zenithNadirDisplayedChanged)
	Q_PROPERTY(Vec3f zenithNadirColor
		   READ getColorZenithNadir
		   WRITE setColorZenithNadir
		   NOTIFY zenithNadirColorChanged)

	Q_PROPERTY(bool eclipticJ2000PolesDisplayed
		   READ getFlagEclipticJ2000Poles
		   WRITE setFlagEclipticJ2000Poles
		   NOTIFY eclipticJ2000PolesDisplayedChanged)
	Q_PROPERTY(Vec3f eclipticJ2000PolesColor
		   READ getColorEclipticJ2000Poles
		   WRITE setColorEclipticJ2000Poles
		   NOTIFY eclipticJ2000PolesColorChanged)

	Q_PROPERTY(bool eclipticPolesDisplayed
		   READ getFlagEclipticPoles
		   WRITE setFlagEclipticPoles
		   NOTIFY eclipticPolesDisplayedChanged)
	Q_PROPERTY(Vec3f eclipticPolesColor
		   READ getColorEclipticPoles
		   WRITE setColorEclipticPoles
		   NOTIFY eclipticPolesColorChanged)

	Q_PROPERTY(bool galacticPolesDisplayed
		   READ getFlagGalacticPoles
		   WRITE setFlagGalacticPoles
		   NOTIFY galacticPolesDisplayedChanged)
	Q_PROPERTY(Vec3f galacticPolesColor
		   READ getColorGalacticPoles
		   WRITE setColorGalacticPoles
		   NOTIFY galacticPolesColorChanged)

	Q_PROPERTY(bool supergalacticPolesDisplayed
		   READ getFlagSupergalacticPoles
		   WRITE setFlagSupergalacticPoles
		   NOTIFY supergalacticPolesDisplayedChanged)
	Q_PROPERTY(Vec3f supergalacticPolesColor
		   READ getColorSupergalacticPoles
		   WRITE setColorSupergalacticPoles
		   NOTIFY supergalacticPolesColorChanged)

	Q_PROPERTY(bool equinoxJ2000PointsDisplayed
		   READ getFlagEquinoxJ2000Points
		   WRITE setFlagEquinoxJ2000Points
		   NOTIFY equinoxJ2000PointsDisplayedChanged)
	Q_PROPERTY(Vec3f equinoxJ2000PointsColor
		   READ getColorEquinoxJ2000Points
		   WRITE setColorEquinoxJ2000Points
		   NOTIFY equinoxJ2000PointsColorChanged)

	Q_PROPERTY(bool equinoxPointsDisplayed
		   READ getFlagEquinoxPoints
		   WRITE setFlagEquinoxPoints
		   NOTIFY equinoxPointsDisplayedChanged)
	Q_PROPERTY(Vec3f equinoxPointsColor
		   READ getColorEquinoxPoints
		   WRITE setColorEquinoxPoints
		   NOTIFY equinoxPointsColorChanged)

	Q_PROPERTY(bool solsticeJ2000PointsDisplayed
		   READ getFlagSolsticeJ2000Points
		   WRITE setFlagSolsticeJ2000Points
		   NOTIFY solsticeJ2000PointsDisplayedChanged)
	Q_PROPERTY(Vec3f solsticeJ2000PointsColor
		   READ getColorSolsticeJ2000Points
		   WRITE setColorSolsticeJ2000Points
		   NOTIFY solsticeJ2000PointsColorChanged)

	Q_PROPERTY(bool solsticePointsDisplayed
		   READ getFlagSolsticePoints
		   WRITE setFlagSolsticePoints
		   NOTIFY solsticePointsDisplayedChanged)
	Q_PROPERTY(Vec3f solsticePointsColor
		   READ getColorSolsticePoints
		   WRITE setColorSolsticePoints
		   NOTIFY solsticePointsColorChanged)

	Q_PROPERTY(bool antisolarPointDisplayed
		   READ getFlagAntisolarPoint
		   WRITE setFlagAntisolarPoint
		   NOTIFY antisolarPointDisplayedChanged)
	Q_PROPERTY(Vec3f antisolarPointColor
		   READ getColorAntisolarPoint
		   WRITE setColorAntisolarPoint
		   NOTIFY antisolarPointColorChanged)

public:
	GridLinesMgr();
	virtual ~GridLinesMgr();

	///////////////////////////////////////////////////////////////////////////
	// Methods defined in the StelModule class
	//! Initialize the GridLinesMgr. This process checks the values in the
	//! application settings, and according to the settings there
	//! sets the visibility of the Equatorial Grids, Ecliptical Grids, Azimuthal Grid, Meridian Line,
	//! Equator Line and Ecliptic Lines.
	virtual void init();

	//! Get the module ID, returns "GridLinesMgr".
	virtual QString getModuleID() const {return "GridLinesMgr";}

	//! Draw the grids and great circle lines.
	//! Draws the Equatorial Grids, Ecliptical Grids, Azimuthal Grid, Meridian Line, Equator Line,
	//! Ecliptic Lines, Precession Circles, Conjunction-Opposition Line, east-west vertical and colures according to the
	//! various flags which control their visibility.
	virtual void draw(StelCore* core);

	//! Update time-dependent features.
	//! Used to control fading when turning on and off the grid lines and great circles.
	virtual void update(double deltaTime);

	//! Used to determine the order in which the various modules are drawn.
	virtual double getCallOrder(StelModuleActionName actionName) const;

	///////////////////////////////////////////////////////////////////////////////////////
	// Setter and getters
public slots:
	//! Setter ("master switch") for displaying any grid/line.
	void setFlagGridlines(const bool displayed);
	//! Accessor ("master switch") for displaying any grid/line.
	bool getFlagGridlines(void) const;

	//! Setter for displaying Azimuthal Grid.
	void setFlagAzimuthalGrid(const bool displayed);
	//! Accessor for displaying Azimuthal Grid.
	bool getFlagAzimuthalGrid(void) const;
	//! Get the current color of the Azimuthal Grid.
	Vec3f getColorAzimuthalGrid(void) const;
	//! Set the color of the Azimuthal Grid.
	//! @param newColor The color of azimuthal grid
	//! @code
	//! // example of usage in scripts
	//! GridLinesMgr.setColorAzimuthalGrid(Vec3f(1.0,0.0,0.0));
	//! @endcode
	void setColorAzimuthalGrid(const Vec3f& newColor);

	//! Setter for displaying Equatorial Grid.
	void setFlagEquatorGrid(const bool displayed);
	//! Accessor for displaying Equatorial Grid.
	bool getFlagEquatorGrid(void) const;
	//! Get the current color of the Equatorial Grid.
	Vec3f getColorEquatorGrid(void) const;
	//! Set the color of the Equatorial Grid.
	//! @param newColor The color of equatorial grid
	//! @code
	//! // example of usage in scripts
	//! GridLinesMgr.setColorEquatorGrid(Vec3f(1.0,0.0,0.0));
	//! @endcode
	void setColorEquatorGrid(const Vec3f& newColor);

	//! Setter for displaying Equatorial J2000 Grid.
	void setFlagEquatorJ2000Grid(const bool displayed);
	//! Accessor for displaying Equatorial J2000 Grid.
	bool getFlagEquatorJ2000Grid(void) const;
	//! Get the current color of the Equatorial J2000 Grid.
	Vec3f getColorEquatorJ2000Grid(void) const;
	//! Set the color of the Equatorial J2000 Grid.
	//! @param newColor The color of equatorial J2000 grid
	//! @code
	//! // example of usage in scripts
	//! GridLinesMgr.setColorEquatorJ2000Grid(Vec3f(1.0,0.0,0.0));
	//! @endcode
	void setColorEquatorJ2000Grid(const Vec3f& newColor);

	//! Setter for displaying Ecliptic Grid of J2000.0.
	void setFlagEclipticJ2000Grid(const bool displayed);
	//! Accessor for displaying Ecliptic Grid.
	bool getFlagEclipticJ2000Grid(void) const;
	//! Get the current color of the Ecliptic J2000 Grid.
	Vec3f getColorEclipticJ2000Grid(void) const;
	//! Set the color of the Ecliptic J2000 Grid.
	//! @param newColor The color of ecliptic J2000 grid
	//! @code
	//! // example of usage in scripts
	//! GridLinesMgr.setColorEclipticJ2000Grid(Vec3f(1.0,0.0,0.0));
	//! @endcode
	void setColorEclipticJ2000Grid(const Vec3f& newColor);

	//! Setter for displaying Ecliptic Grid of Date.
	void setFlagEclipticGrid(const bool displayed);
	//! Accessor for displaying Ecliptic Grid.
	bool getFlagEclipticGrid(void) const;
	//! Get the current color of the Ecliptic of Date Grid.
	Vec3f getColorEclipticGrid(void) const;
	//! Set the color of the Ecliptic Grid.
	//! @param newColor The color of Ecliptic of Date grid
	//! @code
	//! // example of usage in scripts
	//! GridLinesMgr.setColorEclipticGrid(Vec3f(1.0,0.0,0.0));
	//! @endcode
	void setColorEclipticGrid(const Vec3f& newColor);

	//! Setter for displaying Galactic Grid.
	void setFlagGalacticGrid(const bool displayed);
	//! Accessor for displaying Galactic Grid.
	bool getFlagGalacticGrid(void) const;
	//! Get the current color of the Galactic Grid.
	Vec3f getColorGalacticGrid(void) const;
	//! Set the color of the Galactic Grid.
	//! @param newColor The color of galactic grid
	//! @code
	//! // example of usage in scripts
	//! GridLinesMgr.setColorGalacticGrid(Vec3f(1.0,0.0,0.0));
	//! @endcode
	void setColorGalacticGrid(const Vec3f& newColor);

	//! Setter for displaying Supergalactic Grid.
	void setFlagSupergalacticGrid(const bool displayed);
	//! Accessor for displaying Supergalactic Grid.
	bool getFlagSupergalacticGrid(void) const;
	//! Get the current color of the Supergalactic Grid.
	Vec3f getColorSupergalacticGrid(void) const;
	//! Set the color of the Supergalactic Grid.
	//! @param newColor The color of supergalactic grid
	//! @code
	//! // example of usage in scripts
	//! GridLinesMgr.setColorSupergalacticGrid(Vec3f(1.0,0.0,0.0));
	//! @endcode
	void setColorSupergalacticGrid(const Vec3f& newColor);

	//! Setter for displaying Equatorial Line.
	void setFlagEquatorLine(const bool displayed);
	//! Accessor for displaying Equatorial Line.
	bool getFlagEquatorLine(void) const;
	//! Get the current color of the Equatorial Line.
	Vec3f getColorEquatorLine(void) const;
	//! Set the color of the Equator Line.
	//! @param newColor The color of equator line
	//! @code
	//! // example of usage in scripts
	//! GridLinesMgr.setColorEquatorLine(Vec3f(1.0,0.0,0.0));
	//! @endcode
	void setColorEquatorLine(const Vec3f& newColor);

	//! Setter for displaying J2000 Equatorial Line.
	void setFlagEquatorJ2000Line(const bool displayed);
	//! Accessor for displaying J2000 Equatorial Line.
	bool getFlagEquatorJ2000Line(void) const;
	//! Get the current color of the J2000 Equatorial Line.
	Vec3f getColorEquatorJ2000Line(void) const;
	//! Set the color of the J2000 Equator Line.
	//! @param newColor The color of J2000 equator line
	//! @code
	//! // example of usage in scripts
	//! GridLinesMgr.setColorEquatorJ2000Line(Vec3f(1.0,0.0,0.0));
	//! @endcode
	void setColorEquatorJ2000Line(const Vec3f& newColor);

	//! Setter for displaying Ecliptic of J2000 Line.
	void setFlagEclipticJ2000Line(const bool displayed);
	//! Accessor for displaying Ecliptic of J2000 Line.
	bool getFlagEclipticJ2000Line(void) const;
	//! Get the current color of the Ecliptic of J2000 Line.
	Vec3f getColorEclipticJ2000Line(void) const;
	//! Set the color of the Ecliptic of J2000 Line.
	//! @param newColor The color of ecliptic 2000 line
	//! @code
	//! // example of usage in scripts
	//! GridLinesMgr.setColorEcliptic2000Line(Vec3f(1.0,0.0,0.0));
	//! @endcode
	void setColorEclipticJ2000Line(const Vec3f& newColor);

	//! Setter for displaying Ecliptic Line.
	void setFlagEclipticLine(const bool displayed);
	//! Accessor for displaying Ecliptic Line.
	bool getFlagEclipticLine(void) const;
	//! Get the current color of the Ecliptic Line.
	Vec3f getColorEclipticLine(void) const;
	//! Set the color of the Ecliptic Line.
	//! @param newColor The color of ecliptic line
	//! @code
	//! // example of usage in scripts
	//! GridLinesMgr.setColorEclipticLine(Vec3f(1.0,0.0,0.0));
	//! @endcode
	void setColorEclipticLine(const Vec3f& newColor);

	//! Setter for displaying precession circles.
	void setFlagPrecessionCircles(const bool displayed);
	//! Accessor for displaying precession circles.
	bool getFlagPrecessionCircles(void) const;
	//! Get the current color of the precession circles.
	Vec3f getColorPrecessionCircles(void) const;
	//! Set the color of the precession circles.
	//! @param newColor The color of precession circles
	//! @code
	//! // example of usage in scripts
	//! GridLinesMgr.setColorPrecessionCircles(Vec3f(1.0,0.0,0.0));
	//! @endcode
	void setColorPrecessionCircles(const Vec3f& newColor);

	//! Setter for displaying Meridian Line.
	void setFlagMeridianLine(const bool displayed);
	//! Accessor for displaying Meridian Line.
	bool getFlagMeridianLine(void) const;
	//! Get the current color of the Meridian Line.
	Vec3f getColorMeridianLine(void) const;
	//! Set the color of the Meridian Line.
	//! @param newColor The color of meridian line
	//! @code
	//! // example of usage in scripts
	//! GridLinesMgr.setColorMeridianLine(Vec3f(1.0,0.0,0.0));
	//! @endcode
	void setColorMeridianLine(const Vec3f& newColor);

	//! Setter for displaying opposition/conjunction longitude line.
	void setFlagLongitudeLine(const bool displayed);
	//! Accessor for displaying opposition/conjunction longitude line.
	bool getFlagLongitudeLine(void) const;
	//! Get the current color of the opposition/conjunction longitude line.
	Vec3f getColorLongitudeLine(void) const;
	//! Set the color of the opposition/conjunction longitude line.
	//! @param newColor The color of opposition/conjunction longitude line
	//! @code
	//! // example of usage in scripts
	//! GridLinesMgr.setColorLongitudeLine(Vec3f(1.0,0.0,0.0));
	//! @endcode
	void setColorLongitudeLine(const Vec3f& newColor);

	//! Setter for displaying Horizon Line.
	void setFlagHorizonLine(const bool displayed);
	//! Accessor for displaying Horizon Line.
	bool getFlagHorizonLine(void) const;
	//! Get the current color of the Horizon Line.
	Vec3f getColorHorizonLine(void) const;
	//! Set the color of the Horizon Line.
	//! @param newColor The color of horizon line
	//! @code
	//! // example of usage in scripts
	//! GridLinesMgr.setColorHorizonLine(Vec3f(1.0,0.0,0.0));
	//! @endcode
	void setColorHorizonLine(const Vec3f& newColor);

	//! Setter for displaying Galactic Equator Line.
	void setFlagGalacticEquatorLine(const bool displayed);
	//! @deprecated Setter for displaying Galactic "Plane" (i.e., Equator) Line. Left here for compatibility with older scripts.
	//! @note will be deleted in version 0.14
	void setFlagGalacticPlaneLine(const bool displayed) { setFlagGalacticEquatorLine(displayed); }
	//! Accessor for displaying Galactic Equator Line.
	bool getFlagGalacticEquatorLine(void) const;
	//! @deprecated Accessor for displaying Galactic "Plane" (i.e., Equator) Line. Left here for compatibility with older scripts.
	//! @note will be deleted in version 0.14
	bool getFlagGalacticPlaneLine(void) const { return getFlagGalacticEquatorLine(); }
	//! Get the current color of the Galactic Equator Line.
	Vec3f getColorGalacticEquatorLine(void) const;
	//! Set the color of the Galactic Equator Line.
	//! @param newColor The color of galactic equator line
	//! @code
	//! // example of usage in scripts
	//! GridLinesMgr.setColorGalacticEquatorLine(Vec3f(1.0,0.0,0.0));
	//! @endcode
	void setColorGalacticEquatorLine(const Vec3f& newColor);

	//! Setter for displaying Supergalactic Equator Line.
	void setFlagSupergalacticEquatorLine(const bool displayed);
	//! Accessor for displaying Supergalactic Equator Line.
	bool getFlagSupergalacticEquatorLine(void) const;
	//! Get the current color of the Supergalactic Equator Line.
	Vec3f getColorSupergalacticEquatorLine(void) const;
	//! Set the color of the Supergalactic Equator Line.
	//! @param newColor The color of supergalactic equator line
	//! @code
	//! // example of usage in scripts
	//! GridLinesMgr.setColorSupergalacticEquatorLine(Vec3f(1.0,0.0,0.0));
	//! @endcode
	void setColorSupergalacticEquatorLine(const Vec3f& newColor);

	//! Setter for displaying the Prime Vertical Line.
	void setFlagPrimeVerticalLine(const bool displayed);
	//! Accessor for displaying Prime Vertical Line.
	bool getFlagPrimeVerticalLine(void) const;
	//! Get the current color of the Prime Vertical Line.
	Vec3f getColorPrimeVerticalLine(void) const;
	//! Set the color of the Prime Vertical Line.
	//! @param newColor The color of the Prime Vertical line
	//! @code
	//! // example of usage in scripts
	//! GridLinesMgr.setColorPrimeVerticalLine(Vec3f(1.0,0.0,0.0));
	//! @endcode
	void setColorPrimeVerticalLine(const Vec3f& newColor);

	//! Setter for displaying the Colure Lines.
	void setFlagColureLines(const bool displayed);
	//! Accessor for displaying the Colure Lines.
	bool getFlagColureLines(void) const;
	//! Get the current color of the Colure Lines.
	Vec3f getColorColureLines(void) const;
	//! Set the color of the Colure Lines.
	//! @param newColor The color of the Colure lines
	//! @code
	//! // example of usage in scripts
	//! GridLinesMgr.setColorColureLines(Vec3f(1.0,0.0,0.0));
	//! @endcode
	void setColorColureLines(const Vec3f& newColor);

	//! Setter for displaying circumpolar circles.
	void setFlagCircumpolarCircles(const bool displayed);
	//! Accessor for displaying circumpolar circles.
	bool getFlagCircumpolarCircles(void) const;
	//! Get the current color of the circumpolar circles.
	Vec3f getColorCircumpolarCircles(void) const;
	//! Set the color of the circumpolar circles.
	//! @param newColor The color of circumpolar circles
	//! @code
	//! // example of usage in scripts
	//! GridLinesMgr.setColorCircumpolarCircles(Vec3f(1.0,0.0,0.0));
	//! @endcode
	void setColorCircumpolarCircles(const Vec3f& newColor);

	//! Setter for displaying celestial poles of J2000.
	void setFlagCelestialJ2000Poles(const bool displayed);
	//! Accessor for displaying celestial poles of J2000.
	bool getFlagCelestialJ2000Poles(void) const;
	//! Get the current color of the celestial poles of J2000.
	Vec3f getColorCelestialJ2000Poles(void) const;
	//! Set the color of the celestial poles of J2000.
	//! @param newColor The color of celestial poles of J2000
	//! @code
	//! // example of usage in scripts
	//! GridLinesMgr.setColorCelestialJ2000Poles(Vec3f(1.0,0.0,0.0));
	//! @endcode
	void setColorCelestialJ2000Poles(const Vec3f& newColor);

	//! Setter for displaying celestial poles.
	void setFlagCelestialPoles(const bool displayed);
	//! Accessor for displaying celestial poles.
	bool getFlagCelestialPoles(void) const;
	//! Get the current color of the celestial poles.
	Vec3f getColorCelestialPoles(void) const;
	//! Set the color of the celestial poles.
	//! @param newColor The color of celestial poles
	//! @code
	//! // example of usage in scripts
	//! GridLinesMgr.setColorCelestialPoles(Vec3f(1.0,0.0,0.0));
	//! @endcode
	void setColorCelestialPoles(const Vec3f& newColor);

	//! Setter for displaying zenith and nadir.
	void setFlagZenithNadir(const bool displayed);
	//! Accessor for displaying zenith and nadir.
	bool getFlagZenithNadir(void) const;
	//! Get the current color of the zenith and nadir.
	Vec3f getColorZenithNadir(void) const;
	//! Set the color of the zenith and nadir.
	//! @param newColor The color of zenith and nadir
	//! @code
	//! // example of usage in scripts
	//! GridLinesMgr.setColorZenithNadir(Vec3f(1.0,0.0,0.0));
	//! @endcode
	void setColorZenithNadir(const Vec3f& newColor);

	//! Setter for displaying ecliptic poles of J2000.
	void setFlagEclipticJ2000Poles(const bool displayed);
	//! Accessor for displaying ecliptic poles of J2000.
	bool getFlagEclipticJ2000Poles(void) const;
	//! Get the current color of the ecliptic poles of J2000.
	Vec3f getColorEclipticJ2000Poles(void) const;
	//! Set the color of the ecliptic poles of J2000.
	//! @param newColor The color of ecliptic poles of J2000
	//! @code
	//! // example of usage in scripts
	//! GridLinesMgr.setColorEclipticJ2000Poles(Vec3f(1.0,0.0,0.0));
	//! @endcode
	void setColorEclipticJ2000Poles(const Vec3f& newColor);

	//! Setter for displaying ecliptic poles.
	void setFlagEclipticPoles(const bool displayed);
	//! Accessor for displaying ecliptic poles.
	bool getFlagEclipticPoles(void) const;
	//! Get the current color of the ecliptic poles.
	Vec3f getColorEclipticPoles(void) const;
	//! Set the color of the ecliptic poles.
	//! @param newColor The color of ecliptic poles
	//! @code
	//! // example of usage in scripts
	//! GridLinesMgr.setColorEclipticPoles(Vec3f(1.0,0.0,0.0));
	//! @endcode
	void setColorEclipticPoles(const Vec3f& newColor);

	//! Setter for displaying galactic poles.
	void setFlagGalacticPoles(const bool displayed);
	//! Accessor for displaying galactic poles.
	bool getFlagGalacticPoles(void) const;
	//! Get the current color of the galactic poles.
	Vec3f getColorGalacticPoles(void) const;
	//! Set the color of the galactic poles.
	//! @param newColor The color of galactic poles
	//! @code
	//! // example of usage in scripts
	//! GridLinesMgr.setColorGalacticPoles(Vec3f(1.0,0.0,0.0));
	//! @endcode
	void setColorGalacticPoles(const Vec3f& newColor);

	//! Setter for displaying supergalactic poles.
	void setFlagSupergalacticPoles(const bool displayed);
	//! Accessor for displaying supergalactic poles.
	bool getFlagSupergalacticPoles(void) const;
	//! Get the current color of the supergalactic poles.
	Vec3f getColorSupergalacticPoles(void) const;
	//! Set the color of the supergalactic poles.
	//! @param newColor The color of supergalactic poles
	//! @code
	//! // example of usage in scripts
	//! GridLinesMgr.setColorSupergalacticPoles(Vec3f(1.0,0.0,0.0));
	//! @endcode
	void setColorSupergalacticPoles(const Vec3f& newColor);

	//! Setter for displaying equinox points of J2000.
	void setFlagEquinoxJ2000Points(const bool displayed);
	//! Accessor for displaying equinox points of J2000.
	bool getFlagEquinoxJ2000Points(void) const;
	//! Get the current color of the equinox points of J2000.
	Vec3f getColorEquinoxJ2000Points(void) const;
	//! Set the color of the equinox points of J2000.
	//! @param newColor The color of equinox points
	//! @code
	//! // example of usage in scripts
	//! GridLinesMgr.setColorEquinoxJ2000Points(Vec3f(1.0,0.0,0.0));
	//! @endcode
	void setColorEquinoxJ2000Points(const Vec3f& newColor);

	//! Setter for displaying equinox points.
	void setFlagEquinoxPoints(const bool displayed);
	//! Accessor for displaying equinox points.
	bool getFlagEquinoxPoints(void) const;
	//! Get the current color of the equinox points.
	Vec3f getColorEquinoxPoints(void) const;
	//! Set the color of the equinox points.
	//! @param newColor The color of equinox points
	//! @code
	//! // example of usage in scripts
	//! GridLinesMgr.setColorEquinoxPoints(Vec3f(1.0,0.0,0.0));
	//! @endcode
	void setColorEquinoxPoints(const Vec3f& newColor);

	//! Setter for displaying solstice points of J2000.
	void setFlagSolsticeJ2000Points(const bool displayed);
	//! Accessor for displaying solstice points of J2000.
	bool getFlagSolsticeJ2000Points(void) const;
	//! Get the current color of the solstice points of J2000.
	Vec3f getColorSolsticeJ2000Points(void) const;
	//! Set the color of the solstice points of J2000.
	//! @param newColor The color of solstice points
	//! @code
	//! // example of usage in scripts
	//! GridLinesMgr.setColorSolsticeJ2000Points(Vec3f(1.0,0.0,0.0));
	//! @endcode
	void setColorSolsticeJ2000Points(const Vec3f& newColor);

	//! Setter for displaying solstice points.
	void setFlagSolsticePoints(const bool displayed);
	//! Accessor for displaying solstice points.
	bool getFlagSolsticePoints(void) const;
	//! Get the current color of the solstice points.
	Vec3f getColorSolsticePoints(void) const;
	//! Set the color of the solstice points.
	//! @param newColor The color of solstice points
	//! @code
	//! // example of usage in scripts
	//! GridLinesMgr.setColorSolsticePoints(Vec3f(1.0,0.0,0.0));
	//! @endcode
	void setColorSolsticePoints(const Vec3f& newColor);

	//! Setter for displaying antisolar point.
	void setFlagAntisolarPoint(const bool displayed);
	//! Accessor for displaying antisolar point.
	bool getFlagAntisolarPoint(void) const;
	//! Get the current color of the antisolar point.
	Vec3f getColorAntisolarPoint(void) const;
	//! Set the color of the antisolar point.
	//! @param newColor The color of antisolar point
	//! @code
	//! // example of usage in scripts
	//! GridLinesMgr.setColorAntisolarPoint(Vec3f(1.0,0.0,0.0));
	//! @endcode
	void setColorAntisolarPoint(const Vec3f& newColor);

signals:
	void gridlinesDisplayedChanged(const bool) const;
	void azimuthalGridDisplayedChanged(const bool) const;
	void azimuthalGridColorChanged(const Vec3f & newColor) const;
	void equatorGridDisplayedChanged(const bool displayed) const;
	void equatorGridColorChanged(const Vec3f & newColor) const;
	void equatorJ2000GridDisplayedChanged(const bool displayed) const;
	void equatorJ2000GridColorChanged(const Vec3f & newColor) const;
	void eclipticGridDisplayedChanged(const bool displayed) const;
	void eclipticGridColorChanged(const Vec3f & newColor) const;
	void eclipticJ2000GridDisplayedChanged(const bool displayed) const;
	void eclipticJ2000GridColorChanged(const Vec3f & newColor) const;
	void galacticGridDisplayedChanged(const bool displayed) const;
	void galacticGridColorChanged(const Vec3f & newColor) const;
	void supergalacticGridDisplayedChanged(const bool displayed) const;
	void supergalacticGridColorChanged(const Vec3f & newColor) const;
	void equatorLineDisplayedChanged(const bool displayed) const;
	void equatorLineColorChanged(const Vec3f & newColor) const;
	void equatorJ2000LineDisplayedChanged(const bool displayed) const;
	void equatorJ2000LineColorChanged(const Vec3f & newColor) const;
	void eclipticLineDisplayedChanged(const bool displayed) const;
	void eclipticLineColorChanged(const Vec3f & newColor) const;
	void eclipticJ2000LineDisplayedChanged(const bool displayed) const;
	void eclipticJ2000LineColorChanged(const Vec3f & newColor) const;
	void precessionCirclesDisplayedChanged(const bool displayed) const;
	void precessionCirclesColorChanged(const Vec3f & newColor) const;
	void meridianLineDisplayedChanged(const bool displayed) const;
	void meridianLineColorChanged(const Vec3f & newColor) const;
	void longitudeLineDisplayedChanged(const bool displayed) const;
	void longitudeLineColorChanged(const Vec3f & newColor) const;
	void horizonLineDisplayedChanged(const bool displayed) const;
	void horizonLineColorChanged(const Vec3f & newColor) const;
	void galacticEquatorLineDisplayedChanged(const bool displayed) const;
	void galacticEquatorLineColorChanged(const Vec3f & newColor) const;
	void supergalacticEquatorLineDisplayedChanged(const bool displayed) const;
	void supergalacticEquatorLineColorChanged(const Vec3f & newColor) const;
	void primeVerticalLineDisplayedChanged(const bool displayed) const;
	void primeVerticalLineColorChanged(const Vec3f & newColor) const;
	void colureLinesDisplayedChanged(const bool displayed) const;
	void colureLinesColorChanged(const Vec3f & newColor) const;
	void circumpolarCirclesDisplayedChanged(const bool displayed) const;
	void circumpolarCirclesColorChanged(const Vec3f & newColor) const;
	void celestialJ2000PolesDisplayedChanged(const bool displayed) const;
	void celestialJ2000PolesColorChanged(const Vec3f & newColor) const;
	void celestialPolesDisplayedChanged(const bool displayed) const;
	void celestialPolesColorChanged(const Vec3f & newColor) const;
	void zenithNadirDisplayedChanged(const bool displayed) const;
	void zenithNadirColorChanged(const Vec3f & newColor) const;
	void eclipticJ2000PolesDisplayedChanged(const bool displayed) const;
	void eclipticJ2000PolesColorChanged(const Vec3f & newColor) const;
	void eclipticPolesDisplayedChanged(const bool displayed) const;
	void eclipticPolesColorChanged(const Vec3f & newColor) const;
	void galacticPolesDisplayedChanged(const bool displayed) const;
	void galacticPolesColorChanged(const Vec3f & newColor) const;
	void supergalacticPolesDisplayedChanged(const bool displayed) const;
	void supergalacticPolesColorChanged(const Vec3f & newColor) const;
	void equinoxJ2000PointsDisplayedChanged(const bool displayed) const;
	void equinoxJ2000PointsColorChanged(const Vec3f & newColor) const;
	void equinoxPointsDisplayedChanged(const bool displayed) const;
	void equinoxPointsColorChanged(const Vec3f & newColor) const;
	void solsticeJ2000PointsDisplayedChanged(const bool displayed) const;
	void solsticeJ2000PointsColorChanged(const Vec3f & newColor) const;
	void solsticePointsDisplayedChanged(const bool displayed) const;
	void solsticePointsColorChanged(const Vec3f & newColor) const;
	void antisolarPointDisplayedChanged(const bool displayed) const;
	void antisolarPointColorChanged(const Vec3f & newColor) const;

private slots:
	//! Re-translate the labels of the great circles.
	//! Contains only calls to SkyLine::updateLabel().
	void updateLineLabels();
	//! Connect the earth shared pointer.
	//! Must be connected to SolarSystem::solarSystemDataReloaded()
	void connectEarthFromSolarSystem();

	//! Connect from StelApp to reset all fonts of the grids, lines and points.
	void setFontSizeFromApp(int size);

private:
	QSharedPointer<Planet> earth;           // shortcut Earth pointer. Must be reconnected whenever solar system has been reloaded.
	bool gridlinesDisplayed;			// master switch to switch off all grids/lines. (useful for oculars plugin)
	SkyGrid * equGrid;				// Equatorial grid
	SkyGrid * equJ2000Grid;			// Equatorial J2000 grid
	SkyGrid * galacticGrid;			// Galactic grid
	SkyGrid * supergalacticGrid;		// Supergalactic grid
	SkyGrid * eclGrid;				// Ecliptic of Date grid
	SkyGrid * eclJ2000Grid;			// Ecliptic J2000 grid
	SkyGrid * aziGrid;				// Azimuthal grid
	SkyLine * equatorLine;			// Celestial Equator line
	SkyLine * equatorJ2000Line;		// Celestial Equator line of J2000
	SkyLine * eclipticLine;			// Ecliptic line
	SkyLine * eclipticJ2000Line;		// Ecliptic line of J2000
	SkyLine * precessionCircleN;		// Northern precession circle
	SkyLine * precessionCircleS;		// Southern precession circle
	SkyLine * meridianLine;			// Meridian line
	SkyLine * longitudeLine;			// Opposition/conjunction longitude line
	SkyLine * horizonLine;			// Horizon line
	SkyLine * galacticEquatorLine;		// line depicting the Galactic equator as defined by the IAU definition of Galactic coordinates (System II, 1958)
	SkyLine * supergalacticEquatorLine;	// line depicting the Supergalactic equator
	SkyLine * primeVerticalLine;		// Prime Vertical line
	SkyLine * colureLine_1;			// First Colure line (0/12h)
	SkyLine * colureLine_2;			// Second Colure line (6/18h)
	SkyLine * circumpolarCircleN;		// Northern circumpolar circle
	SkyLine * circumpolarCircleS;		// Southern circumpolar circle
	SkyPoint * celestialJ2000Poles;		// Celestial poles of J2000
	SkyPoint * celestialPoles;			// Celestial poles
	SkyPoint * zenithNadir;			// Zenith and nadir
	SkyPoint * eclipticJ2000Poles;		// Ecliptic poles of J2000
	SkyPoint * eclipticPoles;			// Ecliptic poles
	SkyPoint * galacticPoles;			// Galactic poles
	SkyPoint * supergalacticPoles;		// Supergalactic poles
	SkyPoint * equinoxJ2000Points;	// Equinox points of J2000
	SkyPoint * equinoxPoints;			// Equinox points
	SkyPoint * solsticeJ2000Points;		// Solstice points of J2000
	SkyPoint * solsticePoints;			// Solstice points
	SkyPoint * antisolarPoint;			// Solstice points
};

#endif // GRIDLINESMGR_HPP

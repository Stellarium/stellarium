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

#ifndef _GRIDLINESMGR_HPP_
#define _GRIDLINESMGR_HPP_

#include "VecMath.hpp"
#include "StelModule.hpp"

class SkyGrid;
class SkyLine;

//! @class GridLinesMgr
//! The GridLinesMgr controls the drawing of the Azimuthal, Equatorial, Ecliptical and Galactic Grids,
//! as well as the great circles: Meridian Line, Ecliptic Lines of J2000.0 and date, Equator Line (of J2000.0 and date),
//! Precession Circles, and a special line marking conjunction or opposition in ecliptical longitude (of date).
class GridLinesMgr : public StelModule
{
	Q_OBJECT
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
	// NEW
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
	//! Ecliptic Lines, Precession Circles, and Conjunction-Opposition Line according to the
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
signals:
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


private slots:
	//! Sets the colors of: grids and great circles, Equatorial Grid, Azimuthal Grid, 
	//! Meridian Line, Equator Line and Ecliptic Line.
	void setStelStyle(const QString& section);
	//! Re-translate the labels of the great circles.
	//! Contains only calls to SkyLine::updateLabel().
	void updateLineLabels();

private:
	SkyGrid * equGrid;		// Equatorial grid
	SkyGrid * equJ2000Grid;		// Equatorial J2000 grid
	SkyGrid * galacticGrid;		// Galactic grid
	SkyGrid * eclGrid;		// Ecliptic of Date grid
	SkyGrid * eclJ2000Grid;		// Ecliptic J2000 grid
	SkyGrid * aziGrid;		// Azimuthal grid
	SkyLine * equatorLine;		// Celestial Equator line
	SkyLine * equatorJ2000Line;	// Celestial Equator of J2000 line
	SkyLine * eclipticLine;		// Ecliptic line
	SkyLine * eclipticJ2000Line;	// Ecliptic line of J2000
	SkyLine * precessionCircleN;	// Northern precession circle
	SkyLine * precessionCircleS;	// Southern precession circle
	SkyLine * meridianLine;		// Meridian line
	SkyLine * longitudeLine; 	// Opposition/conjunction longitude line
	SkyLine * horizonLine;		// Horizon line
	SkyLine * galacticEquatorLine;	// line depicting the Galactic equator as defined by the IAU definition of Galactic coordinates (System II, 1958)
};

#endif // _GRIDLINESMGR_HPP_

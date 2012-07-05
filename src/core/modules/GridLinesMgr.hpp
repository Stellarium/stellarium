/*
 * Stellarium
 * Copyright (C) 2007 Fabien Chereau
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
//! The GridLinesMgr controls the drawing of the Azimuthal and Equatorial Grids,
//! as well as the great circles: Meridian Line, Ecliptic Line and Equator Line.
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
	Q_PROPERTY(bool eclipticJ2000GridDisplayed
			   READ getFlagEclipticJ2000Grid
			   WRITE setFlagEclipticJ2000Grid
			   NOTIFY eclipticJ2000GridDisplayedChanged)
	Q_PROPERTY(Vec3f equatorJ2000GridColor
			   READ getColorEquatorJ2000Grid
			   WRITE setColorEquatorJ2000Grid
			   NOTIFY equatorJ2000GridColorChanged)
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
	Q_PROPERTY(bool eclipticLineDisplayed
			   READ getFlagEclipticLine
			   WRITE setFlagEclipticLine
			   NOTIFY eclipticLineDisplayedChanged)
	Q_PROPERTY(Vec3f eclipticLineColor
			   READ getColorEclipticLine
			   WRITE setColorEclipticLine
			   NOTIFY eclipticLineColorChanged)
	Q_PROPERTY(bool meridianLineDisplayed
			   READ getFlagMeridianLine
			   WRITE setFlagMeridianLine
			   NOTIFY meridianLineDisplayedChanged)
	Q_PROPERTY(Vec3f meridianLineColor
			   READ getColorMeridianLine
			   WRITE setColorMeridianLine
			   NOTIFY meridianLineColorChanged)
	Q_PROPERTY(bool horizonLineDisplayed
			   READ getFlagHorizonLine
			   WRITE setFlagHorizonLine
			   NOTIFY horizonLineDisplayedChanged)
	Q_PROPERTY(Vec3f horizonLineColor
			   READ getColorHorizonLine
			   WRITE setColorHorizonLine
			   NOTIFY horizonLineColorChanged)
	Q_PROPERTY(bool galacticPlaneLineDisplayed
			   READ getFlagGalacticPlaneLine
			   WRITE setFlagGalacticPlaneLine
			   NOTIFY galacticPlaneLineDisplayedChanged)
	Q_PROPERTY(Vec3f galacticPlaneLineColor
			   READ getColorGalacticPlaneLine
			   WRITE setColorGalacticPlaneLine
			   NOTIFY galacticPlaneLineColorChanged)

public:
	GridLinesMgr();
	virtual ~GridLinesMgr();

	///////////////////////////////////////////////////////////////////////////
	// Methods defined in the StelModule class
	//! Initialize the GridLinesMgr. This process checks the values in the
	//! application settings, and according to the settings there
	//! sets the visibility of the Equatorial Grid, Azimuthal Grid, Meridian Line,
	//! Equator Line and Ecliptic Line.
	virtual void init();

	//! Get the module ID, returns, "gridlines".
	virtual QString getModuleID() const {return "GridLinesMgr";}

	//! Draw the grids and great circle lines.
	//! Draws the Equatorial Grid, Azimuthal Grid, Meridian Line, Equator Line
	//! and Ecliptic Line according to the various flags which control their
	//! visibility.
	virtual void draw(StelCore* core, class StelRenderer* renderer);

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
	void setColorAzimuthalGrid(const Vec3f& newColor);

	//! Setter for displaying Equatorial Grid.
	void setFlagEquatorGrid(const bool displayed);
	//! Accessor for displaying Equatorial Grid.
	bool getFlagEquatorGrid(void) const;
	//! Get the current color of the Equatorial Grid.
	Vec3f getColorEquatorGrid(void) const;
	//! Set the color of the Equatorial Grid.
	void setColorEquatorGrid(const Vec3f& newColor);

	//! Setter for displaying Equatorial Grid.
	void setFlagEquatorJ2000Grid(const bool displayed);
	//! Accessor for displaying Equatorial Grid.
	bool getFlagEquatorJ2000Grid(void) const;
	//! Get the current color of the Equatorial J2000 Grid.
	Vec3f getColorEquatorJ2000Grid(void) const;
	//! Set the color of the Equatorial J2000 Grid.
	void setColorEquatorJ2000Grid(const Vec3f& newColor);

	//! Setter for displaying Ecliptic Grid.
	void setFlagEclipticJ2000Grid(const bool displayed);
	//! Accessor for displaying Ecliptic Grid.
	bool getFlagEclipticJ2000Grid(void) const;
	//! Get the current color of the Ecliptic J2000 Grid.
	Vec3f getColorEclipticJ2000Grid(void) const;
	//! Set the color of the Ecliptic J2000 Grid.
	void setColorEclipticJ2000Grid(const Vec3f& newColor);

	//! Setter for displaying Galactic Grid.
	void setFlagGalacticGrid(const bool displayed);
	//! Accessor for displaying Galactic Grid.
	bool getFlagGalacticGrid(void) const;
	//! Get the current color of the Galactic Grid.
	Vec3f getColorGalacticGrid(void) const;
	//! Set the color of the Galactic Grid.
	void setColorGalacticGrid(const Vec3f& newColor);

	//! Setter for displaying Equatorial Line.
	void setFlagEquatorLine(const bool displayed);
	//! Accessor for displaying Equatorial Line.
	bool getFlagEquatorLine(void) const;
	//! Get the current color of the Equatorial Line.
	Vec3f getColorEquatorLine(void) const;
	//! Set the color of the Meridian Line.
	void setColorEquatorLine(const Vec3f& newColor);

	//! Setter for displaying Ecliptic Line.
	void setFlagEclipticLine(const bool displayed);
	//! Accessor for displaying Ecliptic Line.
	bool getFlagEclipticLine(void) const;
	//! Get the current color of the Ecliptic Line.
	Vec3f getColorEclipticLine(void) const;
	//! Set the color of the Meridian Line.
	void setColorEclipticLine(const Vec3f& newColor);

	//! Setter for displaying Meridian Line.
	void setFlagMeridianLine(const bool displayed);
	//! Accessor for displaying Meridian Line.
	bool getFlagMeridianLine(void) const;
	//! Get the current color of the Meridian Line.
	Vec3f getColorMeridianLine(void) const;
	//! Set the color of the Meridian Line.
	void setColorMeridianLine(const Vec3f& newColor);

	//! Setter for displaying Horizon Line.
	void setFlagHorizonLine(const bool displayed);
	//! Accessor for displaying Horizon Line.
	bool getFlagHorizonLine(void) const;
	//! Get the current color of the Horizon Line.
	Vec3f getColorHorizonLine(void) const;
	//! Set the color of the Horizon Line.
	void setColorHorizonLine(const Vec3f& newColor);

	//! Setter for displaying GalacticPlane Line.
	void setFlagGalacticPlaneLine(const bool displayed);
	//! Accessor for displaying GalacticPlane Line.
	bool getFlagGalacticPlaneLine(void) const;
	//! Get the current color of the GalacticPlane Line.
	Vec3f getColorGalacticPlaneLine(void) const;
	//! Set the color of the GalacticPlane Line.
	void setColorGalacticPlaneLine(const Vec3f& newColor);
signals:
	void azimuthalGridDisplayedChanged(const bool) const;
	void azimuthalGridColorChanged(const Vec3f & newColor) const;
	void equatorGridDisplayedChanged(const bool displayed) const;
	void equatorGridColorChanged(const Vec3f & newColor) const;
	void equatorJ2000GridDisplayedChanged(const bool displayed) const;
	void equatorJ2000GridColorChanged(const Vec3f & newColor) const;
	void eclipticJ2000GridDisplayedChanged(const bool displayed) const;
	void eclipticJ2000GridColorChanged(const Vec3f & newColor) const;
	void galacticGridDisplayedChanged(const bool displayed) const;
	void galacticGridColorChanged(const Vec3f & newColor) const;
	void equatorLineDisplayedChanged(const bool displayed) const;
	void equatorLineColorChanged(const Vec3f & newColor) const;
	void eclipticLineDisplayedChanged(const bool displayed) const;
	void eclipticLineColorChanged(const Vec3f & newColor) const;
	void meridianLineDisplayedChanged(const bool displayed) const;
	void meridianLineColorChanged(const Vec3f & newColor) const;
	void horizonLineDisplayedChanged(const bool displayed) const;
	void horizonLineColorChanged(const Vec3f & newColor) const;
	void galacticPlaneLineDisplayedChanged(const bool displayed) const;
	void galacticPlaneLineColorChanged(const Vec3f & newColor) const;


private slots:
	//! Sets the colors of: grids and great circles, Equatorial Grid, Azimuthal Grid, 
	//! Meridian Line, Equator Line and Ecliptic Line.
	void setStelStyle(const QString& section);
	//! Re-translate the labels of the great circles.
	//! Contains only calls to SkyLine::updateLabel().
	void updateLineLabels();

private:
	SkyGrid * equGrid;      	// Equatorial grid
	SkyGrid * equJ2000Grid; 	// Equatorial J2000 grid
	SkyGrid * galacticGrid; 	// Galactic grid
	SkyGrid * eclJ2000Grid; 	// Ecliptic J2000 grid
	SkyGrid * aziGrid;      	// Azimuthal grid
	SkyLine * equatorLine;  	// Celestial Equator line
	SkyLine * eclipticLine; 	// Ecliptic line
	SkyLine * meridianLine; 	// Meridian line
	SkyLine * horizonLine;		// Horizon line
	SkyLine * galacticPlaneLine;	// line depciting the Galacitc plane as defined by the IAU definition of Galactic coordinates
};

#endif // _GRIDLINESMGR_HPP_

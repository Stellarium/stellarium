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
			   READ isAzimuthalGridDisplayed
			   WRITE setAzimuthalGridDisplayed
			   NOTIFY azimuthalGridDisplayedChanged)
	Q_PROPERTY(Vec3f azimuthalGridColor
			   READ getAzimuthalGridColor
			   WRITE setAzimuthalGridColor
			   NOTIFY azimuthalGridColorChanged)
	Q_PROPERTY(bool equatorGridDisplayed
			   READ isEquatorGridDisplayed
			   WRITE setEquatorGridDisplayed
			   NOTIFY equatorGridDisplayedChanged)
	Q_PROPERTY(Vec3f equatorGridColor
			   READ getEquatorGridColor
			   WRITE setEquatorGridColor
			   NOTIFY equatorGridColorChanged)
	Q_PROPERTY(bool equatorJ2000GridDisplayed
			   READ isEquatorJ2000GridDisplayed
			   WRITE setEquatorJ2000GridDisplayed
			   NOTIFY equatorJ2000GridDisplayedChanged)
	Q_PROPERTY(Vec3f equatorJ2000GridColor
			   READ getEquatorJ2000GridColor
			   WRITE setEquatorJ2000GridColor
			   NOTIFY equatorJ2000GridColorChanged)
	Q_PROPERTY(bool galacticGridDisplayed
			   READ isGalacticGridDisplayed
			   WRITE setGalacticGridDisplayed
			   NOTIFY galacticGridDisplayedChanged)
	Q_PROPERTY(Vec3f galacticGridColor
			   READ getGalacticGridColor
			   WRITE setGalacticGridColor
			   NOTIFY galacticGridColorChanged)
	Q_PROPERTY(bool equatorLineDisplayed
			   READ isEquatorLineDisplayed
			   WRITE setEquatorLineDisplayed
			   NOTIFY equatorLineDisplayedChanged)
	Q_PROPERTY(Vec3f equatorLineColor
			   READ getEquatorLineColor
			   WRITE setEquatorLineColor
			   NOTIFY equatorLineColorChanged)
	Q_PROPERTY(bool eclipticLineDisplayed
			   READ isEclipticLineDisplayed
			   WRITE setEclipticLineDisplayed
			   NOTIFY eclipticLineDisplayedChanged)
	Q_PROPERTY(Vec3f eclipticLineColor
			   READ getEclipticLineColor
			   WRITE setEclipticLineColor
			   NOTIFY eclipticLineColorChanged)
	Q_PROPERTY(bool meridianLineDisplayed
			   READ isMeridianLineDisplayed
			   WRITE setMeridianLineDisplayed
			   NOTIFY meridianLineDisplayedChanged)
	Q_PROPERTY(Vec3f meridianLineColor
			   READ getMeridianLineColor
			   WRITE setMeridianLineColor
			   NOTIFY meridianLineColorChanged)
	Q_PROPERTY(bool horizonLineDisplayed
			   READ isHorizonLineDisplayed
			   WRITE setHorizonLineDisplayed
			   NOTIFY horizonLineDisplayedChanged)
	Q_PROPERTY(Vec3f horizonLineColor
			   READ getHorizonLineColor
			   WRITE setHorizonLineColor
			   NOTIFY horizonLineColorChanged)
	Q_PROPERTY(bool galacticPlaneLineDisplayed
			   READ isGalacticPlaneLineDisplayed
			   WRITE setGalacticPlaneLineDisplayed
			   NOTIFY galacticPlaneLineDisplayedChanged)
	Q_PROPERTY(Vec3f galacticPlaneLineColor
			   READ getGalacticPlaneLineColor
			   WRITE setGalacticPlaneLineColor
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
	void setAzimuthalGridDisplayed(const bool displayed);
	//! Accessor for displaying Azimuthal Grid.
	bool isAzimuthalGridDisplayed(void) const;
	//! Get the current color of the Azimuthal Grid.
	Vec3f getAzimuthalGridColor(void) const;
	//! Set the color of the Azimuthal Grid.
	void setAzimuthalGridColor(const Vec3f& newColor);

	//! Setter for displaying Equatorial Grid.
	void setEquatorGridDisplayed(const bool displayed);
	//! Accessor for displaying Equatorial Grid.
	bool isEquatorGridDisplayed(void) const;
	//! Get the current color of the Equatorial Grid.
	Vec3f getEquatorGridColor(void) const;
	//! Set the color of the Equatorial Grid.
	void setEquatorGridColor(const Vec3f& newColor);

	//! Setter for displaying Equatorial Grid.
	void setEquatorJ2000GridDisplayed(const bool displayed);
	//! Accessor for displaying Equatorial Grid.
	bool isEquatorJ2000GridDisplayed(void) const;
	//! Get the current color of the Equatorial J2000 Grid.
	Vec3f getEquatorJ2000GridColor(void) const;
	//! Set the color of the Equatorial J2000 Grid.
	void setEquatorJ2000GridColor(const Vec3f& newColor);

	//! Setter for displaying Galactic Grid.
	void setGalacticGridDisplayed(const bool displayed);
	//! Accessor for displaying Galactic Grid.
	bool isGalacticGridDisplayed(void) const;
	//! Get the current color of the Galactic Grid.
	Vec3f getGalacticGridColor(void) const;
	//! Set the color of the Galactic Grid.
	void setGalacticGridColor(const Vec3f& newColor);

	//! Setter for displaying Equatorial Line.
	void setEquatorLineDisplayed(const bool displayed);
	//! Accessor for displaying Equatorial Line.
	bool isEquatorLineDisplayed(void) const;
	//! Get the current color of the Equatorial Line.
	Vec3f getEquatorLineColor(void) const;
	//! Set the color of the Meridian Line.
	void setEquatorLineColor(const Vec3f& newColor);

	//! Setter for displaying Ecliptic Line.
	void setEclipticLineDisplayed(const bool displayed);
	//! Accessor for displaying Ecliptic Line.
	bool isEclipticLineDisplayed(void) const;
	//! Get the current color of the Ecliptic Line.
	Vec3f getEclipticLineColor(void) const;
	//! Set the color of the Meridian Line.
	void setEclipticLineColor(const Vec3f& newColor);

	//! Setter for displaying Meridian Line.
	void setMeridianLineDisplayed(const bool displayed);
	//! Accessor for displaying Meridian Line.
	bool isMeridianLineDisplayed(void) const;
	//! Get the current color of the Meridian Line.
	Vec3f getMeridianLineColor(void) const;
	//! Set the color of the Meridian Line.
	void setMeridianLineColor(const Vec3f& newColor);

	//! Setter for displaying Horizon Line.
	void setHorizonLineDisplayed(const bool displayed);
	//! Accessor for displaying Horizon Line.
	bool isHorizonLineDisplayed(void) const;
	//! Get the current color of the Horizon Line.
	Vec3f getHorizonLineColor(void) const;
	//! Set the color of the Horizon Line.
	void setHorizonLineColor(const Vec3f& newColor);

	//! Setter for displaying GalacticPlane Line.
	void setGalacticPlaneLineDisplayed(const bool displayed);
	//! Accessor for displaying GalacticPlane Line.
	bool isGalacticPlaneLineDisplayed(void) const;
	//! Get the current color of the GalacticPlane Line.
	Vec3f getGalacticPlaneLineColor(void) const;
	//! Set the color of the GalacticPlane Line.
	void setGalacticPlaneLineColor(const Vec3f& newColor);
signals:
	void azimuthalGridDisplayedChanged(const bool) const;
	void azimuthalGridColorChanged(const Vec3f & newColor) const;
	void equatorGridDisplayedChanged(const bool displayed) const;
	void equatorGridColorChanged(const Vec3f & newColor) const;
	void equatorJ2000GridDisplayedChanged(const bool displayed) const;
	void equatorJ2000GridColorChanged(const Vec3f & newColor) const;
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
	SkyGrid * aziGrid;      	// Azimuthal grid
	SkyLine * equatorLine;  	// Celestial Equator line
	SkyLine * eclipticLine; 	// Ecliptic line
	SkyLine * meridianLine; 	// Meridian line
	SkyLine * horizonLine;		// Horizon line
	SkyLine * galacticPlaneLine;	// line depciting the Galacitc plane as defined by the IAU definition of Galactic coordinates
};

#endif // _GRIDLINESMGR_HPP_

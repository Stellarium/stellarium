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
	//! Set flag for displaying Azimuthal Grid.
	void setFlagAzimuthalGrid(bool b);
	//! Get flag for displaying Azimuthal Grid.
	bool getFlagAzimuthalGrid(void) const;
	//! Get the current color of the Azimuthal Grid.
	Vec3f getColorAzimuthalGrid(void) const;
	//! Set the color of the Azimuthal Grid.
	void setColorAzimuthalGrid(const Vec3f& v);

	//! Set flag for displaying Equatorial Grid.
	void setFlagEquatorGrid(bool b);
	//! Get flag for displaying Equatorial Grid.
	bool getFlagEquatorGrid(void) const;
	//! Get the current color of the Equatorial Grid.
	Vec3f getColorEquatorGrid(void) const;
	//! Set the color of the Equatorial Grid.
	void setColorEquatorGrid(const Vec3f& v);

	//! Set flag for displaying Equatorial Grid.
	void setFlagEquatorJ2000Grid(bool b);
	//! Get flag for displaying Equatorial Grid.
	bool getFlagEquatorJ2000Grid(void) const;
	//! Get the current color of the Equatorial J2000 Grid.
	Vec3f getColorEquatorJ2000Grid(void) const;
	//! Set the color of the Equatorial J2000 Grid.
	void setColorEquatorJ2000Grid(const Vec3f& v);

	//! Set flag for displaying Galactic Grid.
	void setFlagGalacticGrid(bool b);
	//! Get flag for displaying Galactic Grid.
	bool getFlagGalacticGrid(void) const;
	//! Get the current color of the Galactic Grid.
	Vec3f getColorGalacticGrid(void) const;
	//! Set the color of the Galactic Grid.
	void setColorGalacticGrid(const Vec3f& v);

	//! Set flag for displaying Equatorial Line.
	void setFlagEquatorLine(bool b);
	//! Get flag for displaying Equatorial Line.
	bool getFlagEquatorLine(void) const;
	//! Get the current color of the Equatorial Line.
	Vec3f getColorEquatorLine(void) const;
	//! Set the color of the Meridian Line.
	void setColorEquatorLine(const Vec3f& v);

	//! Set flag for displaying Ecliptic Line.
	void setFlagEclipticLine(bool b);
	//! Get flag for displaying Ecliptic Line.
	bool getFlagEclipticLine(void) const;
	//! Get the current color of the Ecliptic Line.
	Vec3f getColorEclipticLine(void) const;
	//! Set the color of the Meridian Line.
	void setColorEclipticLine(const Vec3f& v);

	//! Set flag for displaying Meridian Line.
	void setFlagMeridianLine(bool b);
	//! Get flag for displaying Meridian Line.
	bool getFlagMeridianLine(void) const;
	//! Get the current color of the Meridian Line.
	Vec3f getColorMeridianLine(void) const;
	//! Set the color of the Meridian Line.
	void setColorMeridianLine(const Vec3f& v);

	//! Set flag for displaying Horizon Line.
	void setFlagHorizonLine(bool b);
	//! Get flag for displaying Horizon Line.
	bool getFlagHorizonLine(void) const;
	//! Get the current color of the Horizon Line.
	Vec3f getColorHorizonLine(void) const;
	//! Set the color of the Horizon Line.
	void setColorHorizonLine(const Vec3f& v);

	//! Set flag for displaying GalacticPlane Line.
	void setFlagGalacticPlaneLine(bool b);
	//! Get flag for displaying GalacticPlane Line.
	bool getFlagGalacticPlaneLine(void) const;
	//! Get the current color of the GalacticPlane Line.
	Vec3f getColorGalacticPlaneLine(void) const;
	//! Set the color of the GalacticPlane Line.
	void setColorGalacticPlaneLine(const Vec3f& v);


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

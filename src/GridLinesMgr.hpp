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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */
 
#ifndef GRIDLINES_H_
#define GRIDLINES_H_

#include <string>
#include "vecmath.h"
#include "StelModule.hpp"

class LoadingBar;
class Translator;
class ToneReproducer;
class Navigator;
class Projector;
class InitParser;

class SkyGrid;
class SkyLine;

using namespace std;

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
	//! ini parser object which is passed, and according to the settings there
	//! sets the visibility of the Equatorial Grid, Azimuthal Grid, Meridian Line,
	//! Equator Line and Ecliptic Line.
	//! @param conf the ini parser object which contains the grid settings.
	//! @param lb the LoadingBar object which is used to draw loading progress.
	virtual void init(const InitParser& conf, LoadingBar& lb);
	
	//! Get the module ID, returns, "gridlines".
	virtual QString getModuleID() const {return "GridLinesMgr";}
	
	//! Draw the grids and great circle lines.
	//! Draws the Equatorial Grid, Azimuthal Grid, Meridian Line, Equator Line 
	//! and Ecliptic Line according to the various flags which control their
	//! visibility.
	virtual double draw(Projector *prj, const Navigator *nav, ToneReproducer *eye);
	
	//! Update time-dependent features.
	//! Used to control fading when turning on and off the grid lines and great circles.
	virtual void update(double deltaTime);
	
	//! Sets the colors of the grids and great circles.
	//! Reads setting from an ini parser object and sets the colors of the 
	//! Equatorial Grid, Azimuthal Grid, Meridian Line, Equator Line and
	//! Ecliptic Line.
	//! @param conf the ini parser object which contains the color settings.
	//! @param section the section of the ini parser in which to locate the
	//! relevant settings.
	virtual void setColorScheme(const InitParser& conf, const QString& section);
	
	//! Used to determine the order in which the various modules are drawn.
	virtual double getCallOrder(StelModuleActionName actionName) const;
	
	///////////////////////////////////////////////////////////////////////////////////////
	// Setter and getters
public slots:
	//! Set flag for displaying Azimuthal Grid.
	void setFlagAzimutalGrid(bool b);
	//! Get flag for displaying Azimuthal Grid.
	bool getFlagAzimutalGrid(void) const;
	//! Get the current color of the Azimuthal Grid.
	Vec3f getColorAzimutalGrid(void) const;
	//! Set the color of the Azimuthal Grid.
	void setColorAzimutalGrid(const Vec3f& v);
	
	//! Set flag for displaying Equatorial Grid.
	void setFlagEquatorGrid(bool b);
	//! Get flag for displaying Equatorial Grid.
	bool getFlagEquatorGrid(void) const;
	//! Get the current color of the Equatorial Grid.
	Vec3f getColorEquatorGrid(void) const;
	//! Set the color of the Equatorial Grid.
	void setColorEquatorGrid(const Vec3f& v);
	
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
	
private:
	SkyGrid * equ_grid;					// Equatorial grid
	SkyGrid * azi_grid;					// Azimutal grid
	SkyLine * equator_line;				// Celestial Equator line
	SkyLine * ecliptic_line;			// Ecliptic line
	SkyLine * meridian_line;			// Meridian line
};

#endif /*GRIDLINES_H_*/

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

class GridLinesMgr : public StelModule
{
public:
	GridLinesMgr();
	virtual ~GridLinesMgr();

	///////////////////////////////////////////////////////////////////////////
	// Methods defined in the StelModule class
	virtual void init(const InitParser& conf, LoadingBar& lb);
	virtual string getModuleID() const {return "gridlines";}
	virtual double draw(Projector *prj, const Navigator *nav, ToneReproducer *eye);
	virtual void update(double deltaTime);
	virtual void setColorScheme(const InitParser& conf, const std::string& section);

	///////////////////////////////////////////////////////////////////////////////////////
	// Setter and getters
	//! Set flag for displaying Azimutal Grid
	void setFlagAzimutalGrid(bool b);
	//! Get flag for displaying Azimutal Grid
	bool getFlagAzimutalGrid(void) const;
	Vec3f getColorAzimutalGrid(void) const;

	//! Set flag for displaying Equatorial Grid
	void setFlagEquatorGrid(bool b);
	//! Get flag for displaying Equatorial Grid
	bool getFlagEquatorGrid(void) const;
	Vec3f getColorEquatorGrid(void) const;

	//! Set flag for displaying Equatorial Line
	void setFlagEquatorLine(bool b);
	//! Get flag for displaying Equatorial Line
	bool getFlagEquatorLine(void) const;
	Vec3f getColorEquatorLine(void) const;

	//! Set flag for displaying Ecliptic Line
	void setFlagEclipticLine(bool b);
	//! Get flag for displaying Ecliptic Line
	bool getFlagEclipticLine(void) const;
	Vec3f getColorEclipticLine(void) const;


	//! Set flag for displaying Meridian Line
	void setFlagMeridianLine(bool b);
	//! Get flag for displaying Meridian Line
	bool getFlagMeridianLine(void) const;
	Vec3f getColorMeridianLine(void) const;

	void setColorAzimutalGrid(const Vec3f& v);
	void setColorEquatorGrid(const Vec3f& v);
	void setColorEquatorLine(const Vec3f& v);
	void setColorEclipticLine(const Vec3f& v);
	void setColorMeridianLine(const Vec3f& v);
	
private:
	SkyGrid * equ_grid;					// Equatorial grid
	SkyGrid * azi_grid;					// Azimutal grid
	SkyLine * equator_line;				// Celestial Equator line
	SkyLine * ecliptic_line;			// Ecliptic line
	SkyLine * meridian_line;			// Meridian line
};

#endif /*GRIDLINES_H_*/

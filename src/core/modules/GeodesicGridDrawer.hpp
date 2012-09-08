/*
 * Stellarium
 * Copyright (C) 2006 Fabien Chereau
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

#ifndef _GEODESICGRIDDRAWER_HPP_
#define _GEODESICGRIDDRAWER_HPP_

#include "StelGeodesicGrid.hpp"
#include "StelModule.hpp"

class StelProjector;
class StelNavigator;
class ToneReproductor;

//GL-REFACTOR: This class seems to be unmaintained - 
//its interface does not match StelModule (e.g. draw() has different parameters)
class StelGeodesicGridDrawer : public StelModule
{
public:
	StelGeodesicGridDrawer(int level);
	virtual ~StelGeodesicGridDrawer();

	///////////////////////////////////////////////////////////////////////////
	// Methods defined in the StelModule class
	virtual void init();
	virtual double draw(StelCore* core, int maxSearchLevel);
	virtual void update(double deltaTime) {;}
	virtual void updateI18n() {;}
	virtual void updateSkyCulture() {;}
	
private:
//	StelGeodesicGrid* grid;
//	GeodesicSearchResult* searchResult;
	QFont font;
};

#endif // _GEODESICGRIDDRAWER_HPP_

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
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */

#ifndef GEODESICGRIDDRAWER_HPP
#define GEODESICGRIDDRAWER_HPP

#include "StelGeodesicGrid.hpp"
#include "StelModule.hpp"
#include <QFont>

class StelProjector;
class StelNavigator;
class ToneReproductor;

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

#endif // GEODESICGRIDDRAWER_HPP

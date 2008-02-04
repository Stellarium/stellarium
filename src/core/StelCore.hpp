/*
 * Copyright (C) 2003 Fabien Chereau
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

#ifndef _STEL_CORE_H_
#define _STEL_CORE_H_

#include <QString>

class Navigator;
class Projector;
class ToneReproducer;
class LoadingBar;
class Observer;
class GeodesicGrid;

//! @class StelCore 
//! Main class for Stellarium core processing.
//! This class provides services like management of sky projections,
//! tone conversion, or reference frame conversion. It is used by the
//! various StelModules to update and display themself.
//! There is currently only one StelCore instance in Stellarium, but
//! in the future they may be more, allowing for example to display 
//! several independent views of the sky at the same time.
//! @author Fabien Chereau
class StelCore
{
public:
	StelCore();
	virtual ~StelCore();

	//! Init and load all main core components.
	void init();

	//! Update all the objects with respect to the time.
	//! @param deltaTime the time increment in sec.
	void update(double deltaTime);

	//! Update core state before drawing modules.
	void preDraw();

	//! Update core state after drawing modules.
	void postDraw();

	//! Get the current projector used in the core.
	Projector* getProjection() {return projection;}
	//! Get the current projector used in the core.
	const Projector* getProjection() const {return projection;}

	//! Get the current navigation (manages frame transformation) used in the core.
	Navigator* getNavigation() {return navigation;}
	//! Get the current navigation (manages frame transformation) used in the core.
	const Navigator* getNavigation() const {return navigation;}

	//! Get the current tone reproducer used in the core.
	ToneReproducer* getToneReproducer() {return tone_converter;}
	//! Get the current tone reproducer used in the core.
	const ToneReproducer* getToneReproducer() const {return tone_converter;}

	//! Get the current observer description.
	Observer* getObservatory() {return observatory;}
	//! Get the current observer description.
	const Observer* getObservatory() const {return observatory;}

	//! Get the shared instance of GeodesicGrid
	GeodesicGrid* getGeodesicGrid() {return geodesic_grid;}
	//! Get the shared instance of GeodesicGrid
	const GeodesicGrid* getGeodesicGrid() const {return geodesic_grid;}
	
private:
	Navigator* navigation;			// Manage all navigation parameters, coordinate transformations etc..
	Observer* observatory;			// Manage observer position
	Projector* projection;			// Manage the projection mode and matrix
	ToneReproducer* tone_converter;		// Tones conversion between stellarium world and display device
	class MovementMgr* movementMgr;		// Manage vision movements
	
	// Manage geodesic grid
	GeodesicGrid* geodesic_grid;
};

#endif // _STEL_CORE_H_

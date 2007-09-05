/*
 * Stellarium
 * Copyright (C) 2007 Guillaume Chereau
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
 
#ifndef _GRID_HPP_
#define _GRID_HPP_

#include <list>

#include "GridObject.hpp"
#include "SphereGeometry.hpp"

using namespace StelGeom;

class Grid : public std::vector<GridObject*>
{
public:
    Grid() {}
    
    virtual ~Grid() {}

	//! Preselect all the objects in the given area
    virtual void filterIntersect(const ConvexS& s) {;}
    
	//! Get all the objects loaded into the grid structure
	virtual std::vector<GridObject*> getAllObjects() = 0;

	//! Insert an element in the resulting object list
	void insertResult(GridObject* obj)
    {
		std::vector<GridObject*>::push_back(obj);
    }
	
	//! Insert several elements in the resulting object list
	void insertResult(const std::vector<GridObject*>& objs)
	{
		std::vector<GridObject*>::insert(std::vector<GridObject*>::end(), objs.begin(), objs.end());
	}
};

#endif // _GRID_HPP_


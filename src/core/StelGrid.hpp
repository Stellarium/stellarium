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
 
#ifndef _STELGRID_HPP_
#define _STELGRID_HPP_

#include "StelGridObject.hpp"
#include "SphereGeometry.hpp"

using namespace StelGeom;

class StelGrid : public std::vector<StelGridObject*>
{
public:
    StelGrid() {}
    
    virtual ~StelGrid() {}

	//! Preselect all the objects in the given area
    virtual void filterIntersect(const ConvexS& s) {;}
    
	//! Get all the objects loaded into the grid structure
	virtual std::vector<StelGridObject*> getAllObjects() = 0;

	//! Insert an element in the resulting object list
	void insertResult(StelGridObject* obj)
    {
		std::vector<StelGridObject*>::push_back(obj);
    }
	
	//! Insert several elements in the resulting object list
	void insertResult(const std::vector<StelGridObject*>& objs)
	{
		std::vector<StelGridObject*>::insert(std::vector<StelGridObject*>::end(), objs.begin(), objs.end());
	}
};

class SimpleGrid : public StelGrid
{
public:
	SimpleGrid() {}

	~SimpleGrid() {}
	void insert(StelGridObject* obj)
	{
		all.push_back(obj);
	}

	template<class Shape>
			void filterIntersect(const Shape& s);

	//! Get all the object loaded into the grid
	virtual std::vector<StelGridObject*> getAllObjects() {return all;}

private:
	typedef std::vector<StelGridObject*> AllObjects;
	AllObjects all;
};

template<class Shape> void SimpleGrid::filterIntersect(const Shape& s)
{
	// This is a totaly non-optimized method
	// because we recompute everything each time, when we
	// should use the previous informations we have
	this->clear();
	for (AllObjects::iterator iter = all.begin(); iter != all.end(); ++iter)
	{
		if (intersect(s, (*iter)->getPositionForGrid()))
		{
			this->StelGrid::insertResult(*iter);
		}
	}
}

#endif // _STELGRID_HPP_


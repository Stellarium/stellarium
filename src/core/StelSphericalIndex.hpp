/*
 * Stellarium
 * Copyright (C) 2009 Fabien Chereau
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

#ifndef _STELSPHERICALINDEX_HPP_
#define _STELSPHERICALINDEX_HPP_

#define MAX_INDEX_LEVEL 8

#include "StelRegionObject.hpp"

class RootNode;

//! @class StelSphericalIndex
//! Container allowing to store and query SphericalRegion.
class StelSphericalIndex
{
public:
	StelSphericalIndex(int maxObjectsPerNode = 100);
	virtual ~StelSphericalIndex();

	//! Insert the given object in the StelSphericalIndex.
	void insert(StelRegionObjectP obj);

	//! Process all the objects intersecting the given region using the passed function object.
	template<class FuncObject> void processIntersectingRegions(const SphericalRegionP region, FuncObject func);

	//! Process all the objects intersecting the given region using the passed function object.
	template<class FuncObject> void processAll(FuncObject func);

private:
	
	//! The maximum allowed number of object per node.
	int maxObjectsPerNode;
	
	//! One tree per object radius. Each query is propagated to each tree with the proper additional margin.
	RootNode* treeForRadius[MAX_INDEX_LEVEL];
	
	double cosRadius[MAX_INDEX_LEVEL];
};

#endif // _STELSPHERICALINDEX_HPP_



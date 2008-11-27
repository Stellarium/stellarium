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
 
#ifndef _TREEGRID_HPP_
#define _TREEGRID_HPP_

//#define TREEGRIDDEBUG 1

#include <vector>
#include "StelGrid.hpp"

struct TreeGridNode
{
    TreeGridNode() {}
    TreeGridNode(const StelGeom::ConvexPolygon& s) : triangle(s) {}
        
	typedef std::vector<StelGridObject*> Objects;
    Objects objects;
    
    StelGeom::ConvexPolygon triangle;
    
    typedef std::vector<TreeGridNode> Children;
    Children children;
    
#ifdef TREEGRIDDEBUG
	double draw(class Projector *prj, const StelGeom::ConvexS&, float opacity = 1.) const;
#endif
};

class TreeGrid : public StelGrid, public TreeGridNode
{
public:
    TreeGrid(unsigned int maxobj = 1000);
    virtual ~TreeGrid();
    
	void insert(StelGridObject* obj)
    {
        insert(obj, *this);
    }
    
    template<class Shape>
    void filterIntersect(const Shape& s);
    
    unsigned int depth() const
    { return depth(*this); }
    
	//! Get all the objects loaded into the grid structure
	//! This is quite slow and should not used for time critical operations
	virtual std::vector<StelGridObject*> getAllObjects();
	
private:
    
	void insert(StelGridObject* obj, TreeGridNode& node);
    void split(TreeGridNode& node);
    
    template<class S>
    void fillIntersect(const S& s, const TreeGridNode& node, StelGrid& grid) const;    
    
    void fillAll(const TreeGridNode& node, StelGrid& grid) const;
	void fillAll(const TreeGridNode& node, std::vector<StelGridObject*>& result) const;
    unsigned int depth(const TreeGridNode& node) const;
    
    unsigned int maxObjects;
    
    // The last filter
    ConvexS filter;
};


template<class S>
void TreeGrid::fillIntersect(const S& s, const TreeGridNode& node, StelGrid& grid) const
{
    for (TreeGridNode::Objects::const_iterator io = node.objects.begin(); io != node.objects.end(); ++io)
    {
		if (intersect(s, (*io)->getPositionForGrid()))
        {
            grid.insertResult(*io);
        }
    }
    for (Children::const_iterator ic = node.children.begin(); ic != node.children.end(); ++ic)
    {
        if (contains(s, ic->triangle))
        {
            fillAll(*ic, grid);
        }
        else
		{
			if(intersect(s, ic->triangle))
        	{
            	fillIntersect(s, *ic, grid);
        	}
		}
    }
}

template<class Shape>
struct NotIntersectPred
{
    Shape shape;
    
    NotIntersectPred(const Shape& s) : shape(s) {}
	bool operator() (const StelGridObject* obj) const
    {
		return !intersect(shape, obj->getPositionForGrid());
    }
};

template<class Shape>
void TreeGrid::filterIntersect(const Shape& s)
{
    // first we remove all the objects that are not in the disk
//     this->remove_if(NotIntersectPred<Shape>(s));
//     // now we add all the objects that are in the disk, but not in the old disk
//     fillIntersect(Difference<Shape, ConvexS>(s, filter), *this, *this);
    this->clear();
    fillIntersect(s, *this, *this);
    
    //filter = ConvexS(s);
}


#endif // _TREEGRID_HPP_


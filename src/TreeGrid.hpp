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

#include <vector>

#include "Grid.hpp"

struct TreeGridNode
{
    TreeGridNode() {}
    TreeGridNode(const ConvexPolygon& s) : triangle(s) {}
        
    typedef std::vector<StelObject*> Objects;
    Objects objects;
    
    ConvexPolygon triangle;
    
    typedef std::vector<TreeGridNode> Children;
    Children children;
};

class TreeGrid : public Grid, public TreeGridNode
{
public:
    TreeGrid(Navigator* nav = NULL, unsigned int maxobj = 1000);
    ~TreeGrid() {}
    
    void insert(StelObject* obj)
    {
        insert(obj, *this);
    }
    
    void filterIntersect(const Disk& s);
    
    unsigned int depth() const
    { return depth(*this); }
    
private:
    
    void insert(StelObject* obj, TreeGridNode& node);
    void split(TreeGridNode& node);
    
    template<class S>
    void fillIntersect(const S& s, const TreeGridNode& node, Grid& grid) const;    
    
    void fillAll(const TreeGridNode& node, Grid& grid) const;
    unsigned int depth(const TreeGridNode& node) const;
    
    unsigned int maxObjects;
    
    // The last filter
    Disk filter;
    
};


template<class S>
void TreeGrid::fillIntersect(const S& s, const TreeGridNode& node,
                                Grid& grid) const
{
    for (TreeGridNode::Objects::const_iterator io = node.objects.begin(); io != node.objects.end(); ++io)
    {
        if (intersect(s, (*io)->getObsJ2000Pos(navigator)))
        {
            grid.insert(*io);
        }
    }
    for (Children::const_iterator ic = node.children.begin(); ic != node.children.end(); ++ic)
    {
        if (contains(s, ic->triangle))
        {
            fillAll(*ic, grid);
        }
        else if(intersect(s, ic->triangle))
        {
            fillIntersect(s, *ic, grid);
        }
    }
}

#endif // _TREEGRID_HPP_


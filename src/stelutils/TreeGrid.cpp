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
 
#include <cassert>
#include "TreeGrid.hpp"

TreeGrid::TreeGrid(unsigned int maxobj):
    maxObjects(maxobj), filter(Vec3d(0,0,0), 0)
{
    // We create the initial triangles forming a tetrahedron :
    const int vertexes[4][3] =
    {
        {+1, +1, +1},
        {-1, +1, -1},
        {-1, -1, +1},
        {+1, -1, -1},
    };
    const int triangles[4][3] =
    {
        {3, 0, 2},
        {1, 2, 0},
        {2, 1, 3},
        {3, 1, 0}
    };
    
    for (int i = 0; i < 4; ++i) {
        int i0 = triangles[i][0];
        int i1 = triangles[i][1];
        int i2 = triangles[i][2];
        
        Vec3d v0(vertexes[i0][0], vertexes[i0][1], vertexes[i0][2]);
        v0.normalize();
        Vec3d v1(vertexes[i1][0], vertexes[i1][1], vertexes[i1][2]);
        v1.normalize();
        Vec3d v2(vertexes[i2][0], vertexes[i2][1], vertexes[i2][2]);
        v2.normalize();
        
        children.push_back(ConvexPolygon(v0, v1, v2));
    }
}

void TreeGrid::insert(GridObject* obj, TreeGridNode& node)
{
    if (node.children.empty())
    {
        node.objects.push_back(obj);
        // If we have too many objects in the node, we split it
        if (node.objects.size() >= maxObjects)
        {
            split(node);
            TreeGridNode::Objects node_objects;
            std::swap(node_objects, node.objects);
            for (TreeGridNode::Objects::iterator iter = node_objects.begin();
                    iter != node_objects.end(); ++iter)
            {
                insert(*iter, node);
            }
        }
    }
    else // if we have children
    {
        for (TreeGridNode::Children::iterator iter = node.children.begin();
                iter != node.children.end(); ++iter)
        {
			if (contains(iter->triangle, obj->getPositionForGrid())) {
                insert(obj, *iter);
                return;
            }
        }
        node.objects.push_back(obj);
    }
}

void TreeGrid::split(TreeGridNode& node)
{
    assert(node.children.empty());
    const Polygon& p = node.triangle;
    
    assert(p.size() == 3);
    
    const Vec3d& e0 = p[0];
    const Vec3d& e1 = p[1];
    const Vec3d& e2 = p[2];
    
    Vec3d e01 = e0 + e1;
    e01.normalize();
    Vec3d e12 = e1 + e2;
    e12.normalize();
    Vec3d e20 = e2 + e0;
    e20.normalize();
    
    node.children.push_back(ConvexPolygon(e0, e01, e20));
    node.children.push_back(ConvexPolygon(e1, e12, e01));
    node.children.push_back(ConvexPolygon(e2, e20, e12));
    node.children.push_back(ConvexPolygon(e01, e12, e20));
}



void TreeGrid::fillAll(const TreeGridNode& node, Grid& grid) const
{
    for (Objects::const_iterator io = node.objects.begin(); io != node.objects.end(); ++io)
    {
        grid.insert(*io);
    }
    for (Children::const_iterator ic = node.children.begin(); ic != node.children.end(); ++ic)
    {
        fillAll(*ic, grid);
    }
}

unsigned int TreeGrid::depth(const TreeGridNode& node) const
{
    if (node.children.empty()) return 0;
    unsigned int max = 0;
    for (Children::const_iterator ic = node.children.begin(); ic != node.children.end(); ++ic)
    {
        unsigned int d = depth(*ic);
        if (d > max) max = d;
    }
    return max + 1;
}

struct NotIntersectPred
{
    Disk shape;
    
    NotIntersectPred(const Disk& s) : shape(s) {}
    bool operator() (const GridObject* obj) const
    {
		return !intersect(shape, obj->getPositionForGrid());
    }
};

void TreeGrid::filterIntersect(const Disk& s)
{
    // first we remove all the objects that are not in the disk
    this->remove_if(NotIntersectPred(s));
    // now we add all the objects that are in the disk, but not in the old disk
    fillIntersect(Difference<Disk, Disk>(s, filter), *this, *this);
    // this->clear();
    // fillIntersect(s, *this, *this);
    
    filter = s;
}



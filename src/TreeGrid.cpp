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
 
#include "TreeGrid.hpp"


// This method is called when we need to split a triangle
// into smaller sub triangles
std::list<const ConvexPolygon*> TreeGridPolicyBase::split(const Polygon* p)
{
    assert(p->size() == 3);
    std::list<const ConvexPolygon*> ret;
    const Vec3d& e0 = (*p)[0];
    const Vec3d& e1 = (*p)[1];
    const Vec3d& e2 = (*p)[2];
    
    Vec3d e01 = e0 + e1;
    e01.normalize();
    Vec3d e12 = e1 + e2;
    e12.normalize();
    Vec3d e20 = e2 + e0;
    e20.normalize();
    
    ConvexPolygon* s1 = new ConvexPolygon(e0, e01, e20);
    ret.push_back(s1);
    
    ConvexPolygon* s2 = new ConvexPolygon(e1, e12, e01);
    ret.push_back(s2);
    
    ConvexPolygon* s3 = new ConvexPolygon(e2, e20, e12);
    ret.push_back(s3);
    
    ConvexPolygon* s4 = new ConvexPolygon(e01, e12, e20);
    ret.push_back(s4);

    return ret;
}

//! The only empty convex
const ConvexPolygon TreeGridBase::_empty_convex;


std::list<const ConvexPolygon*> TreeGridBase::create_tetrahedron() const
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
    
    std::list<const ConvexPolygon*> ret;
    
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
        
        ConvexPolygon* triangle = new ConvexPolygon(v0, v1, v2);
        ret.push_back(triangle);
    }
    
    return ret;
}


//! Destructor
TreeGridBase::~TreeGridBase()
{
    typedef std::vector<const ConvexPolygon*>::iterator Iter;
    for (Iter i = _shapes.begin(); i != _shapes.end(); ++i) {
        delete *i;
    }
}


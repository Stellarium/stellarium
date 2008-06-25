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

static const double icosahedron_G = 0.5*(1.0+std::sqrt(5.0));
static const double icosahedron_b = 1.0/std::sqrt(1.0+icosahedron_G*icosahedron_G);
static const double icosahedron_a = icosahedron_b*icosahedron_G;

static const Vec3d icosahedron_corners[12] =
{
	Vec3d( icosahedron_a, -icosahedron_b,            0.0),
	Vec3d( icosahedron_a,  icosahedron_b,            0.0),
	Vec3d(-icosahedron_a,  icosahedron_b,            0.0),
	Vec3d(-icosahedron_a, -icosahedron_b,            0.0),
	Vec3d(           0.0,  icosahedron_a, -icosahedron_b),
	Vec3d(           0.0,  icosahedron_a,  icosahedron_b),
	Vec3d(           0.0, -icosahedron_a,  icosahedron_b),
	Vec3d(           0.0, -icosahedron_a, -icosahedron_b),
	Vec3d(-icosahedron_b,            0.0,  icosahedron_a),
	Vec3d( icosahedron_b,            0.0,  icosahedron_a),
	Vec3d( icosahedron_b,            0.0, -icosahedron_a),
	Vec3d(-icosahedron_b,            0.0, -icosahedron_a)
};

static const int icosahedron_triangles[20][3] =
{
 { 1, 0,10}, //  1
 { 0, 1, 9}, //  0
 { 0, 9, 6}, // 12
 { 9, 8, 6}, //  9
 { 0, 7,10}, // 16
 { 6, 7, 0}, //  6
 { 7, 6, 3}, //  7
 { 6, 8, 3}, // 14
 {11,10, 7}, // 11
 { 7, 3,11}, // 18
 { 3, 2,11}, //  3
 { 2, 3, 8}, //  2
 {10,11, 4}, // 10
 { 2, 4,11}, // 19
 { 5, 4, 2}, //  5
 { 2, 8, 5}, // 15
 { 4, 1,10}, // 17
 { 4, 5, 1}, //  4
 { 5, 9, 1}, // 13
 { 8, 9, 5}  //  8
};

TreeGrid::TreeGrid(unsigned int maxobj) : maxObjects(maxobj), filter()
{
	for (int i=0;i<20;++i)
	{
		const int* corners = icosahedron_triangles[i];
		children.push_back(ConvexPolygon(icosahedron_corners[corners[2]],icosahedron_corners[corners[1]], icosahedron_corners[corners[0]]));
	}
}

TreeGrid::~TreeGrid()
{
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
            for (TreeGridNode::Objects::iterator iter = node_objects.begin();iter != node_objects.end(); ++iter)
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

	const Vec3d& c0 = p[0];
	const Vec3d& c1 = p[1];
	const Vec3d& c2 = p[2];
	
	assert((c1^c0)*c2 >= 0.0);
	Vec3d e0 = c1+c2;
	e0.normalize();
	Vec3d e1 = c2+c0;
	e1.normalize();
	Vec3d e2 = c0+c1;
	e2.normalize();
	
	node.children.push_back(ConvexPolygon(e1,e2,c0));
	node.children.push_back(ConvexPolygon(e0,c1,e2));
	node.children.push_back(ConvexPolygon(c2,e0,e1));
	node.children.push_back(ConvexPolygon(e2,e1,e0));
}

void TreeGrid::fillAll(const TreeGridNode& node, Grid& grid) const
{
	grid.insertResult(node.objects);
	for (Children::const_iterator ic = node.children.begin(); ic != node.children.end(); ++ic)
	{
		fillAll(*ic, grid);
	}
}

void TreeGrid::fillAll(const TreeGridNode& node, std::vector<GridObject*>& result) const
{
	result.insert(result.end(), node.objects.begin(), node.objects.end());
	for (Children::const_iterator ic = node.children.begin(); ic != node.children.end(); ++ic)
	{
		fillAll(*ic, result);
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

/*************************************************************************
 Get all the objects loaded into the grid structure
*************************************************************************/
std::vector<GridObject*> TreeGrid::getAllObjects()
{
	std::vector<GridObject*> result;
	fillAll(*this, result);
	return result;
}
	
#if 1
#include "Projector.hpp"
#include "Navigator.hpp"				 
double TreeGridNode::draw(Projector *prj, const StelGeom::ConvexS& roi, float opacity) const
{
	glDisable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);	
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // Normal transparency mode
	glColor4f(0,1,0, opacity);
	Vec3d e1, e2;
	if (children.size()==0)
	{
		if (prj->projectLineCheck(triangle[0], e1, triangle[1], e2))
		{
			glBegin(GL_LINES);
			glVertex2f(e1[0], e1[1]);
			glVertex2f(e2[0], e2[1]);
			glEnd();
		}
		if (prj->projectLineCheck(triangle[1], e1, triangle[2], e2))
		{
			glBegin(GL_LINES);
			glVertex2f(e1[0], e1[1]);
			glVertex2f(e2[0], e2[1]);
			glEnd();
		}
		if (prj->projectLineCheck(triangle[2], e1, triangle[0], e2))
		{
			glBegin(GL_LINES);
			glVertex2f(e1[0], e1[1]);
			glVertex2f(e2[0], e2[1]);
			glEnd();
		}
	}
	opacity *= 0.75;
	for (Children::const_iterator ic = children.begin(); ic != children.end(); ++ic)
	{
		ic->draw(prj, roi, opacity);
	}
	return 0.;
}
#endif

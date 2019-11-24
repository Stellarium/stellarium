/*
 * Stellarium Scenery3d Plug-in
 *
 * Copyright (C) 2011-2015 Simon Parzer, Peter Neubauer, Georg Zotti, Andrei Borza, Florian Schaukowitsch
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

#ifndef HEIGHTMAP_HPP
#define HEIGHTMAP_HPP

#include "StelOBJ.hpp"

//! This represents a heightmap for viewer-ground collision
class Heightmap
{
public:
	typedef QVector<unsigned int> IdxList;
	typedef QVector<Vec3f> PosList;

        //! Construct a heightmap from a loaded OBJ mesh.
        //! The mesh is stored as reference and used for calculations.
        //! @param obj Mesh for building the heightmap.
	Heightmap();
        virtual ~Heightmap();

	//! Sets the mesh data to use. If the bbox is given, min/max calculation is skipped and its values are taken.
	void setMeshData(const IdxList& indexList, const PosList& posList, const AABBox *bbox = Q_NULLPTR);

        //! Get z Value at (x,y) coordinates.
        //! In case of ambiguities always returns the maximum height.
        //! @param x x-value
        //! @param y y-value
        //! @return z-Value at position given by x and y
        float getHeight(const float x, const float y) const;

        //! set/retrieve default height
        void setNullHeight(float h){nullHeight=h;}
        float getNullHeight() const {return nullHeight;}

private:
	IdxList indexList;
	PosList posList;

        static const int GRID_LENGTH = 60; // # of grid spaces is GRID_LENGTH^2

	typedef QVector<const unsigned int*> FaceVector; //points to first index in Index list for a face

	struct QuadTreeNode
	{
		//! Constructs a root node
		QuadTreeNode(const Vec2f& min, const Vec2f& max);
		//deletes all child nodes
		~QuadTreeNode();

		//indexes the child nodes, INVALID means outside the current node
		enum Quadrant { MinMin, MinMax, MaxMin, MaxMax, INVALID };

		//Stores the index of the given triangle in the QuadTree
		void putTriangle(const unsigned int* pTriangle, const PosList& posList, int maxLevel = 7);

		float getHeightAtPoint(const Vec2f& point, const PosList &posList) const;

		//Recursively retrieves all triangles which *potentially* lie at the given point
		FaceVector getTriangleCandidatesAtPoint(const Vec2f& point) const;

	private:
		//! Constructs a child node
		QuadTreeNode();
		void init(QuadTreeNode* parent, const Vec2f& min, const Vec2f& max);

		//turns this node from a leaf node into a normal node by adding 4 children
		void subdivide();

		//gets the subquadrant in which the specified point lies
		Quadrant getQuadrantForPoint(const Vec2f& point) const;

		//the parent node in case of a non-root node
		QuadTreeNode* parent;
		//either Q_NULLPTR in case of a leaf, or array of size 4
		QuadTreeNode* children;

		//! The level of this node, starting with 0 for the root node
		int level;
		//! The total depth of this sub-tree, starting with 1 for the root node without children
		int depth;
		//! The total nodecount of this sub-tree, including this node
		int nodecount;
		//contains faces which do not fit wholly in a subnode
		FaceVector faces;

		Vec2f min, max, mid;
	} *rootNode;

        struct GridSpace {
                FaceVector faces;
		float getHeight(const PosList &posList, const float x, const float y) const;
        };

        GridSpace* grid;
	Vec2f min, max, range;
        float nullHeight; // return value for areas outside grid

	void initQuadtree();
        void initGrid();
        GridSpace* getSpace(const float x, const float y) const ;
	static bool triangle_intersects_bbox(const Vec2f &t1, const Vec2f &t2, const Vec2f &t3, const Vec2f &rMin, const Vec2f &rMax);
	//! Check whether points p and q lie on the same side of line ab, helper for line_intersects_triangle
	inline static bool sameSide(const Vec2f& p, const Vec2f& q, const Vec2f& a, const Vec2f& b);
	static bool line_intersects_triangle(const Vec2f &t1, const Vec2f &t2, const Vec2f &t3, const Vec2f &p0, const Vec2f &p1);
	//! Computes the barycentric coordinates of p inside the triangle t1,t2,t3
	static void cartesian_to_barycentric(const Vec2f& t1, const Vec2f& t2, const Vec2f& t3, const Vec2f& p, float* l1, float* l2, float* l3);
	static float face_height_at(const PosList &obj, const unsigned int *pTriangle, const float x, const float y);
};

#endif // HEIGHTMAP_HPP

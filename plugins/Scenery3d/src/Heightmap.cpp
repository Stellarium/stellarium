/*
 * Stellarium Scenery3d Plug-in
 *
 * Copyright (C) 2011-2016 Simon Parzer, Peter Neubauer, Georg Zotti, Andrei Borza, Florian Schaukowitsch
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

#include <limits>

#include "Heightmap.hpp"
#include "VecMath.hpp"
#include "GeomMath.hpp"

#include <QElapsedTimer>

#define INF (std::numeric_limits<float>::max())
#define NO_HEIGHT (-INF)

Heightmap::Heightmap() : rootNode(Q_NULLPTR), grid(Q_NULLPTR), nullHeight(0.0)
{
}

Heightmap::~Heightmap()
{
	delete rootNode;
	delete[] grid;
}

void Heightmap::setMeshData(const IdxList &indexList, const PosList &posList, const AABBox* bbox)
{
	this->indexList = indexList;
	this->posList = posList;

	//re-calc min/max
	min = Vec2f(std::numeric_limits<float>::max());
	max = Vec2f(-std::numeric_limits<float>::max());
	for(int i = 0;i<posList.size();++i)
	{
		min[0] = std::min(min[0], posList.at(i)[0]);
		min[1] = std::min(min[1], posList.at(i)[1]);
		max[0] = std::max(max[0], posList.at(i)[0]);
		max[1] = std::max(max[1], posList.at(i)[1]);
	}

	if(bbox)
	{
		Q_ASSERT(bbox->min[0] == min[0]);
		Q_ASSERT(bbox->min[1] == min[1]);
		Q_ASSERT(bbox->max[0] == max[0]);
		Q_ASSERT(bbox->max[1] == max[1]);
	}

	range = max - min;

	QElapsedTimer timer;
	/*timer.start();
	this->initQuadtree();
	qDebug()<<"initQuadtree\t"<<qSetFieldWidth(12)<<right<<timer.nsecsElapsed();*/
	timer.start();
	this->initGrid();
	qDebug()<<"initGrid\t\t"<<qSetFieldWidth(12)<<right<<timer.nsecsElapsed();
}

/**
 * Returns the height of the ground model for any observer x/y coords.
 * The height is the highest z value of the ground model at these
 * coordinates.
 */
float Heightmap::getHeight(const float x, const float y) const
{
	/*QElapsedTimer timer;
	timer.start();
	if(!rootNode)
		return nullHeight;
	float height = rootNode->getHeightAtPoint(Vec2f(x,y),posList);
	qint64 qtTime = timer.nsecsElapsed();
	timer.start();*/

	Heightmap::GridSpace* space = getSpace(x, y);
	if (space == Q_NULLPTR)
	{
		return nullHeight;
	}
	else
	{
		float h = space->getHeight(posList, x, y);
		//qint64 gridTime = timer.nsecsElapsed();
		//qDebug()<<"qt"<<qtTime<<"grid"<<gridTime;
		//Q_ASSERT(height == h);
		if (h == NO_HEIGHT)
		{
			return nullHeight;
		}
		else
		{
			return h;
		}
	}
}

/**
 * Height query within a single grid space. The list of faces to check
 * for intersection with the observer coords is limited to faces
 * intersecting this grid space.
 */
float Heightmap::GridSpace::getHeight(const PosList& posList, const float x, const float y) const
{
	float h = NO_HEIGHT;

	// this was actually broken for a very long time
	// and checked ALL faces instead of only the ones in the grid cell...
	for(int i=0; i<faces.size(); ++i)
	{
		//points into the index list
		const unsigned int* pTriangle = faces[i];

		float face_h = face_height_at(posList, pTriangle, x, y);
		if(face_h > h)
		{
			h = face_h;
		}
	}

	return h;
}

void Heightmap::initQuadtree()
{
	delete rootNode;
	//TODO enforce square aspect ratio?
	rootNode = new QuadTreeNode(min,max);

	//add all triangles to the tree
	for(int i = 0;i<indexList.size();i+=3)
	{
		rootNode->putTriangle(&indexList.at(i),posList);
	}
}

/**
 * Fills every grid space with
 * pointers to the faces in that space.
 */
void Heightmap::initGrid()
{
	delete[] grid;
	grid = new GridSpace[GRID_LENGTH*GRID_LENGTH];

	for(int i = 0;i<indexList.size(); i+=3)
	{
		const unsigned int* pTriangle = &(indexList.at(i));
		//determine the triangle BBox
		Vec2f triMin(std::numeric_limits<float>::max()), triMax(-std::numeric_limits<float>::max());
		for(int t=0; t<3; ++t)
		{
			const Vec3f& pos = posList.at(pTriangle[t]);
			triMin[0] = std::min(triMin[0], pos[0]);
			triMin[1] = std::min(triMin[1], pos[1]);
			triMax[0] = std::max(triMax[0], pos[0]);
			triMax[1] = std::max(triMax[1], pos[1]);
		}

		//determine the grid faces the BBox overlaps with
		//convert to grid-coordinate system
		triMin = (triMin - min) / range;
		triMax = (triMax - min) / range;
		//convert to indices and clamp
		Vec2i minIdx(triMin * GRID_LENGTH);
		minIdx = minIdx.clamp(Vec2i(0),Vec2i(GRID_LENGTH-1));
		Vec2i maxIdx(triMax * GRID_LENGTH);
		maxIdx = maxIdx.clamp(Vec2i(0),Vec2i(GRID_LENGTH-1));
		//now iterate over the candidates
		for(int y = minIdx[1];y<=maxIdx[1];++y)
		{
			for(int x = minIdx[0];x<=maxIdx[0];++x)
			{
				//bounds of the current area
				Vec2f rectMin = min + (Vec2f(x,y) * range) / GRID_LENGTH;
				Vec2f rectMax = min + (Vec2f(x+1,y+1) * range) / GRID_LENGTH;

				//the three triangle points without Z
				const Vec2f t1(posList.at(pTriangle[0]).data());
				const Vec2f t2(posList.at(pTriangle[1]).data());
				const Vec2f t3(posList.at(pTriangle[2]).data());

				//more expensive check for intersection
				if(triangle_intersects_bbox(t1,t2,t3,rectMin,rectMax))
				{
					FaceVector* faces = &grid[y*GRID_LENGTH + x].faces;
					faces->push_back(pTriangle);
				}
			}
		}
	}
}

/**
 * Returns the GridSpace which covers the area around x/y.
 */
Heightmap::GridSpace* Heightmap::getSpace(const float x, const float y) const
{
	int ix = (x - min[0]) / (range[0]) * GRID_LENGTH;
	int iy = (y - min[1]) / (range[1]) * GRID_LENGTH;

	if ((ix < 0) || (ix >= GRID_LENGTH) || (iy < 0) || (iy >= GRID_LENGTH))
	{
		return Q_NULLPTR;
	}
	else
	{
		return &grid[iy*GRID_LENGTH + ix];
	}
}

/**
 * Returns the height of the face at the given point or -inf if
 * the coordinates are outside the bounds of the face.
 */
float Heightmap::face_height_at(const PosList& obj, const unsigned int* pTriangle, const float x, const float y)
{
	//Vertices in triangle
	const Vec3f& pVertex0 = obj.at(pTriangle[0]);
	const Vec3f& pVertex1 = obj.at(pTriangle[1]);
	const Vec3f& pVertex2 = obj.at(pTriangle[2]);

	float l1,l2,l3;
	cartesian_to_barycentric(Vec2f(pVertex0.data()),Vec2f(pVertex1.data()),Vec2f(pVertex2.data()), Vec2f(x,y), &l1, &l2, &l3);

	if ((l1 < 0) || (l2 < 0) || (l3 < 0))
	{
		return NO_HEIGHT; // (x,y) out of face bounds
	}
	else
	{
		return l1*pVertex0[2] + l2*pVertex1[2] + l3*pVertex2[2];
	}
}

void Heightmap::cartesian_to_barycentric(const Vec2f &t1, const Vec2f &t2, const Vec2f &t3, const Vec2f &p, float *l1, float *l2, float *l3)
{
	// Weight of those vertices is used to calculate exact height at (x,y), using barycentric coordinates, see also
	// http://en.wikipedia.org/wiki/Barycentric_coordinate_system_(mathematics)#Converting_to_barycentric_coordinates
	float det_T = (t2[1]-t3[1]) *
		      (t1[0]-t3[0]) +
		      (t3[0]-t2[0]) *
		      (t1[1]-t3[1]);

	*l1 = ((t2[1]-t3[1]) *
		    (p[0]-t3[0]) +
		    (t3[0]-t2[0]) *
		    (p[1]-t3[1]))/det_T;

	*l2 = ((t3[1]-t1[1]) *
		    (p[0]-t3[0]) +
		    (t1[0]-t3[0]) *
		    (p[1]-t3[1]))/det_T;

	*l3 = 1.0f - *l1 - *l2;
}

/**
 * Returns true if the given face intersects the given area.
 */
bool Heightmap::triangle_intersects_bbox(const Vec2f& t1, const Vec2f& t2, const Vec2f& t3, const Vec2f& rMin, const Vec2f& rMax)
{
	// test if ANY point is contained inside the other shape, if yes, we certainly intersect
	// This may also be true if no lines intersect (one shape fully inside the other)!
	//test if t1 is inside the rectangle
	if(t1[0]>=rMin[0] && t1[0]<=rMax[0] && t1[1]>=rMin[1] && t1[1]<=rMax[1])
		return true;
	//test if rMin is inside the triangle (barycentric coords)
	float l1,l2,l3;
	cartesian_to_barycentric(t1,t2,t3,rMin,&l1,&l2,&l3);
	if ((l1 >= 0) && (l2 >= 0) && (l3 >= 0))
		return true;

	//a triangle intersects the rectangle if any lines intersect each other
	//there are probably faster methods, but as this is not used during each frame it should be ok
	if(!line_intersects_triangle(t1,t2,t3,rMin,Vec2f(rMin[0],rMax[1])))
		if(!line_intersects_triangle(t1,t2,t3,rMin,Vec2f(rMax[0],rMin[1])))
			if(!line_intersects_triangle(t1,t2,t3,rMax,Vec2f(rMin[0],rMax[1])))
				if(!line_intersects_triangle(t1,t2,t3,rMax,Vec2f(rMax[0],rMin[1])))
				{
					return false; //no line intersects, so we have no intersection
				}
	//line intersection true
	return true;
}

bool Heightmap::sameSide(const Vec2f &p, const Vec2f &q, const Vec2f &a, const Vec2f &b)
{
	float z1 = (b[0] - a[0]) * (p[1] - a[1]) - (p[0] - a[0]) * (b[1] - a[1]);
	float z2 = (b[0] - a[0]) * (q[1] - a[1]) - (q[0] - a[0]) * (b[1] - a[1]);
	return z1 * z2 > .0f; //positive if on same side
}

bool Heightmap::line_intersects_triangle(const Vec2f &t0, const Vec2f &t1, const Vec2f &t2, const Vec2f &p0, const Vec2f &p1)
{
	//line/tri intersection from https://gamedev.stackexchange.com/revisions/21110/2
	if (sameSide(t0, t1, p0, p1) && sameSide(t0, t2, p0, p1)) return false;
	if (!sameSide(p0, t2, t0, t1) && !sameSide(p1, t2, t0, t1)) return false;
	if (!sameSide(p0, t0, t1, t2) && !sameSide(p1, t0, t1, t2)) return false;
	if (!sameSide(p0, t1, t2, t0) && !sameSide(p1, t1, t2, t0)) return false;

	return true;
}

Heightmap::QuadTreeNode::QuadTreeNode(const Vec2f &min, const Vec2f &max)
	: parent(Q_NULLPTR), children(Q_NULLPTR), level(-1), depth(-1), nodecount(-1)
{
	init(Q_NULLPTR,min,max);
}

Heightmap::QuadTreeNode::QuadTreeNode()
	: parent(Q_NULLPTR), children(Q_NULLPTR), level(-1), depth(-1), nodecount(-1)
{
}

void Heightmap::QuadTreeNode::init(QuadTreeNode* parent, const Vec2f& min, const Vec2f& max)
{
	this->parent = parent;
	this->min = min;
	this->max = max;
	//the midpoint of the quadrant
	this->mid = min + (max-min)/2.0f;

	//the depth of all parents will get incremented in the subdivide() call later
	depth = 1;
	nodecount = 1;

	if(parent)
	{
		this->level = parent->level+1;
	}
	else
	{
		level = 0;
	}
}

Heightmap::QuadTreeNode::~QuadTreeNode()
{
	delete[] children;
}

void Heightmap::QuadTreeNode::putTriangle(const unsigned int *pTriangle, const PosList &posList, int maxLevel)
{
	//we ignore the Z coordinates of the triangles
	Vec2f p1(posList.at(pTriangle[0]).data());
	Vec2f p2(posList.at(pTriangle[1]).data());
	Vec2f p3(posList.at(pTriangle[2]).data());
	Quadrant q1 = getQuadrantForPoint(p1);
	Quadrant q2 = getQuadrantForPoint(p2);
	Quadrant q3 = getQuadrantForPoint(p3);

	//if this happens something went wrong
	Q_ASSERT(q1 != INVALID);
	Q_ASSERT(q2 != INVALID);
	Q_ASSERT(q3 != INVALID);

	//if all three vertices lie in the same quadrant, it gets put into there!
	if(q1==q2 && q2==q3)
	{
		//check depth limit
		if(level<maxLevel)
		{
			//ensure child nodes exist
			subdivide();
			children[q1].putTriangle(pTriangle,posList);
			return;
		}
	}
	//it gets put into the current node
	faces.append(pTriangle);
}

float Heightmap::QuadTreeNode::getHeightAtPoint(const Vec2f &point, const Heightmap::PosList& posList) const
{
	float height = NO_HEIGHT;

	//lets avoid recursion here
	const QuadTreeNode* curNode = this;
	while(true)
	{
		Quadrant q = curNode->getQuadrantForPoint(point);
		if(q==INVALID) //outside bounds
			break;

		//iterate through all current faces,
		//calculate barycentric coordinates of point on the 2D-projected triangle,
		//and finally, if the point is inside, the height on the triangle
		for(int i=0;i<curNode->faces.size();++i)
		{
			const unsigned int* pTriangle = curNode->faces[i];
			height = std::max(height,face_height_at(posList,pTriangle,point[0],point[1]));
		}

		if(curNode->children)
			curNode = &(curNode->children[q]);
		else
			break;
	}
	return height;
}

Heightmap::FaceVector Heightmap::QuadTreeNode::getTriangleCandidatesAtPoint(const Vec2f &point) const
{
	FaceVector ret;

	//lets avoid recursion here
	const QuadTreeNode* curNode = this;
	while(true)
	{
		Quadrant q = curNode->getQuadrantForPoint(point);
		if(q==INVALID) //outside bounds
			break;

		//append all current node faces
		ret<<curNode->faces;
		if(children)
			curNode = &children[q];
		else
			break;
	}
	return ret;
}

void Heightmap::QuadTreeNode::subdivide()
{
	if(children)
		return; //already subdivided

	children = new QuadTreeNode[4]();
	children[MinMin].init(this,min,mid);
	children[MinMax].init(this,Vec2f(min[0],mid[1]),Vec2f(mid[0],max[1]));
	children[MaxMin].init(this,Vec2f(mid[0],min[1]),Vec2f(max[0],mid[1]));
	children[MaxMax].init(this,mid,max);

	//go back up the parent relations and update the depth, if required
	QuadTreeNode* curNode = this;

	do
	{
		//the node depth is the maximum depth of its subnodes + 1
		int maxDepth = 1;
		for(int i = 0;i<4;++i)
		{
			maxDepth = std::max(maxDepth,curNode->children[i].depth);
		}
		maxDepth+=1;
		if(maxDepth>curNode->depth)
		{
			//update depth
			curNode->depth = maxDepth;
		}
		//update nodecount
		curNode->nodecount+=4;
		//continue upwards to root
		curNode=curNode->parent;
	} while(curNode!=Q_NULLPTR);
}

Heightmap::QuadTreeNode::Quadrant Heightmap::QuadTreeNode::getQuadrantForPoint(const Vec2f &point) const
{
	//check if the point lies within our bounds first
	if(point[0]<min[0] || point[0]>max[0] || point[1]<min[1] || point[1]>max[1])
		return INVALID;

	if(point[0]<mid[0]) //left half
	{
		if(point[1]<mid[1])
			return MinMin; //bottom left
		return MinMax; //upper left
	}
	else //right half
	{
		if(point[1]<mid[1])
			return MaxMin; //bottom right
		return MaxMax; //upper right
	}
}

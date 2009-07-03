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

#include "StelSphericalIndex.hpp"
#include <QVector>

struct NodeElem
{
	NodeElem() {;}
	NodeElem(StelRegionObjectP aobj) : obj(aobj), cap(obj->getRegion()->getBoundingCap()) {;}
	StelRegionObjectP obj;
	SphericalCap cap;
};

struct Node
{
	QVector<NodeElem> elements;
	QVector<Node> children;
	SphericalConvexPolygon triangle;
	virtual void split()
	{
		// Default implementation for HTM triangle more than level 0
		
	}
};

class RootNode : public Node
{
	public:
		RootNode(double amargin, int amaxObjectsPerNode, int amaxLevel) : maxObjectsPerNode(), maxLevel(amaxLevel), margin(amargin) {;}
		
		//! Insert the given object in the StelSphericalIndex.
		void insert(Node& node, const NodeElem& el, int level)
		{
			if (node.children.isEmpty())
			{
				node.elements.append(el);
				// If we have too many objects in the node, we split it
				if (level<maxLevel && node.elements.size() >= maxObjectsPerNode)
				{
					node.split();
					const QVector<NodeElem> nodeElems = node.elements;
					for (QVector<NodeElem>::ConstIterator iter = nodeElems.begin();iter != nodeElems.end(); ++iter)
					{
						insert(node, *iter, level+1);
					}
				}
			}
			else
			{
				// If we have children, store it in a sub-level
				for (QVector<Node>::iterator iter = node.children.begin(); iter!=node.children.end(); ++iter)
				{
					if (iter->triangle.contains(el.cap.n))
					{
						insert(*iter, el, level+1);
						return;
					}
				}
			}
		}

		//! Process all the objects intersecting the given region using the passed function object.
		template<class FuncObject> void processIntersectingRegions(const Node& node, const SphericalRegionP region, FuncObject func)
		{
			foreach (const NodeElem& el, node.elements)
			{
				if (region->intersects(el.obj->getRegion()))
					func(el.obj);
			}
			foreach (const Node& child, node.children)
			{
				if (region->contains(node.triangle))
					processAll(child, func);
				else if (region->intersects(node.triangle))
					processIntersectingRegions(child, region, func);
			}
		}

		//! Process all the objects intersecting the given region using the passed function object.
		template<class FuncObject> void processAll(const Node& node, FuncObject func)
		{
			foreach (const NodeElem& el, node.elements)
				func(el.obj);
			foreach (const Node& child, node.children)
				processAll(child, func);
		}
		
	private:
		//! The maximum number of objects per node.
		int maxObjectsPerNode;
		//! The maximum level of the grid. Prevents grid split into too small triangles if unecessary.
		int maxLevel;
		//! The margin in degree to add to the query region.
		double margin;
};

StelSphericalIndex::StelSphericalIndex(int maxObjPerNode) : maxObjectsPerNode(maxObjPerNode)
{
	for (int i=0;i<MAX_INDEX_LEVEL;++i)
	{
		cosRadius[i]=std::cos(M_PI/180.*180./(2^i));
		treeForRadius[i]=NULL;
	}
}

StelSphericalIndex::~StelSphericalIndex()
{
	for (int i=0;i<MAX_INDEX_LEVEL;++i)
	{
		if (treeForRadius[i]!=NULL)
			delete treeForRadius[i];
	}
}

void StelSphericalIndex::insert(StelRegionObjectP regObj)
{
	NodeElem el(regObj);
	int i;
	for (i=1;i<MAX_INDEX_LEVEL&&cosRadius[i]<el.cap.d;++i) {;}
	RootNode* node = treeForRadius[i-1];
	if (node==NULL)
		treeForRadius[i-1]=new RootNode(cosRadius[i-1], maxObjectsPerNode, i-1);
	node->insert(*node, el, 0);
}

template<class FuncObject> void StelSphericalIndex::processIntersectingRegions(const SphericalRegionP region, FuncObject func)
{
	for (int i=1;i<MAX_INDEX_LEVEL;++i)
	{
		const RootNode* node = treeForRadius[i-1];
		if (node!=NULL)
			node->processIntersectingRegions(*node, region, func);
	}
}


template<class FuncObject> void StelSphericalIndex::processAll(FuncObject func)
{
	for (int i=1;i<MAX_INDEX_LEVEL;++i)
	{
		const RootNode* node = treeForRadius[i-1];
		if (node!=NULL)
			node->processAll(*node, func);
	}
}




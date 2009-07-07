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

#include "StelSphericalIndexMultiRes.hpp"
#include <QVector>

StelSphericalIndexMultiRes::StelSphericalIndexMultiRes(int maxObjPerNode) : maxObjectsPerNode(maxObjPerNode)
{
	for (int i=0;i<MAX_INDEX_LEVEL;++i)
	{
		cosRadius[i]=std::cos(M_PI/180.*180./(2^i));
		treeForRadius[i]=NULL;
	}
}

StelSphericalIndexMultiRes::~StelSphericalIndexMultiRes()
{
	for (int i=0;i<MAX_INDEX_LEVEL;++i)
	{
		if (treeForRadius[i]!=NULL)
			delete treeForRadius[i];
	}
}

void StelSphericalIndexMultiRes::insert(StelRegionObjectP regObj)
{
	NodeElem el(regObj);
	int i;
	for (i=1;i<MAX_INDEX_LEVEL&&cosRadius[i]<el.cap.d;++i) {;}
	RootNode* node = treeForRadius[i-1];
	if (node==NULL)
	{
		node=new RootNode(cosRadius[i-1], maxObjectsPerNode, i-1);
		treeForRadius[i-1]=node;
	}
	node->insert(el, 0);
}



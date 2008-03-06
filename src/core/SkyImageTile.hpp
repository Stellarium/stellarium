/*
 * Copyright (C) 2008 Fabien Chereau
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

#ifndef SKYIMAGETILE_H_
#define SKYIMAGETILE_H_

#include "STextureTypes.hpp"
#include "SphereGeometry.hpp"
#include <QList>

//! Base class for any astro image with a fixed position
class SkyImageTile
{
public:
	//! Constructor
	SkyImageTile();

	//! Desctructor
	virtual ~SkyImageTile();
	
	//! Draw the image on the screen.
	virtual void draw(StelCore* core);
	
private:
	// Image credits
	QString credits;
	
	// Info URL for the image
	QString infoUrl;
	
	// URL where the image is located
	QString url;
	
	// Minimum resolution of the data of the texture in degree/pixel
	float minResolution;
	
	// The texture of the tile
	STextureSP tex;
	
	// Direction of the vertices of each polygons in ICRS frame
	QList<StelGeom::ConvexPolygon> skyConvexPolygons;
	
	// Positions of the vertex of each convex polygons in texture space
	QList< QList<Vec3f> > textureCoords;
	
	// The list of all the subtiles for this tile
	QList<SkyImageTile*> subTiles;
};

#endif /*SKYIMAGETILE_H_*/

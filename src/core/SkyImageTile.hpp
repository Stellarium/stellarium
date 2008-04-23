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
#include <QString>
#include <QStringList>
#include <QVariantMap>

#define DEBUG_SKYIMAGE_TILE

class QIODevice;
class StelCore;

//! Base class for any astro image with a fixed position
class SkyImageTile : public QObject
{
	Q_OBJECT;
public:
	//! Constructor
	SkyImageTile(const QString& url, SkyImageTile* parent=NULL);
	//! Constructor
	SkyImageTile(const QVariantMap& map, SkyImageTile* parent);
	
	//! Destructor
	~SkyImageTile();

	//! Draw the image on the screen.
	void draw(StelCore* core);
	
private slots:
	//! Called when the download for the JSON file terminated
	//! @param id the identifier of the request.
	//! @param error true if an error occurred during the processing; otherwise false
	void downloadFinished(int id, bool error);
		
private:
	//! Load the tile information from a JSON file
	void loadFromJSON(QIODevice& input);
	
	//! Load the tile from a valid QVariantMap
	void loadFromQVariantMap(const QVariantMap& map);
	
	//! Return the list of tiles which should be drawn.
	//! @param result a map containing resolution, pointer to the tiles
	void getTilesToDraw(QMultiMap<double, SkyImageTile*>& result, StelCore* core, const StelGeom::ConvexPolygon& viewPortPoly, bool recheckIntersect=true);
	
	//! Draw the image on the screen.
	void drawTile(StelCore* core);
	
	//! Delete all the subtiles which were not displayed since more than lastDrawTrigger seconds
	void deleteUnusedTiles(double lastDrawTrigger=5.);
	
	//! Delete the texture from memory. It will be reloaded automatically if needed
	void deleteTexture() {tex.reset();}
	
	//! Return the last time the tile content was actually drawn
	double getLastTimeDraw() const {return lastTimeDraw;}
	
	//! Return the base URL prefixed to relative URL
	QString getBaseUrl() const {return baseUrl;}
	
	//! Return the image URL as written in the JSON file
	QString getImageUrl() const  {return imageUrl;}
	
	//! Return the minimum resolution
	double getMinResolution() const {return minResolution;}
	
	// Image credits
	QString credits;
	
	// Info URL for the image
	QString infoUrl;
	
	// URL where the image is located
	QString imageUrl;
	
	// Base URL to prefix to relative URL
	QString baseUrl;
	
	// Minimum resolution of the data of the texture in degree/pixel
	float minResolution;
	
	// The image luminance in cd/m^2
	float luminance;
	
	// Whether the texture must be blended
	bool alphaBlend;
	
	// The texture of the tile
	STextureSP tex;
	// True if the tile is just a list of other tiles without texture for itself
	bool noTexture;
	
	// Direction of the vertices of each polygons in ICRS frame
	QList<StelGeom::ConvexPolygon> skyConvexPolygons;
	
	// Positions of the vertex of each convex polygons in texture space
	QList< QList<Vec2f> > textureCoords;
	
	// The list of all the subTiles URL for this tile
	QStringList subTilesUrls;
	
	// The list of all the created subtiles for this tile
	QList<SkyImageTile*> subTiles;
	
	// Set to true if an error occured with this tile and it should not be displayed
	bool errorOccured;
	
	// Used to download remote JSON files if needed
	class QHttp* http;
	
	// true if the JSON descriptor file is currently downloading
	bool downloading;
	int downloadId;
	
	// Store the time of the last draw
	double lastTimeDraw;
	
#ifdef DEBUG_SKYIMAGE_TILE
	static class SFont* debugFont;
#endif
};

#endif /*SKYIMAGETILE_H_*/

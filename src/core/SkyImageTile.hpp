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

#ifndef _SKYIMAGETILE_HPP_
#define _SKYIMAGETILE_HPP_

#include "STextureTypes.hpp"
#include "SphereGeometry.hpp"
#include <QTimeLine>
#include <QList>
#include <QString>
#include <QStringList>
#include <QVariantMap>

// #define DEBUG_SKYIMAGE_TILE

class QIODevice;
class StelCore;

//! Contain all the credits for a given server hosting the data
class ServerCredits
{
public:
	//! Very short credit to display in the loading bar
	QString shortCredits;
	
	//! Full credits
	QString fullCredits;
	
	//! The URL where to get more info about the server
	QString infoURL;
};

//! Contains all the credits for the creator of the image collection
class DataSetCredits
{
public:
	//! Very short credit to display in the loading bar
	QString shortCredits;
	
	//! Full credits
	QString fullCredits;
	
	//! The URL where to get more info about the data collection
	QString infoURL;
};

//! Base class for any astro image with a fixed position
class SkyImageTile : public QObject
{
	Q_OBJECT;
	
	friend class JsonLoadThread;
	
public:
	//! Constructor
	SkyImageTile(const QString& url, SkyImageTile* parent=NULL);
	//! Constructor
	SkyImageTile(const QVariantMap& map, SkyImageTile* parent);
	
	//! Destructor
	~SkyImageTile();

	//! Draw the image on the screen.
	void draw(StelCore* core);
	
	//! Return the dataset credits to use in the progress bar
	DataSetCredits getDataSetCredits() const {return dataSetCredits;}
	
	//! Return the server credits to use in the progress bar
	ServerCredits getServerCredits() const {return serverCredits;}
	
	//! Return the short name for this image to be used in the loading bar
	QString getShortName() const {return shortName;}
	
signals:
	//! Emitted when loading of data started or stopped
	//! @param b true if data loading started, false if finished
	void loadingStateChanged(bool b);
	
	//! Emitted when the percentage of loading tiles/tiles to be displayed changed
	//! @param percentage the percentage of loaded data
	void percentLoadedChanged(int percentage);
		
private slots:
	//! Called when the download for the JSON file terminated
	void downloadFinished();
	
	//! Called when the JSON file is loaded
	void JsonLoadFinished();
	
private:
	//! init the SkyImageTile
	void initCtor();
			
	//! Load the tile information from a JSON file
	static QVariantMap loadFromJSON(QIODevice& input, bool compressed=false);
	
	//! Load the tile from a valid QVariantMap
	void loadFromQVariantMap(const QVariantMap& map);
	
	//! Return the list of tiles which should be drawn.
	//! @param result a map containing resolution, pointer to the tiles
	void getTilesToDraw(QMultiMap<double, SkyImageTile*>& result, StelCore* core, const StelGeom::ConvexPolygon& viewPortPoly, float limitLuminance, bool recheckIntersect=true);
	
	//! Draw the image on the screen.
	//! @return true if the tile was actually displayed
	bool drawTile(StelCore* core);
	
	//! Delete all the subtiles which were not displayed since more than lastDrawTrigger seconds
	void deleteUnusedTiles(double lastDrawTrigger=2.);
	
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
	
	//! The credits of the server where this data come from
	ServerCredits serverCredits;
	
	//! The credits for the data set
	DataSetCredits dataSetCredits;
	
	//! The very short name for this image set to be used in loading bar
	QString shortName;
	
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
	class QNetworkReply* httpReply;
	
	// true if the JSON descriptor file is currently downloading
	bool downloading;
	
	class JsonLoadThread* loadThread;
			
	// Store the time of the last draw
	double lastTimeDraw;

	// Used for smooth fade in
	QTimeLine* texFader;
	
	// The temporary map filled in a thread
	QVariantMap temporaryResultMap;
	
	void updatePercent(int tot, int numToBeLoaded);
	bool loadingState;
	int lastPercent;
	
#ifdef DEBUG_SKYIMAGE_TILE
	static class SFont* debugFont;
#endif
};

#endif // _SKYIMAGETILE_HPP_

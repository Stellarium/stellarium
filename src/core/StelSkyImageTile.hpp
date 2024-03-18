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
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */


#ifndef STELSKYIMAGETILE_HPP
#define STELSKYIMAGETILE_HPP

#include "MultiLevelJsonBase.hpp"
#include "StelSphereGeometry.hpp"
#include "StelTextureTypes.hpp"

#include <QTimeLine>

//#define DEBUG_STELSKYIMAGE_TILE 1

class QIODevice;
class StelCore;
class StelPainter;

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
class StelSkyImageTile : public MultiLevelJsonBase
{
	Q_OBJECT

	friend class StelSkyLayerMgr;

public:
	//! Default constructor
	StelSkyImageTile();

	//! Constructor
	StelSkyImageTile(const QString& url, StelSkyImageTile* parent=Q_NULLPTR, int decimateBy=1);
	//! Constructor
	StelSkyImageTile(const QVariantMap& map, StelSkyImageTile* parent, int decimateBy=1);

	//! Destructor
	~StelSkyImageTile() override;

	//! Draw the image on the screen.
	void draw(StelCore* core, StelPainter& sPainter, float opacity=1.) override;

	//! Return the dataset credits to use in the progress bar
	DataSetCredits getDataSetCredits() const {return dataSetCredits;}

	//! Return the server credits to use in the progress bar
	ServerCredits getServerCredits() const {return serverCredits;}

	//! Return true if the tile is fully loaded and can be displayed
	bool isReadyToDisplay() const;

	//! Convert the image information to a map following the JSON structure.
	//! It can be saved as JSON using the StelJsonParser methods.
	QVariantMap toQVariantMap() const;

	//! Return the absolute path/URL to the image file
	QString getAbsoluteImageURI() const {return absoluteImageURI;}

	//! Return an HTML description of the image to be displayed in the GUI.
	QString getLayerDescriptionHtml() const override {return htmlDescription;}

protected:
	//! Reimplement the abstract method.
	//! Load the tile from a valid QVariantMap.
	void loadFromQVariantMap(const QVariantMap& map) override;

	//! The credits of the server where this data come from
	ServerCredits serverCredits;

	//! The credits for the data set
	DataSetCredits dataSetCredits;

	//! URL where the image is located
	QString absoluteImageURI;

	//! The image luminance in cd/m^2
	float luminance;

	//! Whether the texture must be blended
	bool alphaBlend;

	//! True if the tile is just a list of other tiles without texture for itself
	bool noTexture;

	//! list of all the polygons.
	QList<SphericalRegionP> skyConvexPolygons;

	//! The texture of the tile
	StelTextureSP tex;

	//! Minimum resolution of the data of the texture in degree/pixel
	float minResolution;

	//! Allow some images to be shown only after this date, e.g. Supernova remnants.
	double birthJD;

	//! Should usually be true. Allow disabling observation of aberration correction for script-loaded SkyImages.
	bool withAberration;

private:
	//! init the StelSkyImageTile
	void initCtor();

	//! Return the list of tiles which should be drawn.
	//! @param result a map containing resolution, pointer to the tiles
	void getTilesToDraw(QMultiMap<double, StelSkyImageTile*>& result, StelCore* core, const SphericalRegionP& viewPortPoly, float limitLuminance, bool recheckIntersect=true);

	//! Draw the image on the screen.
	//! @return true if the tile was actually displayed
	bool drawTile(StelCore* core, StelPainter& sPainter, const Vec3d &vel);

	//! Return the minimum resolution
	double getMinResolution() const {return static_cast<double>(minResolution);}

	//! The list of all the subTiles URL or already loaded JSON map for this tile
	QVariantList subTilesUrls;

	// Used for smooth fade in
	QTimeLine* texFader;

	QString htmlDescription;

	int decimation; //!> Allow texture size reduction for very weak hardware. Default 1, useful 2..8.
};

#endif // STELSKYIMAGETILE_HPP

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

#ifndef STELSKYLAYERMGR_HPP
#define STELSKYLAYERMGR_HPP

#include "StelModule.hpp"
#include "StelSkyLayer.hpp"

#include <QString>
#include <QStringList>
#include <QMap>

class StelCore;
class StelSkyImageTile;

//! Manage the sky background images, including DSS and deep sky objects images.
//! Drawn after Milky Way, but before Zodiacal Light.
//! It is not intended to be used to "overdraw" images into the foreground without a concept of surface magnitude.

class StelSkyLayerMgr : public StelModule
{
	Q_OBJECT
	Q_PROPERTY(bool flagShow READ getFlagShow WRITE setFlagShow NOTIFY flagShowChanged)

public:
	StelSkyLayerMgr();
	~StelSkyLayerMgr();

	///////////////////////////////////////////////////////////////////////////
	// Methods defined in the StelModule class
	//! Initialize
	virtual void init();

	//! Draws sky background
	virtual void draw(StelCore* core);

	//! Update state which is time dependent.
	virtual void update(double) {;}

	//! Determines the order in which the various modules are drawn.
	virtual double getCallOrder(StelModuleActionName actionName) const;

	///////////////////////////////////////////////////////////////////////////
	// Other specific methods
	//! Add a new layer.
	//! @param l the layer to insert.
	//! @param keyHint a hint on which key to use for later referencing the image.
	//! @param show defined whether the layer should be shown by default
	//! @return the reference key to use when accessing this layer later on.
	QString insertSkyLayer(StelSkyLayerP l, const QString& keyHint=QString(), bool show=true);

	//! Remove a layer.
	void removeSkyLayer(StelSkyLayerP l);

	//! Get the list of all the currently loaded layers.
	QMap<QString, StelSkyLayerP> getAllSkyLayers() const;

	StelSkyLayerP getSkyLayer(const QString& key) const;

	//! Get whether Sky Background should be displayed
	bool getFlagShow() const {return flagShow;}

public slots:
	///////////////////////////////////////////////////////////////////////////
	// Properties setters and getters
	//! Set whether Sky Background should be displayed
	void setFlagShow(bool b) {if (flagShow !=b) { flagShow = b; emit flagShowChanged(b);}}
	//! Load an image from a file into a quad described with 4 corner coordinates.
	//! The corners are always given in counterclockwise from top-left, also for azaltimuthal images.
	//! This should not be called directly from scripts because it is not thread safe.
	//! Instead use the similarly named function in the core scripting object.
	//! @param id a string identifier for the image
	//! @param filename the name of the image file to load.  Will be
	//! searched for using StelFileMgr, so partial names are fine.
	//! @param lon0 right ascension/longitude/azimuth of top-left corner 0 in degrees
	//! @param lat0 declination/latitude/altitude of corner 0 in degrees
	//! @param lon1 right ascension/longitude/azimuth of bottom-left corner 1 in degrees
	//! @param lat1 declination/latitude/altitude of corner 1 in degrees
	//! @param lon2 right ascension/longitude/azimuth of bottom-right corner 2 in degrees
	//! @param lat2 declination/latitude/altitude of corner 2 in degrees
	//! @param lon3 right ascension/longitude/azimuth of top-right corner 3 in degrees
	//! @param lat3 declination/latitude/altitude of corner 3 in degrees
	//! @param minRes the minimum resolution setting for the image
	//! @param maxBright the maximum brightness setting for the image
	//! @param visible initial visibility setting
	//! @param frameType Coordinate frame type
	//! @note Last argument has been added 2017-03. Use loadSkyImage(... , StelCore::FrameJ2000) for the previous behaviour!
	//! @note For frameType=AzAlt, azimuth currently is counted from South towards East.
	//! @bug Some image are not visible close to screen center, only when in the corners.
	bool loadSkyImage(const QString& id, const QString& filename,
					  double long0, double lat0,
					  double long1, double lat1,
					  double long2, double lat2,
					  double long3, double lat3,
					  double minRes, double maxBright, bool visible, StelCore::FrameType frameType=StelCore::FrameJ2000);

	//! Decide to show or not to show a layer by its ID.
	//! @param id the id of the layer whose status is to be changed.
	//! @param b the new shown value:
	//! - true means the specified image will be shown.
	//! - false means the specified image will not be shown.
	void showLayer(const QString& id, bool b);
	//! Get the current shown status of a specified image.
	//! @param id the ID of the image whose status is desired.
	//! @return the current shown status of the specified image:
	//! - true means the specified image is currently shown.
	//! - false means the specified image is currently not shown.
	bool getShowLayer(const QString& id) const;

	///////////////////////////////////////////////////////////////////////////
	// Other slots
	//! Add a new SkyImage from its URI (URL or local file name).
	//! The image is owned by the manager and will be destroyed at the end of the program
	//! or when removeSkyImage is called with the same URI
	//! @param uri the local file or the URL where the JSON image description is located.
	//! @param keyHint a hint on which key to use for later referencing the image.
	//! @param show defined whether the image should be shown by default.
	//! @return the reference key to use when accessing this image later on.
	QString insertSkyImage(const QString& uri, const QString& keyHint=QString(), bool show=true);

	//! Remove a sky layer from the list.
	//! Note: this is not thread safe, and so should not be used directly
	//! from scripts - use the similarly named function in the core
	//! scripting API object to delete SkyLayers.
	//! @param key the reference key (id) generated by insertSkyImage.
	void removeSkyLayer(const QString& key);

	//! Return the list of all the layer currently loaded.
	QStringList getAllKeys() const {return allSkyLayers.keys();}

signals:
	void flagShowChanged(bool b);

private slots:
	//! Called when loading of data started or stopped for one collection
	//! @param b true if data loading started, false if finished
	void loadingStateChanged(bool b);

	//! Called when the percentage of loading tiles/tiles to be displayed changed for one collection
	//! @param percentage the percentage of loaded data
	void percentLoadedChanged(int percentage);

	void loadCollection();

private:
	//! Store the informations needed for a graphical element layer.
	class SkyLayerElem
	{
	public:
		SkyLayerElem(StelSkyLayerP t, bool show=true);
		~SkyLayerElem();
		StelSkyLayerP layer;
		class StelProgressController* progressBar;
		bool show;
	};

	SkyLayerElem* skyLayerElemForLayer(const StelSkyLayer*);

	QString keyForLayer(const StelSkyLayer*);

	//! Map image key/layer
	QMap<QString, SkyLayerElem*> allSkyLayers;

	// Whether to draw at all
	bool flagShow;
};

#endif // STELSKYLAYERMGR_HPP

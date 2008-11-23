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

#ifndef _SKYIMAGEMGR_HPP_
#define _SKYIMAGEMGR_HPP_

#include "StelModule.hpp"
#include <QString>
#include <QStringList>
#include <QMap>

class StelCore;
class SkyImageTile;
class QProgressBar;

//! Manage the sky background images, including DSS and deep sky objects images
class SkyImageMgr : public StelModule
{
	Q_OBJECT
			
public:
	SkyImageMgr();
	~SkyImageMgr();

	///////////////////////////////////////////////////////////////////////////
	// Methods defined in the StelModule class
	//! Initialize
	virtual void init();
	
	//! Draws sky background
	virtual void draw(StelCore* core);
	
	//! Update state which is time dependent.
	virtual void update(double deltaTime) {;}
	
	//! Update i18
	virtual void updateI18n() {;}
	
	//! Determines the order in which the various modules are drawn.
	virtual double getCallOrder(StelModuleActionName actionName) const;

	///////////////////////////////////////////////////////////////////////////
	// Other specific methods
	//! Add a new sky image tile in the list of background images
	//! TODO: document params, specifially, what does externallyOwned really mean?
	QString insertSkyImage(SkyImageTile* img, bool show=true, bool externallyOwned=true);
	
	//! Remove a sky image tile from the list of background images
	void removeSkyImage(SkyImageTile* img);
	
public slots:
	///////////////////////////////////////////////////////////////////////////
	// Properties setters and getters
	//! Set whether Sky Background should be displayed
	void setFlagShow(bool b) {flagShow = b;}
	//! Get whether Sky Background should be displayed
	bool getFlagShow() const {return flagShow;}

	//! Load an image from a file.  This should not be called directly from
	//! scripts because it is not thread safe.  Instead use the simiarly 
	//! named function in the core scripting object.
	//! @param id a string identifier for the image
	//! @param filename the name of the image file to load.  Will be 
	//! searched for using StelFileMgr, so partial names are fine.
	//! @param ra0 right ascention of corner 0 in degrees
	//! @param dec0 declenation of corner 0 in degrees
	//! @param ra1 right ascention of corner 1 in degrees
	//! @param dec1 declenation of corner 1 in degrees
	//! @param ra2 right ascention of corner 2 in degrees
	//! @param dec2 declenation of corner 2 in degrees
	//! @param ra3 right ascention of corner 3 in degrees
	//! @param dec3 declenation of corner 3 in degrees
	//! @param minRes the minimum resolution setting for the image
	//! @param maxBright the maximum brightness setting for the image
	//! @param visiable initial visibility setting
	bool loadSkyImage(const QString& id, const QString& filename, 
	                  double ra0, double dec0, 
	                  double ra1, double dec1, 
	                  double ra2, double dec2,
	                  double ra3, double dec3,
	                  double minRes, double maxBright, bool visible);

	//! Decide to show or not to show an image by it's ID
	//! @param id the id of the image whose status is to be changed
	//! @param b the new shown value:
	//! - true means the specified image will be shown
	//! - false means the specified image will not be shown
	void showImage(const QString& id, bool b);
	//! Get the current shown status of a specified image
	//! @param id the ID of the image whose status is desired
	//! @return the current shown status of the specified image:
	//! - true means the specified image is currently shown
	//! - false means the specified image is currently not shown
	bool getShowImage(const QString& id);
	
	///////////////////////////////////////////////////////////////////////////
	// Other slots
	//! Add a new image from its URI (URL or local file name)
	//! The image is owned by the manager and will be destroyed at the end of the program
	//! or when removeSkyImage is called with the same URI
	//! @param uri the local file or the URL where the JSON image description is located
	//! @param show defined whether the image should be shown by default
	//! @return the reference key to use when accessing this image later on
	QString insertSkyImage(const QString& uri, bool show=true);
	
	//! Remove a sky image tile from the list of background images
	//! Note: this is not thread safe, and so should not be used directly
	//! from scripts - use the similarly named function in the core
	//! scripting API object to delete SkyImages.
	//! @param key the reference key (id) generated by insertSkyImage
	void removeSkyImage(const QString& key);
	
	//! Return the list of all the sky images currently loaded
	QStringList getAllKeys() const {return allSkyImages.keys();}

private slots:
	//! Called when loading of data started or stopped for one collection
	//! @param b true if data loading started, false if finished
	void loadingStateChanged(bool b);
	
	//! Called when the percentage of loading tiles/tiles to be displayed changed for one collection
	//! @param percentage the percentage of loaded data
	void percentLoadedChanged(int percentage);

private:
	class SkyImageMgrElem
	{
	public:
		SkyImageMgrElem(SkyImageTile* t, bool show=true, bool externallyOwned=true);
		~SkyImageMgrElem();
		SkyImageTile* tile;
		QProgressBar* progressBar;
		bool show;
		bool externallyOwned;
	};
	
	SkyImageMgrElem* skyBackgroundElemForTile(const SkyImageTile*);
	
	QString keyForTile(const SkyImageTile*);
	
	//! Map image key/image
	QMap<QString, SkyImageMgrElem*> allSkyImages;
	
	// Whether to draw at all
	bool flagShow;
};

#endif // _SKYIMAGEMGR_HPP_

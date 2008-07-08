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

#ifndef _SKYBACKGROUND_HPP_
#define _SKYBACKGROUND_HPP_

#include "StelModule.hpp"
#include <QString>
#include <QList>

class StelCore;
class SkyImageTile;
class QProgressBar;

//! Manage the sky background images, including DSS and deep sky objects images
class SkyBackground : public StelModule
{
	Q_OBJECT
			
public:
	SkyBackground();
	~SkyBackground();

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
	// Properties setters and getters
public slots:
	//! Set whether Sky Background should be displayed
	void setFlagShow(bool b) {flagShow = b;}
	//! Get whether Sky Background should be displayed
	bool getFlagShow() const {return flagShow;}
	
private slots:
	//! Called when loading of data started or stopped for one collection
	//! @param b true if data loading started, false if finished
	void loadingStateChanged(bool b);
	
	//! Called when the percentage of loading tiles/tiles to be displayed changed for one collection
	//! @param percentage the percentage of loaded data
	void percentLoadedChanged(int percentage);
	
private:
	
	class SkyBackgroundElem
	{
		public:
			SkyBackgroundElem(const QString& str);
			SkyBackgroundElem(SkyImageTile* t) : tile(t), progressBar(NULL) {;}
			~SkyBackgroundElem();
			SkyImageTile* tile;
			QProgressBar* progressBar;
	};
	
	SkyBackgroundElem* skyBackgroundElemForTile(const SkyImageTile*);
	
	void addElem(const QString& uri);
	
	//! List of the image collection to display, and associated progress bars
	QList<SkyBackgroundElem*> allSkyImages;
	
	// Whether to draw at all
	bool flagShow;
};

#endif // _SKYBACKGROUND_HPP_

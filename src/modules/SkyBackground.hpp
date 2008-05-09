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

#ifndef SKYBACKGROUND_H_
#define SKYBACKGROUND_H_

#include "StelModule.hpp"
#include <QString>

class StelCore;
class SkyImageTile;

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
	virtual double draw(StelCore* core);
	
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
	
private:
	
	// Direction of the vertices of each polygons in ICRS frame
	QList<SkyImageTile*> allSkyImages;
	
	// Whether to draw at all
	bool flagShow;
};

#endif /*SKYBACKGROUND_H_*/

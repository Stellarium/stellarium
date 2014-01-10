/*
 * Navigational Stars plug-in
 * Copyright (C) 2014 Alexander Wolf
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

#ifndef _NAVSTARS_HPP_
#define _NAVSTARS_HPP_

#include "StelObjectModule.hpp"
#include "StelObject.hpp"
#include "StelTextureTypes.hpp"
#include "StelPainter.hpp"
#include "StarMgr.hpp"
#include "QSettings"

class QPixmap;
class StelButton;
class StelPainter;

//! This is an example of a plug-in which can be dynamically loaded into stellarium
class NavStars : public StelModule
{
	Q_OBJECT
	Q_PROPERTY(bool navStarsVisible
		   READ getFlagShowNavStars
		   WRITE setFlagShowNavStars)
public:	
	NavStars();
	virtual ~NavStars();

	///////////////////////////////////////////////////////////////////////////
	// Methods defined in the StelModule class
	virtual void init();
	virtual void deinit();
	virtual void update(double) {;}
	virtual void draw(StelCore* core);
	virtual double getCallOrder(StelModuleActionName actionName) const;


public slots:

	void setFlagShowNavStars(bool b) { flagShowNavStars=b; }
	bool getFlagShowNavStars(void) { return flagShowNavStars; }


private:
	void setHintColor(void);

	StarMgr* smgr;
	QSettings* conf;
	StelTextureSP markerTexture;

	QList<int> nstar;

	Vec3f navStarColor;
	bool flagShowNavStars;
	bool flagStarName;

	QPixmap* OnIcon;
	QPixmap* OffIcon;
	QPixmap* GlowIcon;
	StelButton* toolbarButton;
};



#include <QObject>
#include "StelPluginInterface.hpp"

//! This class is used by Qt to manage a plug-in interface
class NavStarsStelPluginInterface : public QObject, public StelPluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID "stellarium.StelGuiPluginInterface/1.0")
	Q_INTERFACES(StelPluginInterface)
public:
	virtual StelModule* getStelModule() const;
	virtual StelPluginInfo getPluginInfo() const;
};

#endif // _NAVSTARS_HPP_

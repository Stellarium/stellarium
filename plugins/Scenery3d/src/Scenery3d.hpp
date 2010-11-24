/*
 * Stellarium-Collada Plug-in
 * 
 * Copyright (C) 2010 Simon Parzer, Gustav Oberwandling, Peter Neubauer
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

#ifndef _TESTPLUGIN_HPP_
#define _TESTPLUGIN_HPP_

#include "StelGui.hpp"
#include "StelModule.hpp"

#include <QString>

//! Main class of the plug-in
class Scenery3d : public StelModule
{
	Q_OBJECT

public:
	Scenery3d();
	virtual ~Scenery3d();
	
	///////////////////////////////////////////////////////////////////////////
	// Methods inherited from the StelModule class
	//! called when the plug-in is loaded.
	//! All initializations should be done here.
	virtual void init();
	//! called before the plug-in is un-loaded.
	//! Useful for stopping processes, unloading textures, etc.
	virtual void deinit();
	virtual void update(double deltaTime);
	//! draws on the view port.
	//! If a plug-in draws on the screen, it should be able to respect
	//! the night vision mode.
	virtual void draw(StelCore * core);
	
	//! Key events. stellarium-collada uses WASD for movement.
	virtual void handleKeys (class QKeyEvent *e);
	
	virtual double getCallOrder(StelModuleActionName actionName) const;
	//! called when the "configure" button in the "Plugins" tab is pressed
	virtual bool configureGui(bool show);
	
	///////////////////////////////////////////////////////////////////////////
	// Methods specific to Collada
	
public slots:

private:
    Mat4f projectionMatrix;
    QFont font;
    float rotation;

};


#include "fixx11h.h"
#include <QObject>
#include "StelPluginInterface.hpp"

//! This class is used by Qt to manage a plug-in interface
class Scenery3dStelPluginInterface : public QObject, public StelPluginInterface
{
	Q_OBJECT
	Q_INTERFACES(StelPluginInterface)
public:
	virtual StelModule* getStelModule() const;
	virtual StelPluginInfo getPluginInfo() const;
};

#endif /*_COLLADA_HPP_*/

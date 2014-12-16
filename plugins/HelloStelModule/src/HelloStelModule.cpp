/*
 * Copyright (C) 2007 Fabien Chereau
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

#include "StelProjector.hpp"
#include "StelPainter.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelLocaleMgr.hpp"
#include "StelModuleMgr.hpp"
#include "HelloStelModule.hpp"

#include <QDebug>

/*************************************************************************
 This method is the one called automatically by the StelModuleMgr just 
 after loading the dynamic library
*************************************************************************/
StelModule* HelloStelModuleStelPluginInterface::getStelModule() const
{
	return new HelloStelModule();
}

StelPluginInfo HelloStelModuleStelPluginInterface::getPluginInfo() const
{
	StelPluginInfo info;
	info.id = "HelloStelModule";
	info.displayedName = "Hello World test plugin";
	info.authors = "Stellarium team";
	info.contact = "www.stellarium.org";
	info.description = "An minimal plugin example.";
	return info;
}

/*************************************************************************
 Constructor
*************************************************************************/
HelloStelModule::HelloStelModule()
{
	setObjectName("HelloStelModule");
	font.setPixelSize(25);
}

/*************************************************************************
 Destructor
*************************************************************************/
HelloStelModule::~HelloStelModule()
{
}

/*************************************************************************
 Reimplementation of the getCallOrder method
*************************************************************************/
double HelloStelModule::getCallOrder(StelModuleActionName actionName) const
{
	if (actionName==StelModule::ActionDraw)
		return StelApp::getInstance().getModuleMgr().getModule("NebulaMgr")->getCallOrder(actionName)+10.;
	return 0;
}


/*************************************************************************
 Init our module
*************************************************************************/
void HelloStelModule::init()
{
	qDebug() << "init called for HelloStelModule";
}

/*************************************************************************
 Draw our module. This should print "Hello world!" in the main window
*************************************************************************/
void HelloStelModule::draw(StelCore* core)
{
	StelPainter painter(core->getProjection2d());
	painter.setColor(1,1,1,1);
	painter.setFont(font);
	painter.drawText(300, 300, "Hello World!");
}


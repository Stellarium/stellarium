/*
 * Copyright (C) 2011 Alexander Wolf
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

#include "StelProjector.hpp"
#include "StelPainter.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelLocaleMgr.hpp"
#include "StelModuleMgr.hpp"
#include "SNe.hpp"

#include <QDebug>

/*************************************************************************
 This method is the one called automatically by the StelModuleMgr just 
 after loading the dynamic library
*************************************************************************/
StelModule* SNeStelPluginInterface::getStelModule() const
{
	return new SNe();
}

StelPluginInfo SNeStelPluginInterface::getPluginInfo() const
{
	StelPluginInfo info;
	info.id = "SNe";
	info.displayedName = q_("Historical supernova");
	info.authors = "Alexander Wolf";
	info.contact = "alex.v.wolf@gmail.com";
	info.description = q_("The plugin for visualization of some historical supernovaes.");
	return info;
}

Q_EXPORT_PLUGIN2(SNe, SNeStelPluginInterface)


/*************************************************************************
 Constructor
*************************************************************************/
SNe::SNe()
{
	setObjectName("SNe");
	font.setPixelSize(25);
}

/*************************************************************************
 Destructor
*************************************************************************/
SNe::~SNe()
{
}

/*************************************************************************
 Reimplementation of the getCallOrder method
*************************************************************************/
double SNe::getCallOrder(StelModuleActionName actionName) const
{
	if (actionName==StelModule::ActionDraw)
		return StelApp::getInstance().getModuleMgr().getModule("NebulaMgr")->getCallOrder(actionName)+10.;
	return 0;
}


/*************************************************************************
 Init our module
*************************************************************************/
void SNe::init()
{
	qDebug() << "init called for HelloStelModule";
}

/*************************************************************************
 Draw our module. This should print "Hello world!" in the main window
*************************************************************************/
void SNe::draw(StelCore* core)
{
	StelPainter painter(core->getProjection2d());
	painter.setColor(1,1,1,1);
	painter.setFont(font);
	painter.drawText(300, 300, "Hello World!");
}


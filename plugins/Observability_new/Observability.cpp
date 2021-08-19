/*
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

#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelProjector.hpp"
#include "StelPainter.hpp"
#include "StelLocaleMgr.hpp"
#include "StelModuleMgr.hpp"

#include "DynamicPluginTemplate.hpp"
#include "DynamicPluginTemplateWindow.hpp"

#include <QDebug>

/*************************************************************************
 This method is the one called automatically by the StelModuleMgr just 
 after loading the dynamic library
*************************************************************************/
StelModule* Observability_NewInterface::getStelModule() const
{
	return new Observability_New();
}

StelPluginInfo Observability_NewInterface::getPluginInfo() const
{
	StelPluginInfo info;
	info.id = "Observability_New";
	info.displayedName = N_("Observability new");
	info.authors = "Kirill Snezhko, Alexander Wolf";
	info.contact = "stellarium@googlegroups.com";
	info.description = N_("Startpoint for the dynamic plugin development.");    
	info.acknowledgements = N_("Optional acknowledgements...");
	info.version = OBSERVABILITY_NEW_VERSION;
	info.license = OBSERVABILITY_NEW_LICENSE;
	return info;
}

/*************************************************************************
 Constructor
*************************************************************************/
Observability_New::Observability_New()
{
	setObjectName("Observability_New");
	mainWindow = new Observability_NewWindow();
	font.setPixelSize(26);
}

/*************************************************************************
 Destructor
*************************************************************************/
Observability_New::~Observability_New()
{
}

/*************************************************************************
 Reimplementation of the getCallOrder method
*************************************************************************/
double Observability_New::getCallOrder(StelModuleActionName actionName) const
{
	if (actionName==StelModule::ActionDraw)
		return StelApp::getInstance().getModuleMgr().getModule("LandscapeMgr")->getCallOrder(actionName)+10.;
	return 0.;
}

bool Observability_New::configureGui(bool show)
{
	if (show)
		mainWindow->setVisible(true);

	return true;
}

/*************************************************************************
 Init our module
*************************************************************************/
void Observability_New::init()
{
	qDebug() << "init called for Observability_New";
}

void Observability_New::deinit()
{
	delete mainWindow;
}

/*************************************************************************
 Draw our module. This should print "Hello world!" in the main window
*************************************************************************/
void Observability_New::draw(StelCore* core)
{
	StelPainter painter(core->getProjection2d());
	painter.setColor(1,1,1,1);
	painter.setFont(font);
	painter.drawText(300, 300, "Hello, Dynamic World!");
}
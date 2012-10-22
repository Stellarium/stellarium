/*
 * Copyright (C) 2012 Alexander Wolf
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
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelLocaleMgr.hpp"
#include "StelModuleMgr.hpp"
#include "SimpleDrawLine.hpp"
#include "StelUtils.hpp"
#include "renderer/StelRenderer.hpp"
#include "renderer/StelCircleArcRenderer.hpp"

#include <QDebug>

/*************************************************************************
 This method is the one called automatically by the StelModuleMgr just 
 after loading the dynamic library
*************************************************************************/
StelModule* SimpleDrawLineStelPluginInterface::getStelModule() const
{
	return new SimpleDrawLine();
}

StelPluginInfo SimpleDrawLineStelPluginInterface::getPluginInfo() const
{
	StelPluginInfo info;
	info.id = "SimpleDrawLine";
	info.displayedName = "Simple draw line test plugin";
	info.authors = "Stellarium team";
	info.contact = "www.stellarium.org";
	info.description = "An minimal plugin example for drawing lines.";
	return info;
}

Q_EXPORT_PLUGIN2(SimpleDrawLine, SimpleDrawLineStelPluginInterface)


/*************************************************************************
 Constructor
*************************************************************************/
SimpleDrawLine::SimpleDrawLine()
{
	setObjectName("SimpleDrawLine");
	font.setPixelSize(25);
}

/*************************************************************************
 Destructor
*************************************************************************/
SimpleDrawLine::~SimpleDrawLine()
{
}

/*************************************************************************
 Reimplementation of the getCallOrder method
*************************************************************************/
double SimpleDrawLine::getCallOrder(StelModuleActionName actionName) const
{
	if (actionName==StelModule::ActionDraw)
		return StelApp::getInstance().getModuleMgr().getModule("NebulaMgr")->getCallOrder(actionName)+10.;
	return 0;
}


/*************************************************************************
 Init our module
*************************************************************************/
void SimpleDrawLine::init()
{
	qDebug() << "init called for SimpleDrawLine";
}

/*************************************************************************
 Draw our module. This should draw line in the main window
*************************************************************************/
void SimpleDrawLine::draw(StelCore* core, StelRenderer* renderer)
{
	const StelProjectorP prj = core->getProjection(StelCore::FrameAltAz);
	Vec3d startPoint, endPoint;
	double lon1, lat1, lon2, lat2;
	lon1 = StelUtils::getDecAngle("10d");
	lat1 = StelUtils::getDecAngle("-5d");
	lon2 = StelUtils::getDecAngle("73d");
	lat2 = StelUtils::getDecAngle("80d");
	StelUtils::spheToRect(lon1,lat1,startPoint);
	StelUtils::spheToRect(lon2,lat2,endPoint);
	renderer->setGlobalColor(1.f, 0.5f, 0.5f, 1.f);
	StelCircleArcRenderer circleArcRenderer = StelCircleArcRenderer(renderer, prj);
	circleArcRenderer.drawGreatCircleArc(startPoint, endPoint);
}


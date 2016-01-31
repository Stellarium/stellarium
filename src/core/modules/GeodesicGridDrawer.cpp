/*
 * Stellarium
 * Copyright (C) 2006 Fabien Chereau
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
 
#include <QString>
#include "StelCore.hpp"
#include "StelGeodesicGridDrawer.hpp"
#include "StelProjector.hpp"
#include "StelApp.hpp"
#include "StelLocaleMgr.hpp"
#include "StelPainter.hpp"

// GZ Note. In 2016-01 I found this file unused in the project. 
// I still added the changes to avoid useless GL state changes so that it can more easily be reactivated.

StelGeodesicGridDrawer::StelGeodesicGridDrawer(int maxLevel)
{
	setObjectName("StelGeodesicGridDrawer");
	font = &StelApp::getInstance().getFontManager().getStandardFont(StelApp::getInstance().getLocaleMgr().getAppLanguage());
}

StelGeodesicGridDrawer::~StelGeodesicGridDrawer()
{
}

void StelGeodesicGridDrawer::init()
{
}


double StelGeodesicGridDrawer::draw(StelCore* core, int maxSearchLevel)
{
	const StelProjectorP prj = core->getProjection();
	StelPainter sPainter(prj);
	StelGeodesicGrid* geodesicGrid = core->getGeodesicGrid();

	const GeodesicSearchResult* geodesic_search_result = geodesicGrid->search(prj->unprojectViewport(), maxSearchLevel);
	
	StelPainter::enableTexture2d(false); // glDisable(GL_TEXTURE_2D);
	StelPainter::enableBlend(true);      //glEnable(GL_BLEND);	
	StelPainter::blendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); 
    // glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // Normal transparency mode
	core->setCurrentFrame(StelCore::FrameJ2000);	// set 2D coordinate
	sPainter.setColor(0.2,0.3,0.2);
	
	int lev = (int)(7./pow(prj->getFov(), 0.4))+2;
	if (lev>geodesicGrid->getMaxLevel()) 
		lev = geodesicGrid->getMaxLevel();

	lev = maxSearchLevel;
	Vec3d win1, win2;
	int index;
	Vec3d v0, v1, v2;
	{
		GeodesicSearchInsideIterator it1(*geodesic_search_result, lev);
		while((index = it1.next()) >= 0)
		{
			Vec3d center(0);
			geodesicGrid->getTriangleCorners(lev, index, v0, v1, v2);
			prj->project(v0, win1);
			prj->project(v1, win2);
			center += win1;
			sPainter.drawLine2d(win1[0],win1[1], win2[0],win2[1]);
			prj->project(v1, win1);
			prj->project(v2, win2);
			sPainter.drawLine2d(win1[0],win1[1], win2[0],win2[1]);
			center += win1;
			prj->project(v2, win1);
			prj->project(v0, win2);
			sPainter.drawLine2d(win1[0],win1[1], win2[0],win2[1]);
			center += win1;
			center*=0.33333;
			QString str = QString("%1 (%2)").arg(index)
			                                .arg(geodesicGrid->getPartnerTriangle(lev, index));
			// GZ Why do you enable textures to draw text?
			StelPainter::enableTexture2d(true); //glEnable(GL_TEXTURE_2D);
				prj->drawText(font,center[0]-6, center[1]+6, str);
				StelPainter::enableTexture2d(false); // glDisable(GL_TEXTURE_2D);
		}
	}
	GeodesicSearchBorderIterator it1(*geodesic_search_result, lev);
	while((index = it1.next()) >= 0)
	{
		Vec3d center(0);
		geodesicGrid->getTriangleCorners(lev, index, v0, v1, v2);
		prj->project(v0, win1);
		prj->project(v1, win2);
		center += win1;
		sPainter.drawLine2d(win1[0],win1[1], win2[0],win2[1]);
		prj->project(v1, win1);
		prj->project(v2, win2);
		sPainter.drawLine2d(win1[0],win1[1], win2[0],win2[1]);
		center += win1;
		prj->project(v2, win1);
		prj->project(v0, win2);
		sPainter.drawLine2d(win1[0],win1[1], win2[0],win2[1]);
		center += win1;
		center*=0.33333;
		QString str = QString("%1 (%2)").arg(index)
		                                .arg(geodesicGrid->getPartnerTriangle(lev, index));
		StelPainter::enableTexture2d(true); //glEnable(GL_TEXTURE_2D);
			prj->drawText(font,center[0]-6, center[1]+6, str);
		StelPainter::enableTexture2d(false); //glDisable(GL_TEXTURE_2D);
	}
	
	return 0.;
}

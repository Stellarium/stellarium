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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */
 
#include <sstream>
#include "GeodesicGridDrawer.h"
#include "projector.h"
#include "stelfontmgr.h"
#include "stellocalemgr.h"
#include "stelapp.h"

GeodesicGridDrawer::GeodesicGridDrawer(int maxLevel)
{
	grid = new GeodesicGrid(maxLevel);
	searchResult = new GeodesicSearchResult(*grid);
	font = &StelApp::getInstance().getFontManager().getStandardFont(StelApp::getInstance().getLocaleMgr().getAppLanguage());
}

GeodesicGridDrawer::~GeodesicGridDrawer()
{
	delete searchResult;
	delete grid;
}

void GeodesicGridDrawer::init(const InitParser& conf, LoadingBar& lb)
{
}


double GeodesicGridDrawer::draw(Projector *prj, const Navigator *nav, ToneReproductor *eye)
{
	glDisable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);	
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // Normal transparency mode
	prj->set_orthographic_projection();	// set 2D coordinate
	glColor4f(0.2,0.3,0.2,1);
	
	int lev = (int)(7./pow(prj->get_fov(), 0.4))+2;
	if (lev>grid->getMaxLevel()) lev = grid->getMaxLevel();
	Vec3d e0, e1, e2, e3;
	Vec3d win1, win2;
	prj->unproject_j2000(0,0,e0);
	prj->unproject_j2000(prj->getViewportWidth(),0,e1);
	prj->unproject_j2000(prj->getViewportWidth(),prj->getViewportHeight(),e2);
	prj->unproject_j2000(0,prj->getViewportHeight(),e3);
	searchResult->search(e0, e3, e2, e1, lev);
	int index;
	Vec3d v0, v1, v2;
	{
	GeodesicSearchInsideIterator it1(*searchResult, lev);
	while((index = it1.next()) >= 0)
	{
		Vec3d center;
		grid->getTriangleCorners(lev, index, v0, v1, v2);
		prj->project_j2000(v0, win1);
		prj->project_j2000(v1, win2);
		center += win1;
		glBegin (GL_LINES);
			glVertex2f(win1[0],win1[1]);
			glVertex2f(win2[0],win2[1]);
        glEnd();
		prj->project_j2000(v1, win1);
		prj->project_j2000(v2, win2);
		glBegin (GL_LINES);
			glVertex2f(win1[0],win1[1]);
			glVertex2f(win2[0],win2[1]);
        glEnd();
        center += win1;
		prj->project_j2000(v2, win1);
		prj->project_j2000(v0, win2);
		glBegin (GL_LINES);
			glVertex2f(win1[0],win1[1]);
			glVertex2f(win2[0],win2[1]);
        glEnd();
        center += win1;
        center*=0.33333;
        ostringstream os;
        os << index << " (" << grid->getPartnerTriangle(lev, index) << ")";
        glEnable(GL_TEXTURE_2D);
		font->print(center[0]-6, center[1]+6, os.str());
		glDisable(GL_TEXTURE_2D);
	}
	}
	GeodesicSearchBorderIterator it1(*searchResult, lev);
	while((index = it1.next()) >= 0)
	{
		Vec3d center;
		grid->getTriangleCorners(lev, index, v0, v1, v2);
		prj->project_j2000(v0, win1);
		prj->project_j2000(v1, win2);
		center += win1;
		glBegin (GL_LINES);
			glVertex2f(win1[0],win1[1]);
			glVertex2f(win2[0],win2[1]);
        glEnd();
		prj->project_j2000(v1, win1);
		prj->project_j2000(v2, win2);
		glBegin (GL_LINES);
			glVertex2f(win1[0],win1[1]);
			glVertex2f(win2[0],win2[1]);
        glEnd();
        center += win1;
		prj->project_j2000(v2, win1);
		prj->project_j2000(v0, win2);
		glBegin (GL_LINES);
			glVertex2f(win1[0],win1[1]);
			glVertex2f(win2[0],win2[1]);
        glEnd();
        center += win1;
        center*=0.33333;
        ostringstream os;
        os << index << " (" << grid->getPartnerTriangle(lev, index) << ")";
        glEnable(GL_TEXTURE_2D);
		font->print(center[0]-6, center[1]+6, os.str());
		glDisable(GL_TEXTURE_2D);
	}
	
	prj->reset_perspective_projection();
	return 0.;
}

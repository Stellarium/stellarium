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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */
 
#include "AstroImage.hpp"
#include "Projector.hpp"
#include "StelTextureMgr.hpp"
#include "StelCore.hpp"

AstroImage::AstroImage()
{
}

AstroImage::AstroImage(STextureSP atex, const Vec3d& v0, const Vec3d& v1, const Vec3d& v2, const Vec3d& v3) : tex(atex), poly(StelGeom::ConvexPolygon(v0, v1, v2, v3))
{
}

AstroImage::AstroImage(const Vec3d& v0, const Vec3d& v1, const Vec3d& v2, const Vec3d& v3) : poly(StelGeom::ConvexPolygon(v0, v1, v2, v3))
{
}

AstroImage::~AstroImage()
{
}

void AstroImage::draw(StelCore* core)
{
	if (!tex) return;
	
	if (!tex->bind())
		return;
	
	Projector* prj = core->getProjection();
	
	glEnable(GL_TEXTURE_2D);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
	glColor4f(1.0,1.0,1.0,1.0);
	
	prj->setCurrentFrame(Projector::FrameJ2000);
	
	Vec3d win;
    glBegin(GL_TRIANGLE_STRIP);
        glTexCoord2dv(tex->getCoordinates()[0]);
		prj->project(poly[2],win); glVertex3dv(win);
		glTexCoord2dv(tex->getCoordinates()[1]);
		prj->project(poly[3],win); glVertex3dv(win);
		glTexCoord2dv(tex->getCoordinates()[2]);
		prj->project(poly[1],win); glVertex3dv(win);
		glTexCoord2dv(tex->getCoordinates()[3]);
		prj->project(poly[0],win); glVertex3dv(win);
    glEnd();
}

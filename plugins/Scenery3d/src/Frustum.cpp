/*
 * Stellarium Scenery3d Plug-in
 *
 * Copyright (C) 2011-2015 Simon Parzer, Peter Neubauer, Georg Zotti, Andrei Borza, Florian Schaukowitsch
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

#include "Frustum.hpp"
#include "GLFuncs.hpp"
#include <limits>

Frustum::Frustum()
{
	for(unsigned int i=0; i<CORNERCOUNT; i++)
	{
		corners.push_back(Vec3f(0.0f, 0.0f, 0.0f));
		drawCorners.push_back(Vec3f(0.0f, 0.0f, 0.0f));
	}

	for(unsigned int i=0; i<PLANECOUNT; i++)
	{
		planes.push_back(new Plane());
	}

	fov = 0.0f;
	aspect = 0.0f;
	zNear = 0.0f;
	zFar = 0.0f;
}

Frustum::~Frustum()
{
	for(unsigned int i=0; i<planes.size(); i++)
	{
		delete planes[i];
	}

	planes.clear();
}

const Vec3f &Frustum::getCorner(const Corner corner) const
{
	return corners[corner];
}

const Plane &Frustum::getPlane(const FrustumPlane plane) const
{
	return *planes[plane];
}

void Frustum::calcFrustum(const Vec3d &p, const Vec3d &l, const Vec3d &u)
{
	Vec3d Y = -l;
	Y.normalize();

	Vec3d X = u^Y;
	X.normalize();

	Vec3d Z = Y^X;
	Z.normalize();

	float tang = tanf((static_cast<float>(M_PI)/360.0f)*fov);
	float nh = zNear * tang;
	float nw = nh * aspect;
	float fh = zFar * tang;
	float fw = fh * aspect;

	Vec3d nc = p - Y*zNear;
	Vec3d fc = p - Y*zFar;

	Vec3d ntl = nc + Z * nh - X * nw;
	Vec3d ntr = nc + Z * nh + X * nw;
	Vec3d nbl = nc - Z * nh - X * nw;
	Vec3d nbr = nc - Z * nh + X * nw;

	Vec3d ftl = fc + Z * fh - X * fw;
	Vec3d ftr = fc + Z * fh + X * fw;
	Vec3d fbl = fc - Z * fh - X * fw;
	Vec3d fbr = fc - Z * fh + X * fw;

	corners[NTL] = ntl.toVec3f();
	corners[NTR] = ntr.toVec3f();
	corners[NBL] = nbl.toVec3f();
	corners[NBR] = nbr.toVec3f();
	corners[FTL] = ftl.toVec3f();
	corners[FTR] = ftr.toVec3f();
	corners[FBL] = fbl.toVec3f();
	corners[FBR] = fbr.toVec3f();

	planes[TOP]->setPoints(corners[NTR], corners[NTL], corners[FTL], SPolygon::CCW);
	planes[BOTTOM]->setPoints(corners[NBL], corners[NBR], corners[FBR], SPolygon::CCW);
	planes[LEFT]->setPoints(corners[NTL], corners[NBL], corners[FBL], SPolygon::CCW);
	planes[RIGHT]->setPoints(corners[NBR], corners[NTR], corners[FBR], SPolygon::CCW);
	planes[NEARP]->setPoints(corners[NTL], corners[NTR], corners[NBR], SPolygon::CCW);
	planes[FARP]->setPoints(corners[FTR], corners[FTL], corners[FBL], SPolygon::CCW);


	//reset bbox
	bbox.reset();

	for(unsigned int i=0; i<CORNERCOUNT; i++)
	{
		Vec3f curVert = corners[i];
		bbox.expand(curVert);
	}
}

int Frustum::pointInFrustum(const Vec3f& p)
{
	int result = INSIDE;
	for(int i=0; i<PLANECOUNT; i++)
	{
		if(planes[i]->isBehind(p))
		{
			return OUTSIDE;
		}
	}

	return result;
}

int Frustum::boxInFrustum(const AABBox &bbox)
{
	int result = INSIDE;
	for(unsigned int i=0; i<PLANECOUNT; i++)
	{
		if(planes[i]->isBehind(bbox.positiveVertex(planes[i]->normal)))
		{
			return OUTSIDE;
		}
	}

	return result;
}

void Frustum::saveDrawingCorners()
{
	for(unsigned int i=0; i<CORNERCOUNT; i++)
		drawCorners[i] = corners[i];

	for(unsigned int i=0; i<PLANECOUNT; i++)
		planes[i]->saveValues();

	drawBbox = bbox;
}

void Frustum::resetCorners()
{
	for(unsigned int i=0; i<CORNERCOUNT; i++)
		drawCorners[i] = Vec3f(0.0f, 0.0f, 0.0f);

	for(unsigned int i=0; i<PLANECOUNT; i++)
		planes[i]->resetValues();
}

void Frustum::drawFrustum() const
{
// Minimum to avoid trouble when building on pure OpenGL ES systems
// Not sure about ANGLE!
#if !QT_CONFIG(opengles2)

	Vec3f ntl = drawCorners[NTL];
	Vec3f ntr = drawCorners[NTR];
	Vec3f nbr = drawCorners[NBR];
	Vec3f nbl = drawCorners[NBL];
	Vec3f ftr = drawCorners[FTR];
	Vec3f ftl = drawCorners[FTL];
	Vec3f fbl = drawCorners[FBL];
	Vec3f fbr = drawCorners[FBR];

	glExtFuncs->glColor3f(0.0f, 0.0f, 1.0f);

	glExtFuncs->glBegin(GL_LINE_LOOP);
		//near plane
		glExtFuncs->glVertex3f(ntl.v[0],ntl.v[1],ntl.v[2]);
		glExtFuncs->glVertex3f(ntr.v[0],ntr.v[1],ntr.v[2]);
		glExtFuncs->glVertex3f(nbr.v[0],nbr.v[1],nbr.v[2]);
		glExtFuncs->glVertex3f(nbl.v[0],nbl.v[1],nbl.v[2]);
	glExtFuncs->glEnd();

	glExtFuncs->glBegin(GL_LINE_LOOP);
		//far plane
		glExtFuncs->glVertex3f(ftr.v[0],ftr.v[1],ftr.v[2]);
		glExtFuncs->glVertex3f(ftl.v[0],ftl.v[1],ftl.v[2]);
		glExtFuncs->glVertex3f(fbl.v[0],fbl.v[1],fbl.v[2]);
		glExtFuncs->glVertex3f(fbr.v[0],fbr.v[1],fbr.v[2]);
	glExtFuncs->glEnd();

	glExtFuncs->glBegin(GL_LINE_LOOP);
		//bottom plane
		glExtFuncs->glVertex3f(nbl.v[0],nbl.v[1],nbl.v[2]);
		glExtFuncs->glVertex3f(nbr.v[0],nbr.v[1],nbr.v[2]);
		glExtFuncs->glVertex3f(fbr.v[0],fbr.v[1],fbr.v[2]);
		glExtFuncs->glVertex3f(fbl.v[0],fbl.v[1],fbl.v[2]);
	glExtFuncs->glEnd();

	glExtFuncs->glBegin(GL_LINE_LOOP);
		//top plane
		glExtFuncs->glVertex3f(ntr.v[0],ntr.v[1],ntr.v[2]);
		glExtFuncs->glVertex3f(ntl.v[0],ntl.v[1],ntl.v[2]);
		glExtFuncs->glVertex3f(ftl.v[0],ftl.v[1],ftl.v[2]);
		glExtFuncs->glVertex3f(ftr.v[0],ftr.v[1],ftr.v[2]);
	glExtFuncs->glEnd();

	glExtFuncs->glBegin(GL_LINE_LOOP);
		//left plane
		glExtFuncs->glVertex3f(ntl.v[0],ntl.v[1],ntl.v[2]);
		glExtFuncs->glVertex3f(nbl.v[0],nbl.v[1],nbl.v[2]);
		glExtFuncs->glVertex3f(fbl.v[0],fbl.v[1],fbl.v[2]);
		glExtFuncs->glVertex3f(ftl.v[0],ftl.v[1],ftl.v[2]);
	glExtFuncs->glEnd();

	glExtFuncs->glBegin(GL_LINE_LOOP);
		// right plane
		glExtFuncs->glVertex3f(nbr.v[0],nbr.v[1],nbr.v[2]);
		glExtFuncs->glVertex3f(ntr.v[0],ntr.v[1],ntr.v[2]);
		glExtFuncs->glVertex3f(ftr.v[0],ftr.v[1],ftr.v[2]);
		glExtFuncs->glVertex3f(fbr.v[0],fbr.v[1],fbr.v[2]);
	glExtFuncs->glEnd();

	Vec3f a,b;
	glExtFuncs->glBegin(GL_LINES);
		// near
		a = (ntr + ntl + nbr + nbl) * 0.25;
		b = a + planes[NEARP]->sNormal;
		glExtFuncs->glVertex3f(a.v[0],a.v[1],a.v[2]);
		glExtFuncs->glVertex3f(b.v[0],b.v[1],b.v[2]);

		// far
		a = (ftr + ftl + fbr + fbl) * 0.25;
		b = a + planes[FARP]->sNormal;
		glExtFuncs->glVertex3f(a.v[0],a.v[1],a.v[2]);
		glExtFuncs->glVertex3f(b.v[0],b.v[1],b.v[2]);

		// left
		a = (ftl + fbl + nbl + ntl) * 0.25;
		b = a + planes[LEFT]->sNormal;
		glExtFuncs->glVertex3f(a.v[0],a.v[1],a.v[2]);
		glExtFuncs->glVertex3f(b.v[0],b.v[1],b.v[2]);

		// right
		a = (ftr + nbr + fbr + ntr) * 0.25;
		b = a + planes[RIGHT]->sNormal;
		glExtFuncs->glVertex3f(a.v[0],a.v[1],a.v[2]);
		glExtFuncs->glVertex3f(b.v[0],b.v[1],b.v[2]);

		// top
		a = (ftr + ftl + ntr + ntl) * 0.25;
		b = a + planes[TOP]->sNormal;
		glExtFuncs->glVertex3f(a.v[0],a.v[1],a.v[2]);
		glExtFuncs->glVertex3f(b.v[0],b.v[1],b.v[2]);

		// bottom
		a = (fbr + fbl + nbr + nbl) * 0.25;
		b = a + planes[BOTTOM]->sNormal;
		glExtFuncs->glVertex3f(a.v[0],a.v[1],a.v[2]);
		glExtFuncs->glVertex3f(b.v[0],b.v[1],b.v[2]);
	glExtFuncs->glEnd();

	//drawBbox.render();
#endif
}
